# webpj 건설 표시(RawConstructionVisual) 이식 — WB04-E128

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백스물여덟 번째 부분이다. 건설 중인 건물 위에 흩날리는 입자 효과의 생성·버퍼 준비·좌표 동기화·초기 입자 선택(`Build.cpp`)을 옮긴 `o/RawConstructionVisual`(cpppj 기준선 `1f00a25a`)이다. 선행은 `SidPool`(부모 raw 읽기)과 공유 게임 난수 `GameRandom` 이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(부모 유효성이 단순한 쪽)는 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-construction-visual.js`

- `createConstructionVisualState()` — 공유 색 표(초기 0)·부착 타입(17)·월드→화면 배율(16, 11)을 담은 전역 객체. 실제 월드/클라이언트가 소유한다.
- `createConstructionVisualFields()` — 0x88 바이트 표시 객체의 복원한 필드(부모·zOrder·사각형·수명 범위·픽셀/출력 버퍼·입자 목록 등).
- `RawConstructionVisual(pool, state, random, hooks)`
  - `create(parent, instant)` — 패치 부모 유효성(owner<10·가상 표 유효·살아 있는 자산·종속 체인에 free/void 없음·순환 거부) → 0x88 객체 확보 → 부착 경계(타입 17) → 공유 색 표가 비어 있으면 8색 파랑→흰색·3색 흰색→회색 띠(잡음 2.4)를 만들어 합성 → 수명 범위(일반 1.3~2, 즉시 2~9) → SHP 캡처 → 출력 버퍼를 투명(0xff)으로 채움 → `synchronize`. 부착 경계가 부모 SID·색 표 캐시·부모 좌표를 바꿀 수 있고, 이후 읽기는 변경된 현재 값을 쓴다.
  - `synchronize(fields)` — 부모 raw 의 zOrder BYTE 를 부호 확장하고 월드 좌표×배율(x87 결과를 float 로 저장한 단정도 반올림)으로 사각형 두 모서리를 맞춘 뒤 `seed` 를 다시 실행한다.
  - `seed(fields)` — 불투명 수 → 입자 용량(최소 500) → 기존 입자가 있으면 반납 경계 → 확보 → 마지막 픽셀을 출력으로 복사하고 원본에서 제거 → 중앙 열부터 0,+1,−2,+3 으로 교차하는 열 순서에서 `height/3` 보다 아래 행의 첫 불투명 픽셀 하나씩 선택(수명은 공유 게임 난수 `next(65536)/65536 × |최대−최소| + 최소`, float 저장) → 열 수만큼 두 난수로 입자 전체를 교환. `selected` 는 초기화하지 않고 누적한다.
- `makeConstructionVisualHooks(pool, visual, store, hooks)` — 건설 시작의 표시 효과 경계(`effect`)를 `create` 에 연결하고 초기 상태를 `store` 로 넘긴다. 같은 풀이 아니거나 저장소가 없으면 거부한다.

원본이 `new` 실패 뒤 null 을 계속 쓰는 경로는 옮기지 않고 예외로 거부한다. 입자 열이 범위를 벗어나는 경로는 정상 크기에서 생기지 않지만 방어적으로 `RangeError` 로 거부한다.

## 2. 검증

`tests/construction-visual.test.mjs`(3건):

1. `constructionvisual-x86.tsv` 의 `originals` 592행(Create 520·Seed 60·Sync 12; `originalCD`·`original1037` 은 다른 판본이라 제외) — 부모 raw 입력, 캡처 패턴 7종(전체 불투명/구멍/하단만 불투명/중앙 열만/마지막 픽셀만/색 0 등), 공유 난수 상태, 색 표 캐시 상태, 부착·캡처 경계의 부모/좌표/캐시 변경(Mutate 1~3)을 주었을 때 외부 요청 순서(A 확보/B 부착/P 팔레트/G 색 띠/J 합성/C 캡처/F 입자 반납), 가상 표·필드 21개, 원본 SHP 픽셀(선택된 곳은 투명)·출력 버퍼·초기 입자 목록 전체, 난수 상태, 색 표 캐시가 모두 같다.
2. 계약: 객체/팔레트/버퍼 확보 실패를 첫 효과 전에 거부하고(난수는 소비하지 않음), 입자 확보 실패 시 픽셀을 바꾸기 전에 거부, 높이 0·출력 크기 불일치·가상 표/상태가 잘못된 부모를 거부(외부 요청 없음).
3. 계약: 필수 경계 누락·다른 풀의 어댑터·저장소 없는 어댑터 거부, 건설 시작 어댑터가 즉시 건설의 초기 상태(수명 2~9, 입자 있음)를 넘김, 색 표가 비었을 때 첫 생성이 만든 표를 두 번째 생성이 재사용(팔레트 생성 요청 없음).

최초 실패와 교정: 새 모듈은 592행 fixture 를 첫 실행에 통과했다. 계약 시험 하나의 기대 요청 수(팔레트 재사용 때 5건)를 맞췄다.

## 3. 경계와 남은 것

- C++ 의 `ConstructionVisual_StartStoresInitialStateBeforeConstructionTime`(실제 `RawConstructionProcess::Start` 와 어댑터 연결, 건설 시작 시각·종료 시각 계산)은 `RawConstructionProcess` 이식 뒤에 확인한다. 어댑터 자체(`effect` 호출→`create`→`store`)는 위 3번에서 확인한다.
- 표시 등록/해제(`RawRenderRegistry`·`DestroyConstructionVisual`, `RawConstructionLifetime`)와 프레임(`RawConstructionFrame`), 수집기(`RawRenderCollection`)는 이 모듈의 필드를 이어서 쓰며 다음 단위이다.

## 4. 다음 시작점

`RawConstructionLifetime`(표시 목록 등록/해제·버퍼 반납 순서; fixture 459행) → `RawConstructionFrame` → `RawRenderCollection`(4305행), `RawConstructionProcess` 계열, Missile Host 팩토리(`RawGunProcess` 이후), `RawGunImpact`(SquidFrame/Host 장면 필요), `RawWindAviaryProcess`·`RawWindFlyer`, 비행체, 번개 수명/프레임/화면, `RawPathProcess` 계열(커널/Form 장면 선행), `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책, 그 밖의 작은 몸체이다.
