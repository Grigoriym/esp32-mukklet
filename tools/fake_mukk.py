#!/usr/bin/env python3
"""Fake Mukk: plays the Mukk side of docs/PROTOCOL.md against the display.

The counterpart of fake_display.py: tests the firmware without Mukk.
Standard library only. Connects to ws://<host>/ws, waits for `hello`, sends
a track and state, then a state every 5 s, and obeys the knob's `cmd`s
(play/pause, next/prev through a small playlist, volume, seek), printing
each one.

    python3 tools/fake_mukk.py                      # mukklet.local, until Ctrl-C
    python3 tools/fake_mukk.py --host 192.168.1.50 --seconds 60

The playlist has a Cyrillic title, a long one that has to scroll, a track
without tags and one of unknown duration.
"""

import argparse
import base64
import json
import os
import socket
import struct
import time

PLAYLIST = [
    {"title": "Paranoid Android", "artist": "Radiohead", "album": "OK Computer", "year": 1997,
     "durationMs": 386000},
    {"title": "Группа крови", "artist": "Кино", "album": "Группа крови", "year": 1988, "durationMs": 285000},
    {"title": "The Great Gig in the Sky (2011 Remastered Version)", "artist": "Pink Floyd",
     "album": "The Dark Side of the Moon", "year": 1973, "durationMs": 283000},
    {"title": "track07", "artist": "", "album": "", "year": 0, "durationMs": 95000},
    {"title": "Internet radio", "artist": "Somebody", "album": "", "year": 0, "durationMs": 0},
]
HEARTBEAT_S = 5


class Link:
    def __init__(self, host, port):
        self.sock = socket.create_connection((host, port), timeout=10)
        key = base64.b64encode(os.urandom(16)).decode()
        self.sock.sendall((f"GET /ws HTTP/1.1\r\nHost: {host}\r\nUpgrade: websocket\r\n"
                           f"Connection: Upgrade\r\nSec-WebSocket-Key: {key}\r\n"
                           "Sec-WebSocket-Version: 13\r\n\r\n").encode())
        head = b""
        while b"\r\n\r\n" not in head:
            chunk = self.sock.recv(1024)
            if not chunk:
                raise ConnectionError("closed during handshake")
            head += chunk
        status = head.split(b"\r\n", 1)[0]
        if b" 101 " not in status:
            raise ConnectionError(f"handshake refused: {status.decode(errors='replace')}")
        self.buf = head.split(b"\r\n\r\n", 1)[1]

    def _read(self, n):
        while len(self.buf) < n:
            chunk = self.sock.recv(4096)
            if not chunk:
                raise ConnectionError("display closed the connection")
            self.buf += chunk
        out, self.buf = self.buf[:n], self.buf[n:]
        return out

    def recv(self):
        """Next text message as a dict, or None for other frames."""
        b0, b1 = self._read(2)
        length = b1 & 0x7F
        if length == 126:
            (length,) = struct.unpack(">H", self._read(2))
        elif length == 127:
            (length,) = struct.unpack(">Q", self._read(8))
        payload = self._read(length)  # server frames are unmasked
        opcode = b0 & 0x0F
        if opcode == 0x8:
            raise ConnectionError("display sent close")
        if opcode == 0x9:
            self._send(0xA, payload)  # pong
            return None
        return json.loads(payload) if opcode == 0x1 else None

    def _send(self, opcode, payload):
        mask = os.urandom(4)
        n = len(payload)
        head = bytes([0x80 | opcode])
        if n < 126:
            head += bytes([0x80 | n])
        elif n < 65536:
            head += bytes([0x80 | 126]) + struct.pack(">H", n)
        else:
            head += bytes([0x80 | 127]) + struct.pack(">Q", n)
        self.sock.sendall(head + mask + bytes(b ^ mask[i % 4] for i, b in enumerate(payload)))

    def send(self, obj):
        self._send(0x1, json.dumps(obj, ensure_ascii=False).encode())


class Player:
    def __init__(self):
        self.index = 0
        self.status = "playing"
        self.position_ms = 0
        self.position_at = time.monotonic()
        self.volume = 50

    def position(self):
        pos = self.position_ms
        if self.status == "playing":
            pos += int((time.monotonic() - self.position_at) * 1000)
        dur = PLAYLIST[self.index]["durationMs"]
        return min(pos, dur) if dur else pos

    def set_position(self, ms):
        self.position_ms = max(0, ms)
        self.position_at = time.monotonic()

    def track_msg(self):
        t = PLAYLIST[self.index]
        n = PLAYLIST[(self.index + 1) % len(PLAYLIST)]
        track = dict(t, id=f"t{self.index}", albumArtist=t["artist"], genre="", trackNo=self.index + 1,
                     format="FLAC", hasCover=False)
        return {"type": "track", "track": track, "next": {"title": n["title"], "artist": n["artist"]}}

    def state_msg(self):
        return {"type": "state", "status": self.status, "positionMs": self.position(), "volume": self.volume,
                "repeat": "all", "shuffle": False}

    def apply(self, cmd):
        """Returns True if the track changed."""
        kind = cmd.get("cmd")
        if kind == "play_pause":
            self.set_position(self.position())
            self.status = "paused" if self.status == "playing" else "playing"
        elif kind in ("next", "prev"):
            self.index = (self.index + (1 if kind == "next" else -1)) % len(PLAYLIST)
            self.set_position(0)
            return True
        elif kind == "volume":
            self.volume = max(0, min(100, self.volume + int(cmd.get("delta", 0))))
        elif kind == "seek":
            self.set_position(self.position() + int(cmd.get("deltaMs", 0)))
        return False


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--host", default="mukklet.local")
    ap.add_argument("--port", type=int, default=80)
    ap.add_argument("--seconds", type=float, default=0, help="stop after this long (0 = run until Ctrl-C)")
    args = ap.parse_args()

    link = Link(args.host, args.port)
    link.sock.settimeout(1)  # wake up for the heartbeat
    hello = link.recv()
    print(f"<- {json.dumps(hello, ensure_ascii=False)}", flush=True)
    if not hello or hello.get("type") != "hello":
        raise SystemExit("expected hello first")

    player = Player()
    link.send(player.track_msg())
    link.send(player.state_msg())
    start = last_state = time.monotonic()
    try:
        while not args.seconds or time.monotonic() - start < args.seconds:
            try:
                msg = link.recv()
            except socket.timeout:
                msg = None
            if msg and msg.get("type") == "cmd":
                changed = player.apply(msg)
                print(f"<- {json.dumps(msg)}  => {player.status}, vol {player.volume}, "
                      f"{PLAYLIST[player.index]['title']}", flush=True)
                if changed:
                    link.send(player.track_msg())
                link.send(player.state_msg())
                last_state = time.monotonic()
            elif msg:
                print(f"<- {json.dumps(msg, ensure_ascii=False)}", flush=True)
            if time.monotonic() - last_state >= HEARTBEAT_S:
                link.send(player.state_msg())
                last_state = time.monotonic()
    except KeyboardInterrupt:
        pass
    link.sock.close()


if __name__ == "__main__":
    main()
