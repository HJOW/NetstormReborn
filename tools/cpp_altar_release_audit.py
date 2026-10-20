#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""제단 해방/경고의 실제 연결과 빌드·이력 보존을 읽기 전용으로 확인한다."""
import argparse
import json
from cpp_dais_load_audit import ROOT, sha, verify_pins

REPORT = ROOT / 'cpppj/altar-release-session.json'  # 실제 실행을 이월한 구현과 원본 근거의 기록이다.


def main():
    """원본·현재 소스·최종 빌드를 검사하며 게임/단위 검사 실행은 시작하지 않는다."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if not args.verify:
        parser.error('--verify가 필요합니다')
    record = json.loads(REPORT.read_text('utf-8'))
    # 현재 제품과 기존 원본 기대값은 분리된 해시로 검사한다.
    for group in ('sources','evidence'):
        for name, expected in record[group].items():
            assert sha(ROOT / name) == expected, name
    source = (ROOT / 'cpppj/src/client/RawCampaignRitual.inc').read_text('utf-8')
    assert 'altar.setOwner=[this](Sid sid,std::uint32_t owner) { ownership->SetOwner(sid,owner); }' in source
    assert 'life.chat=[this](std::string_view key)' in source and 'services.priest.shield.tell(key)' in source
    assert 'FillMissing(life.chat)' not in source
    assert record['build']['returnCode'] == 0 and record['build']['warnings'] == record['build']['errors'] == 0
    assert sha(ROOT / record['build']['log']) == record['build']['logSha256']
    assert record['runtimeTestsDeferred'] and not record['gameExecuted'] and not record['testsExecuted'] and not record['goalComplete']
    verify_pins(record)
    print(f"제단 해방: 가상 소유자/실제 Tell·Release 빌드·{len(record['sourcePinChanges'])}개 핀 보존 확인, 실행 검증 이월")


if __name__ == '__main__':
    main()
