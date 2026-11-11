// 본섬 표면·noIsland·다리와 파생 받침을 실제 raw 풀/Graph/Kernel 로 소유하는 새 월드 연결부이다(cpppj `client/RawSurfaceWorld.cpp/.h`, 10.78 전용).
// 사용: const storage = new RawSurfaceStorage(types, frames, shapes); const world = new RawSurfaceWorld(types, frames, shapes, links, terrainType, { storage });
//   world.load(seeds) 는 본섬 → 행 우선 noIsland → 다리 순으로 일반 Pop 을 실행하고 마지막에 전체 Graph 를 재구성한다. 외부 로더는 beginLoad() / create / pop / finishLoad() 사이에서 Create·Pop 을 직접 부른다.
//   core() 는 이 월드의 실제 모듈 묶음(풀·해시·spot·TYPE·프레임·셰이프·장부·소유자 모드·공장·소유자·프레임·Pop·Unpop·후처리·표시·건물 공간·Graph·이웃·삭제·가상 삭제·표면 수명·다리 사건·Form 분배·감시·Host·상태·Kernel·표면 종류)이다.
// 구성 순서(원문 보존): 저장소 선택·검사(대여 전) → 장부/삭제/모드 상태 → Graph → 표시 → 후처리 → Unpop/Pop → 종속 Unpop → 건물 공간 → 공장 → (패치) 전송 → 프레임 → 소유자 → 이웃 → 다리 연결(objects) → 표면 후처리 →
//   공통 삭제 → 가상 삭제 → 표면 수명 → 다리 사건 → Form 분배 → Process Host(공유 Storage Host 대여 또는 독립 Host) → 감시 Form → (패치) 다리 붕괴.
// 모든 실제 물리는 Storage 가 소유하고 월드는 대여(`SurfaceUse`)와 짧은 처리기만 소유한다. 웹에는 소멸자가 없어 `dispose()` 가 반납(Process 대여 반납 → 저장소 대여 반납)이다. 생성자가 예외로 끝나면 대여는 반납된다.
// 아직 연결하지 않은 것: 객체 송신(RawSquidTransmission, 4-C 패킷 단위 — 월드에 squidPacket 방송이 있으면 훅으로 위임), 진단 보고(`Report()` 개발자 출력), 실제 자산에서 만드는 저장소. CD(10.72) 분기는 옮기지 않았다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E299: 4-B RawSurfaceWorld 이식)
import { DEFAULT_BRIDGE_DECAY_MODE } from './bridge-decay.js';
import { RawBridgeConnect } from './raw-bridge-connect.js';
import { RawBridgeDecay, RawBridgeDecayScan } from './raw-bridge-decay.js';
import { RawBridgeEvents, makeBridgeNeighborHooks, makeBridgeRegularHandler } from './raw-bridge-events.js';
import { RawGraph } from './raw-graph.js';
import { RawIslandPostPop } from './raw-island-post-pop.js';
import { RawSquidDestroyDispatch } from './raw-squid-destroy-dispatch.js';
import { RawSquidNeighbors } from './raw-squid-neighbors.js';
import { RawSurfaceLifecycle } from './raw-surface-lifecycle.js';
import { RawSurfaceStorage } from './raw-surface-storage.js';
import { scheduleBridgeFallChecked } from './raw-bridge-fall-parent.js';
import { DependFormHost } from './depend-form-host.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from './squid-destroy.js';
import { SquidDeletionState, SquidDestroyLifecycle } from './squid-destroy-lifecycle.js';
import { SquidDisplay } from './squid-display.js';
import { SquidFactory } from './squid-factory.js';
import { SquidFormDispatch } from './squid-form-dispatch.js';
import { SquidFrame } from './squid-frame.js';
import { makeSquidFrameHooks } from './squid-frame-binding.js';
import { SquidOwner } from './squid-owner.js';
import { SquidPop } from './squid-pop.js';
import { SquidPostPop } from './squid-postpop.js';
import { SquidPostPopState } from './squid-postpop-state.js';
import { SquidProcessHost, createProcessHostRuntime } from './squid-process.js';
import { RawBuildingSpace } from './squid-space.js';
import { SquidUnpop } from './squid-unpop.js';
import { RawSquidContainedUnpop, commonContainedParent, commonContainedPrevious, commonContainedSetPrevious } from './squid-contained-unpop.js';
import { makeAssetOwnerDispatch } from './raw-asset-owner.js';
import { RawContentParentNotice } from './content-parent-notice.js';
import { RAW_POP_RESULT } from './squid-pop.js';

// free/dead/void/contained 상태를 동시에 거르는 기존 마스크, 슬롯 필드 위치(TYPE·상태·프레임 DWORD·좌표 float·소유자 BYTE)이다. (2026-11-11 WB04-E299)
const INACTIVE = 15;
const SLOT_TYPE = 10;
const SLOT_STATE = 11;
const SLOT_X = 14;
const SLOT_Y = 18;
const SLOT_OWNER = 34;
const SLOT_FRAME = 36;

// 첫 자산 TYPE 번호, 플레이어 수(8), 지도 한 변 칸 수, 초기 저장 좌표의 상한(256 미만)이다. (2026-11-11 WB04-E299)
const FIRST_ASSET_TYPE = 70;
const PLAYER_COUNT = 8;
const MAP_SIDE = 256;
const RESERVED_SIDS = 5;

// 비정렬 little endian DWORD 를 읽는다. (2026-11-11 WB04-E299)
function dword(bytes, offset) {
    return (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
}

// float 비트의 읽기 전용 뷰이다. (2026-11-11 WB04-E299)
const BITS = new DataView(new ArrayBuffer(4));

// 비정렬 little endian float32 를 읽는다(좌표 DWORD 의 비트를 그대로 해석한다). (2026-11-11 WB04-E299)
function floatAt(bytes, offset) {
    BITS.setUint32(0, dword(bytes, offset), true);
    return BITS.getFloat32(0, true);
}

// 초기 저장 프레임을 일반 Pop 전에 쓴다(표시/공간은 Pop 에 맡긴다). (2026-11-11 WB04-E299)
function writeFrame(bytes, value) {
    const bits = value >>> 0;
    // 필드 밖의 바이트는 보존한다.
    for (let index = 0; index < 4; index += 1) bytes[SLOT_FRAME + index] = (bits >>> (index * 8)) & 0xff;
}

// 잘못된 판본/크기/타입 요청은 기존 실제 물리를 초기화하기 전에 거부하고 쓸 저장소를 고른다. (2026-11-11 WB04-E299)
function selectStorage(storage, types, frames, shapes, links, terrain, prepared, authority, gameAux) {
    if (prepared && !storage) throw new TypeError('준비된 표면 대여의 실제 PATCH 저장소 누락');
    if (types.length !== frames.length || types.length !== shapes.length) throw new TypeError('월드 대여 전 자산 배열 크기 불일치');
    if (terrain >= types.length || links.bridgeType >= types.length || links.noIslandType >= types.length) throw new RangeError('월드 대여 전 타입 번호 오류');
    if (storage && (storage.types.length !== types.length || storage.frames.length !== frames.length || storage.shapes.length !== shapes.length)) throw new TypeError('월드 대여 전 실제 자산 배열 크기 불일치');
    if (storage && authority && storage.authority !== authority) throw new TypeError('월드 대여 전 실제 권한 소유자 불일치');
    if (storage && gameAux && storage.gameAux !== gameAux) throw new TypeError('월드 대여 전 실제 게임 보조 소유자 불일치');
    return storage ?? new RawSurfaceStorage(types, frames, shapes, { authority, gameAux });
}

// GameAux 의 한 속성을 `{ value }` 상자로 보이는 별칭이다(장기 소유자를 직접 읽고 쓴다). (2026-11-11 WB04-E299)
function auxBox(aux, name) {
    return { get value() { return aux[name]; }, set value(next) { aux[name] = next; } };
}

// 월드의 표면 객체 하나의 초기 위치(저장/지형 어댑터가 넘기는 값)이다. 등록 뒤 현재 값은 raw 슬롯만 읽는다. (2026-11-11 WB04-E299)
// 사용: { type, owner, frame, x, y }.

export class RawSurfaceWorld {
    // 공유 저장소와 그 대여, 이 월드가 빌린 입력 표, 판본 고정 값이다.
    #storage;
    #storageUse;
    #terrainType;
    // 장부/삭제/모드/사건/건물 상태와 로딩 상태이다.
    #bookkeeping;
    #deletion;
    #mode;
    #eventState;
    #buildingState;
    #loadState;
    // 중첩 로딩 깊이(005c89b8)의 소유 상자이다. 실제 Client 는 Template/Core 의 같은 DWORD 상자를 공유하고, 독립 월드는 기존 초기값 1 의 지역 상자를 쓴다(표면 접두가 매 등록마다 현재 값을 다시 읽는다).
    #loadingDepth;
    // 외부 미션 모듈 훅과 현재 표시 장치, 변경 요청 한 번 소비 표시, 효과 관찰 카운터이다.
    #runtime = null;
    #runtimeBound = false;
    #deletionSelected = null;
    #deletionBound = false;
    #displayHooks = {};
    #changed = false;
    #effectRequests = 0;
    #walkerFalls = 0;
    #deletionRequests = 0;
    #loading = false;
    #loaded = false;
    #time = { game: 0, number: 0 };
    // 구성 모듈(순환 호출 때문에 구성이 끝난 뒤 채워진다).
    #graph;
    #display;
    #postPop;
    #unpop;
    #pop;
    #factory;
    #building;
    #frame;
    #owner;
    #neighbors;
    #connect;
    #surface;
    #baseDestroy;
    #virtualDestroy;
    #common;
    #lifecycle;
    #events;
    #formDispatch;
    #dependencies;
    #bridgeDecay = null;
    // 객체 송신기(4-C RawSquidTransmission 이 연결할 `(sid, flags)` 함수; 연결 전에는 단독 월드 기본값으로 아무것도 쓰지 않는다).
    #squidTransmission = null;
    #containedUnpop;
    #sessionContainedUnpop = null;
    #assetOwner;
    // Host: 공유 Storage Host 대여(use)이거나 독립 Host(legacy)이다.
    #processUse = null;
    #legacyHost = null;
    #legacyReclaim = null;
    #host;
    #sharedProcessHost;
    #returnedProcesses = null;
    #kernel;
    #processState;
    // 다리 붕괴 장치(현재 Editor/대피/소리/시각)이다.
    #decayEditor = null;
    #decayCarriers = null;
    #decaySound = null;
    #decayTime = null;
    #disposed = false;

    // links 는 `BridgeConnectState` 이다(준비 대여가 아니면 저장소의 현재 표면 종류로 복사한다). options: { processKernel(독립 Kernel; 없으면 Storage Host 사용), storage, adoptPrepared, sharedSavedGame({ value }), loadingDepth({ value }: Template/Core 와 공유하는 005c89b8 상자, 없으면 지역 상자 초기값 1), authority, gameAux }.
    // 공유 Kernel 이 없는 독립 World 는 Storage 의 독립 Kernel 을 쓴다. (2026-11-11 WB04-E299)
    constructor(types, frames, shapes, links, terrainType, { processKernel = null, storage = null, adoptPrepared = false, sharedSavedGame = null, loadingDepth = null, authority = null, gameAux = null } = {}) {
        if (loadingDepth !== null && typeof loadingDepth?.value !== 'number') throw new TypeError('공유 로딩 깊이는 숫자 value 를 가진 상자여야 합니다.');
        const selected = selectStorage(storage, types, frames, shapes, links, terrainType, adoptPrepared, authority, gameAux);
        this.#storage = selected;
        this.#storageUse = adoptPrepared ? selected.acquirePrepared() : selected.acquire();
        try {
            this.#build(types, frames, shapes, links, terrainType, processKernel, adoptPrepared, sharedSavedGame, loadingDepth);
        } catch (error) {
            // 구성이 도중에 실패하면 자기 대여만 반납한다(앞선 Native 잔류는 건드리지 않음).
            this.#drainCurrentProcessesForShutdown();
            this.#storageUse.release();
            throw error;
        }
    }

    // 순환하는 콜백은 실제 호출 전에 모든 모듈을 구성한다. 생성 중에는 Pop 하지 않는다. (2026-11-11 WB04-E299)
    #build(inputTypes, inputFrames, inputShapes, state, terrain, processKernel, prepared, sharedSavedGame, sharedLoadingDepth) {
        const storage = this.#storage;
        const types = storage.types;
        const frames = storage.frames;
        const shapes = storage.shapes;
        const pool = storage.pool;
        const hash = storage.hash;
        const spots = storage.spots;
        const links = storage.links;
        const aux = storage.gameAux;
        const authority = storage.authority;
        this.#terrainType = terrain;
        this.#bookkeeping = new SquidPostPopState();
        const book = this.#bookkeeping;
        book.bindTotalMade(storage.totalMade);
        book.bindTotalCost(auxBox(aux, 'totalCost'));
        book.bindProviders(storage.sidLists.providers);
        book.bindFactories(storage.sidLists.factories);
        this.#deletion = new SquidDeletionState();
        this.#deletion.sharedLostX = auxBox(aux, 'lostX');
        this.#deletion.sharedLostY = auxBox(aux, 'lostY');
        this.#mode = { ...DEFAULT_BRIDGE_DECAY_MODE };
        this.#eventState = { authority: true, debugKeep: false, bridgeType: 0 };
        // 깊이는 상자의 현재 값을 읽는다: 공유 상자는 생성 때 쓰지 않고(앞선 효과가 정한 값을 보존) 독립 월드만 기존 초기값 1 을 둔다.
        const depthBox = sharedLoadingDepth ?? { value: 1 };
        this.#loadingDepth = depthBox;
        this.#loadState = { get loadingDepth() { return depthBox.value >>> 0; }, savedGame: () => (sharedSavedGame ? Boolean(sharedSavedGame.value) : false) };
        this.#processState = storage.processes.state;
        this.#sharedProcessHost = !processKernel || storage.processes.usesKernel(processKernel);
        this.#kernel = this.#sharedProcessHost ? storage.processes.kernel : processKernel;
        // 명시 Native prefix 의 현재 종류/모드는 보존하고 기존 독립 설치만 새 상태를 쓴다.
        if (!prepared) {
            links.connectorType = state.connectorType;
            links.islandType = state.islandType;
            links.stalagType = state.stalagType;
            links.noIslandType = state.noIslandType;
            links.bridgeType = state.bridgeType;
            links.battle = state.battle;
            links.mission = state.mission;
            links.ownerColors.splice(0, links.ownerColors.length, ...state.ownerColors);
        }
        if (types.length !== inputTypes.length || frames.length !== inputFrames.length || shapes.length !== inputShapes.length) throw new TypeError('raw 표면 월드 실제 물리/판본 불일치');
        if (terrain >= types.length || links.bridgeType >= types.length || links.noIslandType >= types.length) throw new RangeError('raw 표면 월드 타입 번호 오류');
        book.localOwner = 1;
        if (!prepared) {
            storage.ownerMode.fort = false;
            storage.ownerMode.battle = true;
            storage.ownerMode.challenge = false;
        } else this.#mode.editor = true;
        this.#eventState.bridgeType = links.bridgeType;
        const assertion = (expression, file, line) => storage.assertDisplay(expression, file, line);
        const checking = () => storage.checking();
        // 실제 전체 풀은 원문 소진 재구성을 허용한다. 생성 자체는 현재 물리를 쓰지 않는다.
        this.#graph = new RawGraph(pool, hash, spots, types, storage.codes, { storage: storage.graph, recovery: 'fullPool' });
        this.#graph.bindCurrentTypes(types);
        this.#graph.bindDiagnostics({ checking, assertion });
        this.#graph.bindTrace((level, text) => storage.traceGraph(level, text));
        this.#graph.bindDetachState(() => book.graphsEnabled, () => this.#deletion.rebuildGraph);
        this.#graph.bindClock(() => storage.readGraphClock());
        const sink = {
            suppressed: () => Boolean(this.#displayHooks.suppressed && this.#displayHooks.suppressed()),
            invalidate: (rect, flags) => { this.#changed = true; if (this.#displayHooks.invalidate) this.#displayHooks.invalidate(rect, flags); },
            checking,
        };
        this.#display = new SquidDisplay(types, shapes, sink, { cameraX: 0, cameraY: 0, zoom: 65536, viewport: { left: 0, top: 0, right: 640, bottom: 480 } });
        this.#postPop = new SquidPostPop(pool, types, book, this.#graph, (sid) => this.#connect.connect(sid));
        this.#postPop.bindCurrentTypes(types);
        this.#postPop.bindManaDiagnostics({ checking, assertion });
        this.#postPop.setIslandPrefix((sid, flags) => this.#connect.islandPostPopPrefix(sid, flags));
        this.#postPop.setSurfacePrefix((sid, flags) => this.#surface.prefix(sid, flags));
        this.#unpop = new SquidUnpop(pool, hash, spots, this.#display);
        this.#pop = new SquidPop(pool, hash, spots, { display: this.#display, postPop: this.#postPop });
        this.#unpop.bindDiagnostics({ checking, types, assertion });
        // 일반 Pop 의 객체 송신은 현재 연결된 송신기를 호출한다. 단독 월드(방송이 없는 구성)의 기본값은 쓰기 없음이며 송신 결과는 다른 곳에서 읽히지 않는다.
        this.#pop.bindTransmission((sid, flags) => { if (this.#squidTransmission) this.#squidTransmission(sid, flags); });
        this.#pop.bindDiagnostics({ checking, types, metamap: () => storage.ownerMode.challenge, trace: (level, text) => storage.traceGraph(level, text), assertion });
        // 월드 기본 종속 Unpop 종단은 같은 물리와 장기 진단 장치만 빌린다.
        const containedHooks = {
            parent: (sid) => commonContainedParent(pool, sid),
            previous: (sid) => commonContainedPrevious(pool, sid),
            setPrevious: (sid, previous) => commonContainedSetPrevious(pool, sid, previous),
            parentNotice: (parent, form, reason) => new RawContentParentNotice(pool).notice(parent, form, reason),
            assertion,
        };
        this.#containedUnpop = new RawSquidContainedUnpop(pool, {
            contentFormType: () => 6,
            gumpFormType: () => 3,
            uiEvent: () => 0x02000039,
            checking,
            inMetamapMode: () => storage.ownerMode.challenge,
            boss: () => authority.boss,
        }, containedHooks);
        this.#unpop.bindContainedUnpop((sid) => {
            const current = this.#sessionContainedUnpop;
            if (current) current.unpop(sid);
            else this.#containedUnpop.unpop(sid);
        });
        this.#buildingState = { islandType: terrain, bridgeType: links.bridgeType, checking };
        this.#building = new RawBuildingSpace(pool, hash, types, this.#buildingState);
        this.#building.bindCurrentBridgeType(() => links.bridgeType);
        this.#building.bindNotifyDiagnostics(checking, assertion);
        this.#pop.setBuildingSpace(this.#building);
        this.#unpop.setBuildingSpace(this.#building);
        this.#factory = new SquidFactory(pool, types, { unpop: this.#unpop });
        this.#factory.bindDiagnostics({ types, assertion, detail: (text) => storage.recordAssertionDetail(text), checking });
        this.#factory.bindFormDetach((sid) => {
            const current = this.#sessionContainedUnpop;
            if (current) current.detach(sid);
            else this.#containedUnpop.detach(sid);
        });
        this.#factory.bindFormPopDepth({ get value() { return book.depth; }, set value(next) { book.depth = next >>> 0; } });
        this.#factory.bindFormAttach(() => (this.#sessionContainedUnpop ? this.#sessionContainedUnpop.contentFormType : 6), (sid, flags) => {
            if (!this.#runtime?.formTransmit) throw new Error('현재 공통 Form Pop 전송 미연결');
            this.#runtime.formTransmit(sid, flags);
        });
        this.#factory.bindGameAux(aux);
        this.#frame = new SquidFrame(pool, types, makeSquidFrameHooks(pool, types, this.#display, this.#unpop, this.#pop));
        this.#owner = new SquidOwner(pool, types, book, storage.ownerMode);
        this.#owner.bindDiagnostics({ types, assertion });
        this.#neighbors = new RawSquidNeighbors(pool, hash, spots, types, storage.codes);
        this.#neighbors.bindCurrentTypes(types);
        const objects = {
            // 생성/소유자/프레임 변경은 표시 억제 상태와 무관하게 논리 변경을 알린다.
            create: (type, flags) => { this.#changed = true; return this.#factory.create(type, flags); },
            pop: (sid, x, y, flags) => this.#popObject(sid, x, y, flags),
            setFrame: (sid, number, flags) => { this.#frame.set(sid, number, flags); this.#changed = true; },
            notifySurface: (sid) => this.#building.notifySurface(sid),
        };
        this.#assetOwner = makeAssetOwnerDispatch(pool, links, () => book.productionDirty, (sid, player) => this.#owner.set(sid, player));
        objects.setOwner = (sid, player) => { this.#assetOwner(sid, player); this.#changed = true; };
        this.#connect = new RawBridgeConnect(pool, this.#neighbors, links, objects);
        this.#surface = new RawIslandPostPop(pool, hash.view(0), types, storage.codes, links, this.#loadState, {
            objects,
            connect: (sid) => this.#connect.connect(sid),
            findTypeAt: (x, y, type) => this.#connect.findTypeAt(x, y, type),
            updateDisplay: (sid) => { this.#changed = true; this.#display.update(pool.slot(sid)); },
            incompleteSupport: () => { throw new Error('raw 표면 월드의 불완전한 받침 묶음'); },
            assertion,
            detail: (text) => storage.recordAssertionDetail(text),
            typeWas: () => aux.supportTypeWas,
        });
        this.#baseDestroy = new RawSquidDestroy(pool, this.#unpop, types);
        this.#virtualDestroy = new RawSquidDestroyDispatch(this.#baseDestroy, this.#neighbors, this.#mode, this.#graph);
        this.#virtualDestroy.bindCurrentModes(authority);
        // 이 기본값은 단독 표면 기하 검사용 관찰이다. 실제 세션은 bindDeletion 으로 보상/실제 UI 를 공급한다.
        this.#common = new SquidDestroyLifecycle(pool, types, book, this.#deletion, this.#baseDestroy, {
            emit: () => { this.#deletionRequests += 1; },
            forward: (event) => {
                if (event.effect !== SQUID_DESTROY_EFFECT.transmit) throw new Error('raw 표면 월드의 미연결 삭제 효과');
            },
        }, this.#graph);
        this.#lifecycle = new RawSurfaceLifecycle(pool, this.#neighbors, links.connectorType, this.#commonDeletionHooks(), {
            scheduleFall: (sid, x, y) => { scheduleBridgeFallChecked(this.#host, pool, types, sid, x, y, checking); },
            destroy: (sid, flags) => { this.destroy(sid, flags); },
            fallWalker: (sid) => {
                if (this.#runtimeBound) {
                    if (!this.#runtime.walkerFall(sid)) throw new Error('월드 낙하 처리 실패');
                } else this.#walkerFalls += 1;
            },
            bridgeEffect: (event) => { if (this.#runtimeBound) this.#runtime.bridgeEffect(event); else this.#effectRequests += 1; },
        });
        const eventHooks = {
            // 실제 사건의 삭제/재등록도 같은 가상 경로와 SHP 공급을 사용한다.
            notifySurface: objects.notifySurface,
            create: (type) => { this.#changed = true; return this.#factory.create(type); },
            destroy: (sid, flags) => { this.destroy(sid, flags); },
            setOwner: (sid, player) => objects.setOwner(sid, player),
            pop: objects.pop,
        };
        this.#events = new RawBridgeEvents(pool, this.#eventState, makeBridgeNeighborHooks(this.#neighbors, eventHooks));
        // 표면 월드의 실제 자산 재정의/프로세스 삭제도 공유 Form 분배기를 사용한다.
        this.#formDispatch = new SquidFormDispatch(pool, this.#baseDestroy, this.#lifecycle.hooks(), (sid, flags, hooks) => this.#virtualDestroy.destroy(sid, flags, hooks));
        if (this.#sharedProcessHost) {
            const runtime = createProcessHostRuntime({
                destroy: this.#baseDestroy,
                regular: makeBridgeRegularHandler(pool, this.#events),
                asset: this.#formDispatch.hooks(),
                factory: this.#factory,
                postPopDepth: () => book.depth,
                assertion: (expression, file, line) => storage.assertDisplay(expression, file, line),
            });
            this.#processUse = storage.processes.bind(runtime);
            this.#host = storage.processes.host;
        } else {
            this.#legacyHost = new SquidProcessHost(pool, types, this.#kernel, this.#baseDestroy, this.#processState, { regular: makeBridgeRegularHandler(pool, this.#events), asset: this.#formDispatch.hooks() });
            this.#legacyHost.bindFormFactory(this.#factory);
            this.#host = this.#legacyHost;
            // 실제 현재 Form/자산 분배로만 회수하며 raw Kernel 을 Storage 의 다른 Kernel 로 대체하지 않는다.
            this.#legacyReclaim = storage.processes.bindLegacyAllocationReclaim((sid) => { void this.destroy(sid, 0); });
        }
        this.#formDispatch.bind(this.#host);
        this.#dependencies = new DependFormHost(pool, this.#baseDestroy, {
            notice: (watcher, watched, form) => {
                if (this.#runtime?.dependencyNotice) {
                    this.#runtime.dependencyNotice(watcher, watched, form);
                    return;
                }
                // 실제 ProcessForm34 는 server 일 때 PID 의 notify(2, form, watched)를 호출한다.
                this.#host.notifyDependency(watcher, watched, form);
            },
            fallback: this.#formDispatch.hooks(),
            transmit: (form, flags) => {
                if (!this.#runtime?.formTransmit) throw new Error('현재 DependForm 서버 전송 소비자 미연결');
                this.#runtime.formTransmit(form, flags);
            },
        });
        this.#formDispatch.bind(this.#dependencies);
        // 장기 현재 번호를 두 번 읽고 같은 World 의 공통 생성자/진단을 사용한다.
        this.#dependencies.bindCreation(this.#factory, () => aux.dependFormType, assertion);
        this.#dependencies.bindUnpop((sid) => this.#factory.unpopForm(sid));
        const decay = {
            destroy: (sid, flags) => { void this.destroy(sid, flags); },
            notifySurface: (sid) => this.#building.notifySurface(sid),
            refresh: (sid) => { this.#changed = true; this.#display.update(pool.slot(sid)); },
            carriers: (sid) => {
                if (this.#decayCarriers) this.#decayCarriers(sid);
                else if (this.#runtimeBound) throw new Error('실제 다리 붕괴 대피 장치 미연결');
                else this.#effectRequests += 1;
            },
            crackSound: (sid) => {
                if (this.#decaySound) this.#decaySound(sid);
                else if (this.#runtimeBound) throw new Error('실제 다리 균열 소리 장치 미연결');
                else this.#effectRequests += 1;
            },
            assertion: (condition, source, line) => { throw new Error(`원문 다리 붕괴 진단 ${source}:${line}: ${condition}`); },
        };
        this.#bridgeDecay = new RawBridgeDecay(pool, types, storage.codes, hash, spots, this.#graph.records(), this.#mode, decay, { authority, checking });
    }

    // 공통 삭제 훅 proxy 는 후설치 이전 공통 객체의 주소를 보존하지 않아 같은 state/reward 로 교체한 뒤에도 현재 몸체를 호출한다. (2026-11-11 WB04-E299)
    #commonDeletionHooks() {
        return {
            selected: () => (this.#deletionSelected ? this.#deletionSelected() : this.#deletion.selected),
            emit: (event) => {
                if (event.effect === SQUID_DESTROY_EFFECT.preDestroy && this.#deletionSelected) this.#deletion.selected = this.#deletionSelected();
                this.#common.hooks().emit(event);
            },
        };
    }

    // 현재 raw 프레임의 SHP 크기로 실제 공간과 파생 postPop 을 등록한다(원문 앞단/좌표/spot 뒤에 현재 TYPE/SHP 를 조회). (2026-11-11 WB04-E299)
    #popObject(sid, x, y, flags) {
        const result = this.#pop.popCurrent(sid, x, y, flags);
        if (result !== RAW_POP_RESULT.unchanged) this.#changed = true;
    }

    // 외부 Kernel 의 다른 Host/Client 객체는 보존하고 이 월드의 최종 소유권만 반납한다(Native Clear 와 구별). (2026-11-11 WB04-E299)
    #drainCurrentProcessesForShutdown() {
        if (this.#processUse) this.#processUse.drainOwnedForShutdown();
        else if (this.#legacyHost) this.#kernel.drainOwnedForShutdown((process) => this.#legacyHost.owns(process));
    }

    // 월드 모듈이 사라진 뒤 저장소 대여를 푼다: 현재 처리기 대여(공유 Host)·레거시 회수 대여 → 저장소 대여 순서이다(멱등). (2026-11-11 WB04-E299)
    dispose() {
        if (this.#disposed) return;
        this.#disposed = true;
        this.#drainCurrentProcessesForShutdown();
        if (this.#processUse) this.#processUse.release();
        if (this.#legacyReclaim) this.#legacyReclaim.release();
        this.#storageUse.release();
    }

    // 이 뷰는 내부 모듈의 현재 참조만 돌려주며 새 배열/풀을 만들지 않는다. (2026-11-11 WB04-E299)
    core() {
        const storage = this.#storage;
        return {
            pool: storage.pool, hash: storage.hash, spots: storage.spots, types: storage.types, frames: storage.frames, shapes: storage.shapes,
            bookkeeping: this.#bookkeeping, ownerMode: storage.ownerMode, factory: this.#factory, owner: this.#owner, frame: this.#frame, pop: this.#pop, unpop: this.#unpop,
            postPop: this.#postPop, display: this.#display, building: this.#building, graph: this.#graph, neighbors: this.#neighbors, destroy: this.#baseDestroy,
            virtualDestroy: this.#virtualDestroy, surfaces: this.#lifecycle, bridgeEvents: this.#events, forms: this.#formDispatch, dependencies: this.#dependencies,
            processes: this.#host, processState: this.#processState, kernel: this.#kernel, bridgeState: storage.links,
        };
    }

    // 4-C 의 객체 송신기(RawSquidTransmission.pop)를 설치한다. 첫 객체 Pop 전에 한 번만 허용하며 함수 또는 null 이다. (2026-11-11 WB04-E299)
    bindSquidTransmission(transmission) {
        if (transmission !== null && typeof transmission !== 'function') throw new TypeError('Squid 송신기는 (sid, flags) 함수여야 합니다.');
        this.#squidTransmission = transmission;
    }

    // 미니맵도 Session 이 아닌 동일 물리 소유자를 직접 빌린다. (2026-11-11 WB04-E299)
    get storage() {
        return this.#storage;
    }

    // 원본 옵션의 TYPE +c4 쓰기가 같은 월드의 188 TYPE 를 직접 사용하게 한다. 새 사본을 만들지 않는다. (2026-11-11 WB04-E299)
    get mutableTypes() {
        return this.#storage.types;
    }

    // 최초 객체/Runtime 등록 전에 원본 모드의 실제 소유권·연결색·다리 붕괴 소비자를 함께 설정한다. 지정 Take 가 freeCount 를 보존하는 경우도 이미 만든 실제 슬롯을 거부한다. (2026-11-11 WB04-E299)
    configureInstallModes(fort, battle, challenge, editor) {
        const pool = this.#storage.pool;
        if (this.#runtimeBound || this.#loading || this.#loaded || pool.freeCount !== pool.capacity - RESERVED_SIDS) throw new Error('실제설치모드는첫객체/Runtime등록전에필요');
        // 지정 Take 가 freeCount 를 보존하는 경우도 이미 만든 실제 슬롯을 거부한다.
        for (let sid = RESERVED_SIDS; sid < pool.capacity; sid += 1) {
            if (!(pool.slot(sid)[SLOT_STATE] & 1)) throw new Error('실제설치모드설정전에객체존재');
        }
        const mode = this.#storage.ownerMode;
        mode.fort = Boolean(fort);
        mode.battle = Boolean(battle);
        mode.challenge = Boolean(challenge);
        this.#storage.links.battle = Boolean(battle);
        this.#mode.editor = Boolean(editor);
    }

    // 미리보기도 같은 실제 연결 생성기와 현재 표면 전역을 사용한다. (2026-11-11 WB04-E299)
    get bridgeConnections() {
        return this.#connect;
    }

    // 완성된 실제 파생 훅을 설치할 때 Form/Regular 의 기존 callback 도 현재 경계를 읽게 한다. 첫 객체 생성 전에 한 번만 허용하며 미연결 콜백을 생략하지 않는다. (2026-11-11 WB04-E299)
    bindRuntime(hooks) {
        const pool = this.#storage.pool;
        if (this.#runtimeBound || this.#loading || this.#loaded || pool.freeCount !== pool.capacity - RESERVED_SIDS) throw new Error('월드 runtime은 초기 객체 생성 전에 한 번 연결해야 합니다');
        // 지정 Take 는 freeCount 를 보존하므로 실제 슬롯도 확인하여 기존 객체가 있는 늦은 설치를 거부한다.
        for (let id = RESERVED_SIDS; id < pool.capacity; id += 1) {
            if (!(pool.slot(id)[SLOT_STATE] & 1)) throw new Error('월드 runtime 설치 전 객체가 존재합니다');
        }
        if (!hooks?.assets?.emit || !hooks.assets.selected || !hooks.destroy || !hooks.regular || !hooks.transmit?.writeData || !hooks.transmit.transmit
            || !hooks.walkerFall || !hooks.loadingDepth || !hooks.bridgeEffect) throw new TypeError('월드 runtime 필수 경계 누락');
        this.#formDispatch.setAssets(hooks.assets, hooks.destroy);
        this.#host.setRegularHandler(hooks.regular);
        this.#host.setFormTransmit(hooks.transmit);
        this.#runtime = hooks;
        this.#runtimeBound = true;
        this.#runtime.loadingDepth(this.#loadState.loadingDepth);
    }

    // Session 은 엔진을 소유하고 월드는 현재 호출 동안만 pin 하여 생성 실패/반납 뒤 수명 밖 접근을 막는다(웹에서는 약한 참조 대신 명시 해제로 null 을 준다). (2026-11-11 WB04-E299)
    bindContainedUnpop(binding) {
        if (binding && binding.pool !== this.#storage.pool) throw new TypeError('종속 Unpop의 실제 SID 풀 불일치');
        this.#sessionContainedUnpop = binding ?? null;
    }

    // 기존 표면 분배기가 보유한 proxy 를 유지하면서 단일 공통 삭제 state 의 실제 경제/UI 소비자를 교체한다. 첫 객체 생성 전에 한 번만 허용한다. (2026-11-11 WB04-E299)
    bindDeletion(reward, selected, hooks) {
        const pool = this.#storage.pool;
        if (this.#deletionBound || this.#loading || this.#loaded || pool.freeCount !== pool.capacity - RESERVED_SIDS) throw new Error('공통 삭제 실제 소비자는 첫 객체 생성 전에 한 번 연결해야 합니다');
        // Take 는 freeCount 를 보존할 수 있으므로 동일 물리 슬롯도 검사한다.
        for (let id = RESERVED_SIDS; id < pool.capacity; id += 1) {
            if (!(pool.slot(id)[SLOT_STATE] & 1)) throw new Error('공통 삭제 실제 소비자 설치 전 객체가 존재합니다');
        }
        if (!reward || typeof selected !== 'function' || !hooks?.emit || !hooks.forward) throw new TypeError('공통 삭제의 실제 보상/선택/효과/파생 경계 누락');
        const replacement = new SquidDestroyLifecycle(pool, this.#storage.types, this.#bookkeeping, this.#deletion, this.#baseDestroy, hooks, this.#graph, reward);
        this.#common = replacement;
        this.#deletionSelected = selected;
        this.#deletionBound = true;
    }

    // 후설치와 같은 현재 원본 삭제 전역이다. 별도 세션 사본을 만들지 않는다. (2026-11-11 WB04-E299)
    get deletionState() {
        return this.#deletion;
    }

    // BindDeletion 이 교체한 현재 공통 몸체를 반환하고 별도 깊이/보상 사본을 만들지 않는다. (2026-11-11 WB04-E299)
    get commonDeletion() {
        return this.#common;
    }

    // 원본 Fort 순서대로 Create/필드 저장/Pop 하는 로더의 시작이다. 원본 00540414 는 PE 초기값 1 이므로 적재 중에도 Graph 를 켠다. (2026-11-11 WB04-E299)
    beginLoad() {
        if (this.#loaded || this.#loading) throw new Error('raw 월드 중복 로드 시작');
        this.#bookkeeping.graphsEnabled = true;
        this.#loading = true;
        this.setLoadingDepth(1);
    }

    // 모든 실제 객체를 등록한 뒤 원본 Graph/장부를 활성화한다. (2026-11-11 WB04-E299)
    finishLoad() {
        if (!this.#loading || this.#loaded) throw new Error('raw 월드 로드 종료 순서 오류');
        this.#graph.rebuild(true);
        this.finishLoadAfterGraph();
    }

    // 실제 캠페인 부모가 Graph(false)를 이미 실행한 뒤 로딩 상태만 종료한다. Graph/풀/해시를 재구성하지 않는다. (2026-11-11 WB04-E299)
    finishLoadAfterGraph() {
        if (!this.#loading || this.#loaded) throw new Error('raw 월드 로드 종료 순서 오류');
        this.#bookkeeping.graphsEnabled = true;
        this.setLoadingDepth(0);
        this.#loading = false;
        this.#loaded = true;
        this.#changed = true;
    }

    // 복원된 타입 생성자를 같은 풀에서 실행한다. (2026-11-11 WB04-E299)
    create(type, flags = 0) {
        this.#changed = true;
        return this.#factory.create(type, flags);
    }

    // 현재 raw 프레임의 실제 SHP 크기가 공간 등록에 쓰인다. (2026-11-11 WB04-E299)
    pop(sid, x, y, flags = 0) {
        this.#popObject(sid, x, y, flags);
    }

    // 현재 raw 타입의 실제 공간/표시 해제 몸체를 호출한다. (2026-11-11 WB04-E299)
    unpop(sid, flags = 0) {
        const type = this.#storage.types[this.#storage.pool.slot(sid)[SLOT_TYPE]];
        if (!type) throw new RangeError('Unpop 현재 TYPE 번호 오류');
        this.#unpop.unpop(sid, type, flags);
        this.#changed = true;
    }

    // Unpop 뒤 현재 raw 좌표를 읽어 같은 공간에 다시 Pop 한다. 원본 호출자가 해제 순서를 정한다. (2026-11-11 WB04-E299)
    repop(sid, flags = 0) {
        const raw = this.#storage.pool.slot(sid);
        this.#popObject(sid, floatAt(raw, SLOT_X), floatAt(raw, SLOT_Y), flags);
    }

    // 가상 삭제가 거부되면 changed 를 새로 켜지 않는다. 다리 거부 조건과 파생 pre/post 를 동일한 호출로 실행한다. (2026-11-11 WB04-E299)
    destroy(sid, flags = 0) {
        const result = this.#formDispatch.destroy(sid, flags);
        if (result) this.#changed = true;
        return result;
    }

    // 예약 프로세스를 연결 모듈보다 먼저 해제해 가상 소멸자의 참조 수명을 보장한다. (2026-11-11 WB04-E299)
    clearProcesses() {
        this.#kernel.clear();
    }

    // 실제 Native 단일 Clear 가 정상 복귀했을 때만 증빙을 전달한다(공유 Host 대여 필요). (2026-11-11 WB04-E299)
    clearProcessesForState() {
        if (!this.#processUse) throw new Error('장기 ProcessHost 현재 대여 없는 State Clear');
        return this.#processUse.clearForState();
    }

    // 같은 증빙으로 현재 처리기 대여만 끝낸다(Regular 잔류는 유지). 현재 대여를 소비한 뒤에만 증빙을 보관한다. (2026-11-11 WB04-E299)
    returnProcessBindingsForState(pass) {
        if (!this.#processUse || this.#returnedProcesses) throw new Error('ProcessHost State 반납의 단일 실제 대여 필요');
        this.#returnedProcesses = this.#processUse.returnForState(pass);
        this.#processUse = null;
    }

    // 실제 소비된 Returned 증빙의 존재를 조회하며 공개 성공 setter 는 제공하지 않는다. (2026-11-11 WB04-E299)
    get processBindingsReturned() {
        return this.#returnedProcesses !== null;
    }

    // 명시 전체 Kernel 의 최종 소멸 전용이며 원문 Clear 한 번과 구별한다(남은 프로세스가 없을 때까지 Clear 를 반복). (2026-11-11 WB04-E299)
    drainProcesses() {
        // 소멸 중 재등록된 프로세스까지 남지 않도록 비워질 때까지 반복한다.
        do {
            this.#kernel.clear();
        } while (this.#kernel.size);
    }

    // 현재 대여의 일반 종료만 반납하며 이전 Native 잔류와 다른 Storage 는 건드리지 않는다. (2026-11-11 WB04-E299)
    drainCurrentProcessBindingsForShutdown() {
        this.#drainCurrentProcessesForShutdown();
    }

    // 파생 로더의 중첩 깊이를 표면과 비표면 몸체에 동일하게 전달한다. (2026-11-11 WB04-E299)
    setLoadingDepth(depth) {
        this.#loadingDepth.value = depth >>> 0;
        if (this.#runtimeBound) this.#runtime.loadingDepth(depth >>> 0);
    }

    // 저장 객체의 순서와 부착 생성 순서는 구별한다. Graph 는 전체 초기 등록을 마친 뒤 한 번 재구성한다. seeds 는 `{ type, owner, frame, x, y }` 목록이다. (2026-11-11 WB04-E299)
    load(seeds) {
        if (this.#loaded) throw new Error('raw 표면 월드 중복 로드');
        const storage = this.#storage;
        const links = storage.links;
        const sorted = Array.from(seeds, (seed) => ({ ...seed }));
        // 쓰기 전에 전체 초기 입력의 지원 타입/정수 좌표/프레임/소유자를 검사한다.
        for (const seed of sorted) {
            if (seed.type !== this.#terrainType && seed.type !== links.noIslandType && seed.type !== links.bridgeType) throw new TypeError('raw 표면 월드의 미지원 초기 타입');
            if (!Number.isFinite(seed.x) || !Number.isFinite(seed.y) || seed.x < 1 || seed.y < 1 || seed.x >= MAP_SIDE || seed.y >= MAP_SIDE
                || Math.trunc(seed.x) !== seed.x || Math.trunc(seed.y) !== seed.y || !Number.isInteger(seed.owner) || seed.owner > PLAYER_COUNT || seed.owner < 0
                || !Number.isInteger(seed.frame) || seed.frame < 0 || seed.frame >= storage.frames[seed.type].codes.length) throw new RangeError('raw 표면 월드 초기 필드 오류');
        }
        // 비유한 좌표를 거부한 뒤 정렬한다. 받침 접두가 서/북 이웃을 읽으므로 noIsland 는 행 우선이다.
        const rank = (type) => (type === this.#terrainType ? 0 : type === links.noIslandType ? 1 : 2);
        sorted.sort((first, second) => {
            if (rank(first.type) !== rank(second.type)) return rank(first.type) - rank(second.type);
            if (first.y !== second.y) return first.y - second.y;
            return first.x - second.x;
        });
        this.beginLoad();
        // 실제 생성자·가상 소유자·Pop 을 사용하며 아직 Graph 를 변경하지 않는다.
        for (const seed of sorted) {
            const sid = this.#factory.create(seed.type);
            this.setAssetOwner(sid, seed.owner);
            writeFrame(storage.pool.allocated(sid), seed.frame);
            this.#popObject(sid, seed.x, seed.y, 0);
        }
        this.finishLoad();
    }

    // GUI 의 준비 단계는 시각만 쓰며 Kernel 을 중첩 실행하지 않는다. (2026-11-11 WB04-E299)
    prepareFrame(time) {
        const state = this.#processState;
        const current = { ...time };
        // 실제 현재 시각으로 독립 스냅샷의 게임 값만 교정한다.
        if (this.#storage.processes.hasDevices()) current.game = state.now;
        if (!Number.isFinite(current.game) || current.game < this.#time.game) throw new RangeError('raw 월드의 역행/비유한 게임 시각');
        // bound Client 시각은 스냅샷으로 역쓰기하지 않는다.
        if (!this.#storage.processes.hasDevices()) state.now = current.game;
        this.#time = current;
    }

    // 독립 호출자는 같은 준비 뒤 Kernel 을 한 번 실행하며 기존 정지 계약을 유지한다. (2026-11-11 WB04-E299)
    runFrame(time, paused) {
        this.scanBridgeDecay(time, paused);
        if (!paused) this.#kernel.runFrame();
    }

    // 초기 상태를 재설정하지 않고 같은 현재 전역/장치만 설치한다: editor 는 `() => 현재 Editor DWORD` 이다. (2026-11-11 WB04-E299)
    bindBridgeDecay(editor, carriers, sound, time = null) {
        if (this.#decayEditor || typeof carriers !== 'function' || typeof sound !== 'function' || typeof editor !== 'function') throw new Error('실제 다리 붕괴 장치 중복/미완성 연결');
        this.#decayEditor = editor;
        this.#decayCarriers = carriers;
        this.#decaySound = sound;
        this.#decayTime = time;
        this.#virtualDestroy.bindCurrentModes(this.#storage.authority, editor);
    }

    // 같은 시각을 먼저 공급한 호출자가 프레임의 원문 위치에서 한 칸을 요청한다. (2026-11-11 WB04-E299)
    decayBridgeCell(time, sid) {
        this.prepareFrame(time);
        if (!this.#bridgeDecay) throw new Error('다리 붕괴의 현재 PATCH 몸체 미연결');
        if (this.#decayTime) this.#decayTime(this.#time);
        this.#bridgeDecay.cell(sid);
    }

    // 독립 World 의 프레임은 Kernel 전에 같은 저장소 커서를 진행한다. Client 는 전역 공통 입구에서 한 번 호출한다. (2026-11-11 WB04-E299)
    scanBridgeDecay(time, paused) {
        this.prepareFrame(time);
        if (this.#decayTime) this.#decayTime(this.#time);
        if (!this.#bridgeDecay) return;
        const storage = this.#storage;
        const current = { ...this.#mode, server: storage.authority.server, authority: storage.authority.boss };
        if (this.#decayEditor) current.editor = this.#decayEditor() !== 0;
        const scan = new RawBridgeDecayScan(storage.pool, storage.types, storage.bridgeDecayScan, () => storage.bridgeDecayPeriod, current, (sid) => this.#bridgeDecay.cell(sid));
        scan.frame(this.#time, paused);
    }

    // 카메라/viewport 와 현재 Renderer 의 표시 전달 경로를 갱신한다. (2026-11-11 WB04-E299)
    setDisplay(view, hooks) {
        this.#display.setView(view);
        this.#displayHooks = hooks ?? {};
    }

    // 일반 4단계 해시와 실제 타입 필터를 사용하는 조회이다. (2026-11-11 WB04-E299)
    find(x, y, type) {
        return this.#connect.findTypeAt(x, y, type);
    }

    // 받침/종유석/9칸 소유자 갱신은 복원된 원본 몸체를 사용한다. (2026-11-11 WB04-E299)
    setSupportOwner(x, y, owner) {
        this.#surface.setSupportOwner(x, y, owner);
        this.#changed = true;
    }

    // 상위 사제/신전 분배 뒤의 나머지 자산은 같은 현재 VT 와 생산 장부를 사용한다. (2026-11-11 WB04-E299)
    setAssetOwner(sid, owner) {
        this.#assetOwner(sid, owner);
        this.#changed = true;
    }

    // 표시/검사에 현재 살아 있는 자산만 제공한다. ProcessForm 은 제외한다. 임시 객체의 옛 프레임/좌표를 복사하지 않고 raw 슬롯을 읽는다. (2026-11-11 WB04-E299)
    objects() {
        const pool = this.#storage.pool;
        const result = [];
        // 현재 살아 있는 자산 번호만 읽는다.
        for (let id = RESERVED_SIDS; id < pool.capacity; id += 1) {
            const raw = pool.slot(id);
            if ((raw[SLOT_STATE] & INACTIVE) || raw[SLOT_TYPE] < FIRST_ASSET_TYPE) continue;
            result.push({ sid: id, object: { type: raw[SLOT_TYPE], owner: raw[SLOT_OWNER], frame: dword(raw, SLOT_FRAME) | 0, x: floatAt(raw, SLOT_X), y: floatAt(raw, SLOT_Y) } });
        }
        return result;
    }

    // 마지막 PrepareFrame 이 고정한 프레임 시각 사본이다(cpppj Report 첫 줄의 game/number 관측). (2026-11-11 WB04-E299)
    get frameTime() {
        return { ...this.#time };
    }

    // 독립 월드(Runtime 미연결)가 실제 소비자 대신 센 다리 효과·walker 낙하·공통 삭제 요청 횟수이다(cpppj Report 의 effectRequests/deletionRequests 관측). (2026-11-11 WB04-E299)
    get observed() {
        return { effects: this.#effectRequests, walkerFalls: this.#walkerFalls, deletions: this.#deletionRequests };
    }

    // 변경은 Renderer 억제 여부와 무관하게 한 번만 소비한다. (2026-11-11 WB04-E299)
    takeChanged() {
        const value = this.#changed;
        this.#changed = false;
        return value;
    }
}
