import { memo, useMemo } from "react";
import {
  CartesianGrid,
  Legend,
  Line,
  LineChart,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";
import type { TelemetrySample } from "../types/telemetry";
import { theme } from "../theme";

interface SeriesSpec {
  id: string;
  label: string;
  color: string;
}

interface Props {
  metric: string;
  unit?: string;
  series: SeriesSpec[];
  samples: Record<string, TelemetrySample[]>;
}

function MetricChartImpl({ metric, unit, series, samples }: Props) {
  const data = useMemo(() => {
    const byTs = new Map<
      number,
      Record<string, number | null> & { ts: number }
    >();
    for (const s of series) {
      const buf = samples[s.id];
      if (!buf) continue;
      for (const sample of buf) {
        const value = sample.metrics[metric];
        if (value === undefined) continue;
        let row = byTs.get(sample.timestamp);
        if (!row) {
          row = { ts: sample.timestamp };
          byTs.set(sample.timestamp, row);
        }
        row[s.id] = value;
      }
    }
    return [...byTs.values()].sort((a, b) => a.ts - b.ts);
  }, [metric, series, samples]);

  const hasData = data.length > 0;

  return (
    <div className="metric-chart">
      <div className="metric-chart-header">
        <span className="metric-chart-title">
          {metric}
          {unit ? ` (${unit})` : ""}
        </span>
      </div>
      {hasData ? (
        <ResponsiveContainer width="100%" height={160}>
          <LineChart
            data={data}
            margin={{ top: 6, right: 12, left: 0, bottom: 0 }}
          >
            <CartesianGrid stroke={theme.border.b2} strokeDasharray="3 3" />
            <XAxis
              dataKey="ts"
              stroke={theme.text.t4}
              fontSize={10}
              tickFormatter={formatTime}
              minTickGap={32}
            />
            <YAxis
              stroke={theme.text.t4}
              fontSize={10}
              domain={["auto", "auto"]}
              width={40}
            />
            <Tooltip
              contentStyle={{
                background: theme.surface.s2,
                border: `1px solid ${theme.border.b3}`,
                fontSize: 12,
              }}
              labelFormatter={(ts) => formatTime(ts as number)}
            />
            <Legend wrapperStyle={{ fontSize: 11 }} />
            {series.map((s) => (
              <Line
                key={s.id}
                type="monotone"
                dataKey={s.id}
                name={s.label}
                stroke={s.color}
                dot={false}
                isAnimationActive={false}
                strokeWidth={1.5}
                connectNulls
              />
            ))}
          </LineChart>
        </ResponsiveContainer>
      ) : (
        <p className="metric-chart-empty">Waiting for data…</p>
      )}
    </div>
  );
}

function formatTime(ts: number) {
  const d = new Date(ts);
  const hh = String(d.getHours()).padStart(2, "0");
  const mm = String(d.getMinutes()).padStart(2, "0");
  const ss = String(d.getSeconds()).padStart(2, "0");
  return `${hh}:${mm}:${ss}`;
}

export const MetricChart = memo(MetricChartImpl);
