// 같은 Session의 사제 기도 입력·수신·정기를 실제 의식과 내용물에 연결한다(225553df).
import { typeNumber } from '../assets/type-table.js';
import { RawPriestPrayer } from './raw-priest-prayer.js';
import { RawContainedLookup } from './raw-contained-lookup.js';
import { pathDirection } from './raw-path-animation.js';

// 현재 TYPE 표와 같은 Host·의식·운반 부모를 사용하는 기도 생성 구간이다.
export class RawCampaignSessionPrayer {
    // 같은 Session과 한 번 설치·정상 복귀·반납 관문이다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 현재 자산별 기도 시간·기본 프레임과 값형 효과 표다.
    types = null; hooks = null;

    // 구성 전에는 World나 프로세스를 변경하지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 이전 Session에서 빌린 함수의 재진입을 막는다.
    #live() { if (this.#disposed || !this.#ready) throw new Error('반납/미연결 Session 사제 기도'); }

    // 운반 투하 뒤 실제 자산·의식·내용물·프로세스를 연결한다.
    bind() {
        const o = this.#owner, c = o.core;
        if (this.#disposed || this.#started || !o.carrierDropModules?.receiveReady || !o.ritualModules?.ready || o.priestPrayer) throw new Error('Session 사제 기도 설치 순서 오류');
        this.#started = true;
        const types = Array.from({ length: c.types.length }, () => ({ prayTime: 0, defaultFrame: 0 }));
        // 자산 block 번호 그대로 원본 기도 시간과 기본 프레임을 보충한다.
        for (const asset of o.assets.game.types()) {
            const type = 70 + asset.block, value = typeNumber(asset.definition, 'praytime') ?? 0;
            if (!types[type] || !Number.isFinite(value) || value < -2147483648 || value > 2147483647) throw new RangeError('사제 기도 자산 TYPE/시간 범위');
            types[type] = { prayTime: Math.trunc(value), defaultFrame: asset.definition.defaultFrame };
        }
        const hooks = {
            hasEvent: (sid, event) => { this.#live(); return Boolean(c.processes.findEvent(sid, event)); },
            direction: (sid) => { this.#live(); const raw = c.pool.slot(sid), frame = new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getInt32(36, true); return pathDirection(c.frames[raw[10]], frame); },
            movementType: (type) => { this.#live(); return o.priests.movementProcessType(type); },
            movement: (sid, type, ordinal, remove) => { this.#live(); return new RawContainedLookup(c.pool, (form, flags) => o.world.destroy(form, flags)).find(sid, type, ordinal, remove); },
            request: (target, packet) => { this.#live(); if (typeof o.services.carrierRequest !== 'function') throw new Error('기도6d 현재 요청 전송 미연결'); o.services.carrierRequest(target, packet); },
            broadcast: (packet) => { this.#live(); o.tables.broadcast.send(packet); },
            reserveShared: () => { this.#live(); return o.ritualModules.reserveRegular(); },
            addShared: (sid, event, payload) => { this.#live(); o.ritualModules.addRegular(sid, event, payload, 61); },
            ritual: (sid, stage) => { this.#live(); if (!o.ritualActions) throw new Error('기도604 실제 의식 미설치'); o.ritualActions.continue(sid, stage); },
            immobile: (sid) => { this.#live(); return o.priests.priestState.immobile(sid); },
            containedType: (sid, mask, kind, ordinal) => { this.#live(); const form = o.priests.carrier.findContained(sid, mask, kind, ordinal); if (!form) return 0; const raw = c.pool.slot(form); return raw[18] | (raw[19] << 8); },
            discard: (sid) => {
                this.#live(); const form = o.priests.carrier.findContained(sid, 1, 0, 0); if (!form) return;
                const raw = c.pool.slot(form), kind = raw[18] | (raw[19] << 8); o.world.destroy(form, 0); if (kind !== 168) o.priestModules.replenishNuggets();
            },
            create: (sid, kind, argument, flags, attachFlags, quality) => { this.#live(); o.contained.create(sid, kind, argument, flags, attachFlags, quality); },
            setFrame: (sid, frame, flags) => { this.#live(); c.frame.set(sid, frame, flags); },
            panic: (sid) => { this.#live(); o.priestPanic.run(sid); },
            parent: (sid, code, parameter) => { this.#live(); if (!o.carrierDrop) throw new Error('사제 입력의 현재 Carrier 부모 미설치'); return o.carrierDrop.command(sid, code, parameter & 0xffff); },
        };
        o.priestPrayer = new RawPriestPrayer(c.pool, types, () => { this.#live(); return o.state.assetState.readBoss(); }, hooks);
        this.types = types; this.hooks = hooks; this.#ready = true;
    }

    // World 정리 뒤 이전 기도 함수의 재진입을 막는다.
    dispose() { this.#disposed = true; }
    // 이 기도 설치의 정상 복귀이며 전체 Load는 별도다.
    get ready() { return this.#ready; }
}
