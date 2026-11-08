// 건설 확정 raw-construction-confirm.js(RawConstructionConfirm·ConstructionConfirmState·makeConstructionConfirmHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: constructionconfirm-x86.tsv 의 `originals` 508행 — 요청 열 개(타입·좌표·플레이어·인자·방향·flags·시각 두 DWORD·품질)와 전역·타입 flags1·기준점 조회 결과·기록기 상태·프레임 표·조각 수에서
//    하위 효과 순서(B 기준점 조회/C SID 생성/S 통지 전송/H 서버 번호 9 직접 처리/Q 기록기 활성 조회/R 기록)와 통지 본문(좌표·시각 비트, 하위 바이트 필드, SID 목록), 반환값, 만든 SID 개수가 같다.
//    `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 필수 경계 누락·다른 풀의 factory 거부, 예약 번호를 돌려준 SID 생성·타입 번호 범위 밖은 통지 전에 거부, 조각이 없을 때 기록기 인자 0, 공유 FullDeck 의 현재 DWORD 로 기준점 관문 결정(최상위 비트도 참),
//    SID 생성 전에 현재 Canon 진단 선택, 실제 wire 경로(actualConfirm)의 지역 패킷이 typed 통지의 직렬화와 같고 첫 SID 를 기록기에 넘기며 진단/송신 경계가 없으면 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E139: 건설 확정 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { createConstructionPacketHeader, writeConstructionPacket } from '../www/core/raw-construction-packet.js';
import { ConstructionConfirmState, RawConstructionConfirm, makeConstructionConfirmHooks } from '../www/core/raw-construction-confirm.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E139)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'constructionconfirm-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본별 관측 행 수, 실행기가 새 SID 를 서버 영역 첫 번호에서 띄운 거리, 일반/noIsland 타입 번호, 풀 크기, 서버 영역 첫 번호, 사제가 지어야 함 비트이다. (2026-11-09 WB04-E139)
const ROWS = 508;
const BORN_OFFSET = 8;
const PLAIN = 100;
const NO_ISLAND = 157;
const CAPACITY = 24000;
const SERVER_FIRST = 15000;
const NEEDS_BUILDER = 0x8000;

// 예외가 나는지 본다. (2026-11-09 WB04-E139)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 실행기의 frame_codes 와 같은 프레임 코드 표 입력이다. 0=기본, 1=역순, 2=부호 있는 플래그, 3=품질 프레임 없음. (2026-11-09 WB04-E139)
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

// 통지 본문을 실행기와 같은 칸 순서의 한 줄로 만든다. (2026-11-09 WB04-E139)
function noticeText(notice) {
    const text = [notice.type, floatToBits(notice.x), floatToBits(notice.y), notice.argument, notice.direction, notice.player, notice.flags, notice.quality,
        notice.timeLow, notice.timeHigh, notice.sids.length].join(':');
    return `${text}:${notice.sids.length === 0 ? '-' : notice.sids.join(',')}`;
}

// 기록만 하는 경계 묶음이다. 개별 검사가 필요한 것만 바꿔 쓴다. (2026-11-09 WB04-E139)
function quietHooks() {
    return { findBuilder: () => 77, create: () => 0, broadcast: () => {}, handle: () => {}, recorderActive: () => false, record: () => {} };
}

test('확정: 원본 10.78 관측 508행의 효과 순서·통지 본문·반환값·만든 SID 수가 같다', { skip: rows.length === 0 }, () => {
    assert.equal(rows.length, ROWS);
    const pool = new SidPool(CAPACITY, { server: true });
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    const frames = Array.from({ length: 188 }, () => ({ frames: [], defaultFrame: 0 }));
    // 행마다 타입·프레임·전역 입력을 다시 준비한다.
    for (const row of rows) {
        assert.equal(row.length, 4);
        const v = row[1].split(',').map(Number);
        assert.equal(v.length, 18);
        for (const record of types) Object.assign(record, { flags1: 0, flags2: 0, footX: 0, footY: 0 });
        for (const meta of frames) Object.assign(meta, { frames: [], defaultFrame: 0 });
        types[v[0]].flags1 = v[10];
        frames[v[0]] = { frames: inputFrames(v[15]), defaultFrame: v[16] | 0 };
        const state = new ConstructionConfirmState();
        state.skipBuilderCheck = v[11] !== 0;
        state.recorderEnabled = v[13] !== 0;
        const events = [];
        let born = 0;
        const confirm = new RawConstructionConfirm(pool, types, frames, state, {
            findBuilder: (player, type, x, y) => { events.push(`B:${player}:${type}:${floatToBits(x)}:${floatToBits(y)}`); return v[12]; },
            create: (type, flags) => { events.push(`C:${type}:${flags}`); const sid = SERVER_FIRST + BORN_OFFSET + born; born += 1; return sid; },
            broadcast: (notice) => events.push(`S:${noticeText(notice)}`),
            handle: (notice) => events.push(`H:9:0:${noticeText(notice)}`),
            recorderActive: () => { events.push('Q'); return v[14] !== 0; },
            record: (sid) => events.push(`R:${sid}`),
        });
        const result = confirm.confirm({ type: v[0], x: bitsToFloat(v[1]), y: bitsToFloat(v[2]), player: v[3], argument: v[4], direction: v[5], flags: v[6], timeLow: v[7], timeHigh: v[8], quality: v[9] });
        assert.equal(events.length === 0 ? '-' : events.join(';'), row[2], row[1]);
        assert.equal(result ? 1 : 0, Number(row[3]), row[1]);
        assert.equal(born, v[17] * (result ? 1 : 0), row[1]);
    }
});

test('확정: 누락 경계·다른 풀의 factory·잘못된 생성 결과·범위 밖 타입을 통지 전에 거부하고 조각이 없으면 기록기에 0 을 넘긴다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const other = new SidPool(CAPACITY, { server: true });
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    const frames = Array.from({ length: 188 }, () => ({ frames: [], defaultFrame: 0 }));
    frames[PLAIN] = { frames: inputFrames(0), defaultFrame: 3 };
    const state = new ConstructionConfirmState();
    assert.ok(throws(() => new RawConstructionConfirm(pool, types, frames, state, { ...quietHooks(), handle: undefined })));
    for (const name of ['findBuilder', 'create', 'recorderActive', 'record']) {
        assert.ok(throws(() => new RawConstructionConfirm(pool, types, frames, state, { ...quietHooks(), [name]: undefined })), name);
    }
    assert.ok(throws(() => new RawConstructionConfirm({ edition: 'cd1072' }, types, frames, state, quietHooks())));
    assert.ok(throws(() => makeConstructionConfirmHooks(pool, { pool: other, create: () => 0 }, quietHooks())));
    let sent = 0;
    const hooks = { ...quietHooks(), broadcast: () => { sent += 1; } };
    const confirm = new RawConstructionConfirm(pool, types, frames, state, hooks);
    // 생성 경계가 예약 번호 0 을 돌려주면 통지를 보내기 전에 거부한다. 범위 밖 타입도 마찬가지다.
    assert.ok(throws(() => confirm.confirm({ type: PLAIN, x: 20, y: 21, player: 1, argument: 0, direction: 0, flags: 0, timeLow: 0, timeHigh: 0, quality: 0 })));
    assert.equal(sent, 0);
    assert.ok(throws(() => confirm.confirm({ type: 300, x: 20, y: 21, player: 1, argument: 0, direction: 0, flags: 0, timeLow: 0, timeHigh: 0, quality: 0 })));
    assert.equal(sent, 0);
    // 기록기가 켜져 있고 활성이면 첫 SID 를, 조각이 없으면 0 을 넘긴다(원본의 미초기화 값은 재현하지 않는다).
    state.recorderEnabled = true;
    const recorded = [];
    const recording = new RawConstructionConfirm(pool, types, frames, state, { ...hooks, create: () => 15010, recorderActive: () => true, record: (sid) => recorded.push(sid) });
    const request = { type: PLAIN, x: 20, y: 21, player: 1, argument: 0, direction: 0, flags: 0, timeLow: 0, timeHigh: 0, quality: 0 };
    assert.equal(recording.confirm(request), true);
    frames[PLAIN].defaultFrame = -1;
    assert.equal(recording.confirm(request), true);
    assert.deepEqual(recorded, [15010, 0]);
    assert.equal(sent, 2);
    // 같은 풀의 factory 는 SID 생성만 바꾸고 나머지 경계를 보존한다.
    const calls = [];
    const adapted = makeConstructionConfirmHooks(pool, { pool, create: (type, flags) => { calls.push([type, flags]); return 15011; } }, hooks);
    assert.equal(adapted.create(100, 0), 15011);
    assert.deepEqual(calls, [[100, 0]]);
    assert.equal(adapted.broadcast, hooks.broadcast);
});

test('확정: 공유 FullDeck 의 현재 DWORD 가 기준점 관문을 결정한다(최상위 비트도 참)', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    const frames = Array.from({ length: 188 }, () => ({ frames: [], defaultFrame: 0 }));
    types[PLAIN].flags1 = NEEDS_BUILDER;
    frames[PLAIN] = { frames: inputFrames(0), defaultFrame: 3 };
    const state = new ConstructionConfirmState();
    state.skipBuilderCheck = true;
    let fullDeck = 0;
    state.sharedFullDeck = () => fullDeck;
    let builders = 0;
    let sent = 0;
    const confirm = new RawConstructionConfirm(pool, types, frames, state, {
        ...quietHooks(), findBuilder: () => { builders += 1; return 0; }, create: () => 15010, broadcast: () => { sent += 1; },
    });
    const request = { type: PLAIN, x: 20, y: 21, player: 1, argument: 0, direction: 0, flags: 0, timeLow: 0, timeHigh: 0, quality: 0 };
    assert.equal(confirm.confirm(request), false);
    assert.equal(builders, 1);
    assert.equal(sent, 0);
    // 켜진 값(최상위 비트 포함)은 기준점 조회 없이 확정하고 DWORD 값은 바뀌지 않는다.
    for (const value of [0x80000000, 0xffffffff]) {
        fullDeck = value;
        assert.equal(confirm.confirm(request), true);
        assert.equal(builders, 1);
        assert.equal(fullDeck, value);
    }
    fullDeck = 0;
    assert.equal(confirm.confirm(request), false);
    assert.equal(builders, 2);
    assert.equal(sent, 2);
    // 공유 owner 가 없으면 기존 독립 bool 입력을 유지한다.
    state.sharedFullDeck = null;
    assert.equal(confirm.confirm(request), true);
    assert.equal(builders, 2);
    assert.equal(sent, 3);
});

test('확정: SID 생성 전에 현재 Canon 진단 owner 를 소비한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    const frames = Array.from({ length: 188 }, () => ({ frames: [], defaultFrame: 0 }));
    frames[PLAIN] = { frames: [{ side: 65, variant: 80, number: 0, flags: 0x20 }], defaultFrame: 0 };
    const state = new ConstructionConfirmState();
    let reported = 0;
    let allocated = 0;
    state.decoderDiagnostics = { checking: () => 1, assertion: (expression, file, line) => { assert.equal(line, 0x325); reported += 1; throw new Error('확정 Canon 진단 중단'); } };
    const confirm = new RawConstructionConfirm(pool, types, frames, state, { ...quietHooks(), create: () => { allocated += 1; return 0; } });
    assert.ok(throws(() => confirm.confirm({ type: PLAIN, x: 20, y: 21, player: 1, argument: 0, direction: 0, flags: 0, timeLow: 0, timeHigh: 0, quality: 0 })));
    assert.equal(reported, 1);
    assert.equal(allocated, 0);
});

test('확정: 실제 wire 경로의 지역 패킷이 typed 통지의 직렬화와 같고 첫 SID 를 기록기에 넘긴다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    const frames = Array.from({ length: 188 }, () => ({ frames: [], defaultFrame: 0 }));
    frames[NO_ISLAND] = { frames: inputFrames(0), defaultFrame: 3 };
    const state = new ConstructionConfirmState();
    state.recorderEnabled = true;
    const request = { type: NO_ISLAND, x: 20, y: 21, player: 2, argument: 0, direction: 2, flags: 0, timeLow: 0x12345678, timeHigh: 0x40080000, quality: 1 };
    let born = 0;
    const sent = [];
    const recorded = [];
    const typedNotices = [];
    const create = () => { const sid = SERVER_FIRST + 10 + born; born += 1; return sid; };
    const typedConfirm = new RawConstructionConfirm(pool, types, frames, state, {
        ...quietHooks(), create, broadcast: (notice) => typedNotices.push(notice), handle: () => {}, recorderActive: () => false,
    });
    assert.equal(typedConfirm.confirm(request), true);
    assert.equal(typedNotices[0].sids.length, 9);
    born = 0;
    const header = createConstructionPacketHeader();
    const wire = new RawConstructionConfirm(pool, types, frames, state, {
        ...quietHooks(), create, transmitAndHandle: (packet) => sent.push(Uint8Array.from(packet)), assertion: () => { throw new Error('진단이 불리면 안 된다'); },
        broadcast: undefined, handle: undefined, recorderActive: () => true, record: (sid) => recorded.push(sid),
    });
    assert.equal(wire.actualConfirm(request, header), true);
    // 지역 512바이트가 typed 통지를 같은 헤더로 직렬화한 것과 같다.
    const expected = new Uint8Array(512);
    writeConstructionPacket(typedNotices[0], header, expected);
    assert.equal(Buffer.from(sent[0]).toString('hex'), Buffer.from(expected).toString('hex'));
    assert.deepEqual(recorded, [SERVER_FIRST + 10]);
    // typed 경로는 실제 wire 전용 구성을 빈 성공으로 호출하지 않고, 실제 경로는 송신/진단 경계가 없으면 거부한다.
    assert.ok(throws(() => wire.confirm(request)));
    assert.ok(throws(() => typedConfirm.actualConfirm(request, header)));
    // 사제가 지어야 하는 타입에서 기준점이 없으면 실제 경로도 패킷을 만들기 전에 거짓이다.
    types[NO_ISLAND].flags1 = NEEDS_BUILDER;
    const blocked = new RawConstructionConfirm(pool, types, frames, state, {
        ...quietHooks(), findBuilder: () => 0, create, transmitAndHandle: () => { throw new Error('송신하면 안 된다'); }, assertion: () => {},
    });
    assert.equal(blocked.actualConfirm(request, header), false);
});
