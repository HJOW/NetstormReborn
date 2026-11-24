# Burning 실제 Session 대조 — WB04-E1315

2026-11-24, HJOW-Athlon. 5-A E201의 피해 연소 생성 호출을 C++ `DamageableBurningSessionTests.cpp`의 `DamageableBurningChecking_ActualClientSessionRecoveryAndLifetime`와 대조했다. 제품 로직은 바꾸지 않았다.

`campaign-burning-session.test.mjs`는 실제 Client 전역·TheWarBegins Session·HP·contained 검색·Burning 표시 시스템을 사용한다. 신전 15002(TYPE 124)의 HP를 1/4로 놓고 두 연소 Form을 붙인다. checking의 최상위 비트도 비영 진단을 켜야 하며, 진단에서 HP 회복·checking 해제·두 번째 Form 삭제가 일어나도 이번 호출은 진단 전에 읽은 HP와 첫 Form을 보존해야 한다. 다음 갱신은 회복 HP로 첫 Form과 표시 토큰을 제거하고 삭제 깊이를 모두 0으로 되돌린다. 이어서 기본 피해 갱신의 실제 생성자(mode 1·stage 0)와 회복 시 반납까지 확인한다. 게임/전투 Kernel·실제 공격은 실행하지 않았다.

E1314의 Session 준비 장치를 `campaign-visual-session-support.mjs`로 분리해 같은 장치를 공유했다. Node 26.13.0에서 Burning 1건과 기존 EvilRay 3건, 합계 4/4 통과(`webpj/extracted/webpj-e1315-focused.log`). 자식 프로세스 로드 훅으로 중복 checking 분기를 끄는 변형은 진단 개수 단언에서 검출했다(`webpj/extracted/webpj-e1315-mutation.log`). 제품 소스의 디스크 바이트는 변경하지 않았다.

E201은 완료다. 5-A의 마지막 재대조는 섬 투하 지지·입자다.
