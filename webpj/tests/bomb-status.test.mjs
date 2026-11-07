// 117/118 상태 주문 raw-bomb-status.js(RawBombStatus)를 독립 원본 관측 bomb-status-x86.tsv 612행으로 대조한다.
// 행 모드: I(은신 적용 502510)·P(마비 적용 502ff8) 각 267행, i(은신 자격)·p(마비 자격) 각 35행, H(송신 observer 의 선언 길이 확정) 8행.
// 입력은 원본과 같은 두 raw 슬롯(부모 50·대상 60)과 TYPE 전역/동맹 표이며 큰 하위 호출(R 표시 갱신·U/P 재배치·F 울타리·L 플레어)은 원본 기록과 같은 문자열로 남긴다.
// 선택 효과 뒤 현재 TYPE/전역/상태를 바꾸는 명시 입력(mutation 1~3)을 포함해 두 슬롯 전체와 32바이트 패킷까지 비교한다. 기대값은 계산하지 않는다.
// (2026-11-08 WB04-E53: RawBombStatus 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { RawBombStatus } from '../www/core/raw-bomb-status.js';
import { finalizeBombTimerPacket } from '../www/core/raw-bomb-timer-packet.js';
import { SidPool } from '../www/core/sid-pool.js';
import { REPO_ROOT } from './helpers.mjs';
import { put, readRows } from './raw-support.mjs';

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E53)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// 188개 타입 표이다. (2026-11-08 WB04-E53)
const makeTypes = () => Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1 }));

test('bomb-status: 대상 자격·운반체/사제 면제·상태 비트 쓰기·현재 가상 효과와 송신 observer 를 원본 612행과 대조한다', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'bomb-status-x86.tsv'));
    const modes = new Map();
    // 원본 612행을 재생한다.
    for (const row of rows) {
        const words = row[0].split(',');
        const mode = words[0];
        const a = words.slice(1).map((word) => Number(BigInt.asUintN(32, BigInt(word))));
        const pool = new SidPool(24000, { server: false });
        const types = makeTypes();
        const alliances = new Uint32Array(81);
        // 행 1 열 2(부모 owner 1, 대상 owner 2)의 동맹 입력이다.
        alliances[11] = a[5];
        const raw = (sid) => pool.physical(sid).subarray(0, 50);
        // 두 owner 와 가상 효과 표 입력이다.
        for (const sid of [50, 60]) {
            const bytes = raw(sid);
            bytes.fill(0);
            put(bytes, 0, 0x12200000, 4);
            bytes[34] = sid === 50 ? 1 : 2;
        }
        put(raw(50), 12, a[4] ? 60 : 0, 2);
        raw(60)[10] = a[0] & 0xff;
        raw(60)[11] = a[2] & 0xff;
        raw(60)[40] = a[3] & 0xff;
        // 현재 TYPE 변경으로 다시 조회하는 genus 도 원문과 같다.
        for (const type of types) type.flags2 = a[1];
        types[136].flags2 = (types[136].flags2 ^ 0x200000) >>> 0;
        const state = { altar: 164, outpost: 126, priest: 158, fence: 136, flare: 106, alliances };
        let effects = '';
        let result = '-';
        // 순서 있는 큰 경계만 기록한다.
        const event = (value) => {
            if (effects) effects += ';';
            effects += value;
        };
        // 선택 효과 뒤 현재 TYPE/전역/상태를 바꾸는 명시 입력이다.
        const mutation = () => {
            if (a[6] === 1) raw(60)[10] = 136;
            if (a[6] === 2) state.flare = 107;
            if (a[6] === 3) raw(60)[11] = 7;
        };
        const hooks = {
            refresh: (sid) => {
                event(`R:${sid}`);
                mutation();
            },
            unpop: (sid, flags) => {
                event(`U:${sid}:${flags}`);
                mutation();
            },
            repop: (sid, flags) => {
                event(`P:${sid}:${flags}`);
                mutation();
            },
            fence: (sid) => {
                event(`F:${sid}`);
                if (a[6] === 1) state.flare = 107;
            },
            flare: (sid, n, eventId, type, direction) => event(`L:${sid}:${n}:${eventId}:${type}:${direction}`),
        };
        const packet = new Uint8Array(32).fill(0xa5);
        if (mode === 'H') {
            put(packet, 16, a[0], 2);
            finalizeBombTimerPacket(packet, a[1] !== 0);
        } else {
            const table = mode === 'I' || mode === 'i' ? 0x502510 : 0x502ff8;
            put(raw(50), 0, table, 4);
            const logic = new RawBombStatus(pool, types, state, hooks);
            if (mode === 'i' || mode === 'p') result = logic.mayTarget(50, 60) ? '1' : '0';
            else logic.apply(50, 60);
            // 원문 실행기의 가상 호출 표만 비교용으로 정규화한다.
            put(raw(50), 0, 0x12200000, 4);
        }
        const expected = mode === 'i' || mode === 'p' ? (Number(row[1]) ? '1' : '0') : row[1];
        if (!effects) effects = '-';
        assert.equal(result, expected, `상태 주문 행 ${row[0]} 결과`);
        assert.equal(effects, row[2], `상태 주문 행 ${row[0]} 효과`);
        assert.equal(hex(raw(50)) + hex(raw(60)), row[3], `상태 주문 행 ${row[0]} 슬롯`);
        assert.equal(hex(packet), row[4], `상태 주문 행 ${row[0]} 패킷`);
        modes.set(mode, (modes.get(mode) ?? 0) + 1);
    }
    assert.deepEqual(Object.fromEntries([...modes].sort()), { H: 8, I: 267, P: 267, i: 35, p: 35 });
});

test('bomb-status: 구성 오류와 다른 가상 표는 거부하고 선택하지 않은 효과 훅은 요구하지 않는다', () => {
    const pool = new SidPool(24000, { server: false });
    const types = makeTypes();
    const state = { altar: 164, outpost: 126, priest: 158, fence: 136, flare: 106, alliances: new Uint32Array(81) };
    assert.throws(() => new RawBombStatus(pool, types.slice(1), state, {}), TypeError);
    assert.throws(() => new RawBombStatus({ edition: 'original1037' }, types, state, {}), TypeError);
    assert.throws(() => new RawBombStatus(pool, types, null, {}), TypeError);
    const logic = new RawBombStatus(pool, types, state, {});
    for (const sid of [50, 60]) {
        pool.physical(sid).fill(0);
        put(pool.physical(sid), 0, 0x12200000, 4);
    }
    // 알 수 없는 표는 자격/적용 모두 거부한다.
    assert.throws(() => logic.mayTarget(50, 60), TypeError);
    assert.throws(() => logic.apply(50, 60), TypeError);
    // Wind 의 은신 적용 표(502e68)는 적용만 허용하고 자격 판정에는 쓰지 않는다.
    put(pool.physical(50), 0, 0x502e68, 4);
    assert.throws(() => logic.mayTarget(50, 60), TypeError);
    // 운반체(부모 +12 WORD)와 같은 대상은 효과 없이 끝나므로 훅이 없어도 된다.
    put(pool.physical(50), 12, 60, 2);
    logic.apply(50, 60);
    assert.equal(pool.slot(60)[40], 0);
    // 그렇지 않으면 상태 비트를 쓴 뒤 필요한 훅을 요구한다(비트는 이미 쓰여 남는다).
    put(pool.physical(50), 12, 0, 2);
    assert.throws(() => logic.apply(50, 60), TypeError);
    assert.equal(pool.slot(60)[40], 0x10);
    // 동맹 표 크기가 다르면 사제 면제 판정에서 거부한다.
    put(pool.physical(50), 0, 0x502ff8, 4);
    pool.physical(60)[10] = 158;
    state.alliances = new Uint32Array(80);
    assert.throws(() => logic.apply(50, 60), RangeError);
});
