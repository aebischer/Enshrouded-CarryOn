"""Extract local test fixtures from KFC4; never modify or distribute game data.

Usage: python extract-prop-fixtures.py GAME_DIRECTORY OUTPUT_FILE
Requires zstandard. The output is for offline validation only, not the mod package.
"""
import struct
import sys
from pathlib import Path
import zstandard

game, output = map(Path, sys.argv[1:])
header = (game / "enshrouded.kfc").read_bytes()


def array(data, field):
    offset, count = struct.unpack_from("<II", data, field)
    return field + offset, count


keys, count = array(header, 0x60)
values, value_count = array(header, 0x68)
chunks, chunk_count = array(header, 0x88)
assert count == value_count
resources = []
for index in range(count):
    key = header[keys + index * 32:keys + (index + 1) * 32]
    kind, part = struct.unpack_from("<II", key, 16)
    if kind == 0x39768775 and part == 0:
        start, size = struct.unpack_from("<II", header, values + index * 8)
        resources.append((start, size, key[:16]))

result = bytearray()
props = 0
with (game / "enshrouded.kfc_resources").open("rb") as stream:
    for index in range(chunk_count):
        packed, room, length, start, size = struct.unpack_from("<IIIII", header, chunks + index * 20)
        matches = [r for r in resources if start <= r[0] and r[0] + r[1] <= start + size]
        if not matches:
            continue
        stream.seek(packed)
        data = stream.read(length)
        if data.startswith(b"\x28\xb5\x2f\xfd"):
            data = zstandard.ZstdDecompressor().decompress(data, max_output_size=size)
        for address, length, guid in matches:
            model = data[address - start:address - start + length]
            assert len(model) == length and len(model) >= 24
            if model[8] == 8:
                result.extend(guid + struct.pack("<I", len(model)) + model)
                props += 1
assert props == 6282, "Different client build: review fixtures and expected coverage"
output.write_bytes(result)
print(f"Extracted {props} native Prop fixtures locally; do not publish this file.")
