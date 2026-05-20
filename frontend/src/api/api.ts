import type { SceneObject, ScenePayload } from "../types/scene";

const MOCK_OBJECTS: SceneObject[] = [
  {
    id: "obj-1",
    label: "Press A",
    position: { x: 0, y: 0.5, z: 0 },
    size: { x: 1, y: 1, z: 1 },
    description: "Hydraulic stamping press, primary production line.",
    metadata: {
      status: "operational",
      lastService: "2026-03-01",
      cycleCount: 184_220,
      pressureBar: 320,
    },
    connections: [{ to_id: "obj-2" }],
    telemetryMetrics: [
      { name: "temperature", unit: "\u00b0C" },
      { name: "pressure", unit: "bar" },
    ],
  },
  {
    id: "obj-2",
    label: "Conveyor 1",
    position: { x: 3, y: 0.25, z: 0 },
    size: { x: 4, y: 0.5, z: 1 },
    description: "Main feed conveyor connecting Press A to the QC station.",
    metadata: {
      status: "operational",
      speedMps: 0.8,
      lengthM: 4,
    },
    connections: [{ to_id: "obj-6" }],
    telemetryMetrics: [
      { name: "temperature", unit: "\u00b0C" },
      { name: "pressure", unit: "bar" },
    ],
  },
  {
    id: "obj-3",
    label: "Robotic Arm",
    position: { x: -3, y: 1, z: 1 },
    size: { x: 1, y: 2, z: 1 },
    description: "6-axis pick-and-place arm for finished part handling.",
    metadata: {
      status: "idle",
      lastJob: "2026-05-19T18:42:00Z",
      payloadKg: 12,
    },
    connections: [{ to_id: "obj-1" }],
    telemetryMetrics: [
      { name: "temperature", unit: "\u00b0C" },
      { name: "pressure", unit: "bar" },
    ],
  },
  {
    id: "obj-4",
    label: "Storage Rack",
    position: { x: -2, y: 1.5, z: -3 },
    size: { x: 2, y: 3, z: 1.5 },
    description: "Buffer rack for in-progress assemblies.",
    metadata: {
      status: "operational",
      capacity: 24,
      occupied: 17,
    },
    connections: [{ to_id: "obj-3" }],
    telemetryMetrics: [],
  },
  {
    id: "obj-5",
    label: "Control Cabinet",
    position: { x: 4, y: 1, z: -3 },
    size: { x: 1.2, y: 2, z: 0.8 },
    description: "PLC and power distribution for cell 2.",
    metadata: {
      status: "operational",
      temperatureC: 38,
      voltageV: 400,
    },
    connections: [{ to_id: "obj-1" }, { to_id: "obj-2" }],
    telemetryMetrics: [
      { name: "temperature", unit: "\u00b0C" },
      { name: "voltage", unit: "V" },
    ],
  },
  {
    id: "obj-6",
    label: "Pallet",
    position: { x: 1, y: 0.15, z: 3 },
    size: { x: 1.2, y: 0.3, z: 1.2 },
    description: "EUR-pallet staging slot.",
    metadata: {
      status: "loaded",
      itemCount: 6,
    },
    connections: [],
    telemetryMetrics: [],
  },
];

function simulateLatency<T>(value: T, ms = 150): Promise<T> {
  return new Promise((resolve) => setTimeout(() => resolve(value), ms));
}

export const api = {
  baseUrl: "/api",
  useMock: true,

  async getScene(): Promise<ScenePayload> {
    if (api.useMock) {
      return simulateLatency({ objects: MOCK_OBJECTS });
    }
    const res = await fetch(`${api.baseUrl}/scene`);
    if (!res.ok) {
      throw new Error(`Failed to load scene: HTTP ${res.status}`);
    }
    return (await res.json()) as ScenePayload;
  },
};
