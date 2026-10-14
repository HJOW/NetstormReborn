# -*- coding: utf-8 -*-
"""단위26 진행 상태와 다음 실제 세션 경계를 인수인계에 남긴다."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]  # 같은 작업공간이다.
path=ROOT/'LEFT_JOBS.md';text=path.read_text(encoding='utf-8')  # 기존 이력은 보존한다.
assert '## 2026-10-15 진행: 단위26' not in text
section='''## 2026-10-15 진행: 단위26 실제 Player9 송신 분배 (`HJOW-Athlon`)

- [x] AGENTS/양 LEFT 재독과 기준 `7cb5494`의 깨끗한 작업 트리를 확인했다. 현재 및 마지막 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**다. 직접 영향의 이전 전체 원문8개를 `run26-base`에 보존했다. 지난 턴은 세션/화면 코드·전체1135·로컬커밋으로 진척했다.
- [x] 두 판본 송신/지정 제외 송신4몸체를 정밀 export했다. `RawBattleBroadcast`는 같은 Player9의 접속 bit0, NULL 연결, 로컬/지정 제외와 현재 후보 순서를 복원한다. 일반 송신의 로컬 번호 재조회와 제외 송신의 최초 번호 보관을 구별한다. CD는 Player 폭0xac이며 접속 BYTE는 PATCH와 같은+50이다. 첫 미빌드 draft의 잘못된 CD+48은 원문으로 보존하고 정밀 근거로 교정했다.
- [x] 새 Native 고유1356입력/정상반환/assert0과 CPP1356 대조 집중2개, NULL/서버 계약1개가 통과했다. 최초 도구는 몸체/fixture 생성 후 보고서의 Path 이름 오류로 종료했다. 최초 도구/로그/fixture는 보존했으며 수정 전후 입력/기대값은 동일하다. 재실행을 새 고유 입력으로 합산하지 않았다.
- [x] 생산 세션의 Gun/Path 직렬화 결과를 동일 Player/연결 표의 실제 분배기로 기본 연결했다. 빈 송신 관찰 콜백을 요구하지 않는다. 기존 명시 검사 송신 경계는 유지했다. 실제1-1 선택/좌클릭/Host54/이동을 단독 NULL 연결 분배에서도 확인했고 CampaignSession5개가 통과했다.
- [ ] 최종 Release 성공과 집중8개 성공 뒤 전체 회귀 검사를 진행 중이다. 전체 원문/로그/SHA 고정·인수인계 마감·로컬 `20261015-11` 커밋은 완료 결과 후 수행한다.
- [ ] 실제 연결 객체의 가상 송신은 관찰 경계이며 LAN I/O 구현은 이번 범위 밖이다. dependency Form/자산의 전체 직렬화·실제 UI/음향/config/카메라/owner 통지/상위 Ready/login을 이어서 연결해야 한다. UberGump Battle 거부와 Early Missions/1-1~1-2 실제 플레이·완주 목표는 미완료다. 전체화면 오류 제외와 목표active를 유지한다.

'''
marker='## 2026-10-15 완료: 단위25';text=text.replace(marker,section+marker,1)
path.write_text(text,encoding='utf-8',newline='\r\n')
