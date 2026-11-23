#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호된 CD SHP를 읽기만 하여 누락 UI 마나 그림의 단일 블록 C++ 자료를 재생성한다."""
from pathlib import Path
import struct
from shp import TYPE_LOAD_ORDER, read_blocks, decode_frame

# 실행 위치와 관계없이 같은 저장소의 자료와 생성 파일을 사용한다.
ROOT = Path(__file__).resolve().parents[1]


def main():
    """10.78의 앞8개 픽셀/추가 헤더 일치를 확인한 뒤 CD25프레임을 그대로 재배치한다."""
    source = (ROOT / 'originalCD/D/_shapes.shp').read_bytes()
    patch = (ROOT / 'originals/d/_shapes.shp').read_bytes()
    blocks = read_blocks(source)
    index = TYPE_LOAD_ORDER.index('mana')
    frames, current = blocks[index].frames, read_blocks(patch)[index].frames
    assert len(frames) == 25 and len(current) == 8
    for old, new in zip(current, frames):
        # 원본 그림과 부가 크기/기준점이 같은 경우에만 공유 마나 자료로 사용한다.
        assert old.rect == new.rect and decode_frame(patch, old) == decode_frame(source, new)
        assert patch[old.offset-36:old.offset] == source[new.offset-36:new.offset]
    ends = sorted({frame.offset-36 for block in blocks for frame in block.frames} | {len(source)})
    output = bytearray(b'1.10' + struct.pack('<I', len(frames)) + bytes(8 * len(frames)))
    for i, frame in enumerate(frames):
        # 각 압축 그림과36바이트 부가 헤더는 재인코딩하지 않고 원시 바이트로 복사한다.
        start = frame.offset-36
        end = ends[ends.index(start)+1]
        struct.pack_into('<II', output, 8+8*i, len(output)+36, 0)
        output.extend(source[start:end])
    lines = ['// CD 원본 마나25프레임의 단일 SHP 블록이다. 재생성: python tools/extract_cpp_mana_pictures.py']
    for offset in range(0, len(output), 20):
        # 자료만 생성하며 namespace/상수 선언은 포함하는 C++ 구현이 소유한다.
        lines.append(','.join(f'0x{value:02x}' for value in output[offset:offset+20])+',')
    destination = ROOT / 'cpppj/src/client/ManaPictureData.inc'
    destination.write_text('\n'.join(lines)+'\n', encoding='utf-8')
    print(f'{destination.name}: {len(output)} bytes, {len(frames)} frames')


if __name__ == '__main__':
    main()
