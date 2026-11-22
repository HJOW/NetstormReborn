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
// 공통 삭제(CommonDeletion)의 Pre/Post 만 쓰는 열세 가상 표다. 원문 AssetDeletionVtables.inc 의 (전처리 4b0950·후처리 4b0840) 묶음 중 주문 폭발/효과 자산 일곱(0x5026a0..0x502b70)과 0x50c9f0·0x50cc08·0x50db78,
// 전처리가 공통 Pre 와 같은 단일 점프(41f910)인 폭발 효과 셋(0x502c38·0x502cf0·0x502db0)이다. 이전에는 이 표들을 가상 삭제하면 '다른 실제 자산 가상 삭제 미연결' 오류가 났다. (2026-11-22 WB04-E1215)
const COMMON_ONLY_TABLES = new Set([0x5026a0, 0x502778, 0x502840, 0x502908, 0x5029d0, 0x502aa8, 0x502b70, 0x50c9f0, 0x50cc08, 0x50db78, 0x502c38, 0x502cf0, 0x502db0]);
// 거주 구역(0x50f540)과 Player(0x50ee08)의 가상 표다. 후처리(거주 목록 제거)/전처리(Player 기록 정리)가 각각 따로 있다. (2026-11-22 WB04-E1215)
const RESIDENCE_TABLE = 0x50f540;
const PLAYER_TABLE = 0x50ee08;
// Player 기록 한 개의 크기(0xb4)와 PreDestroy 가 만지는 두 필드의 위치(+0x9c 이전 포인터, +0x50 상태 비트)이다. (2026-11-22 WB04-E1215)
const PLAYER_RECORD_BYTES = 0xb4, PLAYER_POINTER_AT = 0x9c, PLAYER_STATE_AT = 0x50;
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
            commonOnly: (sid) => COMMON_ONLY_TABLES.has(table(raw(sid))), // 공통 Pre/Post 만 쓰는 열세 가상 표다(원문 .inc 의 4b0950/4b0840 묶음과 41f910 단일 점프).
            residence: (sid) => table(raw(sid)) === RESIDENCE_TABLE, // 거주 구역: 공통 Pre, 후처리에서 거주 목록 제거 뒤 공통 Post 이다.
            player: (sid) => table(raw(sid)) === PLAYER_TABLE, // Player 객체: 전용 Pre(기록 정리 + 공통 Pre), 공통 Post 이다.
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
            // 원문 Native 삭제 분배(RawSurfaceAssetDeletion/RegistryDeletion)의 남은 가상 표: 공통만 쓰는 표·거주 구역·Player. (2026-11-22 WB04-E1215)
            if (k.commonOnly(sid)) { if (pre) { o.world.commonDeletion.preDestroy(sid, flags); return; } if (post) { o.world.commonDeletion.postDestroy(sid, flags); return; } }
            if (k.residence(sid)) { if (pre) { o.world.commonDeletion.preDestroy(sid, flags); return; } if (post) { this.#residencePost(sid, flags); return; } }
            if (k.player(sid)) { if (pre) { this.#playerPre(sid, flags); return; } if (post) { o.world.commonDeletion.postDestroy(sid, flags); return; } }
            if (typeof previous !== 'function') throw new Error('Session의 다른 기존 자산 삭제 효과 미연결'); previous(event);
        };
        hooks.assets.emit = emit; this.emit = emit;
        hooks.assets = o.priests.destroyHooks(hooks.assets);
        const previousDestroy = hooks.destroy;
        const destroy = (sid, flags, effects) => {
            this.#live();
            if (k.bridge(sid) || k.connector(sid) || o.geyserLifetime.handles(sid) || k.vortex(sid) || k.fenceCommon(sid) || k.flyer(sid) || o.balloonBody.handles(sid) || k.visualDummy(sid) || k.plainDamageable(sid) || k.blocker(sid) || k.growingIce(sid) || k.sunFence(sid) || k.nugget(sid) || (o.daisLifecycle && o.daisLifecycle.handles(sid)) || (o.dudeBehavior && o.dudeBehavior.handles(sid)) || k.priestForcefield(sid) || k.plainCarrier(sid) || k.priest(sid) || k.buried(sid) || k.bomb(sid) || k.rune(sid) || k.commonOnly(sid) || k.residence(sid) || k.player(sid)) return c.virtualDestroy.destroy(sid, flags, effects);
            if (typeof previousDestroy !== 'function') throw new Error('Session의 다른 기존 가상 삭제 미연결'); return previousDestroy(sid, flags, effects);
        };
        this.destroy = hooks.destroy = destroy; this.#ready = true;
    }

    // 거주 구역 후처리(원문 ResidencePostDestroy): 상태 extra(+40)의 비트 9 가 없으면 거주 목록의 일치 항목을 모두 지운 뒤 공통 Post 를 부른다. (2026-11-22 WB04-E1215)
    #residencePost(sid, flags) {
        const o = this.#owner;
        if (!(o.core.pool.slot(sid)[40] & 9)) o.state.assetState.currentResidences().removeAll(sid);
        o.world.commonDeletion.postDestroy(sid, flags);
    }
    // Player 전처리(원문 RawPlayerWorld::PreDestroyNative): 소유자 기록의 이전 포인터(+0x9c)가 이 SID 면 0 으로 쓰고, 공통 Pre 를 부른 뒤 같은 기록의 상태(+0x50)에서 비트 0 을 지우고 0x100 을 켠다.
    // 각 주소는 공통 Pre 가 소유자를 바꿔도 되도록 그때그때 현재 소유자 BYTE 로 다시 계산한다. (2026-11-22 WB04-E1215)
    #playerPre(sid, flags) {
        const o = this.#owner; const pool = o.core.pool;
        const field = (offset) => { // 현재 소유자 기록의 4바이트 필드 DataView 를 돌려준다(범위를 벗어나면 거부).
            const start = pool.slot(sid)[34] * PLAYER_RECORD_BYTES + offset; const records = o.playerRecords.records();
            if (start + 4 > records.length) throw new RangeError('Player 전처리 현재 기록 범위');
            return new DataView(records.buffer, records.byteOffset + start, 4);
        };
        { const view = field(PLAYER_POINTER_AT); if (view.getUint32(0, true) === sid) view.setUint32(0, 0, true); } // 이전 Player 포인터를 비운다.
        o.world.commonDeletion.preDestroy(sid, flags);
        { const view = field(PLAYER_STATE_AT); view.setUint32(0, (view.getUint32(0, true) & 0xfffffffe) >>> 0, true); }
        { const view = field(PLAYER_STATE_AT); view.setUint32(0, (view.getUint32(0, true) | 0x100) >>> 0, true); }
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
