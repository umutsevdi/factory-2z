import type { ScenePayload } from "../types/scene";

export const api = {
  baseUrl: "/api",

  async getScene(): Promise<ScenePayload> {
    const res = await fetch(`${api.baseUrl}/scene`);
    if (!res.ok) {
      throw new Error(`Failed to load scene: HTTP ${res.status}`);
    }
    return (await res.json()) as ScenePayload;
  },
};
