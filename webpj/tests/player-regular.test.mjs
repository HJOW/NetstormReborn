// Player Regular 처리기 raw-player-regular.js(RawPlayerRegular)와 Template State 서비스 raw-player-template-state.js(RawPlayerTemplateState)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - player-regular-x86.tsv 의 10.78(originals) 329행: 직접 처리기·State getter·Notify 를 실제 Factory(Player152)·ProcessForm/Kernel 호스트·Template 9 header/State 스트림에 연결해 실행한다(cpppj PlayerRegular_PatchNativeStateAndActualHost).
//    10.78 은 CD 의 즉시 로딩 깊이 증감이 없어 depth 열은 입력값(0xffffffff) 그대로다.
//  - player-state-expand-x86.tsv 99행: 짧은 State 의 분리 할당 확장(원문 보존·0 채움·position/end 28)과 다음 get 의 같은 버퍼(cpppj PlayerStateExpand_Native99AndSameCurrentStream).
//  - player-state-initialization-x86.tsv 2560행: 빈 State 의 생성 조건·보존 스택 바이트·28 BYTE 쓰기와 기존 State 자연 반환(cpppj PlayerStateInitialization_Native2560).
//  - 계약: PlayerStateInitialization_RequiredEffectsAndEdition 의 10.78 부분.
//  - 범위 밖: PlayerRegular_SameWorldInitialFiveAndSixtySecondChain(RawSurfaceWorld)·CampaignTemplates_* (CampaignTemplateStorage)는 WB05 월드 조립 몫이고, CD 행은 웹 범위 밖이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E55: Player Regular·Template State 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { realPlayerAssets, hex, doubleBits } from './player-scene-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidOwnerMode } from '../www/core/squid-owner.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { Kernel, PROCESS_CAPACITY } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { PLAYER_RECORD_STRIDE, PlayerWorldState, RawPlayerWorld } from '../www/core/raw-player-world.js';
import { PLAYER_REGULAR_ASSERTION, RawPlayerRegular } from '../www/core/raw-player-regular.js';
import { PLAYER_TEMPLATE_STATE_ASSERTION, PLAYER_TEMPLATE_STATE_BOUNDARY, RawPlayerTemplateState, initializePlayerTemplateStateBytes } from '../www/core/raw-player-template-state.js';

// 원본 관측 파일이다. State 두 fixture 는 10.78 PE 만의 관측이라 판본 열이 없다. (2026-11-08 WB04-E55)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const regularRows = readRows(path.join(FIXTURES, 'player-regular-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;
const expandRows = readRows(path.join(FIXTURES, 'player-state-expand-x86.tsv'));
const initializationRows = readRows(path.join(FIXTURES, 'player-state-initialization-x86.tsv'));

// fixture 입력 16칸의 위치이다(cpppj PlayerRegularField). (2026-11-08 WB04-E55)
const FIELD = Object.freeze({ kind: 0, owner: 1, local: 2, templateOwner: 3, battle: 4, event: 5, count: 6, reserve: 7, payload: 8, mine: 9, transient: 10, prevent: 11, mission: 12, dot: 13, end: 14, mutation: 15 });

// 10.78 진단 줄 번호(Template.cpp 와 Player.cpp)이다. (2026-11-08 WB04-E55)
const TEMPLATE_LINES = Object.freeze({
    [PLAYER_TEMPLATE_STATE_ASSERTION.dotFileName]: 0x1c1,
    [PLAYER_TEMPLATE_STATE_ASSERTION.mineTransientFlag]: 0x1c3,
    [PLAYER_TEMPLATE_STATE_ASSERTION.inMissionSave]: 0x1c6,
    [PLAYER_TEMPLATE_STATE_ASSERTION.missingStateTemplate]: 0x1fc,
    [PLAYER_TEMPLATE_STATE_ASSERTION.missingNotifyTemplate]: 0x1c7,
});
const REGULAR_UNKNOWN_LINE = 0x41d;

// 예외가 나는지 본다. (2026-11-08 WB04-E55)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 선택 범위 밖 효과를 기본 성공으로 바꾸지 않는 거부 장치이다. (2026-11-08 WB04-E55)
const missing = () => { throw new Error('Player Regular 선택 밖의 효과 미복원'); };

// 16진 문자열을 바이트로 바꾼다. (2026-11-08 WB04-E55)
const unhex = (text) => Uint8Array.from(Buffer.from(text, 'hex'));

// 원본과 같은 manager 주소 비트·State 데이터·현재 커서를 입력으로 준비한다(주소를 역참조하지 않는다). (2026-11-08 WB04-E55)
function prepareTemplateState(input) {
    const headers = new Uint8Array(180);
    const data = [];
    const streams = [];
    const names = [];
    // 9기록 각각의 다른 바이트와 오래된 커서를 보존한다.
    for (let owner = 0; owner < 9; owner += 1) {
        put(headers, owner * 20, (0x12009000 + owner * 0x100) >>> 0);
        put(headers, owner * 20 + 8, input[FIELD.mine]);
        put(headers, owner * 20 + 12, input[FIELD.transient]);
        put(headers, owner * 20 + 16, input[FIELD.prevent]);
        const bytes = new Uint8Array(64);
        bytes[0] = 0x70 + owner;
        // 관찰된 State 꼬리를 명시 입력으로 준비한다.
        for (let index = 1; index < 64; index += 1) bytes[index] = (index * 7 + owner) & 0xff;
        data.push(bytes);
        streams.push({ bytes, position: 7, end: input[FIELD.end], present: true });
        names.push(input[FIELD.dot] ? '.map' : 'map.fort');
    }
    return { headers, data, streams, names };
}

// player-regular 한 행의 실제 장면을 실행하고 17열 출력을 만든다(cpppj PlayerRegularScene). (2026-11-08 WB04-E55)
function replayRegularRow(row) {
    const { types } = realPlayerAssets();
    const input = row[1].split(',').map((item) => Number(item) >>> 0);
    assert.equal(input.length, 16);
    const pool = new SidPool(32768, { server: true });
    const unpop = new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536));
    const book = new SquidPostPopState();
    const post = new SquidPostPop(pool, types, book);
    const factory = new SquidFactory(pool, types, { unpop });
    const destroy = new RawSquidDestroy(pool, unpop, types);
    const kernel = new Kernel();
    const clock = new SquidProcessState();
    const host = new SquidProcessHost(pool, types, kernel, destroy, clock, { regular: missing });
    const mode = new SquidOwnerMode();
    const playerState = new PlayerWorldState();
    const players = new Uint8Array(9 * PLAYER_RECORD_STRIDE);
    const unselected = { assertion: missing, find: missing, create: missing, setOwner: missing, pop: missing, reserveRegular: missing, addRegular: missing };
    const player = new RawPlayerWorld(pool, post, players, playerState, unselected);
    playerState.battleOwner = input[FIELD.local];
    playerState.templateOwner = input[FIELD.templateOwner];
    playerState.playerKnown = 1;
    mode.battle = input[FIELD.battle] !== 0;
    const mission = { value: input[FIELD.mission] !== 0 };
    clock.now = 17.25;
    const sid = factory.create(152, 2);
    pool.allocated(sid)[34] = input[FIELD.owner] & 0xff;
    const { headers, data, streams, names } = prepareTemplateState(input);
    const events = [];
    let assertions = 0;
    let current = null;
    const templateState = new RawPlayerTemplateState(headers, streams, names, mission, (reason) => {
        events.push(`A:${TEMPLATE_LINES[reason]}`);
        assertions += 1;
    });
    const regular = new RawPlayerRegular(player, mode, {
        assertion: (kind) => {
            assert.equal(kind, PLAYER_REGULAR_ASSERTION.unknownEvent);
            events.push(`A:${REGULAR_UNKNOWN_LINE}`);
            assertions += 1;
        },
        log: (group, text) => {
            events.push(`L:${group}:${Buffer.from(text, 'latin1').toString('hex')}`);
            if (text === 'SORE LOSER TEST: ' && input[FIELD.mutation] === 1) mode.battle = false;
            if (text === 'SORE LOSER TEST: ' && input[FIELD.mutation] === 2) mode.battle = true;
        },
        reserveRegular: () => { events.push('N:40'); return input[FIELD.reserve] !== 0; },
        addRegular: (parent, event, payload) => { current = host.addRegular(parent, event, payload); events.push(`B:46:${parent}:0:80`); },
        templateState: (slot) => {
            events.push(`T:${slot}`);
            const result = templateState.get(slot);
            if (input[FIELD.mutation] === 3) playerState.templateOwner = 3;
            return result;
        },
        templateChanged: (slot) => {
            events.push(`C:${slot}`);
            if (input[FIELD.mutation] === 4) put(headers, slot * 20 + 8, 0);
            templateState.changed(slot);
        },
    });
    let status = 'return';
    let returned = '-';
    let boundary = '-';
    try {
        if (input[FIELD.kind] === 0) returned = String(floatToBits(regular.handle(sid, input[FIELD.event], input[FIELD.count], bitsToFloat(input[FIELD.payload]))));
        else if (input[FIELD.kind] === 1) {
            events.push(`T:${input[FIELD.templateOwner]}`);
            templateState.get(input[FIELD.templateOwner]);
        } else {
            events.push(`C:${input[FIELD.templateOwner]}`);
            templateState.changed(input[FIELD.templateOwner]);
        }
    } catch (error) {
        // 원본 관측이 멈춘 미연결 경계(빈 State 생성·짧은 State 할당·파일 저장)만 받아들인다.
        const where = { [PLAYER_TEMPLATE_STATE_BOUNDARY.initialization]: '0044d1b0', [PLAYER_TEMPLATE_STATE_BOUNDARY.allocation]: '004e4391', [PLAYER_TEMPLATE_STATE_BOUNDARY.save]: '0044d8d0' }[error.message];
        if (!where) throw error;
        status = 'boundary';
        returned = '-';
        boundary = where;
    }
    // get 뒤 현재 position/end 와 State 첫 28 BYTE 를 기록한다.
    const states = data.map((bytes, owner) => `${streams[owner].position},${streams[owner].end},${hex(bytes.subarray(0, 28))}`).join(';');
    const active = [];
    // Kernel 슬롯 순서대로 실제 PID 를 기록한다.
    if (kernel.size) for (let pid = 1; pid <= PROCESS_CAPACITY; pid += 1) if (kernel.get(pid)) active.push(pid);
    const fields = current ? `${doubleBits(current.time)},${current.event},${floatToBits(current.payload)},${current.count}` : '-';
    const form = current ? current.form : 0;
    const output = [status, returned, boundary, events.length ? events.join(';') : '-', hex(headers), states, String(playerState.battleOwner), String(playerState.templateOwner),
        mode.battle ? '1' : '0', String(0xffffffff), fields, hex(pool.slot(sid)), String(form), form ? hex(pool.slot(form)) : '-', active.length ? active.join(',') : '-',
        String(clock.typeRotation), String(assertions)];
    kernel.clear();
    return output;
}

test('PlayerRegular: 원본 10.78 처리기·State getter·Notify 329행이 실제 Regular/Form/Kernel 과 같다', { skip: !regularRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 출력은 실제 몸체 실행 뒤에만 비교한다.
    for (const row of regularRows) {
        assert.equal(row.length, 19);
        assert.deepEqual(replayRegularRow(row), row.slice(2), `PlayerRegular 행 ${count} 입력 ${row[1]}`);
        count += 1;
    }
    assert.equal(count, 329);
});

test('PlayerStateExpand: 원본 99행의 짧은 State 확장(원문 보존·0 확장·position/end)과 다음 get 의 같은 현재 버퍼', { skip: !expandRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // C++/JS 로 기대값을 계산하지 않고 원본 행 전체와 현재 메모리를 대조한다.
    for (const row of expandRows) {
        assert.equal(row.length, 10);
        const length = Number(row[0]);
        const pattern = Number(row[1]);
        const old = new Uint8Array(64);
        const fresh = new Uint8Array(64).fill(0xa5);
        // 원본 관측기와 같은 독립 물리 바이트만 입력한다.
        for (let index = 0; index < old.length; index += 1) old[index] = (index * 17 + pattern) & 0xff;
        const before = old.slice();
        const headers = new Uint8Array(180);
        headers[20] = 1;
        const streams = Array.from({ length: 9 }, () => ({ bytes: new Uint8Array(0), position: 0, end: 0, present: true }));
        streams[1] = { bytes: old, position: 13, end: length, present: true };
        const names = new Array(9).fill('');
        names[1] = 'tutorial1';
        let allocations = 0;
        const body = new RawPlayerTemplateState(headers, streams, names, { value: true }, () => { throw new Error('State 확장 정상 입력의 예상 밖 진단'); }, {
            allocate: (owner, size) => { assert.ok(owner === 1 && size === 28); allocations += 1; return fresh.subarray(0, size); },
        });
        const bytes = body.get(1);
        assert.deepEqual([allocations, streams[1].position, streams[1].end], [Number(row[2]), Number(row[3]), Number(row[4])], `확장 행 ${count}`);
        assert.equal(hex(bytes.subarray(0, 28)), row[5]);
        assert.deepEqual(old, before);
        // 두 정밀도 열과 다음 get 의 같은 현재 버퍼/position 0 도 확인한다.
        for (let index = 2; index < 6; index += 1) assert.equal(row[index], row[index + 4]);
        const again = body.get(1);
        assert.ok(again.buffer === bytes.buffer && again.byteOffset === bytes.byteOffset && streams[1].position === 0 && allocations === Number(row[2]));
        count += 1;
    }
    assert.equal(count, 99);
});

test('PlayerStateInitialization: 원본 2560행의 빈 State 생성 조건·보존 스택/flags·28 BYTE 쓰기와 기존 State 자연 반환', { skip: !initializationRows && '원본 fixture 없음' }, () => {
    assert.equal(initializationRows.length, 2560);
    // 각 행의 스택 미정 바이트는 원본 보장 초기값이 아닌 독립 입력이다.
    for (const row of initializationRows) {
        assert.equal(row.length, 6);
        const input = row[0].split(',').map(Number);
        assert.equal(input.length, 4);
        const [seed, , present, end] = input;
        let stack = unhex(row[1]);
        const expectedNode = unhex(row[3]);
        const buffer = new Uint8Array(64);
        // 같은 원문 버퍼의 생성 경계 입력을 준비하며 첫 28 BYTE 밖의 보존도 대조한다.
        for (let offset = 0; offset < buffer.length; offset += 1) buffer[offset] = (offset * 13 + seed) & 0xff;
        const headers = new Uint8Array(180);
        headers[20] = 1;
        const streams = Array.from({ length: 9 }, () => ({ bytes: new Uint8Array(0), position: 0, end: 0, present: true }));
        streams[1] = { bytes: buffer, position: 17, end, present: present !== 0 };
        let events = 'S';
        const body = new RawPlayerTemplateState(headers, streams, new Array(9).fill(''), { value: true }, () => { throw new Error('State 초기화 예상 밖 진단'); });
        body.bindInitialization({
            bytes: (slot, output) => { assert.equal(slot, 1); output.set(stack); },
            replace: (slot, bytes) => {
                assert.equal(slot, 1);
                events += 'C';
                stack = bytes.slice();
                buffer.set(bytes, 0);
                streams[1] = { bytes: buffer, position: 28, end: 28, present: true };
                return streams[1];
            },
        });
        const result = body.get(1);
        events += 'E';
        assert.ok(result.buffer === buffer.buffer && result.byteOffset === 0);
        assert.equal(hex(stack), row[2]);
        assert.equal(hex(buffer), row[4]);
        assert.equal(events, row[5]);
        assert.deepEqual([streams[1].position, streams[1].end, streams[1].present], [get(expectedNode, 4), get(expectedNode, 8), true]);
    }
});

test('PlayerStateInitialization: 미연결 생성은 앞선 position 0 을 보존한 채 거부하고 생성 결과를 합성하지 않는다', () => {
    const headers = new Uint8Array(180);
    headers[20] = 1;
    const streams = Array.from({ length: 9 }, () => ({ bytes: new Uint8Array(1), position: 17, end: 0, present: true }));
    const state = new RawPlayerTemplateState(headers, streams, new Array(9).fill(''), { value: true }, () => {});
    assert.ok(throws(() => state.get(1)) && streams[1].position === 0);
    assert.ok(throws(() => state.bindInitialization({})));
    assert.ok(throws(() => state.bindMinimumSize(null)) && throws(() => state.bindFileSave(undefined)));
});

test('PlayerTemplateState: 초기화는 원본이 쓰는 필드만 바꾸고 최소 확장 부모가 돌려준 새 스트림을 쓴다(웹 전용 계약)', () => {
    const bytes = new Uint8Array(28).fill(0xff);
    initializePlayerTemplateStateBytes(bytes);
    assert.equal(hex(bytes), 'f9ffffff000000000000ffff000000000000ffffffff00000000ffff');
    assert.ok(throws(() => initializePlayerTemplateStateBytes(new Uint8Array(27))));
    // 최소 확장 부모가 있으면 분리 할당을 쓰지 않는다.
    const headers = new Uint8Array(180);
    headers[0] = 1;
    const short = { bytes: new Uint8Array(64), position: 9, end: 5, present: true };
    const fresh = { bytes: new Uint8Array(28).fill(7), position: 28, end: 28, present: true };
    const streams = [short, ...Array.from({ length: 8 }, () => short)];
    const state = new RawPlayerTemplateState(headers, streams, new Array(9).fill('a'), { value: false }, () => {}, { allocate: () => { throw new Error('분리 할당 금지'); } });
    state.bindMinimumSize((slot, size) => { assert.ok(slot === 0 && size === 28); return fresh; });
    assert.ok(state.get(0) === fresh.bytes && short.position === 0);
    // Notify 는 isMine/preventSave 관문 뒤에만 실제 저장 부모를 부른다.
    const saved = [];
    state.bindFileSave((slot) => saved.push(slot));
    state.changed(0);
    put(headers, 16, 1);
    state.changed(0);
    assert.deepEqual(saved, [0]);
    // manager 0 은 파일명 첫 BYTE 물리 접근 경계로 거부한다.
    headers[0] = 0;
    assert.ok(throws(() => state.changed(0)));
});
