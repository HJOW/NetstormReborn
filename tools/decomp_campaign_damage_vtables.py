#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""현재10.78 PE의 피해·방어 가상 주소83쌍을 정적으로 읽고 생성 표를 감사한다."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import socket
import struct


# 저장소와 보호 PE·기존 검증 주소 목록·생성자 대응·이번 생성물의 위치다.
ROOT = Path(__file__).resolve().parents[1]
PE_PATH = ROOT / 'originals/Netstorm.exe'
VTABLES_PATH = ROOT / 'cpppj/src/o/UnpopVtables.inc'
CONSTRUCTORS_PATH = ROOT / 'cpppj/src/o/TypeConstructors.inc'
RECIPES_PATH = ROOT / 'cpppj/src/o/DerivedConstructors.inc'
INCLUDE_PATH = ROOT / 'cpppj/src/o/CampaignDamageVtables.inc'
REPORT_PATH = ROOT / 'cpppj/campaign-damage-vtables.json'
# 현재 정밀 디컴파일 PC와 기존10.78 목록의 서로 다른 가상 표 수다.
EXPECTED_HOST = 'VM-W11-Codex'
EXPECTED_TABLE_COUNT = 83
# 원본 일반 자산의 피해·방어 슬롯과 기본 생성자의 실제 가상 주소다.
HIT_OFFSET = 0x98
DEFENSE_OFFSET = 0x9c
BASE_VTABLE = 0x501dd0
BASE_HIT = 0x4add10
DAMAGEABLE_HIT = 0x44c8e0
DEFAULT_DEFENSE = 0x43d9b0
# 기존 목록의 주소와 TYPE 관계를 설명하는 입력 파일이다. 어느 파일도 수정하지 않는다.
SOURCE_PATHS = (PE_PATH, VTABLES_PATH, CONSTRUCTORS_PATH, RECIPES_PATH, Path(__file__).resolve())


def digest(path):
    """지정 파일의 실제 바이트를 SHA-256으로 고정한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def relative(path):
    """보고서에 저장소 기준 경로를 슬래시 형식으로 기록한다."""
    return path.relative_to(ROOT).as_posix()


def array_body(path, symbol):
    """기존 C++ 배열 하나의 원문만 읽어 다른 판본의 행이 섞이지 않게 한다."""
    text = path.read_text(encoding='utf-8')
    match = re.search(r'\b' + re.escape(symbol) + r'\s*\{(.*?)\n\}\}?;', text, re.S)
    if match is None:
        raise RuntimeError('기존 배열 원문 누락: ' + relative(path) + ':' + symbol)
    return match.group(1)


def read_pe():
    """PE32의 섹션을 읽으며 주소 조회는 보호 파일의 원시 바이트로만 수행한다."""
    image = PE_PATH.read_bytes()
    if image[:2] != b'MZ':
        raise RuntimeError('보호 원본의 DOS 헤더 오류')
    pe = struct.unpack_from('<I', image, 0x3c)[0]
    if image[pe:pe+4] != b'PE\0\0':
        raise RuntimeError('보호 원본의 PE 서명 오류')
    machine, count = struct.unpack_from('<HH', image, pe+4)
    optional_size = struct.unpack_from('<H', image, pe+20)[0]
    optional = pe+24
    if machine != 0x14c or struct.unpack_from('<H', image, optional)[0] != 0x10b:
        raise RuntimeError('10.78 x86 PE32 원본이 필요하다')
    base = struct.unpack_from('<I', image, optional+28)[0]
    header_size = struct.unpack_from('<I', image, optional+60)[0]
    sections = []
    # 각 섹션의 원시 파일 범위와 실제 로더 주소를 별도로 보존한다.
    for index in range(count):
        offset = optional+optional_size+index*40
        virtual_size, rva, raw_size, raw_offset = struct.unpack_from('<IIII', image, offset+8)
        sections.append((rva, virtual_size, raw_size, raw_offset))

    def bytes_at(address, width):
        """원본 VA의 파일에 존재하는 바이트만 반환하며 가상 여백을 합성하지 않는다."""
        rva = address-base
        if 0 <= rva and rva+width <= header_size:
            return image[rva:rva+width]
        # 요청 전체가 한 원시 섹션에 들어가야 주소를 읽을 수 있다.
        for section_rva, _, raw_size, raw_offset in sections:
            delta = rva-section_rva
            if 0 <= delta and delta+width <= raw_size:
                start = raw_offset+delta
                result = image[start:start+width]
                if len(result) != width:
                    raise RuntimeError('보호 PE 섹션의 파일 범위 오류')
                return result
        raise RuntimeError(f'보호 PE 원시 주소 범위 오류: {address:08x}+{width}')

    if base != 0x400000:
        raise RuntimeError('10.78 보호 PE 기본 주소 불일치')
    return bytes_at


def collect():
    """기존83 VT를 출발점으로 실제+98/+9c 주소와 알려진 생성자 TYPE 관계를 읽는다."""
    tables = [int(value,16) for value in re.findall(
        r'^\s*\{0x([0-9a-fA-F]+)U,', array_body(VTABLES_PATH, 'kPatchDisplayVtables'), re.M)]
    if len(tables) != EXPECTED_TABLE_COUNT or len(set(tables)) != EXPECTED_TABLE_COUNT:
        raise RuntimeError('기존10.78 가상 표83개 고유 주소 불일치')
    constructors = [int(value,16) for value in re.findall(
        r'0x([0-9a-fA-F]+)U', array_body(CONSTRUCTORS_PATH, 'kPatchConstructors'))]
    if len(constructors) != 188:
        raise RuntimeError('기존10.78 생성자188개 배열 불일치')
    recipes = {int(entry,16):int(table,16) for entry,table in re.findall(
        r'^\s*\{0x([0-9a-fA-F]+)U,0x([0-9a-fA-F]+)U,',
        array_body(RECIPES_PATH, 'kPatchRecipes'), re.M)}
    type_references = {table:[] for table in tables}
    unresolved = []
    # 주석의 TYPE 관계는 현재 검증된 생성자 기록만 사용하며 미복원 생성자를 추측하지 않는다.
    for number in range(70, len(constructors)):
        entry = constructors[number]
        if entry and entry not in recipes:
            unresolved.append(dict(type=number,constructor=f'{entry:08x}'))
            continue
        table = recipes[entry] if entry else BASE_VTABLE
        if table not in type_references:
            raise RuntimeError(f'기존83 VT 목록 밖의 생성자 기록: TYPE{number}/{table:08x}')
        type_references[table].append(number)
    read = read_pe()
    rows = []
    # 두 가상 슬롯은 기존 표시 지원 비트와 무관하게 보호 PE에서 직접 읽는다.
    for table in tables:
        rows.append(dict(vtable=f'{table:08x}',
                         hit=f'{struct.unpack("<I",read(table+HIT_OFFSET,4))[0]:08x}',
                         defense=f'{struct.unpack("<I",read(table+DEFENSE_OFFSET,4))[0]:08x}',
                         constructor_types=type_references[table]))
    if read(DEFAULT_DEFENSE,5).hex() != '33c0c20400':
        raise RuntimeError('기본 방어 xor eax,eax;ret4의 실제5바이트 불일치')
    return rows, unresolved


def render(rows):
    """특수 피해·방어 주소도 그대로 남겨 소비자가 명시 경계로 분배할 수 있게 한다."""
    lines = [
        '// 보호10.78 PE의 실제+98/+9c 주소83쌍이다. 생성 도구로 갱신하며 호스트 주소로 호출하지 않는다.',
        '// 알려진 생성자 TYPE 관계만 주석에 기록한다. raw의 현재 VT와 일치하는 실제 진입만 분배한다.',
        '// 일반 Base/Damageable과 사제·신전·특수 피해를 주소로 구별해 기존 엄격한 후속 경계를 유지한다.',
        'constexpr std::array<o::DamageableHitEntry,83> kCampaignDamageHitEntries{{',
    ]
    # 원본 표 순서로 피해 진입과 현재 생성자 TYPE 관계를 출력한다.
    for row in rows:
        types = ','.join(str(value) for value in row['constructor_types']) or '알려진 생성자 연결 없음'
        lines.append(f'    {{0x{row["vtable"]}U,0x{row["hit"]}U}}, // TYPE {types}의 실제 가상+98이다.')
    lines += [
        '}};',
        '// 기본 false와 TYPE142의 특별 방향 방어를 원본 주소 그대로 구별한다. 미복원 진입은 성공으로 바꾸지 않는다.',
        'constexpr std::array<o::SquidDefenseEntry,83> kCampaignDamageDefenseEntries{{',
    ]
    # 방어 표는 피해 표와 같은83 VT 순서이며142의 재정의도 빠뜨리지 않는다.
    for row in rows:
        types = ','.join(str(value) for value in row['constructor_types']) or '알려진 생성자 연결 없음'
        lines.append(f'    {{0x{row["vtable"]}U,0x{row["defense"]}U}}, // TYPE {types}의 실제 가상+9c다.')
    return '\n'.join(lines+['}};', ''])


def summary(rows):
    """주소표의 정적 분포만 세며 원본 함수 실행 횟수로 합산하지 않는다."""
    return dict(hit=dict(sorted(Counter(row['hit'] for row in rows).items())),
                defense=dict(sorted(Counter(row['defense'] for row in rows).items())))


def generate():
    """현재 허용 PC에서 새 include와 정적 근거 JSON만 생성한다."""
    if socket.gethostname().casefold() != EXPECTED_HOST.casefold():
        raise RuntimeError('현재 전체 정밀 디컴파일 PC가 아닌 시스템에서는 재분석이 필요하다')
    rows, unresolved = collect()
    INCLUDE_PATH.write_text(render(rows),encoding='utf-8',newline='\n')
    report = dict(
        schema=1, host=socket.gethostname(), date='2026-10-16', last_full_refined_host=EXPECTED_HOST,
        edition='10.78', evidence='보호 PE 데이터 슬롯의 읽기 전용 정적 관찰',
        static_vtables=len(rows), static_slots=len(rows)*2, dynamic_executions=0, game_gui_runs=0,
        offsets=dict(hit='00000098',defense='0000009c'), distributions=summary(rows),
        rows=rows, unresolved_constructor_types=unresolved,
        default_defense=dict(address=f'{DEFAULT_DEFENSE:08x}',bytes='33c0c20400',behavior='xor eax,eax; ret4'),
        supported_common_hit_addresses=[f'{BASE_HIT:08x}',f'{DAMAGEABLE_HIT:08x}'],
        supported_common_defense_address=f'{DEFAULT_DEFENSE:08x}',
        limitations=[
            '83 VT와 두 슬롯166개는 정적 데이터 행이며 원본 동적 실행·고유 입력 수가 아니다.',
            '특수 피해·방어 주소는 보존하며 일반 몸체로 바꾸지 않는다. 소비자가 별도 파생 또는 명시 경계로 분배해야 한다.',
            'TYPE 주석은 기존 생성자 메타데이터의 관계다. 현재 자산의 가상 표를 TYPE 번호만으로 추정하는 분배를 허용하지 않는다.',
            '미복원 생성자 TYPE153/159/169는 관계표에서 제외한다. CD10.72와 실제 미션의 피해·삭제·경고·AI 완료 증거는 아니다.',
        ],
        sha256={relative(path):digest(path) for path in (*SOURCE_PATHS, INCLUDE_PATH)},
    )
    REPORT_PATH.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
    print('캠페인 피해·방어 정적 표 생성:',len(rows),'VT /',len(rows)*2,'슬롯 / 동적 실행0')


def verify():
    """현재 SHA·기존 목록·실제 PE 슬롯·TYPE 주석·특수 주소 보존을 다시 확인한다."""
    report = json.loads(REPORT_PATH.read_text(encoding='utf-8'))
    expected_paths = {relative(path) for path in (*SOURCE_PATHS, INCLUDE_PATH)}
    if set(report['sha256']) != expected_paths:
        raise RuntimeError('정적 표 원천/생성물 SHA 경로 목록 불일치')
    # 해시만 믿지 않고 현재 입력을 다시 읽는 다음 단계와 함께 바이트 출처를 확인한다.
    for name, expected in report['sha256'].items():
        if digest(ROOT/name) != expected:
            raise RuntimeError('정적 표 SHA 불일치: '+name)
    rows, unresolved = collect()
    if report['rows'] != rows or report['unresolved_constructor_types'] != unresolved:
        raise RuntimeError('현재 PE/생성자 관계와 정적 표 보고서 불일치')
    if INCLUDE_PATH.read_bytes() != render(rows).encode('utf-8'):
        raise RuntimeError('실제 두 슬롯/TYPE 주석과 C++ 생성물 불일치')
    if report['distributions'] != summary(rows) or report['static_vtables'] != len(rows) or report['static_slots'] != len(rows)*2:
        raise RuntimeError('정적 주소 분포/행수 불일치')
    if report['dynamic_executions'] != 0 or report['game_gui_runs'] != 0 or report['last_full_refined_host'] != EXPECTED_HOST:
        raise RuntimeError('정적 분석 범위/호스트 기록 불일치')
    if report['host'].casefold() != EXPECTED_HOST.casefold() or report['edition'] != '10.78' or report['schema'] != 1:
        raise RuntimeError('정적 분석 생성 호스트/판본/형식 기록 불일치')
    if report['offsets'] != dict(hit='00000098',defense='0000009c'):
        raise RuntimeError('정적 분석 가상 슬롯 위치 기록 불일치')
    if report['default_defense'] != dict(address=f'{DEFAULT_DEFENSE:08x}',bytes='33c0c20400',behavior='xor eax,eax; ret4'):
        raise RuntimeError('실제 기본 방어 몸체 기록 불일치')
    if report['supported_common_hit_addresses'] != [f'{BASE_HIT:08x}',f'{DAMAGEABLE_HIT:08x}'] or report['supported_common_defense_address'] != f'{DEFAULT_DEFENSE:08x}':
        raise RuntimeError('소비자가 분배할 일반 피해/방어 주소 기록 불일치')
    print('캠페인 피해·방어 정적 PE/주소/TYPE/SHA 검증 통과:',len(rows),'VT / 동적 실행0')


if __name__ == '__main__':
    # --verify는 생성물을 바꾸지 않고 현재 보호 PE와 기존 자료를 다시 읽기만 한다.
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify',action='store_true')
    arguments = parser.parse_args()
    verify() if arguments.verify else generate()
