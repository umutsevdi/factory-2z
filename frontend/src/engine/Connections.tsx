import { useMemo } from "react";
import { Line } from "@react-three/drei";
import type { SceneObject } from "../types/scene";
import { theme } from "../theme";

interface Props {
  objects: SceneObject[];
}

const LINE_COLOR = theme.text.t5;
const LINE_WIDTH = 1.5;

export function Connections({ objects }: Props) {
  const segments = useMemo(() => {
    const byId = new Map(objects.map((o) => [o.id, o] as const));
    const result: { key: string; points: [number, number, number][] }[] = [];
    for (const obj of objects) {
      for (const conn of obj.connections) {
        const target = byId.get(conn.to_id);
        if (!target) continue;
        result.push({
          key: `${obj.id}->${conn.to_id}`,
          points: [
            [obj.position.x, obj.position.y, obj.position.z],
            [target.position.x, target.position.y, target.position.z],
          ],
        });
      }
    }
    return result;
  }, [objects]);

  return (
    <>
      {segments.map((s) => (
        <Line
          key={s.key}
          points={s.points}
          color={LINE_COLOR}
          lineWidth={LINE_WIDTH}
          transparent
          opacity={0.85}
        />
      ))}
    </>
  );
}
