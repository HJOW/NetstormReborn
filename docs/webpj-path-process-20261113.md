# webpj 경로 프로세스 RawPathProcess (WB04-E349, 2026-11-13, 6-P 경로 부품)

3.1절 6-P 첫 항목 "RawPathProcess 362줄(+10)과 RawPathSearch partial 240줄" 중 남은 **RawPathProcess**(기준선 `.h` 118 + `.cpp` 242 = 360줄, 기준선 뒤 `20261109-53 CP` 의 10줄 변경 반영)를 `core/raw-path-process.js` 로 옮겼다. 이동 유닛의 경로 프로세스 8종(텔레포트 23·사제 54·마나 볼트 비행 39·웜 40·벌룬 42·Dude 24·Bulf 41·Walker 38)이 Kernel 프레임·도착·종료 통지를 분배하는 클래스이며, 이미 옮긴 경로 이미지(RawPathRuntime)·종류별 프레임(Walker/Bulf/Balloon)·공통 Halt/도착 부모·정지 프레임 접두·검색(RawPathSearch)·정리(RawPathCleanup)를 같은 풀·해시·spot·Kernel 위에서 한 객체로 잇는다. 그 연결로 RawPathSearch 쪽 이월(`makePathSearchHooks`와 Init/Kernel 통합 시험)과 E159 경로 정리의 한 장면 통합 시험이 함께 닫힌다.

## 1. 규칙

- **RawPathProcessState:** 프로세스가 읽는 원본 전역이다. `clock`(같은 호스트가 쓰는 `SquidProcessState`, 게임 시각·권한)·`observation`(`{ flags, stamp }`)·종류별 프로세스 번호(`teleportProcess` 23·`priestProcess` 54·`flyProcess` 39·`wormProcess` 40·`balloonProcess` 42·`dudeProcess` 24·`walkerProcess` 38·`bulfProcess` 41)와 부모 TYPE(`priestType` 158·`sunWalkerType` 133·`emptyType` 187·`balloonType` 111·`dudeType` 70·`windWalkerType` 137·`rainWalkerType` 156)·`groundTypes`(길 밖 정지 허용 다섯 타입)·`blockingGenus`(도착 칸 겹침 부류 0x10444200)·`windWalkerCounter`(윈드 보행 전체가 공유하는 `{ value }` 상자). `checking` 은 독립 값 또는 `bindChecking(getter)` 로 빌린 현재 DWORD 이며 매번 현재 값을 읽는다. `sharedBlockingGenus`(getter)가 있으면 `currentBlockingGenus()` 가 그 현재 값을 우선한다(기준선 뒤 `20261109-53 CP`).
- **생성과 훅:** `new RawPathProcess(pool, hash, types, frames, shapes, spots, state, hooks, kinds)` — 종류 플래그(`priest·flying·worm·balloon·dude·bulf·walker`)는 경로 이미지 생성자가 서로 배타적으로 거부한다. 세 표의 길이가 같고 spot 이 65,536 바이트이고 필수 훅 아홉(`unpop·repop·pop·halt·arrival·soundId·stepSound·clearBlocked·cleanup`)이 있어야 하며, 훅은 원본 `std::function` 복사처럼 **사본**을 보관한다. `search·retarget·assertion·balloonFlame·bulfRandom·bulfSound·observationStamp` 는 해당 분기를 지날 때만 요구하고, 없으면 빈 효과로 대체하지 않고 오류로 거부한다.
- **attach(host, process, parent, flags, form):** 호스트와 풀이 같아야 하고, 사제는 부모 TYPE 이 `priestType`, Dude 는 `dudeType`, 웜은 `sunWalkerType`/`emptyType` 이어야 한다. 비행·벌룬·Walker 는 부착 전에 같은 종류(프로세스 번호) 기존 프로세스를 모두 제거한다(`removeAttachedProcesses`, 48af50). `onAttached()` 는 경로 이미지의 +4/+8 에 Form/부모 번호를 쓴다.
- **initializePath (Init +0x20):** 비행/벌룬은 비행 Init(벌룬은 순항 44·현재 고도는 부모 +30 의 BYTE)만 한다. 그 밖에는 시작 칸의 표면 SID 가 0 이면 아무것도 하지 않고(검색 훅이 없어도 오류가 아니다) 반환하며, 있으면 검색 훅 미연결을 오류로 멈춘 뒤 부모가 void 이면 시작점으로 `pop(…, 0)` 을 먼저 하고 검색(Bulf/Walker 는 flags 를 1 로 고정) → `current = 0` 이다.
- **prepare(speedOverride, speed, turningSpeed):** 속도나 회전 속도가 0 이면 거부, override 가 0 이 아니면(음수 포함) 그 속도를 쓰고 0 이면 타입 속도를 쓴다. 시각은 현재 게임 시각 하나로 넘긴다.
- **runFrame:** Walker/Bulf/Balloon 은 전용 프레임 모듈에 위임하고(`immobile`=부모 genus 0x10000 이면서 Regular 0x5105 가 있는지, `halt`, `step`, 도착, `unpop`/`pop`), 그 밖에는 사제/웜/Dude 만 얼어붙음 검사를 하고 → 완료/Halt 면 반환 → 비행은 다음 프레임 예약(20ms) 경계 → Step → 완료면 도착 → 텔레포트/비행은 좌표가 바뀔 때만 `unpop(0)`/`pop(0x40)`, 사제/웜/Dude 는 길 밖 정지 → 부모 Free 면 반환 → SHP 발자국 거리와 45도 배수 각도로 같은 방향 글자 구간의 다음 프레임을 고르고 바뀔 때만 `unpop(0)` → 프레임 DWORD 쓰기 → `pop(0x40)` 이다.
- **길 밖 정지:** 부모 TYPE 이 `groundTypes` 의 다섯 칸 안이고 올림(+0.9999)/내림 칸의 spot 이 둘 다 걸을 수 없으며 권한(또는 로컬 Form 번호 범위)이 있고 주변 3×3 의 표면 아닌 후보 중 `gDAIS`(0x400000)가 없으면 자신을 지우고 참을 반환한다. 칸 좌표가 지도 밖이거나 비유한이면 호스트 오류로 거부한다.
- **arrive:** 사제/웜/Dude 는 먼저 정지 프레임 접두(`RawPathAnimation`, Unpop(0) → 프레임 → Repop(0x40))를 한 뒤, 공통 도착 부모(`runPathArrivalParent`)에 현재 이미지의 부모·Step·Unpop/Pop(0)·robust·도착 막힘·Halt·완료·감시 토큰·가상 도착(대상 두 개)·1000/1001/1002 종단을 연결한다. 겹침 정리 훅은 삭제 뒤에도 현재 부모 좌표·TYPE·소유자(+34)를 다시 읽어 받는다.
- **Halt:** 공통 Halt 부모(`runPathHaltParent`)에 연결한다. 부모 +40 의 0x20 비트면 계속(10)을 종료(11)로 바꾸고, 권한이 있고 부모가 운반 계열(genus 0x30000)이며 목표가 있으면 재탐색 훅을 부른다(훅이 없으면 오류).
- **onFormDestroy(flags):** 권한이 있고 부모가 Dead 가 아니면 `unpop(0)` → `repop(0)` 의 공간 접두를 하고 `cleanup(runtime, parent, flags)` 로 정리 하위 몸체(RawPathCleanup)에 넘긴다.
- **자기 삭제:** 원본의 stack deadFlag 는 소멸자에서 켜진다. Kernel 이 삭제 때 부르는 `destroy()` 가 `{ value }` 감시 상자를 꺼서, 가상 효과가 자신을 지운 뒤에는 상자와 보존한 훅 사본만 읽는다(삭제된 객체의 필드를 다시 읽지 않는다).
- **makePathSearchHooks(pool, search, hooks):** (`raw-path-search.js`) 같은 풀의 검색기를 훅 묶음의 `search` 에 연결한 **새 묶음**을 돌려준다. 풀이 다르면 거부하고 원래 묶음은 바꾸지 않는다.

## 2. 웹 범위와 기준선 뒤 변경

- 10.72(CD) 분기(실제 프레임 BYTE·Robust 호스트 검사·관찰 시계 인자·`CD` 의 허용 타입 네 칸)는 옮기지 않았다(웹은 10.78 만, 풀 판본을 확인해 거부한다). 관찰 시계 훅(`observationStamp`)은 호출 순서만 유지하고 10.78 Step 이 값을 쓰지 않는다.
- 기준선 뒤 변경(3.6절 ①): `20261109-53 CP` 의 `RawPathProcessState::sharedBlockingGenus`(장기 마스크를 읽는 포인터)와 `state_.clock.Boss()`(공유 권한 소유자) 반영이다. 웹은 포인터를 getter 로, `Boss()` 는 `SquidProcessState.boss` 로 옮겼다(PORT_MAP base `5f4b9d30`).
- 위치는 `{ x, y }`(float32), SID 는 숫자, 표는 TYPE `{ flags2, … }`·`FrameCodeTable`·SHP `{ frames: [{ walkStepX, walkStepY }] }` 이다. 풀 슬롯은 `pool.physical()` 로 사본 없이 읽고 쓰기는 `pool.allocated()` 로만 한다.
- 이 모듈은 DOM 을 쓰지 않는다. 앱 수명 연결(부모 가상 Halt/도착/소리를 실제 월드에 잇는 훅 묶음, `makePathSearchHooks` 연결)은 4-D/6 단계 조립 몫이다.

## 3. 검사

- **`tests/raw-path-process.test.mjs` 35건:** 원본 독립 관측 — `pathruntime-x86.tsv` 의 `originals` frame 60행(텔레포트/사제 × 현재 단계 × 시각 × 각도 입력의 Kernel RunFrame 뒤 부모 x/y/frame·경로 current/angle/finished·가상 요청 순서)과 arrival 12행(사제 여부 × 반환 1000/1001/1002 × Halt 10/11, 1001 자기 삭제 뒤 `this` 를 읽지 않고 1002 는 Kernel 에 남음), `flypath-x86.tsv` frame 63행(비행의 20ms 예약·Halt 순서와 다음 프레임 시각 QWORD), `walk-shape-x86.tsv` 1,170행(실제 보호 사제/골렘 SHP 의 모든 논리 프레임), `pathprepare-x86.tsv` originals 24행(가상 Prepare 의 nonzero override/타입 속도 선택별 꼭짓점 시각 QWORD)을 **첫 실행에서 전부 일치**했다. cpppj 시험 이식 — 존재하지 않는 검색 경계와 얼어붙은 사제, Halt checking 3입력의 자기 삭제 뒤 진단 복귀, 도착 4변형(정상/1001·삭제/999)의 진단 줄과 겹침 정리 한 번, 예약 Form 0 의 세 변형. 웹 계약 — 생성자 연결 검사·종류별 프로세스 번호·부착 검사/기존 프로세스 제거(비행·벌룬·Walker)·Prepare 속도 선택(음수 override)·길 밖 정지(허용 타입 목록·gDAIS·권한·칸 좌표 경계)·사제/Dude/웜의 같은 SHP 프레임·같은 도착 접두·종류별 얼어붙음/Halt 연결과 윈드 counter 상자·벌룬 화염 훅·Halt 강제 정지/재탐색 연결·Init 의 flags와 void Pop·종료 통지 세 변형(서버 번호 Form 은 서버 풀 장면)·상태 객체의 checking/마스크.
- **`tests/raw-path-process-search.test.mjs` 3건:** `pathinit-x86.tsv` 의 `originals` 32행(표면 0/존재·void/live·Graph·flags·기존 경로의 가상 Pop 인자·현재 단계·꼭짓점·부모 상태/좌표·거리표/큐 FNV32·방향 3 선행 비트)과 cpppj 시험 2건(시작 표면 0 의 반환·표면이 있으면 실제 BFS 연결 / 검색 꼭짓점을 Kernel 에서 한 단계씩 진행해 1000 도착의 Form 삭제까지 같은 저장소 사용), `makePathSearchHooks` 의 풀 불일치 거부와 원래 묶음 불변. `SearchScene` 은 두 시험이 쓰도록 `tests/path-search-scene.mjs` 로 분리했다.
- **`tests/path-cleanup.test.mjs` +1건:** E159 가 이월한 cpppj `rawPathCleanup_real_process_spatial_prefix_and_shared` — 실제 사제 생성자·SquidPop/postPop 사슬(사제 접두·Carrier·Tail·보호막 조회 두 번)·ProcessHost/Kernel 위에서 경로 프로세스 `onFormDestroy` 가 `unpop(0)` → `repop(0)` 한 뒤 cleanup 훅 → `afterSpatial` 로 건설 helper 를 같은 Host 의 SharedRegular(0x1a2c)로 예약하고 회복 Regular 가 남지 않는다. 이를 위해 시험 장면이 5..11 을 cpppj 처럼 실제 client free list 에서 확보하도록 고쳤다(기존 866행 결과는 그대로).
- 첫 실행의 실패는 모두 시험 구성 쪽이었다: 권한 없음 변형이 로컬 Form 번호 범위(5~14,999) 때문에 계속 이동 허가를 가졌고(서버 번호 Form 은 서버 풀과 송신 경계가 필요), 부모 Dead 를 부착 전에 켜면 패치판이 등록하지 않고 `null` 을 돌려주며, 서버 번호 Form 부착이 `transmitSize != 0` 진단을 낸다(직렬화 크기를 0 이 아닌 값으로 둠). 모듈은 바뀌지 않았다.

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 97개(1차 72개 + 2차 25개)를 `raw-path-process.js` 에 적용해 세 시험 파일(`raw-path-process`·`raw-path-process-search`·`path-cleanup`)로 확인했다. 1차에서 13개가 남았고(앵커 불일치로 건너뛴 5개 포함) 모두 **시험의 공백**이었다. 다음을 보강했다:
  1. 권한 조합(`mayMutate`): 로컬 Form + 권한 없음, 서버 번호 Form + 권한 있음/없음(서버 풀 장면) — `boss ||` 와 Form 범위 두 조건이 따로 검증된다.
  2. 길 밖 정지의 올림/내림 칸 spot(한쪽만 막혀도 정지, 둘 다 걸을 수 있으면 계속), 주변 3×3 의 모서리(우하·좌상) gDAIS, 표면 단계의 gDAIS 는 건너뜀(`skipSurface`; 후보의 위치/발자국이 finder 의 교차 검사를 통과해야 보인다), 정지하지 않을 때 같은 프레임의 Unpop/Pop 이 이어짐.
  3. 텔레포트/비행의 한 축만 바뀐 좌표(x 만·y 만·변화 없음), SHP 발자국 거리와 이동 거리의 동률(엄격 비교, 도착 꼭짓점을 `current + 발자국` float 로 둠).
  4. Halt 동안 켜졌다가 끝나면 꺼지는 감시 토큰, 훅 사본(만든 뒤 원본 훅 객체를 바꿔도 만든 시점의 효과), 관찰 시계 훅 호출 횟수(벌룬 Step 은 부르지 않고 나머지는 Step 마다 한 번), 벌룬·Walker·비행 부착의 기존 프로세스 제거, 음수 override.
  2차에서 1개(`skipSurface` → 0)가 남았고 위 2번의 표면 후보 입력 보강 뒤 잡혔다. **최종 생존 0.**
- 변형하지 않은 방어선: 칸 좌표·SHP 프레임·다음 프레임 범위 진단(`경로 SHP 프레임`·`경로 프레임 글자`·`경로 프레임 구간0`·`경로 다음 프레임`)의 정확한 경계값 일부(글자 15 등), 도착 현재 부모의 `0xffff` 초과 진단, `validAlive` 의 `parent > 0`(원본 관측 입력에서 부모 번호 0 은 나오지 않음).
- 첫 실행에서 원본 관측 1,329행(`pathruntime` 72 + `flypath` 63 + `walk-shape` 1,170 + `pathprepare` 24)과 `pathinit` 32행이 모두 일치했다. 구현 오류로 시험이 실패한 적은 없고, 실패는 모두 시험 입력 구성(서버 번호 Form 조건·부모 Dead 시점·표면 후보 위치·free list)이었다.
- 앱 수명 연결(부모 가상 Halt/도착/소리·정리 훅 묶음을 실제 월드에 연결하고 `makePathSearchHooks` 로 검색을 끼우는 일)와 `RawPathData`(수신/송신 직렬화)·`RawPathStart`(최상위 이동 시작: 클램프·기존 경로 교체·실패 정리 재귀)가 다음이다.
- 3.1절 6-P 첫 항목을 이 단위로 닫는다(RawPathSearch 행도 port). 다음은 **RawPathData**(136줄 +151) → **RawPathStart**(199줄 +2)이다(Start 의 pathstart 512행 시험이 RawPathData 의 `MakePathFormTransmission` 과 client `SelectMovementProcessType` 을 쓰므로 데이터가 먼저).
