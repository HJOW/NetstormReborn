// AI 배치 실행 raw-ai-deployment.js(RawAiDeployment)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 생산 소비 → 대기 항목/Regular → 비용 → 10인자 서버 요청 → 이동 해제/종류별 기한 순서, new 실패는 대기 조각/비용/배치를 되돌리거나 중단하지 않음,
//    생산이 없으면 일반 타입은 무료 배치하고 시설 flags 4200 만 비용 지불, 원본 진단이 반환하면 현재 flags2 를 다시 읽어 비용 여부 결정, 비권한 경로는 별도 예측 조각을 만들어 로컬 5인자 배치로 전달, signed 비용 절삭/감김/데모 재조회와 물리 TYPE/본체 경계,
//    선택된 누락 하위는 앞선 쓰기를 유지한 채 실패하며 이동 플래그를 임의 복구하지 않음, 예약 뒤 현재 rate/비용/owner 와 요청 뒤 현재 시각을 각각 다시 읽음.
//  - 같은 풀의 실제 `RawContainedCreate` 로 대기/예측 내용물을 생성한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E175: AI 배치 실행 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { INDEPENDENT_CONTAINED_STATE, RawContainedCreate } from '../www/core/contained-create.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiDeployment } from '../www/core/raw-ai-deployment.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E175)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const F = Math.fround;

// 실제 내용물 생성과 명시 월드 효과를 한 풀/본체에 연결한다. (2026-11-09 WB04-E175)
class DeploymentInput {
    // 생산 항목과 기존 생성기를 같은 실제 저장소에 준비한다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x45000000, (raw) => raw.fill(0));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.type = 133;
        this.pool = new SidPool(32768, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags2: 0 }));
        this.born = [];
        this.authority = true;
        this.move = { value: false };
        this.reserve = true;
        this.demo = false;
        this.now = 16777216.25;
        this.trace = [];
        this.assertions = [];
        this.requests = [];
        this.locals = [];
        this.timers = [];
        this.owners = [];
        this.parent = this.newSlot();
        this.product = this.newSlot();
        this.types[this.type].cost = F(123.75);
        this.types[this.type].flags2 = 0x4a00;
        this.field(0x260, 2);
        this.field(0x2d8, 1000);
        this.field(0x224, floatToBits(2.5));
        this.create = new RawContainedCreate(this.pool, INDEPENDENT_CONTAINED_STATE, {
            create: (number, flags) => {
                assert.ok(number === 6 && flags === 2);
                const form = this.newSlot();
                this.pool.allocated(form)[10] = 6;
                this.born.push(form);
                return form;
            },
            notifySurface: () => { throw new Error('검사 외 표면 통지'); },
            attach: (form, owner, flags) => {
                assert.ok(owner === this.parent && flags === 0x10);
                put(this.pool.allocated(form), 4, get(this.pool.slot(this.parent), 6, 2), 2);
                put(this.pool.allocated(this.parent), 6, form, 2);
                this.trace.push('create');
            }
        });
        this.hooks = {
            product: (brain, number) => { assert.ok(brain === this.token && number === this.type); this.trace.push('product'); return this.product; },
            technology: (owner) => { this.owners.push(owner); this.trace.push('technology'); return this.parent; },
            destroy: (form, flags) => { assert.ok(form === this.product && flags === 0); this.product = 0; this.trace.push('destroy'); },
            reserveRegular: () => { this.trace.push('reserve'); return this.reserve; },
            addRegular: (form, rate) => { this.timers.push([form, rate]); this.trace.push('timer'); },
            request: (request) => { assert.ok(this.move.value); this.requests.push(request); this.trace.push('server'); },
            localPlace: (request) => { assert.ok(this.move.value); this.locals.push(request); this.trace.push('local'); },
            demo: () => { this.trace.push('demo'); return this.demo; },
            assertion: (text) => { this.assertions.push(text); this.trace.push('assert'); }
        };
    }

    // 실제 할당 블록을 명시 0 입력으로 준비한다. (2026-11-09 WB04-E175)
    newSlot() {
        const sid = this.pool.allocate(2);
        this.pool.allocated(sid).fill(0);
        return sid;
    }

    // 현재 본체 DWORD 를 읽거나(값 없음) 해당 DWORD 만 쓴다. (2026-11-09 WB04-E175)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), at);
        put(this.memory.bytes(this.token), at, value >>> 0);
        return undefined;
    }

    // 현재 QWORD 기한을 읽는다. (2026-11-09 WB04-E175)
    time(at) {
        const view = new DataView(new ArrayBuffer(8));
        view.setUint32(0, this.field(at), true);
        view.setUint32(4, this.field(at + 4), true);
        return view.getFloat64(0, true);
    }

    // 같은 본체/실제 내용물/현재 전역을 조립한다. (2026-11-09 WB04-E175)
    make() { return new RawAiDeployment(this.memory, this.pool, this.types, this.create, { authority: () => this.authority, moveLocalBuilder: this.move, now: () => this.now }, this.hooks); }
}

test('AI 배치 실행: 기존 생산 소비 → 대기 항목/Regular → 비용 → 10인자 서버 요청 → 이동 해제/종류별 기한 순서다', () => {
    const input = new DeploymentInput();
    input.make().place(input.token, input.type, F(40.25), F(41.5), 3, 7);
    assert.deepEqual(input.trace, ['product', 'destroy', 'technology', 'create', 'reserve', 'timer', 'demo', 'server']);
    assert.ok(input.born.length === 1 && input.timers.length === 1 && input.requests.length === 1 && input.field(0x2d8) === 877 && !input.move.value);
    assert.ok(get(input.pool.slot(input.born[0]), 20, 2) === 0x108 && get(input.pool.slot(input.born[0]), 18, 2) === 133);
    assert.ok(input.timers[0][0] === input.born[0] && input.timers[0][1] === 2.5);
    const request = input.requests[0];
    assert.ok(request.type === 133 && request.x === F(40.25) && request.y === F(41.5) && request.player === 2 && request.argument === 3 && request.direction === 7 && !request.flags && !request.timeLow && !request.timeHigh && !request.quality);
    assert.ok(input.time(0x5b8) === input.now + 20 && input.time(0x5c0) === input.now + 20);
});

test('AI 배치 실행: new 실패는 대기 조각/비용/배치 전체를 되돌리거나 중단하지 않는다', () => {
    const input = new DeploymentInput();
    input.reserve = false;
    input.hooks.addRegular = undefined;
    input.make().place(input.token, input.type, 10, 11, 0, 0);
    assert.ok(input.born.length === 1 && input.timers.length === 0 && input.requests.length === 1 && input.field(0x2d8) === 877 && !input.move.value);
});

test('AI 배치 실행: 생산이 없으면 일반 타입은 무료 배치하며 시설 flags 4200 만 별도 비용을 지불한다', () => {
    const plain = new DeploymentInput();
    plain.product = 0;
    plain.types[plain.type].flags2 = 0;
    plain.hooks.reserveRegular = undefined;
    plain.hooks.demo = undefined;
    plain.make().place(plain.token, plain.type, 10, 11, 0, 0);
    assert.ok(plain.field(0x2d8) === 1000 && plain.requests.length === 1 && plain.born.length === 0 && plain.time(0x5b8) === 0 && plain.time(0x5c0) === 0);
    const facility = new DeploymentInput();
    facility.product = 0;
    facility.types[facility.type].flags2 = 0x4200;
    facility.make().place(facility.token, facility.type, 10, 11, 0, 0);
    assert.ok(facility.field(0x2d8) === 877 && facility.born.length === 0 && facility.time(0x5b8) === 0 && facility.time(0x5c0) === facility.now + 20);
});

test('AI 배치 실행: 원본 진단이 반환하면 현재 flags2 를 다시 읽어 비용 여부를 결정한다', () => {
    const input = new DeploymentInput();
    input.product = 0;
    input.types[input.type].flags2 = 0x70000;
    input.hooks.assertion = (text) => { input.assertions.push(text); input.types[input.type].flags2 = 0x4200; };
    input.make().place(input.token, input.type, 10, 11, 0, 0);
    assert.ok(input.assertions.length === 1 && input.field(0x2d8) === 877 && input.requests.length === 1);
});

test('AI 배치 실행: 비권한 경로는 별도 예측 조각을 만들고 기존 로컬 5인자 배치에 그대로 전달한다', () => {
    const input = new DeploymentInput();
    input.authority = false;
    input.hooks.request = undefined;
    input.make().place(input.token, input.type, F(17.25), F(29.5), 255, 0x123);
    assert.ok(input.requests.length === 0 && input.locals.length === 1 && input.born.length === 2 && input.timers.length === 1 && !input.move.value);
    const request = input.locals[0];
    assert.ok(request.preview === input.born[1] && request.x === F(17.25) && request.y === F(29.5) && request.argument === 255 && request.direction === 0x123 && get(input.pool.slot(request.preview), 20, 2) === 0x108);
});

test('AI 배치 실행: signed 비용 절삭/감김/데모 재조회와 물리 TYPE/본체 경계를 확인한다', () => {
    const input = new DeploymentInput();
    const deployment = input.make();
    input.field(0x2d8, 123);
    assert.ok(deployment.canAfford(input.token, input.type));
    input.field(0x2d8, 122);
    assert.ok(!deployment.canAfford(input.token, input.type));
    input.types[input.type].cost = F(-1.9);
    deployment.charge(input.token, input.type);
    assert.equal(input.field(0x2d8), 123);
    input.types[input.type].cost = Number.NaN;
    deployment.charge(input.token, input.type);
    assert.equal(input.field(0x2d8), 123);
    input.types[input.type].cost = 1;
    input.field(0x2d8, 0x80000000);
    deployment.charge(input.token, input.type);
    assert.equal(input.field(0x2d8), 0x7fffffff);
    input.hooks.demo = () => { input.field(0x2d8, 1191); return true; };
    input.make().charge(input.token, input.type);
    assert.equal(input.field(0x2d8), 1191);
    assert.ok(throws(() => input.make().canAfford(input.token, 188)));
    const small = input.memory.allocate(4);
    assert.ok(throws(() => input.make().charge(small, input.type)));
});

test('AI 배치 실행: 선택된 누락 하위는 앞선 실제 쓰기를 유지한 채 실패하며 원본 이동 플래그를 임의 복구하지 않는다', () => {
    const input = new DeploymentInput();
    input.hooks.request = undefined;
    assert.ok(throws(() => input.make().place(input.token, input.type, 10, 11, 0, 0)));
    assert.ok(input.move.value && input.born.length === 1 && input.field(0x2d8) === 877 && input.time(0x5b8) === 0);
    const missing = new DeploymentInput();
    missing.hooks.product = undefined;
    assert.ok(throws(() => missing.make().place(missing.token, missing.type, 10, 11, 0, 0)));
    assert.ok(!missing.move.value && missing.born.length === 0 && missing.field(0x2d8) === 1000);
});

test('AI 배치 실행: 예약 뒤 현재 rate/비용/owner 와 실제 요청 뒤 현재 시각을 각각 다시 읽는다', () => {
    const input = new DeploymentInput();
    input.hooks.reserveRegular = () => { input.field(0x224, floatToBits(5.5)); input.field(0x260, 3); return true; };
    input.hooks.addRegular = (form, duration) => { input.timers.push([form, duration]); input.types[input.type].cost = F(12.5); };
    input.hooks.request = (request) => { assert.ok(input.move.value); input.requests.push(request); input.now += 1; };
    input.make().place(input.token, input.type, 10, 11, 0, 0);
    assert.ok(JSON.stringify(input.owners) === '[2]' && input.timers[0][1] === 5.5 && input.requests[0].player === 3 && input.field(0x2d8) === 988 && input.time(0x5b8) === input.now + 20 && !input.move.value);
});

test('AI 배치 실행 생성자: 본체 소유자/같은 PATCH 풀·내용물 생성/TYPE 표/현재 전역이 없으면 거부한다', () => {
    const input = new DeploymentInput();
    const globals = { authority: () => true, moveLocalBuilder: { value: false }, now: () => 0 };
    assert.ok(throws(() => new RawAiDeployment(undefined, input.pool, input.types, input.create, globals, input.hooks)));
    assert.ok(throws(() => new RawAiDeployment(input.memory, new SidPool(32768, { server: true }), input.types, input.create, globals, input.hooks)));
    assert.ok(throws(() => new RawAiDeployment(input.memory, input.pool, [], input.create, globals, input.hooks)));
    assert.ok(throws(() => new RawAiDeployment(input.memory, input.pool, input.types, input.create, { ...globals, authority: undefined }, input.hooks)));
    assert.ok(throws(() => new RawAiDeployment(input.memory, input.pool, input.types, input.create, { ...globals, moveLocalBuilder: undefined }, input.hooks)));
    assert.equal(input.make().pool, input.pool);
});
