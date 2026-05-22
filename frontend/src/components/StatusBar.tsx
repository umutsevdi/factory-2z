import { useEffect, useState } from "react";
import { getTelemetrySource, type SourceStatus } from "../api/telemetry";

interface Props {
  objectCount: number;
}

export function StatusBar({ objectCount }: Props) {
  const [status, setStatus] = useState<SourceStatus>(() =>
    getTelemetrySource().getStatus()
  );
  const [now, setNow] = useState<number>(() => Date.now());

  useEffect(() => {
    return getTelemetrySource().onStatus(setStatus);
  }, []);

  useEffect(() => {
    const id = setInterval(() => setNow(Date.now()), 1000);
    return () => clearInterval(id);
  }, []);

  const stateClass = status.error
    ? "offline"
    : status.isConnected
      ? "live"
      : "reconnecting";

  const stateLabel = status.error
    ? `Offline — ${status.error}`
    : status.isConnected
      ? "Live"
      : `Reconnecting (attempt ${status.reconnectAttempt})`;

  const lastMsgLabel =
    status.lastMessageAt === null
      ? "no data yet"
      : `last sample ${formatAge(now - status.lastMessageAt)} ago`;

  return (
    <footer className="status-bar">
      <span className={`status-dot ${stateClass}`} aria-hidden />
      <span className="status-bar-state">{stateLabel}</span>
      <span className="status-bar-sep">·</span>
      <span className="status-bar-source">WS</span>
      <span className="status-bar-sep">·</span>
      <span className="status-bar-lastmsg">{lastMsgLabel}</span>
      <span className="status-bar-spacer" />
      <span className="status-bar-count">
        {objectCount} object{objectCount === 1 ? "" : "s"}
      </span>
    </footer>
  );
}

function formatAge(ms: number) {
  if (ms < 1000) return "<1s";
  const s = Math.floor(ms / 1000);
  if (s < 60) return `${s}s`;
  const m = Math.floor(s / 60);
  const rem = s % 60;
  return `${m}m${rem ? ` ${rem}s` : ""}`;
}
