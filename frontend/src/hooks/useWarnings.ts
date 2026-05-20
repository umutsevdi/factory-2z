import { useSyncExternalStore } from "react";
import { warningsStore, type WarningEntry } from "../state/warningsStore";

export interface UseWarningsResult {
  warnings: WarningEntry[];
  unreadCount: number;
  dismiss: (id: string) => void;
  markAllRead: () => void;
  clear: () => void;
}

export function useWarnings(): UseWarningsResult {
  const snap = useSyncExternalStore(
    warningsStore.subscribe,
    warningsStore.getSnapshot,
    warningsStore.getSnapshot
  );
  return {
    warnings: snap.warnings,
    unreadCount: snap.unreadCount,
    dismiss: warningsStore.dismiss,
    markAllRead: warningsStore.markAllRead,
    clear: warningsStore.clear,
  };
}
