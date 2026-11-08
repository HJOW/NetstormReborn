// 건설 요청 raw-construction-request.js(RawConstructionRequest·ConstructionRequestState·makeConstructionRequestHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: constructionrequest-x86.tsv 의 `originals` 816행 — 요청(타입·인자·방향·좌표·플레이어·flags·시각·품질)과 전역(편집 모드·로컬·밀어내기 genus)·발자국·프레임 표·spot 윤곽·표면 객체·후보 목록에서
//    반환값과 효과 순서(M 배치 가능 판정 인자/A 칸 탐색 범위/D 밀어내기/C 확정[받침 선행 포함]/K 실패 정리)가 같다. `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 필수 경계 누락·짧은 지도·표 밖 타입·비유한 좌표 거부(효과 전), 확정 경계가 바꾼 현재 flags1 로 실패 정리 여부를 다시 판정, 어댑터의 풀 불일치 거부와 실제 모듈 연결.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E140: 건설 요청 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { ConstructionRequestState, RawConstructionRequest, SPOT_COUNT, makeConstructionRequestHooks } from '../www/core/raw-construction-request.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E140)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'constructionrequest-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본별 관측 행 수, 분석 입력의 다른 타입/표면 객체 번호, 일반/섬/받침 타입, 가상 표 기록값, 풀 크기이다. (2026-11-09 WB04-E140)
const ROWS = 816;
const OTHER_TYPE = 102;
const SURFACE = 80;
const PLAIN = 100;
const ISLAND = 94;
const NO_ISLAND = 157;
const VTABLE = 0x11010000;
const CAPACITY = 24000;

// 예외가 나는지 본다. (2026-11-09 WB04-E140)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 실행기의 frame_codes 와 같은 프레임 코드 표 입력이다. 0=기본, 1=역순, 2=부호 있는 플래그, 3=품질 프레임 없음. (2026-11-09 WB04-E140)
function inputFrames(profile) {
    const codes = [];
    const base = profile === 2 ? 0x80 : 0;
    // 방향 글자 A~P마다 번호 0~10과 품질 플래그 프레임 둘을 둔다.
    for (let side = 65; side <= 80; side += 1) {
        for (let number = 0; number <= 10; number += 1) codes.push({ side, variant: 80, number, flags: base });
        if (profile !== 3) {
            codes.push({ side, variant: 80, number: 11, flags: base | 0x20 });
            codes.push({ side, variant: 80, number: 12, flags: base | 0x40 });
        }
    }
    if (profile === 1) codes.reverse();
    codes.push(codes[0]);
    return codes;
}

// 같은 저장소를 유지하며 각 행의 타입·프레임·후보·spot 과 기록 경계를 다시 공급하는 장면이다. (2026-11-09 WB04-E140)
class Scene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0 }));
        this.frames = Array.from({ length: 188 }, () => ({ frames: [], defaultFrame: 0 }));
        this.spots = new Uint8Array(SPOT_COUNT);
        this.state = new ConstructionRequestState();
        this.input = [];
        this.events = [];
        this.confirmIndex = 0;
    }

    // 원본 실행기와 같은 타입/전역/spot 입력을 만든다. (2026-11-09 WB04-E140)
    prepare(v) {
        this.input = v;
        for (const record of this.types) Object.assign(record, { flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0 });
        for (const meta of this.frames) Object.assign(meta, { frames: [], defaultFrame: 0 });
        this.hash.reset();
        this.events = [];
        this.confirmIndex = 0;
        this.types[v[0]].footX = v[7] | 0;
        this.types[v[0]].footY = v[8] | 0;
        this.frames[v[0]] = { frames: inputFrames(v[9]), defaultFrame: v[10] | 0 };
        this.types[v[0]].flags1 = v[15];
        this.types[v[0]].flags2 = v[16];
        this.types[OTHER_TYPE].footX = v[11] | 0;
        this.types[OTHER_TYPE].footY = v[12] | 0;
        this.types[ISLAND].footX = v[24] | 0;
        this.types[ISLAND].footY = v[25] | 0;
        if (v[0] !== NO_ISLAND) this.types[NO_ISLAND].footX = this.types[NO_ISLAND].footY = 1;
        this.state = new ConstructionRequestState();
        Object.assign(this.state, { editor: v[13] !== 0, localPlayer: v[14], displaceGenus: v[17] });
        this.spots.fill(v[22] === 8 ? 0 : v[22]);
        if (v[22] === 8) this.spots[22 * 256 + 21] = 2;
        this.pool.physical(0).fill(0);
    }

    // 후보 하나를 지정한 단계의 버킷 머리에 잇는다. (2026-11-09 WB04-E140)
    node(n) {
        const raw = this.pool.physical(n[0]);
        raw.fill(0);
        put(raw, 0, VTABLE);
        raw[10] = n[1];
        raw[40] = n[2];
        put(raw, 14, n[3]);
        put(raw, 18, n[4]);
        raw[34] = n[5];
        put(raw, 36, n[6], 4);
        put(raw, 4, this.hash.bucket(n[7], bitsToFloat(n[3]), bitsToFloat(n[4])), 2);
        this.hash.setBucket(n[7], bitsToFloat(n[3]), bitsToFloat(n[4]), n[0]);
    }

    // buried 객체는 finder 에서는 건너뛰고 표면 조회에서는 abstract 로 보인다. (2026-11-09 WB04-E140)
    surface() {
        const input = this.input;
        if (!input[23]) return;
        const x = Math.trunc(bitsToFloat(input[3]) + 0.9998999834060669);
        const y = Math.trunc(bitsToFloat(input[4]) + 0.9998999834060669);
        if (x < 0 || y < 0 || x >= 256 || y >= 256) { this.pool.physical(0)[40] = 1; return; }
        const raw = this.pool.physical(SURFACE);
        raw.fill(0);
        raw[10] = NO_ISLAND;
        raw[40] = 9;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        put(raw, 4, this.hash.cell(0, x, y), 2);
        this.hash.setCell(0, x, y, SURFACE);
    }

    // 사건의 인자를 순서대로 문자열에 잇는다(DWORD 비트를 10진수로 보존). (2026-11-09 WB04-E140)
    append(letter, values) {
        this.events.push(`${letter}${values.map((value) => `:${value >>> 0}`).join('')}`);
    }

    // 실제 원본에서 대체한 효과만 기록한다. scan 은 대체하지 않은 finder begin 의 관찰이다. (2026-11-09 WB04-E140)
    hooks() {
        const bits = floatToBits;
        return {
            mayPlace: (q) => { this.append('M', [q.type, q.argument, bits(q.x), bits(q.y), q.flags, q.owner, q.mode, this.input[18]]); return this.input[18] !== 0; },
            displace: (x, y, type, player) => { this.append('D', [bits(x), bits(y), type, player, this.input[19]]); return this.input[19] !== 0; },
            confirm: (q) => {
                const result = this.input[this.confirmIndex === 0 ? 20 : 21];
                this.confirmIndex += 1;
                this.append('C', [q.type, bits(q.x), bits(q.y), q.player, q.argument, q.direction, q.flags, q.timeLow, q.timeHigh, q.quality, result]);
                return result !== 0;
            },
            cancel: (x, y, flags, player) => this.append('K', [bits(x), bits(y), flags, player]),
            scan: (a) => this.append('A', [a.left, a.top, a.right, a.bottom]),
        };
    }

    // 현재 장면으로 요청 몸체를 만든다. (2026-11-09 WB04-E140)
    requester(hooks = this.hooks()) {
        return new RawConstructionRequest(this.pool, this.hash, this.types, this.frames, this.spots, this.state, hooks);
    }
}

test('요청: 원본 10.78 관측 816행의 반환값과 효과 순서·인자가 같다', { skip: rows.length === 0 }, () => {
    assert.equal(rows.length, ROWS);
    const scene = new Scene();
    // 각 행에서 현재 입력을 다시 준비한다. 기대값은 JS 로 계산하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 5);
        const v = row[1].split(',').map(Number);
        assert.equal(v.length, 31);
        scene.prepare(v);
        if (row[2] !== '-') {
            // 원본 입력 순서대로 후보를 등록한다.
            for (const entry of row[2].split(';')) {
                const n = entry.split(',').map(Number);
                assert.equal(n.length, 8);
                scene.node(n);
            }
        }
        scene.surface();
        const result = scene.requester().request({ type: v[0], x: bitsToFloat(v[3]), y: bitsToFloat(v[4]), player: v[5], argument: v[1], direction: v[2], flags: v[26], timeLow: v[27], timeHigh: v[28], quality: v[29] });
        assert.equal(result ? 1 : 0, Number(row[3]), `${row[1]} | ${row[2]}`);
        assert.equal(scene.events.join(';'), row[4], `${row[1]} | ${row[2]}`);
    }
});

test('요청: 누락 경계·짧은 지도·표 밖 타입·비유한 좌표를 첫 효과 전에 거부한다', () => {
    const scene = new Scene();
    let effects = 0;
    const hooks = { mayPlace: () => { effects += 1; return true; }, confirm: () => true, displace: () => true, cancel: () => {} };
    for (const name of ['mayPlace', 'confirm', 'cancel', 'displace']) {
        assert.ok(throws(() => scene.requester({ ...hooks, [name]: undefined })), name);
    }
    assert.ok(throws(() => new RawConstructionRequest(scene.pool, scene.hash, scene.types, scene.frames, new Uint8Array(0), scene.state, hooks)));
    assert.ok(throws(() => new RawConstructionRequest({ edition: 'cd1072' }, scene.hash, scene.types, scene.frames, scene.spots, scene.state, hooks)));
    const request = scene.requester(hooks);
    const base = { type: PLAIN, x: 20, y: 21, player: 3, argument: 0, direction: 0, flags: 0, timeLow: 0, timeHigh: 0, quality: 0 };
    assert.ok(throws(() => request.request({ ...base, type: 300 })));
    assert.ok(throws(() => request.request({ ...base, x: bitsToFloat(0x7fc01234) })));
    assert.equal(effects, 0);
    assert.equal(request.pool, scene.pool);
});

test('요청: 확정 경계가 바꾼 현재 타입 flags1 로 실패 정리 여부를 다시 판정한다', () => {
    const scene = new Scene();
    scene.types[PLAIN].footX = scene.types[PLAIN].footY = 1;
    scene.frames[PLAIN] = { frames: inputFrames(0), defaultFrame: 3 };
    scene.spots.fill(2);
    let cancelled = 0;
    const hooks = {
        mayPlace: () => true, displace: () => true,
        confirm: () => { scene.types[PLAIN].flags1 ^= 0x400; return false; },
        cancel: () => { cancelled += 1; },
    };
    const request = scene.requester(hooks);
    const base = { type: PLAIN, x: 20, y: 21, player: 3, argument: 0, direction: 0, flags: 0, timeLow: 0, timeHigh: 0, quality: 0 };
    assert.equal(request.request(base), false);
    assert.equal(cancelled, 1);
    assert.equal(request.request(base), false);
    assert.equal(cancelled, 1);
});

test('요청: 어댑터는 같은 풀의 배치 가능 판정과 서버 확정만 잇는다', () => {
    const scene = new Scene();
    const log = [];
    const placement = { pool: scene.pool, mayPlace: (query) => { log.push(['may', query.type]); return true; } };
    const confirm = { pool: scene.pool, confirm: (request) => { log.push(['confirm', request.type]); return true; } };
    const other = new SidPool(CAPACITY, { server: false });
    const base = { displace: () => true, cancel: () => {} };
    assert.ok(throws(() => makeConstructionRequestHooks(other, placement, confirm, base)));
    assert.ok(throws(() => makeConstructionRequestHooks(scene.pool, { pool: other, mayPlace: () => true }, confirm, base)));
    assert.ok(throws(() => makeConstructionRequestHooks(scene.pool, placement, { pool: other, confirm: () => true }, base)));
    const connected = makeConstructionRequestHooks(scene.pool, placement, confirm, base);
    assert.equal(connected.displace, base.displace);
    assert.equal(connected.cancel, base.cancel);
    scene.prepare([PLAIN, 0, 0, floatToBits(20), floatToBits(21), 3, 1, 1, 1, 0, 3, 1, 1, 0, 3, 0, 0, 0, 1, 1, 1, 1, 2, 0, 3, 3, 0, 0, 0, 0, 0]);
    assert.equal(scene.requester(connected).request({ type: PLAIN, x: 20, y: 21, player: 3, argument: 0, direction: 0, flags: 0, timeLow: 0, timeHigh: 0, quality: 0 }), true);
    assert.deepEqual(log, [['may', PLAIN], ['confirm', PLAIN]]);
});
