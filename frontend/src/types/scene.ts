export interface Vec3 {
  x: number;
  y: number;
  z: number;
}

export interface TelemetryMetric {
  name: string;
  unit: string;
}

export interface Connection {
  to_id: string;
}

export interface SceneObject {
  id: string;
  label: string;
  position: Vec3;
  size: Vec3;
  rotation?: Vec3;
  description: string;
  metadata: Record<string, string | number>;
  connections: Connection[];
  telemetryMetrics: TelemetryMetric[];
}

export interface ScenePayload {
  objects: SceneObject[];
}
