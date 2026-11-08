// AI 월드 등록 raw-ai-world-registration.js(RawAiWorldRegistration)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 초기 전체 스캔이 기존 시설/수집자를 먼저·방어 자산을 나중에 등록하며 SID 1 은 방문하지 않고 free 와 dead 를 구별,
//    임시 목록 300개 용량과 예외 시 임시 1200바이트 반납, 방향 동맹·현재 신전·동일 거리 유지와 자신의 거리만 float 로 저장하는 경계, 표면 소유·섬 마스크·범위 밖 진단과 제거의 좌표 비조회/모든 중복 압축.
//    cpppj 의 네 번째 시험(RawGeyserAiRegistration 의 다음 Player 재조회)은 `raw-geyser-ai-registration.js`(WB04-E78)의 자체 시험이 이미 검사하므로 옮기지 않았다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E180: AI 월드 등록 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiObjectNotice } from '../www/core/raw-ai-object-notice.js';
import { RawAiWorldRegistration } from '../www/core/raw-ai-world-registration.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E180)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 배열 두 개의 원소가 모두 같은지 본다. (2026-11-09 WB04-E180)
const same = (left, right) => left.length === right.length && left.every((value, index) => value === right[index]);

const F = Math.fround;

// 실제 backing 에 명시 입력을 쓰며 원본 실행 관측 fixture 나 게임 대체 AI 로 사용하지 않는다. (2026-11-09 WB04-E180)
class WorldRegistrationInput {
    // 목록은 충분한 실제 512 DWORD 입력 backing 을 갖고 native 생성자를 대신했다고 주장하지 않는다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x43000000, (bytes) => bytes.fill(0xa7));
        this.pool = new SidPool(32768, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        this.players = new Uint8Array(1620);
        this.allies = new Array(81).fill(0);
        this.surface = new Uint16Array(65536);
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.outpost = 126;
        this.now = 0;
        this.trace = [];
        this.farther = 0;
        this.missing = 0;
        const raw = this.memory.bytes(this.token);
        raw.fill(0);
        this.field(0x260, 1);
        put(this.players, 180 + 0xb0, this.token);
        // 원본 16개 목록을 같은 실제 메모리에 준비한다.
        for (const at of [0x284, 0x2dc, 0x2e8, 0x2f4, 0x300, 0x30c, 0x318, 0x324, 0x330, 0x33c, 0x348, 0x354, 0x360, 0x36c, 0x378, 0x384]) {
            this.field(at, this.memory.allocate(2048));
            this.field(at + 4, 512);
        }
        this.types[71].flags2 = 0x4000;
        this.types[72].flags2 = 0x4000000;
        this.types[74].flags2 = 0x10000000;
        this.types[75].flags2 = 2;
        this.types[76].flags2 = 0x1000002;
        this.types[77].flags2 = 0x30000;
        const notices = {
            carrierTargets: () => [0, 0],
            log: (brain, log) => { this.trace.push(`worker${log.object}`); },
            distanceSquared: (sid, other) => { assert.ok(other === 3 && this.field(0x308) === 1); this.trace.push(`guard${sid}`); return 1.0; }
        };
        this.objects = new RawAiObjectNotice(this.memory, this.pool, this.types, this.players, () => this.now, () => this.outpost, () => false, notices);
        const hooks = {
            center: (sid) => { const slot = this.pool.slot(sid); return [get(slot, 14), get(slot, 18)].map((bits) => { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits, true); return view.getFloat32(0, true); }); },
            fartherGeyser: () => { this.farther += 1; },
            missingSurface: () => { this.missing += 1; }
        };
        this.registration = new RawAiWorldRegistration(this.memory, this.pool, this.types, this.players, this.allies, this.surface, this.objects, hooks);
    }

    // 본체의 현재 DWORD 를 읽거나(값 없음) 쓴다. (2026-11-09 WB04-E180)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), at);
        put(this.memory.bytes(this.token), at, value >>> 0);
        return undefined;
    }

    // 명시 raw 장면 입력이며 실제 할당/플레이의 결과로 가장하지 않는다. (2026-11-09 WB04-E180)
    slot(number, type, owner, x = 0, y = 0, state = 0) {
        const raw = this.pool.physical(number);
        raw.fill(0);
        raw[10] = type;
        raw[11] = state;
        raw[34] = owner;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
    }

    // 현재 활성 목록을 읽는 관찰이며 예상 결과를 계산하지 않는다. (2026-11-09 WB04-E180)
    list(at) {
        const result = [];
        for (let index = 0; index < this.field(at + 8); index += 1) result.push(get(this.memory.bytes(this.field(at)), index * 4));
        return result;
    }
}

test('AI 월드 등록: 전체 스캔은 기존 시설/수집자를 먼저, 방어 자산을 나중에 등록하며 SID 1 을 건너뛰고 free 와 dead 를 구별한다', () => {
    const input = new WorldRegistrationInput();
    input.slot(1, 73, 1);
    input.slot(2, 72, 1);
    input.slot(3, 71, 1);
    input.slot(4, 72, 1);
    input.slot(5, 77, 1, 0, 0, 2);
    input.slot(6, 74, 0, 20, 20);
    input.slot(8, 73, 2);
    input.slot(9, 69, 1);
    input.slot(10, 75, 1);
    input.slot(11, 73, 1, 0, 0, 1);
    input.surface[20 * 256 + 20] = 10;
    input.field(0x3c8, 99);
    const allocations = input.memory.allocations.length;
    input.registration.fullScan(input.token);
    assert.ok(same(input.trace, ['worker5', 'guard2', 'guard4']));
    assert.ok(input.field(0x6d4 + 71 * 4) === 1 && input.field(0x6d4 + 72 * 4) === 2 && input.field(0x6d4 + 77 * 4) === 1);
    assert.ok(!input.field(0x6d4 + 73 * 4) && !input.field(0x6d4 + 74 * 4) && !input.field(0x6d4 + 75 * 4));
    assert.ok(same(input.list(0x360), [2, 4]) && same(input.list(0x324), [6]) && same(input.list(0x330), [6]));
    assert.ok(input.field(0x3c8) === 3 && !input.field(0x25c));
    const last = input.memory.allocations.at(-1);
    assert.ok(input.memory.allocations.length === allocations + 1 && last.size === 1200 && input.memory.frees.length === 1 && !input.memory.contains(last.token));
});

test('AI 월드 등록: 임시 목록은 처음 300개만 보존하며 실패해도 임시 1200바이트를 반납한다', () => {
    const input = new WorldRegistrationInput();
    input.slot(3, 71, 1);
    // 용량보다 5개 많은 방어 자산을 명시 입력으로 둔다.
    for (let sid = 100; sid < 405; sid += 1) input.slot(sid, 72, 1);
    input.registration.fullScan(input.token);
    assert.ok(input.field(0x6d4 + 72 * 4) === 300 && input.list(0x360).length === 300 && input.list(0x360)[0] === 100 && input.list(0x360).at(-1) === 399);
    const failed = new WorldRegistrationInput();
    failed.slot(3, 71, 1);
    failed.slot(5, 77, 1);
    failed.field(0x3c8, 99);
    failed.field(0x2dc, 0xdeadbeef);
    assert.ok(throws(() => failed.registration.fullScan(failed.token)));
    // 실패 전 실제 쓰기는 보존하지만 임시 1200 BYTE 는 반납한다.
    assert.ok(failed.field(0x3c8) === 99 && failed.field(0x6d4 + 71 * 4) === 1 && failed.memory.frees.length === 1);
});

test('AI 월드 등록: 방향 동맹/현재 신전/동일 거리 유지와 자신의 거리만 float 로 저장하는 원문을 구분한다', () => {
    const input = new WorldRegistrationInput();
    input.slot(50, 74, 0, 10, 0);
    input.slot(60, 73, 2, 20, 0);
    put(input.players, 360 + 0x7c, 60);
    input.allies[9 + 2] = 1;
    assert.ok(!input.registration.closerToAlly(input.token, 50));
    input.slot(60, 73, 2, 15, 0);
    assert.ok(input.registration.closerToAlly(input.token, 50));
    input.allies[9 + 2] = 0;
    input.allies[18 + 1] = 1;
    assert.ok(!input.registration.closerToAlly(input.token, 50));
    input.allies[9 + 2] = 1;
    // 실제 동맹 거리는 조금 더 멀어도 자신의 fstp DWORD 올림 경계가 비교 기준이다.
    input.slot(50, 74, 0, 1, F(0.0003));
    input.slot(60, 73, 2, 0, F(-0.00001));
    assert.ok(input.registration.closerToAlly(input.token, 50));
    // 자신의 DWORD 내림보다 상대 거리가 크므로 추가하지 않는다.
    input.slot(50, 74, 0, 1, F(0.0002));
    input.slot(60, 73, 2, 0, F(0.00001));
    assert.ok(!input.registration.closerToAlly(input.token, 50));
    input.registration.geyser(input.token, 50, 1);
    assert.ok(input.farther === 0 && input.missing === 1);
    input.slot(60, 73, 2, 1, F(0.0002));
    input.registration.geyser(input.token, 50, 1);
    assert.ok(input.farther === 1 && input.field(0x32c) === 1);
});

test('AI 월드 등록: 표면 소유·섬 마스크·범위 밖 진단과 제거의 좌표 비조회/모든 중복 압축을 확인한다', () => {
    const input = new WorldRegistrationInput();
    input.slot(50, 74, 0, F(-0.8), F(1.9));
    input.slot(51, 75, 1);
    input.surface[256] = 51;
    input.registration.geyser(input.token, 50, 1);
    assert.ok(same(input.list(0x324), [50]) && same(input.list(0x330), [50]));
    input.slot(51, 75, 2);
    input.registration.geyser(input.token, 50, 2);
    assert.ok(input.field(0x32c) === 2 && input.field(0x338) === 1 && !input.field(0x350));
    input.slot(51, 76, 0);
    input.registration.geyser(input.token, 50, 1);
    assert.equal(input.field(0x350), 1);
    input.slot(50, 74, 0, 300, 2);
    input.registration.geyser(input.token, 50, 1);
    assert.ok(input.missing === 1 && input.field(0x350) === 2);
    // 다섯 목록에 같은 제거 입력과 stale 꼬리를 명시해 좌표를 읽지 않는 0 분기를 고립한다.
    for (const at of [0x324, 0x330, 0x33c, 0x348, 0x354]) {
        const list = input.memory.bytes(input.field(at));
        put(list, 0, 0xffffffff);
        put(list, 4, 7);
        put(list, 8, 0xffffffff);
        put(list, 12, 8);
        input.field(at + 8, 4);
    }
    input.registration.geyser(input.token, 0xffffffff, 0);
    // 활성 결과만 압축하며 세 번째 칸의 과거 값은 그대로 남긴다.
    for (const at of [0x324, 0x330, 0x33c, 0x348, 0x354]) assert.ok(same(input.list(at), [7, 8]) && get(input.memory.bytes(input.field(at)), 8) === 0xffffffff);
});

test('AI 월드 등록 생성자: 본체 소유자/같은 풀의 통지기/TYPE 표/Player 9기록/동맹 81/표면 65536 이 없으면 거부한다', () => {
    const input = new WorldRegistrationInput();
    const hooks = {};
    const args = [input.memory, input.pool, input.types, input.players, input.allies, input.surface, input.objects, hooks];
    const make = (index, value) => () => new RawAiWorldRegistration(...args.map((original, at) => (at === index ? value : original)));
    assert.ok(throws(make(0, undefined)));
    assert.ok(throws(make(1, new SidPool(32768, { server: true }))));
    assert.ok(throws(make(2, [])));
    assert.ok(throws(make(3, new Uint8Array(1619))));
    assert.ok(throws(make(4, new Array(80).fill(0))));
    assert.ok(throws(make(5, new Uint16Array(65535))));
    assert.ok(throws(make(6, undefined)));
    assert.ok(throws(() => input.registration.geyser(input.memory.allocate(16), 50, 1)));
});
