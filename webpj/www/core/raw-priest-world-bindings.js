// 사제·보호막·Carrier의 기존 몸체를 같은 월드 풀·공간·Host에 연결한다(cpppj client/RawPriestWorldBindings, 225553df).
// 사용: 생성 → bindProcesses/bindDamageable/bindPlacement → validateReady. Kernel을 비운 뒤 dispose로 등록과 Player 대여를 반납한다.
// Native 장기 몸체는 effects.shared*로 빌리며 실제 Storage 소유자의 설치는 4-D E 구획에서 수행한다. (2026-11-16 WB04-E563)
import { RawMovementProcessTypes, selectMovementProcessType } from './raw-movement-process-types.js';
import { PriestOwnerState, RawPriestOwner, PRIEST_TYPE, PRIEST_VTABLE } from './raw-priest-owner.js';
import { PriestHitPointMode, RawPriestState } from './raw-priest-state.js';
import { PriestForcefieldState, RawPriestForcefield, makePriestForcefieldHooks } from './raw-priest-forcefield.js';
import { PriestShieldState, RawPriestShield, makePriestShieldCreationHooks } from './raw-priest-shield.js';
import { PriestFallState, RawPriestFall, PRIEST_FALL_EVENT, makePriestFallFrameHooks, makePriestFallWalker } from './raw-priest-fall.js';
import { RawFrameAdvance } from './raw-frame-advance.js';
import { RawCarrierCheck } from './raw-carrier-check.js';
import { CarrierPostPopState, RawCarrierPostPop, makeCarrierPostPopHooks } from './raw-carrier-post-pop.js';
import { PriestPostPopState, RawPriestPostPop, PRIEST_REGEN_EVENT } from './raw-priest-post-pop.js';
import { RawPriestPostPopTail, makePriestPostPopTailShieldHooks } from './raw-priest-post-pop-tail.js';
import { ContainedFinderState } from './raw-contained-finder.js';
import { RawCarrierPreDestroy, makeCarrierPreDestroyHooks } from './raw-carrier-pre-destroy.js';
import { RawPriestPreDestroy, makePriestPreDestroyHooks } from './raw-priest-pre-destroy.js';
import { ForcefieldRegularState, RawForcefieldPostPop, RawForcefieldRegular, PATCH_FORCEFIELD_VTABLE, makeForcefieldFrameHooks } from './raw-forcefield-lifecycle.js';
import { RawContainedLookup } from './raw-contained-lookup.js';
import { PriestRegenState, RawPriestRegen } from './raw-priest-regen.js';
import { PriestSpawnState, RawPriestSpawn } from './raw-priest-spawn.js';
import { runPriestRegular } from './raw-priest-regular.js';

// Player 9기록의 PATCH 물리 폭, 지도 칸 수, 슬롯 TYPE/상태 위치다. (2026-11-16 WB04-E563)
const PLAYER_BYTES = 9 * 0xb4;
const MAP_CELLS = 65536;
const TYPE_AT = 10;
const STATE_AT = 11;

// 같은 Client 권한 주소는 Session이 바뀌어도 같은 읽기 함수/내용물 뷰로 빌린다. 키가 사라지면 함께 반납된다. (2026-11-16 WB04-E563)
const AUTHORITY_REFS = new WeakMap();

// C++의 &authority.boss와 같은 주소 정체성을 다음 Session에서도 보존한다. (2026-11-16 WB04-E563)
function authorityRefs(authority) {
    if (!AUTHORITY_REFS.has(authority)) AUTHORITY_REFS.set(authority, { read: () => authority.boss, view: { get value() { return authority.boss; } } });
    return AUTHORITY_REFS.get(authority);
}

// 원본 DWORD 가상 표를 정렬 가정 없이 읽는다. (2026-11-16 WB04-E563)
function vtable(raw) { return (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0; }

// C++의 선택형 외부 장치를 호출 지점의 명시 오류로 보존한다. (2026-11-16 WB04-E563)
function selected(hooks, names) {
    const result = { ...hooks };
    // 생성 시 요구하지 않은 장치도 선택된 순간에는 누락을 알린다.
    for (const name of names) if (typeof result[name] !== 'function') result[name] = () => { throw new Error(`사제 월드 선택 장치 미연결: ${name}`); };
    return result;
}

// 미션 결과·HP·Carrier가 직접 공유하는 상태이며 목록 용량은 월드 초기화가 정한다. (2026-11-16 WB04-E563)
export class RawPriestWorldState {
    // 사제 목록/원소유자 표와 현재 소유자·HP·보호막 전역이다.
    priests = new PriestPostPopState();
    owner = new PriestOwnerState();
    hitPoints = new PriestHitPointMode();
    shield = new PriestShieldState();
    // 회복·낙하·보호막 조회·생성의 실제 상태다.
    regen = new PriestRegenState();
    fall = new PriestFallState();
    forcefield = new PriestForcefieldState();
    spawn = new PriestSpawnState();
    // Carrier 후처리·내용물 검색·보호막 정기 전역이다.
    carrier = new CarrierPostPopState();
    contained = new ContainedFinderState();
    forcefieldRegular = new ForcefieldRegularState();
    // 현재 이동 역할과 같은 사제 TYPE 주소를 읽는 고정 getter다.
    movement;
    priestType;

    // 실제 Client TYPE/역할 owner 또는 독립 기본값을 연결한다. (2026-11-16 WB04-E563)
    constructor(selection = {}) {
        this.movement = new RawMovementProcessTypes(selection);
        this.priestType = () => this.movement.priest;
    }
}

// 기존 하위 계산기와 고정 주소를 소유하며 상위 월드와 같은 물리를 공유한다. (2026-11-16 WB04-E563)
export class RawPriestWorldBindings {
    // 외부 단일 월드·상태·효과와 지연 연결 세 몸체다.
    #core; #state; #effects; #host = null; #damageable = null; #placement = null;
    // 현재 권한 소유자다. 읽기 함수/내용물 뷰는 같은 소유자의 고정 참조 표에서 빌린다.
    #authority = null;
    // 사제/보호막/Carrier의 실제 하위 몸체다.
    #lookup; #priest; #owner; #shield; #fall; #check; #carrierPost; #prefix; #tail;
    #carrierPre; #priestPre; #fieldPost; #fieldRegular; #regen; #spawn;

    // core는 RawPriestWorldCore와 같은 이름의 참조 묶음이며 surfaces는 반드시 hash.view(0)이다. (2026-11-16 WB04-E563)
    constructor(core, state, effects) {
        const { pool, hash, types, frames, spots, surfaces, reward, random, factory, owner, ownerMode, postPop, frame } = core;
        if ([reward, factory, owner, postPop, frame].some((body) => body?.pool !== pool) || types.length !== frames.length
            || spots.length !== MAP_CELLS || surfaces.length !== MAP_CELLS || surfaces !== hash.view(0)
            || ['pop', 'unpop', 'repop', 'destroy', 'claimGround', 'ownerNotice', 'notifySurface', 'dependentFound', 'reserveRegular', 'fallSound'].some((name) => typeof effects?.[name] !== 'function')) {
            throw new TypeError('사제 월드의 풀/공간/필수 효과 연결 오류');
        }
        this.#core = core; this.#state = state; this.#effects = effects;
        state.priests.currentPriests().items();
        const fallFrames = frames.map((codes) => ({ falling: codes.run('J').first, landed: codes.run('E').first }));
        this.#lookup = this.#shared(effects.sharedForcefield, () => new RawPriestForcefield(pool, hash, types, state.forcefield));
        this.#priest = new RawPriestState(pool, reward, types, spots);
        this.#priest.bindCurrentTypes(types);
        this.#owner = new RawPriestOwner(pool, ownerMode, state.owner, {
            base: (sid, player) => { owner.set(sid, player); }, notify: effects.ownerNotice, assertion: effects.ownerAssertion,
        });
        this.#shield = new RawPriestShield(pool, this.#lookup, state.shield, makePriestShieldCreationHooks(pool, factory, owner, {
            ...selected(effects.shield, ['reserveSound', 'loadSound', 'attachSound', 'noticeSound', 'translate', 'tell']), pop: effects.pop,
        }));
        const advance = new RawFrameAdvance(pool, frames, frame);
        this.#fall = new RawPriestFall(pool, this.#priest, frames, fallFrames, surfaces, state.fall, makePriestFallFrameHooks(frame, advance, {
            ensureShield: (sid) => { this.#shield.ensure(sid); }, reserveRegular: effects.reserveRegular,
            addSharedRegular: (sid, event, payload) => { this.#requireHost().addSharedRegular(sid, event, payload); },
            unpop: effects.unpop, repop: effects.repop, sound: effects.fallSound, clearShield: (sid) => { this.#clearShield(sid); },
        }));
        this.#check = new RawCarrierCheck(pool, types, spots, makePriestFallWalker(pool, this.#fall, effects.otherWalkerFall));
        this.#carrierPost = new RawCarrierPostPop(pool, types, state.carrier, makeCarrierPostPopHooks(pool, postPop,
            (sid) => { this.#check.check(sid); }, effects.claimGround));
        this.#carrierPost.bindCurrentTypes(types);
        this.#prefix = new RawPriestPostPop(pool, reward, state.priests, {
            findRegular: (sid, event) => this.#requireHost().findEvent(sid, event) !== null,
            reserveRegular: effects.reserveRegular,
            addRegular: (sid, event, payload) => { this.#requireHost().addRegular(sid, event, payload); },
        }, state.priestType);
        this.#tail = new RawPriestPostPopTail(pool, this.#prefix, this.#carrierPost, this.#priest, frames, spots,
            makePriestPostPopTailShieldHooks(pool, this.#shield, this.#lookup, effects.destroy, (sid) => { this.#fall.begin(sid); }));
        this.#carrierPre = this.#shared(effects.sharedCarrier, () => new RawCarrierPreDestroy(pool, state.contained, {
            validateDamageable: (sid, flags) => { this.#requireDamageable().validate(sid, flags); },
            damageablePre: (sid, flags) => { this.#requireDamageable().preDestroy(sid, flags); }, dependentFound: effects.dependentFound,
        }));
        this.#priestPre = this.#shared(effects.sharedPriestPre, () => new RawPriestPreDestroy(pool, state.priests,
            makeCarrierPreDestroyHooks(pool, this.#carrierPre, makePriestForcefieldHooks(pool, this.#lookup, { destroyForcefield: effects.destroy }))));
        this.#fieldPost = new RawForcefieldPostPop(pool, {
            reserveRegular: effects.reserveRegular, addRegular: (sid, event, payload) => { this.#requireHost().addRegular(sid, event, payload); },
        });
        if (state.forcefieldRegular.sharedPriestType && state.forcefieldRegular.sharedPriestType !== state.priestType) throw new TypeError('보호막 Regular 현재 사제 TYPE 소유자 불일치');
        state.forcefieldRegular.sharedPriestType = state.priestType;
        this.#fieldRegular = this.#shared(effects.sharedForcefieldRegular, () => new RawForcefieldRegular(pool, hash, types, state.forcefieldRegular,
            makeForcefieldFrameHooks(pool, advance, { destroy: effects.destroy, assertion: effects.forcefieldAssertion, findTypeAt: effects.findTypeAt })));
        const contained = new RawContainedLookup(pool, effects.destroy);
        this.#regen = new RawPriestRegen(pool, reward, this.#priest, state.hitPoints, { unpop: effects.unpop, repop: effects.repop }, random, state.regen, {
            ...selected(effects.regen, ['refresh', 'neutralAction', 'patchAudit', 'measurement']),
            damageable: (sid) => { this.damageableRefresh(sid); }, shape: (type) => this.movementProcessType(type),
            occupied: (sid, process) => contained.find(sid, process) !== 0,
        });
        this.#spawn = new RawPriestSpawn(pool, types, spots, state.spawn, {
            mayPlace: (query) => this.#requirePlacement().mayPlace(query), create: (type, flags) => factory.create(type, flags),
            setOwner: (sid, player) => { this.setOwner(sid, player); }, pop: effects.pop, notifySurface: effects.notifySurface,
            checkCarrier: (sid) => { this.#check.check(sid); },
        });
        // 모든 하위 생성이 성공한 뒤에만 후처리의 가상 지원을 등록한다.
        postPop.setBody('priest', this.#tail); postPop.setBody('forcefield', this.#fieldPost);
    }

    // 장기 몸체는 같은 풀만 빌리고 독립 경로는 기존 지역 몸체를 만든다. (2026-11-16 WB04-E563)
    #shared(body, create) {
        if (!body) return create();
        if (body.pool !== this.#core.pool) throw new TypeError('사제/장기 몸체 Pool 불일치');
        return body;
    }
    // 지연 연결이 필요한 실제 선택 지점에서만 누락을 보고한다. (2026-11-16 WB04-E563)
    #requireHost() { if (!this.#host) throw new Error('사제 월드 ProcessHost 미연결'); return this.#host; }
    #requireDamageable() { if (!this.#damageable) throw new Error('사제 월드 Damageable 삭제 몸체 미연결'); return this.#damageable; }
    #requirePlacement() { if (!this.#placement) throw new Error('사제 월드 배치 검사 몸체 미연결'); return this.#placement; }
    // 현재 조회 결과가 주소 범위 안이면 같은 가상 삭제를 요청한다. (2026-11-16 WB04-E563)
    #clearShield(sid) { const field = this.#lookup.find(sid); if (field > 0 && field < this.pool.capacity) this.#effects.destroy(field, 0); }
    // 순환 참조 세 칸은 같은 풀의 최초 연결만 허용한다. (2026-11-16 WB04-E563)
    bindProcesses(host) { if (host?.pool !== this.pool || this.#host) throw new TypeError('사제 ProcessHost 풀/중복 연결 오류'); this.#host = host; }
    bindDamageable(body) { if (body?.pool !== this.pool || this.#damageable) throw new TypeError('사제 Damageable 풀/중복 연결 오류'); this.#damageable = body; }
    bindPlacement(body) { if (body?.pool !== this.pool || this.#placement) throw new TypeError('사제 Placement 풀/중복 연결 오류'); this.#placement = body; }
    // 첫 틱 전에 세 지연 연결을 검사한다. (2026-11-16 WB04-E563)
    validateReady() { this.#requireHost(); this.#requireDamageable(); this.#requirePlacement(); }
    // 로딩 전역 네 칸을 함께 갱신한다. (2026-11-16 WB04-E563)
    setLoadingDepth(depth) {
        this.#state.owner.loadingDepth = depth >>> 0; this.#state.shield.loadingDepth = depth >>> 0;
        this.#state.carrier.loadingDepth = depth >>> 0; this.#state.regen.neutralBlocked = (depth >>> 0) !== 0;
    }
    // 독립 권한만 값으로 갱신하며 대여된 주소는 덮지 않는다. (2026-11-16 WB04-E563)
    setAuthority(authority) {
        const state = this.#state;
        if (state.hitPoints.sharedAuthority || state.fall.sharedAuthority || state.carrier.sharedBoss || state.contained.bossOwner) throw new Error('공유 권한을 대여한 사제 실행기의 값형 권한 변경 금지');
        state.hitPoints.authority = Boolean(authority); state.fall.authority = Boolean(authority);
        state.carrier.boss = Boolean(authority); state.contained.boss = Boolean(authority);
    }
    // 네 소비자가 같은 boss 주소를 읽도록 고정 getter와 뷰를 빌린다. (2026-11-16 WB04-E563)
    bindAuthority(authority) {
        if (!authority || !('boss' in authority) || (this.#authority && this.#authority !== authority)) throw new TypeError('사제 네 소비자의 동일 공유 권한 주소 필요');
        const { read, view } = authorityRefs(authority);
        const state = this.#state;
        if ([state.hitPoints.sharedAuthority, state.fall.sharedAuthority, state.carrier.sharedBoss].some((old) => old && old !== read)
            || (state.contained.bossOwner && state.contained.bossOwner !== view)) throw new TypeError('사제 네 소비자의 동일 공유 권한 주소 필요');
        this.#authority = authority;
        state.hitPoints.sharedAuthority = read; state.fall.sharedAuthority = read; state.carrier.sharedBoss = read; state.contained.bindBoss(view);
    }
    // Player9의 같은 물리 바이트 범위만 빌리며 실패 시 기존 대여를 보존한다. (2026-11-16 WB04-E563)
    bindPlayerRecords(players) {
        const current = this.#state.hitPoints.playerRecords;
        if (!(players instanceof Uint8Array) || players.length !== PLAYER_BYTES || (current.length && (current.buffer !== players.buffer || current.byteOffset !== players.byteOffset))) throw new TypeError('사제 HP의 같은 Player9 고정 기록 필요');
        this.#state.hitPoints.playerRecords = players;
    }
    // 원본 로컬 Player 범위를 검사한 뒤 owner 표식과 안내를 함께 갱신한다. (2026-11-16 WB04-E563)
    setLocalPlayer(player) {
        if (!Number.isInteger(player) || player < 0 || player > 8) throw new RangeError('사제 로컬 플레이어 범위 오류');
        this.#state.owner.battlePlayer = player; this.#state.owner.fortPlayer = player; this.#state.shield.localPlayer = player;
    }
    // 사제만 원소유자 재정의를 거치고 다른 자산은 현재 외부 분배 또는 공통 부모다. (2026-11-16 WB04-E563)
    setOwner(sid, player) {
        if (this.#owner.handles(sid)) this.#owner.set(sid, player);
        else if (this.#effects.nonPriestOwner) this.#effects.nonPriestOwner(sid, player);
        else this.#core.owner.set(sid, player);
    }
    // 사제 HP 변경의 실제 공간 전환을 공유한다. (2026-11-16 WB04-E563)
    setHitPoints(sid, value) { this.#priest.setHitPoints(sid, value, this.#state.hitPoints, this.#effects); }
    // Carrier 지면 검사와 현재 사제 낙하 가상 몸체를 공유한다. (2026-11-16 WB04-E563)
    checkCarrier(sid) { return this.#check.check(sid); }
    fallWalker(sid) { return makePriestFallWalker(this.pool, this.#fall, this.#effects.otherWalkerFall)(sid); }
    // 원문 우선순위로 현재 이동 역할 전역을 읽는다. (2026-11-16 WB04-E563)
    movementProcessType(type) { return selectMovementProcessType(this.#core.types, this.#state.movement, type); }
    // damageEffect가 0이면 후속 생성·소리 장치를 요구하지 않는다. (2026-11-16 WB04-E563)
    damageableRefresh(sid) {
        const raw = this.pool.slot(sid);
        if (raw[STATE_AT] & 1) return;
        if (raw[TYPE_AT] >= this.#core.types.length) throw new RangeError('Damageable 현재 타입 범위 오류');
        if (this.#core.types[raw[TYPE_AT]].damageEffect === 0) return;
        if (typeof this.#effects.regen?.damageable !== 'function') throw new Error('nonzero damageEffect의 burning 전체 몸체 미연결');
        this.#effects.regen.damageable(sid);
    }
    // 고유 회복/낙하만 직접 분배하고 다른 고유 사건은 상위 분배가 맡는다. (2026-11-16 WB04-E563)
    uniqueRegular(sid, event, count, payload) {
        if (event === PRIEST_REGEN_EVENT) return this.#regen.handle(sid, event, count, payload);
        if (event === PRIEST_FALL_EVENT) return this.#fall.handle(sid, event, count, payload);
        throw new RangeError('다른 사제 회복/추락 사건');
    }
    // TYPE와 실제 VT를 먼저 대조하여 사제/보호막 사건을 같은 Host에 분배한다. (2026-11-16 WB04-E563)
    regularHandler(fallback = null) {
        return (sid, event, count, payload) => {
            const raw = this.pool.slot(sid); const table = vtable(raw);
            if (raw[TYPE_AT] === PRIEST_TYPE && table === PRIEST_VTABLE) return runPriestRegular(sid, event, count, payload, {
                destroy: this.#effects.destroy, carrier: fallback,
                unique: (parent, code, number, value) => {
                    if (code === PRIEST_REGEN_EVENT || code === PRIEST_FALL_EVENT) return this.uniqueRegular(parent, code, number, value);
                    if (!fallback) throw new Error('사제 월드 고유 정기 미연결');
                    return fallback(parent, code, number, value);
                },
            });
            if (table === PATCH_FORCEFIELD_VTABLE) return this.#fieldRegular.handle(sid, event, count, payload);
            return fallback ? fallback(sid, event, count, payload) : payload;
        };
    }
    // 기존 Form/선택/재귀 훅을 보존하며 사제 preDestroy만 추가한다. (2026-11-16 WB04-E563)
    destroyHooks(fallback) { return makePriestPreDestroyHooks(this.pool, this.#priestPre, fallback); }
    // 후속 소비자가 같은 실제 몸체와 목록을 재사용하는 접근자다. (2026-11-16 WB04-E563)
    get spawn() { return this.#spawn; }
    get priestState() { return this.#priest; }
    get forcefield() { return this.#lookup; }
    get carrier() { return this.#carrierPre; }
    get carrierCheck() { return this.#check; }
    get priests() { return this.#state.priests; }
    get pool() { return this.#core.pool; }
    // Kernel 정리 뒤 외부 등록과 Session Player 대여를 반납한다. 전역 상태 자체를 초기화하지 않는다. (2026-11-16 WB04-E563)
    dispose() { this.#core.postPop.setBody('priest', null); this.#core.postPop.setBody('forcefield', null); this.#state.hitPoints.playerRecords = new Uint8Array(0); }
}
