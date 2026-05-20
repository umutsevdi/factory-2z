import { useEffect, useState } from "react";
import { api } from "../api/api";
import { useSelection } from "../state/useSelection";
import { sceneObjectsStore } from "../state/sceneObjectsStore";
import type { SceneObject as SceneObjectData } from "../types/scene";
import { SceneObject } from "./SceneObject";
import { Connections } from "./Connections";

interface Props {
  onStatusChange?: (status: {
    loading: boolean;
    error: string | null;
    count: number;
  }) => void;
  onObjectsLoaded?: (objects: SceneObjectData[]) => void;
}

export function SceneLoader({ onStatusChange, onObjectsLoaded }: Props) {
  const [objects, setObjects] = useState<SceneObjectData[]>([]);
  const { selectedIds, setSelection } = useSelection();

  useEffect(() => {
    let cancelled = false;
    onStatusChange?.({ loading: true, error: null, count: 0 });

    api
      .getScene()
      .then((scene) => {
        if (cancelled) return;
        setObjects(scene.objects);
        sceneObjectsStore.set(scene.objects);
        onStatusChange?.({
          loading: false,
          error: null,
          count: scene.objects.length,
        });
        onObjectsLoaded?.(scene.objects);
      })
      .catch((err: unknown) => {
        if (cancelled) return;
        const message = err instanceof Error ? err.message : String(err);
        onStatusChange?.({ loading: false, error: message, count: 0 });
      });

    return () => {
      cancelled = true;
    };
  }, []);

  const handleSelect = (id: string) => {
    setSelection([id]);
  };

  return (
    <>
      <Connections objects={objects} />
      {objects.map((obj) => (
        <SceneObject
          key={obj.id}
          object={obj}
          selected={selectedIds.has(obj.id)}
          onSelect={handleSelect}
        />
      ))}
    </>
  );
}
