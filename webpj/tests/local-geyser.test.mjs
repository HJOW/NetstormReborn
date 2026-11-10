// 로컬 간헐천 생성 raw-local-geyser.js(RawLocalGeyser·LocalGeyserState)를 원본 10.78 관측과 cpppj `LocalGeyserTests.cpp` 계약으로 대조한다(WB04-E256).
//  - local-geyser-x86.tsv 221행(T 후보 / P 요청 / G 전체 생성): 후보 사용 여부·마지막 좌표 비트·로컬 난수 상태·최근 목록 개수와 전체 칸(count 밖 꼬리 포함)·자원 pool·수량 전역·생성 횟수·외부 효과 순서.
//    같은 221행을 독립 값 상태, 공유 자원 상자, 장기 owner(GameAuxState)의 자원/현재 TYPE, 현재 Player base 교체, Player 를 생성 인자로 받지 않는 deferred 구성으로 다시 재생한다.
//  - geyser-placement-x86.tsv 183행(일반 요청의 허용 지형/허공·제외 영역·현재 Player 재조회): 결과·좌표·난수·최근 목록·자원·수량·효과 순서와 Player 9기록 전체 바이트.
//  - 계약(cpppj): 생성 누락 거부, 공유 boss 가 populate 진입을 정함, 일반 제외 요청의 선택 요구, Generate 가 Player 차례마다 base 를 다시 고르고 권한 관문 앞에서 경계를 부름, 제외 순회가 선택한 레코드를 유지.
//  - 경계 계약: fixture 가 구별하지 못해 변형 시험에서 살아남은 입력을 닫는다(마지막 검사들의 주석).
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E256)
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import test from 'node:test';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { LocalGeyserState, RawLocalGeyser } from '../www/core/raw-local-geyser.js';
import { GameRandom } from '../www/core/random.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, floatToBits, get, put, readRows } from './raw-support.mjs';

// 원본 관측 행 수, 합성 풀 크기, Player 기록 폭, 최근 목록 상한이다. (2026-11-10 WB04-E256)
const LOCAL_ROWS = 221;
const PLACEMENT_ROWS = 183;
const POOL_CAPACITY = 32768;
const STRIDE = 0xb4;

// fixture 읽기이다. 파일이 없으면 null 이다. (2026-11-10 WB04-E256)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const localRows = readRows(path.join(FIXTURES, 'local-geyser-x86.tsv'));
const placementRows = readRows(path.join(FIXTURES, 'geyser-placement-x86.tsv'));
const skip = { skip: (!localRows || !placementRows) && '원본 fixture 없음' };

// 10진 문자열 DWORD 를 부호 없는 수로 읽고 signed 로 바꾼다. (2026-11-10 WB04-E256)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const signed = (text) => Number(BigInt.asIntN(32, BigInt(text)));
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 예외 종류와 문구를 함께 확인한다. (2026-11-10 WB04-E256)
const throwsAs = (action, type, pattern) => assert.throws(action, (error) => error instanceof type && pattern.test(error.message));

// fixture 가 쓰는 5종 사각형 입력이다(독립 실행기에 넘긴 같은 명시 사각형). (2026-11-10 WB04-E256)
const AREAS = [
    { left: 16, top: 24, right: 48, bottom: 56 },
    { left: 16.25, top: 24.5, right: 48.75, bottom: 56.25 },
    { left: 16, top: 24, right: 16.5, bottom: 24.5 },
    { left: 16, top: 24, right: 15, bottom: 23 },
    { left: 0, top: 0, right: 2, bottom: 2 },
];
const area = (profile) => AREAS[profile];

// 모든 최근 목록 칸을 원본과 같은 문자열로 기록한다(count 밖 stale 항목 포함). (2026-11-10 WB04-E256)
const recentText = (recent) => recent.entries.join(',');

// 실행기의 9 Player flags/region 입력을 원문 표에 기록한다. (2026-11-10 WB04-E256)
function setPlayers(players, profile) {
    const flags = new Array(9).fill(0);
    const regions = new Array(9).fill(127);
    if (profile === 1) { flags[1] = 2; regions[1] = 1; }
    else if (profile === 2) { flags[1] = 2; regions[1] = 1; flags[2] = 32; regions[2] = 2; }
    else if (profile === 3) { flags[4] = 34; regions[4] = 4; flags[5] = 2; regions[5] = 5; }
    else if (profile === 4) { [flags, regions].forEach((list, index) => list.splice(0, 9, ...(index === 0 ? [0, 1, 2, 32, 34, 0, 0, 2, 32] : [127, 1, 127, 3, 4, 5, 127, 7, 8]))); }
    // 같은 원본 9기록의 각 BYTE/DWORD 만 쓴다.
    for (let owner = 0; owner < 9; owner += 1) {
        put(players, owner * STRIDE + 0x50, flags[owner], 1);
        put(players, owner * STRIDE + 0x78, regions[owner]);
    }
}

// 188칸 TYPE 입력이다(간헐천 비용 2000, 빈 간헐천 발자국 256). (2026-11-10 WB04-E256)
function geyserTypes() {
    const types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0 }));
    types[122].cost = 2000;
    types[84].footX = 256;
    types[84].footY = 256;
    return types;
}

// 실제 생성/공간/설정 경계만 관찰한다. 난수/후보/반복/최근/pool 의 기대값을 재계산하지 않는다(cpppj `GeyserEnvironment`). (2026-11-10 WB04-E256)
class GeyserEnvironment {
    constructor(state, recent, players, types) {
        this.state = state;
        this.recent = recent;
        this.players = players;
        this.types = types;
        this.sharedPool = null;
        this.predictable = 0;
        this.mutation = 0;
        this.created = 0;
        this.time = 0;
        this.trace = [];
    }

    // 외부 삭제/Pop 효과가 실제로 바꾸는 단일 현재 자원 저장소의 읽기/쓰기이다. (2026-11-10 WB04-E256)
    get pool() { return this.sharedPool ? this.sharedPool.value : this.state.resourcePool; }

    set pool(value) {
        if (this.sharedPool) this.sharedPool.value = value;
        else this.state.resourcePool = value;
    }

    // 원본 fixture 와 같은 한 항목의 외부 효과를 기록한다. (2026-11-10 WB04-E256)
    add(text) { this.trace.push(text); }

    // 실제 함수에 해당하는 명시 장치/공간 하위만 공급한다. (2026-11-10 WB04-E256)
    hooks() {
        return {
            configInt: (key, fallback) => {
                assert.equal(key, 'predictableGeysers');
                assert.equal(fallback, 1);
                this.add('Config');
                return this.predictable;
            },
            wallSeconds: () => {
                this.add('Time');
                return this.time;
            },
            playerBounds: (owner) => {
                this.add(`Bounds:${owner}`);
                if (this.mutation === 2 && owner === 1) {
                    this.state.firstPlayerQuantity = 1;
                    put(this.players, 2 * STRIDE + 0x50, 32, 1);
                    put(this.players, 2 * STRIDE + 0x78, 2);
                }
                const origin = owner * 16;
                return { left: origin, top: origin, right: origin + 32, bottom: origin + 32 };
            },
            negativeQuantity: () => this.add('Assert'),
            destroy: (sid, flags) => {
                assert.equal(sid, 9);
                if (this.mutation === 3) this.pool = 12000;
                this.add(`Destroy:${sid}:${flags}`);
            },
            confirm: (request) => {
                const values = [request.type, floatToBits(request.x), floatToBits(request.y), request.player, request.argument, request.direction, request.flags, request.timeLow, request.timeHigh, request.quality];
                this.add(`Confirm:${values.join(',')}`);
            },
            create: (type, flags) => {
                const sid = 50 + this.created;
                this.created += 1;
                this.add(`Create:${type}:${flags}:${sid}`);
                return sid;
            },
            pop: (sid, x, y, flags) => {
                this.add(`Pop:${sid}:${floatToBits(x)}:${floatToBits(y)}:${flags}`);
                if (this.mutation === 1) {
                    this.pool = 16777217;
                    this.types[122].cost = 4000;
                    this.recent.count = 4;
                } else if (this.mutation === 3) {
                    this.pool += 1000;
                }
            },
        };
    }

    // fixture 형식의 효과 순서 문자열이다(비어 있으면 '-'). (2026-11-10 WB04-E256)
    traceText() { return this.trace.length === 0 ? '-' : this.trace.join('|'); }
}

// 같은 원본 221행을 지정한 backend 로 재생해 결과/전체 저장소/효과 순서까지 대조한다.
//  shared: 공유 자원 상자를 쓴다. gameOwner: 장기 owner(GameAuxState)의 자원/현재 TYPE 을 쓴다. currentBase: 생성 뒤 현재 Player base(물리 수명)를 바꾼다. deferred: Player 를 생성 인자로 받지 않는다. (2026-11-10 WB04-E256)
function checkLocalGeyserFixture({ shared = false, gameOwner = false, currentBase = false, deferred = false } = {}) {
    let count = 0;
    // 사례마다 실제 원본과 같은 독립 raw 저장소를 구성한다.
    for (const row of localRows) {
        assert.equal(row.length, 26);
        count += 1;
        const pool = new SidPool(POOL_CAPACITY);
        const hash = new SquidHash();
        const spots = new Uint8Array(65536).fill(number(row[3]));
        let players = new Uint8Array(9 * STRIDE);
        const types = geyserTypes();
        const frames = types.map(() => new FrameCodeTable([]));
        if (number(row[8])) {
            // 모든 검색점에서 같은 명시 empty 결과 9 가 실제 typed finder 로 나오도록 넓은 발자국을 입력한다.
            const raw = pool.physical(9);
            put(raw, 8, 0, 2);
            raw[10] = 84;
            raw[40] = 0;
            put(raw, 14, floatToBits(255));
            put(raw, 18, floatToBits(255));
            for (let y = 0; y < 16; y += 1) for (let x = 0; x < 16; x += 1) hash.setCell(3, x, y, 9);
        }
        const surfaceProfile = number(row[15]);
        if (surfaceProfile) {
            const pattern = surfaceProfile === 1 ? 'FFFFFFFFF' : surfaceProfile === 2 ? 'FBICANGDH' : 'fbicangdh';
            const codes = [];
            // 후보 첫 3×3 과 같은 고정 raw 입력이며 원본 출력으로 좌표를 역산하지 않는다.
            for (let index = 0; index < 9; index += 1) {
                const sid = 20 + index;
                const raw = pool.physical(sid);
                raw[10] = 97;
                put(raw, 36, index);
                hash.setCell(0, 31 + Math.floor(index / 3), 35 + (index % 3), sid);
                codes.push({ side: pattern.charCodeAt(index), variant: 65, flags: 0 });
            }
            frames[97] = new FrameCodeTable(codes);
        }
        const recent = new SquidPostPopList();
        recent.count = 3;
        recent.entries = [31, 32, 33, 34, 35, 36, 37];
        const state = new LocalGeyserState();
        state.boss = number(row[5]) !== 0;
        state.fort = number(row[6]) !== 0;
        state.resourcePool = signed(row[7]);
        state.firstPlayerQuantity = Number(row[12]) | 0;
        state.lastPlayerQuantity = Number(row[13]) | 0;
        setPlayers(players, number(row[9]));
        if (number(row[14]) === 2) {
            put(players, STRIDE + 0x50, 2, 1);
            put(players, STRIDE + 0x78, 1);
        }
        const environment = new GeyserEnvironment(state, recent, players, types);
        environment.predictable = number(row[10]);
        environment.time = new DataView(new BigUint64Array([BigInt(row[11])]).buffer).getFloat64(0, true);
        environment.mutation = number(row[14]);
        const aux = new GameAuxState();
        aux.resourcePool = state.resourcePool;
        const localBox = { value: state.resourcePool };
        const auxBox = { get value() { return aux.resourcePool; }, set value(value) { aux.resourcePool = value; } };
        const sharedBox = gameOwner ? auxBox : localBox;
        if (gameOwner) {
            state.sharedGeyserType = () => aux.geyserType;
            state.geyserType = 70;
        }
        if (shared) {
            state.resourcePool = -777;
            environment.sharedPool = sharedBox;
        }
        const hooks = environment.hooks();
        hooks.playerBoundsRecord = (owner) => hooks.playerBounds(owner);
        const getter = () => players;
        const body = deferred ? RawLocalGeyser.deferred(getter, pool, hash, spots, types, frames, recent, state, hooks) : new RawLocalGeyser(pool, hash, spots, types, frames, players, recent, state, hooks);
        if (currentBase) {
            if (!deferred) body.bindCurrentPlayers(getter);
            // 다음 Player 차례의 실제 base 와 물리 수명만 바꾼다(옛 물리는 쓰레기로 채운다).
            const old = players;
            players = new Uint8Array(old);
            environment.players = players;
            old.fill(0xee);
        }
        assert.ok(body.pool === pool && body.recent === recent);
        if (shared) body.bindResourcePool(sharedBox);
        const random = new GameRandom(number(row[1]));
        const point = { x: bitsToFloat(0xdeadbeef), y: bitsToFloat(0xcafebabe) };
        let result = 0;
        if (row[0] === 'T') result = body.tryCandidate(random, area(number(row[2])), point) ? 1 : 0;
        else if (row[0] === 'P') body.populate(Number(row[4]) | 0, random, area(number(row[2])));
        else if (row[0] === 'G') body.generate();
        else throw new Error('로컬간헐천 fixture 연산 오류');
        const where = `행 ${count} (${row[0]})`;
        assert.equal(result, number(row[16]), where);
        assert.equal(floatToBits(point.x), number(row[17]), where);
        assert.equal(floatToBits(point.y), number(row[18]), where);
        assert.equal(random.state, number(row[19]), where);
        assert.equal(recent.count, number(row[20]), where);
        assert.equal(recentText(recent), row[21], where);
        assert.equal(body.readResourcePool() >>> 0, number(row[22]), where);
        if (shared) assert.equal(state.resourcePool, -777, where);
        // 외부 호출이 없는 행은 fixture 의 빈 열 sentinel 과 비교하며 실제 관찰 내용을 바꾸지 않는다.
        assert.equal(state.firstPlayerQuantity, Number(row[23]) | 0, where);
        assert.equal(environment.created, number(row[24]), where);
        assert.equal(environment.traceText(), row[25], where);
    }
    assert.equal(count, LOCAL_ROWS);
}

test('LocalGeyser: 원본 10.78 221행(독립 값 상태)의 후보·난수·최근 목록·자원·수량·효과 순서가 같다', skip, () => {
    checkLocalGeyserFixture();
});

test('LocalGeyser: 같은 221행을 공유 자원 상자(사본 불변)에서 삭제/Pop 뒤 현재 값을 다시 읽으며 재생한다', skip, () => {
    checkLocalGeyserFixture({ shared: true });
});

test('LocalGeyser: 장기 owner(GameAuxState)의 자원과 반대 fallback 현재 TYPE 을 쓰는 221행도 같다', skip, () => {
    checkLocalGeyserFixture({ shared: true, gameOwner: true });
});

test('LocalGeyser: 생성 뒤 현재 Player base 를 바꿔도 221행의 출력이 유지된다', skip, () => {
    checkLocalGeyserFixture({ shared: true, gameOwner: true, currentBase: true });
});

test('LocalGeyser: Player 를 생성 인자로 보관하지 않는 deferred 구성도 221행을 전부 재생한다', skip, () => {
    checkLocalGeyserFixture({ shared: true, gameOwner: true, currentBase: true, deferred: true });
});

test('LocalGeyser: 실제 생성(create) 누락은 구성 단계에서 거부한다', () => {
    const state = new LocalGeyserState();
    const recent = new SquidPostPopList();
    recent.entries = new Array(5).fill(0);
    const environment = new GeyserEnvironment(state, recent, new Uint8Array(9 * STRIDE), geyserTypes());
    const hooks = environment.hooks();
    delete hooks.create;
    const frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
    throwsAs(() => new RawLocalGeyser(new SidPool(POOL_CAPACITY), new SquidHash(), new Uint8Array(65536), geyserTypes(), frames, new Uint8Array(9 * STRIDE), recent, state, hooks), TypeError, /실제 생성 경계 누락/);
});

test('LocalGeyser: 공유 boss 가 수량 0 의 populate 진입(최근 목록 비움)을 정한다', () => {
    const pool = new SidPool(POOL_CAPACITY);
    const state = new LocalGeyserState();
    const recent = new SquidPostPopList();
    recent.entries = new Array(5).fill(0);
    recent.count = 3;
    const types = geyserTypes();
    const environment = new GeyserEnvironment(state, recent, new Uint8Array(9 * STRIDE), types);
    let boss = false;
    state.sharedBoss = () => boss;
    const frames = types.map(() => new FrameCodeTable([]));
    const geysers = new RawLocalGeyser(pool, new SquidHash(), new Uint8Array(65536), types, frames, new Uint8Array(9 * STRIDE), recent, state, environment.hooks());
    const random = new GameRandom(123);
    geysers.populate(0, random, { left: 0, top: 0, right: 0, bottom: 0 });
    assert.ok(recent.count === 3 && random.state === 123);
    boss = true;
    state.boss = false;
    geysers.populate(0, random, { left: 0, top: 0, right: 0, bottom: 0 });
    assert.ok(recent.count === 0 && random.state === 123 && environment.trace.length === 0);
});

// ---- 일반 요청(허용 지형/허공·제외 영역) ----

// 일반 finder 의 제외 사각형도 실제 CPU 에 넣은 입력만 반환한다. (2026-11-10 WB04-E256)
function generalRectangles(profile) {
    if (profile === 0) return [];
    if (profile === 1) return [{ left: 30, top: 34, right: 36, bottom: 40 }];
    if (profile === 2) return [{ left: 33, top: 37, right: 33, bottom: 37 }];
    if (profile === 3) return [{ left: 50, top: 50, right: 55, bottom: 55 }];
    if (profile === 4) return [...Array.from({ length: 7 }, () => ({ left: 0, top: 0, right: 1, bottom: 1 })), { left: 30, top: 34, right: 36, bottom: 40 }];
    if (profile === 5) return [{ left: 36, top: 40, right: 30, bottom: 34 }];
    throw new RangeError('일반 간헐천 검사 사각형 입력 범위');
}

test('LocalGeyser: 일반 요청 원본 183행(허용 인수·균일 지형·제외 영역·현재 Player 재조회)이 같다', skip, () => {
    let checked = 0;
    // 제품 계산으로 기대 좌표나 성공을 만들지 않는다.
    for (const row of placementRows) {
        assert.equal(row.length, 35);
        const pool = new SidPool(POOL_CAPACITY);
        const hash = new SquidHash();
        const spots = new Uint8Array(65536).fill(number(row[3]));
        const players = new Uint8Array(1620);
        const types = geyserTypes();
        const frames = types.map(() => new FrameCodeTable([]));
        if (number(row[15])) {
            const profile = number(row[15]);
            const pattern = profile === 1 ? 'FFFFFFFFF' : profile === 2 ? 'FBICANGDH' : 'fbicangdh';
            const codes = [];
            // 명시한 9개 지면 슬롯과 프레임 입력만 설치한다.
            for (let index = 0; index < 9; index += 1) {
                const sid = 20 + index;
                const raw = pool.physical(sid);
                raw[10] = 97;
                put(raw, 36, index);
                hash.setCell(0, 31 + Math.floor(index / 3), 35 + (index % 3), sid);
                codes.push({ side: pattern.charCodeAt(index), variant: 65, flags: 0 });
            }
            frames[97] = new FrameCodeTable(codes);
        }
        setPlayers(players, 0);
        if (row[0] === 'P') {
            // constructor 진입 전의 독립 Player 프로필이다.
            for (let owner = 1; owner <= 8; owner += 1) {
                const profile = number(row[21]);
                const active = (profile === 1 && owner === 1) || profile === 3 || (profile === 4 && owner <= 3) || (profile === 5 && owner === 4);
                const flags = active ? 1 : profile === 2 && owner === 1 ? 2 : profile === 2 && owner === 2 ? 32 : 0;
                put(players, owner * 180 + 0x50, flags, 1);
                put(players, owner * 180 + 0x78, active ? owner : 127);
            }
        }
        if (number(row[20])) spots[35 * 256 + 31] = number(row[3]) === 2 ? 0 : 2;
        const recent = new SquidPostPopList();
        recent.count = 3;
        recent.entries = [31, 32, 33, 34, 35, 36, 37];
        const state = new LocalGeyserState();
        state.boss = number(row[5]) !== 0;
        state.fort = number(row[6]) !== 0;
        state.resourcePool = signed(row[7]);
        state.firstPlayerQuantity = Number(row[12]) | 0;
        state.lastPlayerQuantity = Number(row[13]) | 0;
        const environment = new GeyserEnvironment(state, recent, players, types);
        const hooks = environment.hooks();
        hooks.regionExists = (region) => {
            environment.add(`Exists:${region}`);
            if (number(row[23]) === 1 && region === 1) {
                put(players, 2 * 180 + 0x50, 1, 1);
                put(players, 2 * 180 + 0x78, 2);
            }
            return region >= 0 && region < 9 && (number(row[22]) === 1 || (number(row[22]) === 2 && region % 2 !== 0));
        };
        const original = hooks.playerBounds;
        hooks.playerBounds = (owner) => {
            if (number(row[23]) === 2) {
                put(players, 3 * 180 + 0x50, 1, 1);
                put(players, 3 * 180 + 0x78, 3);
            }
            return original(owner);
        };
        const body = new RawLocalGeyser(pool, hash, spots, types, frames, players, recent, state, hooks);
        const random = new GameRandom(number(row[1]));
        const point = { x: bitsToFloat(0xdeadbeef), y: bitsToFloat(0xcafebabe) };
        let result = 0;
        if (row[0] === 'T') result = body.tryCandidate(random, area(number(row[2])), point, number(row[17]), number(row[18]), generalRectangles(number(row[19]))) ? 1 : 0;
        else body.populate(Number(row[4]) | 0, random, area(number(row[2])), { excludePlayers: number(row[16]), allowLand: number(row[17]), allowVoid: number(row[18]) });
        const where = `행 ${checked + 1} (${row[0]})`;
        assert.equal(result, number(row[24]), where);
        assert.ok(floatToBits(point.x) === number(row[25]) && floatToBits(point.y) === number(row[26]), where);
        assert.equal(random.state, number(row[27]), where);
        assert.ok(recent.count === number(row[28]) && recentText(recent) === row[29], where);
        assert.equal(body.readResourcePool() >>> 0, number(row[30]), where);
        assert.ok(state.firstPlayerQuantity === (Number(row[31]) | 0) && environment.created === number(row[32]), where);
        assert.equal(environment.traceText(), row[33], where);
        assert.equal(hex(players), row[34], where);
        checked += 1;
    }
    assert.equal(checked, PLACEMENT_ROWS);
});

test('LocalGeyser: 선택한 제외 요청만 실제 목록 장치를 요구하며 수량 0/권한/부분 쓰기 순서를 보존한다', () => {
    const pool = new SidPool(POOL_CAPACITY);
    const players = new Uint8Array(1620);
    put(players, 180 + 0x50, 1, 1);
    put(players, 180 + 0x78, 1);
    const recent = new SquidPostPopList();
    recent.count = 3;
    recent.entries = new Array(5).fill(0);
    const state = new LocalGeyserState();
    state.boss = false;
    const types = geyserTypes();
    const environment = new GeyserEnvironment(state, recent, players, types);
    const frames = types.map(() => new FrameCodeTable([]));
    const body = new RawLocalGeyser(pool, new SquidHash(), new Uint8Array(65536), types, frames, players, recent, state, environment.hooks());
    const random = new GameRandom(123);
    const rectangle = { left: 0, top: 0, right: 0, bottom: 0 };
    // 권한이 없으면 목록 장치도 최근 목록도 건드리지 않는다.
    body.populate(0, random, rectangle, { excludePlayers: 1, allowLand: 1, allowVoid: 1 });
    assert.ok(recent.count === 3 && random.state === 123);
    state.boss = true;
    // 권한이 있으면 최근 목록을 지운 뒤 선택한 제외 영역이 실제 IslandList 조회를 요구한다.
    throwsAs(() => body.populate(0, random, rectangle, { excludePlayers: 1, allowLand: 1, allowVoid: 1 }), Error, /IslandList 조회 미연결/);
    assert.ok(recent.count === 0 && random.state === 123);
    // 제외 요청이 없으면 조회 장치를 요구하지 않는다.
    body.populate(0, random, rectangle, { excludePlayers: 0, allowLand: 1, allowVoid: 1 });
    assert.equal(random.state, 123);
});

// ---- 현재 Player base 선택 ----

test('LocalGeyser: Generate 는 Player 차례마다 현재 base 를 다시 고르고 경계 부모를 권한 관문 앞에서 호출한다', () => {
    const pool = new SidPool(POOL_CAPACITY);
    const before = new Uint8Array(1620);
    const after = new Uint8Array(1620);
    put(before, STRIDE + 0x50, 2, 1);
    put(before, STRIDE + 0x78, 1);
    put(after, 2 * STRIDE + 0x50, 32, 1);
    put(after, 2 * STRIDE + 0x78, 2);
    const recent = new SquidPostPopList();
    recent.entries = [1, 2, 3, 4, 5];
    recent.count = 3;
    const state = new LocalGeyserState();
    state.boss = false;
    const types = geyserTypes();
    const environment = new GeyserEnvironment(state, recent, before, types);
    environment.predictable = 1;
    const hooks = environment.hooks();
    let current = before;
    const seen = [];
    let reads = 0;
    hooks.playerBoundsRecord = (owner, record) => {
        seen.push(owner);
        assert.ok(record.buffer === (owner === 1 ? before : after).buffer && record.byteOffset === owner * STRIDE);
        if (owner === 1) current = after;
        return { left: 0, top: 0, right: 0, bottom: 0 };
    };
    const frames = types.map(() => new FrameCodeTable([]));
    const body = new RawLocalGeyser(pool, new SquidHash(), new Uint8Array(65536), types, frames, before, recent, state, hooks);
    body.bindCurrentPlayers(() => { reads += 1; return current; });
    body.generate();
    assert.deepEqual(seen, [1, 2]);
    assert.ok(reads === 8 && recent.count === 3 && environment.created === 0);
});

test('LocalGeyser: 제외 영역 순회는 지역 존재 확인 뒤 base 가 바뀌어도 이 차례에 선택한 같은 레코드를 경계에 전달한다', () => {
    const pool = new SidPool(POOL_CAPACITY);
    const before = new Uint8Array(1620);
    const after = new Uint8Array(1620);
    put(before, STRIDE + 0x50, 1, 1);
    put(before, STRIDE + 0x78, 7);
    put(after, 2 * STRIDE + 0x50, 1, 1);
    put(after, 2 * STRIDE + 0x78, 8);
    const recent = new SquidPostPopList();
    recent.entries = [1, 2, 3, 4, 5];
    const state = new LocalGeyserState();
    const types = geyserTypes();
    const environment = new GeyserEnvironment(state, recent, before, types);
    const hooks = environment.hooks();
    let current = before;
    const seen = [];
    hooks.regionExists = (region) => {
        assert.ok(region === 7 || region === 8);
        current = after;
        return true;
    };
    hooks.playerBoundsRecord = (owner, record) => {
        seen.push(owner);
        assert.ok(record.buffer === (owner === 1 ? before : after).buffer && record.byteOffset === owner * STRIDE);
        return { left: get(record, 0x78), top: 0, right: 20, bottom: 30 };
    };
    const frames = types.map(() => new FrameCodeTable([]));
    const body = new RawLocalGeyser(pool, new SquidHash(), new Uint8Array(65536), types, frames, before, recent, state, hooks);
    body.bindCurrentPlayers(() => current);
    const areas = body.playerExclusions();
    assert.deepEqual(seen, [1, 2]);
    assert.equal(areas.length, 2);
    assert.ok(areas[0].left === -5 && areas[1].left === -4);
});

// ---- 경계 계약: fixture 가 구별하지 못해 변형 시험에서 살아남은 입력 (2026-11-10 WB04-E256) ----

// 호출한 한계를 기록하고 미리 정한 값을 차례로 돌려주는 난수 스텁이다(남은 값이 없으면 0). (2026-11-10 WB04-E256)
const stubRandom = (values = []) => ({ calls: [], next(limit) { this.calls.push(limit); return values.length ? values.shift() : 0; } });

// 작은 로컬 간헐천 장면이다. spots 는 모두 spot 값으로 채우고 hooks 는 환경 기본에 override 를 덮는다. (2026-11-10 WB04-E256)
function scene({ spot = 0, hooks = {}, state = new LocalGeyserState(), players = null } = {}) {
    const pool = new SidPool(POOL_CAPACITY);
    const hash = new SquidHash();
    const spots = new Uint8Array(65536).fill(spot);
    const types = geyserTypes();
    const frames = types.map(() => new FrameCodeTable([]));
    const recent = new SquidPostPopList();
    recent.entries = new Array(5).fill(0);
    recent.count = 0;
    const records = players ?? new Uint8Array(9 * STRIDE);
    const environment = new GeyserEnvironment(state, recent, records, types);
    const body = new RawLocalGeyser(pool, hash, spots, types, frames, records, recent, state, { ...environment.hooks(), ...hooks });
    return { pool, hash, spots, types, frames, recent, state, environment, body, players: records };
}

// 점 하나를 입력 사각형과 통제된 난수로 구하고 결과를 돌려준다. (2026-11-10 WB04-E256)
function candidateAt(setup, rectangle, randoms, ...rest) {
    const random = stubRandom(randoms);
    const point = { x: -1, y: -1 };
    const ok = setup.body.tryCandidate(random, rectangle, point, ...rest);
    return { ok, point, random };
}

const BIG = { left: 16, top: 24, right: 48, bottom: 56 };

test('LocalGeyser: 상태 읽기 접근자는 공유 getter 가 있으면 그 현재 값을(부호 규칙대로), 없으면 독립 값을 쓴다', () => {
    const cases = [
        ['readBoss', 'boss', 'sharedBoss', true, () => 0, false],
        ['readFort', 'fort', 'sharedFort', false, () => 1, true],
        ['readGeyserType', 'geyserType', 'sharedGeyserType', 122, () => 0x80000000, 0x80000000],
        ['readEmptyGeyserType', 'emptyGeyserType', 'sharedEmptyGeyserType', 84, () => 0xffffffff, 0xffffffff],
        ['readNoIslandType', 'noIslandType', 'sharedNoIslandType', 157, () => 0x80000001, 0x80000001],
        ['readIsleType', 'isleType', 'sharedIsleType', 97, () => 0xfffffffe, 0xfffffffe],
        ['readFirstQuantity', 'firstPlayerQuantity', 'sharedFirstQuantity', 3, () => 0xfffffffe, -2],
        ['readLastQuantity', 'lastPlayerQuantity', 'sharedLastQuantity', 4, () => 0xfffffffd, -3],
    ];
    // 독립 값은 기본값이고, 공유 getter 를 연결하면 그 현재 값(u32 는 부호 없음, 수량은 signed)을 읽으며 독립 필드를 바꿔도 영향이 없다.
    for (const [reader, field, shared, initial, getter, expected] of cases) {
        const state = new LocalGeyserState();
        assert.equal(state[reader](), initial, reader);
        state[shared] = getter;
        state[field] = typeof initial === 'boolean' ? initial : 55;
        assert.equal(state[reader](), expected, reader);
    }
});

test('LocalGeyser: 생성자는 판본·표 크기·spot·Player·최근 목록·필수 하위 효과 누락을 모두 TypeError 로 거부한다', () => {
    const state = new LocalGeyserState();
    const make = (changes = {}) => {
        const base = {
            pool: new SidPool(POOL_CAPACITY), hash: new SquidHash(), spots: new Uint8Array(65536), types: geyserTypes(),
            frames: Array.from({ length: 188 }, () => new FrameCodeTable([])), players: new Uint8Array(9 * STRIDE), recent: new SquidPostPopList(), hooks: null, current: null,
            ...changes,
        };
        if (!changes.recent) base.recent.entries = new Array(5).fill(0);
        if (!changes.hooks) base.hooks = new GeyserEnvironment(state, base.recent, base.players, base.types).hooks();
        return () => new RawLocalGeyser(base.pool, base.hash, base.spots, base.types, base.frames, base.players, base.recent, state, base.hooks, base.current);
    };
    make()();
    const shortRecent = new SquidPostPopList();
    shortRecent.entries = new Array(4).fill(0);
    const bad = {
        '판본': { pool: { edition: 'cd1072' } },
        '타입 표': { types: geyserTypes().slice(0, 187) },
        '프레임 표': { frames: Array.from({ length: 187 }, () => new FrameCodeTable([])) },
        '타입·프레임 표가 함께 짧음': { types: geyserTypes().slice(0, 187), frames: Array.from({ length: 187 }, () => new FrameCodeTable([])) },
        'spot': { spots: new Uint8Array(65535) },
        'Player': { players: new Uint8Array(9 * STRIDE - 1) },
        '최근 목록': { recent: shortRecent },
    };
    for (const [name, changes] of Object.entries(bad)) throwsAs(make(changes), TypeError, /실제 생성 경계 누락/);
    // 필수 하위 효과 여덟 종은 하나라도 없으면 거부한다.
    for (const hook of ['configInt', 'wallSeconds', 'playerBounds', 'negativeQuantity', 'destroy', 'confirm', 'create', 'pop']) {
        const recent = new SquidPostPopList();
        recent.entries = new Array(5).fill(0);
        const hooks = new GeyserEnvironment(state, recent, new Uint8Array(9 * STRIDE), geyserTypes()).hooks();
        delete hooks[hook];
        throwsAs(make({ recent, hooks }), TypeError, /실제 생성 경계 누락/);
    }
    // 현재 Player 조회 함수가 있으면 선택 레코드 경계(playerBoundsRecord)가 필요하다(Player 인자는 필요 없다).
    const getter = () => new Uint8Array(9 * STRIDE);
    throwsAs(make({ current: getter }), TypeError, /실제 생성 경계 누락/);
    const recent = new SquidPostPopList();
    recent.entries = new Array(5).fill(0);
    const hooks = { ...new GeyserEnvironment(state, recent, new Uint8Array(9 * STRIDE), geyserTypes()).hooks(), playerBoundsRecord: () => ({ left: 0, top: 0, right: 0, bottom: 0 }) };
    make({ recent, hooks, current: getter, players: null })();
});

test('LocalGeyser: 현재 Player 조회/자원 상자 연결은 중복·누락을 거부하고 Player 9기록 크기를 확인한다', () => {
    const base = scene();
    const getter = () => new Uint8Array(9 * STRIDE);
    // 선택 레코드 경계가 없는 구성에는 현재 Player 조회를 연결할 수 없고, 함수가 아닌 값도 거부한다.
    throwsAs(() => base.body.bindCurrentPlayers(getter), TypeError, /중복\/누락/);
    const record = scene({ hooks: { playerBoundsRecord: () => ({ left: 0, top: 0, right: 0, bottom: 0 }) } });
    throwsAs(() => record.body.bindCurrentPlayers(null), TypeError, /중복\/누락/);
    record.body.bindCurrentPlayers(getter);
    throwsAs(() => record.body.bindCurrentPlayers(getter), TypeError, /중복\/누락/);
    // 현재 Player 조회가 9기록보다 작으면 선택 위치에서 거부한다.
    const small = scene({ hooks: { playerBoundsRecord: () => ({ left: 0, top: 0, right: 0, bottom: 0 }), regionExists: () => true } });
    small.body.bindCurrentPlayers(() => new Uint8Array(9 * STRIDE - 1));
    throwsAs(() => small.body.playerExclusions(), RangeError, /Player9 물리 부족/);
    // 같은 자원 상자는 다시 연결해도 되지만 다른 상자는 거부한다.
    const box = { value: 5 };
    base.body.bindResourcePool(box);
    base.body.bindResourcePool(box);
    throwsAs(() => base.body.bindResourcePool({ value: 5 }), Error, /다른 주소 연결/);
    assert.equal(base.body.readResourcePool(), 5);
});

test('LocalGeyser: 후보는 폭이나 높이가 정수로 0 이면 난수 없이 사각형 모서리를 쓰고 한계가 1 이면 시드를 소비한다', () => {
    // 폭이 0.5(→0)이거나 음의 0.5, 무한대인 사각형은 난수를 소비하지 않고 반올림한 모서리를 점으로 삼는다.
    for (const rectangle of [{ left: 16, top: 24, right: 16.5, bottom: 30 }, { left: 16, top: 24, right: 15.5, bottom: 56 }, { left: 16, top: 24, right: Infinity, bottom: 56 },
        { left: 16, top: 24, right: 48, bottom: 24.5 }, { left: 16, top: 24, right: NaN, bottom: 56 }]) {
        const result = candidateAt(scene(), rectangle, [5, 5]);
        assert.deepEqual(result.random.calls, [], JSON.stringify(rectangle));
        assert.deepEqual(result.point, { x: 16, y: 24 });
    }
    // 높이 난수 뒤 폭 난수 순서이며, 한계 1 도 시드를 소비한다(결과 0).
    const one = candidateAt(scene(), { left: 16, top: 24, right: 17, bottom: 25 }, []);
    assert.deepEqual(one.random.calls, [1, 1]);
    const order = candidateAt(scene(), BIG, [7, 9]);
    assert.deepEqual(order.random.calls, [32, 32]);
    assert.deepEqual(order.point, { x: 25, y: 31 });
    const mixed = candidateAt(scene(), { left: 16, top: 24, right: 20, bottom: 56 }, [7, 3]);
    assert.deepEqual(mixed.random.calls, [32, 4]);
    assert.deepEqual(mixed.point, { x: 19, y: 31 });
});

test('LocalGeyser: 후보 점은 float 로 한 번 저장한 합을 0.5 더해 바닥으로 반올림한다', () => {
    // 분수 부분이 .25/.5/.75 인 모서리의 반올림이다(바닥(x+0.5)).
    for (const [left, expected] of [[16.25, 16], [16.5, 17], [16.75, 17], [-0.5, 0], [-0.75, -1]]) {
        const result = candidateAt(scene(), { left, top: left, right: 100, bottom: 100 }, [0, 0]);
        assert.deepEqual(result.point, { x: expected, y: expected }, String(left));
    }
    // 15.4999990463 + 1 = 16.4999990463 은 float 로 저장하면 16.5(짝수로 올림)가 되어 17 이고, 저장 없이 바닥을 하면 16 이다.
    const edge = bitsToFloat(0x4177ffff);
    const stored = candidateAt(scene(), { left: edge, top: edge, right: 100, bottom: 100 }, [1, 1]);
    assert.deepEqual(stored.point, { x: 17, y: 17 });
});

test('LocalGeyser: 제외 사각형은 네 변을 포함하고 3×3 첫 칸은 사각형 왼쪽/위와 같아도 허용하며 제외 영역이 8개를 넘으면 거부한다', () => {
    // 점은 (18, 26): 난수 dy=2, dx=2.
    const at = (exclusions, spots) => candidateAt(scene(spots), BIG, [2, 2], 0, 1, exclusions);
    assert.deepEqual(at([]).point, { x: 18, y: 26 });
    assert.equal(at([]).ok, true);
    // 각 변에 정확히 걸치면 제외, 변 밖이면 통과한다.
    for (const rectangle of [{ left: 18, top: 20, right: 25, bottom: 30 }, { left: 10, top: 20, right: 18, bottom: 30 }, { left: 10, top: 26, right: 25, bottom: 30 }, { left: 10, top: 20, right: 25, bottom: 26 }]) {
        assert.equal(at([rectangle]).ok, false, JSON.stringify(rectangle));
    }
    for (const rectangle of [{ left: 19, top: 20, right: 25, bottom: 30 }, { left: 10, top: 20, right: 17, bottom: 30 }, { left: 10, top: 27, right: 25, bottom: 30 }, { left: 10, top: 20, right: 25, bottom: 25 }]) {
        assert.equal(at([rectangle]).ok, true, JSON.stringify(rectangle));
    }
    // 여덟 개까지는 받고 아홉 개부터는 물리 범위 오류이다.
    const eight = Array.from({ length: 8 }, () => ({ left: 0, top: 0, right: 1, bottom: 1 }));
    assert.equal(at(eight).ok, true);
    throwsAs(() => at([...eight, eight[0]]), RangeError, /실제8개 물리 범위/);
    // 3×3 첫 칸(16, 24)이 사각형 왼쪽/위(16, 24)와 같아도 허용하고, 하나라도 벗어나면 거부한다.
    assert.equal(candidateAt(scene(), BIG, [2, 2]).ok, true);
    assert.equal(candidateAt(scene(), BIG, [2, 1]).ok, false);
    assert.equal(candidateAt(scene(), BIG, [1, 2]).ok, false);
});

test('LocalGeyser: spot 하위 비트 0x3d 와 땅/허공 허용·한 종류 지형 규칙', () => {
    const run = (spot, allowLand, allowVoid) => candidateAt(scene({ spot }), BIG, [2, 2], allowLand, allowVoid, []).ok;
    // 0x3d 에 속하는 비트가 있으면 무조건 거부(bit0·2·3·4·5)한다.
    for (const spot of [1, 4, 8, 16, 32, 0x3d]) assert.equal(run(spot, 1, 1), false, `spot ${spot}`);
    // 허공(0)과 땅(2)은 허용 인수대로, 상위 비트(0x40·0x80)는 종류에 영향이 없다.
    assert.equal(run(0, 0, 1), true);
    assert.equal(run(0, 1, 0), false);
    assert.equal(run(2, 0, 1), false);
    assert.equal(run(2, 1, 0), true);
    assert.equal(run(0x40, 0, 1), true);
    assert.equal(run(0x42, 1, 0), true);
    assert.equal(run(0xc2, 0, 1), false);
    // 3×3 안에서 허공과 땅이 섞이면 거부한다(첫 칸의 종류가 기준이다).
    const mixed = scene({ spot: 0 });
    mixed.spots[26 * 256 + 18] = 2;
    assert.equal(candidateAt(mixed, BIG, [2, 2], 1, 1, []).ok, false);
});

test('LocalGeyser: 지면 프레임 문자 무늬 FBICANGDH 는 보드 모서리(0..2)의 타일도 읽고 소문자를 대문자로 바꿔 비교한다', () => {
    const setup = (pattern, tileType = 97, origin = [0, 0]) => {
        const s = scene();
        const codes = [];
        // x 바깥/y 안쪽 순서의 9칸에 같은 타입의 타일을 놓는다.
        for (let index = 0; index < 9; index += 1) {
            const sid = 20 + index;
            const raw = s.pool.physical(sid);
            raw[10] = tileType;
            put(raw, 36, index);
            s.hash.setCell(0, origin[0] + Math.floor(index / 3), origin[1] + (index % 3), sid);
            codes.push({ side: pattern.charCodeAt(index), variant: 65, flags: 0 });
        }
        s.frames[tileType] = new FrameCodeTable(codes);
        return s;
    };
    const corner = { left: 0, top: 0, right: 8, bottom: 8 };
    const mid = { left: 29, top: 33, right: 40, bottom: 44 };
    // 보드 모서리 (0..2) 무늬도 인식해 거부한다. 같은 칸을 읽지 못하면(무늬 일부 누락) 통과한다.
    assert.equal(candidateAt(setup('FBICANGDH'), corner, [2, 2], 0, 1, []).ok, false);
    assert.equal(candidateAt(setup('fbicangdh'), corner, [2, 2], 0, 1, []).ok, false);
    assert.equal(candidateAt(setup('FBICANGDX'), corner, [2, 2], 0, 1, []).ok, true);
    assert.equal(candidateAt(setup('FFFFFFFFF'), corner, [2, 2], 0, 1, []).ok, true);
    // 무늬 타입이 다른 TYPE 이면 수집하지 않고, 공유 isle TYPE 이면 그 번호를 쓴다.
    const other = setup('FBICANGDH', 150, [31, 35]);
    assert.equal(candidateAt(other, mid, [4, 4], 0, 1, []).ok, true);
    other.state.sharedIsleType = () => 150;
    assert.equal(candidateAt(other, mid, [4, 4], 0, 1, []).ok, false);
});

test('LocalGeyser: 지면 hash0 조회는 보드 오른쪽 밖(x=256)을 다음 행 첫 칸이 아니라 0 번 슬롯으로 읽는다', () => {
    const s = scene();
    s.state.isleType = 97;
    const letters = 'FBICANGDH';
    const codes = [];
    // 3×3(x 254..256, y 250..252) 중 x=254,255 의 여섯 칸은 제자리, x=256 의 세 칸 자리에는 다음 행 첫 칸(0, 251..253)에 같은 무늬 글자 타일을 놓는다.
    for (let index = 0; index < 9; index += 1) {
        const sid = 20 + index;
        const raw = s.pool.physical(sid);
        raw[10] = 97;
        put(raw, 36, index);
        if (index < 6) s.hash.setCell(0, 254 + Math.floor(index / 3), 250 + (index % 3), sid);
        else s.hash.setCell(0, 0, 251 + (index - 6), sid);
        codes.push({ side: letters.charCodeAt(index), variant: 65, flags: 0 });
    }
    s.frames[97] = new FrameCodeTable(codes);
    // 보드 밖 칸은 0 번 슬롯(TYPE 0)이라 글자가 모이지 않아 무늬가 완성되지 않는다.
    assert.equal(candidateAt(s, { left: 254, top: 250, right: 262, bottom: 258 }, [2, 2], 0, 1, []).ok, true);
});

test('LocalGeyser: 지면 TYPE 이 프레임 표 밖이거나 프레임 번호가 코드 표 밖이면 읽기 전에 물리 범위 오류이다', () => {
    const run = (tileType, frame, codeCount) => {
        const s = scene();
        s.state.isleType = tileType;
        const raw = s.pool.physical(20);
        raw[10] = tileType;
        put(raw, 36, frame);
        s.hash.setCell(0, 16, 24, 20);
        if (tileType < 188) s.frames[tileType] = new FrameCodeTable(Array.from({ length: codeCount }, () => ({ side: 70, variant: 65, flags: 0 })));
        return () => candidateAt(s, BIG, [2, 2], 0, 1, []);
    };
    throwsAs(run(200, 0, 0), RangeError, /isle프레임 물리 범위/);
    throwsAs(run(97, 4, 4), RangeError, /isle프레임 물리 범위/);
    throwsAs(run(97, 0xffffffff, 4), RangeError, /isle프레임 물리 범위/);
    throwsAs(run(97, 5, 4), RangeError, /isle프레임 물리 범위/);
    assert.equal(run(97, 3, 4)().ok, true);
});

test('LocalGeyser: spot 읽기는 표 앞뒤 물리 범위를 벗어나면 거부한다', () => {
    // 첫 칸이 (-8, -8) 이면 음수 위치, (0, 256) 이면 정확히 표 길이(65536)이다.
    throwsAs(() => candidateAt(scene(), { left: -8, top: -8, right: 0, bottom: 0 }, [2, 2], 0, 1, []), RangeError, /spot 물리 범위/);
    throwsAs(() => candidateAt(scene(), { left: 0, top: 256, right: 8, bottom: 264 }, [2, 2], 0, 1, []), RangeError, /spot 물리 범위/);
    // 마지막 칸 (256, 255) 가 정확히 표 길이(65536)인 경우도 거부하고, (255, 255) 까지는 읽을 수 있다.
    throwsAs(() => candidateAt(scene(), { left: 254, top: 253, right: 262, bottom: 262 }, [2, 2], 0, 1, []), RangeError, /spot 물리 범위/);
    assert.equal(candidateAt(scene(), { left: 253, top: 253, right: 260, bottom: 260 }, [2, 2], 0, 1, []).ok, true);
});

test('LocalGeyser: 기존 빈 간헐천 재사용은 그 슬롯의 좌표를 그대로 쓰고 TYPE 정확 일치만 인정하며 표면 부류는 hash0 도 찾는다', () => {
    // hash 3단계 모든 칸에 슬롯 9 를 놓는다(넓은 발자국). 슬롯 TYPE 이 빈 간헐천(84)이면 재사용한다.
    const withEmpty = (typeNumber, flags2 = 0, level = 3) => {
        const s = scene();
        s.types[84].flags2 = flags2;
        s.types[typeNumber].footX = 256;
        s.types[typeNumber].footY = 256;
        const raw = s.pool.physical(9);
        put(raw, 4, 0, 2);
        raw[10] = typeNumber;
        raw[40] = 0;
        put(raw, 14, floatToBits(30.5));
        put(raw, 18, floatToBits(31.25));
        if (level === 3) for (let y = 0; y < 16; y += 1) for (let x = 0; x < 16; x += 1) s.hash.setCell(3, x, y, 9);
        else s.hash.setCell(0, 16, 24, 9);
        return s;
    };
    // 좌표 x, y 를 구별해 읽는다.
    const found = candidateAt(withEmpty(84), BIG, [2, 2], 0, 1, []);
    assert.ok(found.ok && found.point.x === 30.5 && found.point.y === 31.25);
    // 다른 TYPE 의 슬롯은 무시하고 평범한 후보 점을 쓴다.
    const other = candidateAt(withEmpty(85), BIG, [2, 2], 0, 1, []);
    assert.deepEqual(other.point, { x: 18, y: 26 });
    // 표면/다리 부류(flags2 & 6)면 hash0 도 조회하고, 아니면 건너뛴다(flags2 의 비트2/비트4 모두 인정).
    for (const flags2 of [2, 4, 6]) {
        const surface = candidateAt(withEmpty(84, flags2, 0), BIG, [2, 2], 0, 1, []);
        assert.deepEqual(surface.point, { x: 30.5, y: 31.25 }, `flags2 ${flags2}`);
    }
    for (const flags2 of [0, 1, 8]) assert.deepEqual(candidateAt(withEmpty(84, flags2, 0), BIG, [2, 2], 0, 1, []).point, { x: 18, y: 26 }, `flags2 ${flags2}`);
    // 현재 빈 간헐천 TYPE 이 표 밖이면 거부한다.
    const out = scene();
    out.state.emptyGeyserType = 188;
    throwsAs(() => candidateAt(out, BIG, [2, 2], 0, 1, []), RangeError, /현재 empty타입 범위/);
});

test('LocalGeyser: 확정·생성·자원은 공유 getter 의 현재 TYPE 과 현재 자원을 쓰고 spot bit1 이 없을 때만 받침을 확정한다', () => {
    const run = (spot, mutate = () => {}) => {
        const confirms = [];
        const creates = [];
        const state = new LocalGeyserState();
        state.sharedNoIslandType = () => 200;
        state.sharedGeyserType = () => 123;
        const s = scene({ spot, state, hooks: {
            confirm: (request) => confirms.push(request.type),
            create: (type) => { creates.push(type); return 50 + creates.length; },
            pop: () => {},
        } });
        s.types[123].cost = 1000;
        s.state.resourcePool = 5000;
        mutate(s);
        s.body.populate(1, stubRandom([2, 2]), BIG, { allowLand: 1, allowVoid: 1 });
        return { confirms, creates, pool: s.body.readResourcePool(), recent: s.recent };
    };
    // 허공(spot 0)은 받침을 확정하고, 땅(spot 2)과 상위 비트가 있는 땅(0x42)은 확정하지 않는다.
    assert.deepEqual(run(0).confirms, [200]);
    assert.deepEqual(run(2).confirms, []);
    assert.deepEqual(run(0x42).confirms, []);
    const result = run(0);
    assert.deepEqual(result.creates, [123]);
    assert.equal(result.pool, 4000);
    assert.ok(result.recent.count === 1 && result.recent.entries[0] === 51);
    // 현재 TYPE 이 표 밖(188)이면 비용 조회 전에 거부한다.
    throwsAs(() => run(0, (s) => { s.state.sharedGeyserType = () => 188; }), RangeError, /현재 cost타입 범위/);
    // 실제 factory 가 0 을 돌려주면 거부하고, 최근 목록 개수가 음수(signed)로 바뀌면 물리 범위 오류이다.
    const failing = scene({ hooks: { create: () => 0 } });
    throwsAs(() => failing.body.populate(1, stubRandom([2, 2]), BIG, { allowLand: 1 }), Error, /factory 생성 실패/);
    const again = scene({ hooks: { create: () => 51, pop: () => { again.recent.count = 0xffffffff; } } });
    throwsAs(() => again.body.populate(1, stubRandom([2, 2]), BIG), RangeError, /최근목록 물리 범위/);
});

test('LocalGeyser: 받침 확정 TYPE/빈 간헐천 재사용/수량은 모두 공유 getter 의 현재 값을 읽는다', () => {
    // 빈 간헐천 TYPE 을 공유 getter(85)로 바꾸면 TYPE 85 슬롯이 교체 대상이 된다(독립 필드는 84 로 남는다).
    const destroys = [];
    const state = new LocalGeyserState();
    state.sharedEmptyGeyserType = () => 85;
    const s = scene({ state, hooks: { destroy: (sid, flags) => destroys.push([sid, flags]), create: () => 51, pop: () => {} } });
    s.types[85].footX = 256;
    s.types[85].footY = 256;
    const raw = s.pool.physical(9);
    put(raw, 4, 0, 2);
    raw[10] = 85;
    raw[40] = 0;
    put(raw, 14, floatToBits(30.5));
    put(raw, 18, floatToBits(31.25));
    for (let y = 0; y < 16; y += 1) for (let x = 0; x < 16; x += 1) s.hash.setCell(3, x, y, 9);
    s.body.populate(1, stubRandom([2, 2]), BIG);
    assert.deepEqual(destroys, [[9, 0x400000]]);
    // 전체 생성은 Player 1~4 와 5~8 의 현재 수량 전역을 각각 읽는다(독립 수량 1/1 이어도 getter 가 2/3 이면 5개를 만든다).
    const quantities = new LocalGeyserState();
    quantities.firstPlayerQuantity = 1;
    quantities.lastPlayerQuantity = 1;
    quantities.sharedFirstQuantity = () => 2;
    quantities.sharedLastQuantity = () => 3;
    const players = new Uint8Array(9 * STRIDE);
    for (const owner of [1, 5]) {
        put(players, owner * STRIDE + 0x50, 0x22, 1);
        put(players, owner * STRIDE + 0x78, owner);
    }
    let created = 0;
    const generated = scene({ state: quantities, players, hooks: { create: () => { created += 1; return 51; }, pop: () => {} } });
    generated.environment.predictable = 1;
    generated.body.generate();
    assert.equal(generated.environment.trace.filter((text) => text.startsWith('Bounds:')).length, 2);
    assert.equal(created, 5);
    // predictableGeysers 는 0 일 때만 현재 TimeSeed 를 쓴다(2·-1 은 고정 시드 0xfed).
    for (const [value, usesTime] of [[0, true], [1, false], [2, false], [-1, false]]) {
        const timed = scene({ state: new LocalGeyserState(), players: new Uint8Array(9 * STRIDE) });
        timed.environment.predictable = value;
        timed.body.generate();
        assert.equal(timed.environment.trace.includes('Time'), usesTime, `predictable ${value}`);
    }
});

test('LocalGeyser: 제외 영역은 지역 DWORD 를 signed 로 전달하고 float 로 저장한 확장 경계를 만든다', () => {
    const players = new Uint8Array(9 * STRIDE);
    put(players, STRIDE + 0x50, 1, 1);
    put(players, STRIDE + 0x78, 0xffffffff);
    const regions = [];
    const setup = scene({ players, hooks: {
        regionExists: (region) => { regions.push(region); return true; },
        playerBounds: () => ({ left: -bitsToFloat(0x42ffffff), top: -bitsToFloat(0x42ffffff), right: bitsToFloat(0x41ffffff), bottom: bitsToFloat(0x41ffffff) }),
    } });
    const areas = setup.body.playerExclusions();
    assert.deepEqual(regions, [-1]);
    // -127.99999237 - 12 = -139.99999237 은 float 로 저장하면 -140 이고, 31.99999809 + 12 = 43.99999809 는 44 이다.
    assert.deepEqual(areas, [{ left: -140, top: -140, right: 44, bottom: 44 }]);
});
