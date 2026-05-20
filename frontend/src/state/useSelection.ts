import { useContext } from "react";
import {
  SelectionContext,
  type SelectionContextValue,
} from "./selectionContextValue";

export function useSelection(): SelectionContextValue {
  const ctx = useContext(SelectionContext);
  if (!ctx) {
    throw new Error("useSelection must be used inside <SelectionProvider>");
  }
  return ctx;
}
