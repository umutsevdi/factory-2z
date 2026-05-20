import type { WarningPayload } from "../types/telemetry";

export interface WarningEntry extends WarningPayload {
  read: boolean;
}

interface State {
  warnings: WarningEntry[];
  unreadCount: number;
}

const MAX_ENTRIES = 100;

let state: State = { warnings: [], unreadCount: 0 };
const listeners = new Set<() => void>();
const seenIds = new Set<string>();

function notify() {
  for (const l of listeners) l();
}

export const warningsStore = {
  getSnapshot(): State {
    return state;
  },

  subscribe(listener: () => void): () => void {
    listeners.add(listener);
    return () => {
      listeners.delete(listener);
    };
  },

  push(w: WarningPayload) {
    if (seenIds.has(w.id)) return;
    seenIds.add(w.id);
    const entry: WarningEntry = { ...w, read: false };
    const next = [entry, ...state.warnings];
    if (next.length > MAX_ENTRIES) {
      const dropped = next.splice(MAX_ENTRIES, next.length - MAX_ENTRIES);
      for (const d of dropped) seenIds.delete(d.id);
    }
    state = {
      warnings: next,
      unreadCount: state.unreadCount + 1,
    };
    notify();
  },

  dismiss(id: string) {
    let removedUnread = 0;
    const next = state.warnings.filter((w) => {
      if (w.id !== id) return true;
      if (!w.read) removedUnread += 1;
      seenIds.delete(w.id);
      return false;
    });
    if (next.length === state.warnings.length) return;
    state = {
      warnings: next,
      unreadCount: Math.max(0, state.unreadCount - removedUnread),
    };
    notify();
  },

  markAllRead() {
    if (state.unreadCount === 0) return;
    state = {
      warnings: state.warnings.map((w) => (w.read ? w : { ...w, read: true })),
      unreadCount: 0,
    };
    notify();
  },

  clear() {
    if (state.warnings.length === 0) return;
    seenIds.clear();
    state = { warnings: [], unreadCount: 0 };
    notify();
  },
};
