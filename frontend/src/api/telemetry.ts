import type {
  ClientMessage,
  ServerMessage,
  TelemetrySample,
  WarningPayload,
} from "../types/telemetry";

export type TelemetryListener = (msg: {
  objectId: string;
  sample: TelemetrySample;
}) => void;

export type WarningListener = (w: WarningPayload) => void;

export interface SourceStatus {
  isConnected: boolean;
  error: string | null;
  reconnectAttempt: number;
  lastMessageAt: number | null;
  kind: "websocket";
}

export type StatusListener = (status: SourceStatus) => void;

export interface TelemetrySource {
  subscribe(objectIds: string[]): void;
  unsubscribe(objectIds: string[]): void;
  onMessage(listener: TelemetryListener): () => void;
  onWarning(listener: WarningListener): () => void;
  onStatus(listener: StatusListener): () => void;
  getStatus(): SourceStatus;
  dispose(): void;
}

class WebSocketTelemetrySource implements TelemetrySource {
  private socket: WebSocket | null = null;
  private listeners = new Set<TelemetryListener>();
  private warningListeners = new Set<WarningListener>();
  private statusListeners = new Set<StatusListener>();
  private subscriptions = new Map<string, number>();
  private isConnected = false;
  private error: string | null = null;
  private reconnectAttempt = 0;
  private reconnectTimer: ReturnType<typeof setTimeout> | null = null;
  private lastMessageAt: number | null = null;
  private readonly url: string;

  constructor(url: string) {
    this.url = url;
    this.connect();
  }

  subscribe(objectIds: string[]): void {
    const newIds: string[] = [];
    for (const id of objectIds) {
      const prev = this.subscriptions.get(id) ?? 0;
      if (prev === 0) newIds.push(id);
      this.subscriptions.set(id, prev + 1);
    }
    if (newIds.length > 0) {
      this.send({ type: "subscribe", objectIds: newIds });
    }
  }

  unsubscribe(objectIds: string[]): void {
    const removedIds: string[] = [];
    for (const id of objectIds) {
      const prev = this.subscriptions.get(id);
      if (prev === undefined) continue;
      if (prev <= 1) {
        this.subscriptions.delete(id);
        removedIds.push(id);
      } else {
        this.subscriptions.set(id, prev - 1);
      }
    }
    if (removedIds.length > 0) {
      this.send({ type: "unsubscribe", objectIds: removedIds });
    }
  }

  onMessage(listener: TelemetryListener): () => void {
    this.listeners.add(listener);
    return () => {
      this.listeners.delete(listener);
    };
  }

  onWarning(listener: WarningListener): () => void {
    this.warningListeners.add(listener);
    return () => {
      this.warningListeners.delete(listener);
    };
  }

  onStatus(listener: StatusListener): () => void {
    this.statusListeners.add(listener);
    listener(this.getStatus());
    return () => {
      this.statusListeners.delete(listener);
    };
  }

  getStatus(): SourceStatus {
    return {
      isConnected: this.isConnected,
      error: this.error,
      reconnectAttempt: this.reconnectAttempt,
      lastMessageAt: this.lastMessageAt,
      kind: "websocket",
    };
  }

  dispose(): void {
    if (this.reconnectTimer) clearTimeout(this.reconnectTimer);
    this.reconnectTimer = null;
    this.socket?.close();
    this.socket = null;
    this.listeners.clear();
    this.warningListeners.clear();
    this.statusListeners.clear();
  }

  private connect() {
    try {
      this.socket = new WebSocket(this.url);
      this.socket.onopen = () => {
        this.reconnectAttempt = 0;
        this.setStatus(true, null);
        const ids = [...this.subscriptions.keys()];
        if (ids.length > 0) this.send({ type: "subscribe", objectIds: ids });
      };
      this.socket.onmessage = (ev) => {
        this.lastMessageAt = Date.now();
        try {
          const msg = JSON.parse(ev.data) as ServerMessage;
          if (msg.type === "telemetry") {
            const sample: TelemetrySample = {
              timestamp: Date.parse(msg.timestamp),
              metrics: msg.metrics,
            };
            for (const cb of this.listeners)
              cb({ objectId: msg.objectId, sample });
          } else if (msg.type === "warning") {
            const w: WarningPayload = {
              id: msg.id,
              timestamp: Date.parse(msg.timestamp),
              severity: msg.severity,
              message: msg.message,
              objectId: msg.objectId,
              metric: msg.metric,
              value: msg.value,
              threshold: msg.threshold,
            };
            for (const cb of this.warningListeners) cb(w);
          }
        } catch (e) {
          console.error("Bad server payload", e);
        }
        this.notifyStatus();
      };
      this.socket.onerror = () => this.setStatus(false, "WebSocket error");
      this.socket.onclose = () => {
        this.setStatus(false, this.error);
        this.scheduleReconnect();
      };
    } catch (e) {
      this.setStatus(false, e instanceof Error ? e.message : String(e));
      this.scheduleReconnect();
    }
  }

  private scheduleReconnect() {
    if (this.reconnectTimer) return;
    this.reconnectAttempt += 1;
    const base = Math.min(30_000, 1_000 * 2 ** (this.reconnectAttempt - 1));
    const jitter = Math.random() * 250;
    const delay = base + jitter;
    this.notifyStatus();
    this.reconnectTimer = setTimeout(() => {
      this.reconnectTimer = null;
      this.connect();
    }, delay);
  }

  private send(msg: ClientMessage) {
    if (this.socket && this.socket.readyState === WebSocket.OPEN) {
      this.socket.send(JSON.stringify(msg));
    }
  }

  private setStatus(isConnected: boolean, error: string | null) {
    this.isConnected = isConnected;
    this.error = error;
    this.notifyStatus();
  }

  private notifyStatus() {
    const status = this.getStatus();
    for (const cb of this.statusListeners) cb(status);
  }
}

// Derive the WebSocket URL from the page origin so it transparently works
// in dev (via Vite's `/telemetry` WS proxy) and in prod (behind a reverse
// proxy that forwards /telemetry to the backend).
const TELEMETRY_WS_URL = (() => {
  const proto = window.location.protocol === "https:" ? "wss" : "ws";
  return `${proto}://${window.location.host}/telemetry`;
})();

let instance: WebSocketTelemetrySource | null = null;

export function getTelemetrySource(): TelemetrySource {
  if (instance) return instance;
  instance = new WebSocketTelemetrySource(TELEMETRY_WS_URL);
  return instance;
}
