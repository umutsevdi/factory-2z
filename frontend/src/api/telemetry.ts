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
  kind: "mock" | "websocket";
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

const METRIC_CONFIG: Record<
  string,
  { high: number; severity: "warning" | "error"; unit: string }
> = {
  temperature: { high: 90, severity: "warning", unit: "\u00b0C" },
  pressure: { high: 6, severity: "warning", unit: "bar" },
  voltage: { high: 450, severity: "warning", unit: "V" },
};

const WARNING_DEDUP_MS = 15_000;

type MetricState = Record<string, number>;

class MockTelemetrySource implements TelemetrySource {
  private subscriptions = new Map<string, number>();
  private listeners = new Set<TelemetryListener>();
  private warningListeners = new Set<WarningListener>();
  private statusListeners = new Set<StatusListener>();
  private timer: ReturnType<typeof setInterval> | null = null;
  private state = new Map<string, MetricState>();
  private lastWarningAt = new Map<string, number>();
  private readonly intervalMs: number;
  private lastMessageAt: number | null = null;

  constructor(intervalMs = 1000) {
    this.intervalMs = intervalMs;
  }

  subscribe(objectIds: string[]): void {
    for (const id of objectIds) {
      const prev = this.subscriptions.get(id) ?? 0;
      this.subscriptions.set(id, prev + 1);
      if (!this.state.has(id)) {
        this.state.set(id, this.seedState(id));
      }
    }
    this.ensureRunning();
  }

  unsubscribe(objectIds: string[]): void {
    for (const id of objectIds) {
      const prev = this.subscriptions.get(id);
      if (prev === undefined) continue;
      if (prev <= 1) {
        this.subscriptions.delete(id);
      } else {
        this.subscriptions.set(id, prev - 1);
      }
    }
    this.maybeStop();
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
      isConnected: true,
      error: null,
      reconnectAttempt: 0,
      lastMessageAt: this.lastMessageAt,
      kind: "mock",
    };
  }

  dispose(): void {
    this.maybeStop(true);
    this.listeners.clear();
    this.warningListeners.clear();
    this.statusListeners.clear();
    this.subscriptions.clear();
  }

  private ensureRunning() {
    if (this.timer !== null) return;
    if (this.subscriptions.size === 0) return;
    this.timer = setInterval(() => this.tick(), this.intervalMs);
  }

  private maybeStop(force = false) {
    if (this.timer === null) return;
    if (!force && this.subscriptions.size > 0) return;
    clearInterval(this.timer);
    this.timer = null;
  }

  private tick() {
    const timestamp = Date.now();
    this.lastMessageAt = timestamp;
    for (const id of this.subscriptions.keys()) {
      const s = this.state.get(id)!;
      for (const key of Object.keys(s)) {
        const config = METRIC_CONFIG[key];
        if (!config) continue;
        if (key === "temperature") {
          s[key] = clamp(s[key] + (Math.random() - 0.5) * 4.0, 10, 120);
        } else if (key === "pressure") {
          s[key] = clamp(s[key] + (Math.random() - 0.5) * 0.4, 0, 10);
        } else if (key === "voltage") {
          s[key] = clamp(s[key] + (Math.random() - 0.5) * 10, 350, 480);
        }
      }

      const metrics: Record<string, number> = {};
      for (const [key, value] of Object.entries(s)) {
        metrics[key] = round(value, key === "pressure" ? 3 : 2);
      }

      const sample: TelemetrySample = { timestamp, metrics };
      for (const cb of this.listeners) cb({ objectId: id, sample });

      for (const [metric, value] of Object.entries(metrics)) {
        const cfg = METRIC_CONFIG[metric];
        if (!cfg) continue;
        if (value < cfg.high) continue;
        const dedupKey = `${id}:${metric}`;
        const last = this.lastWarningAt.get(dedupKey) ?? 0;
        if (timestamp - last < WARNING_DEDUP_MS) continue;
        this.lastWarningAt.set(dedupKey, timestamp);
        const warning: WarningPayload = {
          id: `${id}-${metric}-${timestamp}`,
          timestamp,
          severity: cfg.severity,
          message: `${metric} exceeded ${cfg.high} (current ${value})`,
          objectId: id,
          metric,
          value,
          threshold: cfg.high,
        };
        for (const cb of this.warningListeners) cb(warning);
      }
    }
    this.notifyStatus();
  }

  private notifyStatus() {
    const s = this.getStatus();
    for (const cb of this.statusListeners) cb(s);
  }

  private seedState(id: string): MetricState {
    let h = 0;
    for (let i = 0; i < id.length; i++) {
      h = (h * 31 + id.charCodeAt(i)) >>> 0;
    }
    const state: MetricState = {};
    for (const metric of Object.keys(METRIC_CONFIG)) {
      if (metric === "temperature") {
        const offset = (h % 60) - 30;
        state[metric] = 60 + offset;
      } else if (metric === "pressure") {
        const offset = ((h >>> 5) % 60) / 10 - 2;
        state[metric] = clamp(3 + offset, 0.5, 6.5);
      } else if (metric === "voltage") {
        const offset = ((h >>> 3) % 80) - 40;
        state[metric] = 400 + offset;
      }
    }
    return state;
  }
}

function clamp(v: number, lo: number, hi: number) {
  return Math.max(lo, Math.min(hi, v));
}
function round(v: number, digits: number) {
  const m = 10 ** digits;
  return Math.round(v * m) / m;
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

const USE_MOCK = true;
const TELEMETRY_WS_URL = "ws://localhost:8080/telemetry";

let instance: TelemetrySource | null = null;

export function getTelemetrySource(): TelemetrySource {
  if (instance) return instance;
  instance = USE_MOCK
    ? new MockTelemetrySource(1000)
    : new WebSocketTelemetrySource(TELEMETRY_WS_URL);
  return instance;
}
