#!/usr/bin/env python3
import json
import subprocess
import sys
from pathlib import Path


def main() -> None:
    delegate = Path(__file__).with_name("summarize_permuted_terminal_pair_yield_252.py")
    command = [sys.executable, str(delegate), *sys.argv[1:]]
    completed = subprocess.run(command, check=True, capture_output=True, text=True)
    summary = json.loads(completed.stdout)
    summary["experiment"] = "ATTR-PERMUTED-TERMINAL-PAIR-YIELD-252-v2"
    print(json.dumps(summary, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
