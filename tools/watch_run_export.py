#!/usr/bin/env python3
"""Poll an emulator's .sav and auto-export a newly finalized Run Report.

This is the "optional mGBA/host integration" from docs/SPEC.md's "Terminal
Run Reports": core ROM gameplay never depends on it, and manual
`python3 tools/export_run.py <save>` remains the portable fallback. This
script is emulator-agnostic - it only reads the .sav file mGBA (or any other
emulator) writes to disk, on whatever interval its own auto-save/periodic
flush uses, so a finalized report typically appears within a few seconds of
the in-game "Run Report saved" message.

Duplicate export protection is host-side, by report ID, tracked in a JSON
ledger next to the exported files - no gameplay state is mutated to
acknowledge export, and the same seed replayed twice still produces two
distinct report IDs (see include/run_report.h RunReport_Finalize()), so the
ledger never conflates two different attempts.

    python3 tools/watch_run_export.py <save-file> [-o DIR] [--interval 2.0] [--once]
"""

from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))
import export_run as er  # noqa: E402  (path insert must precede this import)

LEDGER_NAME = ".run_report_exported.json"


def fail(message: str) -> None:
    raise ValueError(message)


def load_ledger(out_dir: Path) -> dict[str, Any]:
    path = out_dir / LEDGER_NAME
    if not path.is_file():
        return {"exported": []}
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
        if isinstance(data, dict) and isinstance(data.get("exported"), list):
            return data
    except (OSError, ValueError):
        pass
    return {"exported": []}


def save_ledger(out_dir: Path, ledger: dict) -> None:
    path = out_dir / LEDGER_NAME
    out_dir.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(ledger, indent=2) + "\n", encoding="utf-8")


def try_export_once(save_path: Path, out_dir: Path, repo: Path | None, ledger: dict) -> bool:
    """Returns True if a new report was exported."""
    try:
        sav = save_path.read_bytes()
        bank = er.unpack_bank(sav)
    except (OSError, ValueError):
        return False  # no report yet, or the save is mid-write - try again later

    names = er.NameTables(repo)
    exported_any = False
    for slot in range(bank["slots_used"]):
        try:
            doc = er.unpack_record(bank["records_raw"][slot], names)
        except ValueError:
            continue  # corrupt or foreign-schema slot - never claim success for it
        report_id = doc["run"]["report_id"]
        if report_id in ledger["exported"]:
            continue

        seed_hex = doc["run"]["seed"][2:]
        result = doc["outcome"]["result"]
        out_dir.mkdir(parents=True, exist_ok=True)
        path = out_dir / f"run_{seed_hex}_{result}.json"
        if path.exists():
            path = out_dir / f"run_{seed_hex}_{result}.{report_id[2:]}.json"
        path.write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")
        ledger["exported"].append(report_id)
        print(f"exported {report_id} -> {path}")
        exported_any = True

    if exported_any:
        save_ledger(out_dir, ledger)
    return exported_any


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("save", type=Path, help="the emulator's .sav file (polled, never modified)")
    parser.add_argument("-o", "--out-dir", type=Path, default=Path("."), help="directory for exported JSON + ledger")
    parser.add_argument("--interval", type=float, default=2.0, help="poll interval in seconds (default: 2.0)")
    parser.add_argument("--once", action="store_true", help="check once and exit instead of polling forever")
    parser.add_argument("--json-only", action="store_true", help="skip repository name-expansion")
    parser.add_argument("--repo", type=Path, default=None, help="repository root for name expansion")
    args = parser.parse_args()

    repo = None
    if not args.json_only:
        repo = args.repo if args.repo is not None else Path(__file__).resolve().parent.parent
        if not (repo / "include" / "run_report.h").is_file():
            repo = None

    ledger = load_ledger(args.out_dir)
    last_size = None
    last_mtime = None

    try:
        while True:
            if not args.save.is_file():
                if args.once:
                    fail(f"{args.save} does not exist")
            else:
                stat = args.save.stat()
                changed = (stat.st_size, stat.st_mtime) != (last_size, last_mtime)
                if changed or args.once:
                    last_size, last_mtime = stat.st_size, stat.st_mtime
                    exported = try_export_once(args.save, args.out_dir, repo, ledger)
                    if args.once:
                        if not exported:
                            print("already exported (or no report present)")
                        return 0
            time.sleep(args.interval)
    except KeyboardInterrupt:
        return 0
    except (OSError, ValueError) as error:
        print(f"watch_run_export.py: error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
