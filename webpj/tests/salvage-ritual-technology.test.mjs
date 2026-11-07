// 건물 회수 명령(요청 44c420·송신 4437c0·54h 수신 4436f0)을 원본 10.78 관측으로 대조하고, GetTechnology·의식 선택 일반 입력의 분배 계약을 검사한다(WB04-E35, cpppj SalvageCommandTests·ThemeTechnologyCommand·RitualChoiceInput 이식).
//  - salvage-command-x86.tsv 3458행(두 x87 정밀도 기대가 모두 같아야 함): 요청 1971·송신 400·수신 1087의 호출 순서·8바이트 송신·현재 전역·세 raw 슬롯·caller 소유 수신 버퍼.
//  - GetTechnology: 뗌은 효과 없이 소비, 누름은 y 전체 DWORD 전달. 의식 선택(RitualChoiceInput)은 원문 관측이 없는 정적 복원이라 패킷 작성/분기/진단 순서를 소스 계약으로 검사한다.
// 세션(RawCampaignSession)에 연결하는 기술 지급(ApplyThemeTechnology)과 실제 송신/비동기 경계는 해당 단위에서 대조한다. (2026-11-08 WB04-E35: 회수·기술·의식 선택 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, readRows } from './raw-support.mjs';
import { InputCode } from '../www/core/input-queue.js';
import { SidPool } from '../www/core/sid-pool.js';
import { RawSalvageCommand } from '../www/core/raw-salvage-command.js';
import { GET_TECHNOLOGY_COMMAND, dispatchGetTechnologyInput } from '../www/core/get-technology-input.js';
import {
    GET_NEW_TECH_COMMAND, REQUEST_STORMPOWER_COMMAND, UPGRADE_ALTAR_COMMAND, constructRitualChoicePacket, dispatchRitualChoiceInput, isRitualChoiceCommand,
} from '../www/core/ritual-choice-input.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E35)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'salvage-command-x86.tsv'));

// 원문 DWORD 읽기(감김), 바이트 16진, 리틀엔디언 WORD/DWORD 쓰기·읽기이다. (2026-11-08 WB04-E35)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const put = (raw, at, value, width = 4) => { for (let index = 0; index < width; index += 1) raw[at + index] = (value >>> (8 * index)) & 0xff; };
const get = (raw, at) => (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0;

// 실제 계산을 대체하지 않고 큰 세계 효과에 공급한 명시 입력만 재생하는 장면이다(cpppj SalvageScene). (2026-11-08 WB04-E35)
class SalvageScene {
    constructor() {
        this.pool = new SidPool(24000, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ cost: 0 }));
        this.state = { localOwner: 0, denySalvage: 0, checking: false, mission: false, boss: false, altarType: 164, daisType: 165, nuggetType: 154, request: { message: 0x54, protocol: 1, length: 8 } };
        this.input = [];
        this.received = new Uint8Array(8);
        this.trace = '';
        this.packet = '-';
        this.line = 0;
    }

    // 할당기 상태를 합성하지 않는 검사 전용 물리 raw 입력이다.
    raw(sid) { return this.pool.physical(sid); }

    // 실제 원본 인자의 폭/순서를 유지한다.
    event(name, args = []) {
        if (this.trace) this.trace += ';';
        this.trace += name;
        for (const value of args) this.trace += `:${value}`;
    }

    // 격리 원본에도 공급한 외부 변경만 재생한다.
    mutate(boundary, sid = 50) {
        const mutation = this.input[17];
        const state = this.state;
        if (boundary === 'query') {
            if (mutation === 1) this.raw(51)[11] = 3;
            if (mutation === 2) state.denySalvage = 1;
            if (mutation === 8) state.altarType = 128;
        }
        if (boundary === 'assert' && this.line === 0x295) {
            if (mutation === 3) { this.raw(50)[10] = 6; put(this.raw(50), 18, 154, 2); }
            if (mutation === 4) { state.localOwner = 2; state.denySalvage = 1; }
        }
        if (boundary === 'assert' && this.line === 0x563 && mutation === 5) {
            state.boss = true;
            put(this.received, 4, 51, 2);
            put(this.received, 6, 0xffff, 2);
            put(this.raw(sid), 0, 0x12200100);
            this.raw(sid)[34] = 2;
        }
        if (boundary === 'refund') {
            if (mutation === 6) { put(this.raw(sid), 0, 0x12200100); put(this.received, 4, 51, 2); put(this.received, 6, 0, 2); }
            if (mutation === 7) { this.raw(sid)[34] = 2; this.raw(sid)[11] = 1; }
        }
    }

    // 원본 하위 관측과 같은 경계이며 환불/삭제 결과를 제품에 주입하지 않는다.
    hooks() {
        return {
            daisAt: (sid) => { this.event('Q', [sid]); this.mutate('query', sid); return sid === 50 && this.input[14] ? 51 : sid === 51 && this.input[14] === 2 ? 52 : 0; },
            tell: (text) => { assert.equal(text, 'DenySalvage'); this.event('T:DenySalvage'); },
            assertion: (value) => { this.line = value; this.event('A', [value]); this.mutate('assert'); },
            send: (target, raw) => { this.event('S', [target]); this.packet = hex(raw); },
            refund: (sid, owner, salvage) => { this.event('R', [sid, owner, salvage ? 1 : 0]); this.mutate('refund', sid); },
            destroy: (sid, flags) => this.event(get(this.raw(sid), 0) === 0x12200000 ? 'D' : 'd', [sid, flags]),
        };
    }

    // 원문 입력 21DWORD 로 장면을 초기화하며 원문 기대 열은 사용하지 않는다.
    prepare(values) {
        const input = values;
        assert.equal(input.length, 21);
        this.input = input;
        Object.assign(this.state, { localOwner: input[6], denySalvage: input[7] | 0, checking: input[8] !== 0, mission: input[9] !== 0, boss: input[10] !== 0, altarType: 164, daisType: 165, nuggetType: 154 });
        this.trace = '';
        this.packet = '-';
        this.line = 0;
        this.state.request = { message: input[19] & 0xff, protocol: input[18] & 0xff, length: input[20] & 0xffff };
        assert.equal(input[13], this.pool.capacity);
        // 실제 범위 안의 앞 후보와 마지막 유효 후보를 같은 명시 입력으로 준비한다.
        for (let number = 1; number < 100; number += 1) this.initialize(number);
        this.initialize(23999);
        if (input[14]) { this.raw(51)[10] = input[14] === 2 ? 164 : 128; this.raw(52)[10] = 128; }
        this.types[164].cost = bitsToFloat(input[15]);
        this.types[165].cost = bitsToFloat(input[16]);
        this.received = new Uint8Array(8);
        this.received[0] = input[18] & 0x7f;
        this.received[1] = input[19] & 0xff;
        put(this.received, 2, input[20], 2);
        put(this.received, 4, input[1], 2);
        put(this.received, 6, input[11] >>> 16, 2);
    }

    // 원본 물리 50바이트의 미사용 부분도 같은 명시 A5 입력으로 유지한다.
    initialize(sid) {
        const raw = this.raw(sid);
        raw.fill(0xa5);
        put(raw, 0, 0x12200000);
        raw[10] = this.input[3] & 0xff;
        raw[11] = this.input[2] & 0xff;
        raw[34] = this.input[5] & 0xff;
        put(raw, 18, this.input[4], 2);
    }

    // 현재 전체 비트와 세 raw 슬롯을 원문 형식으로 반환한다.
    observed() {
        const state = this.state;
        const globals = `${state.localOwner},${state.denySalvage >>> 0},${state.checking ? 1 : 0},${state.mission ? 1 : 0},${state.boss ? 1 : 0},${state.altarType}`;
        return [this.trace === '' ? '-' : this.trace, this.packet, globals, `${hex(this.raw(50))},${hex(this.raw(51))},${hex(this.raw(52))}`, hex(this.received)];
    }
}

// 두 x87 정밀도에서 실제로 반환한 요청/송신/수신 3458 입력을 전수 대조한다.
test('Salvage: 원본10.78 3458행(요청 1971·송신 400·수신 1087의 호출·패킷·전역·raw·수신 버퍼)', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new SalvageScene();
    const operations = [0, 0, 0];
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 11);
        scene.prepare(row[0].split(',').map(number));
        const body = new RawSalvageCommand(scene.pool, scene.types, scene.state, scene.hooks());
        const input = scene.input;
        if (input[0] === 0) body.request(input[1]);
        else if (input[0] === 1) body.send(input[1], input[11]);
        else body.receive(input[12], scene.received);
        const observed = scene.observed();
        // 전체 효과/패킷/현재 전역/raw/수신 버퍼를 두 독립 기대와 비교한다.
        for (let field = 0; field < observed.length; field += 1) {
            assert.equal(observed[field], row[1 + field], `행 ${count + 1} 필드 ${field} (53비트)`);
            assert.equal(observed[field], row[6 + field], `행 ${count + 1} 필드 ${field} (64비트)`);
        }
        operations[input[0]] += 1;
        count += 1;
    }
    assert.deepEqual([count, ...operations], [3458, 1971, 400, 1087]);
});

// 생성자와 패킷 길이 계약이다.
test('Salvage: 풀/타입 계약과 짧은 패킷 거부, 범위 밖 SID', () => {
    const scene = new SalvageScene();
    assert.throws(() => new RawSalvageCommand(scene.pool, scene.types.slice(0, 187), scene.state, {}), TypeError);
    const body = new RawSalvageCommand(scene.pool, scene.types, scene.state, {});
    assert.throws(() => body.receive(1, new Uint8Array(7)), TypeError);
    body.request(0);
    assert.throws(() => body.request(24000), RangeError);
    assert.throws(() => body.request(65536), RangeError);
});

// GetTechnology: 뗌은 요구 없이 소비하고 누름은 y 전체 DWORD 를 전달한다.
test('GetTechnology 입력: 뗌 비요구·전체 DWORD 전달·부모 누락 거부', () => {
    assert.equal(dispatchGetTechnologyInput({ code: 0x43, x: 0, y: 0 }, undefined), false);
    assert.equal(dispatchGetTechnologyInput({ code: (GET_TECHNOLOGY_COMMAND | InputCode.release) >>> 0, x: 0, y: 5 }, undefined), true);
    const seen = [];
    assert.equal(dispatchGetTechnologyInput({ code: (GET_TECHNOLOGY_COMMAND | InputCode.shift) >>> 0, x: 1, y: -2 }, (value) => seen.push(value)), true);
    assert.deepEqual(seen, [0xfffffffe]);
    assert.throws(() => dispatchGetTechnologyInput({ code: GET_TECHNOLOGY_COMMAND, x: 0, y: 1 }, undefined), /미연결/);
});

// 의식 선택: 패킷 작성이 대상 진단보다 먼저이고 대상 WORD/종류 DWORD 폭을 유지한다(원문 관측 없는 정적 복원).
test('의식 선택 입력: 패킷 헤더·세 분기의 대상/종류·진단 순서·뗌 비요구', () => {
    const packet = new Uint8Array(12).fill(0xee);
    constructRitualChoicePacket(packet);
    assert.deepEqual(Array.from(packet.subarray(0, 6)), [1, 0x6b, 10, 0, 0xee, 0xee]);
    assert.throws(() => constructRitualChoicePacket(new Uint8Array(9)), RangeError);
    assert.ok(isRitualChoiceCommand(UPGRADE_ALTAR_COMMAND) && !isRitualChoiceCommand(0xd4));
    const events = [];
    const sent = [];
    let dais = 0x12345;
    const hooks = {
        dais: () => dais, nuggetType: () => 154, daisType: () => 165,
        assertion: (line) => { events.push(`A${line.toString(16)}`); dais = 0x77; },
        construct: (raw) => { events.push('C'); constructRitualChoicePacket(raw); },
        send: (target, raw) => { events.push(`S${target}`); sent.push(hex(raw)); },
    };
    // UpgradeAltar: 대상 = y 하위 WORD, 종류 = daisType.
    assert.equal(dispatchRitualChoiceInput({ code: UPGRADE_ALTAR_COMMAND, x: 0, y: 0x12345678 }, hooks), true);
    assert.equal(sent.at(-1), '016b0a00' + '7856' + 'a5000000');
    // GetNewTech: 제단이 있으면 진단 없이 대상 = dais 하위 WORD, 종류 = y 전체 DWORD.
    assert.equal(dispatchRitualChoiceInput({ code: GET_NEW_TECH_COMMAND, x: 0, y: -2 }, hooks), true);
    assert.equal(sent.at(-1), '016b0a00' + '4523' + 'feffffff');
    // RequestStormpower: 제단이 0 이면 진단(0x838) 뒤 다시 읽어 계속하며 종류 = nuggetType.
    dais = 0;
    events.length = 0;
    assert.equal(dispatchRitualChoiceInput({ code: REQUEST_STORMPOWER_COMMAND, x: 0, y: 0 }, hooks), true);
    assert.deepEqual(events, ['C', 'A838', 'S9']);
    assert.equal(sent.at(-1), '016b0a00' + '7700' + '9a000000');
    // GetNewTech 진단 줄은 0x82f 이다. 뗌과 다른 키는 장치를 요구하지 않는다.
    dais = 0;
    events.length = 0;
    dispatchRitualChoiceInput({ code: GET_NEW_TECH_COMMAND, x: 0, y: 3 }, hooks);
    assert.equal(events[1], 'A82f');
    assert.equal(dispatchRitualChoiceInput({ code: (GET_NEW_TECH_COMMAND | InputCode.release) >>> 0, x: 0, y: 3 }, undefined), true);
    assert.equal(dispatchRitualChoiceInput({ code: 0x58, x: 0, y: 0 }, undefined), false);
    assert.throws(() => dispatchRitualChoiceInput({ code: UPGRADE_ALTAR_COMMAND, x: 0, y: 0 }, {}), /미연결/);
});
