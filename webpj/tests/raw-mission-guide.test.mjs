// Guide 의 현재 미션 포인터·pending/창 관문·단계 반복(raw-mission-guide.js, cpppj `o/RawMissionGuide`)을 원본 10.78 관측과 cpppj 시험, 웹 계약으로 확인한다.
//  - 원본 관측: mission-guide-x86.tsv 1550행(Run 반복 R · Advance A × 두 미션 객체의 flags/sequence × pending/창 관문 × 호출 도중 외부 변경 7가지(action 1~6·8) × 현재 포인터 조회 횟수)의 최종 현재 식별자·두 객체의 전체 208BYTE·외부 호출 순서를 전부 대조한다.
//  - cpppj 시험 이식: MissionGuideTests 의 Native1550 · StageFiveDefersReleaseUntilNextRun · CurrentGetterSeparatesStageSelectionAndTarget · RequiresEveryBoundary(판본 인자는 웹이 10.78 만 옮겨 제외).
//  - 웹 계약: Advance 의 32비트 감김과 flags 비트 0 해제, 단계 6 이면 같은 Run 에서 release 후 종료, 범위 밖 단계 진단, pending 비영이면 창 관문을 읽지 않음(짧은 평가), 해제 비트 8, 현재 NULL 손상 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-12 WB04-E340)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { put, readRows } from './raw-support.mjs';
import { createMissionStartState } from '../www/core/raw-mission-start.js';
import { RawMissionGuide } from '../www/core/raw-mission-guide.js';

// 원본 관측 행이다. 파일이 없으면 재생 검사를 건너뛴다. (2026-11-12 WB04-E340)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'mission-guide-x86.tsv'));
// 원본 객체 전체 크기(0xd0)와 격리된 가상 표 번호이다(호스트 함수 주소가 아님). (2026-11-12 WB04-E340)
const MISSION_BYTES = 0xd0;
const GUIDE_VTABLE = 0x12002000;

// 소문자 16진 문자열로 바꾼다. (2026-11-12 WB04-E340)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');
// 예외가 나는지 본다. (2026-11-12 WB04-E340)
function throws(action, kind) {
    try { action(); } catch (error) { return kind ? error instanceof kind : true; }
    return false;
}

// 한 행의 13개 독립 입력과 두 미션 객체를 만들고 외부 효과를 기록하는 장면이다. cpppj GuideScene 에 대응한다. (2026-11-12 WB04-E340)
class GuideScene {
    // input = active / 두 객체의 flags·sequence / pending / dialog / trigger / action / value / ordinal / budget / runs 이다.
    constructor(values) {
        this.input = values.split(',').map(Number);
        assert.equal(this.input.length, 13);
        this.first = { ...createMissionStartState(), flags: this.input[1], sequence: this.input[2], tellAtIndex: 0x12345679, fieldC0: 0x89abcdee, fieldC4: 0x13579be0 };
        this.second = { ...createMissionStartState(), flags: this.input[3], sequence: this.input[4], tellAtIndex: 0x1234567a, fieldC0: 0x89abcded, fieldC4: 0x13579be1 };
        this.active = this.input[0];
        this.budget = this.input[11];
        this.getters = 0;
        this.trace = '';
    }

    // 외부 경계 호출 한 번의 순서를 이어 붙인다.
    add(value) {
        if (this.trace) this.trace += ';';
        this.trace += value;
    }

    // 식별자에 해당하는 현재 미션을 빌린다. 범위 밖 포인터는 성공으로 바꾸지 않는다.
    object(identifier) {
        if (identifier > 2) throw new RangeError('Guide 현재 입력 번호 범위 오류');
        return identifier === 0 ? null : identifier === 1 ? this.first : this.second;
    }

    // 외부 가상 호출이 받은 실제 대상 객체에서 번호를 찾는다(현재 포인터와 다를 수 있음).
    identifier(state) {
        if (state === this.first) return 1;
        if (state === this.second) return 2;
        throw new Error('Guide 실제 가상 대상 누락');
    }

    // fixture 에 적힌 외부 효과만 적용한다. action 1 은 제품의 실제 advance 를 호출하며 기대 출력을 쓰지 않는다.
    mutate(trigger, target) {
        if (trigger !== this.input[7] || this.budget === 0 || (trigger === 6 && this.getters !== this.input[10])) return;
        this.budget -= 1;
        const state = this.object(target);
        const action = this.input[8];
        if (action === 1 && state) RawMissionGuide.advance(state);
        else if (action === 2 && state) state.flags = (state.flags | 1) >>> 0;
        else if (action === 3 && state) state.flags = (state.flags | 8) >>> 0;
        else if (action === 4 && state) state.sequence = this.input[9];
        else if (action === 5) this.active = 0;
        else if (action === 6) this.active = target === 2 ? 1 : 2;
        else if (action === 8 && state) { state.flags = (state.flags | 1) >>> 0; state.sequence = this.input[9]; }
    }

    // 원본의 명시 외부 경계만 조립한다.
    hooks() {
        return {
            current: () => { this.getters += 1; this.mutate(6, this.active); this.add(`C,${this.active}`); return this.object(this.active); },
            pending: () => { this.add('P'); this.mutate(1, this.active); return this.input[5]; },
            dialogsClear: () => { this.add('D'); this.mutate(2, this.active); return this.input[6] !== 0; },
            stage: (state, sequence) => { const target = this.identifier(state); this.add(`S,${target},${sequence}`); this.mutate(0, target); },
            postpone: (state) => { const target = this.identifier(state); this.add(`H,${target}`); this.mutate(3, target); },
            release: () => { const target = this.active; this.add('L'); this.active = 0; this.mutate(5, target); },
            invalidStage: () => { this.add('A'); this.mutate(4, this.active); },
        };
    }

    // 같은 상태를 원본 물리 필드 위치에 써 전체 208BYTE 표현으로 만든다(다섯 필드 밖 stale 바이트는 보존 확인).
    observe(identifier) {
        const bytes = new Uint8Array(MISSION_BYTES);
        const seed = identifier === 1 ? 0x29 : 0xa7;
        // 원본 준비와 같은 독립 stale 입력 패턴이다.
        for (let index = 0; index < bytes.length; index += 1) bytes[index] = (index * 37 + seed) & 0xff;
        const state = this.object(identifier);
        put(bytes, 0, GUIDE_VTABLE);
        put(bytes, 0x54, state.flags);
        put(bytes, 0x58, state.sequence);
        put(bytes, 0xa0, state.tellAtIndex);
        put(bytes, 0xc0, state.fieldC0);
        put(bytes, 0xc4, state.fieldC4);
        return hex(bytes);
    }
}

test('원본 관측 1550행: 최종 현재 식별자·두 객체 전체 208BYTE·외부 호출 순서가 모두 일치한다', { skip: ROWS === null }, () => {
    let compared = 0;
    // 예상 결과는 규칙으로 계산하지 않고 보호 PE 에서 보존한 관측을 읽는다.
    for (const row of ROWS) {
        if (row[0] === 'kind') continue;
        assert.equal(row.length, 6);
        const scene = new GuideScene(row[1]);
        const guide = new RawMissionGuide(scene.hooks());
        // 반복 Run 도 별도 원본 호출이며 단계 5→6 경계 뒤 다음 Run 을 앞당기지 않는다.
        for (let run = 0; run < scene.input[12]; run += 1) {
            if (row[0] === 'R') guide.run();
            else if (row[0] === 'A') RawMissionGuide.advance(scene.first);
            else throw new Error('Guide 원문 함수 종류 오류');
        }
        assert.equal(scene.active, Number(row[2]));
        assert.equal(scene.observe(1), row[3]);
        assert.equal(scene.observe(2), row[4]);
        assert.equal(scene.trace === '' ? '-' : scene.trace, row[5]);
        compared += 1;
    }
    assert.equal(compared, 1550);
});

test('cpppj MissionGuide_StageFiveDefersReleaseUntilNextRun: 단계 5 의 advance 는 6 까지만 만들고 같은 Run 에서 단계 6/해제를 호출하지 않는다', () => {
    const scene = new GuideScene('1,0,5,268435456,2,0,1,0,1,0,1,1,1');
    const guide = new RawMissionGuide(scene.hooks());
    guide.run();
    assert.equal(scene.first.sequence, 6);
    assert.equal(scene.active, 1);
    assert.equal(scene.trace, 'C,1;P;D;C,1;C,1;C,1;S,1,5;C,1');
    scene.trace = '';
    guide.run();
    assert.equal(scene.active, 0);
    assert.equal(scene.trace, 'C,1;P;D;C,1;C,1;C,1;S,1,6;L;C,0');
});

test('cpppj MissionGuide_CurrentGetterSeparatesStageSelectionAndTarget: 단계 선택과 가상 호출 대상의 현재 조회는 독립 경계이다', () => {
    const scene = new GuideScene('1,0,0,268435456,2,0,1,6,6,0,4,1,1');
    const guide = new RawMissionGuide(scene.hooks());
    guide.run();
    assert.equal(scene.active, 2);
    assert.equal(scene.first.sequence, 0);
    assert.equal(scene.second.sequence, 2);
    assert.equal(scene.trace, 'C,1;P;D;C,1;C,1;C,2;S,2,0;C,2;C,2;P;D;C,2;C,2;C,2;S,2,2;C,2');
});

test('cpppj MissionGuide_RequiresEveryBoundary: 경계 하나라도 없으면 생성을 거부한다', () => {
    const scene = new GuideScene('1,0,0,268435456,2,0,1,0,0,0,1,1,1');
    new RawMissionGuide(scene.hooks());
    // 각 필수 경계 하나만 누락해도 생성이 거부되어야 한다.
    for (const name of ['current', 'pending', 'dialogsClear', 'stage', 'postpone', 'release', 'invalidStage']) {
        const hooks = scene.hooks();
        delete hooks[name];
        assert.ok(throws(() => new RawMissionGuide(hooks), TypeError), name);
        hooks[name] = 5;
        assert.ok(throws(() => new RawMissionGuide(hooks), TypeError), name);
    }
    assert.ok(throws(() => new RawMissionGuide(undefined), TypeError));
});

// 단순 장면: 상태 하나와 호출 기록으로 분기를 직접 확인한다. (2026-11-12 WB04-E340)
function simple(state, overrides = {}) {
    const log = [];
    const hooks = {
        current: () => { log.push('C'); return state.value; },
        pending: () => { log.push('P'); return 0; },
        dialogsClear: () => { log.push('D'); return true; },
        stage: (target, sequence) => { log.push(`S${sequence}`); assert.equal(target, state.value); },
        postpone: () => log.push('H'),
        release: () => { log.push('L'); state.value = null; },
        invalidStage: () => log.push('A'),
        ...overrides,
    };
    return { log, guide: new RawMissionGuide(hooks) };
}

test('Advance: sequence 는 32비트로 감기고 flags 비트 0 만 지운다', () => {
    const state = { ...createMissionStartState(), flags: 0xffffffff, sequence: 0xffffffff };
    RawMissionGuide.advance(state);
    assert.equal(state.sequence, 0);
    assert.equal(state.flags, 0xfffffffe);
    state.flags = 0x80000001;
    RawMissionGuide.advance(state);
    assert.equal(state.flags, 0x80000000);
    assert.equal(state.sequence, 1);
});

test('현재 미션이 없으면 아무 경계도 부르지 않는다', () => {
    const state = { value: null };
    const scene = simple(state);
    scene.guide.run();
    assert.deepEqual(scene.log, ['C']);
});

test('해제 비트 8 이 서 있으면 현재 미션이 아직 있을 때만 release 를 부르고 pending/창을 읽지 않는다', () => {
    const state = { value: { ...createMissionStartState(), flags: 8, sequence: 3 } };
    const scene = simple(state);
    scene.guide.run();
    assert.deepEqual(scene.log, ['C', 'C', 'L']);
    // 두 번째 조회에서 이미 사라졌으면 release 하지 않는다.
    const gone = { value: { ...createMissionStartState(), flags: 9, sequence: 3 } };
    let reads = 0;
    const second = simple(gone, { current: () => { reads += 1; return reads === 1 ? gone.value : null; }, release: () => { throw new Error('호출되면 안 됨'); } });
    second.guide.run();
    assert.equal(reads, 2);
});

test('pending 이 비영이면 창 관문을 읽지 않고 postpone 만 실행한다', () => {
    const state = { value: { ...createMissionStartState(), flags: 1, sequence: 2 } };
    const scene = simple(state, { pending: () => { scene.log.push('P'); return 7; }, dialogsClear: () => { throw new Error('짧은 평가 위반'); } });
    scene.guide.run();
    // 단계는 그대로이므로 postpone 뒤 한 번 더 돌지 않는다.
    assert.deepEqual(scene.log, ['C', 'P', 'C', 'H', 'C']);
    assert.equal(state.value.sequence, 2);
    assert.equal(state.value.flags, 1);
});

test('창이 막혔으면 flags 비트 0 이 있어도 advance 하지 않고 postpone 한다', () => {
    const state = { value: { ...createMissionStartState(), flags: 1, sequence: 0 } };
    const scene = simple(state, { dialogsClear: () => { scene.log.push('D'); return false; } });
    scene.guide.run();
    assert.equal(state.value.sequence, 0);
    assert.deepEqual(scene.log, ['C', 'P', 'D', 'C', 'H', 'C']);
});

test('범위 밖 단계(7 이상)는 stage 대신 invalidStage 를 부르고 단계가 같으면 반복하지 않는다', () => {
    const state = { value: { ...createMissionStartState(), flags: 0, sequence: 7 } };
    const scene = simple(state);
    scene.guide.run();
    assert.deepEqual(scene.log, ['C', 'P', 'D', 'C', 'C', 'A', 'C']);
    // flags 비트 0 이 서 있으면 advance 로 8 이 되어도 마찬가지이고 6 은 정상 단계이다.
    const six = { value: { ...createMissionStartState(), flags: 0, sequence: 6 } };
    const sixScene = simple(six);
    sixScene.guide.run();
    assert.deepEqual(sixScene.log, ['C', 'P', 'D', 'C', 'C', 'C', 'S6', 'L', 'C']);
});

test('단계가 바뀌면 같은 Run 안에서 다시 방문하고 단계 6 에 닿으면 멈춘다', () => {
    const state = { value: { ...createMissionStartState(), flags: 0, sequence: 0 } };
    let stages = 0;
    const scene = simple(state, { stage: (target, sequence) => { stages += 1; scene.log.push(`S${sequence}`); if (sequence < 3) target.sequence = sequence + 1; } });
    scene.guide.run();
    // 0→1→2→3 으로 바뀌는 동안 반복하고 3 에서 변화가 없으면 멈춘다.
    assert.equal(stages, 4);
    assert.equal(state.value.sequence, 3);
});

test('가상 단계 대상의 현재 미션이 NULL 이면 손상 범위를 호스트 예외로 거부한다', () => {
    const state = { value: { ...createMissionStartState(), flags: 0, sequence: 1 } };
    let reads = 0;
    const scene = simple(state, { current: () => { reads += 1; return reads <= 2 ? state.value : null; } });
    assert.throws(() => scene.guide.run(), /현재 미션 NULL/u);
    // 단계 선택 뒤 가상 호출 대상 조회가 NULL 인 경우도 같은 진단이다.
    const target = { value: { ...createMissionStartState(), flags: 0, sequence: 1 } };
    let count = 0;
    const later = simple(target, { current: () => { count += 1; return count <= 3 ? target.value : null; } });
    assert.throws(() => later.guide.run(), /현재 미션 NULL/u);
});
