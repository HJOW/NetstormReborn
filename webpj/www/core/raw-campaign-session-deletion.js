// 독립 NoWorld/typed 생성자1680..1756의 자산 삭제 래퍼·가상 분배다(225553df, WB04-E614).
// 사용: GemReward 뒤 bind. Native 장기 삭제/Outpost 분기는 실제 Client 장치 조립에서 별도로 연결한다.
import { SQUID_DESTROY_EFFECT } from './squid-destroy.js';
import { BRIDGE_VTABLE } from './raw-bridge-events.js';
import { PRIEST_VTABLE } from './raw-priest-owner.js';

// Source364의 공통 Damageable14VT다. 표시 TYPE나 이름을 추가 자격으로 사용하지 않는다.
const PLAIN_DAMAGEABLE = new Set([0x501ea0, 0x509148, 0x509930, 0x5099e8, 0x509aa0, 0x50a820, 0x50a8e8, 0x50b6e8, 0x50b7c0, 0x50b888, 0x50b9b8, 0x50bad0, 0x50e668, 0x50e738]);
// Source2003..2005의 일반 Bomb 파생 표다.163의 전용 그림은 별도 TYPE/VT 쌍이다.
const BOMB_TABLES = new Set([0x502370, 0x502448, 0x502510, 0x502ff8, 0x5030c0, 0x503188, 0x501dd0, 0x5025d8, 0x502e68, 0x502f30]);
// 실제 네 신전 VT다. 표시 TYPE 변경으로 가상 수명 선택을 바꾸지 않는다.
const VORTEX_TABLES = new Set([0x5196f0, 0x5197a8, 0x519860, 0x519918]);
// 같은 현재 raw 슬롯의 비정렬 DWORD를 읽는다.
function table(raw) { return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(0, true); }

// 같은 현재 owner에 이미 구성한 수명/피해/건설 부모를 원문 순서로 합친다.
export class RawCampaignSessionDeletion {
    // 부분 실패/반복·반납 뒤 효과를 구별한다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 원문1969..2018의 독립 typed 수명 자격과 두 최종 함수 값이다.
    kinds = null; emit = null; destroy = null;

    // 생성 자체는 현재 객체/장부/게임 난수를 바꾸지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 현재 VT/역할로 같은 실제 부모만 고른다. Native 전용 관문은 이 NoWorld 몸체에 섞지 않는다.
    #makeKinds() {
        const o = this.#owner; const c = o.core;
        // C++ span과 같이 같은 물리 슬롯을 읽어 역할 getter 뒤의 현재 바이트를 보존한다.
        const raw = (sid) => c.pool.physical(sid);
        return {
            plainDamageable: (sid) => PLAIN_DAMAGEABLE.has(table(raw(sid))), // VT만 비교하는14개 공통 피해 수명이다.
            blocker: (sid) => [0x50e940, 0x50ea08, 0x50ead0].includes(table(raw(sid))), // 세 Blocker의 전용 Post다.
            growingIce: (sid) => table(raw(sid)) === 0x50e888, // 성장형의 고유 Post/Regular다.
            bridge: (sid) => { const bytes = raw(sid); return bytes[10] === c.bridgeState.bridgeType && table(bytes) === BRIDGE_VTABLE; }, // 현재 전체 DWORD 역할과 실제 다리 표다.
            connector: (sid) => { const bytes = raw(sid); return bytes[10] === c.bridgeState.connectorType && table(bytes) === 0x503360; }, // 현재 연결 TYPE와 실제 공통 수명이다.
            rune: (sid) => table(raw(sid)) === 0x5075d8, // Rune의 실제 공통 Pre/Post다.
            sunFence: (sid) => table(raw(sid)) === 0x509470, // 썬 바리케이드 VT에 TYPE136 제한을 덧붙이지 않는다.
            nugget: (sid) => table(raw(sid)) === 0x50dc30, // 너겟 VT에 표시 TYPE154 제한을 덧붙이지 않는다.
            priestForcefield: (sid) => { const bytes = raw(sid); return bytes[10] === 167 && table(bytes) === 0x507770; }, // 독립 typed의 기존 TYPE/VT 계약이다.
            priest: (sid) => { const bytes = raw(sid); return bytes[10] === 158 && table(bytes) === PRIEST_VTABLE; }, // 독립 typed의 사제 수명·삭제 자격이다.
            buried: (sid) => { const bytes = raw(sid); return bytes[10] === o.gemRewardState.buriedType && table(bytes) === 0x50a748; }, // 같은 현재 보상 상태를 읽는다.
            bomb: (sid) => { const bytes = raw(sid); if (bytes[10] === 163 && table(bytes) === 0x509b98) return true; if (bytes[10] >= c.types.length || !(c.types[bytes[10]].flags2 & 0x100)) return false; return BOMB_TABLES.has(table(bytes)); }, // 전용 그림과 일반 후보 genus 관문을 구별한다.
            plainCarrier: (sid) => Boolean(o.carrierRegular && o.carrierRegular.handles(sid) && table(raw(sid)) !== 0x5040c0), // 크랩은 다른 전처리라 제외한다.
            vortex: (sid) => VORTEX_TABLES.has(table(raw(sid))), // 현재 네 신전 가상 표다.
            fenceCommon: (sid) => { const bytes = raw(sid); return (bytes[10] === 90 && table(bytes) === 0x5092d0) || (bytes[10] === 170 && table(bytes) === 0x509388); }, // 표식/포스 필드의 실제 두 쌍이다.
            flyer: (sid) => { const bytes = raw(sid); const vt = table(bytes); return (bytes[10] === 144 && vt === 0x50a388) || (bytes[10] === 73 && vt === 0x50a1e8) || (bytes[10] === 138 && vt === 0x50a050); }, // 세 TYPE/VT의 기존 정적 가상 수명 자격이다.
            visualDummy: (sid) => { const bytes = raw(sid); return bytes[10] === 110 && table(bytes) === 0x50eb98; }, // 표시 더미의 일반 수명이다.
        };
    }

    // 세 기존 래퍼를 합치고 typed 파생 Pre/Post 뒤 사제 전처리·최종 가상 삭제를 연결한다.
    bind() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.gemModules?.ready || this.#started) throw new Error('Session 자산 삭제의 초기화 전/반복 결선'); this.#started = true;
        this.kinds = this.#makeKinds(); const k = this.kinds; const hooks = o.worldHooks;
        hooks.assets = o.construction.destroyHooks(o.lifetime.destroyHooks(o.hits.destroyHooks(hooks.assets)));
        hooks.destroy = o.construction.destroy(o.lifetime.destroy(o.hits.destroy(o.services.destroy)));
        const previous = hooks.assets.emit;
        const emit = (event) => {
            this.#live(); const sid = event.sid; const flags = event.flags; const pre = event.effect === SQUID_DESTROY_EFFECT.preDestroy; const post = event.effect === SQUID_DESTROY_EFFECT.postDestroy;
            if (o.dudeBehavior && o.dudeBehavior.handles(sid)) { if (pre) { o.world.commonDeletion.preDestroy(sid, flags); return; } if (post) { o.dudeBehavior.postDestroy(sid, flags); return; } }
            if ((k.bridge(sid) || k.connector(sid)) && (pre || post)) { c.surfaces.hooks().emit(event); return; }
            if (o.geyserLifetime.handles(sid)) { if (pre) { o.geyserLifetime.preDestroy(sid, flags); return; } if (post) { o.geyserLifetime.postDestroy(sid, flags); return; } }
            if (o.daisLifecycle && o.daisLifecycle.handles(sid)) {
                if (pre) { o.runtimeModules.prepareRitual(); if (o.altarSacrifice.handles(sid)) o.daisLifecycle.altarPreDestroy(sid, flags); else o.altarSacrifice.daisPreDestroy(sid, flags); return; }
                if (post) { o.daisLifecycle.postDestroy(sid, flags); return; }
            }
            if (k.growingIce(sid)) { if (pre) { o.world.commonDeletion.preDestroy(sid, flags); return; } if (post) { o.iceTower.growingPostDestroy(sid, flags); return; } }
            if (k.blocker(sid)) { if (pre) { o.damageablePre.preDestroy(sid, flags); return; } if (post) { o.iceTower.postDestroy(sid, flags); return; } }
            if (k.plainDamageable(sid) || k.sunFence(sid) || k.nugget(sid)) { if (pre) { if (k.sunFence(sid)) o.fenceLifetime.preDestroy(sid, flags); else o.damageablePre.preDestroy(sid, flags); return; } if (post) { this.#post(sid, flags); return; } }
            if (k.plainCarrier(sid) || o.balloonBody.handles(sid)) { if (pre) { if (o.balloonBody.handles(sid)) o.balloonBody.preDestroy(sid, flags); else o.priests.carrier.preDestroy(sid, flags); return; } if (post) { this.#post(sid, flags); return; } }
            if (k.priest(sid) && post) { this.#post(sid, flags); return; }
            if (k.bomb(sid) && o.bombPre.handles(sid) && pre) { o.bombPre.preDestroy(sid, flags); return; }
            if (k.rune(sid) || k.fenceCommon(sid) || k.visualDummy(sid) || k.priestForcefield(sid) || k.buried(sid) || k.bomb(sid)) { if (pre) { o.world.commonDeletion.preDestroy(sid, flags); return; } if (post) { o.world.commonDeletion.postDestroy(sid, flags); return; } }
            if (k.flyer(sid)) { if (pre) { o.flyerPre.preDestroy(sid, flags); return; } if (post) { this.#post(sid, flags); return; } }
            if (k.vortex(sid)) { if (pre) { o.vortexPre.preDestroy(sid, flags); return; } if (post) { this.#post(sid, flags); return; } }
            if (typeof previous !== 'function') throw new Error('Session의 다른 기존 자산 삭제 효과 미연결'); previous(event);
        };
        hooks.assets.emit = emit; this.emit = emit;
        hooks.assets = o.priests.destroyHooks(hooks.assets);
        const previousDestroy = hooks.destroy;
        const destroy = (sid, flags, effects) => {
            this.#live();
            if (k.bridge(sid) || k.connector(sid) || o.geyserLifetime.handles(sid) || k.vortex(sid) || k.fenceCommon(sid) || k.flyer(sid) || o.balloonBody.handles(sid) || k.visualDummy(sid) || k.plainDamageable(sid) || k.blocker(sid) || k.growingIce(sid) || k.sunFence(sid) || k.nugget(sid) || (o.daisLifecycle && o.daisLifecycle.handles(sid)) || (o.dudeBehavior && o.dudeBehavior.handles(sid)) || k.priestForcefield(sid) || k.plainCarrier(sid) || k.priest(sid) || k.buried(sid) || k.bomb(sid) || k.rune(sid)) return c.virtualDestroy.destroy(sid, flags, effects);
            if (typeof previousDestroy !== 'function') throw new Error('Session의 다른 기존 가상 삭제 미연결'); return previousDestroy(sid, flags, effects);
        };
        this.destroy = hooks.destroy = destroy; this.#ready = true;
    }

    // 각 실제 Damageable Post 바로 앞에서 같은 현재 상태를 준비한다.
    #post(sid, flags) { const o = this.#owner; o.damageableModules.preparePost(); o.damageablePost.postDestroy(sid, flags); }
    // 반납한 이전 Session의 삭제 closure가 World에 재진입하지 못한다.
    #live() { if (this.#disposed) throw new Error('반납한 Session 자산 삭제'); }
    // 상위가 실제 Host를 비운 뒤 훅 대여를 닫는다. World 게시/해제는 상위의 몫이다.
    dispose() { this.#disposed = true; }
    // typed 삭제 분배의 정상 복귀이며 실제 Runtime 게시/Load 완료는 아니다.
    get ready() { return this.#ready; }
}
