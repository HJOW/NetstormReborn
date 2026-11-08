// 벌룬 그림자 몸체 raw-balloon-body.js(RawBalloonBody)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: balloon-body-x86.tsv 233행 + balloon-body-height-x86.tsv 256행(두 x87 정밀도 동일) — 입력 11 DWORD(PreDestroy/PostPop·flags·부모 프레임·그림자 WORD·TYPE flags1·높이·boss·지도 비트·외부 변이 종류·내용물 연결·좌표 0 여부)
//    → 외부 효과 순서(C 생성/P Pop/U Unpop/K 전용 check/X 삭제/B 기본 후처리/D Damageable 전처리/R 내용물 발견), 부모와 두 그림자의 전체 150바이트 raw, 현재 `boss,프레임 수`. 원문 안의 finder 내부 표식 `F:` 는 외부 효과가 아니므로 제외한다.
//  - 계약: FortObjectSaveBalloon_RobustCurrentFrameAndType(저장 가상 60 은 signed 프레임을 먼저 읽고 현재 TYPE 의 그림 개수와 비교).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E94: 벌룬 그림자 몸체 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { CarrierPostPopState, RawCarrierPostPop } from '../www/core/raw-carrier-post-pop.js';
import { RawCarrierPreDestroy } from '../www/core/raw-carrier-pre-destroy.js';
import { RawBalloonBody } from '../www/core/raw-balloon-body.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E94)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const bodyRows = readRows(path.join(fixtures, 'balloon-body-x86.tsv'));
const heightRows = readRows(path.join(fixtures, 'balloon-body-height-x86.tsv'));
const ROWS = 489;

// 예외가 나는지 본다. (2026-11-08 WB04-E94)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E94)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원문 안의 실제 finder 호출 표식은 JS 의 기존 실제 finder 외부 효과와 구별한다. (2026-11-08 WB04-E94)
function external(text) {
    const parts = text.split(';').filter((part) => part !== '-' && !part.startsWith('F:'));
    return parts.length === 0 ? '-' : parts.join(';');
}

const freshTypes = () => Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));

test('BalloonBody: 원본 10.78 489입력(그림자 233+고도 256)의 그림자 생성/Pop/표시 깊이/삭제 효과 순서·전체 150바이트 raw·현재 boss/프레임 수가 같다', { skip: (!bodyRows || !heightRows) && '원본 fixture 없음' }, () => {
    let count = 0;
    // 기존 233 원문과 별도 256 고도 원문을 모두 읽는다.
    for (const row of [...bodyRows, ...heightRows]) {
        assert.equal(row.length, 7);
        assert.ok(row[1] === row[4] && row[2] === row[5] && row[3] === row[6]);
        const a = row[0].split(',').map(Number);
        assert.equal(a.length, 11);
        const pool = new SidPool(32768);
        const types = freshTypes();
        types[111].flags1 = a[4];
        const spots = new Uint8Array(65536).fill(a[7]);
        const contained = new ContainedFinderState();
        contained.boss = a[6] !== 0;
        const postState = new CarrierPostPopState();
        postState.loadingDepth = 0;
        let frames = 4;
        const events = [];
        let pops = 0;
        const event = (name, values = []) => events.push(values.reduce((text, value) => `${text}:${value >>> 0}`, name));
        // 서로 다른 부모/두 그림자/실제 내용물 Form 의 원문 입력이다.
        for (const sid of [50, 51, 52, 53]) {
            const raw = pool.physical(sid);
            raw.fill(0);
            put(raw, 0, 0x5040c0);
            raw[10] = 111;
            raw[35] = 17;
            raw[40] = sid === 50 ? 0x10 : 2;
            put(raw, 14, floatToBits(sid === 50 && a[10] ? 0.0 : 98.0));
            put(raw, 18, floatToBits(122.0));
            put(raw, 36, sid === 50 ? a[2] : 7);
            if (sid === 50) { put(raw, 6, a[9], 2); put(raw, 8, a[3], 2); raw[30] = a[5]; }
            if (sid === 53) { raw[10] = 6; put(raw, 18, 111, 2); put(raw, 20, 1, 2); put(raw, 22, 0, 2); put(raw, 24, 50, 2); }
        }
        const parent = pool.physical(50);
        const post = new RawCarrierPostPop(pool, types, postState, {
            validateBase: () => {},
            base: (sid, flags) => event('B', [sid, flags]),
            carrierCheck: () => { throw new Error('별도 Carrier check를 호출하면 오류'); },
            claimGround: () => { throw new Error('loading0의 지면 주장 오류'); }
        });
        const pre = new RawCarrierPreDestroy(pool, contained, {
            validateDamageable: () => {},
            damageablePre: (sid, flags) => { event('D', [sid, flags]); if (a[8] === 3) contained.boss = false; },
            dependentFound: () => event('R')
        });
        const hooks = {
            frames: (type) => { assert.equal(type, 111); return frames; },
            create: (type, flags) => { event('C', [type, flags]); if (a[8] === 1) { put(parent, 36, 1); frames = 5; } return a[3] ? 52 : 51; },
            pop: (sid, x, y, flags) => {
                event('P', [sid, floatToBits(x), floatToBits(y), flags]);
                pops += 1;
                if (a[8] === 2 && pops === 1) { put(parent, 14, floatToBits(98.25)); put(parent, 18, floatToBits(121.75)); parent[30] = 30; }
                if (a[8] === 4 && pops === 2) put(parent, 8, 52, 2);
            },
            unpop: (sid, flags) => {
                event('U', [sid, flags]);
                if (a[8] === 3) { put(parent, 14, floatToBits(13.5)); put(parent, 18, floatToBits(23.25)); parent[40] = 16; frames = 6; }
            },
            check: (sid) => { event('K', [sid]); if (a[8] === 5) contained.boss = true; },
            destroy: (sid, flags) => {
                event('X', [sid, flags]);
                pool.physical(sid)[11] = 1;
                if (a[8] === 1) put(parent, 8, 0, 2);
                if (a[8] === 2) contained.boss = false;
            }
        };
        const body = new RawBalloonBody(pool, types, spots, () => contained.boss, post, pre, hooks);
        if (a[0]) body.preDestroy(50, a[1]); else body.postPop(50, a[1]);
        const raw = [50, 51, 52].map((sid) => hex(pool.slot(sid))).join('');
        const current = `${contained.boss ? 1 : 0},${frames}`;
        const label = `행 ${count}: 입력 ${row[0]}`;
        assert.equal(events.length === 0 ? '-' : events.join(';'), external(row[1]), label);
        assert.equal(raw, row[2], label);
        assert.equal(current, row[3], label);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('BalloonBody: 저장 검사는 signed 프레임을 먼저 읽고 현재 TYPE 의 그림 개수와 비교하며 클라우드 Regular 는 event 3 만 진행한다', () => {
    const pool = new SidPool(32768);
    const types = freshTypes();
    const spots = new Uint8Array(65536);
    const contained = new ContainedFinderState();
    const post = new RawCarrierPostPop(pool, types, new CarrierPostPopState(), { carrierCheck: () => { throw new Error('Robust에서 Carrier check 호출 오류'); }, claimGround: () => { throw new Error('Robust에서 지면 주장 호출 오류'); }, validateBase: () => { throw new Error('Robust에서 ValidateBase 호출 오류'); }, base: () => { throw new Error('Robust에서 base 호출 오류'); } });
    const pre = new RawCarrierPreDestroy(pool, contained, { validateDamageable: () => {}, damageablePre: () => {}, dependentFound: () => {} });
    const raw = pool.physical(50);
    raw.fill(0);
    put(raw, 0, 0x5040c0);
    raw[10] = 111;
    let expected = 111;
    let amount = 4;
    let mutate = false;
    const body = new RawBalloonBody(pool, types, spots, () => contained.boss, post, pre, {
        frames: (type) => { assert.equal(type, expected); if (mutate) put(raw, 36, 999); return amount; }
    });
    put(raw, 36, 3);
    assert.ok(body.robust(50));
    put(raw, 36, 4);
    assert.ok(!body.robust(50));
    put(raw, 36, 0xffffffff);
    assert.ok(body.robust(50));
    put(raw, 36, 3);
    mutate = true;
    assert.ok(body.robust(50) && get(raw, 36) === 999);
    raw[10] = 146;
    expected = 146;
    put(raw, 36, 3);
    assert.ok(body.robust(50));
    put(raw, 0, 0x504198);
    put(raw, 36, 3);
    assert.ok(body.robust(50));
    amount = -2;
    put(raw, 36, 0xffffffff);
    assert.ok(!body.robust(50));
    // 클라우드 플로터 전용 Regular: event 3 만 진행(payload 반환), 그 밖은 기본 Regular 위임, 다른 가상 표는 거부한다.
    const advanced = [];
    assert.equal(body.cloudRegular(50, 3, 7, 0.5, (...args) => advanced.push(args), () => 99), 0.5);
    assert.deepEqual(advanced, [[50, 1, 0]]);
    assert.equal(body.cloudRegular(50, 4, 7, 0.5, null, (sid, event, count, payload) => payload + event + count), 11.5);
    put(raw, 0, 0x5040c0);
    assert.ok(throws(() => body.cloudRegular(50, 3, 0, 0, () => {}, () => 0)));
    assert.ok(throws(() => new RawBalloonBody({ edition: 'cd1072' }, types, spots, () => true, post, pre, {})));
    assert.ok(throws(() => new RawBalloonBody(pool, types, spots.subarray(0, 10), () => true, post, pre, {})));
});
