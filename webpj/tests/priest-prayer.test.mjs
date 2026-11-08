// 사제 기도 raw-priest-prayer.js(RawPriestPrayer)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priest-prayer-x86.tsv(10.78 전용 단일 판본) 1126행 — 기도 명령(C)·수신(R)·605 진행(T)의 입력(상태·첫 605 존재·방향·권한·예약·이동 Form·마비·내용물 종류·개수·payload·명령·희망 종류·변이 단계)
//    → 호출 순서·슬롯 60 전체 raw·반환 비트·최종 권한. 큰 하위(요청 전송·이동 Form·공유 예약·의식·내용물·공황·부모)는 명시 경계로 호출 기록만 남긴다.
//  - 계약: 선택되지 않은 하위 효과는 요구하지 않고 도달한 누락만 진단, 잘못된 풀·표 크기 거부, 짧은 패킷 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E66: 사제 기도 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawPriestPrayer } from '../www/core/raw-priest-prayer.js';

// 원본 관측 행이다(단일 판본 파일). 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E66)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priest-prayer-x86.tsv'));

// 원본 관찰 부모 SID, 독립 행 수, 기도 사제 가상 표(합성 값; 호스트 함수 주소가 아님)이다. (2026-11-08 WB04-E66)
const ACTOR = 60;
const ROWS = 1126;
const VTABLE = 0x12200000;

// 예외가 나는지 본다. (2026-11-08 WB04-E66)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E66)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 188칸 { prayTime, defaultFrame } 표이다. (2026-11-08 WB04-E66)
const prayerTypes = () => Array.from({ length: 188 }, () => ({ prayTime: 0, defaultFrame: 0 }));

test('PriestPrayer: 원본 10.78 1126행의 호출 순서·슬롯 60 raw·반환 비트·최종 권한이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 명령/수신/605 와 모든 명시 관문·변형을 소비한다.
    for (const row of rows) {
        assert.equal(row.length, 5);
        const parts = row[0].split(',');
        assert.equal(parts.length, 15);
        const mode = parts[0];
        // DWORD 상위 비트와 음수 방향/기도 횟수를 보존하는 수 입력이다.
        const a = parts.slice(1).map(Number);
        const pool = new SidPool(24000, { server: true });
        const raw = pool.physical(ACTOR);
        raw.fill(0);
        put(raw, 0, VTABLE);
        raw[10] = 158;
        raw[11] = a[0] & 0xff;
        put(raw, 36, 7);
        put(raw, 42, a[12]);
        put(raw, 46, 0xaabbccdd);
        const types = prayerTypes();
        types[158].defaultFrame = 7;
        types[159].defaultFrame = 9;
        types[113].prayTime = types[168].prayTime = a[9] | 0;
        let boss = a[3] !== 0;
        const effects = [];
        const parameter = ((a[12] << 16) | ACTOR) >>> 0;
        // 효과 순서만 기록하며 실제 판정은 제품이 수행한다.
        const hooks = {
            hasEvent: (sid, code) => { effects.push(`E:${code}`); return a[1] !== 0; },
            direction: (sid) => { effects.push(`D:${sid}`); return a[2] | 0; },
            movementType: (kind) => { effects.push(`M:${kind}`); return 54; },
            movement: (sid, type, ordinal, remove) => {
                effects.push(`L:${sid}:${type}:${ordinal}:${remove ? 1 : 0}`);
                if (a[13] === 2) put(raw, 42, 113);
                return a[5] ? 77 : 0;
            },
            request: (target, packet) => {
                effects.push(`P:${target}:${hex(packet)}`);
                if (a[13] === 3) { boss = false; raw[10] = 159; }
            },
            broadcast: (packet) => {
                effects.push(`B:${hex(packet)}`);
                if (a[13] === 3) { boss = false; raw[10] = 159; }
            },
            reserveShared: () => { effects.push('A:40'); return a[4] !== 0; },
            addShared: (sid, code, payload) => { effects.push(`R:${code}:${sid}:${floatToBits(payload)}`); },
            ritual: (sid, stage) => { effects.push(`S:${sid}:${stage}`); },
            immobile: (sid) => {
                effects.push(`I:${sid}`);
                if (a[13] === 4) { boss = false; put(raw, 42, 113); }
                return a[6] !== 0;
            },
            containedType: (sid, mask, kind, ordinal) => {
                effects.push(`H:${sid}:${mask}:${kind}:${ordinal}`);
                if (a[13] === 5) boss = false;
                return a[7] ? 168 : 0;
            },
            discard: (sid) => {
                effects.push(`Q:${sid}`);
                if (a[13] === 6) { put(raw, 42, 113); raw[10] = 159; }
            },
            create: (sid, kind, argument, flags, attachFlags, quality) => { effects.push(`C:${sid}:${kind}:${argument}:${flags}:${attachFlags}:${quality}`); },
            setFrame: (sid, frame, flags) => {
                effects.push(`F:${sid}:${frame}:${flags}`);
                put(raw, 36, frame >>> 0);
            },
            panic: (sid) => { effects.push(`V:${sid}`); },
            parent: (sid, code, value) => { effects.push(`X:${sid}:${code}:${value}`); return 0x12345678; }
        };
        const logic = new RawPriestPrayer(pool, types, () => boss, hooks);
        let result = 0;
        if (mode === 'C') result = logic.command(ACTOR, a[11] >>> 0, parameter);
        else if (mode === 'R') logic.receive(Uint8Array.of(1, 0x6d, 8, 0, ACTOR, 0, a[12] & 0xff, (a[12] >>> 8) & 0xff));
        else result = floatToBits(logic.tick(ACTOR, a[8] >>> 0, bitsToFloat(a[10] >>> 0)));
        const text = effects.length === 0 ? '-' : effects.join(';');
        assert.equal(text, row[1], `행 ${count}: ${row[0]}`);
        assert.equal(hex(raw), row[2], `행 ${count}: ${row[0]}`);
        assert.equal(String(result), row[3], `행 ${count}: ${row[0]}`);
        assert.equal(String(boss ? 1 : 0), row[4], `행 ${count}: ${row[0]}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('PriestPrayer: 선택되지 않은 하위 효과는 요구하지 않고 도달한 누락만 그 시점에 거부하며 짧은 패킷은 쓰기 전에 거부한다', () => {
    const pool = new SidPool(24000, { server: true });
    const types = prayerTypes();
    const raw = pool.physical(ACTOR);
    raw.fill(0);
    put(raw, 0, VTABLE);
    raw[10] = 158;
    const effects = [];
    // 기도가 아닌 명령은 부모 처리기만 요구한다.
    const prayer = new RawPriestPrayer(pool, types, () => true, { parent: (sid, code, value) => { effects.push(`X:${code}:${value}`); return 7; } });
    assert.equal(prayer.command(ACTOR, 0x12345, 9), 7);
    assert.deepEqual(effects, ['X:74565:9']);
    // 기도 명령은 첫 605 조회가 없으면 그 시점에 거부한다. 상위 6비트는 명령 비교에서 제외한다.
    assert.ok(throws(() => prayer.command(ACTOR, (0x030000de | 0xfc000000) >>> 0, 0)));
    // 공황 명령은 부모에 위임하기 전에 공황을 실행해야 한다.
    assert.ok(throws(() => prayer.command(ACTOR, 0x03000103, 0)));
    // 짧은 패킷은 희망 종류를 쓰기 전에 거부한다.
    put(raw, 42, 5);
    assert.ok(throws(() => prayer.receive(Uint8Array.of(1, 0x6d, 8, 0, ACTOR, 0, 9))));
    assert.equal(raw[42], 5);
    // 풀·표·권한 getter 연결 오류는 구성 때 거부한다.
    assert.ok(throws(() => new RawPriestPrayer(pool, types.slice(1), () => true, {})));
    assert.ok(throws(() => new RawPriestPrayer(pool, types, true, {})));
    // 손상된 희망 종류는 표 밖 접근을 진단한다.
    const hooks = { movementType: () => 54, movement: () => 0, immobile: () => false };
    put(raw, 42, 0xffff);
    assert.ok(throws(() => new RawPriestPrayer(pool, types, () => true, hooks).tick(ACTOR, 0, 1)));
    assert.equal(raw[46], 0);
});
