// 오벨리스크 Regular raw-buried-regular.js(RawBuriedRegular)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: buried-regular-x86.tsv(10.78 전용) 384행 — 사건 0(애니메이션 288)과 기본 사건(96)의 입력 10열(사건·개수·프레임·payload·B 구간 수·난수 시드·QA 등) → 슬롯 50 raw 전체, 반환 float 비트, 게임 난수 상태,
//    실제 SquidFrame 프레임 지정이 부른 표시 갱신 순서(`U:현재 프레임`).
//  - 계약: BuriedRegular_RejectsForeignBindingsAndMissingParent(실제 가상 표/같은 풀/판본을 검증하고 다른 부모의 누락을 성공으로 처리하지 않음).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E75: 오벨리스크 Regular 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { RawBuriedRegular } from '../www/core/raw-buried-regular.js';

// 원본 관측 행이다(단일 판본 파일). 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E75)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'buried-regular-x86.tsv'));

// 관찰 SID, 애니메이션 입력 수와 기본 사건 입력 수이다. (2026-11-08 WB04-E75)
const SOURCE = 50;
const ANIMATION_ROWS = 288;
const BASE_ROWS = 96;

// 예외가 나는지 본다. (2026-11-08 WB04-E75)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E75)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 실제 구간 표 생성기를 사용해 A 8 / B 가변 / C 나머지인 48 프레임 입력을 만든다. (2026-11-08 WB04-E75)
function buriedCodes(count) {
    const codes = [];
    // 입력한 구간별 코드이며 결과 프레임은 계산하지 않는다.
    for (let index = 0; index < 48; index += 1) codes.push({ side: index < 8 ? 0x41 : index < 8 + count ? 0x42 : 0x43, variant: 0x41, flags: 0 });
    return new FrameCodeTable(codes);
}

// 원본 입력과 동일한 단일 풀/프레임 지정기/난수의 검사용 조립체이다. (2026-11-08 WB04-E75)
class BuriedScene {
    constructor(values) {
        this.pool = new SidPool(24000, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
        this.frames = Array.from({ length: 188 }, () => buriedCodes(8));
        this.random = new GameRandom();
        this.input = values.split(',').map(Number);
        assert.equal(this.input.length, 10);
        this.trace = [];
        this.updates = 0;
        this.setter = new SquidFrame(this.pool, this.types, {
            frameSize: () => [1, 1],
            update: (sid, flags) => {
                assert.equal(flags, 0);
                this.trace.push(`U:${get(this.pool.slot(sid), 36)}`);
                this.updates += 1;
                if (this.updates === 2 && this.input[9]) {
                    this.random.setState(0x87654321);
                    put(this.pool.allocated(sid), 12, 0xbeef, 2);
                }
            },
            unpop: () => { throw new Error('오벨리스크 합성 입력은 저장 단계1입니다'); },
            pop: () => { throw new Error('오벨리스크 합성 입력은 재Pop하지 않습니다'); }
        });
        // 실제 번호 50까지 할당한다.
        for (let sid = 5; sid <= SOURCE; sid += 1) assert.equal(this.pool.allocate(2), sid);
        const raw = this.pool.allocated(SOURCE);
        raw.fill(0xa5);
        put(raw, 0, 0x50a748);
        put(raw, 4, 0, 2);
        put(raw, 6, 0, 2);
        put(raw, 8, 0, 2);
        raw[10] = 112;
        raw[11] = 0;
        put(raw, 12, this.input[8], 2);
        put(raw, 14, floatToBits(50.25));
        put(raw, 18, floatToBits(52.5));
        raw[33] = 1;
        put(raw, 36, this.input[2]);
        this.frames[112] = buriedCodes(this.input[5]);
        this.random.setState(this.input[6]);
        this.body = new RawBuriedRegular(this.pool, this.frames, this.setter, this.random);
    }
}

// 사건 0 과 기본 사건은 독립 검사에서 같은 fixture 의 전수 결과를 읽는다. (2026-11-08 WB04-E75)
function compare(animated, expected) {
    let count = 0;
    // 결과를 재계산하지 않고 원본에서 기록한 비트와 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 5);
        const event = Number(row[0].split(',')[0]);
        if ((event === 0) !== animated) continue;
        const scene = new BuriedScene(row[0]);
        const result = scene.body.handle(SOURCE, scene.input[0], scene.input[1], bitsToFloat(scene.input[7]));
        assert.equal(hex(scene.pool.slot(SOURCE)), row[1], `입력 ${row[0]}`);
        assert.equal(floatToBits(result), Number(row[2]), `입력 ${row[0]}`);
        assert.equal(scene.random.state, Number(row[3]), `입력 ${row[0]}`);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), row[4], `입력 ${row[0]}`);
        count += 1;
    }
    assert.equal(count, expected);
}

test('BuriedRegular: 원본 10.78 애니메이션 288입력의 signed 횟수·현재 난수·실제 표시 변경이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    compare(true, ANIMATION_ROWS);
});

test('BuriedRegular: 원본 10.78 기본 사건 96입력은 QA 범위와 signed count 에 무관하게 float 를 그대로 반환한다', { skip: !rows && '원본 fixture 없음' }, () => {
    compare(false, BASE_ROWS);
});

test('BuriedRegular: 실제 가상 표/같은 풀/판본을 검증하고 다른 부모의 누락을 성공으로 처리하지 않는다', () => {
    const scene = new BuriedScene('0,0,0,0,8,8,0,0,114,0');
    assert.ok(scene.body.handles(SOURCE));
    const raw = scene.pool.allocated(SOURCE);
    put(raw, 0, 0x50f540);
    assert.ok(throws(() => scene.body.handle(SOURCE, 0, 0, 0)));
    assert.ok(throws(() => scene.body.handler()(SOURCE, 0, 0, 0)));
    assert.equal(scene.body.handler((sid, event, count, payload) => payload)(SOURCE, 9, 0, 3.0), 3.0);
    const other = new BuriedScene('0,0,0,0,8,8,0,0,114,0');
    assert.ok(throws(() => new RawBuriedRegular(scene.pool, scene.frames, other.setter, scene.random)));
    assert.ok(throws(() => new RawBuriedRegular(scene.pool, scene.frames.slice(1), scene.setter, scene.random)));
});
