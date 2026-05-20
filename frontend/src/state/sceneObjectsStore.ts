import type { SceneObject } from "../types/scene";

let objects: SceneObject[] = [];
const byId = new Map<string, SceneObject>();

export const sceneObjectsStore = {
  set(next: SceneObject[]) {
    objects = next;
    byId.clear();
    for (const o of next) byId.set(o.id, o);
  },

  getAll(): SceneObject[] {
    return objects;
  },

  getById(id: string): SceneObject | undefined {
    return byId.get(id);
  },

  clear() {
    objects = [];
    byId.clear();
  },
};
