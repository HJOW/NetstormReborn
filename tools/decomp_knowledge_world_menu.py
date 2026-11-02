#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""보호10.78의 World 기술 카드와 Storm/World 선택메뉴를 별도 출력에 격리 관측한다."""
import argparse
import hashlib
import json
from pathlib import Path
import socket
import decomp_knowledge_menu as base
from decomp_cursor_position import ranges


class Oracle(base.Oracle):
    """기존 Storm 장면의 독립 조회/GUI 장치를 빌리고 두 새 원문 몸체를 실행한다."""

    def __init__(self, variant):
        """선택한 실제 함수만 추가 허용하며 원본 함수/fixture는 수정하지 않는다."""
        super().__init__()
        self.variant = variant
        ranges(self, (0x493210, 0x4938c0))

    def invoke(self, address, *args, **kwargs):
        """명시 TYPE 입력을 World4..7/제외8로 공급한 뒤 실제 몸체에 진입한다."""
        if address == 0x492b60:
            # 같은12타입을 World 원소로 공급하며 마지막BadTheme는 별도의8이다.
            for index in range(12):  # 각 원문 TYPE 입력의 theme DWORD만 World 범위로 준비한다.
                self.put(base.TYPES+index*500+0x98, 8 if index == 11 else 4+index % 4)
            address = 0x493210 if self.variant == 'World' else 0x4938c0
        return super().invoke(address, *args, **kwargs)


def main():
    """50개 실제 정상반환/노드·행·정렬·배치·표식과 보호 PE SHA를 함께 보존한다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert not args.fixture.exists() and not args.report.exists()
    lines = ['# variant input10 native-nodes-marker-argument-trace']
    observations, returns = 0, 0  # 실제로 완료한 원문 몸체만 집계한다.
    for variant in ('World', 'Side'):  # 두 실제 부모를 독립 장치/반환 수명으로 관측한다.
        oracle = Oracle(variant)
        for row in base.inputs():  # 기존 독립 입력의 화면/언어/기술/제외 조합을 같은 순서로 공급한다.
            lines.append(variant+'\t'+','.join(map(str, row))+'\t'+oracle.sample(row))
            observations += 1
        returns += oracle.allReturns
    args.fixture.write_text('\n'.join(lines)+'\n', encoding='utf-8', newline='\n')
    record = dict(unit=758, host=socket.gethostname(), lastPreciseDecompile=dict(host='HJOW-Athlon', date='2026-10-23'),
                  inputs=observations, normalReturns=returns, gameExecutions=0, multiplayerTests=0,
                  bodies=['00493210 World theme4..7', '004938c0 Storm/World side selector'],
                  explicitBoundaries=['TYPE/ownership/excluded/translation/format/GUI allocation/attach/virtual push-pop/position/thread'],
                  fixtureSha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
                  toolSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  peSha256=base.digest(base.ROOT/'originals/Netstorm.exe'),
                  limitations=['현재 x87기본정밀도이며 두정밀도/재관측 아님', '실제GUI/전체UI/기본메뉴노출/자연플레이의 근거 아님'])
    args.report.write_text(json.dumps(record, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps(dict(inputs=observations, normalReturns=returns, fixtureSha256=record['fixtureSha256'])))


if __name__ == '__main__':
    main()
