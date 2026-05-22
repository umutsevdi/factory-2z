-- Factory 2Z Backend Database Schema
-- Run: sqlite3 sensors.db < recreate.sql

PRAGMA foreign_keys = ON;

-- Devices table
CREATE TABLE IF NOT EXISTS devices (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    machine_id TEXT UNIQUE NOT NULL,
    token TEXT UNIQUE NOT NULL,
    mqtt_broker_host TEXT DEFAULT 'localhost',
    mqtt_broker_port INTEGER DEFAULT 1883,
    sensors TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Sensor data table (updated schema with payload column)
CREATE TABLE IF NOT EXISTS sensor_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    topic TEXT NOT NULL,
    payload TEXT,
    received_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Scene objects table
CREATE TABLE IF NOT EXISTS scene_objects (
    id TEXT PRIMARY KEY,
    label TEXT NOT NULL,
    position_x REAL NOT NULL,
    position_y REAL NOT NULL,
    position_z REAL NOT NULL,
    size_x REAL NOT NULL,
    size_y REAL NOT NULL,
    size_z REAL NOT NULL,
    rotation_x REAL,
    rotation_y REAL,
    rotation_z REAL,
    description TEXT NOT NULL,
    metadata TEXT NOT NULL,
    connections TEXT NOT NULL,
    telemetry_metrics TEXT NOT NULL,
    mqtt_device_id TEXT
);

-- Seed scene objects (21 devices across 4 production cells + infrastructure)
-- Coordinate system: X = left/right, Y = up (height), Z = front/back
-- Ground plane is XZ at y=0. Object bottom = position.y - size.y/2
INSERT OR IGNORE INTO scene_objects (
    id, label, position_x, position_y, position_z,
    size_x, size_y, size_z,
    rotation_x, rotation_y, rotation_z,
    description, metadata, connections, telemetry_metrics,
    mqtt_device_id
) VALUES
-- ============================================================
-- CELL 1: Metal Forming (z = 5.0)
-- ============================================================
('obj-1', 'Press A', -4.0, 0.75, 5.0, 1.5, 1.5, 1.5, NULL, NULL, NULL,
 'Hydraulic stamping press, 320 ton capacity, primary forming station.',
 '{"status":"operational","lastService":"2026-03-01","cycleCount":184220,"pressureBar":320,"model":"Schuler MSA 320"}',
 '[{"to_id":"obj-2"}]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"pressure","unit":"bar"},{"name":"vibration","unit":"mm/s"},{"name":"current","unit":"A"}]',
 'press-a'),

('obj-2', 'CNC Mill 1', -1.0, 0.6, 5.0, 1.2, 1.2, 1.8, NULL, NULL, NULL,
 '3-axis vertical machining center, roughing operations after press.',
 '{"status":"operational","lastService":"2026-04-15","spindleHours":12450,"maxRpm":8000,"powerKw":15,"model":"DMG Mori CMX 800V"}',
 '[{"to_id":"obj-3"}]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"rpm","unit":"rpm"},{"name":"vibration","unit":"mm/s"},{"name":"current","unit":"A"}]',
 'cnc-mill-1'),

('obj-3', 'CNC Mill 2', 2.0, 0.6, 5.0, 1.2, 1.2, 1.8, NULL, NULL, NULL,
 '5-axis machining center, finishing operations with tight tolerances.',
 '{"status":"operational","lastService":"2026-04-20","spindleHours":8930,"maxRpm":12000,"powerKw":22,"model":"Mazak VCN-530C"}',
 '[{"to_id":"obj-13"}]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"rpm","unit":"rpm"},{"name":"vibration","unit":"mm/s"},{"name":"current","unit":"A"}]',
 'cnc-mill-2'),

-- ============================================================
-- CELL 2: Welding (z = 2.5)
-- ============================================================
('obj-4', 'Welder 1', -4.0, 0.5, 2.5, 1.0, 1.0, 1.5, NULL, NULL, NULL,
 'MIG welding robot, primary seam welding station, 60% duty cycle.',
 '{"status":"operational","lastService":"2026-04-10","weldCount":95400,"currentA":250,"dutyCycle":60,"model":"Fronius TPS 500i"}',
 '[{"to_id":"obj-5"}]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"current","unit":"A"},{"name":"voltage","unit":"V"}]',
 'welder-1'),

('obj-5', 'Welder 2', -1.0, 0.5, 2.5, 1.0, 1.0, 1.5, NULL, NULL, NULL,
 'TIG welding robot, precision welds on aluminum components, 80% duty cycle.',
 '{"status":"operational","lastService":"2026-04-18","weldCount":62100,"currentA":180,"dutyCycle":80,"model":"Lincoln Electric PowerWave S500"}',
 '[{"to_id":"obj-6"}]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"current","unit":"A"},{"name":"voltage","unit":"V"}]',
 'welder-2'),

('obj-6', 'Weld Positioner', 2.0, 0.4, 2.5, 1.0, 0.8, 0.8, NULL, NULL, NULL,
 '2-axis welding positioner, 500 kg capacity, rotates workpieces for optimal weld access.',
 '{"status":"operational","lastService":"2026-03-25","loadKg":320,"maxRpm":15,"model":"Lorch PosCon 500"}',
 '[{"to_id":"obj-14"}]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"load_kg","unit":"kg"},{"name":"rpm","unit":"rpm"}]',
 'weld-pos'),

-- ============================================================
-- CELL 3: Injection Molding (z = 0.0)
-- ============================================================
('obj-7', 'Injection Molder 1', -4.0, 0.75, 0.0, 2.0, 1.5, 2.0, NULL, NULL, NULL,
 '200-ton injection molding machine, ABS and polycarbonate parts, 45s cycle time.',
 '{"status":"operational","lastService":"2026-04-05","shotCount":245000,"clampTon":200,"cycleTimeS":45,"model":"Engel Victory 200"}',
 '[{"to_id":"obj-8"}]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"pressure","unit":"bar"},{"name":"cycle_time","unit":"s"},{"name":"vibration","unit":"mm/s"}]',
 'inj-mold-1'),

('obj-8', 'Injection Molder 2', -1.0, 0.75, 0.0, 2.0, 1.5, 2.0, NULL, NULL, NULL,
 '350-ton injection molding machine, large structural components, 60s cycle time.',
 '{"status":"operational","lastService":"2026-04-12","shotCount":178500,"clampTon":350,"cycleTimeS":60,"model":"Arburg Allrounder 570H"}',
 '[{"to_id":"obj-9"}]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"pressure","unit":"bar"},{"name":"cycle_time","unit":"s"},{"name":"vibration","unit":"mm/s"}]',
 'inj-mold-2'),

('obj-9', 'Robotic Arm', 2.0, 1.0, 0.0, 1.0, 2.0, 1.5, NULL, NULL, NULL,
 '6-axis pick-and-place arm, part extraction from molding machines and palletizing.',
 '{"status":"operational","lastJob":"2026-05-19T18:42:00Z","payloadKg":12,"reachMm":1800,"model":"FANUC M-20iA"}',
 '[{"to_id":"obj-14"}]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"load_kg","unit":"kg"},{"name":"vibration","unit":"mm/s"},{"name":"cycle_time","unit":"s"}]',
 'robotic-arm'),

-- ============================================================
-- CELL 4: Assembly (z = -2.5)
-- ============================================================
('obj-10', 'Assembly Station 1', -4.0, 0.5, -2.5, 1.5, 1.0, 1.0, NULL, NULL, NULL,
 'Manual assembly station, sub-assembly build with torque-controlled fastening.',
 '{"status":"operational","lastService":"2026-04-22","dailyTarget":120,"torqueStations":4,"model":"Custom workstation"}',
 '[{"to_id":"obj-11"}]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"cycle_time","unit":"s"}]',
 'assembly-1'),

('obj-11', 'Assembly Station 2', -1.0, 0.5, -2.5, 1.5, 1.0, 1.0, NULL, NULL, NULL,
 'Semi-automatic assembly station, press-fit and snap-fit operations.',
 '{"status":"operational","lastService":"2026-04-22","dailyTarget":120,"pressForceN":5000,"model":"Custom workstation"}',
 '[{"to_id":"obj-12"}]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"cycle_time","unit":"s"}]',
 'assembly-2'),

('obj-12', 'Assembly Station 3', 2.0, 0.5, -2.5, 1.5, 1.0, 1.0, NULL, NULL, NULL,
 'Final assembly station, functional testing and labeling before shipping.',
 '{"status":"operational","lastService":"2026-04-22","dailyTarget":120,"testPoints":8,"model":"Custom workstation"}',
 '[{"to_id":"obj-14"}]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"cycle_time","unit":"s"}]',
 'assembly-3'),

-- ============================================================
-- QUALITY INSPECTION (z = 5.0, end of Cell 1)
-- ============================================================
('obj-13', 'CMM Inspection', 5.0, 1.0, 5.0, 1.5, 2.0, 2.0, NULL, NULL, NULL,
 'Coordinate measuring machine, dimensional inspection with \u00b10.002mm accuracy.',
 '{"status":"operational","lastCalibration":"2026-05-01","partsInspected":34200,"accuracyMm":0.002,"model":"Zeiss CONTURA G2"}',
 '[{"to_id":"obj-14"}]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"vibration","unit":"mm/s"}]',
 'cmm'),

-- ============================================================
-- MATERIAL HANDLING (z = -5.0)
-- ============================================================
('obj-14', 'Conveyor 1', -4.0, 0.25, -5.0, 4.0, 0.5, 0.8, NULL, NULL, NULL,
 'Belt conveyor collecting output from all production cells, 0.8 m/s.',
 '{"status":"operational","speedMps":0.8,"lengthM":4,"motorKw":2.2,"model":"Interroll belt conveyor"}',
 '[{"to_id":"obj-15"}]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"rpm","unit":"rpm"},{"name":"load_kg","unit":"kg"}]',
 'conveyor-1'),

('obj-15', 'Conveyor 2', -1.0, 0.25, -5.0, 4.0, 0.5, 0.8, NULL, NULL, NULL,
 'Belt conveyor, intermediate transport to final staging area, 0.8 m/s.',
 '{"status":"operational","speedMps":0.8,"lengthM":4,"motorKw":2.2,"model":"Interroll belt conveyor"}',
 '[{"to_id":"obj-16"}]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"rpm","unit":"rpm"},{"name":"load_kg","unit":"kg"}]',
 'conveyor-2'),

('obj-16', 'Conveyor 3', 2.0, 0.25, -5.0, 3.0, 0.5, 0.8, NULL, NULL, NULL,
 'Roller conveyor, final transport to pallet staging, 1.0 m/s.',
 '{"status":"operational","speedMps":1.0,"lengthM":3,"motorKw":1.5,"model":"Interroll roller conveyor"}',
 '[{"to_id":"obj-21"}]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"rpm","unit":"rpm"},{"name":"load_kg","unit":"kg"}]',
 'conveyor-3'),

-- ============================================================
-- INFRASTRUCTURE (z = 7.5, back wall)
-- ============================================================
('obj-17', 'Storage Rack', -5.0, 1.5, 7.5, 2.0, 3.0, 2.0, NULL, NULL, NULL,
 'Buffer rack for raw material and in-progress assemblies, 24 slots.',
 '{"status":"operational","capacity":24,"occupied":17,"lastAudit":"2026-05-18"}',
 '[{"to_id":"obj-1"}]',
 '[]',
 NULL),

('obj-18', 'Control Cabinet', -2.0, 1.0, 7.5, 1.2, 2.0, 0.8, NULL, NULL, NULL,
 'PLC and power distribution cabinet, main factory automation controller.',
 '{"status":"operational","temperatureC":38,"voltageV":400,"plcModel":"Siemens S7-1500","lastMaintenance":"2026-04-01"}',
 '[]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"voltage","unit":"V"}]',
 'ctrl-cab'),

('obj-19', 'Compressor', 1.0, 0.75, 7.5, 1.5, 1.5, 1.5, NULL, NULL, NULL,
 'Screw air compressor, 10 bar supply for pneumatics and tooling.',
 '{"status":"operational","lastService":"2026-03-15","runtimeHours":8420,"pressureBar":10,"flowRateLpm":1200,"model":"Atlas Copco GA 37"}',
 '[]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"pressure","unit":"bar"},{"name":"rpm","unit":"rpm"}]',
 'compressor'),

('obj-20', 'Cooling Unit', 4.0, 0.75, 7.5, 1.5, 1.5, 1.5, NULL, NULL, NULL,
 'Industrial chiller, coolant supply for CNC machines and injection molders.',
 '{"status":"operational","lastService":"2026-04-08","coolingKw":45,"flowRateLpm":50,"outletTempC":12,"model":"Carrier AquaEdge 30XA"}',
 '[]',
 '[{"name":"temperature","unit":"\u00b0C"},{"name":"flow_rate","unit":"L/min"},{"name":"pressure","unit":"bar"}]',
 'cooling-unit'),

-- ============================================================
-- FINISHED GOODS
-- ============================================================
('obj-21', 'Pallet', 5.0, 0.15, -5.0, 1.2, 0.3, 1.2, NULL, NULL, NULL,
 'EUR-pallet staging slot for finished goods, ready for shipping.',
 '{"status":"loaded","itemCount":6,"lastScan":"2026-05-20T08:30:00Z"}',
 '[]',
 '[]',
 NULL);
