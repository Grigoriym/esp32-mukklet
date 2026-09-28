#!/usr/bin/env python3
"""Fake Mukklet: plays the ESP32 side of docs/PROTOCOL.md on the PC.

Lets Mukk's display link be built and tested without the hardware. Standard
library only (no pip install). Listens on ws://localhost:<port>/ws, sends
`hello`, prints every message Mukk sends, checks it against the protocol,
shows mono1 covers as text art and saves every cover as a PNG.

    python3 tools/fake_display.py                  # mono1 64x64, like the OLED
    python3 tools/fake_display.py --format rgb565 --size 160   # like the TFT

In Mukk, set the display host to `localhost:8765`.

Keys (type, then Enter): p play/pause, n next, b prev, + / - volume,
f / r seek +/-10 s, q quit.
"""

import argparse
import asyncio
import base64
import hashlib
import json
import os
import struct
import sys
import time
import zlib

WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
HEARTBEAT_TIMEOUT_S = 15
STATUSES = {"playing", "paused", "stopped", "idle"}
REPEATS = {"off", "one", "all"}

KEY_CMDS = {
    "p": {"cmd": "play_pause"},
    "n": {"cmd": "next"},
    "b": {"cmd": "prev"},
    "+": {"cmd": "volume", "delta": 5},
    "-": {"cmd": "volume", "delta": -5},
    "f": {"cmd": "seek", "deltaMs": 10000},
    "r": {"cmd": "seek", "deltaMs": -10000},
}


def warn(msg):
    print(f"  !! PROTOCOL: {msg}")


# --- WebSocket framing (RFC 6455, just what this needs) ---------------------


async def read_frame(reader):
    b0, b1 = await reader.readexactly(2)
    opcode = b0 & 0x0F
    fin = bool(b0 & 0x80)
    masked = bool(b1 & 0x80)
    length = b1 & 0x7F
    if length == 126:
        (length,) = struct.unpack(">H", await reader.readexactly(2))
    elif length == 127:
        (length,) = struct.unpack(">Q", await reader.readexactly(8))
    mask = await reader.readexactly(4) if masked else None
    data = await reader.readexactly(length)
    if mask:
        data = bytes(b ^ mask[i % 4] for i, b in enumerate(data))
    if not masked:
        warn("client frame not masked (RFC 6455 requires it)")
    return fin, opcode, data


async def read_message(reader, writer):
    """Returns (opcode, payload) of the next data message; answers pings."""
    parts, first_opcode = [], None
    while True:
        fin, opcode, data = await read_frame(reader)
        if opcode == 0x9:  # ping
            send_frame(writer, 0xA, data)
            continue
        if opcode == 0xA:  # pong
            continue
        if opcode == 0x8:  # close
            return 0x8, data
        if opcode != 0x0:
            first_opcode = opcode
        parts.append(data)
        if fin:
            return first_opcode, b"".join(parts)


def send_frame(writer, opcode, payload):
    header = bytes([0x80 | opcode])
    n = len(payload)
    if n < 126:
        header += bytes([n])
    elif n < 65536:
        header += bytes([126]) + struct.pack(">H", n)
    else:
        header += bytes([127]) + struct.pack(">Q", n)
    writer.write(header + payload)


def send_json(writer, obj):
    print(f"<- {json.dumps(obj)}")
    send_frame(writer, 0x1, json.dumps(obj).encode())


# --- Cover output -----------------------------------------------------------


def png_bytes(w, h, rgb_rows):
    raw = b"".join(b"\x00" + bytes(row) for row in rgb_rows)

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data))

    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(raw))
        + chunk(b"IEND", b"")
    )


def decode_pixels(fmt, w, h, data):
    """-> list of rows of (r, g, b)."""
    rows = []
    for y in range(h):
        row = []
        for x in range(w):
            if fmt == "mono1":
                byte = data[(y * w + x) // 8]
                v = 255 if byte & (0x80 >> (x % 8)) else 0
                row.append((v, v, v))
            else:  # rgb565, big-endian
                (p,) = struct.unpack_from(">H", data, (y * w + x) * 2)
                row.append((((p >> 11) & 0x1F) * 255 // 31, ((p >> 5) & 0x3F) * 255 // 63, (p & 0x1F) * 255 // 31))
        rows.append(row)
    return rows


def print_mono_art(w, h, rows):
    # Two pixel rows per text line with half blocks, so the art keeps its shape.
    for y in range(0, h, 2):
        line = ""
        for x in range(w):
            top = rows[y][x][0] > 0
            bottom = y + 1 < h and rows[y + 1][x][0] > 0
            line += "█" if top and bottom else "▀" if top else "▄" if bottom else " "
        print("   " + line)


def save_cover(out_dir, track_id, fmt, w, h, data):
    rows = decode_pixels(fmt, w, h, data)
    if fmt == "mono1":
        print_mono_art(w, h, rows)
    path = os.path.join(out_dir, f"cover_{track_id}_{fmt}_{w}x{h}.png")
    flat = [[c for px in row for c in px] for row in rows]
    with open(path, "wb") as f:
        f.write(png_bytes(w, h, flat))
    print(f"   cover saved to {path}")


# --- Protocol checks --------------------------------------------------------


def check_track(msg):
    track = msg.get("track", "MISSING")
    if track == "MISSING":
        warn("`track` message without a `track` field (use null for none)")
        return
    if track is None:
        return
    for key, typ in [("id", str), ("title", str), ("artist", str), ("album", str), ("albumArtist", str),
                     ("genre", str), ("year", int), ("trackNo", int), ("durationMs", int), ("format", str),
                     ("hasCover", bool)]:
        if not isinstance(track.get(key), typ):
            warn(f"track.{key} missing or not {typ.__name__}")
    if track.get("title") == "":
        warn("track.title is empty (use the file name)")
    nxt = msg.get("next", "MISSING")
    if nxt == "MISSING":
        warn("`next` missing (use null when unknown)")


def check_state(msg):
    if msg.get("status") not in STATUSES:
        warn(f"state.status {msg.get('status')!r} not one of {sorted(STATUSES)}")
    if not isinstance(msg.get("positionMs"), int):
        warn("state.positionMs missing or not int")
    vol = msg.get("volume")
    if not isinstance(vol, int) or not 0 <= vol <= 100:
        warn(f"state.volume {vol!r} not an int 0-100")
    if msg.get("repeat") not in REPEATS:
        warn(f"state.repeat {msg.get('repeat')!r} not one of {sorted(REPEATS)}")
    if not isinstance(msg.get("shuffle"), bool):
        warn("state.shuffle missing or not bool")


# --- Session ----------------------------------------------------------------


class Session:
    def __init__(self, args):
        self.args = args
        self.writer = None
        self.last_rx = time.monotonic()

    async def handle(self, reader, writer):
        if self.writer is not None:
            print("-- new client, closing the old one (one client at a time)")
            self.writer.close()
        request = await reader.readuntil(b"\r\n\r\n")
        headers = {}
        for line in request.decode(errors="replace").split("\r\n")[1:]:
            if ":" in line:
                k, v = line.split(":", 1)
                headers[k.strip().lower()] = v.strip()
        path = request.split(b" ")[1].decode(errors="replace")
        if path != "/ws":
            warn(f"client asked for {path!r}, expected '/ws'")
        accept = base64.b64encode(hashlib.sha1((headers.get("sec-websocket-key", "") + WS_GUID).encode()).digest())
        writer.write(b"HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
                     b"Sec-WebSocket-Accept: " + accept + b"\r\n\r\n")
        self.writer = writer
        print(f"-- Mukk connected from {writer.get_extra_info('peername')}")

        cover_fmt = self.args.format
        hello = {"type": "hello", "v": 1, "device": "fake-mukklet", "maxChunk": self.args.max_chunk,
                 "cover": {"w": self.args.size, "h": self.args.size, "format": cover_fmt}}
        send_json(writer, hello)
        self.last_rx = time.monotonic()

        pending_cover = None  # (meta, bytearray)
        current_track_id = None
        try:
            while True:
                opcode, data = await read_message(reader, writer)
                self.last_rx = time.monotonic()
                if opcode == 0x8:
                    print("-- Mukk closed the connection")
                    break
                if opcode == 0x2:
                    if pending_cover is None:
                        warn(f"binary frame ({len(data)} B) without a preceding `cover` message")
                        continue
                    meta, buf = pending_cover
                    if len(data) > self.args.max_chunk:
                        warn(f"binary frame of {len(data)} B exceeds maxChunk {self.args.max_chunk}")
                    buf.extend(data)
                    if len(buf) >= meta["size"]:
                        if len(buf) > meta["size"]:
                            warn(f"got {len(buf)} cover bytes, `size` said {meta['size']}")
                        save_cover(self.args.out, meta["trackId"], meta["format"], meta["w"], meta["h"], bytes(buf))
                        pending_cover = None
                    continue

                try:
                    msg = json.loads(data.decode("utf-8"))
                except (UnicodeDecodeError, json.JSONDecodeError) as e:
                    warn(f"text frame is not UTF-8 JSON: {e}")
                    continue
                if pending_cover is not None:
                    warn("text message arrived before all cover chunks (covers must not be interleaved)")
                    pending_cover = None
                kind = msg.get("type")
                if kind == "state":
                    print(f"-> state {msg.get('status')} pos {msg.get('positionMs')} vol {msg.get('volume')} "
                          f"repeat {msg.get('repeat')} shuffle {msg.get('shuffle')}")
                    check_state(msg)
                    continue
                print(f"-> {json.dumps(msg, ensure_ascii=False)}")
                if kind == "track":
                    check_track(msg)
                    t = msg.get("track")
                    current_track_id = t.get("id") if isinstance(t, dict) else None
                elif kind == "cover":
                    if cover_fmt == "none":
                        warn("cover sent although hello said format none")
                    if msg.get("trackId") != current_track_id:
                        warn(f"cover.trackId {msg.get('trackId')!r} != current track {current_track_id!r}")
                    if msg.get("none"):
                        continue
                    w, h, fmt = msg.get("w"), msg.get("h"), msg.get("format")
                    if (w, h, fmt) != (self.args.size, self.args.size, cover_fmt):
                        warn(f"cover is {w}x{h} {fmt}, hello asked for {self.args.size}x{self.args.size} {cover_fmt}")
                    expected = w * h // 8 if fmt == "mono1" else w * h * 2
                    if msg.get("size") != expected:
                        warn(f"cover.size {msg.get('size')} != {expected} for {w}x{h} {fmt}")
                    pending_cover = (msg, bytearray())
                else:
                    warn(f"unknown message type {kind!r} (fine to ignore, but unexpected from Mukk v1)")
        except (asyncio.IncompleteReadError, ConnectionError):
            print("-- connection dropped")
        finally:
            if self.writer is writer:
                self.writer = None

    async def watchdog(self):
        warned = False
        while True:
            await asyncio.sleep(1)
            silent = time.monotonic() - self.last_rx
            if self.writer is not None and silent > HEARTBEAT_TIMEOUT_S and not warned:
                warn(f"nothing from Mukk for {silent:.0f} s (state heartbeat is every 5 s): display would say offline")
                warned = True
            elif silent <= HEARTBEAT_TIMEOUT_S:
                warned = False

    async def keyboard(self):
        loop = asyncio.get_running_loop()
        while True:
            line = (await loop.run_in_executor(None, sys.stdin.readline))
            if not line:
                return
            key = line.strip()
            if key == "q":
                os._exit(0)
            cmd = KEY_CMDS.get(key)
            if cmd is None:
                print("keys: p play/pause, n next, b prev, + / - volume, f / r seek, q quit")
            elif self.writer is None:
                print("-- no client connected")
            else:
                send_json(self.writer, {"type": "cmd", **cmd})


async def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--format", choices=["mono1", "rgb565", "none"], default="mono1")
    parser.add_argument("--size", type=int, default=64, help="cover width = height in pixels")
    parser.add_argument("--max-chunk", type=int, default=4096)
    parser.add_argument("--out", default="/tmp/fake_display", help="where cover PNGs go")
    args = parser.parse_args()
    os.makedirs(args.out, exist_ok=True)

    session = Session(args)
    server = await asyncio.start_server(session.handle, "0.0.0.0", args.port)
    print(f"fake display on ws://localhost:{args.port}/ws, cover {args.size}x{args.size} {args.format}")
    print("keys: p play/pause, n next, b prev, + / - volume, f / r seek, q quit")
    async with server:
        await asyncio.gather(server.serve_forever(), session.watchdog(), session.keyboard())


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
