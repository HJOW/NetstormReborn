# 파생 객체 메뉴 연결: 신전·배터리·발전기 우클릭 (WB04-E1127)

신전(vortex)·배터리·발전기 같은 객체를 우클릭해도 아무 메뉴가 열리지 않았다. 앱의 공통 객체 메뉴 열기(`ClientObjectMenuOpen`)가 가상 표의 작성(+a8) 주소가 Damageable(`0044cbd0`)·일반(`004b1270`)인 객체만 지원했고, 원본이 신전·배터리 같은 객체에 쓰는 파생 작성 본체는 이식만 해 두고(`core/raw-object-menu-variants.js`, E542) 앱에 연결하지 않았기 때문이다. 원본 스크린샷 `The War Begins! - Rain Temple Context Menu.png` 의 메뉴(`Owner/Alignment/Class`, `View Netstorm Knowledge`, `Salvage gains 1250`, `About`, `Player >`)가 이제 같게 열린다.

## 구현

- **세션** `RawCampaignSession.makeObjectMenuVariants(input)`: 원본 `RawCampaignSession` 의 파생 체인과 같은 게임 값 장치를 현재 Session 에서 읽는다 — 전역(`0594fa4` 미션·`05c85a4` 편집·`055a480` 특수·`0540c70` 로컬 owner 와 원본 초기 명령 전역), 최대 HP, 예약 이벤트 0 존재, owner 의 AI 두뇌 유무(+0xb0), 가상 개수. CHEAT Dig Up 항목은 편집 밖에서 숨긴다. AI 디버그 객체 메뉴는 checking 진단이 꺼져 있으면 효과 없이 돌아가고 켜져 있으면 미연결 오류다(개발자 기능).
- **UI** `ClientObjectMenu.openVariant(sid, populate)`: 항목은 포함·활성 인자를 그대로 메뉴 작성기에 쓰고, 머리말·구분선·공통 메뉴 작성(일반·Damageable·회수)은 같은 작성기와 기존 메뉴를 쓴다.
- **열기** `ClientObjectMenuOpen`: 공통 열기(+ac `004adb30`)이고 작성 주소가 일곱 파생(`0041c210` Meltdown! 배터리·`00422000` 상태 변경·`00462390` Dig Up/Turn To Spell·`00462550` 도움말·`00485600` Storm Power 머리말·`0048eca0` 회수 개수·`004dcca0` 신전 AI·지식 항목)이면 파생 작성을 쓴다. 열기가 다른 `0041e660`(Archer·Detonate 계열)·`004268c0`(Carrier)·`004754b0`(Factory)는 기존 경로 또는 후속이다.

## 시험

- `tests/client-object-menu-open.test.mjs`: 일곱 파생 주소가 지원되고 `fancy → summary → variant → attach → sound` 순서로 열리며 Archer·Detonate 계열은 계속 미지원이다.
- 새 `e2e/object-menu-variants.spec.js`(Chromium, 캠페인 1-1): 내 신전(`rainVortex`)·적 배터리(`windBattery`)·적 신전(`windVortex`)의 객체 메뉴가 열려 팝업이 생기고(결과 1) 오류가 없음을 확인한다. 연결 전에는 신전·배터리가 결과 0 이었다. 신전 메뉴 화면을 원본 스크린샷과 눈으로 대조했다(항목 구성·순서가 같음).
- 객체 메뉴가 얽힌 기존 e2e(영어 Tutorial2 발사기 회수·생산 정보·지식 창) 6건이 계속 통과한다.

## 남은 경계

- 열기(+ac)가 `0041e660`(Archer 계열 16개 VT)인 객체와 Carrier(`004268c0`)·Factory(`004754b0`) 파생의 일부, 특수 대상 메뉴, 채팅 분기는 이 단위 범위 밖이다(`LEFT_JOBS` 6단계 '객체 우클릭 메뉴').
- 메뉴 항목의 명령 실행(Meltdown!·Turn To Spell! 등)이 입력 루프에 연결돼 있는지는 항목별로 확인해야 한다. 이번에는 열림과 항목 구성만 확인했다. 연결되지 않은 명령은 입력 큐에서 먹통을 만들 수 있다([Players 메뉴 동작](webpj-player-menu-actions-20261120.md) 참조).
