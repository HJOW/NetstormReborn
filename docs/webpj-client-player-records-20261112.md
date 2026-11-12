# webpj Client 수명 Player 기록 소유자와 공통 시작 (WB04-E332, 2026-11-12, 4-D A 구획)

3.1절 4-D A 구획의 client **ClientPlayerRecords**(기준선 173줄 = `.cpp` 105 + `.h` 68, 기준선 뒤 +7줄)·**ClientPlayerRuntime**(26줄 = 20 + 6, 변경 없음)과 이 둘이 쓰는 **CurrentAllocationBytes**(헤더 17줄)를 `core/client-player-records.js`·`core/client-player-runtime.js`·`core/current-allocation-bytes.js` 로 옮겼다. 앞선 소유자 단위(E325·E331)와 달리 이 단위는 원문 부모(`initializePlayerRecords`, E7)가 쓰는 **메모리 장치**(실제 할당 블록과 주소 토큰)를 제공한다.

## 1. 규칙

- **주소 토큰과 블록:** 원문 32비트 주소는 호스트 포인터가 아니라 소유자가 발급하는 식별자이다. 첫 토큰 `0x30000000` 부터 `0x1000` 씩 겹치지 않게 발급하며(소진되면 `Player 물리 주소 소진`), 블록은 DWORD 정렬 저장소로 Player 1620BYTE 또는 공장 120BYTE 만 허용한다(다른 크기는 `Player 실제 할당 크기 오류`). 공장 블록은 미초기화 입력을 `allocationBytes` 장치로 채우고(Player 블록은 채우지 않음) 현재 생존 블록만 주소 이하의 가장 큰 블록 시작을 기준으로 정확한 폭으로 해석한다(없는 주소·요청 폭 초과는 `RangeError`). 해제는 NULL(0)을 허용하고 그 외는 정확한 종류의 생존 블록이어야 한다(`Player 실제 반납 주소/종류 오류`).
- **초기화:** `initialize(alliance)`/대여의 `initialize` 는 `initializePlayerRecords` 를 같은 소유자에 실행하고 정상 복귀에서만 `initialized()` 를 켠다(부분 실패는 도달한 접두를 남기고 완료 표시는 내림). 현재 Core 가 연결돼 이전 Player 영역 안을 가리키는 상태이면 원문 앞선 Core 0 선택을 요구하며 아무 것도 지우지 않고 거부한다. 새 base 는 같은 실제 할당의 Core `players` 슬롯에 즉시 반영된다.
- **단일 세션 대여(`acquire()` → Use):** 한 번에 하나, 살아 있는 동안 두 번째 대여와 소유자의 재할당을 거부한다. `initialize` 는 대여당 한 번, `adoptCurrent` 는 앞선 Native 부모가 초기화를 마친 현재 기록만 빌린다(할당/동맹 재실행 없음). 해제는 명시 `release()`(C++ 소멸자 대응, 중복 호출은 다음 대여의 선점을 풀지 않음)이며 Player/목록 내용을 초기화하지 않는다.
- **공장 목록 연결:** `bindFactory(list, owner)` 는 owner 번호만 캡처하고 Player+0(토큰)/+4(용량)/+8(개수)을 호출마다 현재 기록에서 읽는 getter/setter 로 `SquidPostPopList.bindCurrent` 에 연결한다. `reuseOrBindFactory`(기준선 뒤 추가)는 목록이 이미 같은 Client/행에 연결돼 있으면 재사용하고 아니면 새로 연결한다(다른 소유자/행/독립 데이터가 있는 목록의 거부는 유지). 공장 배열 접근은 용량 0 이면 빈 배열, 용량이 `2^32/4` 를 넘거나 블록 폭을 넘으면 거부한다.
- **그 밖의 칸:** `fortCount()`(005951f8)·`lastLoginPlayer()`(005951fc) DWORD 상자, `alliances()` 81칸 표, `productionDeckMemory()`(재생성하지 않는 Deck 할당 소유자), `bindCoreData(core)`(대여 중·다른 Core 에 두 번째 연결은 거부, 연결 시 초기 빈 뷰 공개), `boundCoreData()`, `records()`/`base()`.
- **공통 시작 `initializeClientPlayerRecords(client)`:** 현재 known 이 0 인 초기 경로만 허용하고, 소유자의 `initialize` 에서 동맹 자식을 실제 `RawPlayerParticipation.setAlliance`(00490ea0)로 실행한다. known 0 에서는 읽지 않는 World 조회 세 개(battle·metamap·localTransport)는 도달하면 `공통 Player 동맹 시작의 미선택 World 장치` 오류로 두어 상수 모드로 통과하지 않게 한다.
- **현재 할당 입력(`currentAllocationBytes`):** 원문은 같은 크기 `malloc` 의 미초기화 힙 내용을 복사해 new 가 초기화하지 않는 바이트의 입력으로 쓴다. 웹은 비결정적 힙을 읽을 수 없고 새 `Uint8Array` 가 0 이므로 **입력을 덮어쓰지 않는다**(웹 적응). 검사는 `allocationBytes` 인자로 명시 입력(0x79 등)을 주입해 흐름을 확인한다.

## 2. 기준선 뒤 변경 판정

- `ClientPlayerRecords`(+7줄, 마지막 `7ba0130b6`): ② 반영 — `ReuseOrBindFactory` 와 `bindCurrent` 의 소유자/행 정체성 인자(`sourceOwner`/`sourceSlot`, 웹 `SquidPostPopList.matchesCurrentSource` 는 E258 에서 이미 준비됨). 나머지 변경 없음.
- `ClientPlayerRuntime`: 기준선 뒤 의미 변경 없음(행 base 는 마지막 커밋 `b6bc3e05`). `CurrentAllocationBytes.h`: 마지막 커밋 `c1b8fbf4`, 웹 적응.
- 웹 변경: 생성자의 둘째 인자 `firstToken`(시험용, 제품 기본 `0x30000000`) — 주소 소진 경계를 100MB 할당 없이 검사하려는 것이다.

## 3. 검사 (`tests/client-player-records.test.mjs` 17건)

- **cpppj 시험 이식 10건:** `ClientPlayerRecordsTests` 6건(현재 공장 헤더와 단일 저장소·대여와 재할당 뒤 현재 뷰 재조회·초기화 실패가 도달한 접두를 남김·Client 소유자 지연 할당·Native 재사용 둘)과 `ClientPlayerRuntimeTests` 4건(공통 시작이 실제 기록·Core 뷰·동맹을 구성·살아 있는 Core 는 해제 전 Core 0 선택 필요·초기화된 저장소 채택은 재초기화하지 않음·할당 실패 뒤 부분 뷰와 채택 거부). 실제 `Client` 는 4-D 조립이라 같은 소유자들과 `PlayerWorldState` 를 가진 최소 장치로 대신한다.
- **웹 계약/변형 확인 보강 7건:** 주소 토큰 발급과 공장 포인터 오류 메시지, 현재 할당 입력 모듈, 재할당 뒤 새 뷰·이전 블록 해제·NULL 공장·용량 한계·큰 개수 DWORD 쓰기, 부분 실패 뒤 재초기화와 완료 표시, 잘못된 해제 종류, 중복 반납, 주소 소진.
- 첫 실행의 실패는 모두 시험 쪽이었다: 마지막 시험이 대여를 반납하지 않고 `bindCoreData` 를 호출했고(대여 중 연결 거부가 올바름), 보강 시험에서 이미 해제된 공장 주소를 "살아 있는 블록"으로 가정했다. 모듈은 고치지 않았다.

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 56개(Records 51·Runtime 5)를 돌려 Records 18개가 남았고, 보강 뒤 **6개가 남았으며 모두 동등**하다: ① `bindCoreData` 의 `core.players = this.records()` 제거(초기 base 0 의 빈 뷰이므로 두 빈 뷰가 구별되지 않음) ② 블록 탐색의 `token > start`(블록이 오름차순으로만 삽입돼 마지막 일치가 최대) ③ `#read` 의 범위 선검사(owner/offset 은 고정 상수 0/4/8 이고 owner 범위는 `bindFactory` 가 먼저 막으며 `#memory` 가 폭을 검사) ④ `#write` 의 읽기 선검사(같은 이유) ⑤ 정렬 검사(블록 시작 주소만 발급하므로 항상 정렬됨) ⑥ `adoptCurrent` 의 `base()` 검사(초기화 완료면 base 가 항상 비영). Runtime 5개 중 3개는 동등: 초기 숨김 9BYTE·`localOwner`/`playerKnown` 현재 조회·World 조회 세 개는 known 0 경로에서 읽히지 않는다.
- **앱 수명:** 아직 Client 가 이 객체를 만들지 않는다(PORT_MAP session 0). 4-D 의 `Client` 조립이 `ClientPlayerRecords`·`ClientCoreDataGlobals` 를 한 번씩 만들고 `bindCoreData` 로 잇는다.
- 3.1절 4-D 는 3/25 이다(A 구획 ClientFortGlobals·ClientCoreDataGlobals·ClientPlayerRecords·ClientPlayerRuntime). 다음: A 구획 `ClientGameplayGlobals`.
