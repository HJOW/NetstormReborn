// Guide 미션 교체 guide-mission-transitions.js(Begin/Close/Fork/Restart/End/명령 분배)를 원본 10.78 관측 286행과 cpppj 계약으로 대조한다(WB04-E523).
//  - 원본 관측 guide-transitions-x86.tsv 286행: `T` 256행은 연산(Begin·Fork·Restart·명령 Begin/Fork/Reload/End)×현재/모드/데모/재진입/인자/이름/플래그/x 입력에서 반환·현재 식별자·이름 바이트·mode,demo·
//    효과 순서(F finish·L release·C create·G get·A 잘못된 Begin 진단)를 대조하고, `S` 30행은 제어 이름/인자가 원본 토큰·문자열 고리·큐에 예약되는 결과(사건 코드·x==y·문자열/숫자 인자·고리 번호)를 대조한다.
//  - 계약: 이름은 NUL 앞까지만 소유 복사, Fork 키는 signed `%d`, 선택되지 않은 효과는 도달한 위치에서 거부(빈 성공 없음), 지원하지 않는 명령은 false.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E523)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { InputQueue } from '../www/core/input-queue.js';
import { ScriptStringRing } from '../www/ui/script-command.js';
import {
    MISSION_BEGIN_COMMAND, MISSION_END_COMMAND, MISSION_FORK_COMMAND, MISSION_RELOAD_COMMAND, captureMissionReload, queueMissionControlAction,
} from '../www/core/mission-guide-input.js';
import {
    beginGuideMission, closeGuideMission, dispatchGuideMissionCommand, endGuideMission, forkGuideMission, restartGuideMission,
} from '../www/core/guide-mission-transitions.js';

// 원본 관측 286행이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E523)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'guide-transitions-x86.tsv'));

// 원본 바이트를 문자 하나로 담은 문자열의 소문자 16진이다. (2026-11-15 WB04-E523)
const hex = (text) => Buffer.from(text, 'latin1').toString('hex');
// 16진 문자열을 바이트 그대로의 문자열로 복원한다(UTF-8 치환으로 기대값을 바꾸지 않는다). (2026-11-15 WB04-E523)
const fromHex = (text) => Buffer.from(text, 'hex').toString('latin1');

// 독립 원문 입력과 같은 현재/재진입/이름/필드 및 명시 수명 경계이다(cpppj TransitionScene). (2026-11-15 WB04-E523)
class TransitionScene {
    // fields 는 연산·현재·모드·데모·재진입·인자·이름·플래그·x 아홉 DWORD 이다.
    constructor(fields) {
        this.input = fields.split(',').map(Number);
        assert.equal(this.input.length, 9);
        this.active = this.input[1];
        this.reentrant = this.input[4];
        this.mode = this.input[2];
        this.demo = this.input[3];
        this.guideName = 'TheWarBegins';
        this.beginName = this.input[6] ? 'MasterOfWhirligigs' : 'Tutorial2';
        this.target = this.beginName;
        this.trace = '';
    }

    // 실제 효과 한 번의 순서를 기록한다.
    add(value) { this.trace = this.trace ? `${this.trace};${value}` : value; }

    // 원본 명시 경계와 같은 해제 재진입/설정 무효화/새 공장 필드를 공급한다.
    hooks() {
        return {
            current: () => this.active !== 0,
            finish: () => { this.add(`F,${this.active}`); },
            release: () => {
                this.add(`L,${this.active}`);
                this.active = 0;
                this.guideName = '';
                this.target = 'invalidated';
                if (this.reentrant) {
                    this.active = 2;
                    this.reentrant = 0;
                }
            },
            create: (name) => {
                this.add(`C,${hex(name)}`);
                this.guideName = name;
                this.active = 3;
                this.mode = 0;
                this.demo = 0;
            },
            get: (key) => { this.add(`G,${hex(key)}`); return this.target; },
            mode: (value) => { assert.equal(this.active, 3); this.mode = value; },
            demo: (value) => { assert.equal(this.active, 3); this.demo = value; },
        };
    }

    // 실제 제품 분배/교체 함수를 호출하고 원문의 최종 이름/현재/필드/순서를 직렬화한다.
    run() {
        const hooks = this.hooks();
        let result = 'void';
        const input = this.input;
        // 원본 첫 해제 전에 같은 입력 필드를 캡처한다.
        const restart = () => { restartGuideMission(captureMissionReload(this.guideName, input[2], input[3]), hooks); };
        if (input[0] === 0) beginGuideMission(this.beginName, hooks);
        else if (input[0] === 1) forkGuideMission(input[5], hooks);
        else if (input[0] === 2) restart();
        else {
            const codes = [MISSION_BEGIN_COMMAND, MISSION_FORK_COMMAND, MISSION_RELOAD_COMMAND, MISSION_END_COMMAND];
            const commands = {
                begin: (parameter) => { assert.equal(parameter, input[5]); beginGuideMission(this.beginName, hooks); },
                fork: (parameter) => { forkGuideMission(parameter, hooks); },
                restart,
                end: () => { endGuideMission(hooks); },
                invalidBegin: (parameter) => { assert.equal(parameter, input[5]); this.add('A'); },
            };
            const handled = dispatchGuideMissionCommand({ code: (codes[input[0] - 3] | input[7]) >>> 0, x: input[8] | 0, y: input[5] | 0 }, commands);
            result = handled ? '1' : '0';
        }
        return `${result}|${this.active}|${hex(this.guideName)}|${this.mode},${this.demo}|${this.trace === '' ? '-' : this.trace}`;
    }
}

test('Guide 교체: 원본 10.78 286입력(교체 256·이름/인자/고리 30)이 같은 결과·효과 순서·문자열 고리를 낸다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(rows.length, 286);
    let transitions = 0;
    let rings = 0;
    // 원문 기대값을 제품의 분기로 다시 계산하지 않는다.
    for (const row of rows) {
        if (row[0] === 'T') {
            assert.equal(row.length, 3);
            assert.equal(new TransitionScene(row[1]).run(), row[2], `T ${row[1]}`);
            transitions += 1;
        } else {
            assert.ok(row[0] === 'S' && row.length === 4);
            const raw = fromHex(row[1]);
            const comma = raw.indexOf(',');
            const queue = new InputQueue();
            const strings = new ScriptStringRing();
            assert.equal(queueMissionControlAction(raw.slice(0, comma), raw.slice(comma + 1), queue, strings), true);
            const event = queue.pop(false);
            const expected = row[2].split(',');
            assert.equal(expected.length, 2);
            assert.equal(event.code >>> 0, Number(expected[0]));
            assert.equal(event.x, event.y);
            assert.ok(queue.empty);
            assert.equal(strings.index, Number(row[3]));
            if (expected[1][0] === 's') {
                assert.ok(event.x > 100);
                assert.equal(strings.resolve(event.x >>> 0), fromHex(expected[1].slice(1)));
            } else assert.equal(event.x >>> 0, Number(expected[1].slice(1)));
            rings += 1;
        }
    }
    assert.equal(transitions, 256);
    assert.equal(rings, 30);
});

test('Guide 교체 계약: 이름은 NUL 앞까지 복사하고 Fork 키는 signed %d 이며 Close/End 는 공장을 호출하지 않는다', () => {
    const calls = [];
    let active = true;
    const hooks = {
        current: () => active,
        finish: () => { calls.push('finish'); },
        release: () => { calls.push('release'); active = false; },
        create: (name) => { calls.push(`create:${name}`); active = true; },
        get: (key) => { calls.push(`get:${key}`); return 'Next\0hidden'; },
        mode: () => {},
        demo: () => {},
    };
    beginGuideMission('A\0B', hooks);
    assert.deepEqual(calls, ['finish', 'release', 'create:A']);
    calls.length = 0;
    forkGuideMission(0xffffffff, hooks);
    assert.deepEqual(calls, ['get:fork-1', 'finish', 'release', 'create:Next']);
    calls.length = 0;
    // 현재가 없으면 Close 는 해제를 요구하지 않는다.
    active = false;
    closeGuideMission(hooks);
    assert.deepEqual(calls, []);
    active = true;
    closeGuideMission(hooks);
    assert.deepEqual(calls, ['release']);
    calls.length = 0;
    active = true;
    endGuideMission(hooks);
    assert.deepEqual(calls, ['finish', 'release']);
});

test('Guide 교체 계약: 지원하지 않는 명령은 false 이고 빠진 수명 효과·분배 효과는 도달한 위치에서 거부한다', () => {
    assert.equal(dispatchGuideMissionCommand({ code: 0x1234, x: 0, y: 500 }, {}), false);
    // 상위 플래그는 지우고 비교한다.
    const seen = [];
    assert.equal(dispatchGuideMissionCommand({ code: (MISSION_END_COMMAND | 0x08000000) >>> 0, x: 1, y: 2 }, { end: () => { seen.push('end'); } }), true);
    assert.deepEqual(seen, ['end']);
    // Begin 의 세 번째 값이 101 이상이면 진단 없이 Begin 만 호출하며, 100 이하이면 진단이 먼저이다.
    const order = [];
    const begin = { begin: (value) => { order.push(`begin:${value}`); }, invalidBegin: (value) => { order.push(`invalid:${value}`); } };
    dispatchGuideMissionCommand({ code: MISSION_BEGIN_COMMAND, x: 0, y: 101 }, begin);
    dispatchGuideMissionCommand({ code: MISSION_BEGIN_COMMAND, x: 0, y: 100 }, begin);
    dispatchGuideMissionCommand({ code: MISSION_BEGIN_COMMAND, x: 0, y: -1 }, begin);
    assert.deepEqual(order, ['begin:101', 'invalid:100', 'begin:100', 'invalid:4294967295', 'begin:4294967295']);
    assert.throws(() => beginGuideMission('x', { current: () => false }), /수명 효과 연결 누락/);
    assert.throws(() => dispatchGuideMissionCommand({ code: MISSION_BEGIN_COMMAND, x: 0, y: 5 }, { begin: () => {} }), /수명 효과 연결 누락/);
});
