// AI 이동 raw-ai-movement.js(RawAiMovement·aiDistanceToFootprint)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 현재 TYPE → 순번 0/remove 인자와 정지 0 의 무동작, 서버 실패/희생은 이름을 읽지 않고 성공은 대상 → 출발 이름을 디버그 조회 전에 읽음,
//    비서버의 실제 접근점/현재 헤더 18바이트와 사용자 선택/중복 목적지 관문 없음, 송신 뒤 현재 권한 재조회, 시작 인자의 나머지 0 과 로그 좌표 ftol, 거리 2 이상/unordered 복귀와 효과 뒤 현재 신전 재조회, 발자국 거리의 음수 내부값/x float 저장/unordered x 선택,
//    누락 하위가 먼저 발생한 송신/경로 효과를 보존.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E167: AI 이동 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { BoardTargetState, RawBoardTarget } from '../www/core/raw-board-target.js';
import { CarrierMoveState } from '../www/core/raw-carrier-move.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiMovement, aiDistanceToFootprint } from '../www/core/raw-ai-movement.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E167)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 같은 실제 raw 풀/본체와 명시 경로·전송·이름 장치를 구성한다. (2026-11-09 WB04-E167)
class AiMoveScene {
    // 원본 입력과 관찰할 하위만 준비한다.
    constructor() {
        this.pool = new SidPool(32768, { server: true });
        this.memory = new RawAiBrainMemory(0x4a000000, (raw) => raw.fill(0));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.boss = true;
        this.path = 1;
        this.distance = 2;
        this.packet = new CarrierMoveState();
        this.board = new RawBoardTarget(new BoardTargetState());
        this.trace = [];
        this.packets = [];
        this.started = null;
        this.moved = 0;
        this.logX = 0;
        this.logY = 0;
        this.sourceName = '';
        this.targetName = '';
        this.actor = this.make();
        this.target = this.make();
        this.other = this.make();
        const raw = this.pool.allocated(this.actor);
        raw[10] = 158;
        put(raw, 14, floatToBits(12));
        put(raw, 18, floatToBits(5));
        this.field(0x260, 2);
        this.field(0xb00, 1);
        this.field(0x294, this.target);
        this.hooks = {
            movement: (type) => { assert.equal(type, 158); this.trace.push('movement'); return 54; },
            lookup: (parent, type, index, remove) => { assert.ok(parent === this.actor && type === 54 && index === 0); this.trace.push(remove ? 'remove' : 'lookup'); return 50; },
            footprint: (sid) => { assert.equal(sid, this.target); this.trace.push('footprint'); return { left: 10, top: 10, right: 14, bottom: 14 }; },
            send: (channel, bytes) => { assert.equal(channel, 9); this.trace.push('send'); this.packets.push(Buffer.from(bytes)); },
            sacrifice: (parent, sid) => { assert.ok(parent === this.actor && sid === this.target); this.trace.push('sacrifice'); },
            target: (parent, sid, replace, helper) => { assert.ok(parent === this.actor && !replace && !helper); this.moved = sid; this.trace.push('target'); return this.path; },
            start: (request) => { this.started = request; this.trace.push('start'); return this.path; },
            name: (sid) => { this.trace.push(`name:${sid}`); return sid === this.actor ? 'actor' : `target${sid}`; },
            logTarget: (brain, source, destination) => { assert.equal(brain, this.token); this.sourceName = source; this.targetName = destination; this.trace.push('logTarget'); },
            logPoint: (brain, name, x, y) => { assert.equal(brain, this.token); this.sourceName = name; this.logX = x; this.logY = y; this.trace.push('logPoint'); },
            distance: (point, sid) => { assert.ok(point.x === 12 && point.y === 5 && sid === this.target); this.trace.push('distance'); return this.distance; }
        };
    }

    // 같은 실제 슬롯의 payload 만 명시 0 으로 준비한다. (2026-11-09 WB04-E167)
    make() {
        const sid = this.pool.allocate(2);
        this.pool.allocated(sid).fill(0);
        return sid;
    }

    // 선택된 현재 본체 DWORD 만 쓴다. (2026-11-09 WB04-E167)
    field(at, value) { put(this.memory.bytes(this.token), at, value >>> 0); }

    // 같은 현재 권한/기하/헤더를 빌린다. (2026-11-09 WB04-E167)
    movement() { return new RawAiMovement(this.memory, this.pool, () => this.boss, this.board, this.packet, this.hooks); }
}

// 현재 TYPE → 순번 0/remove 를 전달하며 정지 0 은 하위가 없어도 무동작이다. (2026-11-09 WB04-E167)
test('AI 이동: 현재 TYPE 의 경로 조회와 제거 인자를 전달하며 정지 0 은 하위가 없어도 무동작이다', () => {
    const input = new AiMoveScene();
    assert.equal(input.movement().movementForm(input.actor), 50);
    input.movement().stop(input.actor);
    assert.deepEqual(input.trace, ['movement', 'lookup', 'movement', 'remove']);
    input.hooks = {};
    input.movement().stop(0);
    assert.equal(input.trace.length, 4);
});

test('AI 이동: 서버 실패/희생은 이름을 읽지 않고 성공은 대상 → 출발 이름을 디버그 조회 전에 읽는다', () => {
    const input = new AiMoveScene();
    input.path = 0;
    input.movement().toTarget(input.token, input.actor, input.target, 0);
    assert.deepEqual(input.trace, ['target']);
    input.trace.length = 0;
    input.movement().toTarget(input.token, input.actor, input.target, 1);
    assert.deepEqual(input.trace, ['sacrifice']);
    input.trace.length = 0;
    input.path = 1;
    input.movement().toTarget(input.token, input.actor, input.target, 0);
    assert.deepEqual(input.trace, ['target', `name:${input.target}`, `name:${input.actor}`, 'logTarget']);
    assert.ok(input.sourceName === 'actor' && input.targetName === `target${input.target}`);
    // 이름 조회 중 디버그 로그가 꺼지면 로그를 부르지 않는다.
    input.trace.length = 0;
    input.hooks.name = (sid) => { input.trace.push(`name:${sid}`); input.field(0xb00, 0); return 'current'; };
    input.hooks.logTarget = undefined;
    input.movement().toTarget(input.token, input.actor, input.target, 0);
    assert.deepEqual(input.trace, ['target', `name:${input.target}`, `name:${input.actor}`]);
});

test('AI 이동: 비서버는 실제 접근점/현재 헤더의 18바이트를 보내며 사용자 선택/중복 목적지 관문은 없다', () => {
    const input = new AiMoveScene();
    input.boss = false;
    input.packet.request = { message: 7, protocol: 0xf3, length: 2 };
    input.movement().toTarget(input.token, input.actor, input.target, 0xdeadbeef);
    assert.deepEqual(input.trace, ['footprint', 'send', `name:${input.target}`, `name:${input.actor}`, 'logTarget']);
    assert.equal(input.packets.length, 1);
    const bytes = input.packets[0];
    assert.ok(bytes.length === 18 && bytes[0] === 0x73 && bytes[1] === 7 && get(bytes, 2, 2) === 2 && get(bytes, 4, 2) === input.actor
        && get(bytes, 6) === floatToBits(12) && get(bytes, 10) === floatToBits(10) && get(bytes, 14) === 0xdeadbeef);
});

test('AI 이동: 송신 효과 뒤 권한이 바뀌면 희생 우회 없이 현재 대상 이동/좌표 경로를 시작한다', () => {
    const target = new AiMoveScene();
    target.boss = false;
    target.hooks.send = (channel) => { assert.equal(channel, 9); target.trace.push('send'); target.boss = true; };
    target.movement().toTarget(target.token, target.actor, target.target, 1);
    assert.deepEqual(target.trace, ['footprint', 'send', 'target', `name:${target.target}`, `name:${target.actor}`, 'logTarget']);
    const point = new AiMoveScene();
    point.boss = false;
    point.hooks.send = () => { point.trace.push('send'); point.boss = true; };
    point.path = 0;
    point.movement().toPoint(point.token, point.actor, { x: 4, y: 5 }, 1);
    assert.deepEqual(point.trace, ['send', 'start']);
});

test('AI 이동: 실제 시작 인자는 나머지 0 이고 로그 좌표만 원본 signed 절삭을 적용한다', () => {
    const input = new AiMoveScene();
    input.movement().toPoint(input.token, input.actor, { x: Math.fround(1.9), y: Math.fround(-2.9) }, 0xdeadbeef);
    const started = input.started;
    assert.ok(started.parent === input.actor && started.destination.x === Math.fround(1.9) && started.destination.y === Math.fround(-2.9)
        && !started.replaceStart && !started.helper && !started.tailWord && !started.tailByte && !started.reserved);
    assert.deepEqual(input.trace, ['start', `name:${input.actor}`, 'logPoint']);
    assert.ok(input.logX === 1 && input.logY === -2);
    input.movement().toPoint(input.token, input.actor, { x: Number.NaN, y: Number.POSITIVE_INFINITY }, 0);
    assert.ok(input.logX === 0 && input.logY === 0);
});

test('AI 이동: 거리 2 이상과 unordered 는 복귀하며 거리 효과 뒤 현재 신전 SID 를 다시 읽는다', () => {
    const input = new AiMoveScene();
    input.distance = 1.999;
    input.movement().home(input.token, input.actor);
    assert.deepEqual(input.trace, ['distance']);
    input.trace.length = 0;
    input.distance = 2;
    input.movement().home(input.token, input.actor);
    assert.ok(input.moved === input.target && input.trace[0] === 'distance');
    input.trace.length = 0;
    input.hooks.distance = (point, temple) => {
        assert.ok(point.x === 12 && point.y === 5 && temple === input.target);
        input.trace.push('distance');
        input.field(0x294, input.other);
        put(input.pool.allocated(input.actor), 14, floatToBits(99));
        return Number.NaN;
    };
    input.movement().home(input.token, input.actor);
    assert.ok(input.moved === input.other && input.trace[0] === 'distance');
    input.trace.length = 0;
    input.field(0x294, 0);
    input.hooks = {};
    input.movement().home(input.token, input.actor);
    assert.equal(input.trace.length, 0);
});

test('AI 이동: 발자국 거리는 원본 음수 내부값/x float 저장/unordered x 선택을 따른다', () => {
    const rectangle = { left: 10, top: 10, right: 14, bottom: 14 };
    assert.equal(aiDistanceToFootprint({ x: 12, y: 12 }, rectangle), -2);
    assert.ok(aiDistanceToFootprint({ x: 8, y: 12 }, rectangle) === 2 && aiDistanceToFootprint({ x: 12, y: 8 }, rectangle) === 2);
    assert.ok(Number.isNaN(aiDistanceToFootprint({ x: Number.NaN, y: 8 }, rectangle)));
    assert.equal(aiDistanceToFootprint({ x: 8, y: Number.NaN }, rectangle), 2);
    // x 차이만 float 에 저장하므로 float 로 표현할 수 없는 작은 차이는 반올림된다.
    assert.equal(aiDistanceToFootprint({ x: Math.fround(9.9), y: 12 }, rectangle), Math.fround(10 - Math.fround(9.9)));
});

test('AI 이동: 누락된 이름/경로를 가짜 성공으로 바꾸지 않고 먼저 발생한 송신/경로 효과를 보존한다', () => {
    const input = new AiMoveScene();
    input.hooks.name = undefined;
    assert.ok(throws(() => input.movement().toTarget(input.token, input.actor, input.target, 0)) && JSON.stringify(input.trace) === '["target"]');
    const client = new AiMoveScene();
    client.boss = false;
    client.hooks.send = (channel, bytes) => { client.packets.push(Buffer.from(bytes)); client.boss = true; };
    client.hooks.start = undefined;
    assert.ok(throws(() => client.movement().toPoint(client.token, client.actor, { x: 4, y: 5 }, 0)) && client.packets.length === 1);
});

test('AI 이동 생성자/물리 검사: PATCH 풀·현재 권한/기하/헤더가 없으면 거부하고 가짜 본체/목표 SID 를 거부한다', () => {
    const input = new AiMoveScene();
    assert.ok(throws(() => new RawAiMovement(undefined, input.pool, () => true, input.board, input.packet, input.hooks)));
    assert.ok(throws(() => new RawAiMovement(input.memory, {}, () => true, input.board, input.packet, input.hooks)));
    assert.ok(throws(() => new RawAiMovement(input.memory, input.pool, true, input.board, input.packet, input.hooks)));
    assert.ok(throws(() => new RawAiMovement(input.memory, input.pool, () => true, undefined, input.packet, input.hooks)));
    assert.equal(input.movement().pool, input.pool);
    const small = input.memory.allocate(16);
    assert.ok(throws(() => input.movement().home(small, input.actor)));
    // 손상된 현재 신전 DWORD 는 다른 SID 로 절단하지 않고 거부한다.
    input.field(0x294, 0x10000);
    assert.ok(throws(() => input.movement().home(input.token, input.actor)));
    input.field(0x294, 32768);
    assert.ok(throws(() => input.movement().home(input.token, input.actor)));
});
