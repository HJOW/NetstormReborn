// 사제 의식 동작 raw-priest-ritual-actions.js(RawPriestRitualActions·makePriestRitualActionHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priestritualactions-x86.tsv 10.78(originals) 876행 — 모드(0 Begin / 1 Continue)·SID·단계·코드표 profile·타입·예약 성공·변이 단계·원시 상태 → 호출 기록(F 프레임 / N 예약 / S 604 공유 예약), 지정 슬롯 raw(가상 표 앞 4바이트 제외).
//    CD 10.72 행은 웹 범위(10.78) 밖이다.
//  - 계약: rawPriestRitualActions_real_frame_and_shared_consumer(실제 SquidFrame 쓰기/표시 요청과 같은 호스트/Kernel 의 604 공유 예약, 중복 예약, 범위 밖 SID 와 손상 단계, 누락 효과 거부).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E70: 사제 의식 동작 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { RawPriestRitualActions, makePriestRitualActionHooks } from '../www/core/raw-priest-ritual-actions.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E70)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priestritualactions-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 판본당 독립 입력 수, 합성 가상 표(호스트 주소가 아님)이다. (2026-11-08 WB04-E70)
const ROWS = 876;
const VTABLE = 0x11010000;

// 예외가 나는지 본다. (2026-11-08 WB04-E70)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E70)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다. (2026-11-08 WB04-E70)
const ritualTypes = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 앞 A 둘과 K..P 각 1/2/3개의 실제 글자 구간 입력을 만든다. 기대 선택값을 계산하지 않는다. (2026-11-08 WB04-E70)
function ritualCodes(profile) {
    const codes = [{ side: 0x41, variant: 0x50, flags: 1 }, { side: 0x41, variant: 0x50, flags: 2 }];
    // stage 마다 현재 자산의 같은 글자 구간을 구성한다.
    for (let stage = 0; stage < 6; stage += 1) {
        // 같은 글자의 실제 frame 수를 profile + 1 로 만든다.
        for (let index = 0; index <= profile; index += 1) codes.push({ side: 0x4b + stage, variant: 0x50, flags: index + 1 });
    }
    return new FrameCodeTable(codes);
}

test('PriestRitualActions: 원본 10.78 876행의 현재 타입 프레임 선택·604 공유 예약 호출 기록과 슬롯 raw 가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // 모든 고유 입력마다 새 원본 폭의 풀/프레임 표를 구성한다.
    for (const row of rows) {
        assert.equal(row.length, 4);
        const values = row[1].split(',').map(Number);
        assert.equal(values.length, 8);
        const [mode, sid, stage, profile, type, reserve, mutation, rawState] = values;
        const pool = new SidPool(24000, { server: true });
        const types = ritualTypes();
        const factory = new SquidFactory(pool, types);
        const dataSid = sid > 0 && sid < 24000 ? sid : 5;
        // SID 5 는 client 목록에서 생성하고, 서버 영역 SID 23999 만 실제 수신 Take 로 확보한다.
        assert.equal(dataSid === 5 ? factory.create(type, 2) : factory.take(type, dataSid), dataSid);
        const raw = pool.allocated(dataSid);
        raw.fill(0);
        put(raw, 0, VTABLE);
        raw[10] = type;
        raw[11] = rawState;
        const frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
        frames[133] = ritualCodes(profile);
        frames[158] = ritualCodes(profile);
        const effects = [];
        const hooks = {
            setFrame: (current, index, flags) => {
                assert.ok(current === sid && flags === 0);
                effects.push(`F:${current},${index >>> 0},${flags}`);
                put(raw, 36, index >>> 0);
                if (mutation === 1) raw[10] = type === 158 ? 133 : 158;
            },
            reserveShared: () => { effects.push('N:40'); if (mutation === 2) raw[11] = 2; return reserve !== 0; },
            addShared: (current, event, payload) => {
                assert.ok(current === sid && event === 604 && floatToBits(payload) === 0x3c23d70a);
                effects.push(`S:${event},${current},${floatToBits(payload)}`);
            }
        };
        const actions = new RawPriestRitualActions(pool, frames, hooks);
        if (mode) actions.continue(sid, stage >>> 0);
        else actions.begin(sid, stage >>> 0);
        assert.equal(effects.length === 0 ? '-' : effects.join(';'), row[2], `행 ${checked}: ${row[1]}`);
        const actual = hex(pool.slot(dataSid));
        assert.ok(actual.length === row[3].length && actual.slice(8) === row[3].slice(8), `행 ${checked}: ${row[1]}`);
        checked += 1;
    }
    assert.equal(checked, ROWS);
});

test('PriestRitualActions: 실제 SquidFrame 쓰기/표시 요청과 같은 호스트/Kernel 의 604 공유 예약을 연결한다', () => {
    const pool = new SidPool(24000, { server: true });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const types = ritualTypes();
    types[158].flags2 = 0x210000;
    types[158].constructorAddress = typeConstructorAddress(158);
    const factory = new SquidFactory(pool, types);
    const parent = factory.create(158, 2);
    const raw = pool.allocated(parent);
    raw[11] = 0;
    raw[33] = 1;
    const frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
    frames[158] = ritualCodes(1);
    let updates = 0;
    let reserved = 0;
    const frameHooks = {
        frameSize: (sid, index) => { assert.ok(sid === parent && index >= 0 && index < frames[158].codes.length); return [1, 1]; },
        update: (sid, flags) => { assert.ok(sid === parent && flags === 0); updates += 1; },
        unpop: () => { throw new Error('같은 hash단계 의식 프레임은 Unpop을 실행하지 않는다'); },
        pop: () => { throw new Error('같은 hash단계 의식 프레임은 Pop을 실행하지 않는다'); }
    };
    const frame = new SquidFrame(pool, types, frameHooks);
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, hash, spots), types);
    const kernel = new Kernel(32);
    const clock = new SquidProcessState();
    clock.now = 99;
    const host = new SquidProcessHost(pool, types, kernel, destroy, clock, {
        asset: { selected: () => 0, emit: () => { throw new Error('의식 프레임/예약 검사는 자산 삭제를 실행하지 않는다'); } }
    });
    const actions = new RawPriestRitualActions(pool, frames, makePriestRitualActionHooks(pool, frame, host, { reserveShared: () => { reserved += 1; return true; } }));
    actions.begin(parent, 2);
    assert.ok(updates === 2 && reserved === 0 && kernel.size === 0);
    actions.continue(parent, 4);
    assert.ok(updates === 4 && reserved === 1 && kernel.size === 1);
    const pending = host.findEvent(parent, 604);
    assert.ok(pending && pending.payload === Math.fround(0.01) && pending.count === 0 && pending.time === 99);
    actions.continue(parent, 4);
    assert.ok(updates === 6 && reserved === 2 && kernel.size === 2 && host.findEvent(parent, 604) !== pending);
    assert.equal(actions.pool, host.pool);
    actions.begin(0, 0xffffffff);
    assert.ok(updates === 6 && reserved === 2);
    assert.ok(throws(() => actions.begin(parent, 6)));
    assert.ok(throws(() => new RawPriestRitualActions(pool, frames, {})));
    assert.ok(throws(() => makePriestRitualActionHooks(new SidPool(24000, { server: true }), frame, host, {})));
    kernel.clear();
});
