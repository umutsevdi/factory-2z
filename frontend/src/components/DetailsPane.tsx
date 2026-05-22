import { useMemo, useState } from "react";
import { useSelection } from "../state/useSelection";
import { sceneObjectsStore } from "../state/sceneObjectsStore";
import type { SceneObject } from "../types/scene";
import { TelemetryPanel } from "./TelemetryPanel";

export function DetailsPane() {
  const { selectedIds, clear } = useSelection();
  const ids = useMemo(() => [...selectedIds], [selectedIds]);
  const [activeTab, setActiveTab] = useState(0);

  const objects = useMemo(
    () =>
      ids
        .map((id) => sceneObjectsStore.getById(id))
        .filter((o): o is SceneObject => o !== undefined),
    [ids]
  );

  const telemetryObjects = useMemo(
    () => objects.map((o) => ({ id: o.id, label: o.label })),
    [objects]
  );

  if (objects.length > 1 && activeTab >= objects.length) {
    setActiveTab(0);
  }

  return (
    <aside className="details-pane">
      <header className="details-pane-header">
        <h2>Details</h2>
        {ids.length > 1 && (
          <button className="clear-btn" onClick={clear}>
            Clear ({ids.length})
          </button>
        )}
      </header>

      {objects.length === 0 ? (
        <p className="details-empty">Select an object to see details.</p>
      ) : (
        <>
          {objects.length > 1 ? (
            <>
              <div className="details-tabs">
                {objects.map((obj, i) => (
                  <button
                    key={obj.id}
                    className={`details-tab ${i === activeTab ? "active" : ""}`}
                    onClick={() => setActiveTab(i)}
                  >
                    {obj.label}
                  </button>
                ))}
              </div>
              <ObjectDetailCard obj={objects[activeTab]} />
            </>
          ) : (
            <div className="details-cards">
              {objects.map((obj) => (
                <ObjectDetailCard key={obj.id} obj={obj} />
              ))}
            </div>
          )}
          <TelemetryPanel objects={telemetryObjects} />
        </>
      )}
    </aside>
  );
}

function ObjectDetailCard({ obj }: { obj: SceneObject }) {
  return (
    <div className="details-card">
      <div className="details-id">
        <span className="details-label">ID</span>
        <code>{obj.id}</code>
      </div>
      <h3>{obj.label}</h3>
      <p>{obj.description}</p>
      <h4>Metadata</h4>
      <dl className="details-metadata">
        {Object.entries(obj.metadata).map(([key, value]) => (
          <div key={key} className="details-metadata-row">
            <dt>{key}</dt>
            <dd>{String(value)}</dd>
          </div>
        ))}
      </dl>
    </div>
  );
}
