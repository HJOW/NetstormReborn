// 다이스 내용물 알림 raw-dais-content-notice.js(RawDaisContentNotice)를 원본 10.78 관측과 cpppj `DaisContentNoticeTests` 계약으로 대조한다.
//  - 원본 관측 daiscontentnotice-x86.tsv 의 10.78(originals) 행: 입력 JSON(제단 존재 여부·첫 'A' 프레임 수·내용물 kind·로컬 owner·다이스 owner·reason·원래 owner WORD)에서
//    가상 공간/cue 요청 순서(unpop:7,0·repop:7,0·V:99,0,0)와 Dais(5)·제단(7)·내용물(10) 세 슬롯의 raw 전체(가상 표 DWORD 제외)를 대조한다. 가상 공간과 장치는 명시 경계이며 전체 희생/Pop 의 소비자 완료로 세지 않는다.
//  - 계약: 생성자 VT 가 아닌 부모 거부·다른 풀/프레임 표 크기 거부·reason 2 제거의 자연 반환·선택된 장치 누락의 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E219: 다이스 내용물 알림 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { RawRunePostPop, RunePostPopState } from '../www/core/raw-rune-post-pop.js';
import { PATCH_DAIS_VTABLE, RawDaisContentNotice } from '../www/core/raw-dais-content-notice.js';

// 원본 관측 10.78(originals) 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-10 WB04-E219)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'daiscontentnotice-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 알림 장면의 공통 가상 표, 제단 가상 표(실제 164 최종 가상 표)이다. (2026-11-10 WB04-E219)
const COMMON_VTABLE = 0x11010000;
const PATCH_ALTAR_VTABLE = 0x005076b8;

// 예외가 나는지 본다. (2026-11-10 WB04-E219)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-10 WB04-E219)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 미수행 Rune 생성/예약은 명시적으로 거부한다. (2026-11-10 WB04-E219)
const forbidden = () => { throw new Error('내용물 알림의 미선언 Rune 효과'); };

// 한 입력의 풀/해시/프레임/전역을 새로 만든 장면이다(이전 결과를 쓰지 않음). input 은 { exists, first, kind, local, owner, reason, word }. (2026-11-10 WB04-E219)
function noticeScene(input) {
    const pool = new SidPool(24000, { server: true });
    const hash = new SquidHash();
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    const slots = [];
    // 비교 슬롯 외의 raw 도 원본 입력처럼 zero 로 둔다.
    for (let sid = 5; sid <= 11; sid += 1) {
        assert.equal(pool.allocate(2), sid);
        slots[sid] = pool.allocated(sid);
        slots[sid].fill(0);
    }
    // 실제 입력 체인의 Dais→사제→제단 삽입 순서를 그대로 준비한다.
    const insert = (sid, type) => {
        const raw = slots[sid];
        put(raw, 0, COMMON_VTABLE);
        raw[10] = type;
        put(raw, 14, floatToBits(40.75));
        put(raw, 18, floatToBits(41.25));
        put(raw, 4, hash.bucket(1, 40.75, 41.25), 2);
        hash.setBucket(1, 40.75, 41.25, sid);
        raw[33] = 1;
        raw[34] = 1;
    };
    insert(5, 165);
    insert(6, 158);
    insert(7, 164);
    put(slots[5], 0, PATCH_DAIS_VTABLE);
    put(slots[7], 0, PATCH_ALTAR_VTABLE);
    put(slots[5], 36, 14, 4);
    put(slots[7], 36, 13, 4);
    put(slots[7], 12, 2, 2);
    slots[5][34] = input.owner;
    types[165].footX = types[165].footY = 3;
    types[164].footX = types[164].footY = 1;
    types[158].footX = types[158].footY = 1;
    if (!input.exists) hash.setBucket(1, 40.75, 41.25, get(slots[7], 4, 2));
    put(slots[10], 0, COMMON_VTABLE);
    slots[10][10] = 6;
    put(slots[10], 12, input.word, 2);
    put(slots[10], 18, input.kind, 2);
    put(slots[10], 20, 36, 2);
    // 현재 타입의 실제 첫 표는 입력 코드에서 만들며 기대 프레임을 직접 계산하지 않는다.
    const pattern = 'A'.repeat(input.first) + 'O'.repeat(4) + 'P'.repeat(8);
    const frames = types.map(() => new FrameCodeTable([]));
    frames[164] = new FrameCodeTable(Array.from(pattern, (letter) => ({ side: letter.charCodeAt(0), variant: 80, flags: 0 })));
    const book = new SquidPostPopState();
    const base = new SquidPostPop(pool, types, book);
    const rune = new RawRunePostPop(pool, hash, types, new RunePostPopState(), base, { reserveRegular: forbidden, addRegular: forbidden, startConstruction: forbidden });
    const effects = [];
    const state = { priestType: () => 158, localOwner: () => input.local, cue: () => 99, bridgeType: () => 82 };
    const hooks = {
        unpop: (sid, flags) => { assert.ok(sid === 7 && flags === 0); effects.push('unpop:7,0'); },
        repop: (sid, flags) => { assert.ok(sid === 7 && flags === 0); effects.push('repop:7,0'); },
        stopVoice: (cue, zero, flags) => { assert.ok(cue === 99 && zero === 0 && flags === 0); effects.push('V:99,0,0'); },
        notifySurface: () => { throw new Error('현재 제단 타입은 다리 WORD 표면 통지 대상이 아니다'); },
    };
    return { pool, frames, rune, state, hooks, effects, notice: new RawDaisContentNotice(pool, frames, rune, state, hooks) };
}

test('다이스 내용물 알림: 원본 10.78 부착/제거의 plain 프레임·WORD·현재 cue 와 세 슬롯 raw 가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.ok(rows.length > 0);
    let checked = 0;
    for (const row of rows) {
        assert.equal(row.length, 4);
        const input = JSON.parse(row[1]);
        const scene = noticeScene(input);
        scene.notice.notice(5, 10, input.reason);
        const label = `${row[1]} (#${checked})`;
        assert.equal(scene.effects.length === 0 ? '-' : scene.effects.join(';'), row[2], `${label} 요청`);
        const images = row[3].split(';');
        assert.equal(images.length, 3);
        // 주입한 실제 VT 주소 DWORD 외의 raw 전체를 독립 원본과 비교한다.
        [5, 7, 10].forEach((sid, index) => {
            const image = hex(scene.pool.slot(sid));
            assert.equal(image.length, images[index].length);
            assert.equal(image.slice(8), images[index].slice(8), `${label} 슬롯 ${sid}`);
        });
        checked += 1;
    }
});

test('다이스 내용물 알림 계약: 생성자 VT 가 아닌 부모와 다른 풀/프레임 표 크기를 거부하고 reason 2 는 자연 반환이다', () => {
    const scene = noticeScene({ exists: 1, first: 3, kind: 158, local: 1, owner: 1, reason: 1, word: 2 });
    // 일반 body 로 대신하지 않는다.
    put(scene.pool.allocated(5), 0, COMMON_VTABLE);
    assert.ok(!scene.notice.handles(5));
    assert.ok(throws(() => scene.notice.notice(5, 10, 1)));
    put(scene.pool.allocated(5), 0, PATCH_DAIS_VTABLE);
    assert.ok(scene.notice.handles(5));
    // 제거(reason 2)는 아무 효과도 없다.
    const before = hex(scene.pool.bytes());
    scene.notice.notice(5, 10, 2);
    assert.deepEqual(scene.effects, []);
    assert.equal(hex(scene.pool.bytes()), before);
    // 다른 사제 kind 는 reason 1 이어도 자연 반환이다.
    put(scene.pool.allocated(10), 18, 157, 2);
    scene.notice.notice(5, 10, 1);
    assert.deepEqual(scene.effects, []);
    // 다른 풀의 rune/프레임 크기는 구성에서 거부한다.
    const other = noticeScene({ exists: 1, first: 3, kind: 158, local: 1, owner: 1, reason: 1, word: 2 });
    assert.ok(throws(() => new RawDaisContentNotice(scene.pool, scene.frames, other.rune, scene.state, scene.hooks)));
    assert.ok(throws(() => new RawDaisContentNotice(scene.pool, scene.frames.slice(0, 171), scene.rune, scene.state, scene.hooks)));
});

test('다이스 내용물 알림 계약: 선택된 장치가 없으면 그 위치에서 거부하고 앞선 쓰기는 보존한다', () => {
    const scene = noticeScene({ exists: 1, first: 3, kind: 158, local: 1, owner: 1, reason: 1, word: 5 });
    const missing = new RawDaisContentNotice(scene.pool, scene.frames, scene.rune, scene.state, { unpop: scene.hooks.unpop });
    assert.ok(throws(() => missing.notice(5, 10, 1)));
    // unpop 은 불렸고, 프레임('O' 첫 칸 3)과 WORD(5)는 repop 앞이라 이미 제단 raw 에 반영되었다(원본 순서). repop/stopVoice 는 불리지 않는다.
    assert.deepEqual(scene.effects, ['unpop:7,0']);
    assert.equal(get(scene.pool.slot(7), 36), 3);
    assert.equal(get(scene.pool.slot(7), 12, 2), 5);
    // unpop 이 없으면 아무것도 쓰기 전에 거부한다.
    const none = noticeScene({ exists: 1, first: 3, kind: 158, local: 1, owner: 1, reason: 1, word: 5 });
    const before = hex(none.pool.bytes());
    assert.ok(throws(() => new RawDaisContentNotice(none.pool, none.frames, none.rune, none.state, {}).notice(5, 10, 1)));
    assert.equal(hex(none.pool.bytes()), before);
});
