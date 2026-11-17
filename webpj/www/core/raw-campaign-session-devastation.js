// ConnectDevastation 앞단의 현재 TYPE 표와 상태 주문 결선이다(225553df, WB04-E678).
import { typeNumber, typeString } from '../assets/type-table.js';
import { RawBombStatus } from './raw-bomb-status.js';
import { dispatchCampaignFence } from './raw-campaign-fence-dispatch.js';
import { FENCE_TYPE_OPERATION } from './raw-fence-type-parent.js';

// 실제 Session의 TYPE·World·상태 주문을 한 번 구성한다.
export class RawCampaignSessionDevastation {
    // 같은 owner와 설치 시작·정상 복귀·반납 관문이다.
    #owner; #started = false; #ready = false; #disposed = false;
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

    // World가 프로세스를 비운 뒤 같은 함수 값의 재진입을 차단한다.
    dispose() { this.#disposed = true; }
    // 이 앞단만의 정상 설치 여부이며 전체 주문 설치는 아니다.
    get ready() { return this.#ready; }
}
