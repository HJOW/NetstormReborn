# webpj 애시드 울타리(RawAcidFence) 이식 — WB04-E50

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 쉰 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawAcidFence`(10.78; 원본 48c7a0 최초 0.3 예약 후처리, 48c830 공격, 48ca70 13영역 포획, 48ea30 Regular)를 옮겼다. 이웃 수집은 [E49](webpj-arc-spire-20261108.md)의 `RawArcSpire.collect`를, 포획 명령 작성/수신은 [E48](webpj-acid-zap-20261108.md)의 `RawAcidZap`을 그대로 쓴다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-acid-fence.js` — `postPopAcidFence`, `RawAcidFence`

- `postPopAcidFence(pool, parent, flags, reserve, regular, damageable)` [48c7a0]: 상태 비트 `9`가 없고 `flags & 1`이며 `reserve()`가 참일 때만 `regular(parent, 0, 0.3)`(float), 이어 항상 공통 `damageable(parent, flags)`.
- `new RawAcidFence(pool, types, frames, spots, alliances, state, friendlyFire, special, zap, hooks)`: 10.78 풀·188 타입/프레임·256×256 표면·81개 동맹을 빌린다. `state = {boss, editor, localOwner}`는 후보마다/송신마다 다시 읽고, `friendlyFire()`는 친군 사격 DWORD의 현재 값(비영이면 허용), `special`은 현재 TYPE 전역 여섯 개(`rainFlyer 144`·`sunFlyer 73`·`windFlyer 138`·`rainBalloon 146`·`windBalloon 141`·`sunBalloon 111`)로 **후보마다** 읽는다(효과 도중 바뀌어도 반영). 훅 `{range, find, iterate, reserveRegular, regular, damage, broadcast, assertion, setFrame}`는 도달한 지점에서만 요구한다.
  - `handle(parent, event, count, period)` [48ea30]: 중지 비트(`0x20`)가 없을 때 event0은 count0이어도 `attack`, event1은 count를 DWORD로 올려 `last = 총 프레임 − 1`이 `(int32)count`보다 작으면 0, 아니면 `(last − count)` 프레임으로 `setFrame`. 공통 Regular로 위임하지 않고 `period`를 돌려준다.
  - `attack(parent)` [48c830]: 부모 좌표(절삭)와 범위로 +X/+Y 축을 `RawArcSpire.collect`(find만 연결)로 스캔한다. 가로에서 이웃을 찾으면 `strike(…, 1)`, 이어 세로 `strike(…, 0)`, 어느 한 축이라도 이웃이 있었고 예약이 성공하면 부모에 `regular(parent, 1, 0.2)`.
  - `strike(parent, neighbors, xOriented)` [48ca70]: 이웃마다 ① 이웃 애니메이션 예약 `regular(이웃, 1, 0.2)` ② 첫 영역(genus `0x50100`, 타입 제한 없음) ③ 144/73/138 부모·이웃 발자국 영역 6개(`0x100000`, 후보 현재 TYPE과 비교) ④ 비행체 3(144·138·73)·운반체 3(146·141·111)의 가로/세로별 높이 연결 영역 6개를 이 순서로 모은다(최대 40개, 중복 허용). 수집은 free/dead를 거르지 않고 아군(정방향 동맹만, 친군 사격이 켜지면 허용)·부모·이웃·`0x200` genus·보스 아님·편집 모드·TYPE 불일치를 거른다. 이어 후보마다 **실행 직전** 포인터/상태(`&3`)를 다시 읽어 죽은 후보는 건너뛰고, 사제(`0x200000`)는 피해 1000(부모 현재 owner), 나머지는 `zap.build` → `broadcast` → 패킷 WORD/방향 불변식(0x2fa~0x2fd 진단) → 로컬 `zap.receive`(localOwner, 0).
  - 각 영역의 float 저장 지점(`Stored`)과 binary64 상수 5.3(50e870)은 원본대로 보존하고, 영역 경계는 `+0.99999`(float 502310)를 오른쪽/아래에만 더해 `ftol`한다.

## 2. 검증 (`tests/acid-fence.test.mjs`, 5건)

독립 원본 관측 `acid-fence-x86.tsv` 1125행(10.78 단일 관측): P 50·R 84·A 43·B 43(정수 좌표 경계)·S 903·C 2. 입력은 원본과 같은 47개 raw 슬롯(부모 50·이웃 60/61·후보 70~113)이며 모든 하위 효과(검색 `I`/`F`·예약 `N`/`R`·피해 `H`·송신 `S`·진단 `!`·프레임 `B`·소리 `W`·분해 `X`·삭제 `K`)를 원본 기록과 같은 문자열로 남기고, 효과 도중 슬롯/전역/패킷이 바뀌는 9종 변경(송신 뒤 대상 죽음·부모 owner 변경·삭제 뒤 부모 좌표 이동·스캔 중 좌표 이동·후보 TYPE 변경·피해 뒤 사망·송신 훅의 패킷 대상 변경·예약 중 보스 해제·스캔 중 `rainFlyer` 전역 변경)을 포함해 47개 슬롯 전체와 반환 비트까지 대조한다. 계약: 구성 오류/판본 불일치, 후처리의 비트 9 관문과 0.3 지연, Regular의 마지막 프레임 초과·중지 비트, 송신 훅이 패킷을 바꿨을 때 해당 불변식 진단만 남는지.

최초 실패와 교정: 모듈은 1125행 전부를 첫 실행에 통과했다.

## 3. 경계

- 실제 공간 검색(4b1fa0/4b16d0/4b1810)·예약(`new40`)·Regular 등록(496e80)·사제 피해(대상 +98)·패킷 송신(4e2e30)·진단 장치·프레임 변경(4acee0)은 월드 조립에서 공급한다. 로컬 수신의 소리/분해/삭제는 `RawAcidZap` 훅으로 공급한다. 친군 사격 DWORD(005e4794 계열)는 호출자가 `friendlyFire()` 함수로 넘긴다.

## 4. 다음 시작점

WB04 안의 나머지는 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState`·`RawPlayerGeyserBounds`·`RawPlayerPlacementAnchor`), 종류별 PostPop 본체, `SquidDisplay`/`SquidFrame`이다.
