// 다이스 수명(raw-dais-lifecycle.js) 시험이 함께 쓰는 장면 보조이다(cpppj DaisLifecycleTests 의 장면을 같은 순서로 옮김).
// lifecycleScene 은 원본 관측 한 행의 입력을 채운 합성 장면(실제 Frame/Host/Damageable/Carrier 부품 + 효과 기록 훅)을 만들고,
// 마지막 공통 Damageable/Carrier 진입 경계에서 검사만 중단시키는 표식(LifecycleBodyEnd)을 던지게 한다. 실제 게임·브라우저 실행이 아니다.
// (2026-11-13 WB04-E369: 7-P3 RawDaisLifecycle 이식으로 추가)
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { setRawSquidHitPoints } from '../www/core/raw-squid-word.js';
import { RawFrameAdvance } from '../www/core/raw-frame-advance.js';
import { CarrierPostPopState, RawCarrierPostPop } from '../www/core/raw-carrier-post-pop.js';
import { DamageablePreDestroyState, RawDamageablePreDestroy } from '../www/core/raw-damageable-pre-destroy.js';
import { AltarSacrificeState } from '../www/core/raw-ritual-regular.js';
import { RawAltarSacrifice, PATCH_ALTAR_VTABLE } from '../www/core/raw-altar-sacrifice.js';
import { PATCH_DAIS_VTABLE } from '../www/core/raw-dais-content-notice.js';
import { DaisLifecycleState, RawDaisLifecycle } from '../www/core/raw-dais-lifecycle.js';
import { VTABLE, hex, forbid, corePatterns, replaceAltarFrames, makeWorld } from './dais-support.mjs';

// 마지막 Damageable/Carrier 진입 경계에서 검사만 중단시키는 표식이다. 그 전체 삭제/후처리를 완료로 표시하지 않는다. (2026-11-13 WB04-E369)
export class LifecycleBodyEnd extends Error {}

// 10.78 Player 기록의 크기와 필드 위치(획득 플래그 +0x71, SID 쌍 +0x8c.., 홈 섬 +0x78)이다. (2026-11-13 WB04-E369)
export const STRIDE = 0xb4;
export const GRANT_AT = 0x71;
export const PAIR_AT = 0x8c;
export const HOME_AT = 0x78;

// 제단 몸체가 수명 시험에서 쓰지 않는 하위 효과는 모두 실행 시 실패하게 만든다(typedDais 조회만 쓴다). (2026-11-13 WB04-E369)
export function altarForbiddenHooks() {
    const names = ['setFrame', 'destroy', 'reserveShared', 'addShared', 'randomInt', 'randomFloat', 'playAt', 'playGlobal', 'removeAnimation', 'unpop', 'repop', 'create', 'setOwner', 'pop', 'escapeEffect', 'refreshOwner', 'damageable'];
    return Object.fromEntries(names.map((name) => [name, forbid(`수명 typed 조회의 미선언 하위효과 ${name}`)]));
}

// 한 행의 장면을 만든다. input 은 관측 입력 JSON, bound 가 참이면 실제 Player 기록 9칸(0xb4 바이트)을 묶고 독립 배열은 0xdead 로 오염시킨다.
//  - options.lifecycleHooks: 수명 훅 덮어쓰기(함수 하나만 바꾸는 계약 시험용), options.lifecycleState: 수명 상태 추가 설정(gameTime 상자 등).
// 반환: { world, effects, boundaryImages, lifecycle, lifecycleState, records, beforeRecords, carrierState, snapshot, hooks }. (2026-11-13 WB04-E369)
export function lifecycleScene(input, bound, options = {}) {
    const effects = [];
    const boundaryImages = [];
    let world;
    const snapshot = () => { for (const sid of [5, 7, 8, 9]) boundaryImages.push(hex(world.pool.slot(sid))); };
    let updates = 0;
    const frameHooks = {
        frameSize: () => [1, 1],
        update: (sid, flags) => {
            assert.equal(flags, 0);
            updates += 1;
            if (updates % 2 === 0) effects.push(`F:${sid},${get(world.pool.slot(sid), 36, 4)},0`);
        },
        unpop: forbid('Altar fixture unexpected Frame Unpop'),
        pop: forbid('Altar fixture unexpected Frame Pop'),
    };
    world = makeWorld({ patterns: corePatterns('A'), frameHooks });
    replaceAltarFrames(world);
    // 새 제단 관찰의 입력 코드만 기존 실제 구간표로 바꾼다.
    const pattern165 = `${'K'.repeat(8)}${'L'.repeat(8)}${'M'.repeat(8)}${'N'.repeat(8)}${'O'.repeat(8)}${'P'.repeat(8)}${'A'.repeat(132)}`;
    world.frames[165] = new FrameCodeTable([...pattern165].map((letter, index) => ({ side: letter.charCodeAt(0), variant: 0x50, number: index + 1, flags: 0 })));
    const { pool, slots, insert, state } = world;
    insert(5, 165);
    insert(6, 158);
    insert(7, 164);
    put(slots[5], 0, PATCH_DAIS_VTABLE);
    put(slots[5], 36, input.frame >>> 0, 4);
    put(slots[6], 36, 0, 4);
    put(slots[7], 36, 3, 4);
    slots[5][34] = 1;
    slots[6][34] = 1;
    slots[7][34] = 1;
    setRawSquidHitPoints(pool, 6, 100);
    world.types[158].flags2 = 0x210000;
    put(slots[5], 6, 0, 2);
    put(slots[10], 0, VTABLE);
    slots[10][10] = 6;
    put(slots[10], 12, 2, 2);
    put(slots[10], 18, 0, 2);
    put(slots[10], 20, 36, 2);
    // 원본과 같은 현재 해시 단계/owner/제단 WORD/초기 시각 입력이다.
    for (const sid of [5, 6, 7]) slots[sid][33] = 1;
    slots[5][34] = input.owner;
    slots[5][11] = input.dead ? 2 : 0;
    put(slots[7], 0, PATCH_ALTAR_VTABLE);
    put(slots[7], 12, 2, 2);
    state.localOwner = input.local;
    world.runeState.suppressDuration = input.suppress !== 0;
    world.templeState.boss = input.boss !== 0;
    state.grantFlags.fill(input.saved);
    const raw9 = slots[9];
    put(raw9, 0, VTABLE);
    raw9[10] = input.mode === 'apre' ? 166 : 91;
    raw9[40] = input.runeExtra;
    put(raw9, 14, floatToBits(40.75));
    put(raw9, 18, floatToBits(41.25));
    if (input.mode === 'apre') {
        put(raw9, 4, world.hash.bucket(1, 40.75, 41.25), 2);
        world.hash.setBucket(1, 40.75, 41.25, 9);
    }
    const carrierState = new CarrierPostPopState();
    carrierState.loadingDepth = input.loading;
    const lifecycleState = new DaisLifecycleState(carrierState, input.initial);
    options.lifecycleState?.(lifecycleState);
    // mirror 는 독립 DWORD 이므로 처음부터 같은 값으로 추정하지 않는다.
    for (const player of lifecycleState.players) Object.assign(player, { first: input.first, firstMirror: 123, second: input.second, secondMirror: 124, homeIsland: 17 });
    const damageHooks = {
        collapse: forbid('수명 typed 조회의 collapse'), explosion: forbid('수명 typed 조회의 explosion'), sound: forbid('수명 typed 조회의 sound'),
        releaseContained: forbid('수명 typed 조회의 releaseContained'), base: forbid('수명 typed 조회의 base'),
        validateBase: (sid, flags) => { effects.push(`B:${sid},${flags}`); snapshot(); throw new LifecycleBodyEnd('경계'); },
    };
    const damageable = new RawDamageablePreDestroy(pool, world.spots, world.containedState, new DamageablePreDestroyState(), damageHooks);
    const advance = new RawFrameAdvance(pool, world.frames, world.frame);
    const altarState = new AltarSacrificeState();
    const altar = new RawAltarSacrifice(pool, world.hash, world.types, world.frames, world.footprint, world.rune, world.ritual, world.carrier, world.dais, advance, world.host, damageable, state, altarState, altarForbiddenHooks());
    const baseHook = (sid, flags) => { effects.push(`postpop:${sid},${flags}`); snapshot(); throw new LifecycleBodyEnd('경계'); };
    const carrierPost = new RawCarrierPostPop(pool, world.types, carrierState, { carrierCheck: forbid('수명 Carrier 확인'), claimGround: forbid('수명 지면 소유'), validateBase: baseHook, base: baseHook });
    const records = new Uint8Array(9 * STRIDE).fill(0xa5);
    if (bound) {
        // 원본 관측과 같은 Player 입력을 실제 BYTE/DWORD 위치에 쓴다.
        for (let owner = 0; owner < 9; owner += 1) {
            records[owner * STRIDE + GRANT_AT] = state.grantFlags[owner];
            const value = lifecycleState.players[owner];
            const at = owner * STRIDE + PAIR_AT;
            put(records, at, value.first);
            put(records, at + 4, value.firstMirror);
            put(records, at + 8, value.second);
            put(records, at + 12, value.secondMirror);
            put(records, owner * STRIDE + HOME_AT, value.homeIsland);
        }
        state.bindPlayerRecords(records);
        state.grantFlags.fill(0xde);
        // 사본을 읽으면 원본 행과 달라져 반드시 실패한다.
        for (const player of lifecycleState.players) Object.assign(player, { first: 0xdead, firstMirror: 0xdead, second: 0xdead, secondMirror: 0xdead, homeIsland: 0xdead });
    }
    const beforeRecords = Uint8Array.from(records);
    const bits = (value) => floatToBits(value);
    const hooks = {
        setFrame: (sid, value, flags) => {
            assert.equal(flags, 0);
            effects.push(`F:${sid},${value >>> 0},0`);
            put(pool.allocated(sid), 36, value >>> 0, 4);
        },
        unpop: (sid, flags) => { effects.push(`unpop:${sid},${flags}`); },
        repop: (sid, flags) => { effects.push(`repop:${sid},${flags}`); },
        create: (type, flags) => {
            assert.deepEqual([type, flags], [164, 0]);
            effects.push('C:164,0,8');
            slots[8].fill(0);
            put(slots[8], 0, VTABLE);
            slots[8][10] = 164;
            slots[8][11] = 4;
            return 8;
        },
        setOwner: (sid, owner) => { effects.push(`owner:${sid},${owner}`); slots[sid][34] = owner; },
        pop: (sid, x, y, flags) => {
            effects.push(`P:${sid},${bits(x)},${bits(y)},${flags}`);
            put(slots[sid], 14, bits(x));
            put(slots[sid], 18, bits(y));
        },
        reserveRegular: () => { effects.push('N:40'); return input.reserve !== 0; },
        addRegular: (sid, event, payload) => { effects.push(`REG:${event},${sid},${bits(payload)}`); },
        surface: (x, y) => { effects.push(`SURFACE:${bits(x)},${bits(y)}`); return input.profile ? 9 : 0; },
        hasIsland: (sid) => { effects.push(`HAS:${sid}`); return input.profile !== 1; },
        islandNumber: (sid) => { effects.push(`ISLAND:${sid}`); return input.profile === 3 ? 17 : 18; },
        option: (key) => { effects.push(`OPTION:${key}`); return input.profile === 4; },
        chat: (key) => { effects.push(`CHAT:${key}`); },
        destroy: (sid, flags) => { effects.push(`destroy:${sid},${flags}`); },
        damageablePostDestroy: (sid, flags) => { effects.push(`post:${sid},${flags}`); snapshot(); throw new LifecycleBodyEnd('경계'); },
        ...(options.lifecycleHooks ?? {}),
    };
    const lifecycle = new RawDaisLifecycle(pool, world.hash, world.types, world.footprint, world.ritual, altar, carrierPost, damageable, state, altarState, lifecycleState, hooks);
    return { world, effects, boundaryImages, lifecycle, lifecycleState, records, beforeRecords, carrierState, snapshot, hooks, altar, carrierPost, damageable, altarState };
}
