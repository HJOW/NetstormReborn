# webpj 공간 해시와 일반 사각형 탐색기 — WB04-B

2026-11-05, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서1이다. 원본 10.78의 **SquidHash(4단계 버킷 머리 배열)**와 **RawSquidFinder(일반 사각형 탐색기)**를 WB04-A의 같은 SID 풀에 연결해 옮겼다. 실제 객체 생성·spot 등록·Pop·Fort 적재와 화면 연결은 아직이며 화면 변경은 없다.

## 1. 근거와 동기화 범위

- 시작 트리 `f845d6c5`(WB02-F 직후). 현재 PC는 마지막 전체 정밀 디컴파일 PC(**HJOW-Athlon / 2026-11-03**, 공통 인계)와 같다. 디컴파일 산출물·dotnetpj 구현은 근거로 쓰지 않았다.
- 근거: cpppj `src/o/SquidHash.cpp/.h`(마지막 변경 `a9af29da`), `src/o/RawSquidFinder.cpp`(`f58fec2e`)·`.h`(`225f2322`), `tests/HashTests.cpp`, `tests/RawFinderTests.cpp`, 읽기 전용 원본 관측 `tests/fixtures/hash-x86.tsv`(`65927742`)·`rawfinder-x86.tsv`(`5879152b`). 이식 추적 목록 [manifest](../webpj/tests/fixtures/manifest.json)에 `squid-hash`·`raw-squid-finder`로 등록했다.
- 착수 전 `node webpj/tools/fixture-manifest.mjs`의 cpppj 변경 보고(7곳, [이식 추적4절](webpj-fixture-trace-20261105.md))는 이번 대상 소스와 겹치지 않았다.

| 원본 10.78 | 웹 API | 범위 |
| --- | --- | --- |
| `004b28f0` 초기화 | `new SquidHash(selected)`·`reset()`·`select()` | 네 단계 0으로 지움, 선택 단계 보존 |
| `004b29a0` At / `004b2b00` 정수 버킷 | `at/setAt`, `cell/setCell`, `SquidHash.cellIndex` | y 바깥·x 안쪽 인덱스 |
| `004b2a90` float 버킷 | `bucket/setBucket`, `SquidHash.bucketIndex` | float32 인자 → 0 방향 절삭 → scale 나눗셈 |
| `004ace40` 단계 선택 | `SquidHash.objectLevel(flags2, w, h)` | 섬/다리 0, 프레임 큰 변 ≤2→1, ≤4→2, 그 외 3 |
| `004b16d0` Begin / `004b1810` Next | `RawSquidFinder.begin(area, flags, filter)`·`next()`·`state` | 플래그 1(표면 건너뛰기)·4(높이 확장), buried 제외, OR 교차식 |

## 2. 파일과 사용 계약

신규: `webpj/www/core/squid-hash.js`, `webpj/www/core/raw-squid-finder.js`, `webpj/tests/squid-finder.test.mjs`. DOM에 의존하지 않는 ES 모듈이다.

```javascript
// WB04-A 의 같은 실제 풀과 공간 해시를 만든다.
const pool = new SidPool(32768, { serverOwner });
const hash = new SquidHash();
// 타입 번호 → { footX, footY } 표이다. 실제 타입 표 연결은 후속 단위다.
const finder = new RawSquidFinder(pool, hash, types, heightExtensions);
// 경계를 포함한 정수 사각형(오른쪽/아래 -1 은 보드 끝)과 겹치는 객체를 원본 순서로 하나씩 받는다.
for (let sid = finder.begin({ left: 20, top: 21, right: 20, bottom: 21 }); sid !== 0; sid = finder.next()) { /* ... */ }
```

- **SquidHash:** JS는 참조를 돌려줄 수 없으므로 읽기(`cell`/`bucket`/`at`)와 쓰기(`setCell`/`setBucket`/`setAt`)를 나눴다. 값은 WORD(0~65535)만 받는다. `entries(level)`은 SID 풀의 `slot()`과 같은 정책의 **읽기 사본**이다. cpppj의 `MutableEntries`(Editor 직접 쓰기)는 웹 제외 기능용이라 옮기지 않았다. 범위 밖 단계/칸/좌표·NaN은 `RangeError`다(원본 포인터 산술 대신 cpppj와 같은 보호).
- **float 인자:** `bucketIndex`/`objectLevel`은 입력을 `Math.fround`로 float32로 만든 뒤 계산한다. 예: `255.99999999`는 float32에서 256이 되어 보드 밖이다.
- **RawSquidFinder:** 풀/해시/타입 표는 복사하지 않고 빌린다. 후보마다 `pool.slot(sid)`로 **현재 슬롯을 다시 읽는다**. 후보의 next는 필터·호출자 효과보다 먼저 저장하고, 이후 후보의 좌표/타입/buried와 아직 읽지 않은 버킷 머리는 읽는 시점의 값이다. 같은 버킷 체인 순환·풀 밖 번호·타입 표 밖 번호·음수/넘친 발자국은 오류다. 10.78 풀(`edition === 'patch1078'`)만 받는다.
- `finder.pool`/`finder.hash`는 연결된 실제 저장소(사본 아님)를 공개한다. WB03-AB Fort 로더의 같은 풀 계약(`finder.pool === pool`, `begin(rect)`, `next()`)을 만족하지만 **이번에는 로더에 연결하지 않았다**(생성/Pop/소유자 필수 효과가 없음).
- `state`는 원본 finder +4..+60(사각형·첫/마지막 버킷·단계·x/y·next·current·두 플래그)의 읽기 사본이다. 전체 소진 뒤 단계는 4, current는 0이다.

## 3. 검증

| 검사 | 결과 |
| --- | --- |
| 해시 초기화·주소 | 원본 Init 4, At 100, 정수 버킷 100, float 버킷 607 = **주소 807** 일치 |
| 단계 선택 | `Level` **605행**의 두 결과 열과 일치 |
| 탐색 커서 | 10.78(`originals`) **Find 584행**: 장면 입력(체인·좌표·타입·상태·extra·발자국·확장 높이·단계)을 같은 실제 풀/해시에 넣고, 첫 반환 뒤 원본과 같은 시점의 입력 변경(next 끊기/buried/좌표 이동/타입 교체)을 적용해 매 호출의 커서 15값 전체를 비교 |
| 계약 | 시작 전 next·플래그 8·확장 높이 표 부족·CD 풀 거부, 자기 순환·풀 밖 머리 거부, 필터가 next를 지워도 미리 읽은 next로 진행, 읽지 않은 버킷 머리는 새 값 사용, 탐색이 풀 바이트/네 단계 머리를 바꾸지 않음, 10.78 0 좌표 후보 건너뜀, 단계 독립/초기화/범위 밖 거부 |

- 음성 대조: 임시 사본에서 ① 마지막 버킷 +1 확장 제거 ② buried 검사 제거 ③ 발자국 왼쪽 +1 제거를 각각 넣으면 Find 584 대조가 실패함을 확인하고 임시 파일을 지웠다.
- 신규 `squid-finder.test.mjs` 7/7, 이식 추적 목록 37항목 해시 불일치 0. 전체 결과는 인계 문서 0.1절에 기록한다.
- 쓰지 않은 행: `hash-x86.tsv`의 Genus 3216행(spot 점유 `Squid::EffectiveGenus`, WB04-E), `rawfinder-x86.tsv`의 Pre/Post 24행(다리 삭제 훅 `RawBridgeLifecycle`, WB04-D/E)과 CD/10.37 Find 1168행(웹 범위 밖).
- 원본/CD 실행, analyzeManager, 디컴파일은 하지 않았다.

## 4. 남은 경계와 다음 시작점

- 타입 표의 실제 발자국(`footX`/`footY`)·높이 확장(type +1e4)은 웹 타입 레코드에 아직 없다. finder는 `{footX, footY}` 배열을 받는 계약으로 두고 실제 타입 표 연결은 생성자/Pop 단계(WB04-D/E)에서 한다.
- 버킷 체인 삽입/제거(spot 등록·Unpop)와 표면(flag 8) 이웃 탐색은 미이식이다. 별도 좌표 목록/정렬로 대신하지 않는다.
- 다음은 3.2절 순서2 **WB04-C**(세션 장기 소유 TYPE/모드·게임/CRT 난수·게임 시계·Kernel/Process/Regular·입력/예약 큐)다. 착수 때 `node webpj/tools/fixture-manifest.mjs`로 이번 항목의 `webCommit`(`worktree`)을 실제 커밋으로 갱신한다.
