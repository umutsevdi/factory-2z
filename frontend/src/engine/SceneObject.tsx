import { useState } from "react";
import type { ThreeEvent } from "@react-three/fiber";
import type { SceneObject as SceneObjectData } from "../types/scene";
import { theme } from "../theme";

interface Props {
  object: SceneObjectData;
  selected: boolean;
  onSelect: (id: string) => void;
}

const DEFAULT_COLOR = theme.object.default;
const HOVER_COLOR = theme.object.hover;
const SELECTED_COLOR = theme.object.selected;

export function SceneObject({ object, selected, onSelect }: Props) {
  const [hovered, setHovered] = useState(false);

  const { position, size, rotation } = object;

  const color = selected
    ? SELECTED_COLOR
    : hovered
      ? HOVER_COLOR
      : DEFAULT_COLOR;

  const handleClick = (e: ThreeEvent<MouseEvent>) => {
    if (e.nativeEvent.button !== 0) return;
    e.stopPropagation();
    onSelect(object.id);
  };

  const handlePointerOver = (e: ThreeEvent<PointerEvent>) => {
    e.stopPropagation();
    setHovered(true);
    document.body.style.cursor = "pointer";
  };

  const handlePointerOut = () => {
    setHovered(false);
    document.body.style.cursor = "default";
  };

  return (
    <mesh
      position={[position.x, position.y, position.z]}
      rotation={rotation ? [rotation.x, rotation.y, rotation.z] : [0, 0, 0]}
      onClick={handleClick}
      onPointerOver={handlePointerOver}
      onPointerOut={handlePointerOut}
      userData={{ objectId: object.id, size }}
    >
      <boxGeometry args={[size.x, size.y, size.z]} />
      <meshBasicMaterial color={color} wireframe />
    </mesh>
  );
}
