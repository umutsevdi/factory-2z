import { useEffect, useLayoutEffect, useMemo, useRef, useState } from "react";
import { useWarnings } from "../hooks/useWarnings";
import { useSelection } from "../state/useSelection";

const AT_BOTTOM_THRESHOLD_PX = 4;

export function IncidentTerminal() {
  const { warnings, clear } = useWarnings();
  const { setSelection } = useSelection();

  const entries = useMemo(() => [...warnings].reverse(), [warnings]);

  const scrollRef = useRef<HTMLDivElement>(null);
  const [autoScroll, setAutoScroll] = useState(true);

  const handleScroll = () => {
    const el = scrollRef.current;
    if (!el) return;
    const distance = el.scrollHeight - (el.scrollTop + el.clientHeight);
    setAutoScroll(distance <= AT_BOTTOM_THRESHOLD_PX);
  };

  useLayoutEffect(() => {
    if (!autoScroll) return;
    const el = scrollRef.current;
    if (!el) return;
    el.scrollTop = el.scrollHeight;
  }, [entries.length, autoScroll]);

  useEffect(() => {
    const el = scrollRef.current;
    if (!el) return;
    el.scrollTop = el.scrollHeight;
  }, []);

  const jumpToBottom = () => {
    const el = scrollRef.current;
    if (!el) return;
    el.scrollTop = el.scrollHeight;
    setAutoScroll(true);
  };

  return (
    <section className="incident-terminal" aria-label="Incident log">
      <header className="incident-terminal-header">
        <span className="incident-terminal-title">incidents</span>
        <span className="incident-terminal-spacer" />
        <span className="incident-terminal-count">
          {warnings.length} {warnings.length === 1 ? "entry" : "entries"}
        </span>
        {!autoScroll && (
          <button className="link-btn" onClick={jumpToBottom}>
            jump to bottom
          </button>
        )}
        <button
          className="link-btn"
          onClick={clear}
          disabled={warnings.length === 0}
        >
          clear
        </button>
      </header>
      <div
        className="incident-terminal-body"
        ref={scrollRef}
        onScroll={handleScroll}
      >
        {entries.length === 0 ? (
          <div className="incident-terminal-empty">
            <span className="incident-prompt">$</span> awaiting incidents…
          </div>
        ) : (
          entries.map((w) => (
            <div
              key={w.id}
              className={`incident-row severity-${w.severity}${w.objectId ? " clickable" : ""}`}
              role={w.objectId ? "button" : undefined}
              tabIndex={w.objectId ? 0 : -1}
              onClick={() => w.objectId && setSelection([w.objectId])}
              onKeyDown={(e) => {
                if (!w.objectId) return;
                if (e.key === "Enter" || e.key === " ") {
                  e.preventDefault();
                  setSelection([w.objectId]);
                }
              }}
              title={w.objectId ? `Select ${w.objectId}` : undefined}
            >
              <span className="incident-ts">[{formatTime(w.timestamp)}]</span>
              <span className={`incident-level severity-${w.severity}`}>
                {w.severity.toUpperCase().padEnd(5)}
              </span>
              {w.objectId && (
                <span className="incident-target">{w.objectId}</span>
              )}
              {w.metric && <span className="incident-metric">{w.metric}</span>}
              <span className="incident-msg">{w.message}</span>
              {w.value !== undefined && w.threshold !== undefined && (
                <span className="incident-values">
                  ({w.value} ≥ {w.threshold})
                </span>
              )}
            </div>
          ))
        )}
      </div>
    </section>
  );
}

function formatTime(ts: number) {
  const d = new Date(ts);
  return `${pad(d.getHours())}:${pad(d.getMinutes())}:${pad(d.getSeconds())}`;
}
function pad(n: number) {
  return String(n).padStart(2, "0");
}
