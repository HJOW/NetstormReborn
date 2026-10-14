#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""긴 분석 이력·파일 이름을 줄이고 참조와 전체 SHA 장부를 일관되게 이관한다."""
import argparse
import hashlib
import json
import os
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # 이동 전후의 모든 대상은 이 작업공간 안에 있어야 한다.
MAP = ROOT / 'docs/path-map.json'  # 이전 이름과 새 이름·전체 바이트 변경 근거를 별도로 기록한다.
SKIP = {'.git', 'originals', 'originalCD', 'originalFanGame', 'playingVideos', 'fonts'}  # 보호·외부자료는 이관하지 않는다.
TEXT = {'.py', '.cpp', '.h', '.hpp', '.c', '.cs', '.inc', '.csproj', '.props', '.manifest', '.sln', '.json', '.md', '.txt', '.tsv', '.log', '.stackdump', '.ps1', '.java', '.cmake', '.yml', '.yaml', '.sh', '.bat', '.cfg'}  # 실제 참조를 포함할 수 있는 소스와 빌드 정의의 텍스트 형식이다.
NAME_LIMIT = 48  # 이번 기존파일 정리의 상한이며 신규파일은 문서의40자 기준을 따른다.
PATH_LIMIT = 200  # 작업공간 루트를 포함한 절대경로에 적용할 안전 여유 한도다.
HASH = re.compile(r'(?<![0-9a-fA-F])[0-9a-fA-F]{64}(?![0-9a-fA-F])')  # 전체 SHA256 토큰만 바꾸며 임의 숫자 부분은 건드리지 않는다.
PROOF_NAME = re.compile(r'recovery-([A-Za-z0-9_{}.*-]+)-evidence\.json')  # 실제 파일과 f문자열/와일드카드의 동일한 접두·접미를 함께 줄인다.


def digest(body):
    """전체 원문 바이트의 SHA256을 계산한다."""
    return hashlib.sha256(body).hexdigest()


def inventory():
    """Git 추적파일과 로컬 이력·분석파일을 함께 읽되 빌드/보호자료는 제외한다."""
    tracked = subprocess.check_output(['git', 'ls-files', '-z'], cwd=ROOT).decode('utf-8').split('\0')
    found = {ROOT / name for name in tracked if name and not name.split('/')[0].startswith('original') and name.split('/')[0] not in SKIP and Path(name).name != 'AGENTS.md'}
    # 보존 이력의 ignored 바이너리·로그도 포함하지만 영상·Ghidra 캐시·게임복사본은 읽지 않는다.
    for folder in (ROOT / 'cpppj/recovery-history', ROOT / 'cpppj/hist', ROOT / 'tools', ROOT / 'docs'):
        if not folder.exists():
            continue
        for current, directories, files in os.walk(folder):
            directories[:] = [name for name in directories if name not in {'.git', '__pycache__', 'node_modules'}]
            for name in files:
                path = Path(current) / name
                if path != MAP:
                    found.add(path)
    # 현재 원본 실행기의 참조가 있는 평면 분석 로그·보고서만 추가하고 큰 녹화/이미지는 건드리지 않는다.
    for path in (ROOT / 'extracted').iterdir():
        if path.is_file() and not path.name.startswith('path-') and path.suffix in {'.log', '.json', '.py', '.txt'}:
            found.add(path)
    return sorted(path for path in found if path != MAP and path.name != 'AGENTS.md')


def shorten(name, limit):
    """역할을 읽을 수 있는 접두와 짧은 식별자를 남겨 같은 이름 충돌을 피한다."""
    if len(name) <= limit:
        return name
    tag = hashlib.sha256(name.encode('utf-8')).hexdigest()[:5]  # 이전 전체 이름의 안정적인 충돌 방지 꼬리다.
    return name[:limit - 6].rstrip('-_ ') + '-' + tag


def aliases(paths):
    """이력 루트·직접하위 디렉토리·긴 파일의 전역 대응표를 구성한다."""
    result = {'recovery-history': 'hist', 'campaign-runtime': 'run', 'initial-tests': 'first', 'final-tests': 'final', 'repaired-tests': 'fixed'}
    # 중첩된 이력에서도 동일한 이전 디렉토리 이름은 동일한 짧은 이름으로 바꾼다.
    for path in paths:
        parts = path.relative_to(ROOT).parts
        for index, part in enumerate(parts[:-1]):
            if part == 'recovery-history' and index + 1 < len(parts) - 1:
                previous = parts[index + 1]
                result[previous] = shorten(previous, 18)
        name = path.name
        if name.startswith('recovery-') and name.endswith('-evidence.json'):
            result[name] = path.stem.removeprefix('recovery-').removesuffix('-evidence') + path.suffix
            continue
        limit = NAME_LIMIT if path.suffix in {'.py', '.cpp', '.h', '.json', '.md'} else 64  # 과거 이미지·로그는64자, 소스·보고서는48자 안에 둔다.
        if len(name) <= limit:
            continue
        stem, suffix = path.stem, path.suffix
        if name.endswith('.py'):
            current = stem.replace('analyze_', 'check_').replace('_normalterrain_binding_', '_terrain_').replace('_priest_arrival_cleanup_binding_', '_arrival_cleanup_')
            current = current.replace('_correction', '_fix').replace('_lineage', '_ln')
            current = shorten(current, NAME_LIMIT - len(suffix)).replace('-', '_')
            result[stem] = current
        else:
            current = stem.replace('campaign-runtime', 'run').replace('initial-tests', 'first').replace('final-tests', 'final').replace('repaired-tests', 'fixed')
            result[name] = shorten(current, limit - len(suffix)) + suffix
    return {old: new for old, new in result.items() if old != new}


def decoder(body):
    """기존 UTF-8/UTF-16 BOM을 유지하고 참조 문자열만 읽고 쓴다."""
    if body.startswith(b'\xff\xfe'):
        return body[2:].decode('utf-16-le'), lambda text: b'\xff\xfe' + text.encode('utf-16-le')
    if body.startswith(b'\xfe\xff'):
        return body[2:].decode('utf-16-be'), lambda text: b'\xfe\xff' + text.encode('utf-16-be')
    return body.decode('utf-8'), lambda text: text.encode('utf-8')


def plan():
    """실제 이동과 쓰기 전에 충돌·보호경계·전체길이·SHA 참조의 수렴을 확인한다."""
    paths = inventory()
    names = aliases(paths)
    tokens = [name for name in names if not (name.startswith('recovery-') and name.endswith('-evidence.json'))]  # 보고서 접두/접미는 별도 정규식이 처리한다.
    pattern = re.compile('|'.join(re.escape(name) for name in sorted(tokens, key=len, reverse=True)))
    replace = lambda text: pattern.sub(lambda match: names[match.group()], PROOF_NAME.sub(r'\1.json', text))  # 같은 규칙을 실제파일·정적/동적참조에 적용한다.
    entries, templates, changed, occupied = {}, {}, {}, set()
    # 전체 파일의 원문을 기준으로 이관하고 JSON SHA의 다음 상태를 별도 계산한다.
    for source in paths:
        previous = source.relative_to(ROOT).as_posix()
        target_name = replace(previous)
        target = ROOT / target_name
        assert source.resolve().is_relative_to(ROOT) and target.resolve().is_relative_to(ROOT), previous
        assert not target_name.split('/')[0] in SKIP and target_name != 'AGENTS.md'
        assert target_name.casefold() not in occupied, target_name
        occupied.add(target_name.casefold())
        body = source.read_bytes()
        entries[previous] = dict(target=target_name, before=digest(body), size=len(body))
        if len(entries) % 2000 == 0:
            print('전체 원문 조사', len(entries), '파일', flush=True)
        if source.suffix not in TEXT and source.name not in {'.gitattributes', '.gitignore', 'CMakeLists.txt'}:
            continue
        # 큰 원본격자/디컴파일은 참조 토큰이 없으면 전체 SHA만 기록해 기대출력 바이트를 그대로 유지한다.
        if len(body) > 1024 * 1024 and source.suffix != '.json' and not any(token in body for token in (b'recovery-', b'campaign-runtime', b'initial-tests', b'final-tests', b'analyze_', b'before-')):
            continue
        try:
            text, encode = decoder(body)
        except UnicodeError:
            continue
        next_text = replace(text)
        templates[previous] = (next_text, encode, set(value.lower() for value in HASH.findall(next_text)))
        if next_text != text:
            changed[entries[previous]['before']] = digest(encode(next_text))
    print('이관 계획: 전체파일', len(entries), '텍스트', len(templates), '경로 대응', len(names), flush=True)
    # 보고서/manifest의 서로 다른 SHA 참조를 원래 원문에서 다시 계산해 누적 치환을 피한다.
    for iteration in range(80):
        output, following = {}, dict(changed)
        for previous, (text, encode, references) in templates.items():
            current = HASH.sub(lambda match: changed.get(match.group().lower(), match.group()), text) if references.intersection(changed) else text
            body = encode(current)
            output[previous] = body
            before, after = entries[previous]['before'], digest(body)
            if before != after:
                if before in following and following[before] != after:
                    following[before] = after
                else:
                    following[before] = after
        if following == changed:
            break
        changed = following
        if iteration % 10 == 0:
            print('전체 SHA 참조 계산', iteration + 1, '단계', len(changed), '개', flush=True)
    else:
        raise RuntimeError('SHA 참조가 수렴하지 않아 원문을 변경하지 않습니다')
    for previous, entry in entries.items():
        entry['after'] = digest(output[previous]) if previous in output else entry['before']
    tracked = set(subprocess.check_output(['git', 'ls-files', '-z'], cwd=ROOT).decode('utf-8').split('\0'))
    relevant = [entry['target'] for previous, entry in entries.items() if previous in tracked]
    longest = max((len(str(ROOT / name)), name) for name in relevant)
    assert longest[0] <= PATH_LIMIT, longest
    assert max(len(Path(name).name) for name in relevant) <= 64
    return entries, names, output, dict(tracked_files=len(relevant), moved_files=sum(key != value['target'] for key, value in entries.items()),
                                      changed_files=sum(value['before'] != value['after'] for value in entries.values()),
                                      max_tracked_path=longest[0], longest_path=longest[1], sha_passes=iteration + 1)


def apply():
    """계획이 검증된 뒤 같은 작업공간 안에서 원문 이동과 참조 변경을 수행한다."""
    assert not MAP.exists(), '이미 작성한 이관 대응표를 덮지 않습니다'
    entries, names, output, summary = plan()
    # 파일 단위로 이동하므로 원본/별도 작업 디렉토리를 재귀 이동하거나 삭제하지 않는다.
    for previous, entry in entries.items():
        source, target = ROOT / previous, ROOT / entry['target']
        assert digest(source.read_bytes()) == entry['before'], previous
        if source != target:
            assert source.resolve().is_relative_to(ROOT) and target.resolve().is_relative_to(ROOT)
            assert not target.exists(), entry['target']
            target.parent.mkdir(parents=True, exist_ok=True)
            source.rename(target)
        if entry['before'] != entry['after']:
            target.write_bytes(output[previous])
        assert digest(target.read_bytes()) == entry['after'], entry['target']
    # 이동 뒤 비어 있는 이력 디렉토리만 지우며 비어 있지 않은 디렉토리는 그대로 둔다.
    for current, directories, files in os.walk(ROOT / 'cpppj/recovery-history', topdown=False):
        folder = Path(current)
        if folder.resolve().is_relative_to(ROOT) and not any(folder.iterdir()):
            folder.rmdir()
    MAP.parent.mkdir(parents=True, exist_ok=True)
    MAP.write_text(json.dumps(dict(schema=1, baseline_commit='9d7acad', host='HJOW-Athlon', date='2026-10-14', aliases=names, summary=summary, files=entries), ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(summary, ensure_ascii=False))


def verify():
    """이관한 전체 바이트와 이름 길이를 변경 없이 다시 확인한다."""
    data = json.loads(MAP.read_text(encoding='utf-8'))
    assert not data.get('pending'), '미완료 대응표는 --resume으로 먼저 적용합니다.'
    # Git 기준 원문은이관 대응표에 남기며 현재 전체 자료는 이동 뒤 SHA로 확인한다.
    for previous, entry in data['files'].items():
        target = ROOT / entry['target']
        assert digest(target.read_bytes()) == entry['after'], entry['target']
        if previous != entry['target']:
            assert not (ROOT / previous).exists(), previous
    print('경로 이관 전체 SHA 감사 통과', data['summary'])


# 기본은 읽기 전용 계획이며 명시 --apply에서만 검증된 이동을 수행한다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply', action='store_true')
    parser.add_argument('--verify', action='store_true')
    arguments = parser.parse_args()
    if arguments.verify:
        verify()
    elif arguments.apply:
        apply()
    else:
        print(json.dumps(plan()[3], ensure_ascii=False))
