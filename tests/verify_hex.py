"""Compare Intel HEX bytes/checksums/addresses against ELF load image; stdlib only."""
import pathlib
import struct
import sys

elf, hexfile = map(pathlib.Path, sys.argv[1:3])
image = {}
base = 0
ended = False
entry = None
for line in hexfile.read_text().splitlines():
    assert not ended and line.startswith(':'), 'invalid HEX framing'
    record = bytes.fromhex(line[1:])
    assert len(record) == record[0] + 5 and sum(record) % 256 == 0, 'HEX checksum/length'
    n, kind = record[0], record[3]
    addr = base + int.from_bytes(record[1:3], 'big')
    if kind == 0:
        for i, value in enumerate(record[4:4+n]):
            assert 0x08000000 <= addr+i < 0x08010000 and addr+i not in image
            image[addr+i] = value
    elif kind == 4:
        assert n == 2
        base = int.from_bytes(record[4:6], 'big') << 16
    elif kind == 1:
        assert n == 0
        ended = True
    else:
        assert kind == 5 and n == 4, 'unsupported record'
        entry = int.from_bytes(record[4:8], 'big')
assert ended and image and min(image) == 0x08000000
# ELF32 PT_LOAD physical addresses are the Flash load addresses for .data.
# Skip zero-length segments: objcopy --gap-fill may otherwise extend an image
# to an empty TLS section's RAM LMA with this toolchain/linker-script pair.
binary = elf.read_bytes()
assert binary[:7] == b'\x7fELF\x01\x01\x01'
assert entry == struct.unpack_from('<I', binary, 24)[0], 'entry address mismatch'
phoff = struct.unpack_from('<I', binary, 28)[0]
entsize, count = struct.unpack_from('<HH', binary, 42)
expected = {}
for i in range(count):
    kind, offset, vaddr, physical, size, memsize, flags, align = struct.unpack_from('<8I', binary, phoff+i*entsize)
    if kind == 1 and size:
        for j, value in enumerate(binary[offset:offset+size]):
            assert physical+j not in expected
            expected[physical+j] = value
assert image == expected, 'ELF/HEX load bytes mismatch'
print(f'PASS: {hexfile.name}: {len(image)} data bytes, ELF load segments match')
