# webpj 건설 확정(RawConstructionConfirm)과 건설 계열 통합 이식 — WB04-E139

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백서른아홉 번째 부분이다. 서버의 건설 확정(Construction.cpp `00444590`)을 옮긴 `o/RawConstructionConfirm`(cpppj 기준선 `1f00a25a` 뒤 갱신판 `77a0c3a4` 를 옮겼다: [E141 문서](webpj-construction-data-20261109.md) 3절 참고)과, 지금까지 옮긴 건설 계열 실제 모듈(Confirm·Notice·Clear·Place·Packet)을 실제 SID 생성/소유자/보상/SP 저장소와 함께 한 번에 잇는 통합 시험이다. 배치 가능 판정을 통과한 요청에 대해 조각마다 서버 SID 를 확보하고 배치 통지(메시지 0x3f)를 만들어 보낸 뒤 같은 통지를 자기 쪽 처리기에 직접 넘긴다. 선행은 E134 통지·패킷, E136 Canon 디코더, E137 정리, E138 배치다.

## 1. 옮긴 것

`www/core/raw-construction-confirm.js`

- `ConstructionConfirmState` — `skipBuilderCheck`(0054db14 독립 fallback)·`recorderEnabled`(0059a7e4)·`patternTypes` 와 실제 owner 를 빌리는 함수 `sharedFullDeck`·`sharedRecorderEnabled`·`patternTypesRead`·`decoderDiagnostics`. `skipBuilderCheckNow()`/`recorderEnabledNow()` 가 원문 분기마다 현재 비영 여부를 다시 읽는다(최상위 비트도 참).
- `RawConstructionConfirm(pool, types, frames, state, hooks)` — 10.78 `SidPool`·타입 표 188칸·필수 경계(`findBuilder`·`create`·`recorderActive`·`record` 와 `broadcast`+`handle` 또는 `transmitAndHandle`)를 생성 때 확인한다.
  - `confirm(request)` (typed): 타입 번호(255 이하·표 안)를 효과 전에 확인하고, 타입 flags1 의 "사제가 지어야 함"(0x8000)이 있고 요청 flags 비트0 이 없고 기준점 조회 생략 전역이 꺼져 있으면 먼저 `findBuilder`, 0 이면 효과 없이 거짓. 그 밖에는 타입별 디코더 사본으로 조각 수를 세어(20개 이상이면 SID 생성 전 거부) 조각마다 `create(type, 0)`(번호가 5 미만이거나 풀 밖이면 거부)해 통지(좌표·시각은 비트, 나머지는 하위 바이트)를 담아 `broadcast` → `handle`(서버 번호 9 가 보낸 것처럼 직접 처리) → (기록기가 켜져 있고 활성이면) `record`(첫 SID, 조각이 없으면 0)를 부르고 참을 돌려준다.
  - `actualConfirm(request, header)` (실제 경로): 호출마다 독립 512바이트를 헤더로 초기화(0xff)한 뒤 디코더 생성(현재 패턴 타입) → 조각마다 `create` → 현재 SID WORD 저장 → 개수 증가 → 20개 이상이면 원문 진단(`sidCount < MAX_SIDS`, 0x32d) 복귀 뒤 계속 → 필드 쓰기 → `transmitAndHandle`(같은 패킷 바이트) → (기록기) `record`(지역 첫 SID).
- `makeConstructionConfirmHooks(pool, factory, hooks)` — SID 생성만 같은 풀의 실제 factory 로 바꾼다(다른 풀 거부).

## 2. 검증

`tests/construction-confirm.test.mjs`(5건):

1. `constructionconfirm-x86.tsv` 의 `originals` 508행(`originalCD`·`original1037` 은 다른 판본이라 제외) — 요청 열 개·전역·타입 flags1·기준점 조회 결과·기록기 상태·프레임 표·조각 수에서 효과 순서(B/C/S/H/Q/R)·통지 본문(좌표·시각 비트, 하위 바이트 필드, SID 목록)·반환값·만든 SID 수가 모두 같다.
2. 필수 경계 각각의 누락·다른 판본·다른 풀의 factory 거부, 예약 번호를 돌려준 SID 생성·타입 300 은 통지 전에 거부, 조각이 없으면 기록기에 0, factory 어댑터는 `create` 만 바꾼다.
3. 공유 FullDeck 의 현재 DWORD 가 기준점 관문을 결정한다(0·`0x80000000`·`0xffffffff`, 공유 owner 가 없으면 독립 bool).
4. SID 생성 전에 현재 Canon 진단(금 간 프레임 0x325)을 소비한다(중단 시 생성 0회).
5. 실제 wire 경로의 지역 512바이트가 같은 통지를 `writeConstructionPacket` 으로 직렬화한 것과 바이트 단위로 같고 첫 SID 를 기록기에 넘기며, 송신/진단 경계가 없으면 거부하고, 사제 기준점이 없으면 패킷을 보내기 전에 거짓이다.

`tests/construction-chain.test.mjs`(1건) — **통합**: 실제 `SquidFactory`·`SquidOwner`·`SquidReward`·`ScrambledSpStore` 위에서 noIsland 예측 아홉 칸(클라이언트 영역 SID, abstract)을 놓은 뒤 실제 `RawConstructionConfirm`(서버 영역 SID 생성) → 실제 `RawConstructionNotice`(확정 직접 처리) → 실제 `RawConstructionClear`(예측 아홉 개를 칸 순서대로 같은 조각 플래그 `0x2000000` 로 지우고 SID 반납) → 실제 `RawConstructionPlace`(칸마다 서버 영역 조각 하나, abstract 아님, 소유자 2, 시작 칸 단어 `0x5d4`, Pop `0x41`)로 이어지고 갱신이 정확히 한 번, 최근 건설 위치 이력이 `(20, 21)` 로 바뀐다. E134~E138 의 어댑터 이름 계약(`clear.clear`·`place.place`/`actualPlace`/`refund`)이 실제 모듈로 확인됐다.

최초 실패와 교정: 새 모듈은 508행 fixture 를 첫 실행에 통과했다. 시험의 노이슬랜드 패턴 번호(받침은 1개뿐이라 인자 0)와 통합 시험의 타입 설정(HP 보유 타입이면 확정 조각도 abstract 가 되므로 HP 비트 없는 타입)을 각각 한 번 고쳤다.

## 3. 경계와 남은 것

- 사제 이동·환불 분기(`immobile`·`approach`·`tryMove` 등)와 `RawConstructionRequest`(배치 가능 판정)·`Local`·`Command`(패킷 분배 등록)·`Data`·`Builder` 는 아직 옮기지 않았다.
- `actualConfirm` 의 20개 진단은 패턴 한 칸 최대 15개 때문에 정상 입력에서 닿지 않아 시험하지 않았다(원본 코드 경로만 보존).
- 10.72(CD) 경로는 범위 밖이다.
- **이식 원본의 판:** `RawConstructionConfirm` 은 `77a0c3a4`(20261108-151 CP)의 판(`ActualConfirm`·공유 전역)을 옮겼다. 기준선 판에는 실제 wire 경로가 없으며 PORT_MAP base 를 해당 커밋으로 바로잡았다(E141 기록).

## 4. 다음 시작점

`RawConstructionRequest`·`Local`·`Command`·`Data`·`Builder`(선행 `RawPriestPlacementGeometry`·`RawPacketDispatch`), `RawManaBoltArrival`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
