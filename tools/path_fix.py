#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""중단된 경로 이관을 기준 Git 원문과 실제 현재 파일에서 재구성하여 마감한다."""
import json
import argparse
import platform
import re
import subprocess
from pathlib import Path
import path_move as move

ROOT = move.ROOT  # 기준커밋과 현재 파일은 같은 작업공간을 사용한다.
BASE = '9d7acad'  # 경로변경 전 단위15의 완료 커밋이다.
KEEP = {'.gitattributes', '.github/workflows/ci.yml', 'LEFT_JOBS.md'}  # 새 정책·사용자 인수인계 편집은 현재 원문으로 유지한다.


def recover(apply=False):
    """Git 원문·현재 원문의 두 SHA를 같은 최종 경로로 연결하여 남은 참조를 닫는다."""
    assert not move.MAP.exists(), '이미 작성한 대응표는 --resume으로 이어서 적용합니다.'
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
            body = reader.stdout.read(int(header[2]));assert reader.stdout.read(1) == b'\n'
            # 큰 음악·fixture는 중복 캐시에 유지하지 않아 전체 재구성의 메모리를 제한한다.
            if len(body) <= 1024 * 1024:
                cache[blob] = body
            return body
        return cache[blob]

    # 이미 이동한 파일과 미처리 파일을 모두 동일한 기준 이름으로 기록한다.
    for index, (previous, blob) in enumerate(tracked.items()):
        target_name = transform(previous)
        target, before_path = ROOT / target_name, ROOT / previous
        current_path = target if target.is_file() else before_path
        assert current_path.is_file(), previous
        current = current_path.read_bytes()
        text_kind = current_path.suffix in move.TEXT or current_path.name in {'.gitattributes', '.gitignore', '.editorconfig', 'LICENSE', 'CMakeLists.txt'}
        binary = not text_kind or (current_path.suffix == '.tsv' and len(current) > 1024 * 1024)
        baseline = original(blob)
        # 이름만 바뀌는 바이너리·큰 fixture는 실제 Git 원문과 동일해야 적용할 수 있다.
        if binary:
            eol_only = current_path.suffix == '.tsv' and current.replace(b'\r\n', b'\n') == baseline.replace(b'\r\n', b'\n')
            assert current == baseline or eol_only, f'원본 입력 바이트 변경: {previous}'
        source = current if previous in KEEP else baseline
        # Git의 LF blob과 Windows의 CRLF 원문을 구분하며 현재 체크아웃 줄바꿈을 유지한다.
        if text_kind and not binary and current.count(b'\n') and current.count(b'\r\n') == current.count(b'\n'):
            source = source.replace(b'\r\n', b'\n').replace(b'\n', b'\r\n')
        before, present = move.digest(baseline), move.digest(current)
        entries[previous] = dict(target=target_name, before=before, current_before=present, size=len(baseline), git_blob=blob)
        if binary:
            hashes[before] = present
        if text_kind and not binary:
            try:
                text, encode = move.decoder(source)
                templates[previous] = (transform(text), encode)
                after = move.digest(encode(transform(text)))
                hashes[before] = after
                # 과거 보고서가 기록한 LF/CRLF 전체 원문 SHA도 같은 최종 바이트로 연결한다.
                canonical = baseline.replace(b'\r\n', b'\n')
                hashes[move.digest(canonical)] = after
                hashes[move.digest(canonical.replace(b'\n', b'\r\n'))] = after
                entries[previous]['eol_hashes'] = [move.digest(canonical), move.digest(canonical.replace(b'\n', b'\r\n'))]
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
    output = {name: encode(text) for name, (text, encode) in templates.items()}  # SHA 참조가 없는 큰 소스·fixture는 한 번만 인코딩한다.
    active = {name for name in templates if references[name].intersection(hashes)}  # 실제 장부의 의존 관계가 있는 텍스트만 반복 계산한다.
    seen = set()  # 같은 SHA 상태가 반복되면 추가 파일 쓰기 전에 순환을 보고한다.
    print('SHA 의존 파일', len(active), '전체 텍스트', len(templates), flush=True)
    for iteration in range(256):
        following = dict(hashes)
        # 각 파일의 두 기준 SHA가 동일한 최종 원문을 가리키도록 계산한다.
        for previous in active:
            text, encode = templates[previous]
            updated = move.HASH.sub(lambda match: hashes.get(match.group().lower(), match.group()), text) if references[previous].intersection(hashes) else text
            output[previous] = encode(updated);after = move.digest(output[previous])
            following[entries[previous]['before']] = after
            # 파일의 원래 줄바꿈별 SHA 역시 의존 보고서와 함께 수렴시킨다.
            for original_hash in entries[previous].get('eol_hashes', ()):
                following[original_hash] = after
        if following == hashes:
            break
        state = move.digest(json.dumps(sorted(following.items())).encode('ascii'))
        assert state not in seen, 'SHA 참조의 상태가 반복되어 적용을 중단합니다.'
        seen.add(state)
        hashes = following
        if iteration % 10 == 0:
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
        existing = target if target.is_file() else ROOT / previous
        entry['after'] = move.digest(body) if body is not None else move.digest(existing.read_bytes())
        if apply and body is not None and move.digest(existing.read_bytes()) != entry['after']:
            saved = cache_dir / (entry['after'][:24] + '.txt')
            if saved.exists():
                assert saved.read_bytes() == body
            else:
                saved.write_bytes(body)
            entry['cache'] = saved.relative_to(ROOT).as_posix()
    tracked_targets = [entry['target'] for entry in entries.values() if not entry.get('local')]
    maximum, longest = max((len(str(ROOT / name)), name) for name in tracked_targets)
    summary = dict(tracked_files=len(tracked_targets), moved_files=sum(name != entry['target'] for name, entry in entries.items()), changed_files=sum(entry['before'] != entry['after'] for entry in entries.values()), max_tracked_path=maximum, longest_path=longest, sha_passes=iteration + 1)
    record = dict(schema=2, baseline_commit=BASE, host=platform.node(), date='2026-10-14', resumed_after_guard=True, pending=True, aliases=names, summary=summary, files=entries)
    if not apply:
        print(json.dumps(summary, ensure_ascii=False))
        return
    move.MAP.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    finish(record)


def finish(record):
    """저장한 대응표와 짧은 캐시로 미적용 파일만 재개하고 적용 전 SHA도 확인한다."""
    entries = record['files']  # 중단 전에 저장한 전체 파일의 경로 대응과 SHA를 사용한다.
    # 이관 자체의 출력 로그는 실행 중 계속 쓰이므로 고정된 원문 장부에서 제외한다.
    for previous in list(entries):
        if entries[previous].get('local') and entries[previous]['target'].startswith('extracted/path-'):
            del entries[previous]
    # 현재 원문과 같은 파일은 다시 쓰지 않고 실제 필요한 교정만 수행한다.
    for previous, entry in entries.items():
        target, source = ROOT / entry['target'], ROOT / previous
        assert target.resolve().is_relative_to(ROOT) and source.resolve().is_relative_to(ROOT)
        existing = target if target.is_file() else source
        assert move.digest(existing.read_bytes()) in {entry['current_before'], entry['after']}, f'계산 뒤 내용 변경: {existing}'
        if not target.exists():
            target.parent.mkdir(parents=True, exist_ok=True);source.rename(target)
        if move.digest(target.read_bytes()) != entry['after']:
            body = (ROOT / entry['cache']).read_bytes()
            assert move.digest(body) == entry['after'], previous
            target.write_bytes(body)
        assert entry['after'] == move.digest(target.read_bytes())
    tracked_targets = [entry['target'] for entry in entries.values() if not entry.get('local')]
    maximum, longest = max((len(str(ROOT / name)), name) for name in tracked_targets)
    record['pending'] = False
    move.MAP.write_text(json.dumps(record, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps(record['summary'], ensure_ascii=False))


# 중단된 한 번의 이관 마감에만 사용하며 기존 대응표가 있으면 재실행을 거부한다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--apply', action='store_true')
    mode.add_argument('--resume', action='store_true')
    arguments = parser.parse_args()
    if arguments.resume:
        saved = json.loads(move.MAP.read_text(encoding='utf-8'))
        assert saved['pending'], '완료된 대응표는 다시 적용하지 않습니다.'
        finish(saved)
    else:
        recover(apply=arguments.apply)
