// Session 생성자1318..1339의 총 검색·에너지·마나/사정거리 표시를 같은 owner에 연결한다(225553df, WB04-E580).
// 사용: owner.connectGunAndCursorViews(). 전체 커서/프레임 분배와 Login/Load는 후속 구간이다.
import { RawGunMode } from './raw-gun-mode.js';
import { RawGunTarget } from './raw-gun-target.js';
import { RawEnergySupply, ENERGY_SOURCE_CAPACITY, energyProviderEntry } from './raw-energy-supply.js';
import { bindAiTypeRequiredEnergy } from './raw-type-required-energy.js';
import { CursorManaGlobals, RawCursorMana, bindCursorManaWorld } from './raw-cursor-mana.js';
import { CursorRangeGlobals, RawCursorRange } from './raw-cursor-range.js';
import { SquidPostPopList } from './squid-postpop-state.js';

// 원문489/492/653의 좌표/반경 참조와30칸 선택 목록이다. 기반 자료 생성 때 한 번 구성한다.
export function createCampaignCursorTables(owner) {
    const coordinates = new CursorManaGlobals(); const cells = owner.gameplayGlobals.worldCells(); const radius = owner.gameplayGlobals.manaRadius();
    Object.defineProperties(coordinates, {
        mapLimit: { enumerable: true, get: () => cells.value, set: (value) => { cells.value = value; } },
        radiusLimit: { enumerable: true, get: () => radius.value, set: (value) => { radius.value = value; } },
    });
    const selected = new SquidPostPopList(); selected.entries = new Uint32Array(ENERGY_SOURCE_CAPACITY);
    return { cursorManaGlobals: coordinates, cursorRangeGlobals: new CursorRangeGlobals(coordinates, owner.state.gunTargetState), energySources: selected };
}

// 실제 총 모드/표적과 에너지·표시 논리의 생성 순서를 보관한다. 전체 세션 공개는 수행하지 않는다.
export class RawCampaignSessionGunViews {
    // 같은 기반 자료와 한 번 연결/정상 복귀/반납 상태다. 실패 접두를 되돌리지 않는다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 소유 자료만 빌리며 실제 생성은 bind에서 수행한다.
    constructor(owner) { this.#owner = owner; }
    // 실제 TYPE 표 뒤 생성한다. 모든 조회 콜백은 원문 호출 시점의 같은 현재 자료를 읽는다.
    bind() {
        const o = this.#owner; const c = o.core; const s = o.state;
        if (this.#disposed || !o.typeModules?.ready || this.#started) throw new Error('Session 총/커서의 초기화 전/반복 결선');
        this.#started = true;
        o.gunMode = new RawGunMode(c.pool, c.frames, () => s.gunState.readWindArcherType());
        o.gunTarget = new RawGunTarget(c.pool, c.hash, c.types, o.gunTypes, c.shapes, s.gunTargetState);
        o.energySupply = new RawEnergySupply(c.pool, c.types, c.bookkeeping.currentProviders(), o.energySources, o.alliances,
            () => o.cursorManaGlobals.mapLimit, o.energyState,
            { providerEntry: energyProviderEntry, requirement: bindAiTypeRequiredEnergy(o.assets.game.typeTable.definitions, c.types) },
            { editor: () => o.globals.editor, special: () => o.playerState.special ? 1 : 0, mission: () => o.globals.missionPost ? 1 : 0 });
        const mana = {}; bindCursorManaWorld(mana, o.world, (sid) => o.gunTarget.center(sid));
        o.cursorMana = new RawCursorMana(c.pool, c.types, c.processState, o.cursorManaGlobals, o.cursorDrop, mana);
        const range = { world: {} }; bindCursorManaWorld(range.world, o.world, (sid) => o.gunTarget.center(sid));
        range.range = (type) => { type >>>= 0; if (type >= o.gunTypes.length) throw new RangeError('현재 사정거리 TYPE 범위 오류'); return o.gunTypes[type].range; };
        range.firstContent = (parent) => o.priests.carrier.findContained(parent, 1, 0, 0);
        range.mode = (parent) => o.gunMode.get(parent);
        range.cannonDirection = (query, direction) => o.gunTarget.cannonDirection(query, direction);
        range.footprint = (parent) => o.gunTarget.footprint(parent, false);
        range.directionFrame = (parent, direction) => o.gunTarget.directionFrame(c.pool.slot(parent)[10], direction);
        range.firingOffset = (type, frame) => {
            type >>>= 0; frame |= 0;
            if (type < 70 || type - 70 >= o.assets.game.types().length || frame < 0) throw new RangeError('실제 사정거리 발사구 자산 범위 오류');
            const header = o.assets.game.shapes.squidHeader(o.assets.game.types()[type - 70].block, frame); const view = new DataView(header.buffer, header.byteOffset, header.byteLength);
            return [view.getInt16(32, true), view.getInt16(34, true)];
        };
        range.frameRun = (type, variant) => {
            type >>>= 0; variant >>>= 0;
            if (type >= c.frames.length || variant >= 16) throw new RangeError('실제 사정거리 Run 범위 오류');
            return c.frames[type].run(String.fromCharCode(65 + variant));
        };
        range.diagnostic = o.services.cursorRangeDiagnostic;
        o.cursorRange = new RawCursorRange(c.pool, c.types, c.processState, o.cursorRangeGlobals, o.cursorDrop, range);
        this.#ready = true;
    }
    // 부모가 현재 World 프로세스를 정리한 뒤 연결 단계의 재사용만 차단한다. 소유 논리는 부모와 함께 해제된다.
    dispose() { this.#disposed = true; }
    // 이 생성 구간의 정상 복귀이며 전체 Session/커서 수명 공개 완료는 아니다.
    get ready() { return this.#ready; }
}
