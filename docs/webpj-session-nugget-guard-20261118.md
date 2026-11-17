# 실제 Session 비미션 너겟 관문 (WB04-E702)

기준선 `225553df`의 `RawCampaignSession.cpp`와 `RawBattleNuggetPlacement.cpp`를 대조하고 실제 Tutorial 1 자료를 적재한 typed Session에서 비미션 보충 너겟 callback을 호출했다. 두 C++ 파일에는 기준선 이후 변경이 없다.

단일 미션에서는 원본의 `!inSingle` 진단이 `Addrandom.cpp`의 `0xb4` 지점에서 발생하며, 그 전에 월드 객체와 대기 입력이 바뀌지 않고 Session은 적재 상태로 남는다. 실제 진단 연결에 중단 예외를 주입해 확인했다. RNG와 너겟 생성의 정상 비미션 경로는 기존 부품 검사로만 확인했으며, 실제 비미션 Session 검증은 아직 구현하지 않은 12차 스커미시에서 진행한다. Fort 모드 snapshot과 앱의 미션 수명도 미완료다.

집중 검사 20건이 통과했다. 최초 시험은 실제 후처리에 필요한 `wallSeconds` 입력이 없어 관문 전에 실패했고, 실제 의존성을 제공한 뒤 통과했다. 게임 창이나 실제 플레이는 실행하지 않았으며 별도의 공중 3종 생성·행동 검사도 추가하지 않았다.

Node v26.13.0 전체 회귀 6,134건 통과(실패·취소·건너뜀 0). E698의 Node 22 통과 이후 전체 회귀는 4/8회다. 로그는 Git 제외 `extracted/webpj-e702-node26.log`다.
