import { useCallback, useMemo, useState, type ReactNode } from "react";
import { SelectionContext } from "./selectionContextValue";

export function SelectionProvider({ children }: { children: ReactNode }) {
  const [selectedIds, setSelectedIds] = useState<ReadonlySet<string>>(
    () => new Set()
  );

  const setSelection = useCallback((ids: Iterable<string>) => {
    setSelectedIds(new Set(ids));
  }, []);

  const clear = useCallback(() => {
    setSelectedIds((prev) => (prev.size === 0 ? prev : new Set()));
  }, []);

  const value = useMemo(
    () => ({
      selectedIds,
      setSelection,
      clear,
      isSelected: (id: string) => selectedIds.has(id),
    }),
    [selectedIds, setSelection, clear]
  );

  return (
    <SelectionContext.Provider value={value}>
      {children}
    </SelectionContext.Provider>
  );
}
