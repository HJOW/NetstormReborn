# webpj 표면 월드(RawSurfaceWorld) — WB04-E299 (2026-11-11, 4-B)

3.1절 4-B 의 **RawSurfaceWorld**(client, 기준선 804줄 = `.cpp` 522 + `.h` 282)를 `core/raw-surface-world.js` 로 옮겼다. 본섬 지면·noIsland 표면·다리와 파생 받침/종유석/연결 조각을 **실제 raw 풀·해시·Graph·Kernel** 위에서 만들고 지우는 월드 연결부이며, 앞 단위에서 옮긴 부품(E1~E298)을 한 월드로 조립한다. 선행: E297(RawProcessHostStorage)·E298(RawSurfaceStorage). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- **구성** `new RawSurfaceWorld(types, frames, shapes, links, terrainType, { storage, adoptPrepared, processKernel, sharedSavedGame, loadingDepth, authority, gameAux })`:
  - 저장소 선택·검사를 **대여 전에** 한다(배열 크기·타입 번호·권한/보조 owner 정체성 불일치는 `storage.generation`/`borrowed` 를 바꾸지 않고 거부). 저장소가 없으면 독립 저장소를 만들고, 준비 대여(`adoptPrepared`)는 실제 저장소가 있어야 한다.
  - 일반 대여는 입력의 표면 종류(155/94/95/157/82)·전투/미션 모드·소유자 색을 저장소 장기 값에 다시 쓰고 모드(fort·battle·challenge)를 기본값으로 둔다. 준비 대여는 이들을 **보존**하고 Editor 모드로 시작한다.
  - 구성 순서(원문 보존): 장부/삭제/모드 상태 → Graph(실제 전체 풀은 소진 재구성 허용, 현재 TYPE·진단·추적·시계·분리 상태 빌림) → 표시 → 후처리(섬/표면 접두) → Unpop/Pop → 종속 Unpop → 건물 공간 → 공장 → 프레임 → 소유자 → 이웃 → 다리 연결 → 섬 표면 접두 → 공통 삭제/가상 삭제 → 표면 수명 → 다리 사건 → Form 분배 → Process Host → 감시 Form → 다리 붕괴. 구성이 도중에 실패하면 자기 대여만 반납한다.
  - Process Host: 외부 Kernel 이 없거나 Storage 가 pin 한 같은 Kernel 이면 **공유 Storage Host**(`storage.processes.bind(runtime)` 의 단일 대여), 다른 Kernel 이면 **독립 Host + SID 소진 회수 대여**(`bindLegacyAllocationReclaim`)이다.
- **적재** `load(seeds)`: 쓰기 전에 전체 입력을 검사한다(지원 타입은 지면·noIsland·다리, 좌표는 1~255 정수, 소유자 0~8 정수, 프레임은 정수·코드 수 미만). 정렬은 지면 → noIsland → 다리, 같은 종류는 y 먼저 x 다음이다(받침 접두가 서/북 이웃을 읽음). 이어 `beginLoad`(Graph 활성·깊이 1) → 각 객체 `create`→`setAssetOwner`→프레임 쓰기→`Pop` → `finishLoad`(전체 Graph 재구성·깊이 0·변경 표시). `finishLoadAfterGraph` 는 캠페인 부모가 이미 Graph 를 돌린 뒤 Graph/풀/해시/spot 을 바꾸지 않고 로딩만 끝낸다. 중복 로드/종료 순서 오류는 상태를 바꾸지 않는다.
- **공개 조작**: `create`/`pop`/`unpop`/`repop`(현재 raw 좌표로 재등록)/`destroy`(거부된 가상 삭제는 변경 표시를 올리지 않음)/`find`/`setSupportOwner`/`setAssetOwner`/`objects`(자산 TYPE 70 이상의 살아 있는 슬롯을 raw 에서 읽음)/`takeChanged`(한 번 소비)/`setDisplay`/`frameTime`/`observed`(독립 월드가 실제 소비자 대신 센 다리 효과·walker 낙하·삭제 요청 수).
- **프레임**: `prepareFrame` 은 장치(Client Kernel/시각)가 있으면 Client 시각을 읽고 되쓰지 않으며, 없으면 스냅샷 시각을 쓴다(역행·비유한 시각은 `RangeError`). `runFrame` 은 다리 붕괴 스캔 뒤 정지가 아니면 Kernel 을 한 번 돌린다. `scanBridgeDecay`/`decayBridgeCell`/`bindBridgeDecay` 는 장기 스캔 커서·주기와 현재 권한/편집기 관문을 쓴다(장치가 없을 때 독립 월드는 관찰 횟수, runtime 이 있으면 명시 오류).
- **연결(한 번만, 첫 객체 전)**: `configureInstallModes`(모드·표면 종류·Editor), `bindRuntime`(필수 경계 9가지 중 하나라도 없으면 `TypeError`, 설치 뒤 `loadingDepth` 통지), `bindDeletion`(보상·선택·효과·파생 경계가 있어야 하며 기존 표면 분배기가 쥔 proxy 를 유지한 채 공통 삭제 몸체를 교체), `bindContainedUnpop`(Session 엔진, 다른 풀 거부)·`bindSquidTransmission`(아래). 늦은 설치·중복 설치·로딩 중 설치는 거부한다.
- **Process 증빙/종료**: `clearProcesses`/`drainProcesses`(Clear 반복)/`clearProcessesForState`→`returnProcessBindingsForState`(Clear 증빙이 있어야 현재 대여를 반납, 한 번만)/`processBindingsReturned`/`drainCurrentProcessBindingsForShutdown`. `dispose()` 는 자기 프로세스 반납 → 처리기 대여 → 회수 대여 → 저장소 대여 순서이며 멱등이다(웹에는 소멸자가 없다).
- **공유 로딩 깊이(3.6절 ③ 4-B 반영, E261 `RawIslandPostPop` sharedLoadingDepth)**: `loadingDepth: { value }` 상자를 주면 월드와 섬 접두가 Template/Core 의 005c89b8 같은 상자를 매 등록마다 다시 읽고(`setLoadingDepth`도 같은 상자에 씀) 생성 때 덮어쓰지 않는다. 주지 않으면 기존 초기값 1 의 지역 상자이다.

## 2. 옮기지 않은 것과 차이

- **객체 송신기**: C++ 월드는 장기 `RawSquidTransmission` writer 로 모든 일반 Pop 을 직렬화한다. 웹에는 아직 없고(4-C 패킷 단위) 단독 월드의 방송 결과는 어디서도 읽히지 않으므로, 일반 Pop 의 송신은 `bindSquidTransmission((sid, flags) => …)` 로 연결한 함수를 부른다(기본은 쓰기 없음). 4-C 에서 `RawSquidTransmission.pop` 을 이 자리에 연결한다. `squidPacket`/`squidConnected`/`squidSend`(LAN 전용)는 옮기지 않았다.
- **`Report()`**: 개발자 진단 문자열(`--dump-world`)은 옮기지 않았다. 같은 값은 `core()`·`objects()`·`frameTime`·`observed` 로 읽는다(시험은 같은 요약을 직접 계산). `--dump-world` 어댑터는 GameWorld 판정(3.1절 다음 항목)으로 이월한다.
- 실제 자산에서 월드를 만드는 도우미, Session 조립, CD(10.72) 분기, 길 찾기 작업 배열은 해당 단위 몫이다(E298 문서의 같은 범위).
- **기준선 뒤 cpppj 변경(+780줄, 판정 커밋 `14fd017c`)**: 월드가 Display/PostPop/Pop/Unpop/Factory/Frame/Owner/Neighbors/Connect/Surface/BaseDestroy/VirtualDestroy/Common/Lifecycle/Events/Dependencies 를 Storage 의 **장기 소유자**에서 빌리도록 바뀌었고, 자산별 Bindings 15종·Runtime pin·직렬화 전역 선택·`SynchronizePeer`/`TransmitDeletion`/`AttachBaseAsset`/`BindMetaRefresh`가 추가됐다. 독립 typed 경로(이 단위가 옮긴 것)는 지역 소유로 남아 있어 결과가 같으므로(3.6절 ①) 새 구조는 Session 조립 단위(**3.6절 ③ 4-D**)에서 반영한다. 단 `sharedLoadingDepth` 는 E261 의 ③ 4-B 항목이라 이 단위에서 **②로 반영**했다.

## 3. 검사 (`tests/raw-surface-world.test.mjs` 37건)

원본 관측 파일이 없는 단위 계약 검사이다. cpppj `RawSurfaceWorldTests.cpp` 의 장면(정지 중 Regular 유지와 재개 뒤 끝 칸 교체·단단한 다리와 예약이 남은 미션 종료·표시 억제와 장치 교체·잘못된 적재 거부·Runtime 공유/불완전·늦은 설치·캠페인 부모 Graph 보존·실제 슬롯 스캔 붕괴/균열/서버 0 수명 삭제·즉시 권한/편집기 관문)을 같은 입력으로 옮기고, 웹 고유 계약(저장소 대여·반납, 생성 인자의 대여 전 거부, 준비 대여 보존, 독립 Kernel 회수, 공유 Host 의 Client 시각/Clear 증빙, 종료 시 자기 프로세스 반납, 공유 로딩 깊이, 저장 게임 관문, 설치 모드, 공통 삭제 소비자 교체, 감시 Form 통지, 종속 Unpop 엔진, 송신기 flags, Graph 추적)을 더했다. 실제 `bridge.type` 프레임 코드 한 행(`cpppj/tests/fixtures/bridgedecay-x86.tsv`)과 합성 SHP 셀만 입력이며 기대 관계식은 모듈이 낸 결과의 관계이다.

**변형 확인** (`mutate.mjs`, 3회): 첫 변형 272개 중 148개가 살아남아 시험 37건으로 보강한 뒤 다시 돌렸고(84개), 한 번 더 보강해 **74개**(처음 272개 중; 새 공유 로딩 깊이 변형 5개는 모두 잡혔다)만 남았다. 남은 변형 74개를 분류한다:

- **동등/중복 방어(22)** — 앞선 검사와 같은 오류를 내는 중복 검증: 월드 크기 검사의 frames/shapes(저장소 생성자가 같은 `TypeError`), 구성 안의 입력 크기 3종·빌드 타입 번호(선행 검사가 이미 거부), 적재의 유한 좌표 2종(NaN 은 정수 검사, ∞는 상한 검사가 거부), 설치 모드/runtime/삭제의 슬롯 검사와 loaded 조건(웹 풀은 할당마다 `freeCount` 가 줄어 중복), runtime 필수 경계의 `assets.selected`/`destroy`(하위 `setAssets` 가 같은 `TypeError` 를 상태 변경 전에 던짐), `finishLoad` 순서(뒤따르는 `finishLoadAfterGraph` 가 같은 메시지), `dispose`/`returnProcessBindingsForState` 중복과 `drainProcesses` 반복(멱등·한 번에 비워짐), 준비 시각 되쓰기(장치 시각은 같은 값), 스캔의 `server`(스캔 실행기는 읽지 않고 붕괴 몸체가 권한 owner 를 직접 읽음), 프레임 쓰기 바이트(프레임 < 2^24), `unpop flags`(0x2000 대체 표시 경로는 파생 VT 에서만 다름).
- **변경 표시를 올리는 독립 경로가 여럿(11)** — 표시 sink·생성/프레임/소유자 연결·섬 표면 갱신·사건 생성/소유자/통지·받침 소유자·Pop 결과·붕괴 통지 중 하나를 빼도 같은 동작이 다른 경로로 변경 표시를 올린다(공개 조작이 `takeChanged` 를 올리는지는 동작마다 직접 확인함).
- **일관된 장면에서 관찰되지 않는 진단·현재 표 빌림(21)** — Graph 복구 정책/시계/분리 상태/진단/현재 TYPE, 후처리 현재 TYPE/마나 진단, Pop·종속 Unpop 진단(metamap·보스), 건물 섬/다리 TYPE·통지 진단, 공장·소유자 진단, 이웃 현재 TYPE, 섬 받침 이전 TYPE, 낙하 예약 진단, 붕괴 진단, 사건 다리 TYPE, 표시 초기 카메라: 정상 장면은 진단을 호출하지 않거나 같은 표를 읽는다. 각 모듈 시험이 소유한 연결이며 월드에서는 **4-D Tutorial1 Session 조립 검사**에서 다시 본다.
- **이 장면에 없는 층(15)** — 건물 공간 Pop/Unpop(건물 genus 객체 없음), Form 층(공장 Unpop·Form 깊이/부착·GameAux·종속 detach·레거시 Host 공장·감시 알림/fallback/생성/Unpop·runtime 깊이)과 레거시 회수 flags: 시험 장면이 Form/건물을 직접 Pop 하지 않는다. 4-D 에서 Form·건물 장면과 함께 확인한다.
- **도달 불가 분기(5)** — 붕괴 소리 장치 미연결 오류(대피 장치 오류가 항상 먼저), 기본 삭제 전달 guard, 공통 삭제 선택 getter/기록 둘 중 하나(두 경로가 같은 값을 줌), 교체 삭제의 Graph(11→11 로 같은 결과).

시험을 쓰며 찾은 실제 결함은 구성 중 `RawBridgeDecay` 에 `SquidHash` 대신 `hash.view(0)` 을 넘긴 한 곳(붕괴 스캔이 칸마다 예외를 삼켜 다리 수명이 갱신되지 않던 것)이다.

## 4. 남은 경계

- 3.1절 4-B 는 30/33 이다(남은 것: GameWorld·GameWorldInput 판정, client RawFortWorldLoadBinding, 4-B 적재 통합 시험).
- 4-C: `bindSquidTransmission` 에 `RawSquidTransmission.pop` 을 연결한다. 4-D: 기준선 뒤 장기 소유자 구조(위 2절)와 위 변형 확인에서 관찰하지 못한 Form/건물/진단 연결을 Tutorial1 Session 검사로 닫는다.
