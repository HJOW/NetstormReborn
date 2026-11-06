# 994 — 일반 복귀 State6의 앞뒤 부모

2026-11-07, HJOW-Athlon. 같은 호스트의2026-11-03 정밀 디컴파일에서10.78 `004b6dd0` case6과 대응10.72 `0042d630`을 대조했다. `NativeBattleReturnState`를 추가하고 실제 상태 분배기의6번을 큰 Fort 부모의 반환/공통144BYTE 확정 계약에 포함했다.

앞단은 cleanup 전 결과/출처 지역값을 보관하고 이전 phase의 출처 초기화, 배경 첫BYTE0, 승패별 삭제/배경/음악, 출처 TYPE 배경 조회를 원문 순서로 처리한다. State6 결과0이면 첫BYTE0을 유지하며 titleMenu를 넣지 않는다. 출처 그림이 없으면 보관 지역 source만0으로 바꾸고 전역 출처를 지우지 않는다. 그 뒤 현재 battle/이전phase14에만 실제 Leave 통지를 요청하고 공통 prefix/실제 Fort 연속 부모를 호출한다.

Fort 뒤 HUD1/주변음 기한/ser22의 기존 공통 꼬리에서 현재6이면 실제 battleOver callback에 위임한 뒤 자연 반환하도록 고쳤다. fortPal/다음4/빈섬 예약은 실행하지 않는다. 새 후반 부모는 보관 결과0이면 Multiplayer Options 공장, 결과 비0·보관 출처0이면 signed `BattleOver%d` Tell을 호출하고 정상 복귀 뒤 현재 결과0을 쓴다. 미션 조기 반환은 외곽이 그대로 전달한다.

**실제 Uber State6 선택, 일반 Fort 설치/공개 계약 확장과 Multiplayer Options/LAN 화면은 아직 미연결이다.** 이번 단위는 선택 효과를 명시한 앞뒤 부모·분배기·공통 꼬리를 컴파일한 범위다. 콜백을 빈 성공으로 채우거나 일반 멀티플레이 완성으로 세지 않는다. 다음 통합에서 동일 helper와 기존 Fort 적재 몸체를 사용한다.

Release 제품 빌드는 종료0·경고/오류0이다. 제품11.01은11,646,976바이트, SHA-256은 `cd9ca5f1714874493222834a12d7c526451fec7b4718f990cca3de20fb681031`다. 로그는 `extracted/cpp-resume-994/build.log`, 보고서는 `cpppj/battle-return-parent-20261107.json`이다. 신규 MP/단위/게임/통신 검사0·신규 fixture0이며 보호 AGENTS.md·원본/CD·fixture2800파일은 모두 불변이다. 현재 GUI/ZIP/전체34경로/전체검사는 반복하지 않았다.
