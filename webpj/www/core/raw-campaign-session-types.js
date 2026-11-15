// Session 생성자1268..1317의 Carrier/Balloon과 실제 TYPE 보충 표를 같은 owner에 연결한다(225553df, WB04-E579).
// 사용: Player/소유권 뒤 owner.connectBodiesAndTypeTables(). Gun/피해 등 후속 모듈/전체 Load는 별도다.
import { typeNumber, typeString } from '../assets/type-table.js';
import { readRawArcherTypeData } from './raw-archer-type-data.js';
import { RawBalloonBody } from './raw-balloon-body.js';
import { RawCarrierPostPop, makeCarrierPostPopHooks } from './raw-carrier-post-pop.js';
import { RegularProcess } from './squid-process.js';

// 원문 int 변환의0방향 절삭과 x87/SSE 정수 불확정값이다. float로 먼저 좁히지 않는다.
function integer(value) { return !Number.isFinite(value) || value <= -2147483649 || value >= 2147483648 ? -2147483648 : Math.trunc(value) | 0; }
// 원문 선언/1004의 값 초기화 벡터다. 원소마다 독립 행을 만들고 실제 채움까지0을 유지한다.
export function createCampaignTypeTables(count) {
    return {
        builderTypes: Array.from({ length: count }, () => ({ speed: 0, sourceFrameIndex: 0, firstFrames: new Int32Array(16) })),
        gunTypes: Array.from({ length: count }, () => ({ range: 0, theme: 0, directionFrames: new Int32Array(4), heightExtension: 0 })),
        hitTypes: Array.from({ length: count }, () => ({ damagePerSecond: 0, specialFlyerDamage: false, flyerDamagePerSecond: 0 })),
        firingIntervals: new Float32Array(count), dudeSpeeds: Array.from({ length: count }, () => ({ speed: 0 })),
        archerTypes: Array.from({ length: count }, () => ({ directions: 0, frameCount: 0 })),
        missileTypes: Array.from({ length: count }, () => ({ range: 0, frameCount: 0, firingSound: 0, impactSound: 0, damagePerSecond: 0, specialFlyerDamage: false, flyerDamagePerSecond: 0 })),
        projectileTypes: Array.from({ length: count }, () => ({ defaultFrame: 0, frameCount: 0, directions: 0, shotOffsets: [], heightExtension: 0, range: 0 })),
    };
}

// 같은 Impl의 공통 Carrier/Balloon과 보충 표 생성 구간이다. 예약은 실제 한 Regular 객체를 재사용한다.
export class RawCampaignSessionTypes {
    // 실제 owner와 단일 생성/정상 복귀/반납 상태다. 실패 접두는 유지하고 재실행하지 않는다.
    #owner; #started = false; #ready = false; #disposed = false;

    // 같은 기반 자료만 빌리며 실제 연결은 bind에서 원문 순서로 수행한다.
    constructor(owner) { this.#owner = owner; }

    // 원문1268..1317 typed 경로다. 실제 자산 순서·자료 쓰기·이름 조회와 SHP 읽기 순서를 유지한다.
    bind() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.playerModules?.ready || this.#started) throw new Error('Session 몸체/TYPE의 초기화 전/반복 결선');
        this.#started = true;
        o.carrier = new RawCarrierPostPop(c.pool, c.types, o.state.carrierState, makeCarrierPostPopHooks(c.pool, c.postPop,
            (sid) => { o.priests.checkCarrier(sid); }, (x, y, owner) => o.world.setSupportOwner(x, y, owner)));
        o.carrier.bindCurrentTypes(c.types);
        o.balloonBody = new RawBalloonBody(c.pool, c.types, c.spots, () => o.state.assetState.readBoss(), o.carrier, o.priests.carrier, {
            frames: (type) => c.frames[type].codes.length, create: (type, flags) => o.world.create(type, flags),
            pop: (sid, x, y, flags) => o.world.pop(sid, x, y, flags), unpop: (sid, flags) => o.world.unpop(sid, flags),
            destroy: (sid, flags) => o.world.destroy(sid, flags), check: (sid) => { o.priests.checkCarrier(sid); },
            allocateRegular: () => this.reserveCloudRegular(), constructRegular: (parent, event, delay) => this.attachCloudRegular(parent, event, delay),
        });
        c.postPop.setBody('balloon', o.balloonBody);
        // 빈/내장 타입까지 TYPE+98과A..P의 실제 첫 번호를 모든188행에 전달한다.
        for (let type = 0; type < c.types.length; type += 1) {
            o.gunTypes[type].theme = c.types[type].theme; o.themes[type] = c.types[type].theme >>> 0; o.elements[type] = c.types[type].theme;
            o.builderTypes[type].sourceFrameIndex = c.types[type].theme >>> 0;
            // 없는 Run의-1도 같은 실제 FrameCodeTable에서 읽는다.
            for (let index = 0; index < 16; index += 1) o.builderTypes[type].firstFrames[index] = c.frames[type].run(String.fromCharCode(65 + index)).first;
        }
        // 보호116정의를 원래 블록 순서로 처리한다. 새 배열/행으로 교체하지 않고 현재 벡터에 쓴다.
        for (const asset of o.assets.game.types()) {
            const type = 70 + asset.block; const definition = asset.definition; const target = o.gunTypes[type];
            target.range = Math.fround(typeNumber(definition, 'range') ?? 0); target.heightExtension = integer(typeNumber(definition, 'height') ?? 0);
            o.dudeSpeeds[type].speed = Math.fround(typeNumber(definition, 'speed') ?? 0); o.builderTypes[type].speed = o.dudeSpeeds[type].speed;
            // L/M/N/O의 원문 첫 프레임이며 미지정-1을 임의0으로 바꾸지 않는다.
            for (let direction = 0; direction < 4; direction += 1) target.directionFrames[direction] = c.frames[type].run(String.fromCharCode(76 + direction)).first;
            const hit = o.hitTypes[type]; hit.damagePerSecond = Math.fround(typeNumber(definition, 'hpPerSec') ?? 0);
            hit.specialFlyerDamage = integer(typeNumber(definition, 'useairdamage') ?? 0) !== 0; hit.flyerDamagePerSecond = Math.fround(typeNumber(definition, 'airdamage') ?? 0);
            const missile = o.missileTypes[type]; missile.damagePerSecond = hit.damagePerSecond; missile.specialFlyerDamage = hit.specialFlyerDamage; missile.flyerDamagePerSecond = hit.flyerDamagePerSecond;
            o.firingIntervals[type] = Math.fround(typeNumber(definition, 'delayBetweenShots') ?? 0);
            Object.assign(o.archerTypes[type], readRawArcherTypeData(definition)); missile.range = target.range; missile.frameCount = o.archerTypes[type].directions;
            const fire = typeString(definition, 'fireSound'); const impact = typeString(definition, 'impactSound');
            missile.firingSound = fire ? o.soundNames.lookup(fire) : 0; missile.impactSound = impact ? o.soundNames.lookup(impact) : 0;
            const geometry = o.projectileTypes[type]; geometry.defaultFrame = definition.defaultFrame; geometry.range = target.range;
            geometry.frameCount = o.archerTypes[type].frameCount; geometry.directions = o.archerTypes[type].directions; geometry.heightExtension = target.heightExtension;
            // 원문 물리36BYTE 헤더의 마지막 두 signed WORD이며 표시 hotspot은 사용하지 않는다.
            for (let frame = 0; frame < c.shapes[type].frames.length; frame += 1) {
                const header = o.assets.game.shapes.squidHeader(asset.block, frame); const view = new DataView(header.buffer, header.byteOffset, header.byteLength);
                geometry.shotOffsets.push([view.getInt16(32, true), view.getInt16(34, true)]);
            }
        }
        this.#ready = true;
    }

    // 원문 new40/nothrow 단계다. JS의 순수한 미부착 Regular를 먼저 확보하고 같은 객체를 소비한다.
    reserveCloudRegular() {
        const o = this.#owner; if (o.pendingCloudRegular) throw new Error('클라우드 플로터 Regular 중복 예약');
        try { o.pendingCloudRegular = new RegularProcess(0, 0); } catch (error) { if (error instanceof RangeError) return false; throw error; }
        return true;
    }

    // 확보한 한 객체를 실제 Regular로 초기화→Host 부착→부착 뒤 현재 double 시각으로 ResetTask한다.
    attachCloudRegular(parent, event, delay) {
        const o = this.#owner; if (!o.pendingCloudRegular) throw new Error('클라우드 플로터 Regular 실제 예약 누락');
        const process = o.pendingCloudRegular; o.pendingCloudRegular = null; process.initializeDetached(event, delay);
        const attached = o.core.processes.attachRegular(process, parent); if (attached) attached.resetTask(event, o.core.processState.now);
    }

    // 현재 프로세스 정리 뒤 파생 등록과 미소비 예약을 반납한다. 다른 TYPE 벡터/장기 상태는 초기화하지 않는다.
    dispose() { if (this.#disposed) return; this.#disposed = true; if (this.#owner.balloonBody) this.#owner.core.postPop.setBody('balloon', null); this.#owner.pendingCloudRegular = null; }
    // 이 구간의 정상 복귀다. Gun 등 나머지 모듈/전체 Load 완료는 아니다.
    get ready() { return this.#ready; }
}
