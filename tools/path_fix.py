#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""중단된 경로 이관을 기준 Git 원문과 실제 현재 파일에서 재구성하여 마감한다."""
import json
import re
import subprocess
from pathlib import Path
import path_move as move

ROOT = move.ROOT  # 기준커밋과 현재 파일은 같은 작업공간을 사용한다.
BASE = '9d7acad'  # 경로변경 전 단위15의 완료 커밋이다.
KEEP = {'.gitattributes', '.github/workflows/ci.yml', 'LEFT_JOBS.md'}  # 새 정책·사용자 인수인계 편집은 현재 원문으로 유지한다.


def recover():
    """Git 원문·현재 원문의 두 SHA를 같은 최종 경로로 연결하여 남은 참조를 닫는다."""
    assert not move.MAP.exists()
    tree = subprocess.check_output(['git', 'ls-tree', '-r', '-z', BASE], cwd=ROOT).decode('utf-8').split('\0')
    tracked = {}
    # 보호 원본·외부 게임자료·AGENTS.md를 제외한 기준 파일만 읽는다.
    for row in tree:
        if not row:
            continue
        header, name = row.split('\t', 1)
        if name.split('/')[0].startswith('original') or name.split('/')[0] in move.SKIP or Path(name).name == 'AGENTS.md':
            continue
        tracked[name] = header.split()[2]
    names = move.aliases([ROOT / name for name in tracked])
    tokens = [name for name in names if not (name.startswith('recovery-') and name.endswith('-evidence.json'))]
    pattern = re.compile('|'.join(re.escape(name) for name in sorted(tokens, key=len, reverse=True)))
    transform = lambda text: pattern.sub(lambda match: names[match.group()], move.PROOF_NAME.sub(r'\1.json', text))  # 최초 계획과 동일한 이름 치환을 적용한다.
    entries, templates, hashes, cache = {}, {}, {}, {}
    reader = subprocess.Popen(['git', 'cat-file', '--batch'], cwd=ROOT, stdin=subprocess.PIPE, stdout=subprocess.PIPE)

    def original(blob):
        """같은 Git blob을 중복해서 읽지 않고 기준 전체 원문을 얻는다."""
        if blob not in cache:
            reader.stdin.write((blob + '\n').encode('ascii'));reader.stdin.flush()
            header = reader.stdout.readline().decode('ascii').split()
            cache[blob] = reader.stdout.read(int(header[2]));assert reader.stdout.read(1) == b'\n'
        return cache[blob]

    # 이미 이동한 파일과 미처리 파일을 모두 동일한 기준 이름으로 기록한다.
    for index, (previous, blob) in enumerate(tracked.items()):
        target_name = transform(previous)
        target, before_path = ROOT / target_name, ROOT / previous
        current_path = target if target.is_file() else before_path
        assert current_path.is_file(), previous
        current = current_path.read_bytes()
        text_kind = current_path.suffix in move.TEXT or current_path.name in {'.gitattributes', '.gitignore', 'CMakeLists.txt'}
        binary = not text_kind or (current_path.suffix == '.tsv' and len(current) > 1024 * 1024)
        baseline = current if binary else original(blob)
        source = current if previous in KEEP else baseline
        before, present = move.digest(baseline), move.digest(current)
        entries[previous] = dict(target=target_name, before=before, current_before=present, size=len(baseline), git_blob=blob)
        if text_kind and not binary:
            try:
                text, encode = move.decoder(source)
                templates[previous] = (transform(text), encode)
                after = move.digest(encode(transform(text)))
                hashes[before] = after;hashes[present] = after
            except UnicodeError:
                pass
        if index % 2000 == 0:
            print('기준/현재 재구성', index, '파일', flush=True)
    reader.stdin.close();reader.stdout.close();reader.wait()
    physical = {entry['target'] for entry in entries.values()} | set(entries)
    # Git에 없는 로컬 보존 바이트와 새 정책파일도 현재 위치에서 누락 없이 유지한다.
    for path in move.inventory():
        name = path.relative_to(ROOT).as_posix()
        if name in physical:
            continue
        body = path.read_bytes();target_name = transform(name);before = move.digest(body)
        entries[name] = dict(target=target_name, before=before, current_before=before, size=len(body), local=True)
        if path.suffix in move.TEXT or path.name in {'.gitattributes', '.gitignore'}:
            if len(body) > 1024 * 1024 and path.suffix != '.json' and not any(token in body for token in (b'recovery-', b'campaign-runtime', b'analyze_', b'before-')):
                continue
            try:
                text, encode = move.decoder(body);templates[name] = (transform(text), encode);hashes[before] = move.digest(encode(transform(text)))
            except UnicodeError:
                pass
    # 기준SHA와 중단시점SHA를 모두 최종 전체 원문의SHA로 수렴시킨다.
    references = {name: set(value.lower() for value in move.HASH.findall(text)) for name, (text, _) in templates.items()}
    for iteration in range(256):
        following, output = dict(hashes), {}
        # 각 파일의 두 기준 SHA가 동일한 최종 원문을 가리키도록 계산한다.
        for previous, (text, encode) in templates.items():
            updated = move.HASH.sub(lambda match: hashes.get(match.group().lower(), match.group()), text) if references[previous].intersection(hashes) else text
            output[previous] = encode(updated);after = move.digest(output[previous])
            following[entries[previous]['before']] = after;following[entries[previous]['current_before']] = after
        if following == hashes:
            break
        hashes = following
        if iteration % 20 == 0:
            print('재구성SHA', iteration + 1, '단계', flush=True)
    else:
        raise RuntimeError('SHA 참조 수렴 실패·추가 이동 없음')
    targets = {entry['target'].casefold() for entry in entries.values()}
    assert len(targets) == len(entries)
    assert max(len(str(ROOT / entry['target'])) for entry in entries.values() if not entry.get('local')) <= 200
    # 쓰기 전에 최종 원문을 짧은 객체파일과 대응표에 기록해 재중단에도 계산결과를 보존한다.
    cache_dir = ROOT / 'extracted/path-cache'  # 소스/이력 안에 이력을 다시 복사하지 않는 임시 복구 저장소다.
    cache_dir.mkdir(parents=True, exist_ok=True)
    for previous, entry in entries.items():
        body = output.get(previous)
        target = ROOT / entry['target']
        entry['after'] = move.digest(body) if body is not None else move.digest(target.read_bytes())
        if body is not None and (not target.exists() or move.digest(target.read_bytes()) != entry['after']):
            saved = cache_dir / (entry['after'][:24] + '.txt')
            if saved.exists():
                assert saved.read_bytes() == body
            else:
                saved.write_bytes(body)
            entry['cache'] = saved.relative_to(ROOT).as_posix()
    tracked_targets = [entry['target'] for entry in entries.values() if not entry.get('local')]
    maximum, longest = max((len(str(ROOT / name)), name) for name in tracked_targets)
    summary = dict(tracked_files=len(tracked_targets), moved_files=sum(name != entry['target'] for name, entry in entries.items()), changed_files=sum(entry['before'] != entry['after'] for entry in entries.values()), max_tracked_path=maximum, longest_path=longest, sha_passes=iteration + 1)
    record = dict(schema=2, baseline_commit=BASE, host='HJOW-Athlon', date='2026-10-14', resumed_after_guard=True, pending=True, aliases=names, summary=summary, files=entries)
    move.MAP.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    # 현재 원문과 같은 파일은 다시 쓰지 않고 실제 필요한 교정만 수행한다.
    for previous, entry in entries.items():
        target, source = ROOT / entry['target'], ROOT / previous
        assert target.resolve().is_relative_to(ROOT) and source.resolve().is_relative_to(ROOT)
        if not target.exists():
            target.parent.mkdir(parents=True, exist_ok=True);source.rename(target)
        if previous in output and move.digest(target.read_bytes()) != entry['after']:
            target.write_bytes(output[previous])
        assert entry['after'] == move.digest(target.read_bytes())
    tracked_targets = [entry['target'] for entry in entries.values() if not entry.get('local')]
    maximum, longest = max((len(str(ROOT / name)), name) for name in tracked_targets)
    summary = dict(tracked_files=len(tracked_targets), moved_files=sum(name != entry['target'] for name, entry in entries.items()), changed_files=sum(entry['before'] != entry['after'] for entry in entries.values()), max_tracked_path=maximum, longest_path=longest, sha_passes=iteration + 1)
    record['pending'] = False
    move.MAP.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(summary, ensure_ascii=False))


# 중단된 한 번의 이관 마감에만 사용하며 기존 대응표가 있으면 재실행을 거부한다.
if __name__ == '__main__':
    recover()
