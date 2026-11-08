// 로컬 건설 배치 raw-construction-local.js(RawConstructionLocal·ConstructionLocalState·makeConstructionLocalHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: constructionlocal-x86.tsv 의 `originals` 558행 — 미리보기 Form(타입·반환 플래그·품질)·요청(좌표·인자·방향)·전역(로컬 플레이어)·타입 flags1·spot 윤곽·섬 발자국·프레임 표·
//    효과 경계의 변동(Form 값 변경·타입 플래그 토글)에서 사건 순서(Q 미리보기 연관 객체/D 미리보기 삭제/R 조각 반환/C SID 생성/P 배치[받침 선행 포함]/B 사제 이동/S 서버 요청 21바이트)와
//    생성한 SID 수가 같다. `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 필수 경계 누락·짧은 지도·잘못된 미리보기/좌표/타입/spot 범위·SID 생성 번호 오류를 효과 전(또는 배치/전송 전)에 거부, 미리보기 삭제 전의 타입 캡처와 연관 객체 호출 뒤의 반환 플래그/품질 재독,
//    배치 경계가 바꾼 현재 사제 플래그, 요청 복사본 유지, 어댑터의 풀 불일치 거부와 실제 생성/배치 연결.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E140: 로컬 건설 배치 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ConstructionLocalState, RawConstructionLocal, makeConstructionLocalHooks } from '../www/core/raw-construction-local.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E140)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'constructionlocal-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 분석기의 미리보기/새 조각 번호, 관찰 행 수, 일반/섬/noIsland 타입, 가상 표 기록값, 풀 크기이다. (2026-11-09 WB04-E140)
const PREVIEW = 50;
const BORN = 100;
const ROWS = 558;
const NO_ISLAND = 157;
const ISLAND = 94;
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

// 비교에 필요한 입력과 명시 경계만 갖는 장면이다. (2026-11-09 WB04-E140)
class Scene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0 }));
        this.frames = Array.from({ length: 188 }, () => ({ frames: [], defaultFrame: 0 }));
        this.spots = new Uint8Array(65536);
        this.state = new ConstructionLocalState();
        this.input = [];
        this.events = [];
        this.born = 0;
    }

    // 원본 실행기와 같은 합성 타입/프레임/Form/spot 을 공급한다. (2026-11-09 WB04-E140)
    prepare(v) {
        this.input = v;
        for (const record of this.types) Object.assign(record, { flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0 });
        for (const meta of this.frames) Object.assign(meta, { frames: [], defaultFrame: 0 });
        this.events = [];
        this.born = 0;
        this.types[v[0]].flags1 = v[9];
        this.frames[v[0]] = { frames: inputFrames(v[13]), defaultFrame: v[14] | 0 };
        if (v[0] !== NO_ISLAND) this.frames[NO_ISLAND] = { frames: inputFrames(v[13]), defaultFrame: 3 };
        this.types[ISLAND].footX = v[11] | 0;
        this.types[ISLAND].footY = v[12] | 0;
        this.state = new ConstructionLocalState();
        this.state.localPlayer = v[5];
        this.spots.fill(v[10] === 8 ? 0 : v[10]);
        if (v[10] === 8) {
            const x = Math.trunc(bitsToFloat(v[1]) + 0.9998999834060669);
            const y = Math.trunc(bitsToFloat(v[2]) + 0.9998999834060669);
            if (x >= 0 && x < 256 && y >= 0 && y < 256) this.spots[y * 256 + x] = 2;
        }
        const raw = this.pool.physical(PREVIEW);
        raw.fill(0);
        put(raw, 0, VTABLE);
        put(raw, 18, v[0], 2);
        put(raw, 20, v[6], 2);
        raw[30] = v[7] & 0xff;
    }

    // 사건 하나를 인자와 함께 잇는다(DWORD 로 보존). (2026-11-09 WB04-E140)
    append(letter, values) {
        this.events.push(`${letter}${values.map((value) => `:${value >>> 0}`).join('')}`);
    }

    // 원본에서 대체한 효과만 기록하고 같은 입력으로 raw/타입 변경을 공급한다. (2026-11-09 WB04-E140)
    hooks() {
        const v = () => this.input;
        return {
            previewSource: (sid) => {
                this.append('Q', [sid, v()[8]]);
                const raw = this.pool.physical(sid);
                if (v()[16]) { put(raw, 20, (v()[6] ^ 0x108) >>> 0, 2); raw[30] = (v()[7] ^ 3) & 0xff; }
                return v()[8];
            },
            destroyPreview: (sid, flags) => { this.append('D', [sid, flags]); this.pool.physical(sid).fill(0); },
            returnPiece: (flags, source, quality) => this.append('R', [flags, source, quality]),
            create: (type, flags) => { const sid = BORN + this.born; this.born += 1; this.append('C', [type, flags, sid]); return sid; },
            place: (q) => {
                this.append('P', [q.type, floatToBits(q.x), floatToBits(q.y), q.argument, q.direction, q.player, q.sids.length, q.abstract, q.quality]);
                this.events[this.events.length - 1] += `:${q.sids.length === 0 ? '-' : q.sids.join(',')}`;
                if (q.type === v()[0]) this.types[q.type].flags1 = (this.types[q.type].flags1 ^ v()[15]) >>> 0;
            },
            moveBuilder: (player, sid, low, high) => this.append('B', [player, sid, low, high]),
            send: (target, q) => {
                const bytes = new Uint8Array(21);
                bytes[0] = q.type;
                put(bytes, 1, floatToBits(q.x));
                put(bytes, 5, floatToBits(q.y));
                put(bytes, 9, q.argument, 2);
                bytes[11] = q.direction;
                bytes[12] = q.quality;
                put(bytes, 13, q.timeLow);
                put(bytes, 17, q.timeHigh);
                this.append('S', [target]);
                this.events[this.events.length - 1] += `:${Buffer.from(bytes).toString('hex')}`;
            },
        };
    }

    // 현재 장면으로 로컬 배치 몸체를 만든다. (2026-11-09 WB04-E140)
    local(hooks = this.hooks()) {
        return new RawConstructionLocal(this.pool, this.types, this.frames, this.spots, this.state, hooks);
    }
}

test('로컬 건설: 원본 10.78 관측 558행의 사건 순서·서버 요청 21바이트·SID 수가 같다', { skip: rows.length === 0 }, () => {
    assert.equal(rows.length, ROWS);
    const scene = new Scene();
    // 각 관찰 행에 동일한 입력만 공급해 실제 사건 순서/인자를 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 3);
        const v = row[1].split(',').map(Number);
        assert.equal(v.length, 19);
        scene.prepare(v);
        scene.local().place({ preview: PREVIEW, x: bitsToFloat(v[1]), y: bitsToFloat(v[2]), argument: v[3], direction: v[4] });
        assert.equal(scene.events.join(';'), row[2], row[1]);
        assert.equal(scene.born, v[17] + v[18], row[1]);
    }
});

test('로컬 건설: 잘못된 자료/좌표는 첫 Form 효과 전에 거부하고 생성 번호 오류는 배치/전송 전에 거부한다', { skip: rows.length === 0 }, () => {
    const scene = new Scene();
    const v = rows[0][1].split(',').map(Number);
    scene.prepare(v);
    const hooks = scene.hooks();
    for (const name of ['previewSource', 'destroyPreview', 'returnPiece', 'create', 'place', 'moveBuilder', 'send']) {
        assert.ok(throws(() => scene.local({ ...hooks, [name]: undefined })), name);
    }
    assert.ok(throws(() => new RawConstructionLocal(scene.pool, scene.types, scene.frames, new Uint8Array(0), scene.state, hooks)));
    assert.ok(throws(() => new RawConstructionLocal({ edition: 'cd1072' }, scene.types, scene.frames, scene.spots, scene.state, hooks)));
    const local = scene.local(hooks);
    assert.ok(throws(() => local.place({ preview: 0, x: 20, y: 21, argument: 0, direction: 0 })));
    assert.ok(throws(() => local.place({ preview: PREVIEW, x: bitsToFloat(0x7fc01234), y: 21, argument: 0, direction: 0 })));
    put(scene.pool.physical(PREVIEW), 18, 300, 2);
    assert.ok(throws(() => local.place({ preview: PREVIEW, x: 20, y: 21, argument: 0, direction: 0 })));
    assert.deepEqual(scene.events, []);
    // 받침이 필요한 타입의 spot 좌표가 지도 밖이면 거부한다.
    scene.prepare(v);
    scene.types[v[0]].flags1 = 0x400;
    assert.ok(throws(() => local.place({ preview: PREVIEW, x: -3, y: 21, argument: 0, direction: 0 })));
    assert.deepEqual(scene.events, []);
    // 생성 경계가 예약 번호 0 을 돌려주면 배치/전송 전에 거부한다.
    scene.prepare(v);
    const failed = scene.local({ ...hooks, create: () => 0 });
    assert.ok(throws(() => failed.place({ preview: PREVIEW, x: 20, y: 21, argument: 0, direction: 0 })));
    assert.ok(!scene.events.some((event) => event.startsWith('P:') || event.startsWith('S:')));
});

test('로컬 건설: 미리보기 삭제 전의 타입을 캡처하고 연관 객체 호출 뒤의 반환 플래그/품질을 읽으며 배치 경계의 현재 사제 플래그를 다시 읽는다', { skip: rows.length === 0 }, () => {
    const scene = new Scene();
    const v = rows[0][1].split(',').map(Number);
    v[5] = 0xffffff03;
    v[6] = 8;
    v[7] = 3;
    v[8] = 0;
    v[9] = 0;
    v[15] = 0x8000;
    v[16] = 0;
    scene.prepare(v);
    const request = { preview: PREVIEW, x: 20, y: 21, argument: 0x12345678, direction: 0x100 };
    const hooks = scene.hooks();
    const source = hooks.previewSource;
    // 원본의 스택 인자처럼 입력 사본을 유지한다. 효과가 호출자 요청을 바꿔도 배치/전송 인자는 처음 값이다.
    hooks.previewSource = (sid) => { request.argument = 0; request.direction = 0; return source(sid); };
    scene.local(hooks).place(request);
    const events = scene.events.join(';');
    assert.ok(events.startsWith('Q:50:0;D:50:0;R:8:0:3'), events);
    assert.ok(events.includes('B:4294967043:100:0:0'), events);
    assert.equal(request.argument, 0);
    assert.equal(request.direction, 0);
    assert.ok(events.includes(':305419896:256:4294967043:'), events);
    const raw = scene.pool.slot(PREVIEW);
    assert.equal(raw[18] | (raw[19] << 8), 0);
    assert.equal(raw[30], 0);
});

test('로컬 건설: 어댑터는 같은 풀의 실제 factory 생성과 건설 배치만 잇는다', () => {
    const scene = new Scene();
    const calls = [];
    const factory = { pool: scene.pool, create: (type, flags) => { calls.push(['create', type, flags]); return 101; } };
    const place = { pool: scene.pool, place: (request) => calls.push(['place', request.type]) };
    const other = new SidPool(CAPACITY, { server: false });
    const base = { previewSource: () => 0, send: () => {} };
    assert.ok(throws(() => makeConstructionLocalHooks(other, factory, place, base)));
    assert.ok(throws(() => makeConstructionLocalHooks(scene.pool, { pool: other, create: () => 0 }, place, base)));
    assert.ok(throws(() => makeConstructionLocalHooks(scene.pool, factory, { pool: other, place: () => {} }, base)));
    const connected = makeConstructionLocalHooks(scene.pool, factory, place, base);
    assert.equal(connected.previewSource, base.previewSource);
    assert.equal(connected.send, base.send);
    assert.equal(connected.create(100, 2), 101);
    connected.place({ type: 100 });
    assert.deepEqual(calls, [['create', 100, 2], ['place', 100]]);
});
