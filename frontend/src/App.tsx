import { useEffect, useState } from "react";
import { Scene } from "./engine/Scene";
import { DetailsPane } from "./components/DetailsPane";
import { SelectionProvider } from "./state/SelectionContext";
import { StatusBar } from "./components/StatusBar";
import { IncidentTerminal } from "./components/IncidentTerminal";
import { getTelemetrySource } from "./api/telemetry";
import { warningsStore } from "./state/warningsStore";
import "./App.css";

interface SceneStatus {
  loading: boolean;
  error: string | null;
  count: number;
}

function App() {
  const [status, setStatus] = useState<SceneStatus>({
    loading: true,
    error: null,
    count: 0,
  });

  useEffect(() => {
    return getTelemetrySource().onWarning((w) => warningsStore.push(w));
  }, []);

  return (
    <SelectionProvider>
      <header className="app-header">
        <h1>Factory 2Z Digital Twin</h1>
        {status.error && (
          <span className="status error">Error: {status.error}</span>
        )}
        {!status.error && status.loading && (
          <span className="status">Loading scene…</span>
        )}
      </header>
      <div className="app-body">
        <div className="scene-container">
          <Scene onStatusChange={setStatus} />
        </div>
        <DetailsPane />
      </div>
      <IncidentTerminal />
      <StatusBar objectCount={status.count} />
    </SelectionProvider>
  );
}

export default App;
