// Fort 소유자 후처리 raw-fort-finish-owner.js(RawFortFinishOwner·makeFortFinishOwnerHooks)를 원본 10.78 관측 719입력과 cpppj `FortFinishOwnerTests` 계약으로 대조한다.
//  - 원본 관측 fortfinishowner-x86.tsv 의 10.78(originals) 719행: 모드 F(entry 전체)·B(AddRandom 배치 batch)·S(시각 seed)의 16칸 입력(소유자·모드 3종·지역·목록 프로필·벽시계 비트·사각형 프로필·성공 시도/주기·local seed·타입·수량·force·변이)에서
//    호출 순서(지역 R·시각 T·배치 P·진단 D)와 반환·local PRNG 상태·거주 구역 목록·현재 거주 구역 TYPE 을 대조한다. 같은 입력을 공유 Player 기록 모드(현재 +0x78 DWORD)로도 다시 재생한다.
//  - 계약: 로더 훅 연결(같은 풀의 거주 구역이 있으면 지역/시각/배치를 하지 않음, 다른 풀 거부, 초기 모드 반환이 소유자 범위 조회보다 앞섬)과 by-value 사각형 snapshot·실제 경계 누락/손상된 현재 목록 거부.
// CD 10.72 는 웹 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E221: Fort 소유자 후처리 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { GameRandom } from '../www/core/random.js';
import { createFortFinishOwnerState } from '../www/core/raw-island-runtime.js';
import { RawFortFinishOwner, makeFortFinishOwnerHooks } from '../www/core/raw-fort-finish-owner.js';

// 원본 관측 10.78(originals) 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-10 WB04-E221)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fortfinishowner-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 입력 16칸의 위치 이름과 판본당 입력 수이다. (2026-11-10 WB04-E221)
const F = { owner: 0, mode: 1, fort: 2, special: 3, region: 4, list: 5, timeLow: 6, timeHigh: 7, rect: 8, successAt: 9, period: 10, seed: 11, type: 12, quantity: 13, force: 14, mutation: 15 };
const CASES = 719;

// 예외가 나는지 본다. (2026-11-10 WB04-E221)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 실제 native 의 지역 입력 사각형 프로필 여섯 개이다. 기대 배치 좌표는 계산하지 않는다. (2026-11-10 WB04-E221)
const RECTANGLES = [
    [32, 48, 96, 112], [15.25, 21.5, 16.1, 23.9], [-15.25, -3.5, 4.5, 9.5], [20, 21, 20, 21], [20, 21, 19, 19], [1, 2, 255, 254],
].map(([left, top, right, bottom]) => ({ left: Math.fround(left), top: Math.fround(top), right: Math.fround(right), bottom: Math.fround(bottom) }));

// 원본과 동일한 중복/후행/틀린 소유자/flags/free·dead·void 입력의 거주 구역 목록 여섯 프로필이다(sid·type·state·owner·genus). (2026-11-10 WB04-E221)
function finishNodes(profile, owner) {
    const other = (owner + 1) % 9;
    return [
        [],
        [[50, 89, 0, owner, 0x40000000]],
        [[50, 89, 0, other, 0x40000000]],
        [[50, 100, 0, owner, 0]],
        [[50, 100, 0, owner, 0], [51, 89, 0, other, 0x40000000], [52, 89, 0, owner, 0x40000000], [52, 89, 0, owner, 0x40000000]],
        [[50, 89, 15, owner, 0x40000000]],
    ][profile];
}

// 현재 타입/목록·모드·호출별 시각·배치 정책 응답을 같은 actual pool 에 공급하는 장면이다(cpppj FortFinishScene). (2026-11-10 WB04-E221)
class FinishScene {
    constructor(values) {
        this.values = values;
        this.pool = new SidPool(24000, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
        this.residences = new SquidPostPopList();
        this.state = createFortFinishOwnerState();
        this.events = [];
        this.attempts = 0;
        this.rounds = 0;
        this.lastSeed = 0;
        this.added = false;
        // raw native 입력 SID 를 실제로 할당한다.
        for (let sid = 5; sid <= 100; sid += 1) assert.equal(this.pool.allocate(2), sid);
        this.types[89].flags2 = this.types[90].flags2 = 0x40000000;
        this.residences.entries = new Array(8).fill(0);
        this.state.chaffMode = values[F.mode] !== 0;
        this.state.fort = values[F.fort] !== 0;
        this.state.special = values[F.special] !== 0;
        this.state.residenceType = values[F.type];
        this.state.playerRegions.fill(values[F.region] | 0);
        // 각 목록 항목의 원본 raw 상태만 공급하며 같은 SID 의 중복도 그대로 둔다.
        for (const [sid, type, state, owner, genus] of finishNodes(values[F.list], values[F.owner])) {
            const raw = this.pool.allocated(sid);
            raw.fill(0);
            raw[10] = type;
            raw[34] = owner;
            raw[11] = state;
            this.types[type].flags2 = genus;
            this.residences.entries[this.residences.count] = sid;
            this.residences.count += 1;
        }
    }

    // 실제 효과/인자의 원시 DWORD 를 기록한다. (2026-11-10 WB04-E221)
    event(letter, args) { this.events.push(`${letter}:${args.join(':')}`); }

    // 매 시각 호출마다 전달했던 독립 double 표본을 반환한다. (2026-11-10 WB04-E221)
    clock() {
        const view = new DataView(new ArrayBuffer(8));
        view.setUint32(0, this.values[F.timeLow], true);
        view.setUint32(4, this.values[F.timeHigh], true);
        const time = view.getFloat64(0, true) + this.rounds * 0.125;
        this.rounds += 1;
        view.setFloat64(0, time, true);
        this.event('T', [view.getUint32(0, true), view.getUint32(4, true)]);
        return time;
    }

    // native 의 실제 외부 경계만 공급한다. 생성 성공은 같은 residence 목록에 등록하므로 finish 가 다시 읽는다. (2026-11-10 WB04-E221)
    hooks(finishMode) {
        const values = this.values;
        return {
            regionBounds: (region) => {
                const rect = RECTANGLES[values[F.rect]];
                this.event('R', [region >>> 0, floatToBits(rect.left), floatToBits(rect.top), floatToBits(rect.right), floatToBits(rect.bottom)]);
                if (values[F.mutation] === 1) this.state.residenceType = 90;
                return rect;
            },
            wallSeconds: () => this.clock(),
            place: (random, request) => {
                this.attempts += 1;
                this.event('P', [request.type, floatToBits(request.x), floatToBits(request.y), request.region >>> 0, request.mode, request.force]);
                if (values[F.mutation] === 2 && this.attempts === 1) random.setState(0x98765432);
                this.lastSeed = random.state;
                const success = this.attempts >= values[F.successAt] && (this.attempts - values[F.successAt]) % values[F.period] === 0;
                if (success && finishMode && !this.added) {
                    this.added = true;
                    const raw = this.pool.allocated(80);
                    raw.fill(0);
                    raw[10] = request.type & 0xff;
                    raw[34] = values[F.owner];
                    this.residences.entries[this.residences.count] = 80;
                    this.residences.count += 1;
                }
                return success ? 7 : 0;
            },
            invalidType: () => this.event('D', [29]),
        };
    }

    // 관찰 문자열을 fixture 와 같은 형식으로 묶는다. (2026-11-10 WB04-E221)
    eventText() { return this.events.length === 0 ? '-' : this.events.join(';'); }

    // 같은 실제 목록의 중복/최종 순서를 그대로 비교한다. (2026-11-10 WB04-E221)
    list() {
        const items = this.residences.entries.slice(0, this.residences.count);
        return items.length === 0 ? '-' : items.join(',');
    }
}

// entry/standalone batch/실제 seed 생성 관찰을 동일 입력의 몸체와 대조한다. shared 면 같은 입력을 현재 Player 기록의 첫 지역 DWORD 로도 재생한다. (2026-11-10 WB04-E221)
function replay(shared) {
    assert.equal(rows.length, CASES);
    let count = 0;
    // 한 행마다 다른 actual pool/목록을 구성해 이전 callback 상태가 섞이지 않게 한다.
    for (const row of rows) {
        assert.equal(row.length, 8);
        const scene = new FinishScene(row[2].split(',').map(Number));
        const values = scene.values;
        const finish = new RawFortFinishOwner(scene.pool, scene.types, scene.residences, scene.state, scene.hooks(row[1] === 'F'));
        let result = 0;
        const records = new Uint8Array(9 * 0xb4).fill(0xa5);
        if (shared) {
            // 기존 독립 입력의 지역만 원문에 넣고 state 사본은 다른 값으로 분리한다.
            for (let owner = 0; owner < 9; owner += 1) put(records, owner * 0xb4 + 0x78, values[F.region]);
            finish.bindPlayerRecords(records);
            scene.state.playerRegions.fill(999);
        }
        const before = records.slice();
        if (row[1] === 'F') finish.finish(values[F.owner]);
        else if (row[1] === 'B') {
            const random = new GameRandom(values[F.seed]);
            result = finish.placeRandom(random, RECTANGLES[values[F.rect]], values[F.region] | 0, values[F.quantity], values[F.type], values[F.force]);
            scene.lastSeed = random.state;
        } else {
            result = RawFortFinishOwner.timeSeed(scene.clock());
            scene.lastSeed = result;
        }
        const label = `${row[1]} #${count} ${row[2]}`;
        assert.equal(scene.eventText(), row[3], `${label} 호출`);
        assert.equal(String(result), row[4], `${label} 반환`);
        assert.equal(String(scene.lastSeed), row[5], `${label} local seed`);
        assert.equal(scene.list(), row[6], `${label} 거주 구역 목록`);
        assert.equal(String(scene.state.residenceType), row[7], `${label} 현재 타입`);
        if (shared) {
            assert.deepEqual(records, before);
            assert.ok(scene.state.playerRegions.every((value) => value === 999));
        }
        count += 1;
    }
    assert.equal(count, CASES);
}

test('Fort 소유자 후처리: 원본 10.78 719입력의 entry/AddRandom/시각 seed 관찰이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    replay(false);
});

test('Fort 소유자 후처리: 같은 719입력을 공유 Player 기록의 현재 첫 지역 DWORD 로 재생해도 같고 Player 기록은 쓰지 않는다', { skip: !rows && '원본 fixture 없음' }, () => {
    replay(true);
});

test('Fort 소유자 후처리 계약: 로더 훅은 같은 월드 모듈을 호출하고 거주 구역이 있으면 지역/시각/배치를 하지 않는다', { skip: !rows && '원본 fixture 없음' }, () => {
    const input = rows[0][2].split(',').map(Number);
    input[F.list] = 1;
    input[F.region] = 3;
    input[F.mode] = input[F.fort] = input[F.special] = 0;
    input[F.owner] = 1;
    const scene = new FinishScene(input);
    const finish = new RawFortFinishOwner(scene.pool, scene.types, scene.residences, scene.state, scene.hooks(true));
    const hooks = makeFortFinishOwnerHooks(scene.pool, finish, {});
    hooks.finishOwner(1);
    assert.ok(scene.events.length === 0 && scene.rounds === 0 && scene.attempts === 0);
    assert.ok(throws(() => makeFortFinishOwnerHooks(new SidPool(24000, { server: true }), finish, {})));
    // 초기 모드 반환은 소유자 범위 조회보다 앞선다.
    scene.state.chaffMode = true;
    finish.finish(0xffffffff);
    assert.deepEqual(scene.events, []);
});

test('Fort 소유자 후처리 계약: by-value 사각형 snapshot·수량과 실제 경계 누락/손상된 현재 목록을 거부한다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new FinishScene(rows[0][2].split(',').map(Number));
    const missing = scene.hooks(false);
    delete missing.place;
    assert.ok(throws(() => new RawFortFinishOwner(scene.pool, scene.types, scene.residences, scene.state, missing)));
    let attempts = 0;
    let rect = { left: 1, top: 2, right: 1, bottom: 2 };
    const hooks = scene.hooks(false);
    hooks.place = (_random, request) => { assert.ok(request.x === 1 && request.y === 2); attempts += 1; rect = { left: 100, top: 101, right: 150, bottom: 151 }; return 7; };
    const finish = new RawFortFinishOwner(scene.pool, scene.types, scene.residences, scene.state, hooks);
    const random = new GameRandom(0);
    assert.equal(finish.placeRandom(random, rect, 3, 2, 89, 0), 7);
    assert.equal(attempts, 2);
    assert.equal(random.state, 0);
    // 손상된 현재 목록(count 가 저장소 크기를 넘음)은 거부한다.
    scene.state.playerRegions[1] = 3;
    scene.state.chaffMode = scene.state.special = scene.state.fort = false;
    scene.residences.count = 9;
    assert.ok(throws(() => finish.finish(1)));
    // 타입 0/-1 은 진단 후 0, 수량 0 은 첫 시도를 실행하고 성공 때마다 감김(0xffffffff)으로 계속한다.
    const invalid = new FinishScene(rows[0][2].split(',').map(Number));
    const invalidFinish = new RawFortFinishOwner(invalid.pool, invalid.types, invalid.residences, invalid.state, invalid.hooks(false));
    assert.equal(invalidFinish.placeRandom(new GameRandom(5), RECTANGLES[0], 1, 1, 0, 0), 0);
    assert.equal(invalidFinish.placeRandom(new GameRandom(5), RECTANGLES[0], 1, 1, 0xffffffff, 0), 0);
    assert.deepEqual(invalid.events, ['D:29', 'D:29']);
    // Player 기록 크기와 판본을 확인한다.
    assert.ok(throws(() => invalidFinish.bindPlayerRecords(new Uint8Array(10))));
    assert.ok(throws(() => new RawFortFinishOwner({ edition: 'cd1072' }, invalid.types, invalid.residences, invalid.state, invalid.hooks(false))));
});
