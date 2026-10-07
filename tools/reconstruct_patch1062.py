#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""zpatch.exe를 실행하지 않고 10.37 CD에 10.62 바이트 패치를 적용한다."""
import argparse
import collections
import hashlib
import json
import struct
from pathlib import Path

# 원본 입력은 고정하며 출력은 Git 제외 분석 경로로만 보낸다.
ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / 'extracted/original1062'
# update.bat은 설치 폴더 파일 대신 CD 파일을 복사한 뒤 이 두 패치를 적용한다.
INPUTS = (('Netstorm.exe', 'originalCD/NETSTORM.EXE', '1'),
          ('netstorm.tarc', 'originalCD/NETSTORM.TARC', '2'))


def sha(data):
    """입력 바이트의 SHA-256을 계산한다."""
    return hashlib.sha256(data).hexdigest()


def checksum(data):
    """zpatch 00401710의 unsigned byte 현재값+직전값 누적 DWORD다."""
    return (2 * sum(data) - (data[-1] if data else 0)) & 0xffffffff


def apply_delta(source, delta):
    """00401740의 정상 바이트 경로를 옮기고 손상 범위는 출력 전에 거부한다."""
    position = 0

    def read(width):
        """패치 필드의 little endian 값을 끝 경계 검사 뒤 읽는다."""
        nonlocal position
        if position + width > len(delta):
            raise ValueError('패치 필드가 잘렸습니다')
        value = int.from_bytes(delta[position:position + width], 'little')
        position += width
        return value

    if read(4) != checksum(source):
        raise ValueError('CD 기준 파일 체크섬이 패치와 다릅니다')
    cursor = 0
    result = bytearray()
    records = collections.Counter()
    # 각 레코드는 원본 오프셋, 삭제 폭, 삽입 폭, 삽입 바이트 순서다.
    while position < len(delta):
        flags = read(1)
        offset = read(2 if flags & 0x40 else 4)
        if flags & 0x80:
            removed = inserted = 1
        else:
            removed = read(1 if flags & 1 else 2 if flags & 2 else 4) if flags & 7 else 0
            inserted = read(1 if flags & 8 else 2 if flags & 16 else 4) if flags & 56 else 0
        if not cursor <= offset <= len(source) or offset + removed > len(source):
            raise ValueError('패치 원본 오프셋/삭제 범위 오류')
        if position + inserted > len(delta):
            raise ValueError('패치 삽입 바이트가 잘렸습니다')
        result.extend(source[cursor:offset])
        result.extend(delta[position:position + inserted])
        cursor = offset + removed
        position += inserted
        records['records'] += 1
        records['deleted_bytes'] += removed
        records['inserted_bytes'] += inserted
        records[f'flags_{flags:02x}'] += 1
    result.extend(source[cursor:])
    return bytes(result), dict(records)


def derive():
    """보호된 입력 두 쌍에서 새 파일과 재현 메타데이터를 메모리로 만든다."""
    outputs, facts = {}, {}
    # 실행 파일과 TARC 모두 동일한 CD 기준/패치 규칙을 사용한다.
    for name, base, patch in INPUTS:
        relative = f'originalPatches/NSP1062/{patch}'
        source, delta = (ROOT / base).read_bytes(), (ROOT / relative).read_bytes()
        outputs[name], records = apply_delta(source, delta)
        facts[name] = dict(base=base, base_sha256=sha(source), base_checksum=checksum(source),
                           patch=relative, patch_sha256=sha(delta), result_sha256=sha(outputs[name]),
                           result_size=len(outputs[name]), **records)
    return outputs, facts


def self_test():
    """폭 경계·동일 오프셋 삽입·잘림·잘못된 기준·역순/범위 초과를 검사한다."""
    source = bytes(range(256)) * 300
    header = struct.pack('<I', checksum(source))
    delta = header + bytes([0xc0]) + struct.pack('<H', 3) + b'Z'
    delta += bytes([0x4a]) + struct.pack('<HHB', 256, 256, 3) + b'abc'
    delta += bytes([0x24]) + struct.pack('<III', 70000, 65536 // 16, 1) + b'!'
    output, _ = apply_delta(source, delta)
    expected = source[:3] + b'Z' + source[4:256] + b'abc' + source[512:70000] + b'!' + source[74096:]
    assert output == expected
    same = header + bytes([0x48]) + struct.pack('<HB', 2, 1) + b'A'
    same += bytes([0x48]) + struct.pack('<HB', 2, 1) + b'B'
    assert apply_delta(source, same)[0] == source[:2] + b'AB' + source[2:]
    malformed = [b'', delta[:-1], struct.pack('<I', 1), header + b'\x00',
                 header + b'\x04' + struct.pack('<II', len(source), 1),
                 header + b'\x40' + struct.pack('<H', 4) + b'\x40' + struct.pack('<H', 3)]
    # 각 오류가 조용한 부분 출력 없이 실패하는지 확인한다.
    for data in malformed:
        try:
            apply_delta(source, data)
        except ValueError:
            continue
        raise AssertionError('손상 패치를 허용했습니다')
    print('zpatch 바이트 복원 자체 검사 통과')


if __name__ == '__main__':
    # 공급된 원본/복사본 실행 없이 Python 바이트 읽기/쓰기만 수행한다.
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    if args.self_test:
        self_test()
    else:
        outputs, facts = derive()
        if args.verify:
            assert json.loads((OUTPUT / 'patch-facts.json').read_text(encoding='utf-8')) == facts
            # 저장된 출력의 모든 바이트를 독립 재계산 결과와 비교한다.
            for name, data in outputs.items():
                assert (OUTPUT / name).read_bytes() == data
            print('10.62 CD 기준 체크섬/복원 바이트 확인 통과')
        else:
            OUTPUT.mkdir(parents=True, exist_ok=True)
            # 원본 디렉터리에는 쓰지 않고 고정 분석 폴더에만 결과를 놓는다.
            for name, data in outputs.items():
                (OUTPUT / name).write_bytes(data)
            (OUTPUT / 'patch-facts.json').write_text(json.dumps(facts, indent=2) + '\n', encoding='utf-8', newline='\n')
            print(json.dumps(facts, indent=2))
