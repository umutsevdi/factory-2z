#!/usr/bin/env python3
"""
Factory 2Z Telemetry Simulator

Publishes synthetic sensor data to MQTT topics that the backend listens on.
Matches the mock data behavior from the frontend (random-walk with same
ranges and step sizes). Each device publishes to topics like:

    factory/<device-id>/temperature
    factory/<device-id>/pressure
    factory/<device-id>/voltage
    factory/<device-id>/rpm
    factory/<device-id>/vibration
    factory/<device-id>/current
    factory/<device-id>/load_kg
    factory/<device-id>/cycle_time
    factory/<device-id>/flow_rate

Thresholds that trigger warnings in the backend:
    temperature  > 90 °C
    pressure     > 6 bar
    voltage      > 450 V
    vibration    > 4.5 mm/s
    current      > 40 A
    cycle_time   > 90 s
    load_kg      > 400 kg
"""

import argparse
import random
import time

import paho.mqtt.client as mqtt


# ---------------------------------------------------------------------------
# Device / metric configuration  (mirrors the frontend mock)
# ---------------------------------------------------------------------------

DEVICES = {
    # Cell 1: Metal Forming
    "press-a": {
        "metrics": {
            "temperature": {"seed": 0,  "range": (30, 110),  "step": 3.0, "decimals": 2},
            "pressure":    {"seed": 1,  "range": (0, 12),    "step": 0.3, "decimals": 3},
            "vibration":   {"seed": 50, "range": (0.5, 5.0), "step": 0.2, "decimals": 3},
            "current":     {"seed": 51, "range": (10, 50),   "step": 2.0, "decimals": 1},
        },
    },
    "cnc-mill-1": {
        "metrics": {
            "temperature": {"seed": 2,  "range": (20, 95),   "step": 2.5, "decimals": 2},
            "rpm":         {"seed": 3,  "range": (1000, 8000), "step": 300, "decimals": 0},
            "vibration":   {"seed": 4,  "range": (0.2, 6.0), "step": 0.2, "decimals": 3},
            "current":     {"seed": 52, "range": (5, 25),    "step": 1.5, "decimals": 1},
        },
    },
    "cnc-mill-2": {
        "metrics": {
            "temperature": {"seed": 5,  "range": (20, 100),  "step": 2.5, "decimals": 2},
            "rpm":         {"seed": 6,  "range": (2000, 12000), "step": 400, "decimals": 0},
            "vibration":   {"seed": 7,  "range": (0.1, 5.0), "step": 0.15, "decimals": 3},
            "current":     {"seed": 53, "range": (8, 35),    "step": 2.0, "decimals": 1},
        },
    },

    # Cell 2: Welding
    "welder-1": {
        "metrics": {
            "temperature": {"seed": 8,  "range": (40, 120),  "step": 4.0, "decimals": 2},
            "current":     {"seed": 9,  "range": (50, 300),  "step": 10.0, "decimals": 1},
            "voltage":     {"seed": 10, "range": (18, 36),   "step": 1.0, "decimals": 2},
        },
    },
    "welder-2": {
        "metrics": {
            "temperature": {"seed": 11, "range": (35, 110),  "step": 3.5, "decimals": 2},
            "current":     {"seed": 12, "range": (30, 220),  "step": 8.0, "decimals": 1},
            "voltage":     {"seed": 13, "range": (12, 30),   "step": 0.8, "decimals": 2},
        },
    },
    "weld-pos": {
        "metrics": {
            "temperature": {"seed": 14, "range": (20, 70),   "step": 2.0, "decimals": 2},
            "load_kg":     {"seed": 15, "range": (50, 480),  "step": 20.0, "decimals": 1},
            "rpm":         {"seed": 16, "range": (0, 15),    "step": 1.0, "decimals": 1},
        },
    },

    # Cell 3: Injection Molding
    "inj-mold-1": {
        "metrics": {
            "temperature": {"seed": 17, "range": (150, 220), "step": 5.0, "decimals": 2},
            "pressure":    {"seed": 18, "range": (80, 180),  "step": 4.0, "decimals": 1},
            "cycle_time":  {"seed": 19, "range": (35, 65),   "step": 2.0, "decimals": 1},
            "vibration":   {"seed": 54, "range": (0.3, 3.5), "step": 0.15, "decimals": 3},
        },
    },
    "inj-mold-2": {
        "metrics": {
            "temperature": {"seed": 20, "range": (160, 240), "step": 5.0, "decimals": 2},
            "pressure":    {"seed": 21, "range": (100, 220), "step": 5.0, "decimals": 1},
            "cycle_time":  {"seed": 22, "range": (45, 85),   "step": 3.0, "decimals": 1},
            "vibration":   {"seed": 55, "range": (0.4, 4.0), "step": 0.2, "decimals": 3},
        },
    },
    "robotic-arm": {
        "metrics": {
            "temperature": {"seed": 23, "range": (25, 80),   "step": 2.0, "decimals": 2},
            "load_kg":     {"seed": 24, "range": (0, 12),    "step": 1.0, "decimals": 1},
            "vibration":   {"seed": 56, "range": (0.2, 4.5), "step": 0.2, "decimals": 3},
            "cycle_time":  {"seed": 57, "range": (3, 12),    "step": 0.5, "decimals": 1},
        },
    },

    # Cell 4: Assembly
    "assembly-1": {
        "metrics": {
            "temperature": {"seed": 25, "range": (18, 35),   "step": 1.0, "decimals": 2},
            "cycle_time":  {"seed": 26, "range": (30, 70),   "step": 3.0, "decimals": 1},
        },
    },
    "assembly-2": {
        "metrics": {
            "temperature": {"seed": 27, "range": (18, 35),   "step": 1.0, "decimals": 2},
            "cycle_time":  {"seed": 28, "range": (25, 60),   "step": 2.5, "decimals": 1},
        },
    },
    "assembly-3": {
        "metrics": {
            "temperature": {"seed": 29, "range": (18, 35),   "step": 1.0, "decimals": 2},
            "cycle_time":  {"seed": 30, "range": (20, 55),   "step": 2.0, "decimals": 1},
        },
    },

    # Quality Inspection
    "cmm": {
        "metrics": {
            "temperature": {"seed": 31, "range": (18, 25),   "step": 0.5, "decimals": 2},
            "vibration":   {"seed": 32, "range": (0.05, 1.5), "step": 0.05, "decimals": 3},
        },
    },

    # Material Handling
    "conveyor-1": {
        "metrics": {
            "temperature": {"seed": 33, "range": (20, 60),   "step": 1.5, "decimals": 2},
            "rpm":         {"seed": 34, "range": (30, 120),  "step": 5.0, "decimals": 1},
            "load_kg":     {"seed": 35, "range": (10, 350),  "step": 15.0, "decimals": 1},
        },
    },
    "conveyor-2": {
        "metrics": {
            "temperature": {"seed": 36, "range": (20, 60),   "step": 1.5, "decimals": 2},
            "rpm":         {"seed": 37, "range": (30, 120),  "step": 5.0, "decimals": 1},
            "load_kg":     {"seed": 38, "range": (10, 350),  "step": 15.0, "decimals": 1},
        },
    },
    "conveyor-3": {
        "metrics": {
            "temperature": {"seed": 39, "range": (20, 60),   "step": 1.5, "decimals": 2},
            "rpm":         {"seed": 40, "range": (40, 150),  "step": 5.0, "decimals": 1},
            "load_kg":     {"seed": 41, "range": (10, 400),  "step": 15.0, "decimals": 1},
        },
    },

    # Infrastructure
    "ctrl-cab": {
        "metrics": {
            "temperature": {"seed": 42, "range": (30, 55),   "step": 1.0, "decimals": 2},
            "voltage":     {"seed": 43, "range": (380, 420), "step": 3.0, "decimals": 1},
        },
    },
    "compressor": {
        "metrics": {
            "temperature": {"seed": 44, "range": (40, 90),   "step": 2.5, "decimals": 2},
            "pressure":    {"seed": 45, "range": (8, 12),    "step": 0.2, "decimals": 2},
            "rpm":         {"seed": 46, "range": (2500, 3500), "step": 100, "decimals": 0},
        },
    },
    "cooling-unit": {
        "metrics": {
            "temperature": {"seed": 47, "range": (8, 20),    "step": 0.5, "decimals": 2},
            "flow_rate":   {"seed": 48, "range": (30, 80),   "step": 3.0, "decimals": 1},
            "pressure":    {"seed": 49, "range": (2, 6),     "step": 0.2, "decimals": 2},
        },
    },
}

THRESHOLDS = {
    "temperature": 90,
    "pressure": 6,
    "voltage": 450,
    "vibration": 4.5,
    "current": 40,
    "cycle_time": 90,
    "load_kg": 400,
}

# Probability (per tick) of nudging a metric above its threshold to trigger
# a warning.  Set to 0 for no warnings.
WARNING_PROBABILITY = 0.02


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def clamp(value: float, lo: float, hi: float) -> float:
    return max(lo, min(hi, value))


def seed_value(device_id: str, metric_name: str, metric_cfg: dict) -> float:
    """Deterministic seed based on device + metric name, matching frontend
    MockTelemetrySource.seedState() logic."""
    h = 0
    key = f"{device_id}:{metric_name}"
    for ch in key:
        h = (h * 31 + ord(ch)) & 0xFFFFFFFF

    lo, hi = metric_cfg["range"]
    mid = (lo + hi) / 2

    if metric_name == "temperature":
        offset = (h % 60) - 30
        return clamp(60 + offset, lo, hi)
    elif metric_name == "pressure":
        offset = ((h >> 5) % 60) / 10 - 2
        return clamp(3 + offset, lo, hi)
    elif metric_name == "voltage":
        offset = ((h >> 3) % 80) - 40
        return clamp(400 + offset, lo, hi)
    elif metric_name == "rpm":
        spread = hi - lo
        fraction = (h % 100) / 100
        return lo + spread * fraction
    elif metric_name == "vibration":
        spread = hi - lo
        fraction = (h % 100) / 100
        return lo + spread * fraction * 0.4
    elif metric_name == "current":
        spread = hi - lo
        fraction = (h % 100) / 100
        return lo + spread * fraction
    elif metric_name == "load_kg":
        spread = hi - lo
        fraction = (h % 100) / 100
        return lo + spread * fraction * 0.6
    elif metric_name == "cycle_time":
        spread = hi - lo
        fraction = (h % 100) / 100
        return lo + spread * fraction
    elif metric_name == "flow_rate":
        spread = hi - lo
        fraction = (h % 100) / 100
        return lo + spread * fraction
    return mid


# ---------------------------------------------------------------------------
# Simulator
# ---------------------------------------------------------------------------

class TelemetrySimulator:
    def __init__(self, broker_host: str = "localhost", broker_port: int = 1883,
                 interval: float = 1.0, warning_prob: float = WARNING_PROBABILITY):
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        self.client.on_connect = self._on_connect
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.interval = interval
        self.warning_prob = warning_prob
        self.state: dict[str, dict[str, float]] = {}
        self._init_state()

    def _init_state(self):
        for device_id, device_cfg in DEVICES.items():
            self.state[device_id] = {}
            for metric_name, metric_cfg in device_cfg["metrics"].items():
                self.state[device_id][metric_name] = seed_value(
                    device_id, metric_name, metric_cfg
                )

    def _on_connect(self, client, userdata, flags, reason_code, properties):
        if reason_code == 0:
            print(f"Connected to MQTT broker at {
                  self.broker_host}:{self.broker_port}")
        else:
            print(f"Failed to connect, reason code: {reason_code}")

    def _tick(self):
        for device_id, device_cfg in DEVICES.items():
            for metric_name, metric_cfg in device_cfg["metrics"].items():
                current = self.state[device_id][metric_name]
                step = metric_cfg["step"]
                lo, hi = metric_cfg["range"]
                threshold = THRESHOLDS.get(metric_name)

                # Occasionally push above threshold
                if threshold and random.random() < self.warning_prob:
                    new_value = clamp(
                        threshold +
                        random.uniform(0.5, (hi - threshold) * 0.5),
                        lo, hi
                    )
                else:
                    new_value = clamp(
                        current + (random.random() - 0.5) * step,
                        lo, hi
                    )

                self.state[device_id][metric_name] = new_value
                value = round(new_value, metric_cfg["decimals"])

                topic = f"factory/{device_id}/{metric_name}"
                self.client.publish(topic, str(value))

                flag = " ⚠️" if (threshold and value > threshold) else ""
                print(f"  {topic} = {value}{flag}")

    def run(self):
        self.client.connect(self.broker_host, self.broker_port, 60)
        self.client.loop_start()

        print(f"Publishing telemetry every {
              self.interval}s. Ctrl+C to stop.\n")
        try:
            while True:
                self._tick()
                time.sleep(self.interval)
        except KeyboardInterrupt:
            print("\nStopping...")
        finally:
            self.client.loop_stop()
            self.client.disconnect()


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Factory 2Z MQTT telemetry simulator")
    parser.add_argument("--host", default="localhost", help="MQTT broker host")
    parser.add_argument("--port", type=int, default=1883,
                        help="MQTT broker port")
    parser.add_argument("--interval", type=float, default=1.0,
                        help="Seconds between publishes")
    parser.add_argument("--warning-prob", type=float, default=WARNING_PROBABILITY,
                        help="Probability per tick of exceeding a threshold (0 = no warnings)")
    args = parser.parse_args()

    sim = TelemetrySimulator(
        broker_host=args.host,
        broker_port=args.port,
        interval=args.interval,
        warning_prob=args.warning_prob,
    )
    sim.run()


if __name__ == "__main__":
    main()
