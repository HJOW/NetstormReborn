#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""새 C++ 타입·SHP 로더를 기존 Python 추출기로 모든 실제 픽셀과 대조한다.

원본·CD 게임은 실행하지 않는다. 검토용 BMP와 보고서는 extracted에만 생성한다.
기존 추출기의 Pillow와 PE 로딩 순서 검사용 pefile이 필요하다.
"""
import argparse
import hashlib
import io
import json
import re
import struct
import subprocess
from pathlib import Path

import pefile
from PIL import Image
from taff import TaffArchive
from shp import read_blocks, decode_frame
from typefile import parse_type

# 저장소 루트이며 원본 경로는 읽기만 한다.
ROOT = Path(__file__).resolve().parent.parent


class Reader:
    """C++ 검사 스트림을 경계 검사하며 읽는다. 원본 파일 포맷과는 별도다."""
    def __init__(self, payload):
        """출력 버퍼와 현재 위치를 초기화한다."""
        self.payload, self.position = payload, 0

    def take(self, size):
        """지정한 바이트 수만 소비하며 잘린 검사 출력은 실패한다."""
        end = self.position+size
        if end > len(self.payload):
            raise AssertionError('C++ 자산 스트림 잘림')
        data = self.payload[self.position:end]
        self.position = end
        return data

    def number(self, format_code='I'):
        """리틀 엔디언 32비트 정수 또는 64비트 실수를 읽는다."""
        return struct.unpack('<'+format_code, self.take(struct.calcsize(format_code)))[0]

    def text(self):
        """길이가 붙은 UTF-8 문자열을 읽는다."""
        return self.take(self.number()).decode('utf-8')


def run(executable, *args, success=True):
    """복원된 C++ 실행 파일만 시작한다. 게임 바이너리는 실행하지 않는다."""
    result = subprocess.run([str(executable), *map(str, args)], capture_output=True, timeout=60)
    if success and result.returncode:
        raise RuntimeError(result.stderr.decode('utf-8', errors='replace'))
    return result


def read_data(root, archive, name):
    """C++와 같이 loose 파일을 먼저 읽고 아카이브는 독립 Python 판독기로 읽는다."""
    path = root / name
    if path.is_file():
        return path.read_bytes()
    canonical = '\\'+name.replace('/', '\\').lower()
    entry = next(entry for entry in archive.entries if entry.name.lower() == canonical)
    return archive.read(entry)


def original_order(edition):
    """C++ 상수를 다시 쓰지 않고 두 PE의 실제 타입 이름 포인터 배열을 읽는다."""
    is_cd = edition == 'originalCD'
    pe = pefile.PE(str(ROOT / edition / ('NETSTORM.EXE' if is_cd else 'Netstorm.exe')))
    image, base = pe.get_memory_mapped_image(), pe.OPTIONAL_HEADER.ImageBase
    address, count = (0x51c6f8, 101) if is_cd else (0x540dd0, 116)
    names = []
    # 각 포인터의 NUL 종료 자산 이름을 읽는다.
    for i in range(count):
        pointer, = struct.unpack_from('<I', image, address-base+i*4)
        names.append(image[pointer-base:pointer-base+100].split(b'\0')[0].decode('ascii'))
    return names


def expected_code(name, flag_text):
    """문서·두 로더의 코드 생성 규칙을 독립 regex 클러스터에 적용한다."""
    label, number = re.fullmatch(r'([A-Z]+)(\d+)', name).groups()
    bits = {'fringe': 1, 'suck': 2, 'rim': 4, 'lit': 8, 'unlit': 16, 'cracked': 32, 'hard': 64}
    flags = set(flag_text.lower().split())
    return bytes([ord(label[0]), ord(label[1] if len(label) > 1 else 'P'), int(number) & 255,
                  sum(bit for flag, bit in bits.items() if flag in flags)])


def check_edition(executable, edition, output):
    """타입·모든 프레임·BMP 알파를 비교하고 확인된 판본 차이를 보고한다."""
    root = ROOT / edition
    archive = TaffArchive(str(root / 'netstorm.tarc'))
    shape_data = read_data(root, archive, 'd/_shapes.shp')
    blocks, order = read_blocks(shape_data), original_order(edition)
    edition_args = ['--cd'] if edition == 'originalCD' else []
    stream = Reader(run(executable, '--dump-assets', root, *edition_args).stdout)
    if stream.number() != 1 or stream.number() != len(order):
        raise AssertionError('C++ 자산 스트림 스키마·타입 개수 불일치')
    image_count = special_count = cluster_count = pixel_count = 0
    sampled = {}
    # PE 로딩 순서와 기존 Python parser의 의미를 모두 비교한다.
    for block, asset_name in zip(blocks, order, strict=True):
        type_text = read_data(root, archive, f'd/{asset_name}.type').decode('cp1252')
        expected = parse_type(type_text)
        if (stream.text(), stream.text(), stream.text()) != (asset_name, expected.name, expected.constructor):
            raise AssertionError(f'타입 이름·생성자 불일치: {edition}/{asset_name}')
        flags = [stream.text() for _ in range(stream.number())]
        if flags != expected.flags:
            raise AssertionError(f'타입 플래그 불일치: {edition}/{asset_name}')
        properties = {}
        # C++는 중복 속성도 보존하므로 마지막 값을 조회하는 사전을 따로 만든다.
        for _property in range(stream.number()):
            name, kind = stream.text(), stream.number()
            properties[name.lower()] = stream.number('d') if kind == 0 else stream.text()
        expected_properties = {key.lower(): value for key, value in expected.props.items()}
        if properties != expected_properties:
            raise AssertionError(f'타입 속성 불일치: {edition}/{asset_name}: {properties!r}/{expected_properties!r}')
        count = stream.number()
        if count != len(expected.clusters):
            raise AssertionError(f'클러스터 개수 불일치: {edition}/{asset_name}')
        cluster_count += count
        default, gump, help_frame, base_frame, has_help = 0, 0, -1, -1, False
        # 원본처럼 클러스터와 플래그를 선언 순서대로 처리한다.
        for index, (name, flag_text, refs) in enumerate(expected.clusters):
            if stream.text() != name or stream.take(4) != expected_code(name, flag_text):
                raise AssertionError(f'프레임 코드 불일치: {edition}/{asset_name}/{name}')
            actual_refs = [(stream.text(), stream.number('d')) for _ in range(stream.number())]
            if actual_refs != refs:
                raise AssertionError(f'GIF 참조 불일치: {edition}/{asset_name}/{name}')
            # 여러 default보다 명시적인 help를 우선하는 원본 규칙을 적용한다.
            for flag in flag_text.lower().split():
                if flag == 'default':
                    default = index
                    if not has_help:
                        help_frame = index
                elif flag == 'help':
                    help_frame, has_help = index, True
                elif flag == 'gumpframe':
                    gump = index
                elif flag == 'baseframe':
                    base_frame = index
        if tuple(stream.number('i') for _ in range(4)) != (default, gump, help_frame, base_frame):
            raise AssertionError(f'특수 프레임 인덱스 불일치: {edition}/{asset_name}')
        width, height = int(properties.get('foot_x', 0)), int(properties.get('foot_y', 0))
        if height == 6:
            height = 8
        if tuple(stream.number('i') for _ in range(2)) != (width, height):
            raise AssertionError(f'footprint 후처리 불일치: {edition}/{asset_name}')
        if stream.number() != len(block.frames):
            raise AssertionError(f'SHP 프레임 개수 불일치: {edition}/{asset_name}')
        # 모든 일반 프레임의 투명 여부·팔레트 번호와 특수 레코드 헤더를 대조한다.
        for index, frame in enumerate(block.frames):
            bounds, origin = tuple(stream.number() for _ in range(2)), tuple(stream.number() for _ in range(2))
            rect = tuple(stream.number('i') for _ in range(4))
            special = bool(stream.number())
            if (bounds, origin, rect, special) != (frame.bounds, frame.origin, frame.rect, frame.special):
                raise AssertionError(f'프레임 헤더 불일치: {edition}/{asset_name}/{index}')
            if special:
                special_count += 1
                continue
            dimensions = stream.number(), stream.number()
            if dimensions != (frame.width, frame.height):
                raise AssertionError(f'이미지 크기 불일치: {edition}/{asset_name}/{index}')
            size = frame.width*frame.height
            indices, opacity = stream.take(size), stream.take(size)
            expected_indices, expected_opacity = bytearray(), bytearray()
            # 기존 Python RLE 판독기는 투명을 None으로 표현하여 C++의 별도 마스크와 독립적이다.
            for row in decode_frame(shape_data, frame):
                expected_indices.extend(0 if pixel is None else pixel for pixel in row)
                expected_opacity.extend(0 if pixel is None else 255 for pixel in row)
            if indices != expected_indices or opacity != expected_opacity:
                raise AssertionError(f'압축 픽셀 불일치: {edition}/{asset_name}/{index}')
            image_count += 1
            pixel_count += size
            if asset_name in ('sunCannon', 'dude', 'altar') and index < len(expected.clusters):
                sampled.setdefault((asset_name, 0), (expected.clusters[index][0], frame, indices, opacity))
            if asset_name == 'sunCannon' and len(expected.clusters) <= index < len(expected.clusters)*2:
                sampled.setdefault((asset_name, 1), (expected.clusters[index-len(expected.clusters)][0], frame, indices, opacity))
        if asset_name == 'sunCannon':
            cannon_damage = properties['hppersec']
        if asset_name == 'manabolt':
            manabolt_layout = {'clusters': len(expected.clusters), 'frames': len(block.frames)}
    if stream.position != len(stream.payload):
        raise AssertionError('C++ 자산 출력 끝의 잔여 데이터')
    palette = read_data(root, archive, 'd/gifcloud.col')[8:]
    exported = []
    # 검토용 BMP를 PIL로 독립 재판독하여 RGB·알파·상하 방향을 검증한다.
    for (asset_name, layer), (cluster, frame, indices, opacity) in sampled.items():
        path = output / f'{edition}-{asset_name}-{cluster}-L{layer}.bmp'
        # dude의 실제 typename인 Man 별명으로도 같은 타입을 찾는지 검사한다.
        type_name = 'Man' if asset_name == 'dude' else asset_name
        run(executable, '--export-frame', root, type_name, cluster, layer, path, *edition_args)
        actual = Image.open(io.BytesIO(path.read_bytes())).convert('RGBA')
        rgba = bytearray()
        # 원본 팔레트 번호와 투명 마스크로 기대 RGBA를 구성한다.
        for index, alpha in zip(indices, opacity, strict=True):
            rgba.extend(palette[index*3:index*3+3])
            rgba.append(alpha)
        if actual.size != (frame.width, frame.height) or actual.tobytes() != rgba:
            raise AssertionError(f'BMP RGBA 불일치: {path.name}')
        exported.append(path.relative_to(ROOT).as_posix())
    # 잘못된 판본 선택을 성공으로 처리하면 타입과 SHP가 잘못 연결된다.
    if edition == 'originalCD' and run(executable, '--inspect-assets', root, success=False).returncode == 0:
        raise AssertionError('잘못된 판본 선택이 거부되지 않음')
    return {'types': len(order), 'clusters': cluster_count, 'frames': sum(len(block.frames) for block in blocks),
            'images': image_count, 'special': special_count, 'pixels_compared': pixel_count,
            'sunCannon_hpPerSec': cannon_damage, 'manabolt_layout': manabolt_layout,
            'shape_sha256': hashlib.sha256(shape_data).hexdigest(), 'bmp_rgba_verified': exported}


def main():
    """원본 파일의 해시를 유지하며 두 판본의 전체 자산 검사를 수행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=ROOT / 'cpppj/build/bin/Release/NetstormCpp.exe')
    args = parser.parse_args()
    output = ROOT / 'extracted/cpp-assets-smoke'
    output.mkdir(parents=True, exist_ok=True)
    protected = [ROOT / 'originals/Netstorm.exe', ROOT / 'originalCD/NETSTORM.EXE',
                 ROOT / 'originals/netstorm.tarc', ROOT / 'originalCD/NETSTORM.TARC',
                 ROOT / 'originals/d/_shapes.shp', ROOT / 'originalCD/d/_shapes.shp']
    before = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in protected}
    report = {}
    # 검증기와 내보내기 모두 새 C++ 프로그램만 실행한다.
    for edition in ('originals', 'originalCD'):
        report[edition] = check_edition(args.exe, edition, output)
        print(f'{edition}: 모든 타입·SHP 픽셀·BMP RGBA 대조 완료', flush=True)
    # 실행 파일·아카이브·그래픽 원본이 수정되지 않았음을 별도로 확인한다.
    for path, expected_hash in before.items():
        if hashlib.sha256(path.read_bytes()).hexdigest() != expected_hash:
            raise AssertionError(f'원본 변경 감지: {path}')
    report['original_files_unchanged'] = True
    (output / 'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
