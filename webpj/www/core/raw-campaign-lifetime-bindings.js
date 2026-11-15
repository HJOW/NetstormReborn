// 실제 공통 자산/기술 ContentForm/Player 부모 통지를 같은 세션 풀과 삭제 분배기에 연결하는 client 소비자이다(cpppj `client/RawCampaignLifetimeBindings.cpp/.h`, 10.78 전용).
// 사용:
//   const lifetime = new RawCampaignLifetimeBindings(world, forms, contentState, contentEffects, reward, playerNotice, fallbackParent = null);
//   lifetime.destroyHooks(fallbackHooks) 는 84(빈 간헐천)·103/104/105/106/134 의 입증된 (타입, 가상 표) 쌍의 PreDestroy/PostDestroy 만 공통 삭제 몸체(`core.surfaces.hooks().emit`)로 보내고 다른 효과는 fallback 에 남긴다.
//   lifetime.destroy(fallback) 는 같은 쌍의 가상 삭제를 공통 삭제기(`core.virtualDestroy.destroy`)로 실행하고 다른 파생은 fallback 에 남긴다. lifetime.parentNotice(parent, form, reason) 는 Player 가상 표만 Player 통지로 보낸다.
//   lifetime.contents() · lifetime.pool. 모듈 함수: bindPlayerContentNoticeGumps(registry, events, hooks) · makeCampaignDeletionEffects(effects, unitLost).
// 구성: ContentFormLifetime(TYPE 6 의 실제 pre/Unpop/부모 알림/post/SID 반납)을 `forms`·`state`·`reward` 와 월드의 삭제기/Form 분배 훅으로 한 번 만들어 Form 분배기에 등록한다.
//   세션이 해제되기 전에 World.clearProcesses 를 실행해야 하며(등록 콜백이 이 객체를 가리킨다) 새 보상/Player/공통 삭제 state 사본은 만들지 않는다.
// **옮기지 않은 것:** cpppj 의 Native 장기 Storage 경로(`UsesNativeTransmissionDevices`, Client 수명이 이미 같은 본문을 등록한 경우)는 웹 `RawSurfaceStorage`에 대응 개념이 없다. 웹은 항상 독립(typed) 경로이며
//   Client 장기 소유자(`ClientMain`)가 생길 때 같은 자리에 추가한다(4-D 구획 E). 이 모듈은 DOM 을 쓰지 않는다. 기준선 뒤 cpppj 변경(+9줄)은 Native 경로의 소유자 확인이라 웹 결과에 영향이 없다(3.6절 ①).
// (2026-11-15 WB04-E521: 4-D C 구획 RawCampaignLifetimeBindings 이식)
import { ContentFormLifetime } from './content-form-lifetime.js';
import { SQUID_DELETION_EFFECT } from './squid-destroy-lifecycle.js';
import { SQUID_DESTROY_EFFECT } from './squid-destroy.js';

// 입증된 가상 표 주소이다: 기본 자산(0x501dd0)과 Player(0x50ee08), 그리고 TYPE 별 공통 몸체를 쓰는 표이다. (2026-11-15 WB04-E521)
const BASE_VTABLE = 0x501dd0;
const PLAYER_VTABLE = 0x50ee08;
// 빈 간헐천 TYPE 번호이다. (2026-11-15 WB04-E521)
const EMPTY_GEYSER = 84;
// 공통 삭제 몸체를 쓰는 (TYPE, 가상 표) 쌍이다. 마나(103)와 플레어 등의 +10/+14/+18/+24/+48 은 base 와 같고 range 104 는 실제 생성자 표의 0 으로 base 표를 쓴다. (2026-11-15 WB04-E521)
const COMMON_PAIRS = Object.freeze([
    [103, 0x50c3b0],
    [104, BASE_VTABLE],
    [105, 0x509810],
    [106, 0x5096a0],
    [134, 0x5070c0],
]);

// 원본 포인터 기록 DWORD 를 호스트 실행 주소로 바꾸지 않고 슬롯 첫 네 BYTE 에서 읽는다. (2026-11-15 WB04-E521)
function lifetimeVtable(raw) {
    return (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
}

// 원본 00463ff0 의 같은 등록 배열과 대상 자신의 현재 가상 handle 을 사용하는 Player 내용물 통지 훅이다. registry 는 `GumpCreateState`, events 는 `{ handle }` 이다. (2026-11-15 WB04-E521)
export function bindPlayerContentNoticeGumps(registry, events, hooks = {}) {
    if (!(registry?.entries instanceof Uint32Array) || typeof events?.handle !== 'function') throw new TypeError('Player 내용물의 같은 판본/실제 Gump handle 누락');
    return {
        ...hooks,
        gump: (id) => {
            if (!(id >= 0 && id < registry.entries.length)) throw new RangeError('Player 내용물 현재 Gump 등록ID 물리 범위');
            return registry.entries[id];
        },
        // 처리 결과는 원본처럼 사용하지 않는다.
        handle: (token, event, form, reason) => { events.handle(token, event, form, reason); },
    };
}

// 실제 공통 삭제의 선택/소리 요청을 현재 UI/파생 송신 소비자로 변환한다. Refund 는 실제 reward 가 직접 처리하므로 여기서는 거부한다. (2026-11-15 WB04-E521)
export function makeCampaignDeletionEffects(effects, unitLost) {
    if (typeof effects?.emit !== 'function' || typeof unitLost !== 'function') throw new TypeError('공통 삭제의 실제 선택/소리/파생 효과 누락');
    return {
        forward: effects.emit,
        emit: (event) => {
            if (event.effect === SQUID_DELETION_EFFECT.clearSelection) {
                effects.emit({ effect: SQUID_DESTROY_EFFECT.clearSelection, sid: event.sid, parent: 0, flags: 0 });
                return;
            }
            if (event.effect === SQUID_DELETION_EFFECT.unitLostSound) {
                unitLost(event.sid);
                return;
            }
            throw new Error('공통 삭제 Refund는 같은 실제 SquidReward를 공급해야 합니다');
        },
    };
}

// 기존 내용물 몸체만 소유하고 새 보상/Player/공통 삭제 state 사본을 만들지 않는 연결이다. (2026-11-15 WB04-E521)
export class RawCampaignLifetimeBindings {
    // 실제 풀/장부/공통 삭제/분배기의 참조(월드 Core 뷰)와 같은 풀의 현재 Player+24 통지 몸체, 다른 실제 부모 통지 경계(워커/다이스/워크샾 등)이다.
    #core;
    #player;
    #fallbackParent;
    // 기존 Form 6 의 실제 pre/Unpop/부모 알림/post/SID 반납을 등록한 Content 수명이다.
    #contents;

    // world 와 현재 Content 상태/Host/reward/Player 통지는 이 객체보다 오래 살아야 한다. 같은 PATCH 풀이 아니면 어떤 등록도 하기 전에 거부한다. (2026-11-15 WB04-E521)
    constructor(world, forms, state, effects, reward, player, fallbackParent = null) {
        const core = world.core();
        if (core.pool.edition !== 'patch1078' || player?.pool !== core.pool) throw new TypeError('캠페인 내용물/Player 통지는 같은 PATCH 풀이어야 합니다');
        this.#core = core;
        this.#player = player;
        this.#fallbackParent = fallbackParent;
        // 기존 Form 분배기의 타입 6 만 등록하며 다른 Host/감시/프로세스 경로를 덮어쓰지 않는다.
        this.#contents = new ContentFormLifetime(core.pool, core.types, forms, core.destroy, state, effects, core.forms.hooks(), reward);
        core.forms.bind(this.#contents);
    }

    // 현재 raw 타입과 가상 표를 둘 다 확인하며 일반 base 자산 전체로 확장하지 않는다. (2026-11-15 WB04-E521)
    #emptyGeyser(sid) {
        const raw = this.#core.pool.slot(sid);
        return raw[10] === EMPTY_GEYSER && lifetimeVtable(raw) === BASE_VTABLE;
    }

    // 입증된 타입/가상 표 쌍만 공통 삭제·Pre/Post 몸체로 보낸다. 다른 타입으로 확대하지 않는다. (2026-11-15 WB04-E521)
    #commonAsset(sid) {
        if (this.#emptyGeyser(sid)) return true;
        const raw = this.#core.pool.slot(sid);
        const vtable = lifetimeVtable(raw);
        return COMMON_PAIRS.some(([type, table]) => raw[10] === type && vtable === table);
    }

    // Pre/Post 에만 입증된 공통 모듈을 선택하고 종속/전송/선택은 현재 기존 분배를 유지한다. (2026-11-15 WB04-E521)
    destroyHooks(fallback) {
        const hooks = { ...fallback };
        hooks.emit = (event) => {
            if (this.#commonAsset(event.sid) && (event.effect === SQUID_DESTROY_EFFECT.preDestroy || event.effect === SQUID_DESTROY_EFFECT.postDestroy)) {
                this.#core.surfaces.hooks().emit(event);
                return;
            }
            if (typeof fallback?.emit !== 'function') throw new Error('다른 캠페인 실제 삭제 효과 미연결');
            fallback.emit(event);
        };
        return hooks;
    }

    // 같은 RawSquidDestroy 가 dead/종속/전파/Unpop/PrePost 깊이/반납을 실제로 처리한다. 다른 파생은 fallback 을 쓴다. (2026-11-15 WB04-E521)
    destroy(fallback = null) {
        return (sid, flags, hooks) => {
            if (this.#commonAsset(sid)) return this.#core.virtualDestroy.destroy(sid, flags, hooks);
            if (typeof fallback !== 'function') throw new Error('다른 캠페인 실제 파생 삭제 미연결');
            return fallback(sid, flags, hooks);
        };
    }

    // RawContentParentNotice 의 실제 파생 callback 이다. Player 가상 표만 새 몸체로 보내고 다른 부모는 명시 경계로 둔다. (2026-11-15 WB04-E521)
    parentNotice(parent, form, reason) {
        if (lifetimeVtable(this.#core.pool.slot(parent)) === PLAYER_VTABLE) {
            this.#player.notify(parent, form, reason);
            return;
        }
        if (typeof this.#fallbackParent !== 'function') throw new Error('다른 캠페인 내용물 실제 부모 통지 미연결');
        this.#fallbackParent(parent, form, reason);
    }

    // 같은 내용물 수명/풀의 검증 경계이다. 세션은 해제 전에 World.clearProcesses 를 실행한다. (2026-11-15 WB04-E521)
    contents() { return this.#contents; }

    // 같은 풀 확인용 실제 풀이다. (2026-11-15 WB04-E521)
    get pool() { return this.#core.pool; }
}
