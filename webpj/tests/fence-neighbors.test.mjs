// 썬 바리케이드 이웃 검색 raw-fence-neighbors.js(RawFenceNeighbors)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: fence-neighbors-x86.tsv 1904행 — 부모+네 후보의 전체 250바이트·후보 순서·발자국·소유자 관계(동맹 방향)·끝점 입력 → 최근접 선택 결과 SID 와 검색 경계 네 float 비트(N 행) 또는 축 사거리 판정(W 행), 조회가 입력 객체를 바꾸지 않았는지 전체 250바이트 재대조.
//  - 계약: FenceNeighbors_CurrentHashDirectionsAndReverseAlliance(실제 해시의 방향 분리·가까운 적대를 건너뛰어 먼 동맹으로 대체하지 않음·반대 방향 동맹·연결 수의 extra/state 영향).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E88: 썬 바리케이드 이웃 검색 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { RawFenceNeighbors } from '../www/core/raw-fence-neighbors.js';

// 원본 관측 행이다(머리 행 제외). 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E88)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fence-neighbors-x86.tsv'))?.filter((row) => row[0] !== 'input') ?? null;

// 독립 입력 수와 부모/후보 SID 범위이다. (2026-11-08 WB04-E88)
const ROWS = 1904;
const FIRST = 50;
const LAST = 54;

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E88)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 입력 raw 다섯 슬롯과 현재 TYPE/동맹/해시를 같은 제품 객체에 공급하는 장면이다. (2026-11-08 WB04-E88)
class FenceNeighborScene {
    // 슬롯 번호 50..54 까지 실제로 확보한다.
    constructor() {
        this.pool = new SidPool(24000, { server: false });
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, () => ({ footX: 0, footY: 0 }));
        this.ranges = new Array(188).fill(0);
        this.alliances = new Array(81).fill(0);
        this.body = new RawFenceNeighbors(this.pool, this.hash, this.types, this.ranges, this.alliances);
        // 관찰 입력을 실제 할당 슬롯에만 쓴다.
        for (let sid = 5; sid <= LAST; sid += 1) assert.equal(this.pool.allocate(2), sid);
    }

    // 독립 관찰기에 보존한 전체 250 바이트와 현재 타입/동맹만 준비한다(free/dead 상태 입력은 물리 슬롯에 쓴다).
    setup(row, input) {
        assert.equal(row[1].length, 500);
        for (let number = FIRST; number <= LAST; number += 1) {
            const raw = this.pool.physical(number);
            // 원문 보존 바이트만 읽는다.
            for (let offset = 0; offset < 50; offset += 1) raw[offset] = parseInt(row[1].substr(((number - FIRST) * 50 + offset) * 2, 2), 16);
        }
        // 원문 두 TYPE 의 물리 입력이다.
        for (const type of [135, 136]) {
            this.types[type].footX = Number(row[3]) | 0;
            this.types[type].footY = Number(row[4]) | 0;
            this.ranges[type] = bitsToFloat(Number(input[3]));
        }
        if (Number(row[5]) === 1) this.alliances[2 * 9 + 1] = 1;
        if (Number(row[5]) === 2) this.alliances[1 * 9 + 2] = 1;
    }

    // 실제 해시에 새 현재 좌표를 등록한다. 기대 후보 순서는 계산하지 않는다.
    place(sid, x, y, owner) {
        const raw = this.pool.physical(sid);
        raw.fill(0);
        raw[10] = 136;
        raw[34] = owner;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        put(raw, 4, this.hash.bucket(1, x, y), 2);
        this.hash.setBucket(1, x, y, sid);
    }
}

test('FenceNeighbors: 원본 10.78 1904입력의 최근접 선택·동맹·float 검색 경계·축 사거리 판정이 같고 입력 객체를 바꾸지 않는다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 이전 원문 행의 결과를 다음 입력에 재사용하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 9);
        const input = row[0].split(',');
        assert.equal(input.length, 8);
        const scene = new FenceNeighborScene();
        scene.setup(row, input);
        const label = `행 ${count}: 입력 ${row[0]}`;
        if (input[0] === 'N') {
            // 중복/자기 자신도 원문에 공급한 대로 유지한다.
            const candidates = row[2].split(',').map(Number);
            assert.equal(scene.body.select(FIRST, candidates, Number(input[2]) !== 0), Number(row[7]), label);
            const bounds = scene.body.searchBounds(FIRST, Number(input[1]));
            assert.deepEqual(bounds.map(floatToBits), row[8].split(',').map(Number), label);
        } else {
            const points = row[6].split(',').map((value) => bitsToFloat(Number(value)));
            assert.equal(points.length, 4);
            assert.equal(scene.body.withinRange(FIRST, points), Number(row[7]) !== 0, label);
        }
        // 조회가 입력 객체를 바꾸지 않았는지 전체 250 바이트를 대조한다.
        let current = '';
        for (let number = FIRST; number <= LAST; number += 1) current += hex(scene.pool.slot(number));
        assert.equal(current, row[1], label);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('FenceNeighbors: 실제 해시는 방향을 나누며 가까운 적대를 건너뛰어 먼 동맹으로 대체하지 않고 반대 방향 동맹/연결 수 조건을 따른다', () => {
    const scene = new FenceNeighborScene();
    scene.types[136].footX = 1;
    scene.types[136].footY = 1;
    scene.ranges[136] = 8.5;
    scene.place(50, 50, 50, 1);
    scene.place(51, 50, 46, 2);
    scene.place(52, 54, 50, 2);
    scene.place(53, 50, 54, 2);
    scene.place(54, 46, 50, 2);
    // 실제 네 방향은 서로 다른 해시 후보를 골라야 한다.
    for (let direction = 0; direction < 8; direction += 2) {
        assert.equal(scene.body.neighbor(FIRST, direction), 51 + direction / 2);
        assert.equal(scene.body.neighbor(FIRST, direction, true), 0);
    }
    scene.alliances[1 * 9 + 2] = 1;
    assert.equal(scene.body.connections(FIRST), 0);
    scene.alliances[2 * 9 + 1] = 1;
    assert.equal(scene.body.connections(FIRST), 4);
    scene.pool.physical(51)[40] = 0x20;
    assert.equal(scene.body.connections(FIRST), 4);
    scene.pool.physical(51)[11] = 2;
    assert.ok(scene.body.neighbor(FIRST, 0) === 0 && scene.body.connections(FIRST) === 3);
    scene.pool.physical(51)[11] = 1;
    assert.equal(scene.body.neighbor(FIRST, 0), 51);
    const nearer = scene.pool.allocate(2);
    assert.equal(nearer, 55);
    scene.place(nearer, 50, 48, 3);
    assert.ok(scene.body.neighbor(FIRST, 0) === 55 && scene.body.neighbor(FIRST, 0, true) === 0 && scene.body.connections(FIRST) === 3);
    scene.pool.physical(FIRST)[40] = 8;
    assert.equal(scene.body.connections(FIRST), 0);
    assert.throws(() => scene.body.searchBounds(FIRST, 8));
    assert.throws(() => scene.body.select(0, [51], false));
    assert.throws(() => new RawFenceNeighbors({ edition: 'cd1072' }, scene.hash, scene.types, scene.ranges, scene.alliances));
    assert.throws(() => new RawFenceNeighbors(scene.pool, scene.hash, scene.types, scene.ranges, scene.alliances.slice(1)));
});
