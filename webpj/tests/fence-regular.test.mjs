// 썬 바리케이드 Regular raw-fence-regular.js(RawFenceRegular)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: fence-regular-x86.tsv 1372행(사건 0 애니메이션 A 1305·사건 1/2 위임 E 18·Damageable 위임 D 49) — 입력 12 DWORD(사건·부모 extra·프레임·소유자 캐시·연결 캐시·연결 수 둘·이웃 존재 비트·예약 성공 비트·외부 변경 종류·payload·개수)
//    → 부모+네 이웃 전체 250바이트 raw, 반환 float 비트, 외부 효과 호출 순서(N 이웃/M 표식 삭제/L 연결선 float 비트/C 연결 수/R 예약/A 사건 2 예약/F·B 위임/D Damageable/U 프레임 표시),
//    fence-event3-x86.tsv 301행(실제 원본 호출자/생성자까지 실행한 사건 3: 반환 NaN 비트·전체 250바이트·하위 순서).
//  - 계약: FenceRegular_RequiredBoundariesAndPartialWrites(선택된 하위 누락은 앞선 프레임 쓰기를 보존한 채 거부하고 사건 3 의 owner 캐시를 갱신하지 않으며 보호 bit20 은 Damageable 로 위임, 다른 가상 표는 fallback 없이 거부).
//    같은 fixture 의 실제 SquidProcessHost 재예약(NaN 시각/count 감김) 시험은 Host 재예약 단위(별도)에서 대조한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E89: 썬 바리케이드 Regular 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { RawFenceRegular } from '../www/core/raw-fence-regular.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E89)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const regularRows = readRows(path.join(fixtures, 'fence-regular-x86.tsv'))?.filter((row) => row[0] !== 'kind') ?? null;
const event3Rows = readRows(path.join(fixtures, 'fence-event3-x86.tsv'));

// 관찰 부모 SID, 종류별 독립 입력 수, 사건 3 입력 수이다. (2026-11-08 WB04-E89)
const SOURCE = 50;
const ANIMATION_ROWS = 1305;
const EFFECT_ROWS = 18;
const DAMAGEABLE_ROWS = 49;
const EVENT3_ROWS = 301;

// 예외가 나는지 본다. (2026-11-08 WB04-E89)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E89)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 독립 원본 입력과 같은 A~F 의 8개씩인 48프레임 코드 표이다. (2026-11-08 WB04-E89)
const fenceRegularCodes = () => new FrameCodeTable(Array.from({ length: 48 }, (_, index) => ({ side: 0x41 + Math.floor(index / 8), variant: 0x41, flags: index })));

// 선언한 명시 하위만 관찰하고 실제 부모 판단과 Frame 지정은 제품 모듈로 실행하는 장면이다. (2026-11-08 WB04-E89)
class FenceRegularScene {
    // 실제 Frame 의 앞/뒤 표시와 원본과 같은 독립 입력을 조립한다.
    constructor(values) {
        this.pool = new SidPool(24000, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
        this.frames = Array.from({ length: 188 }, fenceRegularCodes);
        this.input = values.split(',').map(Number);
        assert.equal(this.input.length, 12);
        this.trace = [];
        this.updates = 0;
        this.markers = 0;
        this.reserves = 0;
        this.connectionCalls = 0;
        this.setter = new SquidFrame(this.pool, this.types, {
            frameSize: () => [1, 1],
            update: (sid, flags) => {
                assert.equal(flags, 0);
                this.event(`U:${get(this.pool.slot(sid), 36)}`);
                this.updates += 1;
                if (this.updates === 2 && this.input[9] === 1) this.pool.allocated(SOURCE)[34] = 7;
            },
            unpop: () => { throw new Error('바리케이드 합성 입력은 저장 단계1입니다'); },
            pop: () => { throw new Error('바리케이드 합성 입력은 재Pop하지 않습니다'); }
        });
        // 실제 번호를 확보한다.
        for (let sid = 5; sid <= 54; sid += 1) assert.equal(this.pool.allocate(2), sid);
        // 전체 다섯 입력 객체의 보존 바이트도 비교한다.
        for (let number = 50; number <= 54; number += 1) {
            const raw = this.pool.allocated(number);
            raw.fill(0xa5);
            put(raw, 0, 0x509470);
            put(raw, 4, 0, 2);
            put(raw, 6, 0, 2);
            put(raw, 8, 0, 2);
            raw[10] = 136;
            raw[11] = 0;
            put(raw, 12, 114, 2);
            put(raw, 14, floatToBits(number + 0.25));
            put(raw, 18, floatToBits(number + 0.5));
            raw[33] = 1;
            raw[34] = 1;
            raw[40] = number === SOURCE ? this.input[1] : 0;
            put(raw, 36, this.input[2]);
            put(raw, 42, this.input[3]);
            put(raw, 46, this.input[4]);
        }
        this.body = new RawFenceRegular(this.pool, this.frames, this.setter, this.hooks());
    }

    // 명시 효과를 실제 순서대로 결합한다.
    event(text) { this.trace.push(text); }

    // 실제 게임 계산을 흉내 내는 기대값 대신 원본 관찰의 같은 명시 하위 입력/변경만 공급한다.
    hooks() {
        const pool = this.pool;
        const input = this.input;
        return {
            neighbor: (sid, direction) => {
                assert.equal(sid, SOURCE);
                this.event(`N:${direction}`);
                if (input[9] === 2 && direction === 0) pool.allocated(SOURCE)[34] = 6;
                return (input[7] & (1 << (direction / 2))) ? 51 + direction / 2 : 0;
            },
            clearMarkers: (sid, other) => {
                assert.equal(sid, SOURCE);
                this.event(`M:${other}`);
                this.markers += 1;
                if (this.markers === 1 && input[9] === 3) {
                    put(pool.allocated(51), 18, floatToBits(125.0));
                    put(pool.allocated(SOURCE), 14, floatToBits(120.0));
                }
            },
            clearLine: (points) => { this.event(`L:${points.map(floatToBits).join(',')}`); },
            connections: (sid) => {
                assert.equal(sid, SOURCE);
                this.event('C');
                if (input[9] === 5) pool.allocated(SOURCE)[34] = 5;
                if (input[9] === 9) put(pool.allocated(SOURCE), 46, input[5]);
                this.connectionCalls += 1;
                return this.connectionCalls === 1 ? input[5] : input[6];
            },
            reserveRegular: () => {
                this.event('R');
                const success = (input[8] >>> this.reserves) & 1;
                this.reserves += 1;
                if (this.reserves === 1 && input[9] === 4) { put(pool.allocated(SOURCE), 46, 77); pool.allocated(SOURCE)[34] = 7; }
                return success !== 0;
            },
            addRegular: (sid, event, payload) => { this.event(`A:${event}:${sid}:${floatToBits(payload)}`); },
            refreshNeighbors: (sid) => { assert.equal(sid, SOURCE); this.event('F'); this.mutate(); },
            rebuild: (sid) => { assert.equal(sid, SOURCE); this.event('B'); this.mutate(); },
            damageable: (sid, event, count, payload) => {
                assert.equal(sid, SOURCE);
                this.event(`D:${event},${count},${floatToBits(payload)}`);
                if (input[9] === 8) put(pool.allocated(sid), 12, 0x8888, 2);
                return payload;
            }
        };
    }

    // 호출자가 명시한 외부 효과 후 변경 입력이다.
    mutate() {
        if (this.input[9] === 6) put(this.pool.allocated(SOURCE), 12, 0x7777, 2);
        if (this.input[9] === 7) put(this.pool.allocated(SOURCE), 36, 22);
    }

    // 원본 전체 250바이트와 비교한다.
    objects() { return [50, 51, 52, 53, 54].map((sid) => hex(this.pool.slot(sid))).join(''); }

    // 사건 입력으로 처리기를 실행한다.
    run() { return this.body.handle(SOURCE, this.input[0], this.input[11], bitsToFloat(this.input[10])); }
}

// 원본 fixture 의 범위별 모든 결과 비트를 비교한다. (2026-11-08 WB04-E89)
function compare(kind, expected) {
    let count = 0;
    for (const row of regularRows.filter((candidate) => candidate[0] === kind)) {
        assert.equal(row.length, 5);
        const scene = new FenceRegularScene(row[1]);
        const result = scene.run();
        const label = `${kind} 입력 ${row[1]}`;
        assert.equal(scene.objects(), row[2], label);
        assert.equal(floatToBits(result), Number(row[3]), label);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), row[4], label);
        count += 1;
    }
    assert.equal(count, expected);
}

test('FenceRegular: 원본 10.78 애니메이션 1305입력(두 독립 변경·각 할당 실패·현재 재조회)의 전체 250바이트·반환 비트·하위 순서가 같다', { skip: !regularRows && '원본 fixture 없음' }, () => {
    compare('A', ANIMATION_ROWS);
});

test('FenceRegular: 사건 1/2 위임 18행과 보호 bit20/기본 사건 Damageable 위임 49행이 같다', { skip: !regularRows && '원본 fixture 없음' }, () => {
    compare('E', EFFECT_ROWS);
    compare('D', DAMAGEABLE_ROWS);
});

test('FenceRegular: 실제 원본 호출자/생성자까지 실행한 사건 3 의 301행(반환 NaN 비트·전체 250바이트·하위 순서)이 같다', { skip: !event3Rows && '원본 fixture 없음' }, () => {
    assert.equal(event3Rows.length, EVENT3_ROWS);
    // 원문 명시 입력만 공급하고 JS 출력으로 기대값을 생성하지 않는다.
    event3Rows.forEach((row, index) => {
        assert.equal(row.length, 9);
        const scene = new FenceRegularScene(row[0]);
        put(scene.pool.allocated(SOURCE), 6, 81, 2);
        const result = scene.run();
        const label = `사건3 행 ${index}`;
        assert.equal(scene.objects(), row[2], label);
        assert.equal(floatToBits(result), Number(row[3]), label);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), row[4], label);
        if (!scene.input[1]) { assert.equal(row[7], '65535', label); assert.equal(row[8], '1', label); }
    });
});

test('FenceRegular: 선택된 하위 누락은 앞선 프레임 쓰기를 보존한 채 거부하고 사건 3 은 owner 캐시를 갱신하지 않으며 다른 가상 표를 거부한다', () => {
    const scene = new FenceRegularScene('0,0,7,1,0,0,0,0,0,0,0,0');
    const hooks = scene.hooks();
    delete hooks.connections;
    const missing = new RawFenceRegular(scene.pool, scene.frames, scene.setter, hooks);
    assert.ok(throws(() => missing.handle(SOURCE, 0, 0, 0)));
    assert.equal(get(scene.pool.slot(SOURCE), 36), 0);
    const other = scene.hooks();
    delete other.reserveRegular;
    const missingEvent3 = new RawFenceRegular(scene.pool, scene.frames, scene.setter, other);
    const cachedOwner = get(scene.pool.slot(SOURCE), 42);
    assert.ok(throws(() => missingEvent3.handle(SOURCE, 3, 0, 0)));
    assert.equal(get(scene.pool.slot(SOURCE), 42), cachedOwner);
    scene.pool.allocated(SOURCE)[40] = 32;
    assert.equal(scene.body.handle(SOURCE, 3, 0, 0.25), 0.25);
    put(scene.pool.allocated(SOURCE), 0, 0x50a748);
    assert.ok(throws(() => scene.body.handler()(SOURCE, 0, 0, 0)));
    assert.equal(scene.body.handler((sid, event, count, payload) => payload + 1)(SOURCE, 0, 0, 2), 3);
    assert.ok(throws(() => new RawFenceRegular({ edition: 'cd1072' }, scene.frames, scene.setter, {})));
    assert.ok(throws(() => new RawFenceRegular(new SidPool(24000), scene.frames, scene.setter, {})));
});
