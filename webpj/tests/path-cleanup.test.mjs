// 경로 정리 raw-path-cleanup.js(RawPathCleanup·연결 훅 어댑터)를 원본 10.78 실제 PE 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: pathcleanup-x86.tsv 의 `originals` 866행 — 경로 프로세스 삭제 통지의 공간 접두 뒤 몸체(A: 건설 helper 예약·실패, 귀환 장소의 운반 nugget 삭제, 신전 의식 요청, 환불/삭제, 실패 뒤 raw 결과)를 profile 18종·권한·죽음·사제·예약 성공 여부·
//    checking·하위 효과 뒤 권한 변경 3종·좌표 3종으로 실제 PE 에서 실행해 효과 기록 순서, 남은 helper 값, 최종 권한, 슬롯 5..11 전체 raw(가상 표 주입 4바이트 제외)가 같고 경로 이미지의 helper 외 필드는 보존된다.
//    조회 행(Q: 귀환 장소/근처 타입 finder)은 반환 SID 쌍과 슬롯 불변성이 같다. `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 필수 하위 경계 누락·경로 이미지 판본 폭 불일치·남은 helper 범위 오류 거부, 기존 contained/Graph/공유 예약/환불 모듈을 같은 풀에 연결하는 어댑터.
//  - 이월: C++ 의 실제 경로 프로세스 공간 접두 + 같은 Host/Kernel SharedRegular 예약 통합 시험(`rawPathCleanup_real_process_spatial_prefix_and_shared`)은 경로 프로세스/사제 postPop 사슬이 한 장면에서 연결되는 단위에서 다룬다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E159: 경로 정리 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPopList, SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { RawCarrierPreDestroy } from '../www/core/raw-carrier-pre-destroy.js';
import { PlayerPlacementAnchorState, RawPlayerPlacementAnchor } from '../www/core/raw-player-placement-anchor.js';
import { PATH_LAYOUT, RawPathRuntime } from '../www/core/raw-path-runtime.js';
import { RawPathCleanup, createPathCleanupState, currentNuggetType, makePathCleanupHooks } from '../www/core/raw-path-cleanup.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E159)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'pathcleanup-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 풀 크기, 가상 표 표식, 타입 수, 원본 격리 입력 좌표(단정도)이다. 범위/분기를 여기서 예측하지 않는다. (2026-11-09 WB04-E159)
const CAPACITY = 24000;
const VTABLE = 0x11010000;
const TYPE_COUNT = 188;
const COORDINATES = [
    { x: Math.fround(40.75), y: Math.fround(41.25) },
    { x: Math.fround(40.00004959106445), y: Math.fround(41.00004959106445) },
    { x: Math.fround(40.000003814697266), y: Math.fround(41.000003814697266) }
];

// 예외가 나는지 본다. (2026-11-09 WB04-E159)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E159)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 동일 raw/hash 입력과 기존 contained/Graph 소비자를 한 수명 안에 두는 시험 장면이다. (2026-11-09 WB04-E159)
class CleanupScene {
    // native 와 같은 슬롯/타입/버킷/종속 입력만 만든다. 기대 출력은 계산하지 않는다.
    constructor(input) {
        this.input = input;
        this.pool = new SidPool(CAPACITY, { server: true });
        this.hash = new SquidHash();
        this.types = Array.from({ length: TYPE_COUNT }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0 }));
        this.runtime = new RawPathRuntime({ priest: true, heapFill: 0xaa });
        this.state = createPathCleanupState();
        this.containedState = new ContainedFinderState();
        this.effects = [];
        this.slots = {};
        // 5..11 슬롯의 입력 바이트를 0 으로 채운다.
        for (let sid = 5; sid <= 11; sid += 1) {
            this.slots[sid] = this.pool.physical(sid);
            this.slots[sid].fill(0);
        }
        const [profile, , dead, priest, , checking, , coordinate] = input;
        const point = COORDINATES[coordinate];
        const helper = [1, 2, 3, 4, 5, 6, 7, 14, 15, 16].includes(profile);
        const near = [1, 6, 7, 16].includes(profile);
        const bridge = profile === 8 || profile === 14 || profile === 17;
        if (helper) this.add(6, 128, profile === 7 || profile === 5 ? 1 : 0, (profile === 3 ? 0 : 1) | (profile === 6 ? 8 : 0), near ? point : { x: 80.25, y: 81.25 }, 1, profile === 4 ? 0 : 0x404200, 1, 7);
        const place = [8, 9, 10, 11, 14, 16, 17].includes(profile);
        if (place) this.add(7, bridge ? 126 : 85, profile === 10 ? 2 : 0, profile === 11 ? 1 : 0, point, 1, bridge ? 0 : 0x200, profile === 9 ? 2 : 1, profile === 17 ? 3 : 7);
        if ([12, 13, 14, 15, 16].includes(profile)) this.add(9, 165, 0, profile === 13 || profile === 15 ? 8 : 0, point, 1, 0, 1, 7);
        this.add(11, 97, 0, 0, point, 0, 2, 1, 7);
        if (profile === 17) this.types[126].flags1 = 0x800;
        this.hash.setCell(0, coordinate === 0 ? 41 : 40, coordinate === 0 ? 42 : 41, 11);
        const parent = this.slots[5];
        put(parent, 0, VTABLE);
        parent[10] = priest ? 158 : 133;
        parent[11] = dead ? 2 : 0;
        put(parent, 14, floatToBits(point.x));
        put(parent, 18, floatToBits(point.y));
        parent[34] = 1;
        put(parent, 6, 10, 2);
        this.slots[10][10] = 54;
        put(this.slots[10], 4, place ? 8 : 0, 2);
        put(this.slots[8], 0, VTABLE);
        this.slots[8][10] = 6;
        put(this.slots[8], 18, 154, 2);
        put(this.slots[8], 20, 32, 2);
        this.runtime.setAttachment(10, 5);
        this.runtime.setCommandFields(helper ? 6 : 0, 0x87654321, 0xabcdef01, { x: 17, y: 19 });
        this.state.boss = input[1] !== 0;
        this.state.checking = checking !== 0 ? 1 : 0;
        this.carrier = new RawCarrierPreDestroy(this.pool, this.containedState, {
            validateDamageable: () => { throw new Error('cleanup 독립 조회는 Damageable Validate를 실행하지 않는다'); },
            damageablePre: () => { throw new Error('cleanup 독립 조회는 Damageable PreDestroy를 실행하지 않는다'); },
            dependentFound: () => { throw new Error('cleanup 독립 조회는 Carrier 전역 후처리를 실행하지 않는다'); }
        });
        const anchorState = new PlayerPlacementAnchorState();
        anchorState.graphReady = 1;
        this.anchor = new RawPlayerPlacementAnchor(this.pool, this.types, this.hash.view(0), new SquidPostPopState(), new SquidPostPopList(), this.containedState, anchorState);
        this.cleanup = new RawPathCleanup(this.pool, this.hash, this.types, this.state, this.hooks());
    }

    // 실제 네 버킷 입력에 head 삽입하고 원본 raw next/발자국/현재 타입 속성을 작성한다. (2026-11-09 WB04-E159)
    add(sid, type, rawState, extra, point, level, genus, owner, graph) {
        const raw = this.slots[sid];
        put(raw, 0, VTABLE);
        raw[10] = type;
        raw[11] = rawState;
        raw[40] = extra;
        put(raw, 14, floatToBits(point.x));
        put(raw, 18, floatToBits(point.y));
        put(raw, 4, this.hash.bucket(level, point.x, point.y), 2);
        this.hash.setBucket(level, point.x, point.y, sid);
        raw[34] = owner;
        raw[30] = graph;
        this.types[type].flags2 = genus;
        this.types[type].flags1 = type === 97 ? 0x800 : 0;
        this.types[type].footX = this.types[type].footY = 1;
    }

    // 기존 조회와 명시 효과 요청을 연결하며 하위 효과 뒤 권한 변이를 입력으로 공급한다. (2026-11-09 WB04-E159)
    hooks() {
        const mutation = this.input[6];
        return {
            reserveShared: () => { this.effects.push('N:40'); return this.input[4] !== 0; },
            addShared: (sid, event, payload) => {
                assert.ok(sid === 6 && event === 0x1a2c && payload === 1);
                this.effects.push(`S:${event},${sid},${floatToBits(payload)}`);
                if (mutation === 1) this.state.boss = this.input[1] === 0;
            },
            contained: (sid, mask, kind, index) => {
                this.containedState.boss = this.state.boss;
                this.containedState.checkingDead = this.state.checking;
                return this.carrier.findContained(sid, mask, kind, index);
            },
            graphFor: (sid) => this.anchor.graphFor(sid),
            ritual: (temple, parent, flags) => {
                assert.ok(temple === 9 && parent === 5 && flags === 0);
                this.effects.push('R:9,5,0');
                if (mutation === 2) this.state.boss = !this.state.boss;
            },
            refund: (type, owner) => { this.effects.push(`F:${type},${owner}`); },
            destroy: (sid, flags) => {
                assert.ok((sid === 6 || sid === 8) && flags === 0);
                this.effects.push(`D:${sid},0`);
                this.slots[sid][11] |= 2;
            }
        };
    }

    // 현재 요청 순서만 합쳐 원본 관찰과 비교한다. (2026-11-09 WB04-E159)
    effectsText() { return this.effects.length === 0 ? '-' : this.effects.join(';'); }

    // 가상 표 주입 포인터를 제외한 현재 일곱 슬롯의 모든 byte 를 비교한다. 각 raw 폭과 미사용 필드도 보존되어야 한다. (2026-11-09 WB04-E159)
    checkSlots(expected, where) {
        const images = expected.split(';');
        assert.equal(images.length, 7, where);
        for (let sid = 5; sid <= 11; sid += 1) {
            const actual = hex(this.pool.slot(sid));
            assert.ok(actual.length === images[sid - 5].length && actual.substring(8) === images[sid - 5].substring(8), `${where} 슬롯 ${sid}`);
        }
    }
}

test('경로 정리: 원본 10.78 관측 866행의 효과 순서·남은 helper·권한·슬롯 raw 와 조회 반환이 같다', { skip: rows.length === 0 }, () => {
    assert.equal(allRows.length > rows.length, true);
    let count = 0;
    let queries = 0;
    let rejected = 0;
    // 각 고유 입력마다 같은 풀/hash 를 새로 만든다. 기대 분기나 효과를 다시 계산하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 7);
        const values = row[1].split(',');
        const where = row.slice(0, 2).join(' ');
        if (values[0] === 'Q') {
            assert.equal(values.length, 5);
            const [profile, coordinate, radius, type] = values.slice(1).map(Number);
            const scene = new CleanupScene([profile, 1, 0, 1, 1, 0, 0, coordinate]);
            const point = COORDINATES[coordinate];
            const found = scene.cleanup.returnPlaceAt(point.x, point.y, radius | 0);
            const nearby = scene.cleanup.nearbyType(point.x, point.y, 2, type);
            assert.equal(`Q:${found},${nearby}`, row[3], where);
            scene.checkSlots(row[6], where);
            queries += 1;
            continue;
        }
        assert.equal(values.length, 8);
        const input = values.map(Number);
        const scene = new CleanupScene(input);
        const before = Uint8Array.from(scene.runtime.bytes);
        if (row[3].endsWith(';A') || row[3] === 'A') {
            assert.ok(throws(() => scene.cleanup.afterSpatial(scene.runtime, 5)), where);
            scene.effects.push('A');
            rejected += 1;
        } else scene.cleanup.afterSpatial(scene.runtime, 5);
        assert.equal(scene.effectsText(), row[3], where);
        assert.equal(scene.runtime.helper, Number(row[4]), where);
        assert.equal(scene.state.boss, Number(row[5]) !== 0, where);
        scene.checkSlots(row[6], where);
        const after = scene.runtime.bytes;
        const offset = PATH_LAYOUT.helper;
        assert.ok(Buffer.compare(Buffer.from(before.subarray(0, offset)), Buffer.from(after.subarray(0, offset))) === 0, where);
        assert.ok(Buffer.compare(Buffer.from(before.subarray(offset + 4)), Buffer.from(after.subarray(offset + 4))) === 0, where);
        count += 1;
    }
    assert.ok(count === 704 && queries === 162 && rejected > 0, `${count}/${queries}/${rejected}`);
});

test('경로 정리: 필수 하위 경계·경로 이미지 폭·남은 helper 범위를 거부한다', () => {
    const scene = new CleanupScene([1, 1, 0, 1, 1, 0, 0, 0]);
    const hooks = scene.hooks();
    assert.ok(throws(() => new RawPathCleanup(scene.pool, scene.hash, scene.types, scene.state, { ...hooks, ritual: undefined })));
    assert.ok(throws(() => new RawPathCleanup(scene.pool, scene.hash, scene.types, scene.state, {})));
    assert.ok(throws(() => new RawPathCleanup(scene.pool, scene.hash, scene.types.slice(0, 2), scene.state, hooks)));
    assert.ok(throws(() => scene.cleanup.afterSpatial({ bytes: new Uint8Array(4), helper: 0, clearHelper: () => {} }, 5)));
    // 남은 helper 가 풀 용량 밖이면 거부한다(helper 0 이 아닌 값을 가리키는 경로 이미지).
    scene.runtime.setCommandFields(CAPACITY + 5, 0, 0, { x: 0, y: 0 });
    assert.ok(throws(() => scene.cleanup.afterSpatial(scene.runtime, 5)));
    // 공유 nugget 번호 getter 가 있으면 그 현재 값을 읽는다.
    let shared = 99;
    const state = createPathCleanupState();
    state.sharedNuggetType = () => shared;
    assert.equal(currentNuggetType(state), 99);
    shared = 100;
    assert.equal(currentNuggetType(state), 100);
    assert.equal(currentNuggetType(createPathCleanupState()), 154);
});

test('경로 정리: 기존 contained/Graph/공유 예약/환불 모듈을 같은 풀에 연결하는 어댑터가 정확한 인자로 위임한다', () => {
    const scene = new CleanupScene([1, 1, 0, 1, 1, 0, 0, 0]);
    const calls = [];
    const host = { pool: scene.pool, addSharedRegular: (sid, event, payload) => calls.push(['shared', sid, event, payload]) };
    const place = { pool: scene.pool, refund: (type, owner) => calls.push(['refund', type, owner]) };
    const base = { reserveShared: () => true, ritual: () => {}, destroy: () => {} };
    const hooks = makePathCleanupHooks(scene.pool, scene.carrier, scene.anchor, host, place, base);
    hooks.addShared(6, 0x1a2c, 1);
    hooks.refund(54, 3);
    assert.deepEqual(calls, [['shared', 6, 0x1a2c, 1], ['refund', 54, 3]]);
    assert.equal(typeof hooks.contained, 'function');
    assert.equal(typeof hooks.graphFor, 'function');
    assert.equal(hooks.reserveShared, base.reserveShared);
    assert.equal(hooks.graphFor(5), scene.anchor.graphFor(5));
    assert.ok(throws(() => makePathCleanupHooks(scene.pool, scene.carrier, scene.anchor, { ...host, pool: new SidPool(CAPACITY, { server: true }) }, place, base)));
});
