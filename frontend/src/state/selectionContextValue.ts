import { createContext } from "react";

export interface SelectionContextValue {
  selectedIds: ReadonlySet<string>;
  setSelection: (ids: Iterable<string>) => void;
  clear: () => void;
  isSelected: (id: string) => boolean;
}

export const SelectionContext = createContext<
  SelectionContextValue | undefined
>(undefined);
