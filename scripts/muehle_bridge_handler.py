#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import subprocess
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any

"""
Simple HTTP wrapper around MuehleBridge for Odoo remote mode.
"""

BASE_DIR = Path(__file__).resolve().parent
DB_PATH = (BASE_DIR / ".." / "database" / "database.dat").resolve()

class MuehleBridgeHandler(BaseHTTPRequestHandler):
    bridge_command: list[str] = []
    timeout_seconds: int = 100

    def _send_json(self, status: int, payload: dict[str, Any]) -> None:
        body = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.send_header("Access-Control-Allow-Methods", "POST, OPTIONS")
        self.end_headers()
        self.wfile.write(body)

    def _stream_file(self, filepath: Path) -> None:
        """ Stream the database file """
        if not filepath.is_file():
            self._send_json(404, {"success": False, "error": "Database file not found on server."})
            return

        try:
            file_size = filepath.stat().st_size
            self.send_response(200)
            self.send_header("Content-Type", "application/octet-stream")
            self.send_header("Content-Disposition", f"attachment; filename={filepath.name}")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.send_header("Access-Control-Allow-Methods", "GET, OPTIONS")
            self.end_headers()

            """ Read and write in 1MB chunks to avoid memory issues """
            chunk_size = 1024 * 1024
            with open(filepath, "rb") as file:
                while chunk := file.read(chunk_size):
                    self.wfile.write(chunk)
        except (ClientDisconnectedError, BrokenPipeError, ConnectionResetError):
            """ Client closed the connection early during download """
            pass
        except Exception as ex:
            """ Avoid sending JSON headers if already sent """
            print(f"Error serving downlaod: {ex}")

    def do_OPTIONS(self) -> None:  # noqa: N802
        self._send_json(200, {"ok": True})
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.send_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS")
        self.end_headers()

    def do_GET(self) -> None:  # noqa: N802
        if self.path == "/health":
            self._send_json(200, {"ok": True})
            return
        elif self.path == "/download":
            self._stream_file(DB_PATH)
            return

        self._send_json(404, {"success": False, "error": "Not found"})

    def do_POST(self) -> None:  # noqa: N802
        if self.path != "/evaluate":
            self._send_json(404, {"success": False, "error": "Not found"})
            return

        try:
            content_length = int(self.headers.get("Content-Length", "0"))
            raw_body = self.rfile.read(content_length)
            payload = json.loads(raw_body.decode("utf-8"))
        except Exception as ex:  # pragma: no cover
            self._send_json(400, {"success": False,
                                  "error": f"Invalid JSON body: {ex}"})
            return

        try:
            completed = subprocess.run(
                self.bridge_command,
                input=json.dumps(payload),
                text=True,
                capture_output=True,
                timeout=self.timeout_seconds,
                check=False,
            )
        except Exception as ex:  # pragma: no cover
            self._send_json(500, {"success": False,
                                  "error": f"Bridge process failed: {ex}"})
            return

        stdout = (completed.stdout or "").strip()
        if not stdout:
            self._send_json(500, {
                "success": False,
                "error": completed.stderr or "Bridge produced no output.",
            })
            return

        try:
            bridge_payload = json.loads(stdout)
        except json.JSONDecodeError:
            self._send_json(500, {
                "success": False,
                "error": completed.stderr or "Bridge returned invalid JSON.",
            })
            return

        self._send_json(200, bridge_payload)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Expose MuehleBridge as HTTP /evaluate endpoint")
    parser.add_argument("--bridge", required=True,
                        help="Path to MuehleBridge executable")
    parser.add_argument("--host", default="127.0.0.1", help="Bind host")
    parser.add_argument("--port", type=int, default=8787, help="Bind port")
    parser.add_argument("--timeout", type=int, default=10,
                        help="Bridge process timeout in seconds")
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    MuehleBridgeHandler.bridge_command = [args.bridge]
    MuehleBridgeHandler.timeout_seconds = args.timeout

    server = ThreadingHTTPServer((args.host, args.port), MuehleBridgeHandler)
    print(f"Muehle bridge service listening on http://{args.host}:{args.port}")
    print("POST /evaluate  | GET /health | GET /download")
    server.serve_forever()


if __name__ == "__main__":
    main()
