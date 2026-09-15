"""Frozen, loopback-only research transport for the unchanged HTTP executable.

This is not an official BTC server. Fixed roles are explicit synthetic resume
input; daily planning remains the actual executable's HTTP path.
"""
import argparse
import hashlib
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import os
from pathlib import Path
import subprocess
import threading
import time

ROOT = Path(__file__).resolve().parents[2]
EMPTY_LEDGER = {"brands": [], "totalDailyDistinct": 0, "totalServings": 0}


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest().upper()


def append_event(path, kind, **fields):
    with Path(path).open("a", encoding="utf-8", newline="\n") as stream:
        stream.write(json.dumps({"kind": kind, **fields}, sort_keys=True) + "\n")


class Bridge:
    def __init__(self, executable):
        self.process = subprocess.Popen([str(executable)], stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))

    def request(self, request):
        self.process.stdin.write(json.dumps(request) + "\n")
        self.process.stdin.flush()
        response = self.process.stdout.readline()
        if not response:
            raise RuntimeError("fixture/validator bridge disappeared")
        result = json.loads(response)
        return result

    def close(self):
        self.process.stdin.close()
        self.process.wait(timeout=10)
        error = self.process.stderr.read()
        if self.process.returncode or error:
            raise RuntimeError("bridge error: " + error)


class CaseState:
    def __init__(self, setup, bridge, clock=time.monotonic, wall=time.time):
        self.setup, self.bridge, self.clock, self.wall = setup, bridge, clock, wall
        self.day = 0
        self.agents = [{"kind": 0, "pos": cell, "fuel": setup["fuelLimits"]} for cell in setup["agents"]]
        self.ledger = dict(EMPTY_LEDGER)
        self.deadline = None
        self.ends_at = None
        self.pending = None
        self.plan = None
        self.actions = []
        self.requests = []
        self.failure = None
        self.last_state = None
        self.score = [0, 0, 0]

    def advance(self):
        if self.deadline is not None and self.clock() >= self.deadline:
            if self.pending is None:
                raise RuntimeError("day expired without an accepted dual-valid action")
            self.agents, self.ledger, self.score = self.pending["agents"], self.pending["ledger"], self.pending["score"]
            self.day += 1
            self.deadline = self.ends_at = self.pending = self.plan = None

    def get(self, endpoint):
        self.advance()
        if endpoint == "setup":
            return 200, self.setup
        if endpoint == "start":
            return 200, {"synthetic": True, "started": True}
        if endpoint == "result":
            if self.day < len(self.setup["daySteps"]):
                return 425, {"reason": "synthetic-day-open"}
            return 200, {"synthetic": True, "standings": [], "score": self.score}
        if endpoint != "state":
            return 404, {"reason": "unknown-endpoint"}
        if self.day == len(self.setup["daySteps"]):
            return 404, {"reason": "synthetic-complete"}
        if self.pending is not None:
            return 425, {"reason": "synthetic-await-next-day"}
        if self.deadline is None:
            self.deadline = self.clock() + 5.0
            self.ends_at = int(self.wall() * 1000) + 5000
            self.last_state = {"day": self.day, "endsAt": self.ends_at,
                "agents": self.agents, "others": [], "traffics": []}
        return 200, self.last_state

    def post(self, endpoint, plan):
        # Never accept a late action into the next day's state.
        if endpoint != "actions" or self.deadline is None:
            raise RuntimeError("unexpected assignment or action without delivered state")
        if self.clock() >= self.deadline:
            raise RuntimeError("action arrived after its authoritative synthetic deadline")
        if self.pending is not None:
            if plan != self.plan:
                raise RuntimeError("non-idempotent repeated submission")
            return 200, {"valid": True, "day": self.day + 1, "synthetic": True}
        state = dict(self.last_state, day=self.day + 1, endsAt=self.ends_at // 1000)
        checked = self.bridge.request({"op": "step", "setup": self.setup,
            "state": state, "ledger": self.ledger, "plan": plan})
        if not checked.get("ok") or not checked.get("agrees"):
            raise RuntimeError("submitted action failed dual validation: " + str(checked))
        self.plan, self.pending = plan, checked
        self.actions.append({"wire_day": self.day, "state": self.last_state, "plan": plan,
            "validated": checked, "arrival_ms": round((self.clock() - (self.deadline - 5)) * 1000, 3),
            "deadline_margin_ms": round((self.deadline - self.clock()) * 1000, 3)})
        return 200, {"valid": True, "day": self.day + 1, "synthetic": True}


def handler_for(case, match_id):
    class Handler(BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"

        def log_message(self, *args):
            pass  # No request headers or credentials are persisted.

        def respond(self, method):
            try:
                prefix = f"/api/v1/matches/{match_id}/"
                if not self.path.startswith(prefix):
                    raise RuntimeError("unexpected request path")
                endpoint = self.path[len(prefix):]
                if method == "GET":
                    status, body = case.get(endpoint)
                else:
                    size = int(self.headers.get("Content-Length", "0"))
                    if not 0 < size <= 1024 * 1024:
                        raise RuntimeError("invalid action body size")
                    status, body = case.post(endpoint, json.loads(self.rfile.read(size)))
                case.requests.append({"method": method, "endpoint": endpoint,
                    "status": status, "wire_day": case.day, "at_ms": int(case.wall() * 1000)})
            except Exception as error:
                case.failure = str(error)
                status, body = 500, {"reason": "synthetic-harness-failure"}
            raw = json.dumps(body).encode()
            self.send_response(status)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(raw)))
            self.send_header("Connection", "close")
            self.end_headers()
            self.wfile.write(raw)

        def do_GET(self):
            self.respond("GET")

        def do_POST(self):
            self.respond("POST")
    return Handler


def run_case(spec, setup, manifest, outdir, bridge):
    seed = spec["seed"]
    prefix = outdir / str(seed)
    replay = prefix.with_suffix(".replay.jsonl")
    # Fixed all-Patrol lane, not role-selection performance. No accepted days are
    # fabricated: these two events only supply the pre-match role assignment.
    with replay.open("x", encoding="utf-8", newline="\n") as stream:
        for kind, body in (("synthetic_fixture", {"experiment": manifest["experiment"], "seed": seed}),
                           ("assignment", [0, 0, 0]),
                           ("assignment_result", {"valid": True, "synthetic": True})):
            stream.write(json.dumps({"kind": kind, "body": body, "atUnixMs": int(time.time()*1000), "status": 200}) + "\n")
    case = CaseState(setup, bridge)
    match_id = f"m-{seed}"
    server = HTTPServer(("127.0.0.1", 0), handler_for(case, match_id))
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    env = os.environ.copy()
    env["HEXUDON_TOKEN"] = "synthetic-loopback-only-no-credential"
    command = [str(ROOT / manifest["btc_binary"]), "http", "--url",
        f"http://127.0.0.1:{server.server_port}", "--match", match_id,
        "--response-ms", "5000", "--poll-ms", "220", "--replay", str(replay)]
    try:
        with prefix.with_suffix(".stdout").open("x") as stdout, prefix.with_suffix(".stderr").open("x") as stderr:
            process = subprocess.Popen(command, stdout=stdout, stderr=stderr, env=env,
                creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
            # Bound transport-harness failure; preserve the process rather than
            # killing or blindly restarting a potentially accepted day.
            return_code = process.wait(timeout=90)
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=5)
    transport = {"synthetic": True, "seed": seed, "setup": setup,
        "actions": case.actions, "requests": case.requests, "failure": case.failure, "score": case.score}
    transport_path = prefix.with_suffix(".transport.json")
    transport_path.write_text(json.dumps(transport, indent=2, sort_keys=True)+"\n", encoding="utf-8")
    if return_code or case.failure or prefix.with_suffix(".stderr").stat().st_size:
        raise RuntimeError(f"case {seed} failed; evidence preserved, no resume authorized")
    if case.day != 4 or len(case.actions) != 4:
        raise RuntimeError("incomplete lifecycle")
    from summarize_http_baseline_314 import safety
    safety(replay)  # Registered operational checks only, never partial scores.
    checked = subprocess.run([str(ROOT / manifest["btc_binary"]), "replay-check", "--replay", str(replay)],
        capture_output=True, text=True, timeout=30, creationflags=getattr(subprocess,"CREATE_NO_WINDOW",0))
    prefix.with_suffix(".replay-check.txt").write_text(checked.stdout + checked.stderr, encoding="utf-8")
    if checked.returncode or "summary days=4 reconciled_transitions=3" not in checked.stdout:
        raise RuntimeError("replay-check failed to reconcile all four days")
    result = {"kind": "case_complete", **spec, "http_score": case.score,
        "replay_sha256": digest(replay), "transport_sha256": digest(transport_path),
        "replay_check_sha256": digest(prefix.with_suffix(".replay-check.txt")),
        "actions": 4, "transitions": 3, "failure": None}
    result_path = prefix.with_suffix(".result.json")
    with result_path.open("x", encoding="utf-8") as stream:
        json.dump(result, stream, indent=2, sort_keys=True)
        stream.write("\n")
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    for path, expected in manifest["hashes"].items():
        if digest(ROOT / path) != expected:
            raise RuntimeError("frozen input mismatch: " + path)
    args.output.mkdir(parents=True, exist_ok=False)
    bridge = Bridge(ROOT / manifest["bridge_binary"])
    try:
        for spec, setup in zip(manifest["cases"], manifest["setups"], strict=True):
            actual = bridge.request({"op": "fixture", **spec})
            if not actual.get("ok") or actual["setup"] != setup:
                raise RuntimeError("fixture identity mismatch before starting the bot")
        for spec, setup in zip(manifest["cases"], manifest["setups"], strict=True):
            run_case(spec, setup, manifest, args.output, bridge)
            print(f"case_complete seed={spec['seed']}", flush=True)
        for path, expected in manifest["hashes"].items():
            if digest(ROOT / path) != expected:
                raise RuntimeError("frozen input changed during run: " + path)
        (args.output / "run_complete.json").write_text(json.dumps({"cases": 12,
            "manifest_sha256": digest(args.manifest)}, indent=2)+"\n", encoding="utf-8")
        print("run_complete cases=12", flush=True)
    finally:
        bridge.close()


if __name__ == "__main__":
    main()
