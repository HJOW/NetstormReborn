# webpj 비로컬 의식 보상의 SP 모드 대조 RawForeignRitualReward (WB04-E371, 2026-11-13, 7-P3 신전·제단·의식 부품 일곱 번째 항목)

3.1절 7-P3 항목 "RawForeignRitualReward (partial 57줄)"의 남은 부분 — **mode 1(SP) 180행의 실제 분산 SP 저장소/정책 대조** — 를 닫아 항목을 `partial` → `port` 로 올렸다. 모듈 `core/raw-foreign-ritual-reward.js`(E106: `grantForeignRitualSp`·`grantForeignRitualType`)의 **코드는 바뀌지 않았다.** 정책 helper `readPlayerSpAmount`/`setPlayerSpAmount`(E278 `raw-fort-money-load.js`)와 분산 SP 저장소 `ScrambledSpStore`(`sp-store.js`)가 이미 이식돼 있어, 이 단위는 E106 이 미뤄 둔 대조를 시험으로 수행한다. 기준선 뒤 cpppj 변경은 이 파일에 없다(`git diff 1f00a25a HEAD` 변경 0 — PORT_MAP base `1f00a25a` 유지). E167·E181 이 미확정으로 남긴 "분산 SP 정책 연결"은 정책 helper 가 E278 에서 이식된 시점부터 연결 가능했고, 이 단위에서 원본 관측으로 확정했다.

## 1. 규칙 (E106 에서 옮긴 규칙의 재확인)

- **`grantForeignRitualSp(owner, balance, set)` (004482b0):** 현재 잔액 getter(`balance(owner)`, float)에 5000 을 더해 기본 x87 53비트 정밀도(JS 배정도)로 절삭한 signed64 하위 DWORD(비유한·범위 밖은 0)를 정책 setter `set(owner 의 signed32 비트, 값)` 에 넘긴다. 가산용 AddSp·AI 지갑을 부르지 않는다.
- **실제 정책 연결(이 단위가 대조):** getter 는 `readPlayerSpAmount(owner, store, reads)`(0040ef30: 활성이면서 owner 가 로컬 owner 와 같으면 로컬 지갑, 아니면 owner 1..8 의 지갑, 그 밖은 0 — 비활성/비로컬 0 은 lazy 저장소를 읽지 않음), setter 는 `setPlayerSpAmount(owner, amount, store, reads, refresh)`(00447810: 활성 로컬이면 편집기 값 또는 float 로 저장하고 표시 갱신 `refresh()`, 아니면 owner 1..8 에만 저장, 편집기 지정 시 특수 로컬 SP)이다. 둘은 상태 사본을 공유하지 않고 현재 known/owner/편집기를 호출 때마다 다시 읽는다.
- **기술 Core (`grantForeignRitualType`):** 기술 비트 -1 은 쓰지 않고, 범위(0..31) 밖은 진단 0x15a 뒤 정상 복귀하며 `1 << (bit & 31)` 로 계속한다. 진단 뒤 현재 장부의 비정렬 DWORD(owner×16 + 10)만 읽고 써 이웃 필드를 보존하며, 물리 끝에 못 미치면 부분 쓰기 없이 거부한다.

## 2. 웹 범위

- 10.72(CD)는 범위 밖이다. 이 모듈의 호출 지점(의식 보상 선택·획득 종류 반영 `grantCapturedType`)은 앞선 단위(E367 `RawDaisRegular`)가 훅으로 부르며 앱 수명에는 연결하지 않았다. DOM 을 쓰지 않는다.

## 3. 검사 (`tests/foreign-ritual-reward.test.mjs` 6건 — 기존 2건 + 신규 4건)

- **원본 독립 관측(10.78) — 신규:** `foreign-ritual-reward-x86.tsv` mode 1 의 기본 53비트 제어(0x27f) **180행**을 실제 `ScrambledSpStore`(같은 `GameRandom`, 재시드 7, 모든 owner 지갑을 관측의 원시 float 비트로 준비)와 현재 `readPlayerSpAmount`/`setPlayerSpAmount` 로 재생한다. getter/setter 호출이 각각 정확히 1회, setter 에 넘기는 owner 의 부호 없는 비트가 입력과 같고, 원본 `S:owner:값` 저장 이벤트와 같은 값이 owner 지갑에만 써지며(쓰기 여부가 owner 1..8 과 일치) `U` 표시 갱신 횟수가 같고 **이웃 지갑 여덟 개는 입력 그대로**임을 확인했다 — 모두 첫 실행에서 일치했다. 확장 정밀도(0x37f) 180행은 원시 감사용이라 수만 센다(cpppj `ForeignRitualReward_SpReplaysNative53Bit180WithActualStoreAndPolicy` 와 같은 범위).
- **웹 계약 — 신규:** SP getter 가 비활성 로컬 0·활성 비로컬 `0xffffffff` 에서 lazy 저장소를 초기화하지 않고 0 이며 활성 로컬 owner 비교 뒤 현재 owner 를 다시 조회하는 것(cpppj `ForeignRitualReward_GetterPreservesLazyAndCurrentOwnerReads`), Client 수명의 실제 9기록 장부(`ClientCoreDataGlobals.records()`) 직접 쓰기와 같은 주소 유지·이웃 필드 보존·비트 없는 TYPE 무요구(cpppj `ForeignRitualReward_CoreDiagnosticAndClientLedgerAreCurrent`), 그리고 웹 추가로 기술 DWORD 가 물리 장부 끝에 정확히 들어맞는 경계(길이 14 / origin 2 + 길이 16 허용, 13 거부, 시작 위치가 길이 이상이면 거부).
- 기존 2건(E106)은 그대로 통과한다: 기술 모드 144행의 256바이트 Core 창·효과 순서와 SP 덧셈 계약.

## 4. 변형 확인과 남은 경계

- 모듈의 연산자·상수 자동 변형 43개를 새 시험까지 포함해 적용했다. 1차에서 5개가 남았다: 정확히 맞는 길이의 경계(`4 > 남은 길이`의 `4`·`>` 두 건)는 신규 경계 시험으로 잡았고, 남은 동등 변형 3개는 `value >= 2^63`→`>`(정확히 2^63 의 하위 32비트도 0)·`value < -2^63`→`<=`(정확히 -2^63 도 하위 0)·`offset > 길이`→`>=`(시작 위치가 길이와 같으면 바로 이어지는 남은 길이 검사가 같은 경우를 거부)이다.
- 이 단위로 7-P3 신전·제단·의식 부품 7개가 모두 끝난다(개별 항목의 실제 월드 연결은 7 단계). 이월: `DaisRitualConsumer` 통합 시험은 `SquidPostPop` 다이스 몸체 연결과 함께(E369 문서 4절).
- 다음 단계: 3.1절에서 **현재 단계 4-D 의 남은 항목은 모두 선행 대기**(0.1절 다음 작업)이고 6-P·7-P1~P3 이월 부품이 모두 끝났다. 4-D 가 요구하는 client 부품(메뉴·커서·바인딩·소리·경로)을 5-A·6 영역에서 앞당겨 연다.
