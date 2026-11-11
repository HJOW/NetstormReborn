// Client 의 실제 배열을 Session 보다 오래 보관하고 같은 물리를 두 월드에 중복 설치하지 않는 장기 저장소이다(cpppj `client/RawSurfaceStorage.cpp` + `RawSurfaceWorld.h` 의 RawSurfaceStorage 선언, 10.78 전용).
// 사용: const storage = new RawSurfaceStorage(types, frames, shapes, { source, authority, gameAux }); const lease = storage.acquire(); … lease.release();
//   - 물리: SID 풀(32,768)·공간 해시·spot 65,536BYTE·Graph 기록/스택·집계 3표·섬 3,204BYTE·청크 지도·State SID 목록·사제 owner 9 DWORD·TYPE/프레임/셰이프/코드 배열은 대여마다 교체되지 않는 같은 객체이다.
//   - acquire(): 중복 대여를 먼저 거부한 뒤 일반 설치의 fresh 입력만 호환한다(GameAux 일반 월드 재설정·BoardTarget 실패점·권한 true/true·풀/해시/spot 재초기화(두 번째 세대부터)·사제 owner 0·Graph 전체 0 후 작은 reset·State SID 목록·집계 3표·섬/청크의 fresh 입력).
//     acquirePrepared(): 앞선 명시 정리가 끝난 물리를 초기화 없이 대여하고 같은 관문에서 중복을 거부한다. 반환 `Use.release()` 는 대여만 풀고(C++ ~Use) 물리는 그대로 둔다.
//   - 장기 장치(Client 수명): 표시 진단·상세 owner, 할당 소진 진단, Graph 시계/추적, checking 뷰는 첫 World 전에 한 번 연결하며 World 마다 교체하지 않는다. 주변 바람은 Client 난수를 직접 소비한다.
// C++ 의 `shared_ptr`/주소 동일성은 웹에서 객체 동일성이며, 대여가 "새 값 대입"인 곳은 같은 객체를 제자리에서 초기화해 참조를 유지한다. 웹에는 소멸자가 없으므로 `Use.release()` 와 `dispose()` 가 명시 반납이다.
// 아직 옮기지 않은 것: 길 찾기 작업 배열(RawPathSearchStorage, 6-P RawPathSearch), SID 소진 복구 할당 연결(SidPool.BindAllocation — 웹 풀은 소진을 거부만 한다, 3.6절 ③ 4-D), 실제 자산에서 만드는 MakeRawSurfaceStorage(Client 자산 단위).
// CD(10.72)의 시작 값/분기는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E298: 4-B RawSurfaceStorage 이식)
import { ChunkMap } from '../assets/territory.js';
import { BoardTargetState } from './raw-board-target.js';
import { BridgeConnectState } from './raw-bridge-connect.js';
import { createAmbientWindState, stepAmbientWind } from './raw-ambient-wind.js';
import { GameAuxState } from './game-aux-state.js';
import { RawAuthorityState } from './raw-authority-state.js';
import { RawGraphStorage, resetRawGraphTable } from './raw-graph.js';
import { createRawIslandStorage, resetRawIslandRecords } from './raw-island-runtime.js';
import { RawProcessHostStorage } from './raw-process-host-storage.js';
import { RawStateSidLists } from './raw-state-sid-lists.js';
import { SidPool } from './sid-pool.js';
import { RawTotalMadeStorage } from './squid-postpop-state.js';
import { SquidHash } from './squid-hash.js';
import { SquidOwnerMode } from './squid-owner.js';
import { buildSquidFrameAssertionDetail } from './squid-display.js';
import { finishSquidPoolReset } from './raw-squid-pool-reset.js';

// 기존 실제 월드의 SID 풀 크기(다음 대여에서 재확보하지 않음)와 원본 256×256 spot 배열의 바이트 수, 사제 owner 상태 DWORD 수(플레이어 8 + 1)이다. (2026-11-11 WB04-E298)
const POOL_CAPACITY = 32768;
const SPOT_CELLS = 65536;
const PRIEST_OWNER_COUNT = 9;

// 증빙 객체를 이 모듈 밖에서 만들지 못하게 하는 비공개 표식이다(C++ 의 private Use 생성자). (2026-11-11 WB04-E298)
const TOKEN = Symbol('RawSurfaceStorage lease');

// 대여 중에는 두 번째 월드가 같은 풀을 초기화하지 못하게 한다. 반납(release)은 월드의 모든 모듈을 반납한 뒤 대여만 해제하며 Kernel 을 정리하지 않는다(멱등). (2026-11-11 WB04-E298)
export class SurfaceUse {
    // 반납하면 null 이 되는 대여 소유 저장소이다.
    #storage;

    constructor(token, storage) {
        if (token !== TOKEN) throw new TypeError('대여는 RawSurfaceStorage.acquire 만 발급합니다.');
        this.#storage = storage;
    }

    // 월드의 모든 모듈을 반납한 뒤 대여만 해제한다(C++ ~Use). 다른 대여가 이미 있으면 영향을 주지 않는다. (2026-11-11 WB04-E298)
    release() {
        if (this.#storage && this.#storage._lease() === this) this.#storage._setLease(null);
        this.#storage = null;
    }
}

// 같은 물리를 Session 보다 오래 보관하는 장기 저장소이다. (2026-11-11 WB04-E298)
export class RawSurfaceStorage {
    // 권한(서버/보스)과 게임 보조(TYPE 전역) 장기 owner 이다(주면 그 객체, 없으면 새로 만듦).
    #authority;
    #gameAux;
    // 대여/World 반납 뒤에도 같은 현재 Client 진단 DWORD 를 읽는 checking 뷰(기본 0), 표시 진단 상세 owner(`{ value: string|null }`)와 현재 표시 진단 장치이다.
    #checking = () => 0;
    #displayDetail = null;
    #displayAssertion = null;
    // Client 수명 동안 현재 게임 시각을 읽는 장치, 현재 로그 장치, CD 소진 전용 객체 덤프/Tell 장치(웹 풀에는 연결이 없어 보관만 한다)이다.
    #graphClock = null;
    #graphTrace = null;
    #allocationDump = null;
    #allocationTell = null;
    // 장기 process owner(첫 사용에 지연 생성), 최초 자산 정체성, 단일 대여와 호스트 관측 세대 번호이다.
    #processes = null;
    #source;
    #lease = null;
    #generation = 0;

    // types/frames/shapes 는 같은 길이의 입력 표이며 복사해 보관한다. authority/gameAux 는 Client 와 공유하는 장기 owner 이다. (2026-11-11 WB04-E298)
    constructor(types, frames, shapes, { source = null, authority = null, gameAux = null } = {}) {
        if (!Array.isArray(types) || !Array.isArray(frames) || !Array.isArray(shapes) || frames.length !== types.length || shapes.length !== types.length) {
            throw new TypeError('실제 월드 저장소의 자산 배열 크기 불일치');
        }
        this.#authority = authority ?? new RawAuthorityState();
        this.#gameAux = gameAux ?? new GameAuxState();
        this.#source = source;
        this.types = types.map((type) => ({ ...type }));
        this.frames = frames.slice();
        this.shapes = shapes.slice();
        // 같은 타입의 실제 프레임 코드만 최초 보관한다.
        this.codes = this.frames.map((table) => table.codes.slice());
        const authorityRef = this.#authority;
        const serverOwner = { get value() { return authorityRef.server; }, set value(next) { authorityRef.server = next; } };
        this.pool = new SidPool(POOL_CAPACITY, { server: true, serverOwner });
        this.hash = new SquidHash();
        this.spots = new Uint8Array(SPOT_CELLS);
        // 005c8458 의 최초 BSS 0 이며 모든 World 가 같은 금지 DWORD 를 빌린다.
        this.disallowSquidAllocation = { value: 0 };
        // 545370/545374 실패점을 Pool 과 같은 장기 수명에 두고 Board/AI 가 직접 빌린다.
        this.boardTargetState = new BoardTargetState();
        // 기록 255행과 flood 4096 DWORD 만 장기 보유한다. 실행기/TYPE snapshot/복구 정책은 World 지역이다.
        this.graph = new RawGraphStorage();
        // 실제 정적 SID 8목록/count 와 등록 20칸을 World/Session 전후에 보존한다.
        this.sidLists = new RawStateSidLists();
        // 현재 로컬/누적 로컬/전역 256 DWORD 만 장기 보유하며 ownerFactories/로딩 정책은 World 지역이다.
        this.totalMade = new RawTotalMadeStorage();
        // 같은 3,204BYTE 섬과 mode/크기/256 청크를 Session 전후에도 유지한다. 실행기/효과는 포함하지 않는다.
        this.islands = createRawIslandStorage();
        this.chunks = new ChunkMap();
        // 00595428..00595448 의 9 DWORD 최초 BSS 0 이다. 실제 사제 SID 목록/count 는 sidLists 에 있고 지갑은 포함하지 않는다.
        this.priestOwnerState = new Uint32Array(PRIEST_OWNER_COUNT);
        // 미니맵과 현재 월드가 같은 실제 세 모드 값을 읽는다.
        this.ownerMode = new SquidOwnerMode();
        // 생산 UI 와 현재 월드가 같은 실제 표면 종류 전역을 읽는다.
        this.links = new BridgeConnectState();
        // 0052f96c/970·005453a8/b8 의 같은 현재 스캔 범위·커서·기한을 World 반납 뒤에도 유지한다: PATCH 의 초기 first/last 는 실제 PE DWORD -1 이며 cursor/기한만 BSS 0 이다.
        this.bridgeDecayScan = { first: -1, last: -1, cursor: 0, next: 0 };
        // 0052f968 의 원문 최초 float 10 이며 Native 정리는 이 현재 값을 읽고 덮어쓰지 않는다.
        this.bridgeDecayPeriod = 10;
        // 실제 PE 최초 0/1/0 이며 World acquire/반납에서 재초기화하지 않는 Client 수명이다.
        this.wind = createAmbientWindState();
        // Pool 생성자가 이미 실행한 하위 Reset 뒤 원문 시작의 기록 80 DWORD·Y/X 꼬리를 한 번 잇는다.
        finishSquidPoolReset(this.pool, this.boardTargetState.invalid);
        // 기존 10.78 표면/초기 생산 경계의 실제 종류이다. 월드 설치는 자기 판본의 실제 값을 직접 쓴다.
        this.links.bridgeType = 82;
        this.links.islandType = 94;
        this.links.stalagType = 95;
        this.links.connectorType = 155;
        this.links.noIslandType = 157;
    }

    // 대여 소유권을 읽고 쓰는 내부 접근이다(SurfaceUse 만 호출). (2026-11-11 WB04-E298)
    _lease() {
        return this.#lease;
    }

    _setLease(value) {
        this.#lease = value;
    }

    // Client 전역/Session 과 같은 실제 서버·권한 소유자의 정체성이다. (2026-11-11 WB04-E298)
    get authority() {
        return this.#authority;
    }

    // 같은 Gameplay/Host 가 pin 하는 실제 게임 보조 owner 이다. (2026-11-11 WB04-E298)
    get gameAux() {
        return this.#gameAux;
    }

    // 모든 실제 타입 소비자의 현재 DWORD(gameAux.geyserType)이다. 대여/반납으로 타입 주소를 교체하지 않는다. (2026-11-11 WB04-E298)
    get geyserType() {
        return this.#gameAux.geyserType;
    }

    set geyserType(value) {
        this.#gameAux.geyserType = value;
    }

    // Client 자산 정체성이다(독립 raw 검사에서는 null). (2026-11-11 WB04-E298)
    get source() {
        return this.#source;
    }

    // 같은 Storage 의 장기 Host/상태와 현재 World 대여를 제공한다(첫 사용에 만들고 이후 같은 객체). (2026-11-11 WB04-E298)
    get processes() {
        if (!this.#processes) this.#processes = new RawProcessHostStorage(this.pool, this.types, this.#authority, this.#gameAux);
        return this.#processes;
    }

    // 현재 실제 월드가 물리를 빌렸는지 읽는다. (2026-11-11 WB04-E298)
    get borrowed() {
        return this.#lease !== null;
    }

    // 성공한 월드 대여 횟수이며 원본 게임 값으로 쓰지 않는다(호스트 관측용). (2026-11-11 WB04-E298)
    get generation() {
        return this.#generation;
    }

    // 비소유 현재 뷰(함수)를 보관한다: 호출 위치에서 현재 owner 의 DWORD 를 읽게 한다. 독립 저장소 기본값은 0 이다. (2026-11-11 WB04-E298)
    bindChecking(checking) {
        if (typeof checking !== 'function') throw new TypeError('checking 은 현재 DWORD 를 읽는 함수여야 합니다.');
        this.#checking = checking;
    }

    // 호출 위치에서 현재 owner 를 읽을 수 있는 같은 뷰이다. (2026-11-11 WB04-E298)
    checking() {
        return this.#checking();
    }

    // 첫 World 이전에 같은 Client 상세 owner(`{ value }`)와 표시 진단 장치를 연결한다. 짧은 World/Session 포인터는 보관하지 않는다. (2026-11-11 WB04-E298)
    bindDisplayDiagnostics(detail, assertion) {
        if (!detail || typeof assertion !== 'function') throw new TypeError('표시 진단은 상세 owner 와 진단 함수가 필요합니다.');
        this.#displayDetail = detail;
        this.#displayAssertion = assertion;
    }

    // 00436d20 의 문구 복사를 같은 Client 상세 owner 에 연결한다. 장치가 없는 독립 저장소는 선택 시 명시 실패한다. (2026-11-11 WB04-E298)
    recordAssertionDetail(text) {
        if (!this.#displayDetail) throw new Error('오류 상세 Client owner 미연결');
        this.#displayDetail.value = String(text);
    }

    // 43d40 의 현재 타입/shape 와 세 인자를 같은 Client 상세 owner 에 기록한다. (2026-11-11 WB04-E298)
    recordDisplayFrameError(type, frame, abstract) {
        if (!this.#displayDetail) throw new Error('표시 오류 상세 Client owner 미연결');
        this.#displayDetail.value = buildSquidFrameAssertionDetail(this.types, this.shapes, type, frame, abstract);
    }

    // 진단을 선택한 지점만 장치를 요구하며 복귀 뒤 다음 TYPE 조회를 막지 않는다. (2026-11-11 WB04-E298)
    assertDisplay(expression, file, line) {
        if (!this.#displayAssertion) throw new Error('표시 원문 진단 장치 미연결');
        this.#displayAssertion(expression, file, line);
    }

    // 장기 물리는 현재 Client 장치만 보관하며 실제 소진 전에는 덤프/대화상자를 실행하지 않는다. (2026-11-11 WB04-E298)
    bindAllocationDiagnostics(dump, tell) {
        if (typeof dump !== 'function' || typeof tell !== 'function') throw new TypeError('할당 진단은 덤프와 Tell 함수가 필요합니다.');
        this.#allocationDump = dump;
        this.#allocationTell = tell;
    }

    // 보관된 소진 진단 장치를 읽는다(웹 풀 연결 전까지 소비자는 4-D). (2026-11-11 WB04-E298)
    get allocationDiagnostics() {
        return { dump: this.#allocationDump, tell: this.#allocationTell };
    }

    // Client 의 현재 로그 서비스를 장기 owner 에 보관한다. World 대여마다 다른 Session 포인터로 교체하지 않는다. (2026-11-11 WB04-E298)
    bindGraphTrace(trace) {
        if (typeof trace !== 'function') throw new TypeError('Graph 추적은 함수여야 합니다.');
        this.#graphTrace = trace;
    }

    // 독립 저장소는 기존 무출력 계약을 유지하고 실제 Client 는 필터/FILE writer 의 원문 계약을 적용한다. (2026-11-11 WB04-E298)
    traceGraph(level, text) {
        if (this.#graphTrace) this.#graphTrace(level, text);
    }

    // 삭제 Graph 의 두 시각 조회가 실제 현재 Client 시계를 읽게 한다. (2026-11-11 WB04-E298)
    bindGraphClock(clock) {
        if (typeof clock !== 'function') throw new TypeError('Graph 시계는 함수여야 합니다.');
        this.#graphClock = clock;
    }

    // 독립 저장소에는 외부 시계 효과가 없다(0). 실제 Client 는 GameSeconds 를 호출한다. (2026-11-11 WB04-E298)
    readGraphClock() {
        return this.#graphClock ? this.#graphClock() : 0;
    }

    // 각도 부모는 같은 현재 checking 과 같은 실제 Client 원문 진단 장치를 값 뷰로 보관한다. (2026-11-11 WB04-E298)
    angleDiagnostics() {
        return { checking: () => this.#checking(), assertion: (expression, file, line) => this.assertDisplay(expression, file, line) };
    }

    // 매 프레임 동일 게임 난수를 직접 소비하며 Session 또는 별도 난수 사본을 만들지 않는다. (2026-11-11 WB04-E298)
    updateAmbientWind(random) {
        stepAmbientWind(this.wind, random, this.angleDiagnostics());
    }

    // 일반 설치의 fresh 입력을 같은 객체 위에서 호환한다(C++ 의 값 대입). 대여 거부 관문 뒤에만 호출한다. (2026-11-11 WB04-E298)
    #freshGeneralInputs() {
        this.#gameAux.resetForGeneralWorld();
        // 일반 새 World 의 기존 실패점 기본값만 같은 객체에서 호환한다(prepared/반납에는 이 쓰기가 없다).
        this.boardTargetState.invalid.x = -10000;
        this.boardTargetState.invalid.y = -10000;
        // 일반 설치의 기존 fresh 상태이다.
        this.#authority.server = true;
        this.#authority.boss = true;
        // 두 번째 세대부터 풀·해시·spot 을 같은 배열에서 다시 초기화한다.
        if (this.#generation) {
            this.pool.reset(0);
            this.hash.reset();
            this.spots.fill(0);
        }
        // 기존 일반 Session 지역 9 DWORD 의 fresh 0 호환만 유지한다(Native 00492240 의 지정 1..8 쓰기와 다른 부모).
        this.priestOwnerState.fill(0);
        // 기존 일반 World 의 fresh 지역 Graph 의미를 같은 객체에서 유지한다(전체 0 뒤 작은 reset; Native 시작 00462eb0/부분 cleanup 의 실행으로 세지 않음).
        for (const record of this.graph.records) {
            record.surfaces = 0;
            record.inUse = 0;
            record.reserved = 0;
        }
        this.graph.stack.fill(0);
        resetRawGraphTable(this.graph);
        // 일반 설치만 기존 새 목록 의미를 유지한다.
        this.sidLists.prepareGeneralWorld();
        // 기존 일반 World 의 새 장부 3×256 전체 0 의미만 유지한다(Native 의 각 255칸 부분 reset 과 구별).
        this.totalMade.localCounts.fill(0);
        this.totalMade.localSecondaryCounts.fill(0);
        this.totalMade.globalCounts.fill(0);
        // 기존 일반 Session 의 fresh 입력/Reset/기본 청크 의미를 같은 객체에서 유지한다.
        this.islands.fill(0);
        resetRawIslandRecords(this.islands);
        const fresh = new ChunkMap();
        this.chunks.xLength = fresh.xLength;
        this.chunks.yLength = fresh.yLength;
        this.chunks.mode = 1;
        for (let index = 0; index < this.chunks.entries.length; index += 1) {
            const entry = this.chunks.entries[index];
            const blank = fresh.entries[index];
            entry.island = blank.island;
            entry.side = blank.side;
            entry.record.fill(0);
            entry.ordinal = blank.ordinal;
        }
    }

    // 동시 설치를 거부한 뒤에만 이전 물리를 초기화한다. TYPE/삭제 기록과 배열 주소는 유지한다. (2026-11-11 WB04-E298)
    acquire() {
        if (this.#lease) throw new Error('실제 월드 저장소 중복 대여');
        const use = new SurfaceUse(TOKEN, this);
        this.#freshGeneralInputs();
        this.#lease = use;
        this.#generation += 1;
        return use;
    }

    // 앞선 명시 정리가 끝난 실제 배열을 초기화 없이 대여하며 동시 사용은 같은 관문에서 거부한다. (2026-11-11 WB04-E298)
    acquirePrepared() {
        if (this.#lease) throw new Error('준비된 실제 월드 저장소 중복 대여');
        const use = new SurfaceUse(TOKEN, this);
        this.#lease = use;
        this.#generation += 1;
        return use;
    }

    // 최종 소멸: 장기 process owner 가 있으면 실제 Pool/TYPE 가 살아 있는 동안 최종 소유권을 반납한다. (2026-11-11 WB04-E298)
    dispose() {
        if (this.#processes) this.#processes.dispose();
        this.#processes = null;
        this.#lease = null;
    }
}
