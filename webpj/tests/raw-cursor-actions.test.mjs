// 커서 잠금/해제/회전 raw-cursor-actions.js(RawCursorActions.lock·unlock·rotate)를 원본 10.78 관측 289행과 cpppj 시험 계약으로 대조한다(WB04-E547).
//  - cursor-actions-x86.tsv 289행: 세 부모(0 Lock·1 Unlock·2 Rotate)의 14 DWORD 독립 입력(깊이·인자·방향·Gump·TYPE 그룹·선택 타입·특별 타입·재진입 변경)에서 외부 효과 호출 순서와 전체 전역.
//  - 계약: 미연결 경계 거부·TYPE 표 크기 거부·읽기 함수/상자 두 가지 전역 연결·DWORD 감김.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E547)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { CursorDropState } from '../www/core/raw-cursor-drop.js';
import { CursorLifecycleState } from '../www/core/raw-cursor-lifecycle.js';
import { RawCursorActions } from '../www/core/raw-cursor-actions.js';

// 원본 관측 fixture 이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E547)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'cursor-actions-x86.tsv'));
// fixture 14개 독립 입력 열이다. (2026-11-15 WB04-E547)
const Column = Object.freeze({ Operation: 0, Depth: 1, Argument: 2, Direction: 3, Gump: 4, Group: 5, Selected: 6, Special: 7, Activate: 8, Create: 9, Sound: 10, Remove: 11, Select: 12, Range: 13 });
// 원본 DWORD 목록만 읽으며 기대 출력을 호스트 수식으로 생성하지 않는다. (2026-11-15 WB04-E547)
const numbers = (value) => value.split(',').map((field) => Number(field) >>> 0);

// 원본 실행기와 같은 명시 효과 입력만 갖는 현재 커서 장면이다. (2026-11-15 WB04-E547)
class ActionScene {
    constructor() {
        this.types = Array.from({ length: 188 }, () => ({ group: 0 }));
        this.drop = new CursorDropState();
        this.life = new CursorLifecycleState();
        this.special = 0;
        this.rangeType = 51;
        this.input = [];
        this.trace = '';
    }

    // 각 경계의 전체 DWORD 를 원문과 같은 문자열에 기록한다(저장된 DWORD 십진수 유지).
    event(letter, values = []) { this.trace = `${this.trace}${this.trace ? ';' : ''}${letter}${values.map((value) => `:${value >>> 0}`).join('')}`; }

    // 이전 관측 결과를 다음 독립 입력에 넘기지 않는다.
    prepare(values) {
        const input = values;
        assert.equal(input.length, 14);
        this.input = input;
        this.drop = new CursorDropState();
        this.life = new CursorLifecycleState();
        this.trace = '';
        this.types[100].group = input[Column.Group] | 0;
        this.types[101].group = 1;
        this.drop.lockDepth = input[Column.Depth];
        this.drop.direction = input[Column.Direction];
        this.drop.legalGump = input[Column.Gump];
        this.drop.type = input[Column.Selected];
        this.drop.cursorGump = 2;
        this.life.abstractType = 100;
        this.life.previews[0] = 100;
        this.special = input[Column.Special];
    }

    // 재진입 변경만 같은 실제 저장소에 적용하고 부모 계산은 모듈에 맡긴다.
    hooks() {
        const input = this.input;
        return {
            activate: (force) => {
                this.event('A', [force]);
                if (input[Column.Activate] === 1) this.drop.lockDepth = 3;
                if (input[Column.Activate] === 2) this.drop.direction = 0xfffffff9;
                if (input[Column.Activate] === 3) { this.life.abstractType = 101; this.drop.type = this.special; }
            },
            createPreviews: () => {
                this.event('B');
                if (input[Column.Create] === 1) this.drop.legalGump = 9;
                if (input[Column.Create] === 2) this.drop.lockDepth = 5;
                if (input[Column.Create] === 3) { this.life.previews[0] = 101; this.types[100].group = 1; }
            },
            selectCursor: (cursor) => { this.event('K', [cursor]); if (input[Column.Select]) this.life.abstractType = 101; },
            sound: (text) => {
                assert.equal(text, 'rotatepiece.wav');
                this.event('S:rotatepiece.wav', [0, 0, 0, 0, 0]);
                if (input[Column.Sound] === 1) this.drop.lockDepth = 1;
                if (input[Column.Sound] === 2) this.life.abstractType = 101;
            },
            removeRange: (parent, type) => {
                this.event('D', [parent, type, 0, 1]);
                if (input[Column.Remove] === 1) this.life.previews[0] = 102;
                if (input[Column.Remove] === 2) { this.life.abstractType = 100; this.types[100].group = 2; }
            },
            createRange: (parent, form) => { this.event('G', [parent, form]); return input[Column.Range] ? 1 : 0; },
        };
    }

    // 관측된 전역과 같은 순서로 현재 DWORD 를 읽는다.
    values() {
        return [this.drop.lockDepth, this.drop.direction, this.drop.legalGump, this.drop.type, this.life.previews[0], this.drop.cursorGump, this.life.abstractType, this.types[100].group >>> 0, this.types[101].group >>> 0].map((value) => value >>> 0);
    }
}

test('커서 잠금/해제/회전: 원본 10.78 289입력의 외부 효과 순서·전체 전역이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new ActionScene();
    let count = 0;
    // 원본 전체 고유 입력 289행을 독립 초기화하여 비교한다.
    for (const fields of rows) {
        assert.equal(fields.length, 3);
        scene.prepare(numbers(fields[0]));
        const actions = new RawCursorActions(scene.types, scene.drop, scene.life, { get value() { return scene.special; } }, { get value() { return scene.rangeType; } }, scene.hooks());
        const input = scene.input;
        if (input[Column.Operation] === 0) actions.lock(input[Column.Argument]);
        else if (input[Column.Operation] === 1) actions.unlock();
        else actions.rotate(input[Column.Argument] | 0);
        const label = `행 ${count}: ${fields[0]}`;
        assert.equal(scene.trace || '-', fields[1], label);
        assert.deepEqual(scene.values(), numbers(fields[2]), label);
        count += 1;
    }
    assert.equal(count, 289);
});

test('커서 동작 계약: TYPE 표 크기·미연결 경계 거부와 함수형 전역·DWORD 감김', () => {
    const scene = new ActionScene();
    scene.prepare(numbers('0,0,0,0,0,0,100,140,0,0,0,0,0,1'));
    const make = (hooks, special = () => 0, range = () => 51) => new RawCursorActions(scene.types, scene.drop, scene.life, special, range, hooks);
    assert.throws(() => new RawCursorActions(scene.types.slice(1), scene.drop, scene.life, { value: 0 }, { value: 51 }, {}), TypeError);
    // 선택한 경계가 없으면 성공으로 대체하지 않는다(Activate 는 처음 잠글 때만 필요하다).
    assert.throws(() => make({}).lock(0), /미연결/);
    scene.drop.lockDepth = 1;
    make({}).lock(0);
    assert.equal(scene.drop.lockDepth, 2);
    // 해제는 DWORD 로 감기며 0 이 아니면 아무 효과도 부르지 않는다. 0 에서 내려가면 0xffffffff 가 된다.
    scene.drop.lockDepth = 0;
    make({}).unlock();
    assert.equal(scene.drop.lockDepth, 0xffffffff);
    scene.drop.lockDepth = 1;
    assert.throws(() => make({}).unlock(), /미연결/);
    // 함수형 전역(special/range)을 읽고 회전은 방향을 8 로 접는다.
    const calls = [];
    scene.drop.lockDepth = 1;
    scene.drop.direction = 6;
    scene.life.abstractType = 100;
    scene.types[100].group = 1;
    scene.life.previews[0] = 100;
    make({ sound: () => calls.push('sound'), removeRange: (parent, type) => calls.push(['remove', parent, type]), createRange: (parent, form) => { calls.push(['create', parent, form]); return 77; } }, () => 5, () => 99).rotate(5);
    assert.equal(scene.drop.direction, 3);
    assert.equal(scene.drop.cursorGump, 77);
    assert.deepEqual(calls, ['sound', ['remove', 100, 99], ['create', 100, 0]]);
});

test('커서 회전 계약: 현재 논리 TYPE 번호가 TYPE 표 크기(188) 이상이면 호스트 범위 오류로 거부한다', () => {
    const scene = new ActionScene();
    scene.prepare(numbers('0,0,0,0,0,0,100,140,0,0,0,0,0,1'));
    scene.life.abstractType = 188;
    const hooks = { activate: () => {}, selectCursor: () => {}, createPreviews: () => {}, sound: () => {}, removeRange: () => {}, createRange: () => 0 };
    const actions = new RawCursorActions(scene.types, scene.drop, scene.life, { value: 0 }, { value: 51 }, hooks);
    assert.throws(() => actions.rotate(1), RangeError);
    scene.life.abstractType = 187;
    assert.doesNotThrow(() => actions.rotate(1));
});
