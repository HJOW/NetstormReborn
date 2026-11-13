# webpj 신전 Ascend 의 실제 섬 런타임 직접 읽기 RawVortexAscend (WB04-E370, 2026-11-13, 7-P3 신전·제단·의식 부품 여섯 번째 항목)

3.1절 7-P3 항목 "RawVortexAscend (partial 181줄, +4)"의 남은 부분 — **`RawIslandRuntime` 을 복사 없이 직접 읽는 두 번째 생성자** — 를 `core/raw-vortex-ascend.js` 에 더해 항목을 닫았다(`partial` → `port`). 신전 Ascend 후보(004db680)·이벤트 0 본체(004db8e0)·섬 배열 경로의 외부 소유 섬 수(0046f480)는 E120([문서](webpj-vortex-ascend-20261108.md))에서 이미 옮겼고, 기준선 뒤 cpppj 변경 `+4`(`VortexAscendState::sharedNoIslandType`·`NoIslandType()`)는 E260 판정 ①로 웹이 이미 후보마다 `sharedNoIslandType` 상자를 읽고 있어 이 단위에서 바꿀 것이 없었다(PORT_MAP base `11fbb988` 유지 — `RawVortexAscend.cpp/.h` 의 마지막 변경 커밋).

## 1. 규칙

- **생성:** JS 는 생성자 중복 정의가 없어 `islands` 인자를 두 가지로 받는다 — ① 기존 배열 `{ exists, owner }[]` ② 같은 풀의 `RawIslandRuntime`(`instanceof` 로 구분). 하위 효과 네 가지(`islandNumber`·`reserveCandidateBuffer`·`releaseCandidateBuffer`·`destroy`)·판본·표/지도 크기 검사가 먼저이고(둘 다 틀리면 하위 효과 문구), 그다음 실제 목록의 풀이 `pool` 과 다르면 `Ascend 섬 목록과 raw 풀 불일치` 로 거부한다(C++ `invalid_argument`). 배열 경로는 풀 검사를 하지 않는다.
- **`countForeignOwnedIslands(owner)` (실제 목록):** 원본 두 판본처럼 반복마다 `count`(DWORD 를 signed 로 읽음 — `0x80000000` 은 0회)를 다시 읽고, `exists(i)` 가 참일 때만 `owner(i) !== owner >>> 0` 와 `owner(i) !== 0` 을 이어서 평가한다(같은 owner 면 owner 는 한 번, 외부 소유·소유자 0 이면 두 번 조회 — 원본 호출 횟수 보존). 삭제·owner 변경이 곧바로 보이고 기록·난수 상태는 바꾸지 않는다.
- **후보/반경 판정(`candidate`·`tick`):** 섬 목록과 무관하므로 두 경로가 같다.

## 2. 웹 범위

- 10.72(CD) 분기는 옮기지 않는다. 이 모듈이 Regular 어댑터·월드 조립에 연결되는 곳(신전 이벤트 0 호출자·`GameWorld`)은 후속 단계다. DOM 을 쓰지 않는다.

## 3. 검사 (`tests/vortex-island-binding.test.mjs` 4건, 기존 `tests/vortex-ascend.test.mjs` 2건 그대로 통과)

- **원본 독립 관측(10.78):** `vortexascend-x86.tsv` 의 originals Count 20행을 실제 3204바이트 기록에서 재사용 — 저장된 입력(owner·변형)대로 존재 DWORD(+4)·소유자 DWORD(+8)만 32바이트 간격에 쓰고(이웃/padding 은 `0xa5` 보존) 반환 개수·기록 전체 불변·난수 상태 불변이 원본과 같다(첫 실행 일치). 하위 효과는 Count 에서 호출되면 실패하는 훅이다(cpppj `VortexIslandBinding_ReusesOriginalCountRowsOnLiveRecords` 대응).
- **웹 계약:** 생성·삭제·owner 변경 뒤 현재 기록 재조회, owner 인자의 부호 없는 32비트 비교(`-1` = `0xffffffff`), signed count, 다른 풀 거부와 배열 경로 유지, 실제 목록을 줬을 때 배열을 쓰지 않음, 호출 횟수(`count` 반복마다·`exists` 뒤에만 `owner`·외부 소유/소유자 0 은 두 번), 하위 효과 네 가지를 하나씩 빼거나 함수가 아닌 값으로 바꿀 때의 거부와 검사 순서(cpppj `VortexIslandBinding_SeesMutableCountOwnerAndRejectsOtherPool` 대응 + 웹 추가).

## 4. 변형 확인과 남은 경계

- 새 코드 줄(생성자 60~75·실제 목록 계수 154~170)의 연산자·상수 자동 변형 36개 + 의미 변형 8개 = 44회를 적용했다. 1차에서 4개가 남았다: 생성자의 하위 효과 검사 `||` 두 건(`reserveCandidateBuffer`·`releaseCandidateBuffer` 를 개별로 빼는 입력이 없었음 → 계약 시험으로 보강)과 동등 변형 두 건(실제 목록일 때 `#islands` 를 `[]` 로 두는 구현 — 실제 경로는 그 배열을 읽지 않으므로 `this.#islands = islands` 로 단순화해 변형을 없앰)이다. 보강 뒤 생존 0.
- 시험이 먼저 실패한 곳은 모두 시험 쪽이었다(삭제한 섬 기록을 되살린 뒤의 기대 개수 계산 착오, 실제 `owner()` 가 내부에서 `exists`/`count` 를 다시 불러 호출 기록이 부풀던 스파이).
- 다음 7-P3 항목은 **RawForeignRitualReward**(partial 57줄; mode 1(SP) 180행의 실제 분산 SP 저장소/정책 대조 — E167·E181 에서 미확정으로 남긴 분산 SP 정책 연결과 함께 정한다)이다.
