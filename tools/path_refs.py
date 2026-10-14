#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""같은 내용의 LF/CRLF 보존본을 전역 SHA 대신 파일 경로별로 연결한다."""
import hashlib
import argparse
import json
import re
from pathlib import Path
from functools import lru_cache

ROOT = Path(__file__).resolve().parents[1]  # 모든 수정 대상은 경로 대응표 내부의 JSON 장부로 제한한다.
MAP = ROOT / 'docs/path-map.json'  # 최초 Git 원문과 최종 파일별 SHA를 보존한다.
HASH = re.compile(r'^[0-9a-fA-F]{64}$')  # 완전한 SHA256 값만 참조 교정 대상으로 삼는다.


def digest(body):
    """원문 전체 바이트의 SHA256을 계산한다."""
    return hashlib.sha256(body).hexdigest()


@lru_cache(maxsize=None)
def names(report, name):
    """현재 파일·보존 루트·평면 보존 위치의 경로 후보를 순서대로 반환한다."""
    result = [ROOT / name]
    # 보존 장부는 가까운 당시 루트와 평면 파일을 현재 루트보다 먼저 사용한다.
    if 'hist' in report.parts:
        result = [report.parent / name, report.parent / Path(name).name]
        for parent in report.parents:
            if parent == ROOT:
                break
            result.append(parent / name)
        result.append(ROOT / name)
    return list(dict.fromkeys(path.relative_to(ROOT).as_posix() for path in result if path.is_relative_to(ROOT)))


def update(value, report, entries, owned, source=None):
    """실제로 지정한 파일의 기존 SHA와 일치하는 필드만 현재 SHA/크기로 연결한다."""
    def pin(name, expected):
        """이름과 이전 전체 SHA가 모두 일치하는 대응표 항목을 찾는다."""
        # 이름이 일치하는 현재/보존 위치만 SHA와 함께 검사한다.
        for candidate in names(report, name):
            entry = entries.get(candidate)
            if not entry:
                continue
            known = {entry['before'], entry['current_before'], entry['after'], entry.get('migration_after'), *entry.get('eol_hashes', ()), *entry.get('sha_history', ())}
            if expected.lower() in known:
                return entry
        return None

    def historical(name, expected, kind):
        """같은 장부에 명시된 work/git 보존본을 역할과 파일명으로 정확히 연결한다."""
        suffix = '/' + kind + '/' + name
        if 'hist' in report.parts:
            # 평면 baseline.json은 자기 폴더의 work/git 트리를 직접 가리킨다.
            for parent in report.parents:
                if parent == ROOT:
                    break
                relative = (parent / kind / name).relative_to(ROOT).as_posix()
                entry = pin(relative, expected)
                if entry:
                    return entry
        # 다른 이력 단위의 같은 이름을 추측하지 않고 장부가 가진 경로만 사용한다.
        for candidate in owned:
            if candidate.endswith(suffix):
                entry = pin(candidate, expected)
                if entry:
                    return entry
        return None

    if isinstance(value, dict):
        result = {}
        # 경로를 키로 사용하는 files/source_hashes 장부를 먼저 교정한다.
        for key, child in value.items():
            if isinstance(child, str) and HASH.fullmatch(child):
                kind = 'git' if 'git' in key else 'work' if 'work' in key else None
                entry = historical(source, child, kind) if source and kind else pin(key, child)
                result[key] = entry['after'] if entry else child
                if entry and kind and kind + '_bytes' in value:
                    result[kind + '_bytes'] = entry['final_size']
            else:
                result[key] = update(child, report, entries, owned, key if '/' in key else source)
        path = value.get('path')
        if isinstance(path, str):
            # pin 형식의 경로·바이트수·SHA를 함께 바꾸어 길이 검증도 일관되게 유지한다.
            for key in ('sha256', 'sha', 'hash'):
                expected = value.get(key)
                if isinstance(expected, str) and HASH.fullmatch(expected):
                    entry = pin(path, expected)
                    if entry:
                        result[key] = entry['after']
                        if 'bytes' in result:
                            result['bytes'] = entry['final_size']
        return result
    if isinstance(value, list):
        # 판본별/단계별 중첩된 pin 목록도 같은 파일별 규칙으로 교정한다.
        return [update(child, report, entries, owned, source) for child in value]
    return value


def owned_paths(value, entries):
    """JSON 장부가 직접 명시한 보존 파일의 경로만 수집한다."""
    found = set()
    if isinstance(value, dict):
        # 파일 키와 path 필드 양쪽에서 실제 대응표 항목을 찾는다.
        for key, child in value.items():
            if key in entries:
                found.add(key)
            if key == 'path' and isinstance(child, str) and child in entries:
                found.add(child)
            found.update(owned_paths(child, entries))
    elif isinstance(value, list):
        # 단계별 보존 목록도 같은 장부 내부의 경로에 포함한다.
        for child in value:
            found.update(owned_paths(child, entries))
    return tuple(sorted(found))


def main(apply=False):
    """SHA를 경로별로 수렴시킨 뒤 JSON 장부와 대응표만 변경하고 원본 입력은 유지한다."""
    data = json.loads(MAP.read_text(encoding='utf-8'))
    assert not apply or not data.get('finalized'), '마감한 이관 대응표에는 새 교정을 적용하지 않습니다.'
    entries = {entry['target']: entry for entry in data['files'].values()}
    templates = {}
    # 입력/기대값이 아닌 분석 JSON 장부만 읽으며 소스·fixture의 바이트는 바꾸지 않는다.
    for name, entry in entries.items():
        path = ROOT / name
        entry['final_size'] = path.stat().st_size
        if path.suffix != '.json' or not (name.startswith('cpppj/') or name.startswith('tools/ghidra/')):
            continue
        body = path.read_bytes()
        try:
            document = json.loads(body.decode('utf-8-sig'))
        except (UnicodeError, json.JSONDecodeError):
            continue
        templates[name] = (document, body, b'\r\n' in body, owned_paths(document, entries))
    groups = {}
    # 같은 원문을 보존한 JSON들은 같은 참조 값을 유지하여 전체 원문 비교가 계속 성립하게 한다.
    for name in templates:
        entry = entries[name]
        group = entry.get('eol_hashes', [entry['before']])[0]
        groups.setdefault(group, []).append(name)
    representatives = {}
    # 전체 work 보존본을 우선하고 같은 JSON 값에는 원래 각 파일의 줄바꿈만 유지한다.
    for members in groups.values():
        selected = min(members, key=lambda name: (0 if '/work/' in name and '/hist/' in name else 1 if '/hist/' in name else 2, len(Path(name).parts), name))
        for name in members:
            representatives[name] = selected
    changed = {}
    for iteration in range(80):
        updates = {}
        # 매번 원래 JSON 값에서 계산하여 중간 SHA의 누적 치환을 방지한다.
        for name, (document, original, crlf, owned) in templates.items():
            selected = representatives[name]
            selected_document, _, _, selected_owned = templates[selected]
            result = update(selected_document, ROOT / selected, entries, selected_owned)
            body = original if result == document else (json.dumps(result, ensure_ascii=False, indent=2) + '\n').encode('utf-8')
            if crlf and result != document:
                body = body.replace(b'\n', b'\r\n')
            after = digest(body)
            if entries[name]['after'] != after:
                updates[name] = (body, after)
        if not updates:
            break
        # 한 단계의 SHA를 동시에 반영하여 의존 JSON의 다음 계산에 전달한다.
        for name, (body, after) in updates.items():
            entry = entries[name]
            history = entry.setdefault('sha_history', [])  # 여러 교정 단계의 중간 SHA도 후속 장부가 역조회할 수 있게 유지한다.
            if entry['after'] not in history:
                history.append(entry['after'])
            entry.setdefault('migration_after', entry['after'])
            entry['after'] = after
            entry['final_size'] = len(body)
            changed[name] = body
        print('파일별 SHA', iteration + 1, '단계', len(updates), '파일', flush=True)
    else:
        raise RuntimeError('파일별 SHA 참조가 수렴하지 않아 쓰기를 중단합니다.')
    if not apply:
        print('파일별 SHA 참조 검증: 교정 필요', len(changed), '파일')
        raise SystemExit(1 if changed else 0)
    # 계산이 완료된 뒤 필요한 JSON만 쓰고 최초 이관 캐시는 별도 이름으로 보존한다.
    for name, body in changed.items():
        (ROOT / name).write_bytes(body)
        if 'cache' in entries[name]:
            entries[name]['migration_cache'] = entries[name].pop('cache')
    data['context_sha_passes'] = iteration + 1
    data['context_sha_files'] = len(changed)
    MAP.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print('파일별 SHA 연결 완료', len(changed))


# 명시 실행에서만 분석 장부의 경로별 참조를 교정한다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply', action='store_true')
    arguments = parser.parse_args()
    main(apply=arguments.apply)
