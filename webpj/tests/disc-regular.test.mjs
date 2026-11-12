// 썬 디스크 가상 Handle raw-disc-regular.js(RawDiscRegular)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측 1: disc-regular-x86.tsv 의 originals 96행 — 현재 프레임·프레임 수·방향 수·event/count(미사용)·payload float 비트 입력에 대해 다음 프레임(방향당 프레임 modulo)과 반환 payload 비트·Frame.Set 의 표시 갱신 두 번이 같다
//    (cpppj DiscRegular_PatchNative 이식).
//  - 원본 관측 2: disc-regular-boundary-1219-x86.tsv 70행 — 정상 47행은 signed IDIV 세 번과 DWORD 감김의 다음 프레임, 하드웨어 나눗셈 오류 23행(0 분모·INT_MIN ÷ −1 포함)은 거부
//    (cpppj ClientFrameRegular_DiscNativeIntegerBoundaryGolden 이식, nextFrameNative).
//  - 웹 계약: 생성자 검사, nextFrame 의 범위 검사 네 갈래, handles/handle/handler 분배, TYPE getter 와 살아 있는 슬롯 읽기, Native 산술 선택.
// 10.72(CD)는 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E364: 7-P2 RawDiscRegular 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { RawDiscRegular } from '../www/core/raw-disc-regular.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E364)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const rows = readRows(path.join(fixtures, 'disc-regular-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;
const boundaryRows = readRows(path.join(fixtures, 'disc-regular-boundary-1219-x86.tsv'));

// 썬 디스크 가상 표와 int32 한계이다. (2026-11-13 WB04-E364)
const DISC_TABLE = 0x50b570;
const INT_MIN = -2147483648;
const INT_MAX = 2147483647;

// 예외가 나는지 본다. (2026-11-13 WB04-E364)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 188칸 TYPE 입력과 방향 자료, 같은 해시 단계에서 표시 갱신만 세는 프레임 지정기를 묶은 장면이다(cpppj CheckDiscRegular). (2026-11-13 WB04-E364)
function makeScene() {
    const pool = new SidPool(24000, { server: false });
    const object = pool.allocate(2);
    const types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0 }));
    const directions = Array.from({ length: 188 }, () => ({ directions: 0, frameCount: 0 }));
    const scene = { pool, object, types, directions, updates: 0 };
    // 명시 외부 SHP 입력과 표시 요청 관찰이다. 해시 단계가 같은 입력이라 Unpop/Pop 은 부르지 않는다.
    scene.setter = new SquidFrame(pool, types, {
        frameSize: () => [1, 1],
        update: (sid, flags) => { assert.equal(flags, 0); scene.updates += 1; },
        unpop: () => { throw new Error('동일 해시 단계 입력에서 예상 밖 Unpop'); },
        pop: () => { throw new Error('동일 해시 단계 입력에서 예상 밖 Pop'); },
    });
    scene.logic = new RawDiscRegular(pool, directions, scene.setter);
    // 썬 디스크 슬롯(가상 표·TYPE 83·상태 0·플래그 1)과 현재 프레임을 준비한다.
    scene.prepare = (frame) => {
        const raw = pool.allocated(object);
        put(raw, 0, DISC_TABLE);
        raw[10] = 83;
        raw[11] = 0;
        raw[33] = 1;
        put(raw, 36, frame >>> 0, 4);
        scene.updates = 0;
    };
    return scene;
}

test('DiscRegular: 원본 10.78 96입력의 다음 프레임(방향당 modulo)·반환 payload 비트·표시 갱신 두 번이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    let count = 0;
    // 새 독립 원본 행을 같은 풀/지정기에 재생한다.
    for (const row of rows) {
        const current = Number(row[1]) | 0;
        scene.prepare(current);
        scene.directions[83] = { directions: Number(row[3]) | 0, frameCount: Number(row[2]) | 0 };
        assert.equal(RawDiscRegular.nextFrame(current, Number(row[2]) | 0, Number(row[3]) | 0), Number(row[7]) | 0);
        const returned = scene.logic.handle(scene.object, Number(row[4]), Number(row[5]), bitsToFloat(Number(row[6])));
        assert.equal(floatToBits(returned), Number(row[9]));
        assert.equal(get(scene.pool.slot(scene.object), 36, 4), Number(row[7]) >>> 0);
        assert.deepEqual([Number(row[8]), scene.updates], [0, 2]);
        count += 1;
    }
    assert.equal(count, 96);
});

test('DiscRegular: 원본 10.78 경계 70입력의 signed IDIV 세 번과 DWORD 감김 다음 프레임이 같고 나눗셈 오류 23건은 거부한다(nextFrameNative)', { skip: !boundaryRows && '원본 fixture 없음' }, () => {
    let count = 0;
    let faults = 0;
    // 원문 정상 47/IDIV 오류 23 모두 같은 입력으로 대조한다.
    for (const row of boundaryRows) {
        assert.equal(row.length, 8);
        const [current, frames, directions] = [Number(row[0]) | 0, Number(row[1]) | 0, Number(row[2]) | 0];
        if (row[3] === 'divide-error') {
            assert.throws(() => RawDiscRegular.nextFrameNative(current, frames, directions), RangeError, `행 ${count}`);
            faults += 1;
        } else {
            assert.equal(RawDiscRegular.nextFrameNative(current, frames, directions) >>> 0, Number(row[4]), `행 ${count}`);
            assert.deepEqual([Number(row[5]), Number(row[6])], [0, 0x3d800000]);
        }
        count += 1;
    }
    assert.deepEqual([count, faults], [70, 23]);
});

test('DiscRegular 생성자: PATCH 풀·같은 풀의 지정기·188칸 TYPE 표만 받는다', () => {
    const scene = makeScene();
    assert.throws(() => new RawDiscRegular({ edition: 'cd1072' }, scene.directions, scene.setter), TypeError);
    assert.throws(() => new RawDiscRegular(scene.pool, scene.directions, new SquidFrame(new SidPool(24000, { server: false }), scene.types, { frameSize: () => [1, 1], update: () => {}, unpop: () => {}, pop: () => {} })), (error) => error instanceof TypeError && /풀 불일치/.test(error.message));
    assert.throws(() => new RawDiscRegular(scene.pool, scene.directions.slice(1), scene.setter), RangeError);
    assert.throws(() => new RawDiscRegular(scene.pool, null, scene.setter), RangeError);
});

test('DiscRegular nextFrame: 현재 프레임 음수·INT_MAX, 방향 수 1 미만, 프레임 수가 방향 수보다 작으면 거부하고 경계값은 허용한다', () => {
    const range = (error) => error instanceof RangeError && /나눗셈 범위 오류/.test(error.message);
    for (const [current, frames, directions] of [[-1, 4, 1], [INT_MAX, 4, 1], [0, 4, 0], [0, 4, -1], [0, 3, 4], [0, 0, 1], [INT_MIN, 4, 1]]) {
        assert.throws(() => RawDiscRegular.nextFrame(current, frames, directions), range, `${current},${frames},${directions}`);
    }
    // 프레임 수가 방향 수와 같으면(방향당 1) 허용되며 0, INT_MAX−1 도 허용된다.
    assert.equal(RawDiscRegular.nextFrame(0, 4, 4), 0);
    assert.equal(RawDiscRegular.nextFrame(7, 4, 4), 7);
    assert.equal(RawDiscRegular.nextFrame(0, 4, 1), 1);
    assert.equal(RawDiscRegular.nextFrame(3, 4, 1), 0);
    assert.equal(RawDiscRegular.nextFrame(5, 8, 2), 6);
    assert.equal(RawDiscRegular.nextFrame(7, 8, 2), 4);
    // 상한 직전: 다음 INT_MAX % 4 = 3, 현재 % 4 = 2 이므로 3 − 2 + (INT_MAX − 1) = INT_MAX.
    assert.equal(RawDiscRegular.nextFrame(INT_MAX - 1, 4, 1), INT_MAX);
});

test('DiscRegular nextFrameNative: 음수 피연산자의 부호를 피제수에 맞추고 DWORD 로 감기며 0 분모·INT_MIN ÷ −1 은 같은 오류이고 −0 을 만들지 않는다', () => {
    // 부호 있는 나머지(피제수 부호)의 손으로 계산한 결과: (−2 % 4) − (−3 % 4) + (−3) = −2 − (−3) − 3 = −2.
    assert.equal(RawDiscRegular.nextFrameNative(-3, 4, 1), -2);
    // 음수 분모: 방향당 프레임 4/−1 = −4, (6 % −4) − (5 % −4) + 5 = 2 − 1 + 5 = 6.
    assert.equal(RawDiscRegular.nextFrameNative(5, 4, -1), 6);
    // DWORD 감김: 다음이 INT_MIN 으로 감기고 (INT_MIN % 4) − (INT_MAX % 4) + INT_MAX = 0 − 3 + INT_MAX.
    assert.equal(RawDiscRegular.nextFrameNative(INT_MAX, 4, 1), INT_MAX - 3);
    // 모든 항이 0 인 입력의 결과는 −0 이 아니라 0 이다.
    assert.ok(Object.is(RawDiscRegular.nextFrameNative(0, 4, 4), 0));
    // DWORD 감김: 현재가 INT_MIN 이면 (−2147483647 % 4 = −3) − (INT_MIN % 4 = 0) + INT_MIN = −2147483651 이 int32 로 감겨 2147483645 이다.
    assert.equal(RawDiscRegular.nextFrameNative(INT_MIN, 4, 1), 2147483645);
    assert.equal(RawDiscRegular.nextFrameNative(INT_MIN, 8, 2), 2147483645);
    for (const [frames, directions] of [[4, 0], [INT_MIN, -1]]) {
        assert.throws(() => RawDiscRegular.nextFrameNative(1, frames, directions), (error) => error instanceof RangeError && /IDIV 오류/.test(error.message));
    }
    // 나머지의 분모(방향당 프레임)가 0 이어도 같은 오류이다(프레임 수 < 방향 수 → 몫 0).
    assert.throws(() => RawDiscRegular.nextFrameNative(1, 3, 4), RangeError);
});

test('DiscRegular handle/handles/handler: raw 가상 표로만 판정하고 event/count 는 쓰지 않으며 payload 를 그대로 돌려주고 다른 부모는 기존 처리기로 넘긴다', () => {
    const scene = makeScene();
    scene.prepare(2);
    scene.directions[83] = { directions: 2, frameCount: 8 };
    assert.equal(scene.logic.handles(scene.object), true);
    // 가상 표 DWORD 는 정확히 네 바이트만 읽는다: 바로 뒤 바이트(종속 chain WORD)가 비영이어도 anim 이 아닌 것이 되지 않는다.
    scene.pool.allocated(scene.object)[4] = 0x7f;
    scene.pool.allocated(scene.object)[5] = 0x01;
    assert.equal(scene.logic.handles(scene.object), true);
    scene.pool.allocated(scene.object)[4] = 0;
    scene.pool.allocated(scene.object)[5] = 0;
    // 같은 입력에서 event/count 값이 달라도 결과는 같고 payload 는 비트까지 유지된다(음수 0 포함).
    for (const [event, count, payload] of [[0, 0, 0.5], [99, 9, -0], [123, 456, 1e-30]]) {
        scene.prepare(2);
        const returned = scene.logic.handle(scene.object, event, count, payload);
        assert.equal(floatToBits(returned), floatToBits(payload));
        assert.equal(get(scene.pool.slot(scene.object), 36, 4), 3);
    }
    // 가상 표가 다르면 handles 는 거짓이고 handle 은 TypeError 이며 슬롯은 바뀌지 않는다.
    scene.pool.allocated(scene.object).set([0, 0, 0, 0]);
    const before = Uint8Array.from(scene.pool.slot(scene.object));
    assert.equal(scene.logic.handles(scene.object), false);
    assert.throws(() => scene.logic.handle(scene.object, 0, 0, 0.5), (error) => error instanceof TypeError && /가상 표 미지원/.test(error.message));
    assert.deepEqual(Uint8Array.from(scene.pool.slot(scene.object)), before);
    // handler: 썬 디스크는 직접 처리하고 그 밖은 fallback 의 인자·결과를 그대로 쓰며 fallback 이 없으면 거부한다.
    const calls = [];
    const handler = scene.logic.handler((sid, event, count, payload) => { calls.push([sid, event, count, payload]); return 7.5; });
    assert.equal(handler(scene.object, 1, 2, 0.25), 7.5);
    assert.deepEqual(calls, [[scene.object, 1, 2, 0.25]]);
    assert.throws(() => scene.logic.handler()(scene.object, 1, 2, 0.25), /Disc 외부 Regular 부모 미연결/);
    scene.prepare(2);
    assert.equal(scene.logic.handler(() => 9)(scene.object, 0, 0, 0.25), 0.25);
    assert.equal(get(scene.pool.slot(scene.object), 36, 4), 3);
    // 표 밖 TYPE(188 이상)은 TYPE 오류이고, 표 안의 마지막 TYPE(187)은 자료가 0 이라 방향 수 오류로 거부된다.
    scene.pool.allocated(scene.object)[10] = 188;
    assert.throws(() => scene.logic.handle(scene.object, 0, 0, 0.5), (error) => error instanceof RangeError && /TYPE 오류/.test(error.message));
    scene.pool.allocated(scene.object)[10] = 187;
    assert.throws(() => scene.logic.handle(scene.object, 0, 0, 0.5), (error) => error instanceof RangeError && /나눗셈 범위 오류/.test(error.message));
});

test('DiscRegular handleIn: 현재 TYPE getter 를 먼저 읽고(없으면 거부) 그 뒤의 살아 있는 슬롯 프레임에서 진행하며 Native 산술 선택에서만 범위 검사를 건너뛴다', () => {
    const scene = makeScene();
    scene.prepare(5);
    const seen = [];
    // getter 가 슬롯의 프레임을 바꾸면 다음 프레임은 바뀐 값에서 계산한다.
    const type = (number) => { seen.push(number); put(scene.pool.allocated(scene.object), 36, 6, 4); return { directions: 2, frameCount: 8 }; };
    assert.equal(RawDiscRegular.handleIn(scene.pool, scene.setter, type, scene.object, 0.75), 0.75);
    assert.deepEqual(seen, [83]);
    assert.equal(get(scene.pool.slot(scene.object), 36, 4), 7);
    // getter 가 없으면 거부하고 슬롯을 바꾸지 않는다.
    const before = Uint8Array.from(scene.pool.slot(scene.object));
    assert.throws(() => RawDiscRegular.handleIn(scene.pool, scene.setter, null, scene.object, 0.75), /TYPE getter 미연결/);
    assert.deepEqual(Uint8Array.from(scene.pool.slot(scene.object)), before);
    // 현재 프레임이 음수(DWORD 감김)이면 검사 산술은 거부하고 Native 산술은 원래 정수 연산으로 진행한다.
    scene.prepare(-1);
    const data = () => ({ directions: 2, frameCount: 8 });
    assert.throws(() => RawDiscRegular.handleIn(scene.pool, scene.setter, data, scene.object, 0.5), RangeError);
    assert.equal(RawDiscRegular.handleIn(scene.pool, scene.setter, data, scene.object, 0.5, true), 0.5);
    assert.equal(get(scene.pool.slot(scene.object), 36, 4) | 0, RawDiscRegular.nextFrameNative(-1, 8, 2));
    assert.ok(throws(() => RawDiscRegular.handleIn(scene.pool, scene.setter, () => ({ directions: 0, frameCount: 8 }), scene.object, 0.5, true)));
});
