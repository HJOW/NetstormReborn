// 같은 캠페인 월드에서 일반 서버 건설 확정/배치 통지와 로컬 간헐천의 noIsland 확정·실제 생성 사슬을 조립하는 client 연결이다(cpppj `client/RawCampaignConstructionBindings.cpp/.h`, 10.78 전용).
// 사용:
//   const state = new RawCampaignConstructionState();   // 세션이 끝날 때까지 유지할 실제 건설 전역(request·command·confirm·clear·notice·place·packet·geyser). 호출자가 현재 모드/소유자 값을 직접 갱신한다.
//   const bounds = makePlayerGeyserBoundsState(core.ownerMode, { mission, mapSize, puzzlePieceType, playerDirections });   // battle/metamap 은 같은 월드의 현재 소유자 모드를 읽는다.
//   const construction = new RawCampaignConstructionBindings(world, hp, sp, reward, frames, chunks, players, broadcast, state, bounds, recent, allGeysers, effects, gameplay);
//   construction.confirm(request) · request(request) · receive(sender, packet) · receiveCommandPacket(sender, packet) · receiveNotice(packet) · refund(type, owner) · reject(…) · chargeShortfall(…) ·
//   register(dispatch, prepare) · confirmSupport(request) · generateLocalGeysers() · populateGeysers(quantity, random, rectangle, options) · bindLocal(hooks) · lastPacket() · destroyHooks(fallback) · destroy(fallback) ·
//   localGeysers · playerBounds · pool.
// 첫 인자는 RawSurfaceWorld 이거나 `bindConstructionRuntime(world)` 로 만든 실행 부모({ core, storage, create, pop, destroy, broadcast? })이다. 실행 부모 형태는 World 생성/대여/모드 변경을 하지 않는다.
// 입력: hp 는 같은 풀/장부의 SquidReward, sp 는 ScrambledSpStore, reward 는 SquidRewardState(간헐천 자원 pool 을 같은 소유자에서 읽고 쓴다), frames 는 TYPE 별 { frames, defaultFrame } 188칸,
//   chunks 는 ChunkMap, players 는 실제 9 Player 기록(Uint8Array 9×0xb4), broadcast 는 RawBattleBroadcast(실행 부모에 방송이 없을 때만 빌린다), recent 는 전체 간헐천 목록과 다른 전용 5칸 이상 목록이다.
//   effects = { configInt(key, fallback), wallSeconds(), refresh(), presentation(비용 문구/안내/SP·취소 송신 훅), negativeQuantity(), boundsAssertion(code), islandExists?(region) },
//   gameplay = { setOwner?(sid, owner), request(mayPlace·displace·cancel), confirm(findBuilder·recorderActive·record), notice(moveBuilder·findPriest·containedType·removeContained·immobile·approach·tryMove·start) }.
// 선택되지 않은 일반 건설 경계는 빈 성공으로 대신하지 않고 `캠페인 일반 건설 미연결: 이름` 오류로 그 호출에서 거부한다. 구성 불일치는 첫 생성 전에 `TypeError` 로, 현재 전역 불일치(validateState)는 `Error` 로 거부한다.
// 조립 순서(원문 보존): 실행 부모/저장소 정체성 확인 → 월드/모드/목록/서비스 계약 확인 → validateState → Place → Clear → Notice → Confirm(방송 → 진단 사본 → 같은 wire 직접 처리) → Request → Command → PlayerBounds → LocalGeyser(자원 pool 연결).
// **옮기지 않은 것:** cpppj 의 Native 장기 경로(`UsesNativeTransmissionDevices`: 저장소에서 직접 빌리는 `BindConstructionCore(storage)`/`BindConstructionRuntime(storage)`, 현재 Player base 조회기 `runtime.players`,
//   Player span/방송 객체 없는 세 번째 생성자, `PlayerBounds.BoundsRecord`)는 웹 `RawSurfaceStorage` 에 대응 개념이 없다. 웹은 항상 독립(typed) 경로이며 Client 장기 소유자가 생길 때 같은 자리에 추가한다(4-D 구획 E).
// 기준선 뒤 cpppj 변경 없음(기준선 `225553df` 와 HEAD 소스 동일, 2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E562: 4-D C 구획 RawCampaignConstructionBindings 이식)
import { ISLAND_VTABLE, STALAG_VTABLE } from './raw-bridge-connect.js';
import { ConstructionClearState, RawConstructionClear } from './raw-construction-clear.js';
import { RawConstructionCommand, registerConstructionCommand } from './raw-construction-command.js';
import { ConstructionConfirmState, RawConstructionConfirm, makeConstructionConfirmHooks } from './raw-construction-confirm.js';
import { makeConstructionLocalHooks } from './raw-construction-local.js';
import { ConstructionNoticeState, RawConstructionNotice, makeConstructionNoticeHooks } from './raw-construction-notice.js';
import { createConstructionPacketHeader, finalizeConstructionPacket } from './raw-construction-packet.js';
import { ConstructionPlaceState, RawConstructionPlace, makeConstructionPlaceHooks } from './raw-construction-place.js';
import { ConstructionRequestState, RawConstructionRequest } from './raw-construction-request.js';
import { LocalGeyserState, RawLocalGeyser } from './raw-local-geyser.js';
import { RawPlayerGeyserBounds } from './raw-player-geyser-bounds.js';
import { NO_ISLAND_VTABLE } from './raw-surface-lifecycle.js';
import { RawSurfaceWorld } from './raw-surface-world.js';
import { SQUID_DESTROY_EFFECT } from './squid-destroy.js';

// HP/운반(flags1)과 다이스 genus(flags2)는 일반 건설의 미복원 분기를 요구하므로 noIsland 타입에 있으면 거부한다. (2026-11-15 WB04-E562)
const NO_ISLAND_UNSUPPORTED_FLAGS = 0x8010;
const DAIS_GENUS = 0x400000;
// 실제 임시 패킷·최근 목록·Player 9기록의 물리 크기이다. (2026-11-15 WB04-E562)
const PACKET_BYTES = 512;
const RECENT_MINIMUM = 5;
const PLAYER_BYTES = 9 * 0xb4;
// 배치 통지 패킷의 SID 개수 BYTE 위치와 첫 SID 위치, 통지 메시지 번호와 등록 값(버전 1·정책 0·가변 길이)이다. (2026-11-15 WB04-E562)
const PACKET_SID_COUNT = 26;
const PACKET_FIRST_SID = 27;
const NOTICE_MESSAGE = 0x3f;
const NOTICE_VERSION = 1;
const NOTICE_VARIABLE_LENGTH = 0xffff;
// 원본 그루터기 TYPE(96)과 공통 Squid 가상 표, 슬롯의 TYPE 위치이다. (2026-11-15 WB04-E562)
const STUMP_TYPE = 96;
const BASE_VTABLE = 0x501dd0;
const SLOT_TYPE = 10;
// 통지 observer 가 보고하는 원문 진단 위치이다. (2026-11-15 WB04-E562)
const PACKETS_CPP = '\\Ns\\O\\_p_packets.cpp';
const INVALID_SID_COUNT_LINE = 0x7c9;

// 선택되지 않은 일반 건설 경계를 빈 성공으로 대신하지 않고 필요한 하위 이름을 진단한다. (2026-11-15 WB04-E562)
function constructionMissing(name) {
    return () => { throw new Error(`캠페인 일반 건설 미연결: ${name}`); };
}

// 비어 있는 선택 경계만 명시 오류로 채운다(이미 연결된 실제 하위는 그대로 둔다). (2026-11-15 WB04-E562)
function fillMissing(hooks, names) {
    // 이름표의 각 경계를 확인한다.
    for (const [key, name] of names) {
        if (typeof hooks[key] !== 'function') hooks[key] = constructionMissing(name);
    }
}

// 원본 VT 기록 DWORD 를 호스트 포인터로 실행하지 않고 실제 little endian 값으로 읽는다. (2026-11-15 WB04-E562)
function constructionVtable(raw) {
    return (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
}

// 상태의 현재 패턴 여덟 TYPE 이다(현재 값 조회기가 있으면 그것을, 없으면 독립 배열을 쓴다). (2026-11-15 WB04-E562)
function patternsOf(state) {
    if (typeof state.currentPatternTypes === 'function') return state.currentPatternTypes();
    return typeof state.patternTypesRead === 'function' ? state.patternTypesRead() : state.patternTypes;
}

// 두 패턴 표가 같은 길이의 같은 DWORD 열인지 비교한다(원소마다 전체 DWORD 를 본다). (2026-11-15 WB04-E562)
function samePatterns(first, second) {
    return first.length === second.length && first.every((value, index) => (value >>> 0) === (second[index] >>> 0));
}

// 세션이 끝날 때까지 유지할 실제 건설 전역이다. 호출자가 현재 모드/소유자 값을 직접 갱신한다. (2026-11-15 WB04-E562)
export class RawCampaignConstructionState {
    // 전체 서버 MayPlace/충돌/받침 확정의 현재 전역이다.
    request = new ConstructionRequestState();
    // 실제 0x3e 수신의 차단/비용/로컬 owner 전역이다(blocked: 비영이면 본문을 읽기 전에 반환, chargeBlocked: 비용 선검사 생략, localPlayer: sender 전체 DWORD 와 비교).
    command = { blocked: 0, chargeBlocked: false, localPlayer: 1, chargeBlockedRead: null };
    // 현재 확정/기록/Canon 타입 전역이다.
    confirm = new ConstructionConfirmState();
    // 현재 권한과 예측 조각 정리의 Canon 타입 전역이다.
    clear = new ConstructionClearState();
    // 같은 로컬 소유자/권한과 실제 최근 건설 좌표 이력이다.
    notice = new ConstructionNoticeState();
    // 같은 서버/권한/로컬 소유자와 비용 전역이다.
    place = new ConstructionPlaceState();
    // 실제 0x3f 패킷의 현재 등록 헤더이다.
    packet = createConstructionPacketHeader();
    // 실제 요청 수량과 현재 간헐천/받침 타입 전역이다.
    geyser = new LocalGeyserState();
}

// 독립 World 가 이미 소유한 실제 부품 13개만 고른다. 현재 값/상태를 복사하지 않는다. (2026-11-15 WB04-E562)
export function bindConstructionCore(core) {
    return {
        pool: core.pool, hash: core.hash, spots: core.spots, types: core.types, frames: core.frames, bookkeeping: core.bookkeeping, ownerMode: core.ownerMode, factory: core.factory,
        owner: core.owner, building: core.building, virtualDestroy: core.virtualDestroy, surfaces: core.surfaces, bridgeState: core.bridgeState,
    };
}

// 건설/간헐천의 실제 물리와 가상 생성·공간·삭제 부모를 본체보다 오래 사는 기존 World 에서 빌린다. World 를 새로 만들거나 대여/reset 하지 않는다. (2026-11-15 WB04-E562)
export function bindConstructionRuntime(world) {
    return {
        core: bindConstructionCore(world.core()),
        storage: world.storage,
        create: (type, flags) => world.create(type, flags),
        pop: (sid, x, y, flags) => { world.pop(sid, x, y, flags); },
        destroy: (sid, flags) => world.destroy(sid, flags),
        broadcast: null,
    };
}

// PlayerBounds 가 매번 다시 읽는 현재 상태 뷰를 만든다. battle/metamap 은 같은 월드의 소유자 모드(ownerMode.battle/challenge)를 읽고,
// mission·mapSize·puzzlePieceType·playerDirections·decoderDiagnostics 는 source 의 현재 속성을 읽는다(source 속성은 getter 여도 된다). (2026-11-15 WB04-E562)
export function makePlayerGeyserBoundsState(ownerMode, source) {
    if (!ownerMode || typeof source !== 'object' || source === null) throw new TypeError('Player 간헐천 범위 상태의 소유자 모드/현재 값 소유자 누락');
    return {
        ownerMode,
        get battle() { return ownerMode.battle; },
        get metamap() { return ownerMode.challenge; },
        get mission() { return source.mission; },
        get mapSize() { return source.mapSize; },
        get puzzlePieceType() { return source.puzzlePieceType; },
        get playerDirections() { return source.playerDirections; },
        get decoderDiagnostics() { return source.decoderDiagnostics ?? null; },
    };
}

// 원본 일반 서버 확정/배치 통지와 로컬 간헐천이 같은 실제 월드를 사용한다. 모든 참조는 이 연결보다 오래 살아야 한다. (2026-11-15 WB04-E562)
export class RawCampaignConstructionBindings {
    // 같은 실제 장기 물리와 현재 가상 생성/Pop/삭제 부모(특정 World 타입에 결합하지 않음), 같은 Pool/TYPE/hash/spot/장부의 참조 묶음이다.
    #runtime;
    #core;
    // 현재 모드/타입/수량/이력을 프레임 사본 없이 읽는 전역과 실제 설정/벽시각/표시/진단의 외부 소비자이다.
    #state;
    #effects;
    // 마지막 방송 직후의 진단 사본(활성 Confirm wire 의 owner 가 아니다)과 그 본문/SID 조회 범위이다. 최초 통지 전에는 0 이다.
    #packet = new Uint8Array(PACKET_BYTES);
    #packetLength = 0;
    // 같은 HP/SP/소유자와 실제 Pop 을 쓰는 배치기, 예측 조각 정리기, 직접 수신 통지 처리기, 서버 요청/확정, 0x3e 수신, Player 범위, 로컬 간헐천이다.
    #place;
    #clear;
    #notice;
    #requestBody;
    #command;
    #confirm;
    #bounds;
    #geysers;

    // 첫 생성 전에 같은 저장소/필수 서비스와 실제 일반 건설 경계를 구성한다. (2026-11-15 WB04-E562)
    constructor(source, hp, sp, reward, frames, chunks, players, broadcast, state, bounds, recent, allGeysers, effects, gameplay = {}) {
        const runtime = { ...(source instanceof RawSurfaceWorld ? bindConstructionRuntime(source) : source) };
        // typed 기존 방송만 명시 대여하며 이미 연결된 실제 방송은 보존한다.
        if (typeof runtime.broadcast !== 'function') runtime.broadcast = broadcast ? (packet) => { broadcast.sendMutable(packet); } : null;
        const core = runtime.core;
        const storage = runtime.storage;
        // 생성/reset 없이 실제 owner 정체성만 확인한다.
        if (!storage || !core || core.pool !== storage.pool || core.hash !== storage.hash || core.types !== storage.types || core.spots !== storage.spots
            || typeof runtime.create !== 'function' || typeof runtime.pop !== 'function' || typeof runtime.destroy !== 'function' || typeof runtime.broadcast !== 'function') {
            throw new TypeError('건설 실행 부모의 동일 장기 물리/실제 가상 경계 누락');
        }
        const input = effects ?? {};
        if (core.pool.edition !== 'patch1078' || hp?.pool !== core.pool || hp.bookkeeping !== core.bookkeeping || frames?.length !== core.types.length
            || !(players instanceof Uint8Array) || players.length !== PLAYER_BYTES || recent === allGeysers || !(recent?.entries?.length >= RECENT_MINIMUM)
            || bounds?.ownerMode !== core.ownerMode
            || typeof input.configInt !== 'function' || typeof input.wallSeconds !== 'function' || typeof input.refresh !== 'function'
            || typeof input.negativeQuantity !== 'function' || typeof input.boundsAssertion !== 'function') {
            throw new TypeError('캠페인 받침/간헐천의 같은 월드/현재 모드/전용 목록/실제 서비스 계약 오류');
        }
        this.#runtime = runtime;
        this.#core = core;
        this.#state = state;
        this.#effects = input;
        this.#validateState();
        const assertion = (expression, file, line) => { storage.assertDisplay(expression, file, line); };
        const destroy = (sid, flags) => { this.#destroy(sid, flags); };
        // 현재 비용/UI 소비자를 사용하며 비어 있는 선택 경계는 명시 오류다. 실제 wire Place 의 진단도 같은 Client 진단으로 정상 복귀한다.
        let placeHooks = { ...(input.presentation ?? {}) };
        placeHooks.notifySurface = (sid) => { core.building.notifySurface(sid); };
        placeHooks.pop = (sid, x, y, flags) => { runtime.pop(sid, x, y, flags); };
        placeHooks.assertion = assertion;
        fillMissing(placeHooks, [['sendMoney', 'SP 통지'], ['format', '비용 안내 서식'], ['tellOthers', '다른 Player 비용 안내'], ['tellPlayer', 'Player 비용 안내'], ['tell', '로컬 비용 안내'], ['sendReject', '배치 취소 통지']]);
        placeHooks = makeConstructionPlaceHooks(core.pool, core.factory, core.owner, hp, sp, state.place, placeHooks);
        // 내려놓은 사제의 원래 소유자와 전투 표식을 가상 몸체가 복원한다.
        if (typeof gameplay.setOwner === 'function') placeHooks.setOwner = gameplay.setOwner;
        this.#place = new RawConstructionPlace(core.pool, core.types, frames, state.place, hp, placeHooks);
        this.#place.bindPlayerRecords(players);
        // 삭제는 관찰/목록 제거가 아닌 같은 가상 수명 전체를 실행한다.
        this.#clear = new RawConstructionClear(core.pool, core.hash, core.types, frames, state.clear, { destroy });
        // 같은 실제 Session 의 운반/사제/시작을 선택 시 호출한다. 정리/배치/환불/삭제/갱신은 내부가 연결한다.
        const noticeHooks = { ...(gameplay.notice ?? {}) };
        fillMissing(noticeHooks, [['moveBuilder', '건설 운반 생성/이동'], ['findPriest', '소유 사제 조회'], ['containedType', '사제 contained 타입'], ['removeContained', '사제 contained 삭제'],
            ['immobile', '사제 이동 불가'], ['approach', '사제 접근점'], ['tryMove', '사제 건설 접근 이동']]);
        noticeHooks.destroy = destroy;
        fillMissing(noticeHooks, [['start', '건설 Process 시작']]);
        noticeHooks.refresh = input.refresh;
        this.#notice = new RawConstructionNotice(core.pool, core.types, state.notice, makeConstructionNoticeHooks(core.pool, this.#clear, this.#place, noticeHooks));
        // 서버 SID 와 실제 wire 전송 뒤 원본처럼 같은 통지를 직접 처리한다. 기준점과 선택적 실제 기록기는 호출자가 준다.
        const confirmHooks = { ...(gameplay.confirm ?? {}) };
        fillMissing(confirmHooks, [['findBuilder', '작업장/Graph 기준점']]);
        confirmHooks.transmitAndHandle = (current) => {
            // 각 대상의 실제 송신 관문 뒤 observer 가 같은 호출별 512BYTE 를 변경한다.
            runtime.broadcast(current);
            // 진단 조회만 사본에 보관하고 직접 수신은 계속 지역 current 를 사용한다.
            this.#packet.set(current.subarray(0, Math.min(current.length, PACKET_BYTES)));
            // NULL peer 이면 선언 WORD 를 꾸미지 않고 본문 범위만 기존 조회 계약으로 보인다.
            this.#packetLength = Math.min(current.length, PACKET_FIRST_SID + current[PACKET_SID_COUNT] * 2);
            this.#notice.actualHandle(current);
        };
        confirmHooks.assertion = assertion;
        fillMissing(confirmHooks, [['recorderActive', '기록기 활성 조회'], ['record', '기록기 첫 SID 기록']]);
        this.#confirm = new RawConstructionConfirm(core.pool, core.types, frames, state.confirm, makeConstructionConfirmHooks(core.pool, core.factory, confirmHooks));
        // 명시된 실제 Session 하위만 사용한다. Confirm 은 내부가 연결한다.
        const requestHooks = { ...(gameplay.request ?? {}) };
        fillMissing(requestHooks, [['mayPlace', '전체 MayPlace'], ['displace', '발자국 유닛 밀어내기'], ['cancel', '실패 섬 정리']]);
        requestHooks.confirm = (value) => this.#confirm.actualConfirm(value, state.packet);
        this.#requestBody = new RawConstructionRequest(core.pool, core.hash, core.types, frames, core.spots, state.request, requestHooks);
        // 비용 선검사와 실패 환불도 같은 실제 Place 이다.
        this.#command = new RawConstructionCommand(core.types, state.command, {
            money: (owner) => this.#place.money(owner),
            shortfall: (value) => { this.#place.chargeShortfall(value.type, value.player, value.x, value.y, value.argument, value.direction); },
            request: (value) => this.#requestBody.request(value),
            reject: (value) => { this.#place.reject(value.type, value.player, value.x, value.y, value.argument, value.direction); },
        });
        // 독립 span 은 기존 생성 시 9기록 계약을 유지한다.
        this.#bounds = new RawPlayerGeyserBounds(core.pool, chunks, core.types, core.frames, players, bounds, input.boundsAssertion);
        // factory/Pop/삭제/받침 모두 현재 같은 월드의 실제 몸체다.
        this.#geysers = new RawLocalGeyser(core.pool, core.hash, core.spots, core.types, core.frames, players, recent, state.geyser, {
            configInt: input.configInt,
            wallSeconds: input.wallSeconds,
            negativeQuantity: input.negativeQuantity,
            regionExists: input.islandExists,
            playerBounds: (player) => this.#bounds.bounds(player),
            destroy,
            confirm: (request) => { this.#confirmSupport(request); },
            create: (type, flags) => runtime.create(type, flags),
            pop: (sid, x, y, flags) => { runtime.pop(sid, x, y, flags); },
        });
        // 삭제 보상과 로컬 생성이 같은 자원 pool 을 읽고 쓴다(사본 없음).
        this.#geysers.bindResourcePool({
            get value() { return reward.currentGeyserPool(); },
            set value(next) { reward.storeGeyserPool(next); },
        });
    }

    // 실제 현재 전역의 불일치/일반 건설 분기를 생성 전에 거부한다. 지원 범위를 임의 성공으로 넓히지 않는다. (2026-11-15 WB04-E562)
    #validateState() {
        const state = this.#state;
        const core = this.#core;
        // 같은 현재 owner 만 검증하고 준비 사본으로 정상 변경을 거부하지 않는다.
        const type = state.place.noIslandTypeNow();
        const patterns = patternsOf(state.confirm);
        if (type >= core.types.length || type !== (core.bridgeState.noIslandType >>> 0) || type !== state.geyser.readNoIslandType()
            || type !== (patterns[3] >>> 0) || !samePatterns(patterns, patternsOf(state.clear))
            || !samePatterns(patterns, patternsOf(state.place)) || state.place.bridgeTypeNow() !== (state.clear.bridgeType >>> 0)
            || state.place.bridgeTypeNow() !== (core.bridgeState.bridgeType >>> 0) || state.place.localPlayerNow() !== state.notice.localPlayerNow()
            || state.place.authorityNow() !== state.clear.authorityNow() || state.place.authorityNow() !== state.notice.authorityNow()
            || state.place.authorityNow() !== state.geyser.readBoss() || state.geyser.readFort() !== Boolean(core.ownerMode.fort)
            || ((core.types[type].flags1 >>> 0) & NO_ISLAND_UNSUPPORTED_FLAGS) || ((core.types[type].flags2 >>> 0) & DAIS_GENUS)
            || type === state.notice.daisTypeNow()) {
            throw new Error('캠페인 건설/간헐천의 현재 전역 불일치');
        }
    }

    // 원본 LocalGeyser 의 명시 0 인자만 허용하여 커서/SP/운반 미연결을 통과하지 못하게 한다. (2026-11-15 WB04-E562)
    #confirmSupport(request) {
        this.#validateState();
        if (request.type !== this.#state.place.noIslandTypeNow() || request.player || request.argument || request.direction || request.flags
            || request.timeLow || request.timeHigh || request.quality) {
            throw new Error('캠페인 받침 확정은 LocalGeyser의 noIsland/Player0 요청만 지원합니다');
        }
        return this.#confirm.actualConfirm(request, this.#state.packet);
    }

    // 가상 삭제 거부를 성공한 예측 정리/empty 교체로 간주하지 않는다. (2026-11-15 WB04-E562)
    #destroy(sid, flags) {
        if (!this.#runtime.destroy(sid, flags)) throw new Error('캠페인 받침/간헐천의 실제 가상 삭제 거부');
    }

    // 실제 표면/받침/종유석의 확인된 가상 표만 표면 수명으로 분배한다. 살아 있는 받침도 DropIsland 가 정리한다. (2026-11-15 WB04-E562)
    #support(sid) {
        const core = this.#core;
        const raw = core.pool.slot(sid);
        const table = constructionVtable(raw);
        if (raw[SLOT_TYPE] === this.#state.place.noIslandTypeNow() && table === NO_ISLAND_VTABLE) return true;
        // 94 의 고유 pre(442240)·157 의 고유 post(442640)는 기존 표면 수명이 구별한다. 원본 96 그루터기의 생성자 0 은 공통 Squid 표/수명이다.
        return (raw[SLOT_TYPE] === (core.bridgeState.islandType >>> 0) && table === ISLAND_VTABLE)
            || (raw[SLOT_TYPE] === (core.bridgeState.stalagType >>> 0) && table === STALAG_VTABLE)
            || (raw[SLOT_TYPE] === STUMP_TYPE && table === BASE_VTABLE);
    }

    // 일반 서버 요청도 원본 확정/통지/배치 몸체를 그대로 사용한다. 배치 가능 판정은 호출자의 선행 책임이다. (2026-11-15 WB04-E562)
    confirm(request) {
        this.#validateState();
        return this.#confirm.actualConfirm(request, this.#state.packet);
    }

    // 로컬 예측은 서버 확정/통지와 같은 실제 배치기를 사용하며 나머지 Form/반환/송신 경계를 유지한다. (2026-11-15 WB04-E562)
    bindLocal(hooks) { return makeConstructionLocalHooks(this.#core.pool, this.#core.factory, this.#place, hooks); }

    // 전체 요청의 실제 MayPlace/abstract 충돌/밀어내기/받침/본 확정을 차례로 실행한다. (2026-11-15 WB04-E562)
    request(value) {
        this.#validateState();
        return this.#requestBody.request(value);
    }

    // 원본 0x3e 수신은 요청 실패 때 같은 실제 Place 에서 환불/취소를 요청한다. (2026-11-15 WB04-E562)
    receive(sender, packet) {
        this.#validateState();
        this.#command.receive(sender, packet);
    }

    // 기존 3e 등록과 같은 raw command 로 직접 들어가며 추가 validateState 를 선행하지 않는다. (2026-11-15 WB04-E562)
    receiveCommandPacket(sender, packet) { this.#command.receive(sender, packet); }

    // 실제 3fh 는 요청/받침 validateState 를 선행하지 않고 같은 현재 BYTE 통지 본문으로 들어간다. (2026-11-15 WB04-E562)
    receiveNotice(packet) { this.#notice.actualHandle(packet); }

    // Cast/Drop 의 잘못된 소유자도 기존 원본 환불 몸체(442a40)를 사용한다. (2026-11-15 WB04-E562)
    refund(type, owner) {
        this.#validateState();
        this.#place.refund(type, owner);
    }

    // 비용 부족 뒤 계속하는 서버 분기는 같은 원본 취소 통지(00442af0)를 요청한다. (2026-11-15 WB04-E562)
    reject(type, owner, x, y, argument, direction) {
        this.#validateState();
        this.#place.reject(type, owner, x, y, argument, direction);
    }

    // 같은 이미 복원된 442b50 계산 본체를 실제 6ch/운반 실패와 공유한다. (2026-11-15 WB04-E562)
    chargeShortfall(type, owner, x, y, argument, direction) {
        this.#validateState();
        this.#place.chargeShortfall(type, owner, x, y, argument, direction);
    }

    // 분배기/전송 계층의 실제 수신 지점에 현재 몸체를 연결한다. prepare 는 두 원문 수신 입구가 빌리는 현재 Session 전역 준비 장치이다(선택). (2026-11-15 WB04-E562)
    register(dispatch, prepare = null) {
        const storage = this.#runtime.storage;
        registerConstructionCommand(dispatch, this.#command, prepare);
        dispatch.observePacket(NOTICE_MESSAGE, (packet, sending) => {
            finalizeConstructionPacket(packet, Boolean(sending), () => { storage.assertDisplay('sidCount != (unsigned char)-1', PACKETS_CPP, INVALID_SID_COUNT_LINE); });
        });
        // 40b4b0/52ecbe 의 실제 가변 통지 metadata 이다.
        dispatch.version(NOTICE_MESSAGE, NOTICE_VERSION, 0);
        dispatch.requireLength(NOTICE_MESSAGE, NOTICE_VARIABLE_LENGTH);
        // sender/nullable 연결을 원문처럼 사용하지 않는다.
        dispatch.registerDirect(NOTICE_MESSAGE, (sender, connection, packet) => {
            if (typeof prepare === 'function') prepare();
            this.receiveNotice(packet);
        });
    }

    // LocalGeyser 전용 입구는 기존 noIsland/player 0/나머지 0 계약을 유지한다. (2026-11-15 WB04-E562)
    confirmSupport(request) { return this.#confirmSupport(request); }

    // 실제 로그인/Player 초기화 뒤 미션 부모가 지정한 위치에서만 호출한다. 현재 Player 9 와 실제 PlayerBounds 를 읽어 원본 로컬 생성 부모를 실행한다. (2026-11-15 WB04-E562)
    generateLocalGeysers() {
        this.#validateState();
        this.#geysers.generate();
    }

    // 전체 맵 부모도 같은 현재 Pool/자원/최근 목록과 일반 제외·허용 인수를 사용한다. 각 생성 요청 직전에 같은 실제 건설 전역의 정합성을 확인한다. (2026-11-15 WB04-E562)
    populateGeysers(quantity, random, rectangle, options) {
        this.#validateState();
        this.#geysers.populate(quantity, random, rectangle, options);
    }

    // 최근 생성/자원/풀의 실제 단일 저장소를 빌린다. 독립 간헐천 모형을 만들지 않는다. (2026-11-15 WB04-E562)
    get localGeysers() { return this.#geysers; }

    // 현재 Player/청크 지도의 기존 정밀 몸체를 공유한다. (2026-11-15 WB04-E562)
    get playerBounds() { return this.#bounds; }

    // 마지막 방송 직후 진단 사본의 본문/SID 범위를 빌린다. 실제 선언 WORD/전송 길이와 별개이며 다음 방송 때 덮어쓴다. (2026-11-15 WB04-E562)
    lastPacket() { return this.#packet.subarray(0, this.#packetLength); }

    // 실제 표면/받침/종유석의 Pre/Post 를 기존 표면 수명으로 분배하고 다른 파생 효과는 fallback 에 보존한다. (2026-11-15 WB04-E562)
    destroyHooks(fallback = {}) {
        const rest = fallback ?? {};
        const surfaces = this.#core.surfaces.hooks();
        return {
            ...rest,
            emit: (event) => {
                if (this.#support(event.sid) && (event.effect === SQUID_DESTROY_EFFECT.preDestroy || event.effect === SQUID_DESTROY_EFFECT.postDestroy)) {
                    surfaces.emit(event);
                    return;
                }
                if (typeof rest.emit !== 'function') throw new Error('다른 캠페인 건설 자산 삭제 효과 미연결');
                rest.emit(event);
            },
        };
    }

    // 예측 Clear 와 실제 DropIsland 가 요청한 검증된 받침들의 삭제를 같은 Pool/Graph/종속 수명에 연결한다. 다른 자산은 fallback 이 맡는다. (2026-11-15 WB04-E562)
    destroy(fallback = null) {
        return (sid, flags, hooks) => {
            if (this.#support(sid)) return this.#core.virtualDestroy.destroy(sid, flags, hooks);
            if (typeof fallback !== 'function') throw new Error('다른 캠페인 건설 자산 가상 삭제 미연결');
            return fallback(sid, flags, hooks);
        };
    }

    // 같은 월드의 생성/삭제/공간을 공유하는지 상위 연결에서 확인하는 실제 풀이다. (2026-11-15 WB04-E562)
    get pool() { return this.#core.pool; }
}
