// 독립 NoWorld/typed 생성자1757..1814의 전체 Regular 조립이다(225553df, WB04-E615).
// 사용: 자산 삭제 결선 뒤 bind. Native 장기 Regular 분배와 실제 Runtime 게시1825는 후속이다.
import { RawBuriedRegular } from './raw-buried-regular.js';
import { RawFrameAdvance } from './raw-frame-advance.js';
import { makeBridgeRegularHandler } from './raw-bridge-events.js';
import { runSpriteRegular } from './raw-sprite-regular.js';
import { runDecorationRegular } from './raw-decoration-regular.js';
import { handleHealBurstRegular, handleGravityBurstRegular } from './raw-bomb-heal.js';
import { COMMON_DAMAGEABLE_REGULAR_TABLES } from './campaign-damageable-regular-tables.js';
import { connectedCampaignBombTable } from './raw-campaign-session-particles.js';
import { PRIEST_VTABLE } from './raw-priest-owner.js';
import { logRegularFailure } from './client-diagnostics.js';

// 현재 raw 슬롯의 비정렬 가상 표 DWORD다.
function table(raw) { return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(0, true); }
// 실패 시점의 float32 페이로드 비트를 로그에 보낸다.
function floatBits(value) { const raw = new DataView(new ArrayBuffer(4)); raw.setFloat32(0, value, true); return raw.getUint32(0, true); }
// 선택된 후설치 모듈의 누락을 같은 호출 지점에서 거부한다.
function need(value, message) { if (!value) throw new Error(message); return value; }

// 같은 현재 Host와 실제 정기 몸체를 원문 우선순서로 조립한다.
export class RawCampaignSessionRegular {
    // 부분 실패/반복/반납과 원문 하위 함수 값이다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 검사/상위 결선이 조회하는 같은 함수 값과 두 현재 훅 표다.
    inner = null; handler = null; regular = null; playerHooks = null; priestHooks = null; decorationHooks = null;

    // 생성 자체는 프로세스·슬롯·게임 난수를 소비하지 않는다.
    constructor(owner) { this.#owner = owner; }

    // Buried와 현재 Player/사제 후단 뒤 실제 다층 처리기·특수 주문과 실패 로그를 구성한다.
    bind() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.deletionModules?.ready || this.#started || o.buriedRegular) throw new Error('Session 전체 Regular의 초기화 전/반복 결선'); this.#started = true;
        o.buriedRegular = new RawBuriedRegular(c.pool, c.frames, c.frame, o.random);
        const hooks = o.worldHooks; hooks.playerRegular ??= {}; hooks.priestRegular ??= {};
        hooks.playerRegular.state = (owner) => { this.#live(); return need(o.templateState, 'Player 정기의 현재 State getter 설치 전 호출').get(owner); }; // 현재 같은 큰 getter다.
        hooks.playerRegular.changed = (owner) => { this.#live(); need(o.templateState, 'Player 정기의 현재 Notify 설치 전 호출').changed(owner); }; // 같은 Player 통지다.
        hooks.priestRegular.unique = (sid, event, count, payload) => { // 실제 고유 후단만 선택하며 미설치 의식/기도를 기본 Carrier로 바꾸지 않는다.
            this.#live(); if (event === 602 || event === 603) return need(o.priests, '사제 회복/추락 몸체 설치 전 호출').uniqueRegular(sid, event, count, payload);
            if (event === 604) return need(o.priestRitualFrame, '사제604 몸체 설치 전 호출').tick(sid);
            if (event === 605) return need(o.priestPrayer, '사제605 몸체 설치 전 호출').tick(sid, count, payload);
            throw new Error('다른 사제 고유 정기 사건');
        };
        this.playerHooks = hooks.playerRegular; this.priestHooks = hooks.priestRegular;
        const inner = (sid, event, count, payload) => {
            this.#live();
            if (o.daisRegular && o.daisRegular.handles(sid)) { o.runtimeModules.prepareRitual(); return o.daisRegular.handle(sid, event, count, payload); }
            if (o.altarSacrifice && o.altarSacrifice.handles(sid)) { o.runtimeModules.prepareRitual(); return o.altarSacrifice.handle(sid, event, count, payload); }
            if (o.playerRegular && o.playerRegular.handles(sid)) return o.playerRegular.handle(sid, event, count, payload);
            if (o.manaRegular && o.manaRegular.handles(sid)) return o.manaRegular.run(sid, event, count);
            if (o.nuggetRegular && o.nuggetRegular.handles(sid)) return o.nuggetRegular.handle(sid, event, count, payload);
            const raw = c.pool.physical(sid);
            if (table(raw) === 0x50cc08) return runSpriteRegular(c.pool, c.types, c.frame, sid, payload);
            if (table(raw) === 0x50e668) return o.arcSpire.handle(sid, event, count, payload);
            if (table(raw) === 0x50e738) return o.acidFence.handle(sid, event, count, payload);
            if (o.deletionModules.kinds.growingIce(sid)) return o.iceTower.handle(sid, event, count, payload);
            if (table(raw) === 0x50a050) return o.windFlyer.handle(sid, event, count, payload);
            if (table(raw) === 0x504198) return o.balloonBody.cloudRegular(sid, event, count, payload,
                (parent, delta, flags) => { const advance = new RawFrameAdvance(c.pool, c.frames, c.frame); advance.advance(parent, delta, flags); },
                (parent, code, number, value) => o.carrierRegular.handle(parent, code, number, value));
            if (table(raw) === 0x512208) return o.contentForms.damage(sid, count | 0, payload); // ContentForm의 보존 API명이며 피해 호출이 아니다.
            const priest = raw[10] === 158 && table(raw) === PRIEST_VTABLE;
            if (priest && event === 604) return need(o.priestRitualFrame, '사제604 몸체 설치 전 호출').tick(sid);
            if (priest && event === 605) return need(o.priestPrayer, '사제605 몸체 설치 전 호출').tick(sid, count, payload);
            if (o.carrierRegular.handles(sid) || (priest && (event < 0x259 || event > 0x25d))) return o.carrierRegular.handle(sid, event, count, payload);
            if (table(raw) === 0x50ea08 || COMMON_DAMAGEABLE_REGULAR_TABLES.includes(table(raw))) return o.assetBindings.regular.damageableHandle(sid, event, count, payload);
            throw new Error('캠페인 현재 Kernel의 다른 실제 Regular 부모 미연결: TYPE' + c.pool.slot(sid)[10] + ' event' + event);
        };
        this.inner = inner;
        let actual = o.hits.handler(o.combat.handler(o.assetBindings.handler(o.buriedRegular.handler(o.priests.regularHandler(inner)))));
        actual = o.flare.handler(actual); actual = o.fenceRegular.handler(actual); actual = makeBridgeRegularHandler(c.pool, c.bridgeEvents, actual); // 실제 가상 표를 원문 우선순서로 검사한다.
        this.handler = actual;
        const decoration = {
            maxHitPoints: (sid) => o.hp.maxHitPoints(sid), // 현재 TYPE/역할과 같은 HP 몸체다.
            hitPoints: (sid, amount, owner, attacker) => { const call = need(o.decorationHit, '장식의 현재 가상98 미연결'); call(sid, amount >>> 0, owner | 0, attacker); }, // 현재 함수 값을 고른 뒤 인수 비트를 보존한다.
            destroy: (sid, flags) => { o.world.destroy(sid, flags); }, damageable: (sid, event, count, payload) => o.assetBindings.regular.damageableHandle(sid, event, count, payload), // 같은 가상 삭제/공통 Regular다.
        };
        this.decorationHooks = decoration;
        const refresh = (sid) => c.display.update(c.pool.slot(sid), 0); // 힐/그라비티의 같은 현재 표시 갱신이다.
        const destroy = (sid, flags) => { o.world.destroy(sid, flags); }; // 특수 주문도 현재 World에서 삭제한다.
        const regular = (sid, event, count, payload) => {
            this.#live();
            try {
                const vt = table(c.pool.slot(sid));
                if (vt === 0x50a820 || vt === 0x50a8e8) return runDecorationRegular(c.pool, () => o.state.assetState.readBoss(), sid, event, count, payload, decoration);
                if (vt === 0x502cf0 || vt === 0x502db0) return handleHealBurstRegular(c.pool, c.frames, c.frame, c.processes, refresh, destroy, sid, count, payload);
                if (vt === 0x502c38) return handleGravityBurstRegular(c.pool, c.frames, c.frame, c.processes, refresh, destroy, sid, count, payload);
                if (o.dudeRegular && o.dudeRegular.handles(sid)) return o.dudeRegular.handle(sid, event, count, payload);
                if (o.devastationRegular && connectedCampaignBombTable(table(c.pool.slot(sid)))) { o.runtimeModules.prepareDevastation(); return o.devastationRegular.handle(sid, event, count, payload); }
                return actual(sid, event, count, payload);
            } catch (error) {
                // CP1377의 현재 객체/프레임 문맥을 추가 기록하고 기존 stderr 대응 행과 원래 예외도 보존한다.
                logRegularFailure(c.pool, sid, event, count, payload);
                // C++ fprintf처럼 로그 장치 자체의 실패가 원래 게임 예외를 덮지 못한다.
                try { const type = sid >= 0 && sid < c.pool.capacity ? c.pool.slot(sid)[10] : 0xffff; console.error('실제 Regular 실패 SID' + (sid >>> 0) + ' TYPE' + type + ' event' + (event >>> 0) + ' count' + (count >>> 0) + ' payload' + floatBits(payload).toString(16).padStart(8, '0')); } finally { throw error; }
            }
        };
        this.regular = hooks.regular = regular; this.#ready = true;
    }

    // 반납한 이전 Session의 현재 World에 정기 효과로 재진입하지 못한다.
    #live() { if (this.#disposed) throw new Error('반납한 Session 전체 Regular'); }
    // 실제 Host를 비운 상위가 훅 대여를 닫는다. Runtime 공개/해제는 상위가 맡는다.
    dispose() { this.#disposed = true; }
    // 실제 typed 정기 조립의 정상 복귀이며 World 게시/Load의 완료는 아니다.
    get ready() { return this.#ready; }
}
