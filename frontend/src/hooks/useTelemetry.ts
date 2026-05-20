import { useEffect, useRef, useState } from "react";
import { getTelemetrySource, type SourceStatus } from "../api/telemetry";
import type { TelemetrySample } from "../types/telemetry";

export interface UseTelemetryResult {
  samples: Record<string, TelemetrySample[]>;
  isConnected: boolean;
  error: string | null;
}

const FLUSH_INTERVAL_MS = 500;

export function useTelemetry(
  ids: readonly string[],
  bufferSize = 60
): UseTelemetryResult {
  const idsKey = [...ids].sort().join(",");

  const samplesRef = useRef<Record<string, TelemetrySample[]>>({});
  const [samples, setSamples] = useState<Record<string, TelemetrySample[]>>({});
  const [status, setStatus] = useState<SourceStatus>(() =>
    getTelemetrySource().getStatus()
  );

  const pendingRef = useRef<Map<string, TelemetrySample[]>>(new Map());
  const flushTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  useEffect(() => {
    const src = getTelemetrySource();
    return src.onStatus(setStatus);
  }, []);

  const activeIdsRef = useRef<Set<string>>(new Set());

  useEffect(() => {
    const src = getTelemetrySource();
    const unsub = src.onMessage(({ objectId, sample }) => {
      if (!activeIdsRef.current.has(objectId)) return;
      let list = pendingRef.current.get(objectId);
      if (!list) {
        list = [];
        pendingRef.current.set(objectId, list);
      }
      list.push(sample);
      scheduleFlush();
    });
    return unsub;
  }, []);

  function scheduleFlush() {
    if (flushTimerRef.current !== null) return;
    flushTimerRef.current = setTimeout(() => {
      flushTimerRef.current = null;
      const pending = pendingRef.current;
      if (pending.size === 0) return;
      const next = { ...samplesRef.current };
      for (const [id, newSamples] of pending) {
        const prev = next[id] ?? [];
        const merged = prev.concat(newSamples);
        next[id] =
          merged.length > bufferSize ? merged.slice(-bufferSize) : merged;
      }
      pending.clear();
      samplesRef.current = next;
      setSamples(next);
    }, FLUSH_INTERVAL_MS);
  }

  useEffect(() => {
    const src = getTelemetrySource();
    const wanted = new Set(ids);
    const current = activeIdsRef.current;

    const toAdd: string[] = [];
    const toRemove: string[] = [];
    for (const id of wanted) if (!current.has(id)) toAdd.push(id);
    for (const id of current) if (!wanted.has(id)) toRemove.push(id);

    if (toAdd.length) src.subscribe(toAdd);
    if (toRemove.length) src.unsubscribe(toRemove);

    if (toRemove.length) {
      const next = { ...samplesRef.current };
      for (const id of toRemove) delete next[id];
      samplesRef.current = next;
      setSamples(next);
      for (const id of toRemove) pendingRef.current.delete(id);
    }

    activeIdsRef.current = wanted;
  }, [idsKey]);

  useEffect(() => {
    return () => {
      const src = getTelemetrySource();
      const ids = [...activeIdsRef.current];
      if (ids.length) src.unsubscribe(ids);
      activeIdsRef.current = new Set();
      if (flushTimerRef.current !== null) {
        clearTimeout(flushTimerRef.current);
        flushTimerRef.current = null;
      }
    };
  }, []);

  return {
    samples,
    isConnected: status.isConnected,
    error: status.error,
  };
}
