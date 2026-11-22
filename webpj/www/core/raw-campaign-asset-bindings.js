// 실제 캠페인의 자산 PostPop/Regular/간헐천 AI/Gun 을 같은 raw 월드와 공유하게 조립하는 client 연결이다(cpppj `client/RawCampaignAssetBindings.cpp/.h`, 10.78 전용).
// 사용:
//   const bindings = new RawCampaignAssetBindings(world, damageable, assets, random, islands, playerRecords, playerState, globals, effects);
//   bindings.activatePostPop() 은 첫 저장 객체 Pop 전에 자산/Factory 후처리 몸체를 같은 공통 PostPop 분배기에 등록한다(같은 객체의 재활성 허용). bindings.dispose() 는 활성 등록만 해제한다(C++ 소멸자).
//   bindings.prepareFrame(now) 는 현재 게임 시각과 Player 의 Fort 모드를 Gun/Regular/자산 전역에 전달한다. bindings.setLoadingDepth(depth) 는 Fort 로딩 중첩값을 자산 상태에 전달한다.
//   bindings.bindPatchGeyserAi(update) · bindings.setState(sid, value)(Factory 가상 +84: Level 저장 뒤 기본 표시 갱신) · bindings.adjustFactoryFrame(sid)(00453780).
//   bindings.handler(fallback) 는 자산 Regular 처리기에 fallback 을 합성하고, bindings.transmission(fallback) 은 Gun(34) 프로세스 패킷만 실제 writer/송신으로 분배하고 나머지는 fallback 에 넘긴다.
//   bindings.postPop · bindings.regular · bindings.gun 은 상위 설치/검사가 조회하는 같은 원본 모듈이다(수명은 이 연결 객체에 있다).
// 입력:
//   world 는 RawSurfaceWorld, damageable 은 같은 풀의 RawCarrierPostPop, assets 는 AssetPostPopState, random 은 GameRandom, islands 는 같은 풀의 RawIslandRuntime,
//   playerRecords 는 실제 9 Player 기록(Uint8Array 9×0xb4), playerState 는 PlayerWorldState(battleOwner·fortMode 를 매 호출 읽는다),
//   globals = { regular: AssetRegularState, gun: GunProcessState, elements(188칸 원소 번호), archerNoticeCommand: { value }(PATCH 005429b4, PE 초기값 0x030000b0), factoryTypes?: FactoryPostPopTypes },
//   effects = { factory(warnDuplicate·regionOwnership 등), levelAssertion(), postPop(…), regular(…), gun(…), queueCommand(command, sid, 0), sendGunPacket(bytes) } 이다.
// 누락된 선택 효과는 성공/무동작으로 바꾸지 않고 그 효과가 실제로 호출될 때 `Error` 로 거부한다(cpppj `RejectMissing`). 구성 불일치는 어떤 공유 상태도 쓰기 전에 `TypeError` 로 거부한다.
// 구성 때 같은 World 의 권한 소유자(RawAuthorityState)를 자산·Gun·Regular 전역에 연결하고, 자산 상태의 현재 Bridge TYPE getter 를 같은 World 의 BridgeState 에 연결한다(이미 다른 소유자에 연결돼 있으면 거부).
// **옮기지 않은 것:** cpppj 의 Native 장기 Storage 경로(`UsesNativeTransmissionDevices`: 장기 Factory 후처리 공유·공통 전송 버퍼·`SerializeProcessForm`/`BroadcastProcessForm`)는 웹 `RawSurfaceStorage` 에 대응 개념이 없다.
//   웹은 항상 독립(typed) 경로(지역 Factory 본체·지역 64BYTE Gun 패킷 버퍼·`sendGunPacket` 송신)이며 Client 장기 소유자가 생길 때 같은 자리에 추가한다(4-D 구획 E). CD(10.72) 분기는 옮기지 않았다.
// 기준선 뒤 cpppj 변경 없음(기준선 `225553df` 와 HEAD 소스 동일, 2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E561: 4-D C 구획 RawCampaignAssetBindings 이식)
import { RawAssetPostPop, makeAssetPostPopFinder } from './raw-asset-post-pop.js';
import { RawAssetRegular } from './raw-asset-regular.js';
import { RawContainedLookup } from './raw-contained-lookup.js';
import { RawFactoryPostPop, makeFactoryPostPopHooks } from './raw-factory-post-pop.js';
import { RawFrameAdvance } from './raw-frame-advance.js';
import { RawGeyserAiRegistration } from './raw-geyser-ai-registration.js';
import { GunProcess, RawGunProcess, createGunProcess } from './raw-gun-process.js';
import { rawPathSurfaceAt } from './raw-path-surface.js';
import { RawSquidFinder } from './raw-squid-finder.js';
import { RawSquidLevel } from './raw-squid-level.js';
import { snapRegionCoordinate } from './raw-region-ownership.js';

// 일반 세계의 noIsland 번호, 10.78 의 TYPE 표 길이, Player 한 기록의 BYTE 폭과 기록 수, Player 번호 상한이다. (2026-11-15 WB04-E561)
const NO_ISLAND = 127;
const TYPE_COUNT = 188;
const PLAYER_STRIDE = 0xb4;
const PLAYER_RECORDS = 9;
const PLAYER_COUNT = 8;
// 슬롯 필드 위치이다: 다음 Form WORD(+4)·머리 Form WORD(+6)·섬 번호 WORD(+8)·TYPE(+10)·extra BYTE(+40). (2026-11-15 WB04-E561)
const SLOT_NEXT = 4;
const SLOT_HEAD = 6;
const SLOT_ISLAND = 8;
const SLOT_TYPE = 10;
const SLOT_EXTRA = 40;
// 섬 getter 가 허용하는 genus 비트 묶음(지면 2·다리 4·섬 받침 0x1000000), 다리 genus 비트(섬 번호 대신 127), 신전 genus 비트, 부모 차단 extra 비트이다. (2026-11-15 WB04-E561)
const ISLAND_GENUS = 0x1000006;
const BRIDGE_GENUS = 4;
const VORTEX_GENUS = 0x200;
const PARENT_BLOCKED = 0x20;
// 현재 TYPE 프레임 구간의 첫 프레임을 돌려주는 TYPE DWORD 위치 두 개('A' 구간 +0x130, 'B' 구간 +0x134)이다. (2026-11-15 WB04-E561)
const TYPE_FRAME_A = 0x130;
const TYPE_FRAME_B = 0x134;
// 독립(typed) Gun writer 가 쓰는 지역 패킷 버퍼의 BYTE 수이다. (2026-11-15 WB04-E561)
const GUN_PACKET_BYTES = 64;
// 난수 16비트 한계와 그 역수이다(발사 지연 난수). (2026-11-15 WB04-E561)
const RANDOM_LIMIT = 65536;
const RANDOM_SCALE = 1 / 65536;

// 비정렬 원본 BYTE/WORD/DWORD 를 현재 raw 원문에서 읽는다(폭은 호출자가 명시한다). (2026-11-15 WB04-E561)
function read(raw, offset, width) {
    let value = 0;
    // 지정한 저장 폭의 낮은 바이트부터 조합한다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// 실제 finder/island getter 의 유한 signed32 범위에 있는 plain ftol 입력이다. 범위 밖·비유한 좌표는 RangeError 이다. (2026-11-15 WB04-E561)
function truncate(value) {
    if (!Number.isFinite(value) || value < -2147483648 || value >= 2147483648) throw new RangeError('캠페인 자산 조회 좌표 원본 정수 범위 오류');
    return Math.trunc(value) | 0;
}

// 누락된 선택 효과를 성공/무동작으로 바꾸지 않고 해당 실제 호출에서 거부하는 함수로 채운다. (2026-11-15 WB04-E561)
function rejectMissing(hooks, name, message) {
    if (typeof hooks[name] !== 'function') hooks[name] = () => { throw new Error(message); };
}

// 같은 BridgeState 에는 항상 같은 현재 TYPE getter 를 돌려준다(C++ 의 주소 동일성 비교를 함수 동일성으로 옮긴 것). (2026-11-15 WB04-E561)
const BRIDGE_TYPE_GETTERS = new WeakMap();
function bridgeTypeGetter(state) {
    let getter = BRIDGE_TYPE_GETTERS.get(state);
    if (!getter) {
        getter = () => state.bridgeType;
        BRIDGE_TYPE_GETTERS.set(state, getter);
    }
    return getter;
}

// 실제 모듈을 조립하며 world/Core/Player/상태의 복사 월드를 만들지 않는다. 상위 실제 월드가 이 연결 객체보다 오래 살아야 한다. (2026-11-15 WB04-E561)
export class RawCampaignAssetBindings {
    // 실제 Pop/Unpop/Repop/Destroy 를 담당하는 상위 공유 월드와 같은 장기 월드의 참조 묶음(사본 저장소가 아니다)이다.
    #world;
    #core;
    // 현재 실제 목록/로딩/신전 상태(상위 캠페인 소유), 다른 월드 기능과 같은 LCG 난수, 같은 CoreData 의 섬 기록, 현재 battleOwner/fortMode 전역이다.
    #assets;
    #random;
    #islands;
    #playerState;
    // 장기 전역/TYPE 원소 배열(참조)과 외부 UI/음향/아직 없는 하위의 실제 소비자이다.
    #globals;
    #effects;
    // 같은 9 Player 의 현재 AI 포인터를 번호순으로 읽는 등록기, 현재 Form 체인의 타입/순번 조회 몸체이다.
    #geyser;
    #contained;
    // 원본 초기 총 상태 기계, 초기 자산 가상 PostPop 분배 몸체, Factory/아웃포스트 몸체, baseLevel 진단 몸체, 초기 예약의 가상 Handle 몸체이다.
    #gun;
    #postPop;
    #factory;
    #level;
    #regular;
    // 독립 typed Gun writer 의 지역 64BYTE 버퍼(미작성 바이트 관측 보존)와 송신 분배·공통 PostPop 등록의 현재 상태이다.
    #packet = new Uint8Array(GUN_PACKET_BYTES);
    #writingGun = false;
    #active = false;

    // 저장 객체 Pop 전에 같은 Pool/TYPE/9기록 계약을 확인하고 실제 모듈을 구성한다. 아직 현재 PostPop 등록은 바꾸지 않는다. (2026-11-15 WB04-E561)
    constructor(world, damageable, assets, random, islands, playerRecords, playerState, globals, effects) {
        const core = world.core();
        if (damageable?.pool !== core.pool || islands?.pool !== core.pool || core.types.length !== TYPE_COUNT || core.frames.length !== TYPE_COUNT || core.shapes.length !== TYPE_COUNT
            || globals?.elements?.length !== TYPE_COUNT || !(playerRecords instanceof Uint8Array) || playerRecords.length !== PLAYER_RECORDS * PLAYER_STRIDE) {
            throw new TypeError('캠페인 자산 현재 Core/TYPE/Player 저장소 불일치');
        }
        // 같은 World 가 모듈보다 오래 소유하는 두 bool 의 실제 소유자이다. 다른 실행기가 이미 연결한 권한 참조를 몰래 바꾸지 않는다.
        const authority = world.storage.authority;
        if ((assets.sharedAuthority && assets.sharedAuthority !== authority) || (globals.gun.sharedAuthority && globals.gun.sharedAuthority !== authority)
            || (globals.regular.sharedAuthority && globals.regular.sharedAuthority !== authority)) {
            throw new TypeError('캠페인 자산 실제 권한 소유자 불일치');
        }
        // WORD setter 와 Building.NotifySurface 가 같은 World BridgeState 의 현재 TYPE 을 읽는다.
        const bridgeType = bridgeTypeGetter(core.bridgeState);
        if (assets.currentBridgeType && assets.currentBridgeType !== bridgeType) throw new TypeError('캠페인 자산 현재 Bridge TYPE 소유자 불일치');
        this.#world = world;
        this.#core = core;
        this.#assets = assets;
        this.#random = random;
        this.#islands = islands;
        this.#playerState = playerState;
        this.#globals = globals;
        this.#effects = effects ?? {};
        // 실제 조립은 한 owner 만 연결하며 기존 bool 사본으로 동기화하지 않는다.
        assets.sharedAuthority = authority;
        globals.gun.sharedAuthority = authority;
        globals.regular.sharedAuthority = authority;
        assets.currentBridgeType = bridgeType;
        this.#geyser = new RawGeyserAiRegistration(playerRecords);
        this.#contained = new RawContainedLookup(core.pool, (sid, flags) => { world.destroy(sid, flags); });
        const advance = new RawFrameAdvance(core.pool, core.frames, core.frame);
        this.#gun = new RawGunProcess(core.pool, core.types, globals.gun.globals(), this.#gunHooks());
        this.#postPop = new RawAssetPostPop(core.pool, core.types, core.postPop, damageable, assets, this.#postHooks());
        this.#postPop.bindPlayerRecords(playerRecords, () => playerState.battleOwner);
        this.#factory = new RawFactoryPostPop(core.pool, core.hash, core.types, core.frames, core.postPop, damageable, assets, this.#factoryHooks(), globals.factoryTypes);
        const assertion = typeof this.#effects.levelAssertion === 'function' ? this.#effects.levelAssertion : () => { throw new Error('캠페인 Factory 실제 Level 진단 미연결'); };
        this.#level = new RawSquidLevel(core.pool, core.types, assertion);
        this.#regular = new RawAssetRegular(core.pool, core.hash, core.types, core.frames, globals.elements, core.frame, advance, random, assets, globals.regular, this.#regularHooks());
        this.#prepare(core.processState.now);
    }

    // 기존 Factory body 의 예약/공간/프레임만 같은 Core 에 연결한다. 큰 진단/지역 효과는 제공한 경계를 쓴다. (2026-11-15 WB04-E561)
    #factoryHooks() {
        const core = this.#core;
        const hooks = { ...(this.#effects.factory ?? {}) };
        hooks.reserveRegular = this.#effects.postPop?.reserveRegular;
        hooks.addRegular = (sid, event, payload) => {
            if (!core.processes.addRegular(sid, event, payload)) throw new Error('캠페인 Factory Regular 부착 실패');
        };
        hooks.destroy = (sid, flags) => { this.#world.destroy(sid, flags); };
        hooks.updateState = (sid) => { core.display.update(core.pool.slot(sid)); };
        rejectMissing(hooks, 'reserveRegular', '캠페인 Factory 실제 Regular 확보 미연결');
        rejectMissing(hooks, 'warnDuplicate', '캠페인 Factory 중복 진단 미연결');
        rejectMissing(hooks, 'regionOwnership', '캠페인 아웃포스트 실제 지역 소유권 미연결');
        return makeFactoryPostPopHooks(core.pool, core.frame, hooks);
    }

    // 현재 게임 시각과 local/fort 모드를 같은 Player 전역에서 조회한다. 다른 상태/목록은 복사하지 않는다. (2026-11-15 WB04-E561)
    #prepare(now) {
        const state = this.#playerState;
        if (!Number.isFinite(now) || state.battleOwner > PLAYER_COUNT) throw new RangeError('캠페인 자산 현재 clock/Player owner 오류');
        this.#globals.gun.now = now;
        this.#globals.gun.fortMode = state.fortMode;
        this.#globals.regular.fortMode = state.fortMode;
        this.#assets.fortMode = state.fortMode;
    }

    // 실제 getter 0046f530 은 plain 절삭한 hash 0 단계 칸을 읽고 bridge/없음이면 127 이다. (2026-11-15 WB04-E561)
    #islandNumber(x, y) {
        const core = this.#core;
        // 원문 0046f530 은 좌표에 0041d7a0 의 편향 snap(0.99999 를 더해 절삭)을 먼저 적용한 뒤 plain 절삭으로 hash 0 칸을 읽는다. (2026-11-22 WB04-E1219)
        truncate(x); truncate(y); // 비유한/범위 밖 좌표는 snap 보다 먼저 같은 오류로 거부한다.
        const cellX = truncate(snapRegionCoordinate(x));
        const cellY = truncate(snapRegionCoordinate(y));
        if (cellX < 0 || cellY < 0 || cellX >= 256 || cellY >= 256) return NO_ISLAND;
        const sid = core.hash.cell(0, cellX, cellY);
        if (!sid) return NO_ISLAND;
        const raw = core.pool.slot(sid);
        const type = raw[SLOT_TYPE];
        if (type >= core.types.length || !((core.types[type].flags2 >>> 0) & ISLAND_GENUS)) throw new Error('캠페인 자산 island getter 원본 genus assert');
        return (core.types[type].flags2 >>> 0) & BRIDGE_GENUS ? NO_ISLAND : read(raw, SLOT_ISLAND, 2);
    }

    // 원본 one-point 일반 finder 의 현재 순서/중복을 유지한다. 표면 전용 finder 로 바꾸지 않는다. (2026-11-15 WB04-E561)
    #atPosition(x, y) {
        const core = this.#core;
        const cellX = truncate(x);
        const cellY = truncate(y);
        const finder = new RawSquidFinder(core.pool, core.hash, core.types);
        const result = [];
        // 현재 후보/다음 링크의 원본 순서를 그대로 수집한다.
        for (let sid = finder.begin({ left: cellX, top: cellY, right: cellX, bottom: cellY }); sid; sid = finder.next()) result.push(sid);
        return result;
    }

    // Residence 의 일반 owner 는 실제 Core.owner 이다. Vortex 파생은 제공한 실제 가상 몸체를 요구한다. (2026-11-15 WB04-E561)
    #setOwner(sid, owner) {
        const core = this.#core;
        const virtual = this.#effects.postPop?.setOwner;
        if (typeof virtual === 'function') {
            virtual(sid, owner);
            return;
        }
        const type = core.pool.slot(sid)[SLOT_TYPE];
        if (type >= core.types.length || ((core.types[type].flags2 >>> 0) & VORTEX_GENUS)) throw new Error('캠페인 자산 실제 Vortex 가상 owner 미연결');
        core.owner.set(sid, owner);
    }

    // 부모 Form 체인에서 실제 fire(35)/rotate(36) 의 첫 번호를 찾는 기존 Gun helper 계약이다. (2026-11-15 WB04-E561)
    #animationForm(parent) {
        const core = this.#core;
        let sid = read(core.pool.slot(parent), SLOT_HEAD, 2);
        let visited = 0;
        // 원본 첫 일치 순서를 보존하며 손상된 무한 체인은 유한 풀 경계에서 거부한다.
        while (sid) {
            visited += 1;
            if (sid >= core.pool.capacity || visited >= core.pool.capacity) throw new Error('캠페인 총 현재 Animation Form 체인 오류');
            const raw = core.pool.slot(sid);
            if (raw[SLOT_TYPE] === core.processState.fireAnimationType || raw[SLOT_TYPE] === core.processState.rotateAnimationType) return sid;
            sid = read(raw, SLOT_NEXT, 2);
        }
        return 0;
    }

    // 현재 실제 Gun Form 과 raw fields 가 같은 Kernel 객체인지 확인한다. (2026-11-15 WB04-E561)
    #currentGun(fields) {
        const process = this.#core.processes.fromForm(fields.form);
        if (!(process instanceof GunProcess) || process.fields !== fields) throw new Error('캠페인 총 현재 Host/raw 이미지 불일치');
        return process;
    }

    // 초기 생성·현재 Form 전송·차단·난수를 실제 현재 Core 에 연결한다. (2026-11-15 WB04-E561)
    #gunHooks() {
        const core = this.#core;
        const hooks = { ...(this.#effects.gun ?? {}) };
        hooks.removeDependency = (watcher, watched) => { core.dependencies.remove(watcher, watched, 0); };
        hooks.addDependency = (watched, fields) => {
            const process = this.#currentGun(fields);
            core.dependencies.create(watched, process.form);
        };
        hooks.repopForm = (fields) => { core.processes.refreshForm(this.#currentGun(fields)); };
        hooks.animationForm = (parent) => this.#animationForm(parent);
        hooks.killAnimation = (form, flags) => {
            const process = core.processes.fromForm(form);
            const type = core.pool.slot(form)[SLOT_TYPE];
            if (!process || (type !== core.processState.fireAnimationType && type !== core.processState.rotateAnimationType)) throw new Error('캠페인 총 Animation 현재 Host 오류');
            core.processes.kill(process, flags);
        };
        hooks.parentBlocked = (parent) => (core.pool.slot(parent)[SLOT_EXTRA] & PARENT_BLOCKED) !== 0;
        // 원문은 float 인자를 double 로 넓혀 계산한다(난수 16비트 × 2^-16 × |high − low| + low).
        hooks.randomBetween = (low, high) => {
            const from = Math.fround(low);
            return this.#random.next(RANDOM_LIMIT) * RANDOM_SCALE * Math.abs(Math.fround(high) - from) + from;
        };
        rejectMissing(hooks, 'refreshRange', '캠페인 총 현재시각 압축/공간통지 미연결');
        rejectMissing(hooks, 'targetingMode', '캠페인 총 현재 공격방향 getter 미연결');
        rejectMissing(hooks, 'findTarget', '캠페인 총 실제 표적 finder 미연결');
        rejectMissing(hooks, 'startAnimation', '캠페인 총 실제 파생 Animation factory 미연결');
        rejectMissing(hooks, 'validateIslandOwners', '캠페인 총 현재 지면소유자 진단 미연결');
        rejectMissing(hooks, 'checkForms', '캠페인 총 현재 Form 진단 미연결');
        rejectMissing(hooks, 'shotContext', '캠페인 총 현재8DWORD 발사문맥 미연결');
        rejectMissing(hooks, 'shoot', '캠페인 총 실제 MayFire 미연결');
        rejectMissing(hooks, 'firingInterval', '캠페인 총 실제 TYPE 발사간격 미연결');
        rejectMissing(hooks, 'attach', '캠페인 총 단독 Construct 대신 실제 CreateGunProcess가 필요합니다');
        return hooks;
    }

    // 현재 자산 PostPop 의 Core 몸체만 설치하고 UI/음향/기타 파생은 선택 시 거부한다. (2026-11-15 WB04-E561)
    #postHooks() {
        const core = this.#core;
        const world = this.#world;
        const effects = this.#effects;
        const hooks = { ...(effects.postPop ?? {}) };
        hooks.addRegular = (sid, event, payload) => {
            if (!core.processes.addRegular(sid, event, payload)) throw new Error('캠페인 자산 실제 Regular 부모 부착 실패');
        };
        // 신규 생성과 수신이 같은 005410f8 을 읽는다.
        hooks.createGunProcess = (sid) => {
            this.#prepare(core.processState.now);
            if (!createGunProcess(core.processes, this.#gun, sid, world.storage.gameAux.gunProcessType)) throw new Error('캠페인 자산 실제 Gun 부모 부착 실패');
        };
        hooks.removeAnimation = (sid) => {
            if (typeof effects.queueCommand !== 'function') throw new Error('캠페인 발사기 실제 입력명령 큐 미연결');
            effects.queueCommand(this.#globals.archerNoticeCommand.value, sid, 0);
        };
        hooks.surface = (x, y) => rawPathSurfaceAt(core.hash, x, y);
        hooks.setOwner = (sid, owner) => {
            this.#prepare(core.processState.now);
            this.#setOwner(sid, owner);
        };
        hooks.notifySurface = (sid) => { core.building.notifySurface(sid); };
        hooks.geyserRegistration = (sid, state) => { this.#geyser.register(sid, state); };
        hooks.atPosition = (x, y) => this.#atPosition(x, y);
        // 간헐천은 Unpop/Repop 효과 뒤 실제 일반 finder 의 현재 Next 를 소비한다.
        hooks.visitAtPosition = makeAssetPostPopFinder(core.pool, core.hash, core.types);
        hooks.unpop = (sid, flags) => { world.unpop(sid, flags); };
        hooks.repop = (sid, flags) => { world.repop(sid, flags); };
        const typeDword = hooks.typeDword;
        hooks.typeDword = (sid, offset) => {
            const type = core.pool.slot(sid)[SLOT_TYPE];
            if (type >= core.frames.length) throw new RangeError('캠페인 자산 현재TYPE 프레임 표 오류');
            if (offset === TYPE_FRAME_A || offset === TYPE_FRAME_B) return core.frames[type].run(offset === TYPE_FRAME_A ? 'A' : 'B').first >>> 0;
            if (typeof typeDword !== 'function') throw new Error('캠페인 자산 실제 TYPE 추가DWORD 미연결');
            return typeDword(sid, offset);
        };
        hooks.islandNumber = (x, y) => this.#islandNumber(x, y);
        hooks.islandExists = (island) => Boolean(this.#islands.exists(island));
        // cpppj 의 islandTemple 훅(기존 신전 assert 조회)은 CD(10.72) 후처리만 부르므로 10.78 전용인 웹에서는 연결하지 않는다.
        hooks.registerTemple = (island, sid) => { this.#islands.setTemple(island, sid); };
        hooks.destroy = (sid, flags) => { world.destroy(sid, flags); };
        rejectMissing(hooks, 'reserveRegular', '캠페인 자산 실제 Regular 확보 경계 미연결');
        rejectMissing(hooks, 'checkForms', '캠페인 자산 현재 Form 진단 미연결');
        rejectMissing(hooks, 'gunFrame', '캠페인 다른 총 실제 각도→Graph표 미연결');
        rejectMissing(hooks, 'setGraph', '캠페인 다른 총 실제 Graph BYTE 쓰기 미연결');
        rejectMissing(hooks, 'startBlocker', '캠페인 바리케이드 실제 프로세스 미연결');
        rejectMissing(hooks, 'playImpactSound', '캠페인 신전 실제 impactSound 미연결');
        rejectMissing(hooks, 'clearPlacement', '캠페인 실제 배치커서 취소 미연결');
        rejectMissing(hooks, 'regionNumber', '캠페인 실제 지역BYTE 지도 미연결');
        rejectMissing(hooks, 'hasContent', '캠페인 신전 실제 Slot 내용물 조회 미연결');
        rejectMissing(hooks, 'createSlot', '캠페인 신전 실제 Slot 생성 미연결');
        rejectMissing(hooks, 'refreshProduction', '캠페인 신전 실제 생산 갱신 미연결');
        return hooks;
    }

    // 현재 거주 구역/간헐천/신전 예약은 기존 Handle 을 사용하고 외부 사건은 같은 fallback 에 남긴다. (2026-11-15 WB04-E561)
    #regularHooks() {
        const core = this.#core;
        const hooks = { ...(this.#effects.regular ?? {}) };
        hooks.notifySurface = (sid) => { core.building.notifySurface(sid); };
        hooks.surface = (x, y) => rawPathSurfaceAt(core.hash, x, y);
        hooks.setOwner = (sid, owner) => { this.#setOwner(sid, owner); };
        hooks.displayUpdate = (sid, flags) => { core.display.update(core.pool.slot(sid), flags); };
        hooks.islandNumber = (x, y) => this.#islandNumber(x, y) >>> 0;
        hooks.findAttached = (parent, type) => this.#contained.find(parent, type);
        rejectMissing(hooks, 'residenceEvent2', '캠페인 거주 구역 실제 사건2 미연결');
        rejectMissing(hooks, 'ascendTick', '캠페인 신전 실제 Ascend 진행 미연결');
        rejectMissing(hooks, 'playerLeavingNotice', '캠페인 현재 Player 이탈 UI 통지 미연결');
        rejectMissing(hooks, 'countForeignOwnedIslands', '캠페인 현재 다른소유자 섬 조회 미연결');
        rejectMissing(hooks, 'requestLeave', '캠페인 실제 신전 이탈 packet 미연결');
        rejectMissing(hooks, 'regionNumber', '캠페인 실제 신전 지역BYTE 조회 미연결');
        rejectMissing(hooks, 'removeAnimation', '캠페인 실제 신전 입력명령 통지 미연결');
        rejectMissing(hooks, 'damageGround', '캠페인 실제 지형피해/효과 사슬 미연결');
        rejectMissing(hooks, 'hitPoints', '캠페인 실제 가상피해 분배 미연결');
        rejectMissing(hooks, 'startConstruction', '캠페인 실제 건설 프로세스 시작 미연결');
        return hooks;
    }

    // 상위가 현재 Kernel/반환 callback 수명을 끝낸 뒤 활성 자산 분배만 해제한다(C++ 소멸자). 활성화하지 않은 연결은 다른 등록을 건드리지 않는다. (2026-11-15 WB04-E561)
    dispose() {
        if (!this.#active) return;
        this.#core.postPop.setBody('asset', null);
        this.#core.postPop.setBody('factory', null);
        this.#active = false;
    }

    // 실제 초기 Pop 전에 한 번 등록하며 동일 객체의 재활성은 허용한다. (2026-11-15 WB04-E561)
    activatePostPop() {
        this.#core.postPop.setBody('asset', this.#postPop);
        this.#core.postPop.setBody('factory', this.#factory);
        this.#active = true;
    }

    // 원본 clock 과 현재 Player 모드를 조회하여 다음 실제 프레임에 사용한다. 신전 DWORD 복사는 수행하지 않는다. (2026-11-15 WB04-E561)
    prepareFrame(now) { this.#prepare(now); }

    // 실제 부모 로더의 중첩값을 사본 상태 없이 기존 자산 모듈에 전달한다. (2026-11-15 WB04-E561)
    setLoadingDepth(depth) { this.#assets.loadingDepth = depth >>> 0; }

    // 최초 Pop 의 같은 번호순 Player 순회가 실제 AI 본체 목록(004153c0)을 갱신하도록 연결한다. (2026-11-15 WB04-E561)
    bindPatchGeyserAi(update) { this.#geyser.bindPatch(update); }

    // actual 가상 +84 의 Level 저장 뒤 기본 표시 갱신을 생략하지 않는다. (2026-11-15 WB04-E561)
    setState(sid, value) { this.#factory.setState(this.#level, sid, value); }

    // postPop 과 업그레이드가 같은 프레임 구간/지정기를 공유한다(00453780). (2026-11-15 WB04-E561)
    adjustFactoryFrame(sid) { this.#factory.adjustFrame(sid); }

    // 실제 지원 가상 표만 처리하고 다른 부모는 호출자가 준 실제 fallback 에 넘긴다. (2026-11-15 WB04-E561)
    handler(fallback = null) { return this.#regular.handler(fallback); }

    // 같은 서버 flags 0 의 Gun packet 만 분배하고 다른 현재 Process packet 은 기존 경계(fallback)를 유지한다. (2026-11-15 WB04-E561)
    transmission(fallback = {}) {
        const rest = fallback ?? {};
        return {
            writeData: (process, parentChanged) => {
                this.#writingGun = process instanceof GunProcess;
                if (this.#writingGun) return process.writePacket(this.#packet, parentChanged);
                if (typeof rest.writeData !== 'function') throw new Error('캠페인 다른실제Process packet writer 미연결');
                return rest.writeData(process, parentChanged);
            },
            writeFullData: (process, force) => {
                this.#writingGun = process instanceof GunProcess;
                if (this.#writingGun) return process.writePacket(this.#packet, false, (force >>> 0) !== 0);
                if (typeof rest.writeFullData !== 'function') throw new Error('다른 Process의 실제 전체 writer 미연결');
                return rest.writeFullData(process, force);
            },
            packet: (length) => {
                if (!this.#writingGun) {
                    if (typeof rest.packet !== 'function') throw new Error('다른 Process의 실제 작성 버퍼 미연결');
                    return rest.packet(length);
                }
                if (length < 0 || length > this.#packet.length) throw new RangeError('Gun 실제 작성 버퍼 범위');
                return this.#packet.subarray(0, length);
            },
            transmit: (length) => {
                if (this.#writingGun) {
                    if (typeof this.#effects.sendGunPacket !== 'function') throw new Error('캠페인 Gun flags0 실제 packet 송신 미연결');
                    if (length < 0 || length > this.#packet.length) throw new RangeError('캠페인 Gun 실제 packet 길이 오류');
                    this.#effects.sendGunPacket(this.#packet.subarray(0, length));
                    return;
                }
                if (typeof rest.transmit !== 'function') throw new Error('캠페인 다른실제Process packet 송신 미연결');
                rest.transmit(length);
            },
        };
    }

    // 상위 설치/검사가 같은 원본 PostPop 몸체를 조회한다. (2026-11-15 WB04-E561)
    get postPop() { return this.#postPop; }

    // 상위 같은 월드 Regular 분배가 실제 Handle 을 조회한다. (2026-11-15 WB04-E561)
    get regular() { return this.#regular; }

    // 초기 총의 현재 생성/상태 기계와 같은 모듈을 조회한다. (2026-11-15 WB04-E561)
    get gun() { return this.#gun; }
}
