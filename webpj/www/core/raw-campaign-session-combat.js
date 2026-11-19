// Session1484..1536의 Archer/Missile·아이스 반사를 같은 실제 World/HP/장기 장치에 연결한다(225553df, WB04-E586).
// 사용: owner.connectTeleport() 뒤 connectCombat(). 발사/회전 수신·전체 Frame/Load 설치는 상위의 몫이다.
import { RawCampaignArcherBindings } from './raw-campaign-archer-bindings.js';
import { RawIceRicochet, makeIceRicochetWorldHooks } from './raw-ice-ricochet.js';
import { createMissile } from './raw-missile.js';

// C++ 값형 Archer 효과의 함수 묶음을 복사한다. 함수가 빌리는 실제 owner/장치는 복제하지 않는다.
export function copyCampaignArcherEffects(input = {}) {
    return { ...input, animation: { ...input.animation, common: { ...input.animation?.common } }, missile: { ...input.missile }, beam: { ...input.beam } };
}

// 실제 입자/기하/피해와 기존 Archer 몸체를 같은 난수·Host에 묶는 생성 구간이다.
export class RawCampaignSessionCombat {
    // 기반 owner와 한 번 결선/정상 복귀/반납 상태다. 부분 실패를 되돌리지 않는다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 원문의 지역 효과·반사 장치 사본이다. 실제 처리기는 각 함수 사본을 사용한다.
    effects = null; ricochetHooks = null;
    // 기반 자료를 빌린다. 실제 하위 생성은 bind에서 원문 순서로 수행한다.
    constructor(owner) { this.#owner = owner; }

    // 현재 전용 효과→실제 Hit/Impact→반사→Archer/Missile 순서로 생성한다.
    bind() {
        const o = this.#owner; const c = o.core; const s = o.state;
        if (this.#disposed || !o.teleportModules?.ready || this.#started) throw new Error('Session Archer의 초기화 전/반복 결선');
        this.#started = true;
        const e = copyCampaignArcherEffects(o.services.archer); this.effects = e;
        e.iceCooling ??= (parent) => { o.particles.addCooling(parent, o.projectileGeometry.hotspot(parent)); };
        // 반더의 실제 빔 생성과 현재 시각/피해를 강제로 같은 장치에 연결한다.
        e.beam.create = (source, target) => { o.particles.addBeam(source, target, 'shortZap.wav'); };
        e.beam.refresh = (source) => { o.gunHit.setTime(source, c.processState.now); };
        e.beam.hit = (source, target) => { o.gunHit.hit(source, target, c.processState.now); };
        // 현재 raw 좌표 비트를 float로 읽고 실제 SoundList 이름과 현재 위치 재생을 사용한다.
        e.animation.common.sound ??= (sid, name) => {
            const raw = c.pool.slot(sid); const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
            if (typeof o.services.sound.playOnceAt !== 'function') throw new Error('캠페인 발사 실제 위치 음향 미연결');
            o.services.sound.playOnceAt(view.getFloat32(14, true), view.getFloat32(18, true), o.soundNames.lookup(name), 0, 0);
        };
        e.animation.common.targetPoint ??= (firing, target) => o.fenceTarget.point(firing, target);
        e.missile.muzzle = (sid) => o.projectileGeometry.muzzle(sid);
        e.missile.rayEnd = (from, to, sid) => o.projectileGeometry.cannonRayEnd(from, to, sid);
        e.missile.hotspot = (sid) => o.projectileGeometry.hotspot(sid);
        e.missile.directionFrame = (from, to, type) => o.projectileGeometry.directionFrame(from, to, type);
        e.missile.targetDimensions = (sid) => o.projectileGeometry.dimensions(sid);
        e.missile.targetBounds = (sid, point) => o.projectileGeometry.targetBounds(sid, point);
        const otherFlight = e.missile.flightEffect;
        e.missile.flightEffect = (type, parent) => {
            if (type === 11) { o.particles.add(type, parent); return; }
            if (typeof otherFlight !== 'function') throw new Error(`실제 비행 표시 효과 미복원 TYPE${type}`);
            otherFlight(type, parent);
        };
        // 발사/충돌 토큰과 동적 파일명은 같은 현재 위치 재생 장치로 보낸다.
        const missileSound = (x, y, token) => {
            if (typeof o.services.sound.playOnceAt !== 'function') throw new Error('캠페인 투사체 실제 위치 음향 미연결');
            o.services.sound.playOnceAt(Math.fround(x), Math.fround(y), token >>> 0, 0, 0);
        };
        e.missile.firingSound ??= missileSound; e.missile.impactSound ??= missileSound;
        e.missile.impactName ??= (x, y, name) => {
            if (typeof o.services.sound.playOnceAt !== 'function') throw new Error('아이스 투사체의 실제 파일명 위치 음향 미연결');
            o.services.sound.playOnceAt(Math.fround(x), Math.fround(y), o.soundNames.lookup(name), 0, 0);
        };
        s.missileState.hitCommand = o.tutorialTechnologyCommand;
        s.missileState.checking = () => o.gameplayGlobals.aiDebug().checking().value >>> 0;
        // 이 진단 함수는 원문처럼 현재 서비스의 값 사본을 캡처한다. 이후 서비스 교체와 구별한다.
        const assertion = o.services.ai.assertion;
        e.missile.assertion = (expression, _file, line) => {
            if (typeof assertion !== 'function') throw new Error(`투사체 실제 Baseprocess 진단 장치 미연결 ${line}`);
            assertion(expression);
        };
        e.missile.queueCommand = (command, display, target) => {
            if (typeof o.services.assets.queueCommand !== 'function') throw new Error('투사체 실제 충돌 입력 큐 미연결');
            o.services.assets.queueCommand(command >>> 0, display | 0, target | 0);
        };
        e.missile = o.hits.missileHooks(o.gunHit, s.missileState, e.missile);
        e.missile.damage ??= o.hits.damageHooks({}).damage;
        this.ricochetHooks = { angles: o.world.storage.angleDiagnostics(), initialPointBytes: e.ricochetPointBytes,
            createProjectile: (request) => {
                if (!o.combat) throw new Error('아이스 반사 실제 Missile 설치 전 생성');
                if (!createMissile(c.processes, o.combat.missile, request)) throw new Error('아이스 반사 실제 Host 부착 실패');
            },
        };
        o.iceRicochet = new RawIceRicochet(c.pool, c.types, o.missileTypes, o.random, makeIceRicochetWorldHooks(c.pool, c.hash, c.types, o.hp, this.ricochetHooks));
        e.missile.ricochet = (fields) => { o.iceRicochet.generate(fields); };
        // 캐논은 Canon 배열이 아니라 같은 실제 World의 FrameCodeTable과 현재 교체값을 읽는다.
        const cannonFrames = c.frames.map((_table, type) => ({ get frames() { return c.frames[type]; } }));
        o.combat = new RawCampaignArcherBindings(o.world, o.gunTarget, o.random, o.archerTypes, cannonFrames, o.missileTypes, s.archerState, s.missileState, e);
        this.#ready = true;
    }
    // 부모가 같은 World 프로세스를 정리한 뒤 재사용을 차단한다. 현재 장기 전역은 초기화하지 않는다.
    dispose() { this.#disposed = true; }
    // 이 생성 구간의 정상 복귀이며 전체 Session/Load·전투 프레임 설치 완료와 구별한다.
    get ready() { return this.#ready; }
}
