#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""실제 여덟 이동 유닛의 간헐천/크리스탈 호버 커서를 명시 테스트 자산 변형과 구별해 관측한다."""
import argparse
import hashlib
import struct
from contextlib import contextmanager
from pathlib import Path

from cpp_bridge_ritual_runtime import ROOT, save
from cpp_game_menu_smoke import protected_run
from cpp_world_smoke import states
from cpp_fort_smoke import Catalog
import fort

# 원본 TEST02의 여섯 이동 타입과 동일 저장 형식의 명시 두 타입 대체다.
BASE = ('priest', 'windWalker', 'windBalloon', 'rainBalloon', 'bulf', 'rainWalker')
REMAP = {'windWalker': 'sunWalker', 'windBalloon': 'sunBalloon'}


@contextmanager
def variant(data_root, remap, output):
    """명시 사본의 TypeNames만 바꾸고 성공/실패 모두 원래 바이트로 복구한다."""
    path = data_root / 'd/TEST02.fort'
    original = path.read_bytes()
    catalog = Catalog(data_root, 'd', 118)
    sections, end = fort.split_sections(original)
    names = bytearray(sections[7])  # 첫 해시만 변환하는 원본 규칙에 맞게 중복 해시 없이 두 이름을 맞바꾼다.
    conversion = catalog.conversion(names)
    # 원본의 첫 해시 일치 규칙을 지키며 요청한 각 두 타입만 교환한다.
    for old, new in remap.items():
        first, second = catalog.flags[old], catalog.flags[new]
        assert (first[0] & 0x23020) == (second[0] & 0x23020)
        assert (first[1] & 0x4004) == (second[1] & 0x4004)
        assert bool(first[1] & fort.F2_OWNER_MASK) == bool(second[1] & fort.F2_OWNER_MASK)
        indices = [index for index, name in conversion.items() if name == old]
        partners = [index for index, name in conversion.items() if name == new]
        assert len(indices) == len(partners) == 1
        struct.pack_into('<I', names, 1 + indices[0] * 4, fort.type_hash(new))
        struct.pack_into('<I', names, 1 + partners[0] * 4, fort.type_hash(old))
    sections[7] = bytes(names)
    changed = original[:2] + b''.join(struct.pack('<H', len(section) + 2) + section for section in sections) + original[end:]
    output.mkdir(parents=True, exist_ok=False)
    (output / 'TEST02-before.fort').write_bytes(original)
    (output / 'TEST02-variant.fort').write_bytes(changed)
    save(output / 'variant.json', dict(remap=remap, testOnlyTypeRemap=bool(remap),
         beforeSha256=hashlib.sha256(original).hexdigest(), variantSha256=hashlib.sha256(changed).hexdigest()))
    try:
        path.write_bytes(changed)
        yield
    finally:
        path.write_bytes(original)
        assert path.read_bytes() == original


def one(directory, data_root, unit, expected, camera):
    """선택·호버는 실제 입력으로 실행하며 보고서의 커서/선택/활성 상태를 검증한다."""
    cx, cy = camera
    target = (144 * 16 - cx - 24, 182 * 11 - cy - 16)  # 원본 역투영의 +16/+11 뒤 목표 셀 안쪽을 가리킨다.
    ground = (150 * 16 - cx - 24, 184 * 11 - cy - 16)
    steps = [(5, 'report', 'briefing'), (10, 'click', 'Go!'), (20, 'report', 'initial'),
             (25, 'click', 'world:' + unit), (30, 'report', 'selected'),
             (35, 'pointer', f'0x80010000,{target[0]},{target[1]}'), (40, 'report', 'target'),
             (40, 'snapshot', str(directory / 'target.bmp')),
             (45, 'pointer', f'0x80010000,{ground[0]},{ground[1]}'), (50, 'report', 'ground')]
    report, run = protected_run(directory, 1280, 720, steps, 56, data_root=data_root, mission='TEST02')
    observed = states(report)
    initial = observed['initial']
    assert tuple(map(int, initial['camera'])) == camera
    for label in ('selected', 'target', 'ground'):  # 같은 현재 SID가 이동 모드5에서 계속 선택돼 있어야 한다.
        state = observed[label]
        sid = int(state['raw_input'][0])
        assert state['raw_input'][1] == '5' and state['objects'][sid][0:2] == [unit, '1'], (label, state['raw_input'])
        assert state['mouse_cursor'][1] == '1', '비활성 창의 커서 관측'
    assert observed['target']['mouse_cursor'][0] == str(expected), (unit, observed['target']['mouse_cursor'], expected)
    assert observed['ground']['mouse_cursor'][0] == '18'
    result = dict(unit=unit, targetCursor=int(observed['target']['mouse_cursor'][0]), groundCursor=18,
                  selectedSid=int(observed['target']['raw_input'][0]), active=True, run=run)
    save(directory / 'summary.json', result)
    return result


def main():
    """원본 경로를 거부하고 사본의 네 변형을 순서대로 검사한 뒤 복구한다."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--data-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--camera', default='1664,1554')
    args = parser.parse_args()
    data_root, output = args.data_root.resolve(), args.output.resolve()
    assert not any(data_root == (ROOT / name).resolve() or (ROOT / name).resolve() in data_root.parents for name in ('originals', 'originalCD'))
    camera = tuple(map(int, args.camera.split(',')))
    assert len(camera) == 2
    output.mkdir(parents=True, exist_ok=False)
    results = []  # 원본 맵 여섯 종류와 명시 두 타입 변형을 각각 구별한다.
    # 두 대상 변형을 순서대로 실행하며 간헐천과 크리스탈 결과를 섞지 않는다.
    for target, target_remap in (('geyser', {}), ('nugget', {'geyser': 'nugget'})):
        # 원래 여섯 종류와 명시 태양 타입 두 종류를 별도 자산/결과 폴더에 둔다.
        for group, units, remap in (('base', BASE, target_remap), ('sun', ('sunWalker', 'sunBalloon'), {**REMAP, **target_remap})):
            folder = output / (target + '-' + group)
            with variant(data_root, remap, folder):
                # 각 유닛은 새 프로세스에서 선택하고 두 호버 지점을 확인한다.
                for unit in units:
                    expected = 13 if target == 'nugget' else 3 if unit in ('sunBalloon', 'windBalloon', 'rainBalloon') else 18
                    results.append(dict(target=target, group=group, **one(folder / unit, data_root, unit, expected, camera)))
    save(output / 'summary.json', dict(results=results, testOnlyTypeRemapExplained=True, naturalCaptureNotVerified=True))
    print('이동 여덟 타입 × 간헐천/크리스탈 커서 실제16사례 통과')


if __name__ == '__main__':
    main()
