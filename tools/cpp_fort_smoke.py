#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""복원한 C++ `.fort` 로더와 타입 표를 실제 두 판본의 파일 전체로 검사한다. 원본 게임은 실행하지 않는다.

- 모든 `.fort`(낱개 파일 + 아카이브)의 구조를 기존 Python 판독기(tools/fort.py)의 결과와 줄 단위로 비교한다.
- C++ 타입 표의 이름 해시가 실제 파일의 `TypeNames` 섹션과 같은지 확인한다(타입 번호 체계의 근거).
- C++ 타입 플래그의 저장 관련 비트를 Python 판독기의 플래그와 비교한다.
- 원본 파일이 바뀌지 않았는지 확인한다.

python tools/cpp_fort_smoke.py
"""
import argparse
import collections
import hashlib
import json
from pathlib import Path
import struct
import subprocess

import fort
from shp import TYPE_LOAD_ORDER, load_type_texts
from taff import TaffArchive
from typefile import parse_type

# C++ 빌드와 보존된 원본 자산의 공통 루트.
ROOT = Path(__file__).resolve().parent.parent
# 검사 산출물을 두는 Git 제외 폴더.
OUTPUT = ROOT / 'extracted/cpp-fort-smoke'
# 판본별 (폴더, 데이터 폴더, 아카이브, 명령줄 판본 인자, .type 타입 수).
EDITIONS = [
    ('originals', 'd', 'netstorm.tarc', [], 116),
    ('originalCD', 'D', 'NETSTORM.TARC', ['--cd'], 101),
]
# 미션 머리 값을 확인할 미션(판본별). TEST01은 사용자가 만든 커스텀 맵, thewarbegins는 캠페인 1-1이다.
MISSIONS = {'originals': ['TEST01', 'thewarbegins', 'savetheisland'], 'originalCD': ['thewarbegins', 'savetheisland']}
# 미션 검사 명령이 적는 머리 값의 키.
MISSION_KEYS = ['title', 'missionNumber', 'moreGeysers', 'myStartMoney', 'myTech', 'myAllyList'] + [
    f'ai{number}{suffix}' for number in range(2, 9) for suffix in ('Name', 'Tech', 'StartMoney', 'AllyList', 'color', 'Ability')]
# 로더가 읽는 섹션 수(실행 파일의 이름 목록 길이).
SECTION_COUNT = 35
# Python 판독기와 비교할 플래그 비트: 저장 형식을 정하는 비트와 플래그 단어가 직접 켜는 비트.
FLAG1_COMPARED = fort.F1_SAVE_FRAME | fort.F1_SAVE_QA | fort.F1_SAVE_QB | fort.F1_CONTAINER
# 그룹 속성과 후처리가 켜는 플래그 2 비트(Python 판독기는 계산하지 않는다).
FLAG2_DERIVED = 0x800 | 0x4000000 | 0x8000000 | 0x10


def run(executable, *arguments):
    """새 C++ 검사 실행 파일만 호출한다."""
    result = subprocess.run([str(executable), *map(str, arguments)], capture_output=True, timeout=600)
    if result.returncode:
        raise RuntimeError(result.stderr.decode('utf-8', errors='replace'))
    return result.stdout


class Catalog(fort.TypeCatalog):
    """판본의 로딩 순서 길이에 맞춘 Python 타입 목록(CD판은 앞 101개)."""
    def __init__(self, root, data_dir, count):
        """원본 .type 글에서 플래그를 만든다. Python 판독기의 규칙을 그대로 쓴다."""
        texts = load_type_texts(str(root)) if data_dir == 'd' else load_type_texts_cd(root)
        self.order = TYPE_LOAD_ORDER[:count]
        self.flags = {}
        # 로딩 목록의 타입마다 typeflags 를 비트로 바꾼다.
        for name in self.order:
            definition = parse_type(texts[name.lower()])
            f1 = f2 = 0
            # 플래그 단어마다 대응 비트를 더한다(대소문자 무시).
            for word in definition.flags:
                f1 |= next((b for k, b in fort.FLAG1_BITS.items() if k.lower() == word.lower()), 0)
                f2 |= next((b for k, b in fort.FLAG2_BITS.items() if k.lower() == word.lower()), 0)
            self.flags[name] = fort.derive_flags(f1, f2)
        self.by_hash = {fort.type_hash(n): n for n in self.order}

    def conversion(self, typenames):
        """TypeNames 섹션으로 "파일 타입 번호 → 이름" 표를 만든다."""
        if len(typenames) == 0:
            return {fort.TYPE_INDEX_BASE + i: n for i, n in enumerate(self.order)}
        count = typenames[0]
        hashes = struct.unpack_from(f'<{count}I', typenames, 1)
        return {i: self.by_hash[h] for i, h in enumerate(hashes) if h in self.by_hash}


def load_type_texts_cd(root):
    """CD판 아카이브에서 .type 글을 읽는다(이름은 소문자 키)."""
    archive = TaffArchive(str(root / 'NETSTORM.TARC'))
    texts = {}
    # 아카이브의 .type 엔트리를 모두 읽는다.
    for entry in archive.entries:
        if entry.name.lower().endswith('.type'):
            texts[entry.name.lower().rsplit('\\', 1)[-1][:-5]] = archive.read(entry).decode('latin-1')
    return texts


def iter_forts(root, data_dir, archive_name):
    """판본의 모든 .fort를 (이름, 바이트)로 돌려준다. C++ 명령과 같은 순서다."""
    # 낱개 파일은 이름순.
    for path in sorted((root / data_dir).glob('*.fort'), key=lambda p: p.name):
        yield path.name, path.read_bytes()
    archive = TaffArchive(str(root / archive_name))
    # 아카이브는 디렉터리 순서.
    for entry in archive.entries:
        if entry.name.lower().endswith('.fort'):
            yield 'tarc:' + entry.name, archive.read(entry)


def split_sections(data):
    """머리 2바이트 뒤의 길이 접두 섹션을 최대 35개 자른다. (섹션 목록, 남은 바이트 수)"""
    sections, pos = [], 2
    # 길이에는 길이 필드 2바이트가 들어 있다.
    while len(sections) < SECTION_COUNT and pos + 2 <= len(data):
        length, = struct.unpack_from('<H', data, pos)
        if length < 2 or length > 0x7fff or pos + length > len(data):
            break
        sections.append(data[pos + 2:pos + length])
        pos += length
    return sections, len(data) - pos


def script_value(text, key):
    """미션 스크립트에서 키의 첫 줄 값을 읽는다(따옴표 제거, 따옴표 밖 주석 제거, 끝 공백 제거). 없으면 None."""
    # 줄을 훑어 `키 =` 로 시작하는 첫 줄을 찾는다.
    for line in text.split('\n'):
        stripped = line.lstrip(' \t')
        if not stripped.lower().startswith(key.lower()):
            continue
        rest = stripped[len(key):].lstrip(' \t')
        if not rest.startswith('='):
            continue
        rest = rest.lstrip('= \t').rstrip('\r')
        value, quoted = '', False
        # 따옴표는 빼고, 따옴표 밖의 // 부터는 버린다.
        for index, char in enumerate(rest):
            if char == '"':
                quoted = not quoted
            elif not quoted and rest[index:index + 2] == '//':
                break
            else:
                value += char
        return value.rstrip(' \t')
    return None


def check_missions(executable, root, data_dir, archive_name, flag, missions):
    """미션 검사 명령의 머리 값과 요새 경로를 스크립트 글에서 직접 읽은 값과 비교한다."""
    archive = TaffArchive(str(root / archive_name))
    checked = {}
    # 미션마다 스크립트를 디스크 → 아카이브 순서로 찾는다.
    for mission in missions:
        loose = root / data_dir / f'{mission}.english'
        if loose.exists():
            text = loose.read_bytes().decode('cp1252')
        else:
            entry = next(e for e in archive.entries if e.name.lower() == f'\\d\\{mission.lower()}.english')
            text = archive.read(entry).decode('cp1252')
        output = run(executable, '--inspect-mission', root, mission, *flag).decode('utf-8').splitlines()
        fort_name = script_value(text, 'loadFort') or mission
        wanted = [f'Mission: {mission}', f'  script: \\D\\{mission}.english (loaded)',
                  f'  mission type: {script_value(text, "missionType") or ""}', f'  fort: {fort_name} -> \\D\\{fort_name}.fort']
        # 스크립트에 있는 머리 값만 줄이 생긴다.
        for key in MISSION_KEYS:
            value = script_value(text, key)
            if value is not None:
                wanted.append(f'  {key} = {value}')
        wanted.append(f'Fort: \\D\\{fort_name}.fort')
        if output[:len(wanted)] != wanted:
            raise AssertionError(f'미션 머리 값 불일치: {root.name} {mission}: {output[:len(wanted)]} / {wanted}')
        checked[mission] = len(wanted) - 5
    return checked


def signed16(value):
    """부호 없는 16비트를 부호 있는 값으로 바꾼다."""
    return value - 0x10000 if value >= 0x8000 else value


def describe_contents(items, stored_flags):
    """내용물 목록을 C++ 출력과 같은 꼴로 적는다."""
    parts = []
    # 항목을 저장 순서대로 적는다.
    for item in items:
        quantity = item['qa'] if 'qa' in item else signed16(item['qb'] & 0xffff) if 'qb' in item else 0
        text = f"({item['type']} {quantity}"
        if stored_flags:
            text += f" lf={item['listFlags']}"
        if 'contents' in item:
            text += ' ' + describe_contents(item['contents'], False)
        parts.append(text + ')')
    return '[' + ''.join(parts) + ']'


def describe(name, data, catalog):
    """Python 판독기로 파일 하나를 읽어 C++ 출력과 같은 글로 적는다."""
    sections, trailing = split_sections(data)
    named = {fort.SECTION_NAMES[i]: s for i, s in enumerate(sections)}
    lines = [f'FORT {name} flag={data[1]} sections={len(sections)} trailing={trailing}']
    subscriber = named.get('Subscriber', b'')
    if subscriber:
        ident, length = struct.unpack_from('<IH', subscriber, 0)
        lines.append(f'SUB {ident} {subscriber[6:6 + length].hex() or "-"}')
    money = named.get('Money', b'')
    if len(money) >= 4:
        lines.append(f'MONEY {struct.unpack_from("<I", money)[0]:08x}')
    typenames = named.get('TypeNames', b'')
    lines.append(f'TYPES {typenames[0] if typenames else 0}')
    conversion = catalog.conversion(typenames)
    technology = named.get('Technology', b'')
    if technology:
        # 목록 뒤의 남는 바이트는 Python 판독기가 0 한 개만 허용한다. 개수로 직접 센다.
        items = fort.read_technology(technology, conversion, catalog)
        consumed = 1 + sum(technology_item_size(item) for item in items)
        lines.append(f'TECH {len(items)} trailing={len(technology) - consumed} ' + describe_contents(items, True))
    deck = named.get('Deck', b'')
    if deck:
        entries = []
        # 항목은 4바이트: 타입, chance, power(부호 있음), 남은 횟수.
        for i in range(deck[0]):
            stored, chance, power, remaining = struct.unpack_from('<BBbB', deck, 1 + 4 * i)
            entries.append(f' ({conversion.get(stored, "#0")} {chance} {power} {remaining})')
        lines.append(f'DECK {deck[0]}' + ''.join(entries))
    # 오브젝트가 들어 있는 섹션.
    for section_name in ['Chaff'] + [f'Terr{i:02d}' for i in range(20)]:
        section = named.get(section_name, b'')
        if len(section) <= 1:
            continue
        version, chunks = fort.read_chunks(section, conversion, catalog)
        lines.append(f'SEC {section_name} v={version} chunks={len(chunks)}')
        # 청크 번호는 섹션 안의 저장 순서다.
        for index, objects in enumerate(chunks):
            # 청크 안의 오브젝트를 저장 순서대로 적는다.
            for obj in objects:
                fields = obj.fields
                text = f'O {index} {obj.cell[0]},{obj.cell[1]} {obj.type_index} {obj.type_name}'
                if obj.type_index >= fort.BRIDGE_SHORTHAND_MIN:
                    text += f" sh={fields['owner']}"
                if 'frame' in fields:
                    text += f" frame={fields['frame']}"
                if 'qa' in fields:
                    text += f" qa={fields['qa']}"
                if 'qb' in fields:
                    text += f" qb={signed16(fields['qb'])}"
                if 'bridge' in fields:
                    text += f" bridge={fields['bridge']}"
                if 'factory' in fields:
                    text += f" factory={fields['factory']}"
                if 'owner' in fields and obj.type_index < fort.BRIDGE_SHORTHAND_MIN:
                    text += f" owner={fields['owner']}"
                if 'skip' in fields:
                    text += f" skip={fields['skip']}"
                if 'contents' in fields:
                    text += ' contents=' + describe_contents(fields['contents'], False)
                lines.append(text)
    return lines


def technology_item_size(item):
    """Technology 항목 하나가 파일에서 차지하는 바이트 수(Python 판독기의 읽기 순서 기준)."""
    size = 2  # 타입 번호와 목록 플래그.
    size += 1 if 'qa' in item else 0
    size += 2 if 'qb' in item else 0
    if 'contents' in item:
        size += 1 + sum(technology_item_size(nested) - 1 for nested in item['contents'])
    return size


def main():
    """두 판본의 `.fort` 전체와 타입 표를 검사하고 보고서를 남긴다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=ROOT / 'cpppj/build/bin/Release/NetstormCpp.exe')
    args = parser.parse_args()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    report = {}
    tables = {}
    # 판본마다 타입 표를 먼저 읽는다. 낱개 파일에는 다른 판본으로 저장한 것도 섞여 있다.
    for edition, data_dir, archive_name, flag, type_count in EDITIONS:
        rows = [line.split('\t') for line in run(args.exe, '--dump-types', ROOT / edition, *flag).decode('utf-8').splitlines()]
        tables[edition] = rows
    # 판본마다 타입 플래그·파일 구조·해시·미션을 검사한다.
    for edition, data_dir, archive_name, flag, type_count in EDITIONS:
        root = ROOT / edition
        watched = [root / archive_name] + sorted((root / data_dir).glob('*.fort'))
        before = hashlib.sha256(b''.join(hashlib.sha256(path.read_bytes()).digest() for path in watched)).hexdigest()
        catalog = Catalog(root, data_dir, type_count)
        rows = tables[edition]
        # 1) 타입 플래그: 저장 관련 비트와 플래그 단어가 직접 켜는 비트를 Python 판독기와 비교한다.
        for index, name in enumerate(catalog.order):
            row = rows[fort.TYPE_INDEX_BASE + index]
            f1, f2 = catalog.flags[name]
            # 원본 구조체의 이름은 20글자를 넘으면 설명 필드와 이어져 읽히므로 앞 20글자만 비교한다.
            if row[1][:20] != name[:20]:
                raise AssertionError(f'타입 이름 불일치: {edition} {row[1]} / {name}')
            if (int(row[3], 16) & FLAG1_COMPARED) != (f1 & FLAG1_COMPARED) or (int(row[4], 16) & ~FLAG2_DERIVED) != (f2 & ~FLAG2_DERIVED):
                raise AssertionError(f'타입 플래그 불일치: {edition} {name}: {row[3]} {row[4]} / {f1:08x} {f2:08x}')
        # 2) 파일 전체 비교.
        expected = []
        hashes_by_table = collections.Counter()
        type_counts = collections.Counter()
        objects = 0
        files = 0
        # Python 판독기의 결과를 C++ 출력과 같은 글로 만든다.
        for name, data in iter_forts(root, data_dir, archive_name):
            files += 1
            lines = describe(name, data, catalog)
            expected += lines
            objects += sum(1 for line in lines if line.startswith('O '))
            sections, _ = split_sections(data)
            typenames = sections[7] if len(sections) > 7 else b''
            if typenames:
                stored = [f'{h:08x}' for h in struct.unpack_from(f'<{typenames[0]}I', typenames, 1)]
                type_counts[typenames[0]] += 1
                # 3) 이 파일의 해시 표가 어느 판본의 C++ 타입 표와 완전히 같은지 센다.
                for table_edition, table in tables.items():
                    if stored == [row[2] for row in table]:
                        hashes_by_table[table_edition] += 1
        actual = run(args.exe, '--dump-forts', root, *flag).decode('utf-8').splitlines()
        if actual != expected:
            (OUTPUT / f'{edition}-actual.txt').write_text('\n'.join(actual) + '\n', encoding='utf-8', newline='\n')
            (OUTPUT / f'{edition}-expected.txt').write_text('\n'.join(expected) + '\n', encoding='utf-8', newline='\n')
            first = next((i for i, (a, b) in enumerate(zip(actual, expected)) if a != b), min(len(actual), len(expected)))
            raise AssertionError(f'.fort 구조 불일치: {edition} 줄 {first + 1} (extracted/cpp-fort-smoke 에 두 출력을 남김)')
        if not hashes_by_table.get(edition):
            raise AssertionError(f'{edition}: C++ 타입 표와 해시가 완전히 같은 TypeNames 섹션이 없음')
        after = hashlib.sha256(b''.join(hashlib.sha256(path.read_bytes()).digest() for path in watched)).hexdigest()
        if before != after:
            raise AssertionError(f'원본 파일 변경 감지: {edition}')
        missions = check_missions(args.exe, root, data_dir, archive_name, flag, MISSIONS[edition])
        report[edition] = {'fort_files': files, 'objects': objects, 'compared_lines': len(expected),
                           'mission_head_values': missions,
                           'type_table_size': len(rows), 'typenames_counts': dict(sorted(type_counts.items())),
                           'typenames_identical_to_table': dict(hashes_by_table), 'originals_sha256': before}
    (OUTPUT / 'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
