// 소형 일반 입력/메뉴 부모 여섯 개(CreateAt 입력·DetonateBomb 입력·MoveDialog·영상 SDK 타이머·인트로 메뉴·메인 메뉴 무활동 감시)를 원본 10.78 관측으로 대조한다(WB04-E33, cpppj CreateAtInputTests·DetonateInputTests·MoveDialogTests·MovieTimerTests·IntroMenuTests·MainMenuActivityTests 이식).
//  - create-at-input-x86.tsv 408행(+이름표 15행의 이름→번호), move-dialog-x86.tsv 73행(노드 float 비트·Push/Attach 순서), movie-timer-x86.tsv 14행, intro-menu-x86.tsv 84행, main-menu-activity-x86.tsv 1944행(double 비트·포인터·helper 호출 순서).
//  - DetonateBomb 의 768행 관측은 실제 객체 변환(RawObjectConversion) 몸체가 필요해 그 단위에서 대조하고, 여기서는 분배 계약만 검사한다.
// 실제 Client 설정 소유자·BaseFile 존재 조회 정책·대본 큐는 해당 단위를 옮길 때 대조한다. (2026-11-08 WB04-E33: 소형 입력 여섯 개 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, floatToBits, readRows } from './raw-support.mjs';
import { InputCode } from '../www/core/input-queue.js';
import { CREATE_AT_COMMAND, createAtScriptCode, dispatchCreateAtInput } from '../www/core/create-at-input.js';
import { DETONATE_BOMB_COMMAND, dispatchDetonateInput } from '../www/core/detonate-input.js';
import { moveDialog } from '../www/core/move-dialog.js';
import { waitMovieTimer } from '../www/core/movie-timer.js';
import { PLAY_INTRO_COMMAND, detectIntroMovie, dispatchPlayIntroInput } from '../www/core/intro-menu.js';
import { MAIN_MENU_ACTIVITY_EVENT, MAIN_MENU_DEMO_COMMAND, handleMainMenuActivity, idleMainMenuActivity } from '../www/core/main-menu-activity.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E33)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const createRows = fixture('create-at-input-x86.tsv');
const nameRows = fixture('create-at-names-x86.tsv');
const moveRows = fixture('move-dialog-x86.tsv');
const timerRows = fixture('movie-timer-x86.tsv');
const introRows = fixture('intro-menu-x86.tsv');
const activityRows = fixture('main-menu-activity-x86.tsv');

// 원문 DWORD 읽기(감김), signed 변환, 바이트 16진 복원, 8자리 16진이다. (2026-11-08 WB04-E33)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const signed = (value) => value | 0;
const bits = (value) => value >>> 0;
const hexToText = (hex) => (hex === '-' ? '' : Buffer.from(hex, 'hex').toString('latin1'));
const hex8 = (value) => (value >>> 0).toString(16).padStart(8, '0');
// 64비트 패턴과 double 사이 변환이다. (2026-11-08 WB04-E33)
const doubleOf = (text) => { const view = new DataView(new ArrayBuffer(8)); view.setBigUint64(0, BigInt(text), true); return view.getFloat64(0, true); };
const doubleBits = (value) => { const view = new DataView(new ArrayBuffer(8)); view.setFloat64(0, value, true); return view.getBigUint64(0, true).toString(); };

// Guide/Gump 소비는 원문처럼 일반 명령 분배보다 먼저 적용한다.
test('CreateAt 입력: 원본10.78 408행(두 인자·권한·정지/Console·상위 플래그)', { skip: !createRows && '원본 fixture 없음' }, () => {
    assert.equal(createRows.length, 408);
    for (const row of createRows) {
        assert.equal(row.length, 13);
        let trace = '';
        if (!number(row[5]) && !number(row[6])) {
            dispatchCreateAtInput({ code: number(row[0]), x: signed(number(row[1])), y: signed(number(row[2])) }, number(row[7]) !== 0, number(row[8]) !== 0, (token) => { trace = `createAt:${token}`; });
        }
        assert.equal(trace === '' ? '-' : trace, row[9], `${row[0]}/${row[7]}/${row[8]}`);
        assert.ok(row[11] === '1' && row[12] === '0');
    }
});

// 이름→번호 대응만 읽는다(문자열 고리와 큐 예약은 대본 큐 단위에서).
test('CreateAt 입력: 이름표 15행의 이름→번호와 첫 NUL', { skip: !nameRows && '원본 fixture 없음' }, () => {
    assert.equal(nameRows.length, 15);
    for (const row of nameRows) {
        assert.equal(row.length, 9);
        const name = hexToText(row[1]).split(',')[0];
        assert.equal(createAtScriptCode(name), row[6] === '-' ? 0xffffffff : number(row[6].split(',')[0]), name);
    }
    assert.equal(createAtScriptCode('cReAtEaT\0ignored'), CREATE_AT_COMMAND);
    assert.equal(createAtScriptCode('CreateAts'), 0xffffffff);
});

// 미도달 부모는 요구하지 않고 선택된 실제 생성 실패를 정상 성공으로 바꾸지 않는다.
test('CreateAt 입력: 도달한 부모만 요구하고 실패는 전파한다', () => {
    assert.equal(dispatchCreateAtInput({ code: 0x43, x: 0, y: 0 }, true, true, undefined), false);
    assert.equal(dispatchCreateAtInput({ code: (CREATE_AT_COMMAND | InputCode.release) >>> 0, x: 0, y: 0 }, true, true, undefined), true);
    assert.equal(dispatchCreateAtInput({ code: CREATE_AT_COMMAND, x: 0, y: 0 }, false, false, undefined), true);
    assert.throws(() => dispatchCreateAtInput({ code: CREATE_AT_COMMAND, x: 0, y: 0 }, true, false, undefined), /미연결/);
    let calls = 0;
    let token = 0;
    assert.throws(() => dispatchCreateAtInput({ code: CREATE_AT_COMMAND, x: 17, y: -1 }, false, true, (current) => { calls += 1; token = current; throw new Error('명시 실제 생성 부모 실패'); }), /명시/);
    assert.deepEqual([calls, token], [1, 0xffffffff]);
});

// DetonateBomb 분배 계약이다: 뗌/0번 대상은 하위를 요구하지 않고 반환은 무시한다.
test('DetonateBomb 입력: 뗌·낮은 WORD 0·선택된 가상 6c', () => {
    assert.equal(dispatchDetonateInput({ code: 0x02000017, x: 0, y: 50 }, undefined), false);
    assert.equal(dispatchDetonateInput({ code: (DETONATE_BOMB_COMMAND | InputCode.release) >>> 0, x: 0, y: 50 }, undefined), true);
    assert.equal(dispatchDetonateInput({ code: DETONATE_BOMB_COMMAND, x: 0, y: 0 }, undefined), true);
    assert.equal(dispatchDetonateInput({ code: DETONATE_BOMB_COMMAND, x: 0, y: 65536 }, undefined), true);
    assert.throws(() => dispatchDetonateInput({ code: DETONATE_BOMB_COMMAND, x: 0, y: 50 }, undefined), /미연결/);
    let called = 0;
    assert.equal(dispatchDetonateInput({ code: DETONATE_BOMB_COMMAND, x: 999, y: 65586 }, (sid, code) => { assert.deepEqual([sid, code], [50, DETONATE_BOMB_COMMAND]); called += 1; return 1; }), true);
    assert.equal(called, 1);
});

// 문자열·화면·창 치수·현재 포인터 변경을 독립 원문과 대조한다.
test('MoveDialog: 원본10.78 73행(b/t/r/l 순서·float 저장·Push 뒤 화면 크기·Attach 순서)', { skip: !moveRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of moveRows) {
        assert.equal(row.length, 4);
        const values = row[1].split(',').map(number);
        const expected = Buffer.from(row[2], 'hex');
        assert.equal(values.length, 8);
        assert.equal(expected.length, 0xa0);
        let width = signed(values[1]);
        let height = signed(values[2]);
        const node = { width: bitsToFloat(values[3]), height: bitsToFloat(values[4]), x: bitsToFloat(values[5]), y: bitsToFloat(values[6]), flags: 0x71717171, depth: 0x7171 };
        const other = { x: 999 };
        let events = '';
        const hooks = {
            push: () => { events = 'P'; if (values[7]) { width = 1920; height = 1080; node.width = 123.5; node.height = 222.25; node.x = -10.5; node.y = 12.25; } },
            attach: (x, y, parent, mode) => { events += `;A:${hex8(floatToBits(x))},${hex8(floatToBits(y))},${hex8(parent)},${hex8(mode)}`; },
        };
        moveDialog(values[0] ? node : null, hexToText(row[0]), () => width, () => height, hooks);
        const label = `행 ${count + 1} ${row[0]}`;
        assert.deepEqual([floatToBits(node.width), floatToBits(node.height)], [expected.readUInt32LE(0x3c), expected.readUInt32LE(0x40)], `${label} 크기`);
        assert.deepEqual([floatToBits(node.x), floatToBits(node.y)], [expected.readUInt32LE(0x4c), expected.readUInt32LE(0x50)], `${label} 위치`);
        assert.deepEqual([node.flags, node.depth, other.x], [0x71717171, 0x7171, 999]);
        assert.equal(events === '' ? '-' : events, row[3], `${label} 호출 순서`);
        count += 1;
    }
    assert.equal(count, 73);
});

// 장치 요구 계약이다: 노드가 없으면 Push/Attach 를 요구하지 않는다.
test('MoveDialog: 노드 없음은 장치 비요구, 장치 누락은 거부', () => {
    moveDialog(null, 'b', () => 0, () => 0, {});
    assert.throws(() => moveDialog({ x: 0, y: 0, width: 1, height: 1 }, 'b', () => 0, () => 0, { push: () => {} }), TypeError);
    // b 보다 t, r 보다 l 이 우선하고 m/c 는 좌표를 바꾸지 않는다.
    const node = { x: 5, y: 6, width: 100, height: 50 };
    moveDialog(node, 'btrl', () => 640, () => 480, { push: () => {}, attach: () => {} });
    assert.deepEqual([node.x, node.y], [0, 0]);
    node.x = 5; node.y = 6;
    moveDialog(node, 'mc\0b', () => 640, () => 480, { push: () => {}, attach: () => {} });
    assert.deepEqual([node.x, node.y], [5, 6]);
    moveDialog(node, 'br', () => 640, () => 480, { push: () => {}, attach: () => {} });
    assert.deepEqual([node.x, node.y], [540, 430]);
});

// 최초 기한·대기·late 재설정·ready/fast·간격 0·DWORD 감김을 실제 SDK 관측과 대조한다.
test('MovieTimer: 원본10.78 SDK 14행(최초 기한·대기·late 재설정·ready/fast·간격 0·감김)', { skip: !timerRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of timerRows) {
        assert.equal(row.length, 9);
        const state = { fast: number(row[0]), ready: number(row[1]), due: number(row[2]), interval: number(row[3]) };
        let trace = '';
        const hooks = {
            milliseconds: () => { trace = 'time'; return number(row[4]); },
            yield: () => { trace += trace ? ';yield' : 'yield'; },
        };
        const result = waitMovieTimer(state, hooks);
        const label = `행 ${count + 1}`;
        assert.equal(Number(result), number(row[5]), `${label} 반환`);
        assert.deepEqual([state.ready, state.due], [number(row[6]), number(row[7])], `${label} ready/due`);
        assert.equal(trace === '' ? '-' : trace, row[8], `${label} 장치 순서`);
        count += 1;
    }
    assert.equal(count, 14);
    // fast/ready 는 시계를 읽지 않는다.
    assert.equal(waitMovieTimer({ fast: 1, ready: 0, due: 0xffffffff, interval: 1 }, {}), false);
    assert.equal(waitMovieTimer({ fast: 0, ready: 1, due: 0xffffffff, interval: 1 }, {}), false);
    assert.throws(() => waitMovieTimer({ fast: 0, ready: 0, due: 0, interval: 1 }, {}), /미연결/);
});

// 원문 00435de0 의 독립 CPU 관측 84행을 같은 명시 파일 장치로 재생해 전역/반환/조회 횟수를 대조한다.
test('IntroMenu: 원본10.78 감지 84행(선행 0·빈 경로·첫 NUL·출처 정규화)', { skip: !introRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of introRows) {
        assert.equal(row.length, 6);
        const holder = { value: number(row[2]) };
        let calls = 0;
        const result = detectIntroMovie(holder, () => hexToText(row[0]), () => { calls += 1; return number(row[1]); });
        const label = `행 ${count + 1} ${row[0]}`;
        assert.deepEqual([result, holder.value, calls], [number(row[3]), number(row[4]), number(row[5])], label);
        count += 1;
    }
    assert.equal(count, 84);
});

// 빈 경로·첫 NUL·각 출처·예외의 선행 0 과 비영 성공의 1 정규화를 원문 호출 순서대로 확인한다.
test('IntroMenu: 감지 계약과 PlayIntro 입력의 State 예약', () => {
    const holder = { value: 7 };
    assert.equal(detectIntroMovie(holder, () => '', undefined), 0);
    assert.equal(holder.value, 0);
    for (const source of [0, 1, 2, 3, 4, 5, 0xffffffff]) {
        holder.value = 77;
        let calls = 0;
        const result = detectIntroMovie(holder, () => { assert.equal(holder.value, 0); return 'intro.smk\0ignored'; }, (probe) => { assert.equal(probe, 'intro.smk'); calls += 1; return source; });
        assert.deepEqual([result, holder.value, calls], [source ? 1 : 0, source ? 1 : 0, 1]);
    }
    holder.value = 9;
    assert.throws(() => detectIntroMovie(holder, () => { throw new Error('path'); }, undefined), /path/);
    assert.equal(holder.value, 0);
    holder.value = 9;
    assert.throws(() => detectIntroMovie(holder, () => 'intro.smk', () => { throw new Error('probe'); }), /probe/);
    assert.equal(holder.value, 0);
    // release/다른 명령은 State 를 그대로 두고 수정키는 명령 실행을 막지 않는다.
    for (const code of [PLAY_INTRO_COMMAND, (PLAY_INTRO_COMMAND | InputCode.release) >>> 0, (PLAY_INTRO_COMMAND | InputCode.control) >>> 0, 0x02000001]) {
        const next = new Uint32Array(36).fill(0x12345678);
        const before = Uint32Array.from(next);
        const selected = ((code & 0x03ffffff) >>> 0) === PLAY_INTRO_COMMAND;
        assert.equal(dispatchPlayIntroInput({ code, x: -7, y: 99 }, { next }), selected);
        if (selected && (code & InputCode.release) === 0) { before[0] = 39; before[35] = 1; }
        assert.deepEqual(Array.from(next), Array.from(before));
    }
});

// 독립 CPU 기대값 1944행의 double 비트·포인터·helper 호출 순서 전체를 재생한다.
test('MainMenuActivity: 원본10.78 1944행(double 비트·포인터·helper 호출 순서)', { skip: !activityRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of activityRows) {
        assert.equal(row.length, 13);
        const state = { deadline: doubleOf(row[6]), pointer: { x: 0, y: 0 } };
        const pointer = { x: signed(number(row[3])), y: signed(number(row[4])) };
        let trace = '';
        let calls = 0;
        const hooks = {
            popup: () => { trace += 'p'; return number(row[2]) !== 0; },
            now: () => { trace += 'n'; const value = doubleOf(calls === 0 ? row[7] : row[8]); calls += 1; return value; },
            queue: (command, first, second) => { assert.deepEqual([command, first, second], [MAIN_MENU_DEMO_COMMAND, 'DemoStormRages', 'DemoStormRages']); trace += 'q'; },
            kill: () => { trace += 'k'; },
        };
        idleMainMenuActivity(state, () => number(row[0]), () => signed(number(row[1])), () => pointer, () => number(row[5]), 1, hooks);
        const label = `행 ${count + 1}`;
        assert.equal(doubleBits(state.deadline), row[9], `${label} 기한`);
        assert.deepEqual([state.pointer.x, state.pointer.y], [signed(number(row[10])), signed(number(row[11]))], `${label} 포인터`);
        assert.equal(trace === '' ? '-' : trace, row[12], `${label} helper 순서`);
        count += 1;
    }
    assert.equal(count, 1944);
});

// 상태 쓰기 뒤 helper 실패, 현재 모달 재조회, 동일 프레임 시계 도약과 활동 사건 release 를 확인한다.
test('MainMenuActivity: 지연 현재 조회와 부분 효과', () => {
    const state = { deadline: 4, pointer: { x: 7, y: 8 } };
    let enabled = 0;
    let modal = 1;
    let html = 0;
    let pointer = { x: 9, y: 10 };
    idleMainMenuActivity(state, () => enabled, () => html, () => pointer, () => modal, 1, {});
    assert.deepEqual([state.deadline, state.pointer.x], [4, 7]);
    enabled = 1;
    html = 1;
    let calls = 0;
    const hooks = { now: () => { calls += 1; modal = 2; pointer = { x: 11, y: 12 }; return 5; } };
    idleMainMenuActivity(state, () => enabled, () => html, () => pointer, () => modal, 1, hooks);
    // HTML 양수는 팝업을 생략하고 now 가 바꾼 현재 모달/포인터를 뒤에서 읽는다.
    assert.deepEqual([calls, state.deadline, state.pointer.x, state.pointer.y], [1, 50, 11, 12]);
    html = 0;
    modal = 1;
    state.deadline = 0;
    state.pointer = { x: 11, y: 12 };
    let trace = '';
    Object.assign(hooks, { popup: () => { trace += 'p'; return false; }, now: () => { trace += 'n'; return 50; }, queue: () => { trace += 'q'; }, kill: undefined });
    // 실제 queue 가 정상 복귀한 선행 효과를 Kill 장치 누락이 되돌리지 않는다.
    assert.throws(() => idleMainMenuActivity(state, () => enabled, () => html, () => pointer, () => modal, 1, hooks), /Kill/);
    assert.deepEqual([trace, state.deadline], ['pnq', 0]);
    assert.equal(handleMainMenuActivity(state, (MAIN_MENU_ACTIVITY_EVENT | InputCode.release) >>> 0, () => 10), true);
    assert.deepEqual([state.deadline, state.pointer.x], [55, 11]);
    assert.equal(handleMainMenuActivity(state, 0, undefined), false);
});
