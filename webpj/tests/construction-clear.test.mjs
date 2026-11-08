// 건설 예측 정리 raw-construction-clear.js(RawConstructionClear·ConstructionClearState·cellArea)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: constructionclear-x86.tsv 의 `originals` 420행 — 통지 입력(타입·인자·방향·좌표·플레이어·권한·발자국·프레임 표 윤곽·기본 프레임·다른 타입 발자국)과 후보 목록(번호·타입·extra·좌표·소유자·
//    프레임·해시 단계)에서 칸마다의 탐색 범위(A)와 지운 후보·삭제 플래그(K)의 순서가 같다. `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 칸 범위의 자르기/지도 밖 축소, 한 칸에서 첫 후보 하나만 지움(서버 영역·abstract 아님·권한 있을 때 다른 타입은 제외, 소유자·프레임이 다르면 플래그 0, 권한이 없으면 다른 타입도 후보),
//    삭제 경계 누락·타입 번호 범위 밖 거부, 공유 boss 의 변경을 다음 후보 비교에서 읽기, 정리가 finder 보다 먼저 같은 현재 Canon 진단을 선택.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E137: 건설 예측 정리 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { ConstructionClearState, RawConstructionClear, SAME_PIECE_FLAG, cellArea } from '../www/core/raw-construction-clear.js';
import { ConstructionNoticeState, RawConstructionNotice, makeConstructionNoticeHooks } from '../www/core/raw-construction-notice.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E137)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'constructionclear-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본별 관측 행 수, 실행기가 후보로 섞는 다른 타입 번호, 일반 타입 번호, 가상 표 기록값, 풀 크기, 서버 영역 첫 번호이다. (2026-11-09 WB04-E137)
const ROWS = 420;
const OTHER_TYPE = 102;
const PLAIN_TYPE = 100;
const VTABLE = 0x11010000;
const CAPACITY = 24000;
const SERVER_FIRST = 15000;

// 예외가 나는지 본다. (2026-11-09 WB04-E137)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 실행기의 frame_codes 와 같은 프레임 코드 표 입력이다. 0=기본, 1=역순, 2=부호 있는 플래그, 3=품질 프레임 없음. (2026-11-09 WB04-E137)
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

// 풀·해시·타입·프레임과 후보 등록을 행마다 다시 준비하는 장면이다. (2026-11-09 WB04-E137)
class Scene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0 }));
        this.frames = Array.from({ length: 188 }, () => ({ frames: [], defaultFrame: 0 }));
        this.state = new ConstructionClearState();
        this.events = [];
    }

    // 놓을 타입과 다른 타입의 발자국, 프레임 표, 권한 전역을 준비하고 해시를 비운다. (2026-11-09 WB04-E137)
    prepare(type, footX, footY, otherFootX, otherFootY, frameProfile, defaultFrame, authority) {
        for (const record of this.types) Object.assign(record, { flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0 });
        for (const meta of this.frames) Object.assign(meta, { frames: [], defaultFrame: 0 });
        this.hash.reset();
        this.events = [];
        this.types[type].footX = footX;
        this.types[type].footY = footY;
        this.types[OTHER_TYPE].footX = otherFootX;
        this.types[OTHER_TYPE].footY = otherFootY;
        this.frames[type] = { frames: inputFrames(frameProfile), defaultFrame: defaultFrame | 0 };
        this.state = new ConstructionClearState();
        this.state.authority = authority;
    }

    // 후보 하나의 raw 슬롯을 쓰고 지정한 단계의 버킷 체인 머리에 잇는다. 실행기의 등록 순서와 같다. (2026-11-09 WB04-E137)
    node(sid, type, extra, x, y, owner, frame, level) {
        const raw = this.pool.physical(sid);
        raw.fill(0);
        put(raw, 0, VTABLE);
        raw[10] = type;
        raw[40] = extra;
        raw[34] = owner;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        put(raw, 36, frame, 4);
        put(raw, 4, this.hash.bucket(level, x, y), 2);
        this.hash.setBucket(level, x, y, sid);
    }

    // 삭제는 기록만 하고 해시에서 빼지 않는다(실행기의 경계와 같다). 탐색 범위도 기록한다. (2026-11-09 WB04-E137)
    hooks() {
        return {
            destroy: (sid, flags) => this.events.push(`K:${sid}:${flags}`),
            scan: (area) => this.events.push(`A:${area.left}:${area.top}:${area.right}:${area.bottom}`),
        };
    }

    // 현재 장면으로 정리 몸체를 만든다. (2026-11-09 WB04-E137)
    clearer(hooks = this.hooks()) {
        return new RawConstructionClear(this.pool, this.hash, this.types, this.frames, this.state, hooks);
    }
}

test('예측 정리: 원본 10.78 관측 420행의 탐색 범위와 지운 후보·삭제 플래그 순서가 같다', { skip: rows.length === 0 }, () => {
    assert.equal(rows.length, ROWS);
    const scene = new Scene();
    // 행마다 장면을 다시 만들고 같은 호출을 한다.
    for (const row of rows) {
        assert.equal(row.length, 4);
        const v = row[1].split(',').map(Number);
        assert.equal(v.length, 13);
        scene.prepare(v[0], v[7] | 0, v[8] | 0, v[11] | 0, v[12] | 0, v[9], v[10], v[6] !== 0);
        if (row[2] !== '-') {
            // 후보 칸은 번호, 타입, extra, x, y, 소유자, 프레임, 해시 단계이다.
            for (const entry of row[2].split(';')) {
                const n = entry.split(',').map(Number);
                assert.equal(n.length, 8);
                scene.node(n[0], n[1], n[2], bitsToFloat(n[3]), bitsToFloat(n[4]), n[5], n[6], n[7]);
            }
        }
        scene.clearer().clear({ type: v[0], x: bitsToFloat(v[3]), y: bitsToFloat(v[4]), argument: v[1], direction: v[2], player: v[5] });
        assert.equal(scene.events.length === 0 ? '-' : scene.events.join(';'), row[3], `${row[1]} | ${row[2]}`);
    }
});

test('예측 정리: 칸 범위는 발자국만큼 넓히고 1~255 로 자르며 지도 밖 칸은 자른 점 하나가 된다', () => {
    const same = (area, left, top, right, bottom) => area.left === left && area.top === top && area.right === right && area.bottom === bottom;
    assert.ok(same(cellArea(20.5, 21.25, 1, 1), 20, 21, 21, 22));
    assert.ok(same(cellArea(20.5, 21.25, 3, 2), 18, 20, 21, 22));
    assert.ok(same(cellArea(20, 21, 1, 1), 20, 21, 20, 21));
    // 왼쪽/위로 물린 점은 1 아래로 내려가지 않는다. 칸 좌표가 유효하면 칸 쪽 모서리는 그대로이다.
    assert.ok(same(cellArea(0.5, 0.5, 2, 2), 0, 0, 1, 1));
    // 칸 좌표가 0 이하이거나 256 이상이면 두 모서리 모두 자른 점이다.
    assert.ok(same(cellArea(0, 5, 1, 1), 1, 5, 1, 5));
    assert.ok(same(cellArea(256, 300, 1, 1), 255, 255, 255, 255));
    assert.ok(same(cellArea(-3.5, -2.5, 2, 2), 1, 1, 1, 1));
});

test('예측 정리: 한 칸에서 첫 후보 하나만 지우고 서버 영역·abstract 아님·다른 타입은 건드리지 않는다', () => {
    const scene = new Scene();
    scene.prepare(PLAIN_TYPE, 1, 1, 1, 1, 0, 3, true);
    // 같은 타입·프레임·소유자의 로컬 예측 두 개, abstract 가 아닌 조각, 서버 영역 번호, 다른 타입을 같은 칸에 둔다.
    scene.node(60, PLAIN_TYPE, 1, 20.5, 21.25, 2, 3, 0);
    scene.node(61, PLAIN_TYPE, 1, 20.5, 21.25, 2, 3, 0);
    scene.node(62, PLAIN_TYPE, 0, 20.5, 21.25, 2, 3, 0);
    scene.node(SERVER_FIRST + 4, PLAIN_TYPE, 1, 20.5, 21.25, 2, 3, 0);
    scene.node(63, OTHER_TYPE, 1, 20.5, 21.25, 2, 3, 0);
    const removed = [];
    // 지운 조각이 정확히 하나이고 번호와 삭제 플래그가 기대와 같은지 본다.
    const only = (sid, flags) => removed.length === 1 && removed[0][0] === sid && removed[0][1] === flags;
    const clear = scene.clearer({ destroy: (sid, flags) => removed.push([sid, flags]) });
    clear.clear({ type: PLAIN_TYPE, x: 20.5, y: 21.25, argument: 7, direction: 0, player: 2 });
    // 체인의 머리는 마지막에 등록한 것부터이다. 다른 타입(63)과 서버 번호를 건너뛰고 61 하나만 같은 조각으로 지운다.
    assert.ok(only(61, SAME_PIECE_FLAG), JSON.stringify(removed));
    // 소유자가 다르거나 프레임이 다르면 삭제 플래그가 0 이다.
    removed.length = 0;
    clear.clear({ type: PLAIN_TYPE, x: 20.5, y: 21.25, argument: 7, direction: 0, player: 5 });
    assert.ok(only(61, 0), JSON.stringify(removed));
    // 권한이 없으면 타입이 다른 abstract 조각도 후보가 되어 먼저 만난 63 을 플래그 0 으로 지운다.
    scene.state.authority = false;
    removed.length = 0;
    clear.clear({ type: PLAIN_TYPE, x: 20.5, y: 21.25, argument: 7, direction: 0, player: 2 });
    assert.ok(only(63, 0), JSON.stringify(removed));
    // 삭제 경계가 없거나 타입 번호가 범위 밖이면 거부한다.
    assert.ok(throws(() => scene.clearer({})));
    assert.ok(throws(() => new RawConstructionClear({ edition: 'cd1072' }, scene.hash, scene.types, scene.frames, scene.state, scene.hooks())));
    const quiet = new Scene();
    quiet.prepare(PLAIN_TYPE, 1, 1, 1, 1, 0, 3, true);
    assert.ok(throws(() => quiet.clearer().clear({ type: 300, x: 20.5, y: 21.25, argument: 0, direction: 0, player: 1 })));
    assert.deepEqual(quiet.events, []);
    assert.equal(quiet.clearer().pool, quiet.pool);
});

test('예측 정리: 다리 타입은 프레임이 달라도 방향 글자가 같으면 같은 조각으로 지운다', () => {
    const scene = new Scene();
    scene.prepare(82, 1, 1, 1, 1, 0, 3, true);
    // 다리 패턴 0 번은 방향 글자 'J'(변형 1) 한 칸이라 디코더 프레임은 A~P × 번호 0~10 표에서 (J−A)×13+1 = 118 이다(한 방향 글자마다 번호 0~10 과 품질 프레임 둘, 모두 13개).
    const removed = [];
    const clear = scene.clearer({ destroy: (sid, flags) => removed.push([sid, flags]) });
    const request = { type: 82, x: 20, y: 21, argument: 0, direction: 0, player: 2 };
    // 같은 방향 글자의 다른 번호(119)는 같은 조각이다.
    scene.node(60, 82, 1, 20, 21, 2, 119, 0);
    clear.clear(request);
    assert.deepEqual(removed, [[60, SAME_PIECE_FLAG]]);
    // 방향 글자가 다른 프레임(K, 번호 1 = 131)은 같은 조각이 아니라 플래그 0 이다.
    removed.length = 0;
    scene.hash.reset();
    scene.node(60, 82, 1, 20, 21, 2, 131, 0);
    clear.clear(request);
    assert.deepEqual(removed, [[60, 0]]);
    // 디코더와 프레임이 정확히 같으면 방향 비교 없이 같은 조각이다.
    removed.length = 0;
    scene.hash.reset();
    scene.node(60, 82, 1, 20, 21, 2, 118, 0);
    clear.clear(request);
    assert.deepEqual(removed, [[60, SAME_PIECE_FLAG]]);
    // 후보 프레임이 프레임 표 밖이면 방향 글자 비교에서 거부한다.
    scene.hash.reset();
    scene.node(60, 82, 1, 20, 21, 2, 100000, 0);
    removed.length = 0;
    assert.ok(throws(() => clear.clear(request)));
    assert.deepEqual(removed, []);
});

test('예측 정리: 공유 boss 의 변경을 다음 후보 비교에서 읽는다', () => {
    const scene = new Scene();
    scene.prepare(110, 1, 1, 1, 1, 0, 0, true);
    scene.node(50, OTHER_TYPE, 1, 20, 21, 1, 0, 0);
    let boss = true;
    scene.state.sharedAuthority = () => boss;
    let removed = 0;
    const clear = scene.clearer({ destroy: (sid) => { assert.equal(sid, 50); removed += 1; } });
    clear.clear({ type: 110, x: 20, y: 21, argument: 0, direction: 0, player: 1 });
    assert.equal(removed, 0);
    boss = false;
    clear.clear({ type: 110, x: 20, y: 21, argument: 0, direction: 0, player: 1 });
    assert.equal(removed, 1);
});

test('예측 정리: finder 와 삭제보다 먼저 같은 현재 Canon 진단을 선택한다', () => {
    const scene = new Scene();
    scene.prepare(PLAIN_TYPE, 1, 1, 1, 1, 0, 0, true);
    scene.frames[PLAIN_TYPE].frames = [{ side: 65, variant: 80, number: 0, flags: 0x20 }];
    let reported = 0;
    const diagnostics = {
        checking: () => 0x80000000,
        assertion: (expression, file, line) => { assert.equal(line, 0x325); reported += 1; throw new Error('정리 Canon 진단 중단'); },
    };
    scene.state.decoderDiagnostics = diagnostics;
    assert.ok(throws(() => scene.clearer().clear({ type: PLAIN_TYPE, x: 20, y: 21, argument: 0, direction: 0, player: 1 })));
    assert.equal(reported, 1);
    assert.deepEqual(scene.events, []);
});

test('예측 정리: 배치 통지 어댑터를 거쳐 실제 정리가 배치보다 먼저 실행된다', () => {
    const scene = new Scene();
    scene.prepare(PLAIN_TYPE, 1, 1, 1, 1, 0, 3, true);
    scene.node(60, PLAIN_TYPE, 1, 20, 21, 2, 3, 0);
    const log = [];
    const clear = scene.clearer({ destroy: (sid, flags) => log.push(`K:${sid}:${flags}`) });
    const place = { pool: scene.pool, place: (request) => log.push(`P:${request.type}:${request.sids.join(',')}`), actualPlace: () => {}, refund: () => {} };
    const base = {
        moveBuilder: () => {}, findPriest: () => 0, immobile: () => true, approach: () => ({ x: 0, y: 0 }), tryMove: () => false, destroy: () => {}, start: () => {},
        refresh: () => log.push('W'), containedType: () => 0, removeContained: () => {},
    };
    const noticeState = new ConstructionNoticeState();
    noticeState.localPlayer = 2;
    const handler = new RawConstructionNotice(scene.pool, scene.types, noticeState, makeConstructionNoticeHooks(scene.pool, clear, place, base));
    // 프레임 3, 소유자 2 의 예측 조각이 통지와 같은 조각 플래그로 지워진 뒤 확정 조각이 놓인다.
    handler.handle({ type: PLAIN_TYPE, x: 20, y: 21, argument: 0, direction: 0, player: 2, flags: 0, quality: 0, timeLow: 0, timeHigh: 0, sids: [SERVER_FIRST + 1] });
    assert.deepEqual(log, [`K:60:${SAME_PIECE_FLAG}`, `P:${PLAIN_TYPE}:${SERVER_FIRST + 1}`, 'W']);
});
