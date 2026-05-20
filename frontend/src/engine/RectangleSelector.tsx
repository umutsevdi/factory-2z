import { useEffect } from "react";
import { useThree } from "@react-three/fiber";
import { Vector2, Vector3 } from "three";
import { useSelection } from "../state/useSelection";
import type { SceneObject } from "../types/scene";

interface Rect {
  /** Page coordinates (relative to viewport). */
  x: number;
  y: number;
  w: number;
  h: number;
}

interface Props {
  objects: SceneObject[];
  /** Called with the live drag rectangle (in canvas-local pixel space) or null. */
  onRectChange: (rect: Rect | null) => void;
}

const DRAG_THRESHOLD_PX = 3;

/**
 * Box-selection driver. Mounted inside the R3F canvas so it can read camera +
 * scene + the canvas DOM element. Renders nothing in 3D; the visible drag
 * rectangle is drawn by the parent DOM overlay.
 *
 * Interaction:
 *   - Left mouse down on empty canvas area starts a drag.
 *   - Pointer down on a mesh is ignored (the mesh's own click handler runs).
 *   - On pointer up: if the rect is below the drag threshold, treat as an
 *     empty-space click and clear selection. Otherwise, replace the selection
 *     with every object whose projected screen-space AABB intersects the rect.
 */
export function RectangleSelector({ objects, onRectChange }: Props) {
  const { gl, camera, raycaster } = useThree();
  const { setSelection, clear } = useSelection();

  useEffect(() => {
    const canvas = gl.domElement;

    let dragStart: { x: number; y: number } | null = null;
    let dragCurrent: { x: number; y: number } | null = null;
    let active = false;

    const onPointerDown = (ev: PointerEvent) => {
      if (ev.button !== 0) return; // only left button starts a rect

      // If the pointer is over a mesh, let SceneObject handle the click.
      const rect = canvas.getBoundingClientRect();
      const nx = ((ev.clientX - rect.left) / rect.width) * 2 - 1;
      const ny = -((ev.clientY - rect.top) / rect.height) * 2 + 1;
      raycaster.setFromCamera(new Vector2(nx, ny), camera);
      // Test against meshes that carry our marker.
      const hits = raycaster.intersectObjects(
        // Walk the scene to collect candidate meshes.
        collectSelectables(),
        false
      );
      if (hits.length > 0) return;

      active = true;
      dragStart = { x: ev.clientX - rect.left, y: ev.clientY - rect.top };
      dragCurrent = { ...dragStart };
      canvas.setPointerCapture(ev.pointerId);
    };

    const onPointerMove = (ev: PointerEvent) => {
      if (!active || !dragStart) return;
      const rect = canvas.getBoundingClientRect();
      dragCurrent = {
        x: ev.clientX - rect.left,
        y: ev.clientY - rect.top,
      };
      const r = normalizeRect(dragStart, dragCurrent);
      // Only show the rectangle once the user has moved past the click threshold.
      if (r.w >= DRAG_THRESHOLD_PX || r.h >= DRAG_THRESHOLD_PX) {
        onRectChange(r);
      }
    };

    const onPointerUp = (ev: PointerEvent) => {
      if (!active || !dragStart || !dragCurrent) {
        active = false;
        dragStart = null;
        dragCurrent = null;
        onRectChange(null);
        return;
      }

      const r = normalizeRect(dragStart, dragCurrent);
      onRectChange(null);
      try {
        canvas.releasePointerCapture(ev.pointerId);
      } catch {
        /* ignore */
      }

      if (r.w < DRAG_THRESHOLD_PX && r.h < DRAG_THRESHOLD_PX) {
        // Treat as click on empty space.
        clear();
      } else {
        const rect = canvas.getBoundingClientRect();
        const selected = objects.filter((obj) =>
          objectIntersectsRect(obj, r, camera, rect.width, rect.height)
        );
        // Per spec: empty drag clears selection.
        setSelection(selected.map((o) => o.id));
      }

      active = false;
      dragStart = null;
      dragCurrent = null;
    };

    const onPointerCancel = () => {
      if (active) {
        active = false;
        dragStart = null;
        dragCurrent = null;
        onRectChange(null);
      }
    };

    const collectSelectables = () => {
      const meshes: Parameters<typeof raycaster.intersectObjects>[0] = [];
      // Traverse the parent scene of the camera. In R3F, camera.parent is the root.
      const root = camera.parent;
      if (!root) return meshes;
      root.traverse((obj) => {
        if (
          (obj as { isMesh?: boolean }).isMesh &&
          obj.userData &&
          typeof obj.userData.objectId === "string"
        ) {
          meshes.push(obj);
        }
      });
      return meshes;
    };

    canvas.addEventListener("pointerdown", onPointerDown);
    canvas.addEventListener("pointermove", onPointerMove);
    canvas.addEventListener("pointerup", onPointerUp);
    canvas.addEventListener("pointercancel", onPointerCancel);

    return () => {
      canvas.removeEventListener("pointerdown", onPointerDown);
      canvas.removeEventListener("pointermove", onPointerMove);
      canvas.removeEventListener("pointerup", onPointerUp);
      canvas.removeEventListener("pointercancel", onPointerCancel);
    };
  }, [gl, camera, raycaster, objects, onRectChange, setSelection, clear]);

  return null;
}

function normalizeRect(
  a: { x: number; y: number },
  b: { x: number; y: number }
): Rect {
  const x = Math.min(a.x, b.x);
  const y = Math.min(a.y, b.y);
  return { x, y, w: Math.abs(a.x - b.x), h: Math.abs(a.y - b.y) };
}

/**
 * Project the eight corners of the object's axis-aligned (in local space) box
 * to screen space and test whether their AABB overlaps the drag rectangle.
 *
 * `rect` is in canvas-local pixels (origin top-left). `canvasW`/`canvasH` are
 * the canvas dimensions in pixels.
 */
function objectIntersectsRect(
  obj: SceneObject,
  rect: Rect,
  camera: import("three").Camera,
  canvasW: number,
  canvasH: number
): boolean {
  const hx = obj.size.x / 2;
  const hy = obj.size.y / 2;
  const hz = obj.size.z / 2;
  const cx = obj.position.x;
  const cy = obj.position.y;
  const cz = obj.position.z;

  // 8 corners in world space (rotation ignored — uniform with current SceneObject
  // until rotated boxes are needed; can be extended easily).
  const corners: [number, number, number][] = [
    [cx - hx, cy - hy, cz - hz],
    [cx + hx, cy - hy, cz - hz],
    [cx - hx, cy + hy, cz - hz],
    [cx + hx, cy + hy, cz - hz],
    [cx - hx, cy - hy, cz + hz],
    [cx + hx, cy - hy, cz + hz],
    [cx - hx, cy + hy, cz + hz],
    [cx + hx, cy + hy, cz + hz],
  ];

  let minX = Infinity;
  let minY = Infinity;
  let maxX = -Infinity;
  let maxY = -Infinity;
  const v = new Vector3();
  let anyInFront = false;

  for (const [x, y, z] of corners) {
    v.set(x, y, z).project(camera);
    // v.z within [-1, 1] is inside the camera frustum (depth-wise).
    if (v.z < 1) anyInFront = true;
    const sx = ((v.x + 1) / 2) * canvasW;
    const sy = ((1 - v.y) / 2) * canvasH;
    if (sx < minX) minX = sx;
    if (sx > maxX) maxX = sx;
    if (sy < minY) minY = sy;
    if (sy > maxY) maxY = sy;
  }

  if (!anyInFront) return false;

  // AABB overlap test.
  return !(
    maxX < rect.x ||
    minX > rect.x + rect.w ||
    maxY < rect.y ||
    minY > rect.y + rect.h
  );
}
