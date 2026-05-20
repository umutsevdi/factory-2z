import { useMemo } from "react";
import { useTelemetry } from "../hooks/useTelemetry";
import { MetricChart } from "./MetricChart";
import { sceneObjectsStore } from "../state/sceneObjectsStore";
import { theme } from "../theme";

interface ObjectInfo {
  id: string;
  label: string;
}

interface Props {
  objects: ObjectInfo[];
}

const COLORS = theme.chartPalette;

export function TelemetryPanel({ objects }: Props) {
  const ids = useMemo(() => objects.map((o) => o.id), [objects]);
  const { samples } = useTelemetry(ids);

  const series = useMemo(
    () =>
      objects.map((o, i) => ({
        id: o.id,
        label: o.label,
        color: COLORS[i % COLORS.length],
      })),
    [objects]
  );

  const metrics = useMemo(() => {
    const map = new Map<string, string>();
    for (const id of ids) {
      const obj = sceneObjectsStore.getById(id);
      if (!obj) continue;
      for (const m of obj.telemetryMetrics) {
        if (!map.has(m.name)) {
          map.set(m.name, m.unit);
        }
      }
    }
    return [...map.entries()]
      .sort(([a], [b]) => a.localeCompare(b))
      .map(([name, unit]) => ({ name, unit }));
  }, [ids]);

  if (objects.length === 0 || metrics.length === 0) return null;

  return (
    <section className="telemetry-panel">
      <header className="telemetry-panel-header">
        <h3>Live Data</h3>
      </header>
      {metrics.map((m) => (
        <MetricChart
          key={m.name}
          metric={m.name}
          unit={m.unit}
          series={series}
          samples={samples}
        />
      ))}
    </section>
  );
}
