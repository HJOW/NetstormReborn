# 989 — 일반 전투섬 생성과 원격 Fort 실제 배치

2026-11-07, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의 2026-11-03 자료다. 사용자 /goal의 기존 테스트 제외를 유지하며 멀티플레이는 코드·원문 대조와 제품 컴파일만 진행한다.

## 원문 부모와 연결

`0046d9b0→0046da60→0046e2c0`(CD 대응00490430/00490510/00490fa0)의 실제 지역 builder 수명을 `RawBattleIsland`에 복원했다. 최초 bounds 네 값은-1이며 SetWorld의 실제 청크 지도/owner를 사용한다. owner0 진단 뒤 지역120BYTE를0으로 만들고 현재 Template의 기존 Territory getter로 읽는다.

20개6BYTE 레코드 가운데 flags BYTE+1의0x04가 켜진 첫 영역을 찾고, 하나도 없으면 원문처럼0번을 선택한다. 기존 PlayerBounds의 `00432050`으로 현재 배치 사각형을 얻고, 같은 `RawIslandBuilder::Place(record,territory,true)`로 실제 섬 목록/청크에 배치한다. 현재 Player+78에 반환 섬 번호를 쓴 뒤 같은 정상/군도 지형 root와 섬별 후처리를 실행한다.

그 뒤 현재 boss를 다시 읽는다. 권한이 있을 때만 기존 원시 객체 적재 부모 `004be610`으로 선택한 `[territory,territory+1)`을 읽고 생성하며, 비서버에서는 이 객체 적재를 생략한다. 마지막은 같은 전체 Graph의 `Rebuild(false)`다. Session의 기존 `ReadFortObjectStreams`에 범위를 전달하도록 확장했고, 기본값0..20인 기존 미션/Editor 호출은 유지했다. 객체를 편의 파싱 사본으로 먼저 만들거나 사용자 Fort 파일을 대체 읽지 않는다.

서버 Login의 `makeBattleIsland`와 client packet2a의 이미지 읽기 뒤 `buildFort`를 같은 실제 Session 부모에 연결했다. watcher 분기는 기존 Login 부모에서 선택한다. 이 부모는 Ready/게임 시작·Session.loaded·전체 State 전환을 합성하지 않는다.

## 검증과 잔여

Release 첫 빌드는 종료0/경고1(C4458)이었다. `start` 매개변수가 기존 멤버를 가려 `firstTerritory`로 고쳤고 최종 빌드는 종료0·경고/오류0이다. EXE는11,579,392바이트, SHA-256 `44f7759ca405d94b4640fb3f1287f82e0f8f2b2ae71a0d5fea31a43df2fe2403`다. AGENTS.md·원본/CD·fixture2800파일은 모두 이전의 새 기준 SHA와 일치했다. `cpppj/battle-island-20261107.json`이 기계 판독 기록이다. 신규 MP 단위/게임/통신 검사는 사용자 지시대로 실행하지 않았다.

다음은 중앙섬 metadata와 생성, ordinary 자기 Fort 부모, 일반 State6/7·Commander/roster/LAN UI, 전체 파생 writer/decoder, 메타/저장/재접속/Outpost/checking8이다. 이 단위의 배치 구현·컴파일을 실제 원격 게임 검증 완료로 세지 않는다.
