// 운반물 입력 raw-carrier-input.js(RawCarrierInput)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrier-input-x86.tsv 454행 — 입력 33 DWORD(연산 0 Evaluate/1 Highlight/2 Commit/3 Acknowledge·genus·flags·extra·소유자·로컬·즉시 이동 불가·내용물·후보·Graph·연결·표면·간헐천·귀환·제단·허용·조기 희생·다중·표시·강조 힘·부모 번호·초기 커서·스탬프·프레임·인덱스·TYPE·점 x/y·콜백 변이)
//    → 반환 코드, 효과 순서(I 진단/H 이동불가/C 내용물/Q 후보/D 위치/G Graph/L 연결/S 표면/Y 간헐천/V 허용/R 귀환/A 제단/F 허용/E 조기/M 다중/U 선택/B 강조/X 커서/T 요청/P 포획/N 내려놓기/W·J·K 희생/Z 표시/O 갱신/? 난수/a 이름 소리/s 소리),
//    caller 의 최종 점 8바이트, 전역 여섯 값(커서·제단·내부점·표시 카운터·로컬), TYPE 두 플래그와 프레임 장부, 여섯 슬롯 전체 raw. 53/64비트 정밀도 열이 같다.
//  - 계약: 미연결 하위·판본/TYPE 표·음향 장부·SID 범위 오류를 해당 분기를 쓰는 시점에 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E97: 운반물 입력 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { CarrierInputState, RawCarrierInput } from '../www/core/raw-carrier-input.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E97)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'carrier-input-x86.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E97)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const hex = (bytes) => Buffer.from(bytes).toString('hex');
const bits = (value) => floatToBits(value);

// 독립 입력과 현재 저장소만 구성하고 분기 기대값은 fixture 에서 읽는 장면이다. (2026-11-08 WB04-E97)
function makeScene(row) {
    const scene = {
        pool: new SidPool(24000, { server: true }),
        types: Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0 })),
        audio: Array.from({ length: 188 }, () => ({ moveSound: 0, dropSound: 0 })),
        stamps: new Uint32Array(188),
        state: new CarrierInputState(),
        c: row[0].split(',').map(Number),
        frame: { value: 0 },
        local: 0,
        trace: []
    };
    const c = scene.c;
    assert.equal(c.length, 33);
    scene.frame.value = c[27];
    scene.local = c[5];
    const state = scene.state;
    state.cursor = c[25];
    state.dais = 77;
    state.insidePoint = [11, 12];
    state.markers = c[22] !== 0;
    state.markerCounter = 1;
    state.markerTypes = [99, 100];
    scene.types[c[29]].flags1 = c[2];
    scene.types[c[29]].flags2 = c[1];
    scene.audio[c[29]] = { moveSound: 123, dropSound: 124 };
    scene.stamps[c[29]] = c[26];
    // 독립 원문과 같은 여섯 물리 슬롯을 바이트별로 준비한다.
    for (const id of [50, 60, 61, 62, 63, 64]) {
        const raw = scene.pool.physical(id);
        raw.fill(0xa5);
        put(raw, 0, 0x12200000);
        raw[10] = c[29];
        raw[34] = c[4];
        raw[40] = c[3];
        put(raw, 14, bits(20));
        put(raw, 18, bits(21));
    }
    put(scene.pool.physical(60), 18, 134, 2);
    // 원문 관측기의 명시 외부 변경만 재생한다.
    const mutate = (where) => {
        const m = c[32];
        if (m === 1 && where === 'assert') scene.types[c[29]].flags2 = 0x20000;
        if (m === 2 && where === 'immobile') scene.pool.physical(50)[40] = 0x20;
        if (m === 3 && where === 'candidate') scene.types[c[29]].flags2 = 0x20000;
        if (m === 4 && where === 'candidate') scene.types[c[29]].flags1 = 0x1000000;
        if (m === 5 && where === 'ack') scene.pool.physical(50)[34] = 2;
        if (m === 6 && where === 'allowed') scene.local = 2;
        if (m === 7 && where === 'select') state.cursor = 18;
        if (m === 8 && where === 'early') scene.pool.physical(50)[34] = 2;
    };
    // 원문과 같은 전체 인자를 순서대로 기록한다.
    const event = (name, values = []) => scene.trace.push(values.reduce((text, value) => `${text}:${value >>> 0}`, name));
    scene.hooks = {
        assertion: (line) => { event('I', [line]); mutate('assert'); },
        immobile: (sid) => { event('H', [sid]); mutate('immobile'); return c[6] !== 0; },
        content: (sid, mask) => { event('C', [sid, mask, 0, 0]); return mask === 0x20 ? c[7] : 0; },
        candidate: (sid, p) => { event('Q', [sid, bits(p.x), bits(p.y)]); mutate('candidate'); return { sid: c[8], nugget: c[9] !== 0 }; },
        dropPoint: (sid, type) => { event('D', [sid, type]); return [30, 31]; },
        graph: (sid) => { event('G', [sid]); return sid === 50 || c[11] ? c[10] : c[10] + 1; },
        connected: (a, b) => { event('L', [bits(a.x), bits(a.y), bits(b.x), bits(b.y)]); return c[12] !== 0; },
        surface: (p) => { event('S', [bits(p.x), bits(p.y)]); return c[13]; },
        geyser: (p) => { event('Y', [bits(p.x), bits(p.y), 122]); return c[14]; },
        validGeyser: (sid, target) => { event('V', [sid, target]); return c[15] !== 0; },
        returnPlace: (p) => { event('R', [bits(p.x), bits(p.y), 0]); return c[16]; },
        daisAt: (p) => { event('A', [bits(p.x), bits(p.y), 0x400000]); return c[17]; },
        altarAllowed: (dais, sid) => { event('F', [dais, sid]); mutate('allowed'); return c[18] !== 0; },
        earlySacrifice: () => { event('E'); mutate('early'); return c[19] !== 0; },
        localOwner: () => scene.local,
        multiple: () => { event('M'); return c[21]; },
        checking: () => c[20] !== 0,
        select: (a, b, d) => { event('U', [a, b, d]); mutate('select'); },
        highlight: (mode) => event('B', [mode]),
        selectCursor: (mode) => event('X', [mode]),
        request: (sid, p, sacrifice) => event('T', [sid, bits(p.x), bits(p.y), sacrifice]),
        pickup: (sid) => event('P', [sid]),
        dropCargo: (sid, v) => event('N', [sid, v]),
        earlyLocal: () => event('W'),
        earlyRemote: (owner) => { event('J', [owner]); scene.trace[scene.trace.length - 1] += ':EarlySacrifice'; },
        showSacrifice: (dais) => event('K', [dais]),
        marker: (p, type) => event('Z', [bits(p.x), bits(p.y), type]),
        refreshCursor: () => event('O'),
        random: (lo, hi) => { event('?', [lo, hi]); return 1000; },
        // 보호 PE 의 실제 이름 표는 연속 번호가 아니다.
        namedSound: (priest, index) => {
            const priests = [1, 3];
            const golems = [1, 2, 4, 5];
            const table = priest ? priests : golems;
            // 골렘 인덱스 4 는 표를 조회하지 않고 인접 이름을 쓴다.
            if (!(!priest && index === 4)) assert.ok(index < table.length);
            const name = !priest && index === 4 ? '@1' : `${priest ? 'priestMove' : 'golemMove'}${table[index]}.wav`;
            event(`a:${name}`, [0, 0, 0, 1, 0]);
            mutate('ack');
        },
        sound: (handle) => { event('s', [handle, 0, 0, 0, 1, 0]); mutate('ack'); }
    };
    // 실제 부모를 실행한 뒤 원문 전체 상태를 직렬화한다.
    scene.run = () => {
        const body = new RawCarrierInput(scene.pool, scene.types, scene.audio, scene.stamps, scene.frame, state, scene.hooks);
        const point = { x: bitsToFloat(c[30]), y: bitsToFloat(c[31]) };
        let result = '-';
        if (c[0] === 0) result = String(body.evaluate(50, point));
        else if (c[0] === 1) body.highlight(c[24], point, c[23]);
        else if (c[0] === 2) body.commit(c[24], point);
        else body.acknowledge(50, c[28] | 0);
        const p = new Uint8Array(8);
        put(p, 0, bits(point.x));
        put(p, 4, bits(point.y));
        const globals = [state.cursor, state.dais, bits(state.insidePoint[0]), bits(state.insidePoint[1]), state.markerCounter >>> 0, scene.local].map((value) => value >>> 0).join(',');
        const raws = [50, 60, 61, 62, 63, 64].map((sid) => hex(scene.pool.slot(sid))).join(';');
        const type = scene.types[c[29]];
        return [result, scene.trace.length === 0 ? '-' : scene.trace.join(';'), hex(p), globals, `${type.flags1 >>> 0},${type.flags2 >>> 0},${scene.stamps[c[29]]}`, raws];
    };
    return scene;
}

test('CarrierInput: 원본 10.78 454입력의 평가/강조/클릭/확인 음향 반환·효과 순서·최종 점·전역·TYPE·장부·여섯 슬롯 raw 가 두 정밀도 열과 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 모든 입력을 53비트 열과 대조하고 64비트 열이 같은지도 확인한다.
    for (const row of rows) {
        assert.equal(row.length, 13);
        const actual = makeScene(row).run();
        assert.deepEqual(actual, row.slice(1, 7), `행 ${count}: 입력 ${row[0]}`);
        assert.deepEqual(row.slice(7), row.slice(1, 7), `행 ${count}: 64비트 열`);
        count += 1;
    }
    assert.equal(count, 454);
});

test('CarrierInput: 미연결 하위·판본/TYPE 표/음향 장부/SID 범위 오류는 해당 분기를 쓰는 시점에 거부한다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = makeScene(rows[0]);
    const make = (hooks = {}) => new RawCarrierInput(scene.pool, scene.types, scene.audio, scene.stamps, scene.frame, scene.state, hooks);
    assert.ok(throws(() => make({}).evaluate(50, { x: 1, y: 1 })));
    assert.ok(throws(() => make({}).acknowledge(50, 0)));
    assert.ok(throws(() => make({}).highlight(0, { x: 1, y: 1 }, 0)));
    assert.ok(throws(() => make({}).commit(0, { x: 1, y: 1 })));
    assert.ok(throws(() => make(scene.hooks).highlight(24000, { x: 1, y: 1 }, 0)));
    assert.ok(throws(() => make(scene.hooks).evaluate(0, { x: 1, y: 1 })));
    assert.ok(throws(() => new RawCarrierInput({ edition: 'cd1072' }, scene.types, scene.audio, scene.stamps, scene.frame, scene.state, {})));
    assert.ok(throws(() => new RawCarrierInput(scene.pool, scene.types.slice(0, 171), scene.audio, scene.stamps, scene.frame, scene.state, {})));
    assert.ok(throws(() => new RawCarrierInput(scene.pool, scene.types, scene.audio.slice(0, 10), scene.stamps, scene.frame, scene.state, {})));
    assert.ok(throws(() => new RawCarrierInput(scene.pool, scene.types, scene.audio, new Uint32Array(10), scene.frame, scene.state, {})));
});
