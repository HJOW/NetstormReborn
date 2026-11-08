// AI 생성/삭제 통지 raw-ai-object-notice.js(RawAiObjectNotice)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 비활성/signed 비양수 owner/NULL 은 본체를 선택하지 않고 다음 호출은 현재 Player 포인터를 읽음, 복합 genus 생성의 실제 중복 정책/거리 전체 방문/타이머 정밀도,
//    삭제의 중복 압축/가득 찬 제외 목록/시설 재예약과 inactive 꼬리 보존, 사제와 현재 outpost 제외 번호 분기.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E180: AI 생성/삭제 통지 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { AI_OBJECT_LOG, RawAiObjectNotice } from '../www/core/raw-ai-object-notice.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E180)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 배열 두 개의 원소가 모두 같은지 본다. (2026-11-09 WB04-E180)
const same = (left, right) => left.length === right.length && left.every((value, index) => value === right[index]);

// 결과를 계산하지 않고 실제 등록 메모리에 명시 입력을 배치하는 검사 도구이다. (2026-11-09 WB04-E180)
class NoticeInput {
    // 원본 생성자를 대신했다고 주장하지 않는 명시 본체/목록 입력을 준비한다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x40000000, (bytes) => bytes.fill(0xa7));
        this.pool = new SidPool(32768, { server: true });
        this.sid = this.pool.allocate(2);
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        this.players = new Uint8Array(1620);
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.outpost = 126;
        this.now = 16777216.25;
        this.disabled = false;
        this.distances = [];
        this.logs = [];
        const raw = this.memory.bytes(this.token);
        raw.fill(0);
        put(this.players, 180 + 0xb0, this.token);
        // 모든 목록에 실제 backing 을 확보하되 inactive 꼬리는 a7 로 유지한다(원본 생성자의 16개 목록 위치).
        for (const at of [0x284, 0x2dc, 0x2e8, 0x2f4, 0x300, 0x30c, 0x318, 0x324, 0x330, 0x33c, 0x348, 0x354, 0x360, 0x36c, 0x378, 0x384]) {
            put(raw, at, this.memory.allocate(16));
            put(raw, at + 4, 4);
        }
        const object = this.pool.allocated(this.sid);
        object[10] = 71;
        object[34] = 1;
        put(object, 14, floatToBits(26.5));
        put(object, 18, floatToBits(-12.5));
        this.hooks = {
            carrierTargets: () => [701, 999],
            distanceSquared: (first, second) => { assert.equal(first, this.sid); this.distances.push(second); return this.distances.length === 2 ? 180.499 : 180.5; },
            log: (brain, log) => { assert.equal(brain, this.token); this.logs.push(log); }
        };
        this.notice = new RawAiObjectNotice(this.memory, this.pool, this.types, this.players, () => this.now, () => this.outpost, () => this.disabled, this.hooks);
    }

    // 본체의 현재 DWORD 를 읽거나(값 없음) 덮어쓴다. (2026-11-09 WB04-E180)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), at);
        put(this.memory.bytes(this.token), at, value >>> 0);
        return undefined;
    }

    // 실제 목록 포인터의 활성 입력을 준비하고 비활성 꼬리는 보존한다. 값 없이 부르면 활성 목록을 복사해 돌려준다. (2026-11-09 WB04-E180)
    list(at, values) {
        if (values === undefined) {
            const result = [];
            for (let index = 0; index < this.field(at + 8); index += 1) result.push(get(this.memory.bytes(this.field(at)), index * 4));
            return result;
        }
        values.forEach((value, index) => put(this.memory.bytes(this.field(at)), 4 * index, value));
        this.field(at + 8, values.length);
        return undefined;
    }

    // 타이머의 실제 QWORD 비트를 double 로 해석한다. (2026-11-09 WB04-E180)
    time(at) {
        const view = new DataView(new ArrayBuffer(8));
        view.setUint32(0, this.field(at), true);
        view.setUint32(4, this.field(at + 4), true);
        return view.getFloat64(0, true);
    }
}

test('AI 통지: 비활성/signed owner/NULL 은 본체를 선택하지 않고 다음 호출은 현재 Player 포인터를 읽는다', () => {
    const input = new NoticeInput();
    input.disabled = true;
    input.field(0x6d4 + 71 * 4, 7);
    put(input.players, 180 + 0xb0, 0xdead0000);
    input.notice.notice(1, input.sid, true);
    input.disabled = false;
    // signed 비양수와 NULL 은 잘못된 포인터나 SID 에 도달하지 않는다.
    for (const owner of [0, 0xffffffff, 0x80000000, 8]) input.notice.notice(owner, 65535, true);
    assert.ok(input.field(0x6d4 + 71 * 4) === 7 && input.logs.length === 0);
    assert.ok(throws(() => input.notice.notice(9, input.sid, true)));
    assert.ok(throws(() => input.notice.notice(1, input.sid, true)));
    put(input.players, 180 + 0xb0, input.token);
    input.notice.notice(1, input.sid, true);
    assert.ok(input.field(0x6d4 + 71 * 4) === 8 && input.notice.pool === input.pool);
    const small = input.memory.allocate(16);
    put(input.players, 180 + 0xb0, small);
    assert.ok(throws(() => input.notice.notice(1, input.sid, true)));
    input.memory.free(small);
    assert.ok(throws(() => input.notice.notice(1, input.sid, true)));
});

test('AI 통지: 복합 genus 생성의 실제 중복 정책/거리 전체 방문/타이머 정밀도를 독립 명시 결과와 대조한다', () => {
    const input = new NoticeInput();
    const sid = input.sid;
    input.types[71].flags2 = 0x4000000 | 0x400000 | 0x30000 | 0x800 | 0x4000 | 0x200;
    input.field(0x294, 101);
    input.list(0x300, [101, 102, 103]);
    input.list(0x2dc, [sid, sid]);
    input.list(0x2f4, [sid]);
    input.list(0x30c, [101]);
    input.list(0x2e8, [8, 8, 9]);
    input.field(0x2ec, 3);
    input.field(0x3b0, 4);
    input.field(0x3c8, 0xfffffffe);
    input.field(0x6d4 + 71 * 4, 7);
    input.field(0x228, floatToBits(3.25));
    input.field(0x5b8, 1);
    input.field(0x5c0, 1);
    input.notice.notice(1, input.sid, true);
    assert.ok(input.field(0x6d4 + 71 * 4) === 8 && input.field(0x2a4) === sid && input.field(0x2c8) === 1 && input.field(0x3c8) === 1);
    assert.ok(same(input.distances, [101, 101, 102, 103]));
    assert.ok(same(input.list(0x360), [sid]) && same(input.list(0x2dc), [sid, sid, sid]) && same(input.list(0x2f4), [sid, sid]) && same(input.list(0x2e8), [9, sid]) && same(input.list(0x30c), [101, sid]));
    assert.ok(input.time(0x5a8) === 16777219.5 && input.time(0x5b0) === 16777220.5 && input.time(0x5b8) === 0 && input.time(0x5c0) === 0);
    assert.equal(input.logs.length, 1);
    assert.ok(input.logs[0].kind === AI_OBJECT_LOG.built && input.logs[0].type === 71 && input.logs[0].x === 26 && input.logs[0].y === -12);
});

test('AI 통지: 삭제의 중복 압축/가득 찬 제외 목록/시설 재예약과 signed 감김을 함께 검사한다', () => {
    const input = new NoticeInput();
    const sid = input.sid;
    input.types[71].flags2 = 0x4000000 | 0x400000 | 0x30000 | 0x800 | 0x4200;
    input.field(0x248, 8);
    input.field(0x294, sid);
    input.field(0x2a4, sid);
    input.field(0x3f8, sid);
    input.field(0x3fc, sid);
    input.field(0x3b0, 3);
    input.list(0x330, [700, 701, 702]);
    input.list(0x33c, [900]);
    input.field(0x340, 1);
    input.list(0x384, [sid, 21, sid]);
    input.list(0x2dc, [sid, 12, sid, 13]);
    input.list(0x360, [sid, 51, sid]);
    input.list(0x36c, [sid, 22]);
    input.list(0x378, [sid]);
    input.list(0x300, [sid, 40, 41, sid]);
    input.list(0x30c, [sid, 40]);
    input.list(0x318, [3, sid]);
    input.list(0x2e8, [sid, sid, 51]);
    input.list(0x2f4, [sid]);
    input.notice.notice(1, input.sid, false);
    assert.ok(input.field(0x6d4 + 71 * 4) === 0xffffffff && !input.field(0x294) && !input.field(0x2a4) && !input.field(0x3f8) && !input.field(0x3fc) && !input.field(0x2c8));
    assert.ok(same(input.list(0x330), [700, 702]) && same(input.list(0x33c), [900]));
    assert.ok(same(input.list(0x384), [21]) && input.time(0x5e0) === 16777246.25);
    assert.ok(same(input.list(0x2dc), [12, 13]) && get(input.memory.bytes(input.field(0x2dc)), 8) === sid);
    assert.ok(same(input.list(0x360), [51]) && same(input.list(0x36c), [22]) && input.list(0x378).length === 0);
    assert.ok(same(input.list(0x300), [40, 41]) && same(input.list(0x30c), [40, 41]) && same(input.list(0x318), [3]));
    assert.ok(same(input.list(0x2e8), [51]) && input.list(0x2f4).length === 0);
    assert.ok(input.field(0x3e4) === 6 && input.field(0x3f0) === 1 && !input.field(0x3e0) && !input.field(0x3f4) && !input.field(0x400) && !input.field(0x3ec));
    assert.equal(input.logs.length, 2);
    assert.ok(input.logs[0].kind === AI_OBJECT_LOG.madeGeyserIneligible && input.logs[1].kind === AI_OBJECT_LOG.built);
});

test('AI 통지: 사제도 경로 정리는 수행하되 일반 수집자 Built 처리는 없고 현재 outpost 제외 번호를 존중한다', () => {
    const input = new NoticeInput();
    input.types[71].flags2 = 0x230000;
    input.field(0x2a0, input.sid);
    input.field(0x29c, input.sid);
    input.field(0x248, 8);
    input.notice.notice(1, input.sid, false);
    assert.ok(!input.field(0x2a0) && !input.field(0x29c) && input.logs.length === 1 && input.logs[0].kind === AI_OBJECT_LOG.cannotExcludeGeyser);
    input.logs.length = 0;
    input.types[71].flags2 = 0x4000;
    input.outpost = 71;
    input.field(0x5c0, 1);
    input.notice.notice(1, input.sid, true);
    assert.ok(input.list(0x300).length === 0 && input.list(0x30c).length === 0 && input.time(0x5c0) === 0 && input.logs.length === 0);
    input.outpost = 126;
    input.notice.notice(1, input.sid, true);
    assert.ok(same(input.list(0x300), [input.sid]));
});

test('AI 통지 생성자: 본체 소유자/PATCH 풀/TYPE 표/Player 9기록/현재 모드와 시각 조회기가 없으면 거부한다', () => {
    const input = new NoticeInput();
    const make = (memory, pool, types, players, now, outpost, disabled) => () => new RawAiObjectNotice(memory, pool, types, players, now, outpost, disabled, input.hooks);
    const ok = [input.memory, input.pool, input.types, input.players, () => 0, () => 126, () => false];
    assert.ok(throws(make(...ok.map((value, index) => (index === 0 ? undefined : value)))));
    assert.ok(throws(make(...ok.map((value, index) => (index === 1 ? {} : value)))));
    assert.ok(throws(make(...ok.map((value, index) => (index === 2 ? [] : value)))));
    assert.ok(throws(make(...ok.map((value, index) => (index === 3 ? new Uint8Array(1619) : value)))));
    assert.ok(throws(make(...ok.map((value, index) => (index === 4 ? undefined : value)))));
    assert.ok(throws(make(...ok.map((value, index) => (index === 5 ? undefined : value)))));
    assert.ok(throws(make(...ok.map((value, index) => (index === 6 ? undefined : value)))));
});
