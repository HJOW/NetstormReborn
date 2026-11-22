# webpj: Native 장기 결선(RawSurface*·ClientConstruction*)과 웹 typed 조립의 훅 대조 (WB04-E1206~E1215, 2026-11-22)

cpppj 의 `client/RawSurface*.cpp`·`ClientConstruction*.cpp` 는 "Native 장기 결선"으로 각 원문 모듈에 훅을 조립한다. 웹은 같은 훅을 `webpj/www/core/raw-campaign-session-*.js`(C++ `RawCampaignSession.cpp` 의 typed 경로)에서 조립한다. 행을 닫을 때마다 훅 이름별로 웹의 소유 지점을 찾아 대조했고, 차이는 아래 유형으로 나눴다.

| 유형 | 뜻 | 결과 |
| --- | --- | --- |
| A | 원문 Native 는 `AssertDisplay` 로 진단을 표시하고 계속하는데 웹 훅은 오류로 중단 | 아래 "고친 것" |
| B | 원문에는 있는데 웹에 훅/분기가 없음 | 거주 구역·Player·스프라이트·주문 효과의 가상 삭제(고침), Wind 홈 통지(도달 불가) |
| C | 상수·기본값·호출 순서 차이 | 모두 기본값이 같거나 쓰이지 않음(무해) |
| D | `native=true`·`EnableNativeBody()`·`nativeArithmetic` 가 끄는 typed 선검사 | 정상 입력에서는 같고 비정상(비유한·범위 밖·이미 free) 입력에서만 갈림(무해) |

## 고친 것 (실제 동작이 달랐던 곳)

| 단위 | 곳 | 내용 |
| --- | --- | --- |
| E1207 | `raw-campaign-session-gems.js` | Gem(0xf6·0x104·0x10a)·Deck(`isOpen()`·목록 추가 여섯) 진단을 표시 후 계속 |
| E1207 | `raw-campaign-session-damageable.js` | 신전 삭제(Vortex.cpp 0x16f·0x2ab·0x2ac) 진단을 표시 후 계속 |
| E1211 | players·carrier-regular·flyers·spells·damageable·`raw-dude-task.js` | Player 정기(0x41d)·내용물 타이머 부모(Baseprocess 0xa4)·더스트 데빌(0xa4·0x19e)·애시드 송신(0x2fa..0x2fd)·Dude(0xde·0x127·0x9c)·낙하 task 교체 진단을 표시 후 계속 |
| E1215 | `raw-campaign-session-deletion.js` | 실제 앱 탐침에서 `'다른 실제 자산 가상 삭제 미연결'` 17종 → 거주 구역(0x50f540: 공통 Pre·목록 제거+공통 Post)·Player(0x50ee08: 이전 포인터/상태 정리 전처리)·주문 폭발 효과 열 개·0x50c9f0·0x50cc08·0x50db78(공통 Pre/Post)을 원문 분배표대로 연결. 남은 것은 멀티플레이 전용 Outpost 하나 |
| E1215 | `raw-campaign-session-runtime.js` | 의존 통지(`dependencyNotice`)를 감시자 VT 별 원문 분류로 교체: Factory 업그레이드 그림(건물과 같은 TYPE) 같은 자산 감시자는 flags 0 으로 삭제, 비행장 홈은 홈 통지, 나머지 자산·Depend/Gump/Content Form 은 빈 반환, Process Form 만 PID 통지 |
| E1215 | `raw-campaign-session-state.js` | 보호막 Regular 상태의 `checking` 연결(점검 모드의 Dais.cpp 0x479 진단) |

## 닫지 못한 차이 (PORT_MAP 해당 행에 기록)

- **DestroyPatch 진단 열두 지점**(Squid 삭제의 `AssertPatch` 0x24d·0x269·0x276·0x27d·0x280·0x27e·0x28f·0x2a2·0x2a9·0x15c·0x15d·0x123)과 `SquidDestroyLifecycle` 0x771: 원문은 표시 후 계속, 웹 `squid-destroy.js` 는 같은 지점에서 오류 중단. 식을 표로 옮겨 `assertDisplay` 로 바꾸는 단위가 필요하다(`RawSurfaceDeletion`·`RawSurfaceLifetime` 행).
- **적재 중 unit-lost 억제**(`unitLostSuppressed`)가 웹에서 설정되지 않음, `rememberPlacement` 미연결(`RawSurfaceDeletion`).
- **DependFormHost 진단**: notify 뒤 `watchedFid≠0`(Squid.cpp 0xadf), remove 인자 이상(0xb48·0xb49·0xb4b·0xb4c·0xb4e), FormFinder 시작 진단(Squidfinder.cpp 0x158)(`RawSurfaceDepend`).
- **Mana frameZero 훅**(비정상 SHP 에서 진단 후 계속), **Reward refreshSp**(SP 가산 뒤 표시 갱신), **Vortex islandNumber 편향**(`kSnapBias`)의 원본 여부 확인(`RawSurfaceMana`·`RawSurfaceReward`·`RawSurfaceVortex`).
- **타깃(0x130)·활성화(0x206)·타이머 패킷(0xcc/0xe6) 진단**: cpppj typed 와 웹이 같은 오류 중단이며 원문 헤더는 복귀 가능한 진단이라 적는다(원문 식 문자열을 PE 에서 읽어야 한다).

## 방법 메모

- 실제 앱 탐침: 미션을 시작한 뒤 `window.netstorm._campaignWorld.session().world()` 로 TYPE 별 `create`→`destroy` 를 시도해 오류 문구를 모았다(임시 Playwright spec, 확인 뒤 삭제). 좌표 없이 만든 자산은 `Damageable 해방 좌표 범위 오류` 가 나오는 탐침 부작용이므로 제외한다.
- 읽기 전용 에이전트 네 개가 `RawSurface*` 스물세 파일의 훅 대응을 나눠 대조했고, 위 고친 것/닫지 못한 차이는 그 보고를 코드로 다시 확인한 뒤 반영했다(확인하지 못한 항목은 보고에서 '미확인'으로 표시).
