import { useEffect, useRef, useState } from "react";
import { Canvas } from "@react-three/fiber";
import { Grid, OrbitControls } from "@react-three/drei";
import { MOUSE } from "three";
import type { OrbitControls as OrbitControlsImpl } from "three-stdlib";
import { SceneLoader } from "./SceneLoader";
import { RectangleSelector } from "./RectangleSelector";
import type { SceneObject as SceneObjectData } from "../types/scene";
import { theme } from "../theme";

interface Props {
  onStatusChange?: (status: {
    loading: boolean;
    error: string | null;
    count: number;
  }) => void;
}

interface DragRect {
  x: number;
  y: number;
  w: number;
  h: number;
}

export function Scene({ onStatusChange }: Props) {
  const [objects, setObjects] = useState<SceneObjectData[]>([]);
  const [dragRect, setDragRect] = useState<DragRect | null>(null);
  const controlsRef = useRef<OrbitControlsImpl>(null);

  useEffect(() => {
    const apply = (shift: boolean) => {
      const c = controlsRef.current;
      if (!c) return;
      c.mouseButtons.RIGHT = shift ? MOUSE.PAN : MOUSE.ROTATE;
    };
    const onKey = (e: KeyboardEvent) => {
      if (e.key === "Shift") apply(e.type === "keydown");
    };
    window.addEventListener("keydown", onKey);
    window.addEventListener("keyup", onKey);
    const onBlur = () => apply(false);
    window.addEventListener("blur", onBlur);
    return () => {
      window.removeEventListener("keydown", onKey);
      window.removeEventListener("keyup", onKey);
      window.removeEventListener("blur", onBlur);
    };
  }, []);

  return (
    <div
      className="scene-container-inner"
      onContextMenu={(e) => e.preventDefault()}
    >
      <Canvas
        camera={{ position: [8, 8, 8], fov: 50 }}
        style={{ background: theme.surface.s0 }}
      >
        <ambientLight intensity={0.6} />
        <directionalLight position={[5, 10, 5]} intensity={0.8} />

        <Grid
          infiniteGrid
          cellSize={1}
          cellThickness={0.6}
          sectionSize={5}
          sectionThickness={1}
          sectionColor={theme.text.t5}
          cellColor={theme.border.b3}
          fadeDistance={50}
        />
        <axesHelper args={[2]} />

        <SceneLoader
          onStatusChange={onStatusChange}
          onObjectsLoaded={setObjects}
        />

        <RectangleSelector objects={objects} onRectChange={setDragRect} />

        <OrbitControls
          ref={controlsRef}
          makeDefault
          enableDamping
          mouseButtons={{
            LEFT: undefined as unknown as MOUSE,
            MIDDLE: MOUSE.DOLLY,
            RIGHT: MOUSE.ROTATE,
          }}
        />
      </Canvas>
      {dragRect && (
        <div
          className="drag-rect"
          style={{
            left: dragRect.x,
            top: dragRect.y,
            width: dragRect.w,
            height: dragRect.h,
          }}
        />
      )}
    </div>
  );
}
