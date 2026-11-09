// 미션 승패 조건 raw-mission-outcome.js(Check 004c36c0·Event 004c3940·사제 사망/포획 004922e0)를 원본 10.78 관측과 cpppj `MissionOutcomeTests` 계약으로 대조한다.
//  - 원본 관측 missionoutcome-x86.tsv 의 10.78(originals) 364행: 같은 명시 입력(로컬 번호·데모·Tell 차단·섹션 발견·변이·영역 소유자·플레이어 9명 활성/신전/사제 상태·방향별 9×9 동맹·43개 사건 잠금·사제 목록·슬롯 raw)에서
//    종료 시점의 입력/잠금 전체 문자열·효과 호출 순서와 그 당시 상태·반환값이 한 바이트도 다르지 않다. 같은 행을 독립 입력 상태로(일반 재생)와 실제 Player 원문/방향별 동맹/현재 전역 getter 로(실연결 재생: 단방향 0/비영 동맹
//    조합과 콜백 변이 8행) 두 번 실행해, 독립 입력 사본을 오염시켜도 현재 입력을 읽는지 확인한다.
//  - 계약: 사건 콜백 재진입은 막지 않고 잠금은 콜백이 끝난 뒤 쓴다, 효과 누락/다른 풀 보상 거부, 사제 사망/포획이 공유 표의 2/1/0 변경을 즉시 읽고 데이터 reset 이 같은 주소에 적용된다.
// CD 10.72(originalCD) 행은 웹 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E233: 미션 승패 조건 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { PriestPostPopState, resetPriestOwnerState } from '../www/core/raw-priest-post-pop.js';
import { RawMissionOutcome, createMissionOutcomeState, makeMissionOutcomeHooks } from '../www/core/raw-mission-outcome.js';

// 저장된 독립 기계어 출력을 한 번만 읽는다(19칸). originals 관찰만 쓴다. (2026-11-10 WB04-E233)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'missionoutcome-x86.tsv'));
const observations = rows ? rows.filter((row) => row[0] === 'originals') : null;

// 독립 관찰의 입력 수와 실제 SidPool 이 요구하는 용량이다. 관찰의 사용 SID 는 128 미만이다. (2026-11-10 WB04-E233)
const CASES = 364;
const CAPACITY = 32768;
const STRIDE = 0xb4;

// 비정렬 DWORD 를 little endian 으로 읽는다. (2026-11-10 WB04-E233)
const dwordAt = (bytes, at) => (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
// 쉼표 구분 수 목록이다(빈 문자열은 빈 목록). (2026-11-10 WB04-E233)
const numbers = (text) => (text === '' ? [] : text.split(',').map(Number));
// DWORD 목록을 fixture 와 같은 쉼표 순서로 기록한다. (2026-11-10 WB04-E233)
const valuesText = (values) => Array.from(values).join(',');
// 원본 E0~188 의 43개 잠금 순서에 대응하는 (대상, 키) 쌍이다. 플레이어 0 의 잠금은 원본 검사에 없다. (2026-11-10 WB04-E233)
function latchSlots(state) {
    const slots = new Array(43);
    for (let player = 1; player <= 8; player += 1) {
        const latch = state.latches[player];
        const first = (player - 1) * 5;
        slots[first] = [latch, 'templeDead'];
        slots[first + 1] = [latch, 'templeHalfDead'];
        slots[first + 2] = [latch, 'priestDead'];
        slots[first + 3] = [latch, 'priestCaptured'];
        slots[first + 4] = [latch, 'priestSaved'];
    }
    slots[40] = [state, 'goodTeamDead'];
    slots[41] = [state, 'badTeamDead'];
    slots[42] = [state, 'failed'];
    return slots;
}
// 모든 효과 시점의 현재 공유 상태를 원본과 같은 문자열로 보존한다. (2026-11-10 WB04-E233)
function snapshot(view, priests) {
    const flags = view.players.map((player) => player.flags);
    const temples = view.players.map((player) => player.temple);
    const latches = latchSlots(view).map(([target, key]) => target[key]);
    return `${valuesText([view.localPlayer, view.demo, view.tellBlocked])}/${valuesText(flags)}/${valuesText(temples)}/${valuesText(priests.ownerState)}/${valuesText(latches)}`;
}
// 임의 성공/실패 조건을 계산하지 않고 명시한 callback 입력 변화만 수행한다. (2026-11-10 WB04-E233)
function mutate(kind, mutation, mutated, view, priests) {
    if (mutated.has(kind)) return;
    mutated.add(kind);
    if (kind === 'section' && mutation === 1) { view.localPlayer = 2; view.players[2].flags = 0; priests.ownerState[1] = 2; view.players[1].temple = 0; }
    else if (kind === 'close' && mutation === 2) { view.tellBlocked = 1; view.demo = 1; priests.ownerState[1] = 1; }
    else if (kind === 'stop' && mutation === 3) { view.latches[1].templeHalfDead = 7; priests.ownerState[2] = 2; }
    else if (kind === 'region' && mutation === 4) view.localPlayer = 2;
}

// 같은 fixture 를 독립 입력 상태로(live=false: 364행) 또는 실제 Player 원문/방향별 동맹/현재 전역 getter 로(live=true: 조합 8행) 재생한다. (2026-11-10 WB04-E233)
function checkFixture(live) {
    const counts = { originals: 0 };
    const liveAlliesSeen = new Set();
    let number = 0;
    for (const row of observations) {
        assert.equal(row.length, 19);
        number += 1;
        if (live && Number(row[6]) === 0) {
            if (row[1] !== 'Check' || row[2] !== '1' || row[3] !== '0' || row[4] !== '0' || row[5] !== '1') continue;
            const pairs = numbers(row[12]); // 단방향 0/비영 조합마다 기존 native 사례 하나만 추가 대조한다.
            const key = `${row[0]}:${pairs[11]}:${pairs[19]}`;
            if ((pairs[11] !== 0 && pairs[11] !== 7) || (pairs[19] !== 0 && pairs[19] !== 9) || liveAlliesSeen.has(key)) continue;
            liveAlliesSeen.add(key);
        }
        counts.originals += 1;
        const pool = new SidPool(CAPACITY);
        const priests = new PriestPostPopState();
        const state = createMissionOutcomeState();
        state.localPlayer = Number(row[2]);
        state.demo = Number(row[3]);
        state.tellBlocked = Number(row[4]);
        const flags = numbers(row[9]);
        const temples = numbers(row[10]);
        const statuses = numbers(row[11]);
        const allies = numbers(row[12]);
        const latches = numbers(row[13]);
        for (let player = 0; player < 9; player += 1) { // 번호 0 의 입력도 보존해 유효 번호만 읽는지 검사한다.
            state.players[player] = { flags: flags[player], temple: temples[player] };
            priests.ownerState[player] = statuses[player];
            for (let other = 0; other < 9; other += 1) state.allies[player][other] = allies[player * 9 + other]; // 동맹 표는 bool 이 아닌 원본 DWORD 값이다.
        }
        const slots = latchSlots(state);
        for (let index = 0; index < slots.length; index += 1) slots[index][0][slots[index][1]] = latches[index]; // 원본 시작 잠금은 0 뿐 아니라 비제로 다른 DWORD 도 포함한다.
        priests.priests.entries = numbers(row[14]);
        priests.priests.count = priests.priests.entries.length;
        const types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0 }));
        for (const item of row[15].split(';')) { // raw 필드는 기계어 입력의 폭/비트로만 작성한다.
            if (item === '') continue;
            const node = numbers(item);
            const raw = pool.physical(node[0]);
            raw[10] = node[1];
            raw[11] = node[2];
            raw[34] = node[3];
            put(raw, 26, node[4], 4);
            put(raw, 14, node[6]);
            put(raw, 18, node[7]);
            types[node[1]].maxHitPoints = node[5] | 0;
            types[node[1]].flags1 = 0x10;
        }
        const reward = new SquidReward(pool, types, new SquidPostPopState(), new SquidRewardState(), new ScrambledSpStore(new GameRandom(), () => 1));
        const records = new Uint8Array(9 * STRIDE);
        const currentAllies = new Array(81).fill(0);
        let local = state.localPlayer;
        let demo = state.demo;
        let blocked = state.tellBlocked;
        // 조회 callback 의 관찰용 view 만 만들며 실제 outcome 입력은 계속 raw 원문에서 읽는다.
        const current = () => {
            if (!live) return state;
            const view = { ...state, players: [], allies: [], latches: state.latches.map((latch) => ({ ...latch })), localPlayer: local, demo, tellBlocked: blocked };
            for (let p = 0; p < 9; p += 1) { // 각 기록의 실제 폭/필드만 관찰한다. outcome state 의 입력 사본은 읽지 않는다.
                view.players[p] = { flags: records[p * STRIDE + 0x50], temple: dwordAt(records, p * STRIDE + 0x7c) };
                view.allies[p] = [];
                for (let other = 0; other < 9; other += 1) view.allies[p][other] = currentAllies[p * 9 + other]; // 방향별 원본 DWORD 를 관찰용 문자열에만 복사한다.
            }
            return view;
        };
        // native callback 이 지정한 입력 변경을 실제 연결 원문에 쓰며 latch 는 같은 장기 상태에 쓴다.
        const publish = (view) => {
            local = view.localPlayer;
            demo = view.demo;
            blocked = view.tellBlocked;
            view.latches.forEach((latch, index) => Object.assign(state.latches[index], latch));
            for (let p = 0; p < 9; p += 1) {
                records[p * STRIDE + 0x50] = view.players[p].flags;
                put(records, p * STRIDE + 0x7c, view.players[p].temple >>> 0, 4);
                for (let other = 0; other < 9; other += 1) currentAllies[p * 9 + other] = view.allies[p][other]; // 양방향을 합치지 않고 각 DWORD 를 그대로 쓴다.
            }
        };
        if (live) {
            publish(state);
            state.players = Array.from({ length: 9 }, () => ({ flags: 0, temple: 0 })); // 독립 입력을 오염시켜 stale 사본 사용을 검출한다.
            state.allies = Array.from({ length: 9 }, () => new Array(9).fill(0));
            state.localPlayer = 9;
            state.demo = 7;
            state.tellBlocked = 9;
        }
        const mutation = Number(row[6]);
        const mutated = new Set();
        let trace = '';
        const add = (value) => { trace += (trace === '' ? '' : '|') + value; };
        const change = (kind) => {
            if (!live) { mutate(kind, mutation, mutated, state, priests); return; }
            const view = current();
            mutate(kind, mutation, mutated, view, priests);
            publish(view);
        };
        const snap = () => snapshot(current(), priests);
        const hooks = {
            regionOwner: (x, y) => { add(`region:${valuesText([floatToBits(x), floatToBits(y)])}#${snap()}`); change('region'); return Number(row[7]); },
            section: (name) => { add(`section:${name}#${snap()}`); change('section'); return Number(row[5]) !== 0; },
            closeTransientGumps: () => { add(`close#${snap()}`); change('close'); },
            onSection: () => { add(`stop#${snap()}`); change('stop'); },
            invalidPlayerAssert: (player) => { add(`assert:${player}#${snap()}`); },
        };
        const outcome = new RawMissionOutcome(pool, priests, state, makeMissionOutcomeHooks(pool, reward, hooks));
        let result = 0;
        if (live) outcome.bindCurrentInputs({ players: records, allies: currentAllies, localPlayer: () => local, demo: () => demo, tellBlocked: () => blocked });
        if (row[1] === 'Check') outcome.check();
        else if (row[1] === 'Dead') result = Number(outcome.priestDead(Number(row[8])));
        else if (row[1] === 'Captured') result = Number(outcome.priestCaptured(Number(row[8])));
        else if (row[1] === 'Event') result = Number(outcome.event(state, 'goodTeamDead', Number(row[8]) !== 0, 'BadTeamDead'));
        else assert.fail('미션 fixture 동작 이름 오류');
        const label = `행 ${number} ${row[0]} ${row[1]}`;
        assert.equal(snap(), row[16], `${label} 상태`);
        assert.equal(trace, row[17], `${label} trace`);
        assert.equal(result, Number(row[18]), `${label} 반환`);
    }
    assert.equal(counts.originals, live ? 8 : CASES);
}

test('미션 승패 조건: 원본 10.78 364행의 사건 순서·잠금·사제 상태·HP 경계·동맹·효과 당시 상태가 모두 같다', { skip: !rows && '원본 fixture 없음' }, () => checkFixture(false));

test('미션 승패 조건: 실제 Player 원문/방향별 동맹/현재 전역 getter 만 읽어도 callback 8행·동맹 조합 8행이 같다(독립 입력 사본은 오염시킴)', { skip: !rows && '원본 fixture 없음' }, () => checkFixture(true));

test('미션 승패 조건 계약: 같은 사건 callback 의 재진입도 최종 잠금 쓰기 전에는 실행 가능하다', () => {
    const pool = new SidPool(CAPACITY);
    const priests = new PriestPostPopState();
    const state = createMissionOutcomeState();
    let trace = '';
    let nested = false;
    let outcome = null;
    const hooks = {
        maxHitPoints: () => 100,
        regionOwner: () => 0,
        section: () => {
            trace += 'S';
            assert.equal(state.badTeamDead, 0);
            if (!nested) { nested = true; assert.ok(outcome.event(state, 'badTeamDead', true, 'BadTeamDead')); assert.equal(state.badTeamDead, 1); }
            return true;
        },
        closeTransientGumps: () => { trace += 'C'; },
        onSection: () => { trace += 'V'; },
    };
    outcome = new RawMissionOutcome(pool, priests, state, hooks);
    assert.ok(outcome.event(state, 'badTeamDead', true, 'BadTeamDead'));
    assert.equal(trace, 'SSCVCV');
    assert.equal(state.badTeamDead, 1);
    assert.ok(!outcome.event(state, 'badTeamDead', false, 'BadTeamDead'));
    assert.equal(state.badTeamDead, 1);
});

test('미션 승패 조건 계약: 연결 누락과 다른 풀의 보상 몸체를 조기 거부한다', () => {
    const pool = new SidPool(CAPACITY);
    const other = new SidPool(CAPACITY);
    const priests = new PriestPostPopState();
    const state = createMissionOutcomeState();
    assert.throws(() => new RawMissionOutcome(pool, priests, state, {}), TypeError);
    const complete = { maxHitPoints: () => 0, regionOwner: () => 0, section: () => false, closeTransientGumps: () => {}, onSection: () => {} };
    for (const name of Object.keys(complete)) assert.throws(() => new RawMissionOutcome(pool, priests, state, { ...complete, [name]: undefined }), TypeError, name);
    const types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0 }));
    const reward = new SquidReward(other, types, new SquidPostPopState(), new SquidRewardState(), new ScrambledSpStore(new GameRandom(), () => 1));
    assert.throws(() => makeMissionOutcomeHooks(pool, reward, {}), TypeError);
    // 실연결 입력이 부족하면 이전 연결을 유지한 채 거부한다.
    const outcome = new RawMissionOutcome(pool, priests, state, complete);
    assert.throws(() => outcome.bindCurrentInputs({ players: new Uint8Array(10), allies: new Array(81).fill(0), localPlayer: () => 1, demo: () => 0, tellBlocked: () => 0 }), TypeError);
    assert.throws(() => outcome.bindCurrentInputs({ players: new Uint8Array(9 * STRIDE), allies: new Array(80).fill(0), localPlayer: () => 1, demo: () => 0, tellBlocked: () => 0 }), TypeError);
    assert.throws(() => outcome.bindCurrentInputs({ players: new Uint8Array(9 * STRIDE), allies: new Array(81).fill(0), localPlayer: () => 1, demo: () => 0 }), TypeError);
});

test('미션 승패 조건 계약: 사제 사망/포획 조회가 공유 표의 2/1/0 변경을 즉시 읽고 데이터 reset 이 같은 주소에 적용된다', () => {
    const pool = new SidPool(CAPACITY);
    const priests = new PriestPostPopState();
    const state = createMissionOutcomeState();
    const shared = new Array(9).fill(31);
    priests.ownerState.fill(2);
    priests.sharedOwnerState = shared;
    priests.priests.entries = [50, 60, 77];
    priests.priests.count = 1;
    state.players[1].flags = 0x22;
    const fail = (message) => () => { throw new Error(message); };
    const outcome = new RawMissionOutcome(pool, priests, state, {
        maxHitPoints: fail('사제 owner 상태 조회에서 HP 선택'), regionOwner: fail('사제 owner 상태 조회에서 지역 선택'), section: fail('사제 owner 상태 조회에서 Tell 선택'),
        closeTransientGumps: fail('사제 owner 상태 조회에서 UI 선택'), onSection: fail('사제 owner 상태 조회에서 UI 선택'),
    });
    const entries = [...priests.priests.entries];
    const fallback = [...priests.ownerState];
    shared[1] = 2;
    assert.ok(outcome.priestDead(1) && !outcome.priestCaptured(1));
    shared[1] = 1;
    assert.ok(!outcome.priestDead(1) && outcome.priestCaptured(1));
    resetPriestOwnerState(priests.currentOwnerState(), 1);
    assert.ok(!outcome.priestDead(1) && !outcome.priestCaptured(1) && shared[1] === 0 && shared[0] === 31);
    assert.deepEqual(priests.ownerState, fallback);
    assert.deepEqual(priests.priests.entries, entries);
    assert.equal(priests.priests.count, 1);
});

test('미션 승패 조건 계약: 범위 밖 번호의 진단 인자는 부호 없는 DWORD, 음수 최대 HP 의 절반은 0 쪽 절삭, 로컬 번호·실연결 입력 길이/getter 거부', () => {
    const pool = new SidPool(CAPACITY);
    const priests = new PriestPostPopState();
    const state = createMissionOutcomeState();
    const names = [];
    const asserted = [];
    let maximum = 100;
    const hooks = {
        maxHitPoints: () => maximum, regionOwner: () => 0, section: (name) => { names.push(name); return false; }, closeTransientGumps: () => {}, onSection: () => {},
        invalidPlayerAssert: (player) => { asserted.push(player); },
    };
    const outcome = new RawMissionOutcome(pool, priests, state, hooks);
    // 범위 밖 번호(−1)는 진단 인자가 부호 없는 DWORD 이고 죽음/포획 모두 true 이다.
    assert.ok(outcome.priestDead(-1) && outcome.priestCaptured(-1));
    assert.deepEqual(asserted, [4294967295, 4294967295]);
    // 신전 HP −2, 최대 HP −3: 절반은 0 방향 절삭 −1 이므로 −2 < −1 이다(내림이면 −2 < −2 가 거짓).
    state.players[1].flags = 0x22;
    state.players[1].temple = 60;
    put(pool.physical(60), 26, 0xfffffffe, 4);
    maximum = -3;
    state.localPlayer = 1;
    outcome.check();
    assert.ok(names.includes('ai1TempleHalfDead'));
    // 사제가 살아 있는 활성 플레이어가 있으면 로컬 번호가 9 일 때 동맹 조회 전에 범위 오류로 거부한다(메시지로 구별).
    state.localPlayer = 9;
    assert.throws(() => outcome.check(), (error) => error instanceof RangeError && error.message.includes('로컬 번호'));
    // 실연결 입력은 정확히 9×180 BYTE·81 DWORD 와 세 getter 가 모두 있어야 한다.
    const good = { players: new Uint8Array(9 * STRIDE), allies: new Array(81).fill(0), localPlayer: () => 1, demo: () => 0, tellBlocked: () => 0 };
    assert.throws(() => outcome.bindCurrentInputs({ ...good, players: new Uint8Array(9 * STRIDE + 1) }), TypeError);
    for (const name of ['localPlayer', 'demo', 'tellBlocked']) assert.throws(() => outcome.bindCurrentInputs({ ...good, [name]: undefined }), TypeError, name);
    outcome.bindCurrentInputs(good);
});

