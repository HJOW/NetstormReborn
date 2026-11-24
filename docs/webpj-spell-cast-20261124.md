# 단독 미션 주문 생성과 실제 Heal 시전 — WB04-E1318

2026-11-24, HJOW-Athlon. 사용자 제보의 주문 사용 오류를 Session 수신 생성 훅에서 재현했다. `raw-campaign-session-carrier-drop.js`는 여섯 주문만 허용하고 나머지에 ‘다른 주문 연결은 멀티플레이 단계로 이월’을 던졌다. 이는 C++ `RawCampaignCarrierDrop.inc`에서 옮긴 구현 범위 제한이며, 단독 미션에서도 사용하는 주문을 잘못 막았다.

현재 호스트는 공통 인계의 마지막 디컴파일 호스트와 같다. 원본 10.78 SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다. 기존 정밀 산출물 `extracted/cpp-m6-delayed-prep/l1-bridge/native/00444bc0.c`와 `.asm`을 읽었다. `00444cd3~00444ce4`는 TYPE flags2의 `0x100`만 검사하고 `00444dd9~00444ddc`는 원래 TYPE를 일반 공장 `004af530`으로 보낸다. 소유자·시전자·Pop·활성화가 뒤따르며 여섯 종류 whitelist는 없다. 과거 범위 제한의 출처는 `docs/exe/cpp-carrier-drop-session-20261020.md`다. 원본 및 정밀 산출물을 변경하거나 원본 게임을 실행하지 않았다.

이 제한만 제거했다. 기존 TYPE 범위 검사와 각 실제 World 생성자·활성화의 진단은 유지한다. `raw-campaign-session-carrier-drop.test.mjs`는 실제 단독 World에 주문 genus 19종 전부를 생성한다. 교정 전 첫 TYPE113에서 예외가 나고, 교정 뒤 생성한 TYPE와 Pop 전 void 상태가 같다. 공중 유닛 소환이나 19종 효과 전체의 동적 검증을 뜻하지 않는다.

`e2e/mission-spell-cast.spec.js`는 영어 Tutorial3의 안내 창을 실제 클릭으로 닫고, 실제 Session 생성·Pop·내용물 부착으로 Heal을 가진 골렘과 피해를 입은 인접 골렘을 준비한다. 실제 객체 메뉴 Cast 행을 클릭하면 로컬 입력/요청/수신을 거쳐 주문이 한 번 생성되고 SP가 한 번 차감된다. 고정 시각의 실제 Regular를 진행하면 인접 골렘 HP가 원문 공식 `(현재HP+최대HP)/2`가 되고 주문이 반납된다. 오류·입력 큐 잔여가 없다. 시전자·주문·피해는 명시 검사 입력이며 자연 획득이나 미션 완주 검사는 아니다.

집중 Node 26 검사 16/16 통과(기존 운반 투하 fixture 413행 포함). Chromium 강화 검사 1/1 통과(25초). 처음 e2e에서 안내 단추/언어 준비를 교정했고, Heal이 시전자 자신을 제외하는 원문 규칙에 맞춰 인접 환자로 HP 단언을 보강했다. webpack 번들 3,423,110바이트로 한도 통과. 기록은 `webpj/extracted/webpj-e1318-{before,focused,browser,build}.log`다. 전체 회귀는 E1316 뒤 첫 완료 단위이므로 이번에는 집중 검사만 수행했다.

같은 C++ whitelist의 제거와 주문 보유 골렘 포획의 NB18 mask 교정은 사용자의 후속 지시에 따라 별도 단위로 진행한다. 사제 포획 완료로 이 단위를 세지 않는다.
