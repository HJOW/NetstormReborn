// 상태 주문 타이머 raw-bomb-effect-timer.js(RawBombEffectTimer·BombEffectTimerProcess)와 가변 패킷 raw-bomb-timer-packet.js 를 독립 원본 관측으로 대조한다.
//  - bomb-effect-timer-x86.tsv 4416행: 은닉/마비/중복/비활성/만료·NaN 기한 입력과 unpop·repop·refresh 콜백이 목록/기한/flags/대상 상태를 바꾸는 변형(mutation 1~5)에서의 효과 순서·최종 기한 비트·목록·세 슬롯 전체.
//  - bomb-timer-constructor-x86.tsv 12행: 기본 cdecl 생성(Init 없음)의 필드(빈 목록·기한 0·미작성 flags)와 부착 전 form/부모 0, 풀 자유 슬롯 불변.
//  - bomb-timer-packet-x86.tsv 602행: C 생성(105개 입력 중 앞쪽 복사) · W writer · R 누적 읽기 · A 신규 수신 · P 부모 선택(자세한 모드는 시험 본문 주석).
// 입력은 원본과 같은 raw 슬롯이며 콜백이 바꾸는 현재 슬롯/목록만 수행하고 기대값은 계산하지 않는다. (2026-11-08 WB04-E52: RawBombEffectTimer·RawBombTimerPacket 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { BombEffectTimerFields, BombEffectTimerProcess, RawBombEffectTimer } from '../www/core/raw-bomb-effect-timer.js';
import { bombTimerParent, finalizeBombTimerPacket, readBombTimerPacket, receiveBombTimerPacket, writeBombTimerPacket } from '../www/core/raw-bomb-timer-packet.js';
import { SidPool } from '../www/core/sid-pool.js';
import { REPO_ROOT } from './helpers.mjs';
import { floatToBits, get, put, readRows } from './raw-support.mjs';

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E52)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// fixture 파일 경로이다. (2026-11-08 WB04-E52)
const fixture = (name) => path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name);

// 188개 타입 표이다. (2026-11-08 WB04-E52)
const makeTypes = () => Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1 }));

test('bomb-effect-timer: 목록 갱신·만료 해제·효과 뒤 현재 기한/목록/상태 재조회를 원본 4416행과 대조한다', () => {
    const rows = readRows(fixture('bomb-effect-timer-x86.tsv'));
    let count = 0;
    // 원본 4416행을 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 4);
        const a = row[0].split(',').map(Number);
        assert.equal(a.length, 8);
        const pool = new SidPool(24000, { server: false });
        const types = makeTypes();
        const now = 100.25;
        types[133].flags2 = a[3] >>> 0;
        types[136].flags2 = (a[3] >>> 0 ^ 0x200000) >>> 0;
        const writable = (sid) => pool.physical(sid);
        // 원본과 같은 세 슬롯의 명시 초기 입력이다.
        for (const sid of [1, 60, 127]) {
            const raw = writable(sid);
            raw.fill(0);
            put(raw, 0, 0x12200000, 4);
            raw[10] = a[4] & 0xff;
            raw[11] = a[1] & 0xff;
            raw[40] = a[2] & 0xff;
        }
        let items = [];
        if (a[7] === 1) items = [60];
        else if (a[7] === 2) items = [60, 127, 60];
        else if (a[7] === 3) items = [60, 1, 127];
        let fields = null;
        let effects = '';
        let didU = false;
        let didR = false;
        let didG = false;
        // 외부 호출의 순서/현재 SID/flags 만 기록한다.
        const event = (tag, sid, value) => {
            if (effects) effects += ';';
            effects += `${tag}:${sid}`;
            if (value !== undefined) effects += `:${value}`;
        };
        const hooks = {
            unpop: (target, flags) => {
                event('U', target, flags);
                if (!didU) {
                    didU = true;
                    if (a[6] === 3) fields.members[0] = 127;
                }
            },
            repop: (target, flags) => {
                event('R', target, flags);
                if (!didR) {
                    didR = true;
                    if (a[6] === 1) fields.endTime = 1000.25;
                    if (a[6] === 2) {
                        fields.flags = 3;
                        writable(target)[40] = 48;
                    }
                }
            },
            refresh: (target) => {
                event('G', target);
                if (!didG) {
                    didG = true;
                    if (a[6] === 4) writable(target)[10] = 136;
                    if (a[6] === 5) writable(target)[11] |= 2;
                }
            },
            fence: (target) => event('F', target),
        };
        const logic = new RawBombEffectTimer(pool, types, () => now, hooks);
        fields = logic.construct(a[5] === 2 ? 0 : a[5] | 0, items, a[0] >>> 0);
        if (a[5] === 2) fields.endBits = 0x7ff8000000000000n;
        logic.frame(fields, () => {
            if (effects) effects += ';';
            effects += 'K:0';
        });
        if (!effects) effects = '-';
        let packed = `${fields.endBits},${fields.flags}`;
        for (const member of fields.members) packed += `,${member}`;
        const slots = [1, 60, 127].map((sid) => hex(pool.slot(sid))).join('');
        assert.equal(effects, row[1], `상태 효과 타이머 행 ${count}:${row[0]} 효과`);
        assert.equal(packed, row[2], `상태 효과 타이머 행 ${count} 기한/목록`);
        assert.equal(slots, row[3], `상태 효과 타이머 행 ${count} 슬롯`);
        count += 1;
    }
    assert.equal(count, 4416);
});

test('bomb-timer-constructor: 기본 생성은 Init 없이 빈 목록·기한 0·미작성 flags 를 남기고 부착 전 form/부모는 0 이다', () => {
    const rows = readRows(fixture('bomb-timer-constructor-x86.tsv'));
    let count = 0;
    // 원본 12행을 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 5);
        const native = Uint8Array.from(Buffer.from(row[3], 'hex'));
        assert.equal(native.length, 40);
        const inputFlags = (Number(row[0]) * 0x01010101) >>> 0;
        const nowBits = BigInt(row[2]);
        const view = new DataView(new ArrayBuffer(8));
        view.setBigUint64(0, nowBits, true);
        let now = view.getFloat64(0, true);
        const pool = new SidPool(24000, { server: false });
        const free = pool.freeCount;
        const process = new BombEffectTimerProcess(pool, makeTypes(), () => now, {}, inputFlags);
        const fields = process.fields;
        assert.equal(fields.members.length, 0);
        assert.equal(fields.members.length, get(native, 0x14, 4));
        assert.equal(fields.capacity, get(native, 0x10, 4));
        assert.equal(fields.flags, get(native, 0x20, 4));
        assert.equal(process.form, get(native, 4, 4));
        assert.equal(process.parent, get(native, 8, 4));
        assert.equal(pool.freeCount, free);
        assert.equal(fields.endBits, (BigInt(get(native, 0x1c, 4)) << 32n) | BigInt(get(native, 0x18, 4)));
        const members = Uint8Array.from(Buffer.from(row[4], 'hex'));
        assert.equal(members.length, 400);
        assert.ok(members.every((byte) => byte === 0xd7));
        // 생성 후 현재 시각을 바꿔도 기한을 뒤늦게 초기화하지 않는다.
        now = 99999.5;
        assert.equal(fields.endTime, 0);
        assert.equal(fields.flags, inputFlags);
        count += 1;
    }
    assert.equal(count, 12);
});

// 패킷 시험의 고정 입력 목록이다(중복·0·상위 비트 입력을 반복). (2026-11-08 WB04-E52)
function members(count) {
    const values = [60, 127, 60, 0, 65535, 0x12345678];
    return Array.from({ length: count }, (_, index) => values[index % values.length]);
}

test('bomb-timer-packet: 생성·누적 읽기·writer·신규 수신·부모 선택을 원본 602행과 대조한다', () => {
    const rows = readRows(fixture('bomb-timer-packet-x86.tsv'));
    const kinds = new Map();
    // 원본 602행을 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 5);
        const parts = row[0].split(',');
        const mode = parts[0];
        const a = parts.slice(1).map(Number);
        const pool = new SidPool(24000, { server: false });
        const types = makeTypes();
        types[131].flags2 = 0x40000000;
        let fields = new BombEffectTimerFields();
        fields.endBits = 0x7ff8000000001234n;
        fields.flags = 0x80000003;
        const packet = new Uint8Array(256).fill(0xa5);
        put(packet, 4, 50, 2);
        put(packet, 6, 60, 2);
        put(packet, 16, 3, 2);
        put(packet, 18, 2, 4);
        const timeBits = floatToDoubleBits(123.25);
        put(packet, 8, Number(timeBits & 0xffffffffn), 4);
        put(packet, 12, Number(timeBits >> 32n), 4);
        const input = members(105);
        let effects = '';
        let result = '-';
        // 원문 입력의 WORD 패킷을 별도로 준비한다.
        for (let index = 0; index < input.length; index += 1) put(packet, 22 + 2 * index, input[index], 2);
        const event = (value) => {
            if (effects) effects += ';';
            effects += value;
        };
        if (mode === 'C') {
            const now = [0, 100.25, 1e12][a[2]];
            fields = new RawBombEffectTimer(pool, types, () => now, {}).construct(a[1] | 0, input.slice(0, a[0]), a[3] >>> 0);
        } else if (mode === 'W') {
            fields.members = members(a[0]);
            result = String(writeBombTimerPacket(fields, 50, 60, packet, a[1], a[2]));
        } else if (mode === 'R') {
            fields.capacity = a[0] >>> 0;
            put(packet, 16, a[2], 2);
            // 기존 목록을 제품 읽기 전에 입력한다.
            for (let index = 0; index < a[1]; index += 1) fields.members.push(900 + index);
            if (a[4]) {
                put(packet, 8, 0x4321, 4);
                put(packet, 12, 0x7ff80000, 4);
            }
            // 반복 수신으로 포화 뒤의 동작도 비교한다.
            for (let index = 0; index <= a[3]; index += 1) readBombTimerPacket(fields, packet);
        } else if (mode === 'A') {
            const raw = pool.physical(50);
            raw[11] = a[0];
            const state = { boss: a[1], processType: 57 };
            receiveBombTimerPacket(pool, state, packet, {
                assertion: (line) => {
                    event(`D:${line}`);
                    if (line === 0xcc && a[2] === 1) raw[11] = 1;
                },
                create: (type, x, y) => {
                    event(`T:${type}`);
                    event(`C:${x}:${y}`);
                    fields.capacity = 100;
                    fields.members = [];
                    if (a[2] === 2) state.processType = 58;
                    return 1;
                },
                read: (token, current) => {
                    assert.equal(token, 1);
                    if (a[2] === 3) {
                        put(packet, 4, 51, 2);
                        put(packet, 6, 70, 2);
                    }
                    readBombTimerPacket(fields, current);
                },
                attach: (token, type, parent, form, flags) => {
                    assert.equal(token, 1);
                    event(`A:${type}:${parent}:${form}:${flags}`);
                },
            });
        } else if (mode === 'P') {
            const current = [[], [60], [60, 127, 1], [1, 127, 60]][a[0]];
            // 후보의 상태값도 추가 필터 없이 입력한다.
            for (const [sid, kind] of [[60, 130], [127, 131], [1, 132]]) {
                const raw = pool.physical(sid);
                raw[10] = kind;
                raw[11] = a[2];
            }
            if (a[1] === 1) pool.physical(60)[10] = 131;
            if (a[1] === 2) pool.physical(127)[10] = 130;
            result = String(bombTimerParent(pool, types, current));
        } else {
            assert.fail(`알 수 없는 모드 ${mode}`);
        }
        let packed = `${fields.capacity},${fields.endBits},${fields.flags}`;
        // 실제 최종 목록 전체를 비교하며 중복을 줄이지 않는다.
        for (const member of fields.members) packed += `,${member}`;
        if (!effects) effects = '-';
        assert.equal(result, row[1], `타이머 패킷 행 ${row[0]} 결과`);
        if (mode !== 'C') assert.equal(effects, row[2], `타이머 패킷 행 ${row[0]} 효과`);
        else assert.equal(row[2], 'N:400');
        assert.equal(packed, row[3], `타이머 패킷 행 ${row[0]} 필드`);
        assert.equal(hex(packet), row[4], `타이머 패킷 행 ${row[0]} 패킷`);
        kinds.set(mode, (kinds.get(mode) ?? 0) + 1);
    }
    assert.equal([...kinds.values()].reduce((sum, value) => sum + value, 0), 602);
});

// 수 하나를 double 의 64비트 표현(BigInt)으로 바꾼다. (2026-11-08 WB04-E52)
function floatToDoubleBits(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return view.getBigUint64(0, true);
}

test('bomb-timer-packet: 짧은 버퍼/손상된 입력은 거부하고 송신 observer 는 쓰기일 때만 선언 길이를 확정한다', () => {
    const fields = new BombEffectTimerFields();
    fields.members = [60];
    const shortPacket = new Uint8Array(23);
    assert.throws(() => writeBombTimerPacket(fields, 50, 60, shortPacket, 22), RangeError);
    put(shortPacket, 16, 1, 2);
    assert.throws(() => readBombTimerPacket(fields, shortPacket), RangeError);
    const pool = new SidPool(24000, { server: false });
    assert.throws(() => bombTimerParent(pool, makeTypes(), [0x80000000]), RangeError);
    assert.throws(() => bombTimerParent({ edition: 'original1037' }, makeTypes(), []), TypeError);
    // 송신이 아니면 아무것도 하지 않고, 송신이면 (수 + 11) × 2 로 선언 길이를 쓴다.
    const packet = new Uint8Array(32).fill(0xa5);
    put(packet, 16, 3, 2);
    finalizeBombTimerPacket(packet, false);
    assert.equal(get(packet, 2, 2), 0xa5a5);
    finalizeBombTimerPacket(packet, true);
    assert.equal(get(packet, 2, 2), 28);
    put(packet, 16, 65535, 2);
    assert.throws(() => finalizeBombTimerPacket(packet, true), Error);
    // 수신 훅 누락은 도달한 지점에서만 거부한다(점유 Form 이 아니면 e6 진단 뒤 종료).
    put(packet, 4, 50, 2);
    assert.throws(() => receiveBombTimerPacket(pool, { boss: 0, processType: 57 }, packet, {}), TypeError);
    // 구성 오류.
    assert.throws(() => new RawBombEffectTimer(pool, makeTypes().slice(1), () => 0, {}), TypeError);
    assert.throws(() => new RawBombEffectTimer(pool, makeTypes(), 0, {}), TypeError);
});

test('bomb-effect-timer: 호스트가 없는 프로세스는 만료 때 자기 삭제를 못해 거부하고 만료 전에는 아무것도 하지 않으며 통지는 빈 함수이다', () => {
    const pool = new SidPool(24000, { server: false });
    const now = 100;
    const expired = BombEffectTimerProcess.withTimer(pool, makeTypes(), () => now, -1, [], 0, {});
    assert.throws(() => expired.runFrame(), Error);
    const waiting = BombEffectTimerProcess.withTimer(pool, makeTypes(), () => now, 5, [], 0, {});
    waiting.runFrame();
    assert.equal(waiting.notify(1, 2, 3), undefined);
    assert.equal(waiting.fields.endTime, 105);
    // 패킷 기록은 부착 전 form/부모 0 을 그대로 쓰고 실제 길이를 돌려준다.
    const packet = new Uint8Array(32).fill(0xa5);
    assert.equal(waiting.writePacket(packet, 4), 22);
    assert.equal(get(packet, 4, 2), 0);
    assert.equal(get(packet, 6, 2), 0);
});
