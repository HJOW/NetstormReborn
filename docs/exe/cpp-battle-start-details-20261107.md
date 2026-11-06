# 992 — 일반 전투 시작의 밀도·연결 안내

2026-11-07, HJOW-Athlon. 같은 호스트의2026-11-03 정밀 디컴파일과 보호10.78 PE를 읽기 전용으로 대조했다. 원문 `004b6dd0`(CD `0042d630`), `0041c9b0/0041c9e0`, `004558f0`, `004b6090`(CD `004331c0`)의 현재 값/호출 순서를 복원했다.

`RawMissionBattleOptions`는 현재 옵션 BYTE+7에서 하한10/15/20/25, 상한30/35/40/45를 각각 조회한다. 일반 State7 서버는 같은 장기 하한·상한에 순서대로 저장하고, 두 번 모두 현재 한계의 signed 반값 범위에서 같은 게임 난수를 소비하여 강제 밀도에 DWORD 합계를 쓴다. PE `004b7666..769e`를 확인해 디컴파일에서 첫 호출의 두 번째 인자가 누락된 표현을 교정했다. 별도 난수·상수 밀도·정규화는 쓰지 않는다.

기존 `RawLocalMissionState`의 simulconnect listener와 일반 시작의 밀도→Chat40h→80h→연결 안내, State8의 새 기한 뒤 연결/재접속 안내를 연결했다. 실제 Session은 현재 전송 listener·같은 GameAux/난수·기존 Chat 부품을 빌린다. Client의 진행 콜백은 현재 UberGump를 호출 순간에 조회하고 기존 transient HTML 창/Cancel/serial을 재사용한다. 원문은 주소 formatter의 지역 결과와 별개로 빈 `%s`를 넘기므로 실제 표시 치환도 빈 주소와 countdown 정수만 처리한다. State9 로그인 송신 뒤 안내는 `004b6090`의 missionPost/savedGame에 따라 선택한다. 기존 독립 부분 입력만 progress 부재 시 이전 refresh 경계를 유지한다.

일반 전투의 **호스트 State7 진입/World 교체 부모**, State6/Commander/roster/LAN UI는 아직 남는다. 이번 세부 분기 연결을 일반 멀티플레이 완성으로 세지 않는다. 단독 미션의 기존 호스트 State7 앞단/후반은 그대로 사용한다.

Release 제품 빌드는 종료0·경고/오류0이다. 제품11.01은11,632,640바이트, SHA-256은 `2abd3ae3119d059ba961bc177ef230b26239f9749c12bc39ee157cffae899a7c`다. 로그는 `extracted/cpp-resume-992/build.log`, 보고서는 `cpppj/battle-start-details-20261107.json`이다. 보호 AGENTS.md·원본/CD·fixture2800파일은 불변이고 새 fixture는 없다. 신규 MP/단위/게임/통신 검사0이며 현재 GUI·ZIP·전체34경로/전체검사는 반복하지 않았다.
