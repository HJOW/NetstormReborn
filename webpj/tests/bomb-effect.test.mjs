// 0x5f 주문 효과 수신 raw-bomb-effect.js(RawBombEffect)를 독립 원본 관측 bomb-effect-x86.tsv 1676행으로 대조한다.
// 입력 11개 DWORD: [부모 상태, 운반체·대상 상태, 운반체 SID, 부모 TYPE, 권한, 대상 수, 대상 상태 패턴, 변형 1~12, 비용 float 비트, 누적 카운터, 두 번째 확보 성공 여부].
// 변형은 시전 종료·대상 효과·지연 생성·소리·삭제 콜백이 현재 TYPE/가상 표/패킷 대상 수/권한/전역 종류를 바꾸는 명시 입력이다. 효과 문자열(호출 순서와 인자 DWORD)·열 개 raw 슬롯 전체·22바이트 패킷·누적 카운터·두 전역을 원본(두 x87 정밀도)과 비교하며 기대값은 계산하지 않는다.
// 40aca0 송신 observer(finalizeBombEffectPacket)와 Reward/Login 이 공유하는 자원 DWORD 의 unsigned 재조회는 계약 시험으로 확인한다. (2026-11-08 WB04-E54: RawBombEffect 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { finalizeBombEffectPacket, RawBombEffect } from '../www/core/raw-bomb-effect.js';
import { SidPool } from '../www/core/sid-pool.js';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, put, readRows } from './raw-support.mjs';

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E54)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// 입력 십진수를 원본 하위 32비트 수로 읽는다. (2026-11-08 WB04-E54)
const unsigned = (text) => Number(BigInt.asUintN(32, BigInt(text)));

// 원본 실행기의 실제 표와 격리 교체 표 토큰, 독립 원문이 전체 바이트를 보존한 물리 슬롯이다. (2026-11-08 WB04-E54)
const EFFECT_VTABLE = 0x502370;
const ALTERNATE_VTABLE = 0x20000800;
const EFFECT_SLOTS = [0, 50, 51, 52, 60, 61, 62, 63, 64, 65];

// 원본의 명시 외부 입력만 공급하며 수신 판단과 누적은 제품이 실행하는 장면이다. (2026-11-08 WB04-E54)
class EffectScene {
    pool = new SidPool(24000, { server: false });
    types = Array.from({ length: 188 }, () => ({ cost: 0, sound: 0, delay: 0 }));
    state = null;
    input = [];
    packet = new Uint8Array(22);
    trace = '';
    allocations = 0;
    applied = 0;

    // 슬롯의 쓰기 가능한 50바이트이다.
    raw(sid) {
        return this.pool.physical(sid).subarray(0, 50);
    }

    // 각 효과의 실제 인자를 원문과 같은 표현으로 기록한다.
    event(name, args = []) {
        if (this.trace) this.trace += ';';
        this.trace += name;
        // DWORD 인자의 순서와 폭을 유지한다.
        for (const value of args) this.trace += `:${value >>> 0}`;
    }

    // 입력을 설치하며 예상 순회/선택/누적 결과는 계산하지 않는다.
    prepare(text) {
        this.trace = '';
        this.allocations = 0;
        this.applied = 0;
        this.input = text.split(',').map(unsigned);
        assert.equal(this.input.length, 11);
        const input = this.input;
        this.state = { boss: input[4], firstType: 117, secondType: 118, processType: 57, spentCounter: input[9], sharedSpentCounter: null };
        // 원문의 10개 물리 입력 슬롯과 상태 패턴만 그대로 공급한다.
        for (const number of EFFECT_SLOTS) {
            const raw = this.raw(number);
            raw.fill(0);
            put(raw, 0, EFFECT_VTABLE, 4);
            raw[10] = number === 50 ? input[3] & 0xff : 133;
            if (number === 50) raw[11] = input[0] & 0xff;
            else if (number === 0 || number === 51 || number === 52) raw[11] = input[1] & 0xff;
            else if (input[6] === 1 && number % 2) raw[11] = 1;
            else raw[11] = input[6] === 2 ? 2 : 0;
        }
        put(this.raw(50), 12, input[2], 2);
        // 세 TYPE 의 명시 비용/소리/지연 입력이다. 결과 비용 절삭은 제품만 수행한다.
        for (const kind of [117, 118, 168]) this.types[kind] = { cost: kind === input[3] ? bitsToFloat(input[8]) : 33.75, sound: 1000 + kind, delay: kind === 117 ? 9 : kind === 118 ? 11 : 13 };
        this.packet.fill(0);
        this.packet[0] = 1;
        this.packet[1] = 0x5f;
        put(this.packet, 2, 22, 2);
        put(this.packet, 4, 50, 2);
        put(this.packet, 6, input[5], 4);
        // 중복/0번 대상은 원문에 제공한 입력 목록이며 거부/선택 결과가 아니다.
        for (let index = 0; index < 6; index += 1) put(this.packet, 10 + 2 * index, input[6] === 3 ? [60, 60, 0, 63, 64, 65][index] : 60 + index, 2);
    }

    // 제품 제어 흐름과 별개로 원본에 선언한 큰 하위 효과만 재생한다.
    hooks() {
        const input = this.input;
        return {
            // 시전 종료 뒤 현재 TYPE/운반체가 바뀌는 명시 입력이다.
            stopCast: (carrier) => {
                this.event('stop-cast', [carrier]);
                if (input[7] === 1) this.raw(50)[10] = 118;
                if (input[7] === 11) put(this.raw(50), 12, 52, 2);
            },
            // 두 확보 경계의 순서와 두 번째 확보 실패만 공급한다.
            allocate: (bytes) => {
                this.event('allocate', [bytes]);
                this.allocations += 1;
                return this.allocations === 1 || input[10] !== 0;
            },
            // 매 호출에 제품이 전달한 현재 가상 표를 사용한다.
            apply: (parent, table, target) => {
                assert.equal(parent, 50);
                assert.ok(table === EFFECT_VTABLE || table === ALTERNATE_VTABLE);
                this.event(table === EFFECT_VTABLE ? 'apply' : 'apply-alt', [parent, target]);
                this.applied += 1;
                if (this.applied !== 1) return;
                if (input[7] === 2) put(this.raw(parent), 0, ALTERNATE_VTABLE, 4);
                if (input[7] === 3) {
                    this.raw(62)[11] = 2;
                    this.raw(target)[11] = 2;
                }
                if (input[7] === 4) put(this.packet, 6, 1, 4);
                if (input[7] === 5) put(this.packet, 6, 5, 4);
                if (input[7] === 6) this.state.boss = 0;
                if (input[7] === 7) this.raw(parent)[10] = 118;
                if (input[7] === 12) this.state.secondType = 168;
            },
            // 지연 생성 경계에서 관측한 인자와 저장 목록을 보존한다.
            construct: (delay, capacity, targets, kind) => {
                this.event('construct', [delay, kind, capacity, targets.length]);
                // 중복/0번과 효과 뒤 dead 대상도 받은 순서 그대로 기록한다.
                for (const target of targets) this.trace += `:${target}`;
                if (input[7] === 8) this.raw(50)[10] = 168;
                return 1;
            },
            // 실제 예약 인자 계산은 독립 원문에도 선언한 경계다.
            argument: (first, second) => {
                this.event('schedule-argument', [first, second]);
                return 777;
            },
            // 확보 실패 때 객체 0 경계 호출도 기록한다. 실제 NULL 객체 부착의 성공을 가정하지 않는다.
            attach: (process, kind, argument, first, second) => {
                assert.ok(process <= 1);
                this.event('attach', [process, kind, argument, first, second]);
            },
            // 소리 효과가 최종 삭제 가상 표를 바꾸는 입력을 공급한다.
            sound: (args) => {
                this.event('sound');
                // 원본의 여섯 인자를 동일한 폭으로 기록한다.
                for (const value of args) this.trace += `:${value}`;
                if (input[7] === 9) put(this.raw(50), 0, ALTERNATE_VTABLE, 4);
            },
            // 제품이 소리 뒤 읽은 현재 가상 표와 삭제 flags 만 사용한다.
            destroy: (parent, table, flags) => {
                assert.ok(table === EFFECT_VTABLE || table === ALTERNATE_VTABLE);
                this.event(table === EFFECT_VTABLE ? 'destroy' : 'destroy-alt', [parent, flags]);
                if (input[7] === 10) this.raw(parent)[11] = 2;
            },
            // 최초 대상 배열을 해제하는 외부 효과다.
            release: () => this.event('release', [1]),
        };
    }

    // 독립 원문과 같은 10슬롯 전체를 조립하며 상태를 해석하지 않는다.
    raws() {
        return EFFECT_SLOTS.map((number) => hex(this.raw(number))).join(',');
    }
}

test('bomb-effect: 시전 종료·대상 효과·지연 타이머·소리·삭제와 누적/전역 재조회를 원본 1676행과 대조한다', () => {
    const scene = new EffectScene();
    const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'bomb-effect-x86.tsv'));
    let count = 0;
    // 제품을 매 독립 입력에서 호출하고 각 효과/상태의 실제 출력만 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 13);
        scene.prepare(row[0]);
        new RawBombEffect(scene.pool, scene.types, scene.state, scene.hooks()).receive(scene.packet);
        const actual = [scene.trace || '-', scene.raws(), hex(scene.packet), String(scene.state.spentCounter), String(scene.state.boss), String(scene.state.secondType)];
        // 원본의 효과/전체 raw/패킷/누적/두 현재 전역을 각각 확인한다.
        for (let field = 0; field < actual.length; field += 1) {
            assert.equal(actual[field], row[1 + field], `Bomb 효과 입력 ${row[0]} 필드 ${field}`);
            assert.equal(row[1 + field], row[7 + field]);
        }
        count += 1;
    }
    assert.equal(count, 1676);
});

test('bomb-effect: 패킷 경계와 누락된 하위·확보 실패·예외 해제의 보호 경로를 확인한다', () => {
    const scene = new EffectScene();
    scene.prepare('0,4,51,168,0,3,0,0,1065353216,0,1');
    const empty = new RawBombEffect(scene.pool, scene.types, scene.state, {});
    assert.equal(empty.pool, scene.pool);
    assert.throws(() => empty.receive(scene.packet.subarray(0, 5)), RangeError);
    // dead 부모는 count 를 읽기 전에 반환한다.
    scene.raw(50)[11] = 2;
    empty.receive(scene.packet.subarray(0, 6));
    scene.raw(50)[11] = 0;
    assert.throws(() => empty.receive(scene.packet.subarray(0, 6)), RangeError);
    put(scene.packet, 6, 7, 4);
    assert.throws(() => empty.receive(scene.packet), RangeError);
    put(scene.packet, 6, 0xffffffff, 4);
    assert.throws(() => empty.receive(scene.packet), RangeError);
    put(scene.packet, 6, 3, 4);
    // 최초 확보 실패는 효과/해제를 성공으로 보고하지 않는다.
    assert.throws(() => new RawBombEffect(scene.pool, scene.types, scene.state, { allocate: () => false }).receive(scene.packet), /확보 실패/);
    let releases = 0;
    const base = { allocate: () => true, release: () => { releases += 1; } };
    // 확보 뒤 실제 대상 효과 누락도 배열을 해제한다.
    assert.throws(() => new RawBombEffect(scene.pool, scene.types, scene.state, base).receive(scene.packet), TypeError);
    assert.equal(releases, 1);
    // 효과가 버퍼 밖 대상까지 늘려도 읽기 전에 거부한다.
    const growing = { ...base, apply: () => put(scene.packet, 6, 7, 4) };
    assert.throws(() => new RawBombEffect(scene.pool, scene.types, scene.state, growing).receive(scene.packet), RangeError);
    assert.equal(releases, 2);
    // 구성 오류.
    assert.throws(() => new RawBombEffect({ edition: 'original1037' }, scene.types, scene.state, {}), TypeError);
    assert.throws(() => new RawBombEffect(scene.pool, scene.types.slice(1), scene.state, {}), TypeError);
});

test('bomb-effect: 송신 observer 는 쓰기일 때만 대상 수로 선언 길이를 확정하고 범위를 넘으면 거부한다', () => {
    const packet = new Uint8Array(22).fill(0xa5);
    put(packet, 6, 3, 4);
    finalizeBombEffectPacket(packet, false);
    assert.equal(packet[2], 0xa5);
    finalizeBombEffectPacket(packet, true);
    assert.deepEqual([packet[2], packet[3], packet[4]], [16, 0, 0xa5]);
    // 버퍼에 못 들어가는 수와 100 초과는 거부한다.
    put(packet, 6, 7, 4);
    assert.throws(() => finalizeBombEffectPacket(packet, true), RangeError);
    const large = new Uint8Array(300);
    put(large, 6, 101, 4);
    assert.throws(() => finalizeBombEffectPacket(large, true), RangeError);
    put(large, 6, 100, 4);
    finalizeBombEffectPacket(large, true);
    assert.deepEqual([large[2], large[3]], [210, 0]);
    assert.throws(() => finalizeBombEffectPacket(new Uint8Array(9), true), RangeError);
});

test('bomb-effect: 시전 종료 뒤 공유 자원 DWORD 를 unsigned 비트로 다시 읽고 후속 확보 실패에도 선행 누적을 보존한다', () => {
    const owner = new GameAuxState();
    const scene = new EffectScene();
    scene.prepare('0,0,51,117,0,0,0,0,1065353216,77,1');
    scene.state.sharedSpentCounter = owner;
    scene.types[118].cost = 7.75;
    scene.state.spentCounter = 77;
    owner.resourcePool = 123;
    const hooks = {
        stopCast: (carrier) => {
            assert.equal(carrier, 51);
            owner.storeResourcePoolBits(0xffffffff);
            scene.raw(50)[10] = 118;
        },
        allocate: () => false,
    };
    assert.throws(() => new RawBombEffect(scene.pool, scene.types, scene.state, hooks).receive(scene.packet), /확보 실패/);
    // signed −1 의 동일 DWORD + 7 이 unsigned 6 으로 감기며 owner 1/성공으로 정규화하지 않는다.
    assert.equal(owner.resourcePoolBits, 6);
    assert.equal(scene.state.spentCounter, 77);
});
