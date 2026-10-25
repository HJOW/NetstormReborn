#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""단일 커서 미리보기 DWORD의 조기 반환·준비 prefix·캡처 시점을 두 PE에서 먼저 관측한다."""
import argparse
import itertools
import json
import socket
from pathlib import Path
from decomp_cursordrop_oracle import DropOracle, ROOT, KEYS, drop_case, digest

FIXTURE = ROOT / 'cpppj/tests/fixtures/cursor-preview-width-x86.tsv'  # 제품 전에 고정할 실제 관측이다.
REPORT = ROOT / 'cpppj/cursor-preview-width-native-20261026.json'  # 실행 범위와 명시 준비 장치를 구별한다.
WORDS = (0, 1, 4, 5, 50, 65535, 0x10032, 0x80000032, 0x7fffffff, 0xffffffff)  # 하위WORD가 같은 다른 DWORD와 경계를 입력으로 준다.


class PrepareStop(Exception):
    """명시 준비 경계의 중단을 Native 정상 반환과 구별한다."""


class WidthOracle(DropOracle):
    """기존 전체 Drop 원문 실행기에 DWORD 입력과 준비 장치의 현재값 변경만 추가한다."""
    def put(self, key, value):
        """최초 입력 구성 중에만 기존 고정50 대신 명시한 전체 DWORD를 설치한다."""
        if key == 'preview' and getattr(self, 'seeding', False):
            value = self.initial_preview
        super().put(key, value)

    def invoke(self, entry, arguments, control, value=False):
        """CPU 실행 전에 입력 구성을 마치고 지정된 준비 중단만 별도 관측으로 반환한다."""
        self.seeding = False
        try:
            return super().invoke(entry, arguments, control, value)
        except PrepareStop:
            self.completed = False
            return 0

    def stub(self, letter, key):
        """원래 준비 사건을 기록한 뒤 명시한 현재 preview DWORD 변경·중단을 수행한다."""
        super().stub(letter, key)
        if key == 'prepare':
            if self.prepared == self.mutation_prepare:
                self.put('preview', self.mutation_value)
            if self.prepared == self.stop_prepare:
                raise PrepareStop()

    def sample(self, initial, case, stop, phase, mutation, control):
        """원시 입력만 구성하고 실제 전체 Drop의 사건·현재 필드·raw 결과를 읽는다."""
        self.initial_preview, self.stop_prepare, self.mutation_prepare, self.mutation_value = initial, stop, phase, mutation
        self.seeding, self.completed = True, True
        events, final = self.drop(case, control)
        return [str(initial), str(stop), str(phase), str(mutation), ','.join(str(case[key]) for key in KEYS), str(int(self.completed)), events, final]


def main():
    """기존 결과를 덮어쓰지 않고 실제 CPU 기대 생성 또는 최초 SHA 검증을 수행한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        result = json.loads(REPORT.read_text(encoding='utf-8'))
        for name, wanted in result['sha256'].items():  # 실제 원문·관측 도구·fixture를 직접 대조한다.
            if digest(ROOT / name) != wanted:
                raise RuntimeError('커서 DWORD 최초 SHA 불일치: ' + name)
        print(json.dumps({'verified': True, 'observations': result['observations']}))
        return
    if FIXTURE.exists() or REPORT.exists():
        raise RuntimeError('최초 커서 DWORD fixture 덮어쓰기 금지')
    cases = []
    for initial in WORDS:  # 잘못된 번호도 선택하지 않은 Pool 접근을 앞당기지 않는 입력이다.
        for lock, legal, query in ((1, 1, 0), (0xffffffff, 1, 0), (0, 0, 0), (0, 1, 1)):
            cases.append((initial, drop_case(lock=lock, legal=legal, queryChange=query, cursorGump=0), 0, 0, 0))
        cases.append((initial, drop_case(cursorGump=0), 1, 0, 0))
    for changed, moving in itertools.product(WORDS, (0, 0x80)):  # 유효50의 캡처 뒤 준비가 현재 전역만 변경한다.
        cases.append((50, drop_case(cursorGump=0, inputFlags=moving), 0, 1, changed))
    rows = ['# edition / control / initialPreview / stopPrepare / mutationPrepare / mutationValue / original30Inputs / completed / events / final14']
    normal = stopped = 0
    paths = {'originals/Netstorm.exe', 'originalCD/NETSTORM.EXE', 'tools/decomp_cursor_preview_width.py', 'tools/decomp_cursordrop_oracle.py', 'tools/decomp_constructionplace_oracle.py', 'tools/decomp_owner_oracle.py', 'tools/decomp_priestgeometry_oracle.py', 'tools/decomp_damageablepredestroy_oracle.py', 'extracted/refined/originals/Netstorm.c', 'extracted/refined/originalCD/NETSTORM.c', 'cpppj/tests/fixtures/cursor-preview-width-x86.tsv'}
    for edition in ('originals', 'originalCD'):  # 두 판본은 서로 다른 원문·전역·stride를 사용한다.
        oracle = WidthOracle(edition)
        paths.update(path.relative_to(ROOT).as_posix() for path in oracle.exports)
        for initial, case, stop, phase, mutation in cases:
            observations = []
            for control in (0x27f, 0x37f):  # 두 x87 상태의 독립 결과를 비교한다.
                row = oracle.sample(initial, case, stop, phase, mutation, control)
                observations.append(row)
                rows.append('\t'.join([edition, str(control), *row]))
                normal += int(oracle.completed)
                stopped += int(not oracle.completed)
            if observations[0] != observations[1]:
                raise RuntimeError('커서 DWORD x87 정밀도 차이')
    FIXTURE.write_text('\n'.join(rows) + '\n', encoding='utf-8', newline='\n')
    result = {'host': socket.gethostname(), 'lastFullDecompileHost': 'HJOW-Athlon', 'lastFullDecompileDate': '2026-10-23', 'observations': len(rows) - 1, 'uniqueInputsPerEdition': len(cases), 'normalReturns': normal, 'explicitPrepareStops': stopped, 'precisionDifferences': 0, 'originalGameExecuted': False, 'osExecuted': False, 'scope': '실제 전체 Drop·반올림·잠금은 원문 CPU이고 준비/가상/로컬배치는 명시 장치다. 큰 DWORD의 잠금/불법 조기 반환·첫 준비 중단과 유효50 캡처 뒤 큰 현재 전역 변경을 관측했다. 잘못된 큰 번호의 실제 Pool 역참조나 임의32비트 포인터 alias 동작은 검증 범위가 아니다.', 'sha256': {name: digest(ROOT / name) for name in sorted(paths)}}
    REPORT.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: result[key] for key in ('observations', 'normalReturns', 'explicitPrepareStops')}))


if __name__ == '__main__':
    main()
