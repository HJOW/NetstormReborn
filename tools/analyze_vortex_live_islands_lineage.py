#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Ascend의 실제 IslandList 참조 연결과 기준09 전체 기존 본문·원본 입력 보존을 감사한다."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent  # 같은 저장소의 절대 기준 경로다.
REPORT = ROOT / 'cpppj/vortex-live-islands-lineage.json'  # 이전 원본 보고서를 보존하는 별도 연결 근거다.
SOURCES = ('cpppj/src/o/RawVortexAscend.h', 'cpppj/src/o/RawVortexAscend.cpp')  # 참조 overload와 실제목록 분기를 추가한 두 파일이다.
PRIOR = ('cpppj/vortexascend.json', 'cpppj/run9.json')  # 당시 전체 보고서 원문을 연결하며 관찰을 합산하지 않는다.
TEST = 'cpppj/tests/VortexIslandBindingTests.cpp'  # 기존Count40개와 현재목록의 실제 소비자 검사다.


def sha(data):
    """전체 바이트와 주석 제외 토큰의 SHA256을 반환한다."""
    return hashlib.sha256(data).hexdigest()


def git(*args):
    """기준 git 객체를 읽기만 한다."""
    return subprocess.run(['git', *args], cwd=ROOT, capture_output=True, check=True).stdout


def tokens(text):
    """문자열을 보존하고 주석·공백만 제외한 전체 토큰을 만든다."""
    parts = re.findall(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\w+|[^\s]', text)
    return '\0'.join(part for part in parts if not part.startswith(('//', '/*'))).encode('utf-8')


def remove_block(text, first, last):
    """명시한 새 블록만 제거하고 기존 뒤쪽 표식과 전체 본문은 보존한다."""
    assert text.count(first) == 1 and text.count(last) == 1
    start = text.index(first)
    end = text.index(last, start)
    return text[:start] + text[end:]


def restore(name, text):
    """새 전방 선언·overload·참조멤버·Count분기만 역제거한다."""
    if name.endswith('.h'):
        text = remove_block(text, '// 실제3204바이트 IslandList', '// 원본 IslandList의 존재')
        text = remove_block(text, '    // 실제countDWORD/32바이트 기록을 참조한다.', '    // 004db680/CD0044e230:')
        member = '    const RawIslandRuntime* liveIslands_{}; // 실제 월드의 현재count/존재/owner를 읽는다. 기존 fixture 경로에서는null이다.\n'
        assert text.count(member) == 1
        return text.replace(member, '')
    include = '#include "o/RawIslandRuntime.h"\n'
    assert text.count(include) == 1
    text = text.replace(include, '')
    text = remove_block(text, '// 기존 후보/삭제 효과와 같은풀의 실제 IslandList를 공유한다.', '// little endian 필드를 지정한 폭으로 읽는다.')
    return remove_block(text, '    if (liveIslands_) {', '    std::uint32_t count=0;')


def observe(commit):
    """이전 전체 후보/삭제/Count본문과 당시1496개 관찰 원문을 보존했는지 확인한다."""
    sources, prior = {}, {}
    # 새 부분 역제거 뒤 기존 전체 파일을 문자와 토큰 단위로 모두 비교한다.
    for name in SOURCES:
        original = git('show', commit + ':' + name)
        baseline = original.decode('utf-8').replace('\r\n', '\n')
        current = (ROOT / name).read_text(encoding='utf-8')
        restored = restore(name, current)
        assert restored == baseline and tokens(restored) == tokens(baseline), name
        sources[name] = dict(baseline_sha256=sha(original), current_sha256=sha((ROOT / name).read_bytes()),
                             entire_existing_token_sha256=sha(tokens(baseline)), entire_existing_source_unchanged=True)
    # 당시 보고서는 바이트까지 보존한다. 역사적 바이너리SHA는 현재값으로 덮어쓰지 않는다.
    for name in PRIOR:
        path = ROOT / name
        assert path.read_bytes() == git('show', commit + ':' + name), name
        data = json.loads(path.read_text(encoding='utf-8'))
        dependencies = {}
        # 기존 원본 입력의 PE/fixture/도구와 사용 가능한 내보내기만 현재 바이트까지 확인한다.
        for file, expected in data.get('files', {}).items():
            if not (file.startswith(('tools/', 'originals/', 'originalCD/')) or '/fixtures/' in file or file.endswith(('creation.c', 'functions.tsv'))):
                continue
            if not (ROOT / file).exists():
                assert file.startswith('extracted/')
                dependencies[file] = dict(reported_sha256=expected, available=False)
                continue
            actual = sha((ROOT / file).read_bytes())
            assert actual == expected, file
            dependencies[file] = dict(reported_sha256=expected, current_sha256=actual, available=True)
        prior[name] = dict(sha256=sha(path.read_bytes()), total=data.get('total'), dependencies=dependencies)
        if name.endswith('run9.json'):
            prior[name]['dependencies'] = {}  # 09의전체 당시SHA는원문만보존하며 개별수정은전용lineage로연결한다.
    fixture = ROOT / 'cpppj/tests/fixtures/vortexascend-x86.tsv'
    rows = [line.split('\t') for line in fixture.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]
    counts = {edition: sum(row[0] == edition and row[1] == 'Count' for row in rows) for edition in ('originals', 'originalCD')}
    assert len(rows) == 1496 and counts == {'originals': 20, 'originalCD': 20}
    return dict(sources=sources, prior_reports=prior, reused_count_rows=counts,
                test_sha256=sha((ROOT / TEST).read_bytes()), fixture_sha256=sha(fixture.read_bytes()))


def generate():
    """기준09 전체보존과 현재 실제목록 연결을 별도 근거로 저장한다."""
    commit = git('rev-parse', '5b10ce8').decode('ascii').strip()
    data = dict(host='HJOW-Athlon', date='2026-10-14', baseline_commit=commit, lineage=observe(commit),
                tool_sha256=sha(Path(__file__).read_bytes()), additional_unique_native_cases=0,
                reused_existing_count_cases=40, new_cpp_tests=2,
                scope='실제3204바이트/countDWORD/32바이트기록 참조. 반복마다 signed현재count/Exists/Owner를 조회하며 owner가다르면Owner를다시조회한다. 같은pool계약·현재변경보존. 기존Candidate/Tick/spanCount본문 전체불변.',
                limitations=['기존40개 입력 재사용이며 새native관찰이 아니다.', '현재 실제 GameWorld에 Ascend를 설치하거나 캠페인 완주를 확인한 근거가 아니다.'])
    REPORT.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    verify()


def verify():
    """전체역변환·현재SHA·기존원본Count입력 보존을 다시 감사한다."""
    data = json.loads(REPORT.read_text(encoding='utf-8'))
    assert data['lineage'] == observe(data['baseline_commit']) and data['tool_sha256'] == sha(Path(__file__).read_bytes())
    assert data['additional_unique_native_cases'] == 0
    print('Ascend 실제목록: 기준09 전체 기존본문/1496입력 보존·Count40 재사용·새native0')


# 기본 호출은 근거 기록이며 --verify는 저장한 현재 바이트의 감사다.
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    arguments = parser.parse_args()
    verify() if arguments.verify else generate()
