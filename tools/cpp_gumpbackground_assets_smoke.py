#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""두 실제 GameAssets/SHP 저장소를 raw 배경에 공급한 콘솔 헤더/픽셀을 독립 PE/SHP로 대조한다.

기본 CTest는 원본 자산이 필요 없다. 이 선택 검사는 읽기 전용 실제 두 판본을 사용한다.
원본/복사본 게임·GUI·소리 장치 없이 새 C++ 검사 실행 파일의 콘솔 출력을 검사한다.
"""
import argparse
import json
import struct
import subprocess
from pathlib import Path
from cpp_assets_smoke import original_order
from cpp_renderer_smoke import snapshots
from cpp_window_smoke import read_data, screen_palette
from shp import read_blocks, read_clusters, decode_frame
from taff import TaffArchive
from decomp_gumpbackgrounddraw_oracle import BackgroundDrawOracle, CONTROLS, signed, digest

# 이 단계의 검사 자료/보고서는 Git 제외 산출물 경로에만 저장한다.
ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / 'extracted/gumpbackgrounddraw-assets'


def native_maps(edition, palette):
    """독립 원본 몸체가 남긴 밝음/어두움 표를 같은 입력 팔레트에 한정해 읽는다."""
    logical = b''.join(bytes(rgb) + b'\0' for rgb in palette)
    path = ROOT / 'cpppj/tests/fixtures/gumpbackground-x86.tsv'
    # C++ 출력에서 기대 색 표를 얻지 않고 이미 저장한 실제 PE 표 관찰을 찾는다.
    for line in path.read_text(encoding='utf-8').splitlines():
        fields = line.split('\t')
        if len(fields) == 6 and fields[:2] == [edition, 'maps'] and bytes.fromhex(fields[3]) == logical:
            return bytes.fromhex(fields[4]), bytes.fromhex(fields[5])
    raise AssertionError(f'실제 팔레트와 일치하는 원본 명암 관찰 없음: {edition}')


def raster(events, pixels, rect, rows, maps):
    """원본의 실제 VFX/선 호출 인자를 독립 SHP RLE 결과로 32×24 표면에 합성한다."""
    result = bytearray.fromhex(pixels)
    clip = (0, 0, 32, 24)
    # texture clip/shape와 flat line 순서는 PE가 정한다. 격자/테두리 계획을 Python으로 다시 계산하지 않는다.
    for event in events.split(';'):
        fields = event.split(',')
        kind, args = fields[0], list(map(int, fields[1:]))
        if kind == 'C':
            clip = tuple(signed(value) for value in args)
        elif kind == 'D':
            _, _, x, y, shade = args
            left, top = signed(x) + rect[0], signed(y) + rect[1]
            low_y, high_y = max(0, clip[1], top), min(24, clip[3], top + len(rows))
            low_x, high_x = max(0, clip[0], left), min(32, clip[2], left + (len(rows[0]) if rows else 0))
            # 원본 압축 프레임의 불투명 픽셀만 현재 실제 clip 안에서 합성한다.
            for py in range(low_y, high_y):
                # 명암은 소스 번호 치환이며 transparent skip는 이전 화면을 보존한다.
                for px in range(low_x, high_x):
                    color = rows[py - top][px - left]
                    if color is not None:
                        result[py * 32 + px] = maps[shade - 1][color] if shade else color
        elif kind == 'L':
            x0, y0, x1, y1 = (signed(value) for value in args[:4])
            color = args[4] & 255
            assert x0 == x1 or y0 == y1
            low_x, high_x = max(0, clip[0], min(x0, x1)), min(32, clip[2], max(x0, x1) + 1)
            low_y, high_y = max(0, clip[1], min(y0, y1)), min(24, clip[3], max(y0, y1) + 1)
            # 실제 선의 양 끝을 포함하고 현재 화면/clip 밖 픽셀은 쓰지 않는다.
            for py in range(low_y, high_y):
                # VFX 호출 순서대로 모서리를 뒤 선이 덮는다.
                for px in range(low_x, high_x):
                    result[py * 32 + px] = color
    return bytes(result)


def check(executable, edition):
    """실제 PE 이름 표·.type 논리 수·SHP 헤더/RLE·팔레트를 C++ 자산 출력과 전수 대조한다."""
    root = ROOT / edition
    before = snapshots(root)
    args = [str(executable), '--inspect-gump-background-assets', str(root)]
    if edition == 'originalCD':
        args.append('--cd')
    process = subprocess.run(args, capture_output=True, timeout=60)
    assert process.returncode == 0, process.stdout.decode('utf-8', errors='replace') + process.stderr.decode('utf-8', errors='replace')
    actual = json.loads(process.stdout)
    archive = TaffArchive(root / 'netstorm.tarc')
    data = read_data(root, 'd', archive, '_shapes.shp')
    blocks, order = read_blocks(data), original_order(edition)
    headers = []
    # 이름/블록 대응은 C++ 상수 대신 실제 PE의 포인터 표에서 읽는다.
    for block, name in zip(blocks, order, strict=True):
        clusters = read_clusters(read_data(root, 'd', archive, name + '.type').decode('cp1252'))
        # 모든 논리 프레임의 원본 추가 헤더 signed WORD를 직접 읽는다.
        for index in range(len(clusters)):
            width, height = struct.unpack_from('<hh', data, block.frames[index].offset - 12)
            headers.append([70 + block.index, index, width, height, block.index])
    assert actual['headers'] == headers and actual['types'] == len(order)
    block_index = order.index('fortGump')
    clusters = read_clusters(read_data(root, 'd', archive, 'fortGump.type').decode('cp1252'))
    frame_index = next(index for index, cluster in enumerate(clusters) if cluster[0] == 'A00')
    frame = blocks[block_index].frames[frame_index]
    width, height = struct.unpack_from('<hh', data, frame.offset - 12)
    assert (actual['fortType'], actual['frame'], actual['tileWidth'], actual['tileHeight']) == (70 + block_index, frame_index, width, height)
    rows = decode_frame(data, frame)
    palette = screen_palette(read_data(root, 'd', archive, 'gifcloud.col'))
    maps = native_maps(edition, palette)
    near = min(range(256), key=lambda index: sum((value - 255) ** 2 for value in palette[index]))
    far = min(range(256), key=lambda index: sum((value - 128) ** 2 for value in palette[index]))
    oracle = BackgroundDrawOracle(edition)
    compared, traces = 0, []
    # 네 실제 raw 출력에 같은 실제 헤더/팔레트 번호 입력을 원본 몸체에 공급한다.
    for flags, pixels in actual['samples']:
        case = (1, 3, 2, 26.0, 19.0, flags, 82, 0, width, height, (0, 0, 32, 24), 0, 0, near, far)
        events, _, _, native_pixels = oracle.run(case, CONTROLS[0])
        expected = raster(events, native_pixels, frame.rect, rows, maps)
        assert bytes.fromhex(pixels) == expected, (edition, flags)
        traces.append(dict(flags=flags, native_events=events, pixel_sha256=digest_bytes(expected)))
        compared += len(expected)
    assert snapshots(root) == before, f'{edition}: 원본 파일/존재 상태 변경'
    (OUTPUT / (edition + '-cpp.json')).write_text(json.dumps(actual, ensure_ascii=False), encoding='utf-8')
    return dict(types=len(order), logical_headers=len(headers), raw_flows=len(traces), pixels=compared,
        original_files_unchanged=len(before), source_sha256=digest(root / 'netstorm.tarc'), samples=traces)


def digest_bytes(raw):
    """공유할 검사 결과의 실제 픽셀 바이트 SHA-256을 계산한다."""
    import hashlib
    return hashlib.sha256(raw).hexdigest()


def main():
    """두 판본 읽기 전용 콘솔 대조 결과를 기록한다. GUI/장치/원본 게임을 실행하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=ROOT / 'cpppj/build/bin/Release/netstorm_tests.exe')
    args = parser.parse_args()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    report = {edition: check(args.exe.resolve(), edition) for edition in ('originals', 'originalCD')}
    report['script_sha256'] = digest(Path(__file__))
    (OUTPUT / 'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({edition: {key: value for key, value in evidence.items() if key != 'samples'}
        for edition, evidence in report.items() if isinstance(evidence, dict)}, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
