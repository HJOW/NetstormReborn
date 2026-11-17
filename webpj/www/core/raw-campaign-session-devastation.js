// ConnectDevastation 앞단의 현재 TYPE 표와 상태 주문 결선이다(225553df, WB04-E678).
import { typeNumber, typeString } from '../assets/type-table.js';
import { RawBombStatus } from './raw-bomb-status.js';
import { dispatchCampaignFence } from './raw-campaign-fence-dispatch.js';
import { FENCE_TYPE_OPERATION } from './raw-fence-type-parent.js';
import { RawWindSpell } from './raw-wind-spell.js';
import { RawManoSpell } from './raw-mano-spell.js';
import { WindAviaryProcessFields } from './raw-wind-aviary-state.js';
import { WindAviaryProcess, findAttachedProcess } from './raw-wind-aviary-process.js';
import { currentAllocationBytes } from './current-allocation-bytes.js';

// 실제 Session의 TYPE·World·상태 주문을 한 번 구성한다.
export class RawCampaignSessionDevastation {
    // 같은 owner와 설치 시작·정상 복귀·반납 관문이다.
    #owner; #started = false; #ready = false; #summonsStarted = false; #summonsReady = false; #disposed = false;
    // 현재 상태 주문 효과를 후속 시전 분배기에 빌린다.
    statusHooks = null;
    // 구성 시 World나 난수는 변경하지 않는다.
    constructor(owner) { this.#owner = owner; }
    // 완료 전 또는 반납된 함수 값의 실행을 거부한다.
    #live() { if (this.#disposed || !this.#ready) throw new Error('반납/미연결 Session 파괴 주문'); }

    // 같은 타입 번호의 시전 프레임·비용·음향과 상태 주문을 설치한다.
    bindStatus() {
        const o = this.#owner, c = o.core;
        if (this.#disposed || this.#started || !o.bridgeCarriers || o.bombStatus) throw new Error('Session 파괴 주문 설치 순서 오류');
        this.#started = true;
        const native = o.world.storage.usesNativeTransmissionDevices?.() ?? false;
        if (!native) o.devastationRegularTypes = new Array(c.types.length);
        o.devastationEffectTypes = new Array(c.types.length);
        // 같은 TYPE 번호의 실제 A 프레임·시전 시간과 비용을 사용한다.
        for (let type = 0; type < c.types.length; type += 1) {
            if (!native) o.devastationRegularTypes[type] = { firstFrame: c.frames[type].run('A').first, castTime: o.carrierRegularModules.types[type].castTime };
            o.devastationEffectTypes[type] = { cost: c.types[type].cost, sound: 0, delay: 0 };
        }
        // 자산 block 번호로 현재 음향 토큰과 지연을 덧붙인다.
        for (const asset of o.assets.game.types()) {
            const type = 70 + asset.block, row = o.devastationEffectTypes[type];
            if (!row) throw new RangeError('파괴 주문 실제 자산 TYPE 범위');
            const sound = typeString(asset.definition, 'fireSound'), delay = typeNumber(asset.definition, 'effecttime') ?? 0;
            if (!Number.isFinite(delay) || delay < -2147483648 || delay > 2147483647) throw new RangeError('파괴 주문 effecttime 범위');
            row.sound = sound ? o.soundNames.lookup(sound) : 0; row.delay = Math.trunc(delay);
        }
        const hooks = {
            refresh: (sid) => { this.#live(); c.display.update(c.pool.slot(sid), 0); },
            unpop: (sid, flags) => { this.#live(); o.world.unpop(sid, flags); },
            repop: (sid, flags) => { this.#live(); o.world.repop(sid, flags); },
            fence: (sid) => { this.#live(); dispatchCampaignFence(o, sid, FENCE_TYPE_OPERATION.refresh); },
            flare: (sid, count, event, type, direction) => { this.#live(); o.flare.spawnDirection(sid, count, event, type, direction); },
        };
        o.bombStatusState.alliances = o.alliances;
        o.bombStatus = new RawBombStatus(c.pool, c.types, o.bombStatusState, hooks);
        this.statusHooks = hooks; this.#ready = true;
    }

    // 같은 현재 spawns와 장기 TYPE를 Wind·Mano 주문에 연결한다.
    bindSummons() {
        const o = this.#owner, c = o.core;
        if (this.#disposed || !this.#ready || this.#summonsStarted || !o.windAviary || o.windSpell || o.manoSpell) throw new Error('Session 소환 주문 설치 순서 오류');
        this.#summonsStarted = true;
        if (o.world.storage.usesNativeTransmissionDevices?.()) throw new Error('Native 장기 주문 spawns는 4D-8에서 연결');
        o.windSpellCounts = new Int32Array(c.types.length);
        // 실제 자산 block의 현재 spawns를 같은 TYPE 저장소에 채운다.
        for (const asset of o.assets.game.types()) {
            const type = 70 + asset.block, count = typeNumber(asset.definition, 'spawns') ?? 0;
            if (type < 0 || type >= o.windSpellCounts.length || !Number.isFinite(count) || count < -2147483648 || count > 2147483647) throw new RangeError('소환 주문 spawns TYPE/값 범위');
            o.windSpellCounts[type] = Math.trunc(count);
        }
        o.windSpellState.counts = o.windSpellCounts;
        const summons = {
            exists: (parent, type) => { this.#live(); return findAttachedProcess(c.pool, parent, type) !== 0; },
            allocate: () => { this.#live(); const fields = new WindAviaryProcessFields(); currentAllocationBytes(fields.bytes); return fields; },
            attach: (fields, parent, type, flags) => { this.#live(); c.processes.attach(new WindAviaryProcess(o.windAviary, fields), type, parent, flags); },
        };
        const env = { get server() { return c.processState.server; }, get now() { return c.processState.now; } };
        o.windSpell = new RawWindSpell(c.pool, env, o.windSpellState, summons);
        const mano = {
            wallSeconds: o.services.sound.wallSeconds,
            spellType: (level) => { this.#live(); const types = o.gameplayGlobals.gameAux().manoSpellTypes; if (level >= types.length) throw new RangeError('맨 오 워 주문 레벨 범위'); return types[level]; },
            flyerType: () => { this.#live(); return o.gameplayGlobals.gameAux().rainFlyerType; },
            create: (type, flags) => { this.#live(); return c.factory.create(type, flags); },
            owner: (sid, unused, owner) => { this.#live(); o.ownership.setOwner(sid, owner); },
            header: (sid, abstract, inlined) => {
                this.#live(); const type = c.pool.slot(sid)[10], record = c.types[type], shape = c.shapes[type];
                if (!record || !shape) throw new RangeError('맨 오 워 주문 현재 헤더 TYPE 물리 범위');
                const limit = Math.imul(shape.frameCount, (record.flags1 & 0x440000) ? 2 : 1);
                if (limit < 1) o.world.storage.recordDisplayFrameError(type, 0, abstract);
                if (limit < 1 || !shape.loaded || record.maxHitPoints === -0x22222223 || record.maxHitPoints === -0x32323233) o.world.storage.assertDisplay('frameCheck( abstract, frame, false )', inlined ? 'c:\\ns\\o\\rifttype.h' : '..\\o\\rifttype.h', 0xe1);
                const header = c.shapes[type].frames[0]; if (!header) throw new RangeError('맨 오 워 주문 현재 SHP 프레임 범위');
                return [header.height, header.hotspotY];
            },
            center: (sid) => { this.#live(); return c.neighbors.center(sid); },
            pop: (sid, unused, x, y, flags) => { this.#live(); o.world.pop(sid, x, y, flags); },
        };
        o.manoSpell = new RawManoSpell(c.pool, () => c.processState.server, o.windSpellCounts, mano);
        this.#summonsReady = true;
    }

    // World가 프로세스를 비운 뒤 같은 함수 값의 재진입을 차단한다.
    dispose() { this.#disposed = true; }
    // 이 앞단만의 정상 설치 여부이며 전체 주문 설치는 아니다.
    get ready() { return this.#ready; }
    // 주문 결선 완료는 전체 파괴 주문 설치 완료와 구별한다.
    get summonsReady() { return this.#summonsReady; }
}
