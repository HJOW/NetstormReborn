# webpj 아크 스파이어(RawArcSpire) 이식 — WB04-E49

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 마흔아홉 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawArcSpire`(10.78; 원본 48c080 최초 Regular 예약 후처리, 48c110 공격, 48c300 이웃 수집, 48c410 연쇄 타격, 48c6e0 Regular)를 옮겼다. 다음 단위의 애시드 울타리(`RawAcidFence`)가 이 모듈의 `collect`를 그대로 부른다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-arc-spire.js` — `postPopArcSpire`, `RawArcSpire`

- `postPopArcSpire(pool, parent, flags, reserve, regular, damageable)` [48c080]: 상태 비트 `9`가 없고 `flags & 1`이며 `reserve()`가 참일 때만 `regular(parent, 0, 1.0)`. 이어 항상 공통 `damageable(parent, flags)`. 각 훅은 도달한 지점에서만 요구한다.
- `new RawArcSpire(pool, types, frames, spots, alliances, state, hooks)`: 10.78 풀·188 타입/프레임·256×256 표면 바이트·방향 있는 81개 동맹 값을 빌린다. `state.editor`(편집 모드 전역 005c85a4)는 피해 직전마다 다시 읽는다. 훅 `{range, damageRate, find, iterate, beam, damage, setFrame, damageable}`는 호출 시점에만 요구한다(없으면 `TypeError`).
  - `handle(parent, event, count, period)` [48c6e0]: 상태 비트 `0x20`이 없을 때 event0이고 count≠0이면 `attack`, event1이면 count를 DWORD로 올려 `(int32)count > 총 프레임`이면 공통 후처리 없이 0을 돌려주고, 아니면 `(총 프레임 − count)` 프레임으로 `setFrame`. 반환은 공통 `damageable`의 float.
  - `attack(parent, period)` [48c110]: 피해 = `ftol(period × damageRate)`. 부모 좌표(절삭)와 범위로 +X/+Y 두 축만 검사한다. `(x, y, 끝−1, y)` 사각형의 표면 OR에 비트 1이 있으면 칸별로 표면 비트 1인 곳에서 `collect`하고 첫 양방향 동맹을 찾으면 그 축을 끝낸 뒤, **각 축마다 별도로** `strike`한다(세로 축은 가로 효과 뒤의 현재 부모를 읽는다). 지도 밖 칸은 `RangeError`.
  - `collect(parent, buffer, count, x, y, type)` [48c300]: `find`가 돌려준 같은 위치의 이웃이 free/dead(비트 `0x21`)가 아니고 양방향 동맹이며 현재 좌표가 정확히 (x, y)일 때 `{found: true}`. 버퍼가 가득 차도(`count ≥ buffer.length`) 유효한 첫 이웃이면 참이고 개수는 그대로. 동맹 owner가 9 이상이면 `RangeError`.
  - `strike(parent, neighbors, damage)` [48c410]: 이웃마다 부모−2~이웃+0.99999(float 상수 502310) 사각형을 `iterate`로 훑어 적 후보를 최대 40개 모은다(free/dead·부모·이웃·아군 제외, form 피해 genus는 상태 비트 1, 아니면 `flags1 & 0x10`, genus `0x120000` 제외). 이어 후보마다 `beam(커서, 후보)` → 편집 모드가 아니면 `damage(후보, 피해, 부모 현재 owner, 부모)` → 후보 상태 비트 7이 없으면 커서를 후보로 옮기고, 마지막에 `beam(커서, 이웃)`.

## 2. 검증 (`tests/arc-spire.test.mjs`, 5건)

독립 원본 관측 `arc-spire-x86.tsv` 665행(10.78 단일 관측; 행마다 x87 두 정밀도 결과가 같은 것만 수록): P 50·R 84·N 192·A 243·S 90·T 6. 입력은 원본과 같은 12개 raw 슬롯(부모 50·이웃 60/61·후보 70~113)이며 효과(`A`/`F`/`I`/`L`/`H`/`B`/`C`/`N`/`R`/`D`)는 원본 문자열과 같게 기록하고, 콜백이 바꾸는 슬롯 6개 전체·반환 float 비트·개수·버퍼 hex까지 비교한다. 계약: 구성 오류/판본 불일치, 후처리의 비트 9 관문과 필요한 훅만 요구, Regular의 프레임 초과·DWORD 감김·중지 비트, 지도 밖 축 스캔 거부, 이웃 없을 때 `iterate` 불필요, 동맹 owner 범위.

최초 실패와 교정: 모듈은 665행 전부를 첫 실행에 통과했다.

## 3. 경계

- 실제 공간 검색(4b1fa0 같은 위치·타입 검색, 4b16d0/4b1810 flags1 순서 검색)·빔 생성(472700; 생존 관문과 `shortZap.wav`)·피해 전달(대상 +98)·프레임 변경(4acee0 Unpop/Pop)·공통 Regular(44b900)는 월드 조립에서 공급한다. 타입 메타(`range`/`damageRate`)도 호출자가 현재 TYPE 값에서 읽어 넘긴다.
- 편집 모드 전역·동맹 표(81 DWORD)·표면 지도는 세션이 소유하고 이 모듈은 빌려 쓴다.

## 4. 다음 시작점

`RawAcidFence`(+`acid-fence-x86`, `PostPopAcidFence`)가 이 `collect`와 [E48](webpj-acid-zap-20261108.md)의 `RawAcidZap.build`를 쓴다. 이후 Player 나머지·종류별 PostPop·`SquidDisplay`/`SquidFrame`이다.
