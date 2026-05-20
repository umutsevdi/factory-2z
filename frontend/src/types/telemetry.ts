export interface TelemetrySample {
  timestamp: number;
  metrics: Record<string, number>;
}

export type WarningSeverity = "info" | "warning" | "error";

export interface WarningPayload {
  id: string;
  timestamp: number;
  severity: WarningSeverity;
  message: string;
  objectId?: string;
  metric?: string;
  value?: number;
  threshold?: number;
}

export interface SubscribeMessage {
  type: "subscribe";
  objectIds: string[];
}

export interface UnsubscribeMessage {
  type: "unsubscribe";
  objectIds: string[];
}

export interface TelemetryMessage {
  type: "telemetry";
  timestamp: string;
  objectId: string;
  metrics: Record<string, number>;
}

export interface WarningMessage {
  type: "warning";
  id: string;
  timestamp: string;
  severity: WarningSeverity;
  message: string;
  objectId?: string;
  metric?: string;
  value?: number;
  threshold?: number;
}

export type ClientMessage = SubscribeMessage | UnsubscribeMessage;
export type ServerMessage = TelemetryMessage | WarningMessage;
