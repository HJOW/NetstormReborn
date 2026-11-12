// 스크립트 명령 큐 넣기(queueScriptCommand, 원본 004c9090)와 80칸 문자열 고리(ScriptStringRing, 004c7910)를 원본 10.78 관측과 cpppj 시험, 웹 계약으로 확인한다.
//  - 원본 관측: script-command-x86.tsv 262행 중 Command 행(명령 이름/인자 토큰·표 조회 시점·코드 0 진단 뒤 계속·0xffffffff 진단 뒤 거부·큐 사건 코드/인자·고리 위치·진단 시점)과 Ring 행(연속 store/resolve, 81번째 복사부터 이전 수명만 무효)을
//    대조한다. Token/Config 행은 기존 시험(startup·data-manager-names)과 raw-text-token 시험이 같은 관측을 읽는다.
//  - cpppj 시험 이식: ScriptCommandTests 의 IndependentPe262 중 Command/Ring 분기와 ConfigOverwriteAndQueueLifetime 중 큐 수명 부분(Config 명령은 큐에 쌓일 뿐 적용 전에는 설정이 바뀌지 않음, 해제된 토큰 거부).
//  - 웹 계약: 표 조회 미연결 거부, 진단 미연결 거부, NUL 절단, 같은 문자열도 새 칸, 숫자 인자의 32비트 감김(음수·큰 값), 큐 용량(39개 초과는 새 사건 폐기).
// 실제 게임·브라우저 실행이 아니다. (2026-11-12 WB04-E343)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { InputQueue } from '../www/core/input-queue.js';
import { SCRIPT_CONFIG, SCRIPT_WARNING, ScriptStringRing, queueScriptCommand } from '../www/ui/script-command.js';

// 원본 관측 행이다. 파일이 없으면 재생 검사를 건너뛴다. 첫 줄(열 이름)은 `operation` 으로 시작해 `#` 이 아니므로 직접 거른다. (2026-11-12 WB04-E343)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'script-command-x86.tsv'))?.filter((row) => row[0] !== 'operation') ?? null;

// 16진 문자열을 한 글자 한 바이트의 원문으로 복원한다(`-` 는 빈 값). (2026-11-12 WB04-E343)
function bytesOf(encoded) {
    if (encoded === '-') return '';
    let result = '';
    // 두 16진 글자를 한 바이트로 돌린다.
    for (let index = 0; index < encoded.length; index += 2) result += String.fromCharCode(Number.parseInt(encoded.slice(index, index + 2), 16));
    return result;
}

test('원본 관측 262행 중 Command 행: 큐 사건·고리 위치·진단 시점이 일치한다', { skip: ROWS === null }, () => {
    let count = 0;
    // Command 행마다 새 큐/고리를 쓴다.
    for (const row of ROWS.filter((candidate) => candidate[0] === 'Command')) {
        assert.equal(row.length, 9);
        const source = bytesOf(row[1]);
        const queue = new InputQueue();
        const strings = new ScriptStringRing();
        const diagnostics = [];
        const hooks = {
            lookup(name) {
                assert.equal(strings.index, 0);
                const lower = name.toLowerCase();
                return lower === 'config' ? (row[3] === '1' ? 0 : SCRIPT_CONFIG) : lower === 'warning' ? SCRIPT_WARNING : 0xffffffff;
            },
            unknown() { diagnostics.push(strings.index); },
        };
        queueScriptCommand(source, queue, strings, hooks);
        assert.equal(strings.index, Number(row[7]));
        if (row[6] === '-') assert.equal(queue.empty, true);
        else {
            assert.equal(queue.empty, false);
            const actual = queue.pop(false);
            const split = row[6].indexOf(',');
            assert.equal(actual.code, Number(row[6].slice(0, split)));
            assert.equal(actual.x, actual.y);
            const expected = row[6].slice(split + 1);
            if (expected[0] === 's') assert.equal(strings.resolve(actual.x >>> 0), bytesOf(expected.slice(1)));
            else assert.equal(actual.x >>> 0, Number(expected.slice(1)));
            assert.equal(queue.empty, true);
        }
        const logCount = [...row[8]].filter((ch) => ch === 'L').length;
        assert.equal(diagnostics.length, logCount);
        // 첫 진단 시점: 코드 0 은 인자 처리 전(고리 위치 0), 0xffffffff 는 인자 처리 뒤의 현재 위치이다.
        if (diagnostics.length > 0) assert.equal(diagnostics[0], row[3] === '1' ? 0 : strings.index);
        count += 1;
    }
    assert.ok(count > 0);
});

test('원본 관측 262행 중 Ring 행: 연속 store/resolve 와 81번째 복사부터의 수명 무효화', { skip: ROWS === null }, () => {
    const continuous = new ScriptStringRing();
    const tokens = [];
    const stored = [];
    let count = 0;
    // Ring 행은 한 고리에 이어서 쌓인다.
    for (const row of ROWS.filter((candidate) => candidate[0] === 'Ring')) {
        const source = bytesOf(row[1]);
        const token = continuous.store(source);
        tokens.push(token);
        stored.push(source);
        assert.equal(continuous.resolve(token), bytesOf(row[4]));
        assert.equal(continuous.index, Number(row[7]));
        // 기존 80수명이 보존되고 81번째 복사부터 정확히 이전 수명만 무효화되는지 확인한다.
        for (let index = 0; index < tokens.length; index += 1) {
            let rejected = false;
            try { assert.equal(continuous.resolve(tokens[index]), stored[index]); } catch (error) {
                if (!(error instanceof RangeError)) throw error;
                rejected = true;
            }
            assert.equal(rejected, tokens.length - index > 80);
        }
        count += 1;
    }
    assert.ok(count > 0);
    // 행 수: Token + Config + Command + Ring = 262 (헤더 제외).
    assert.equal(ROWS.length, 262);
});

test('cpppj ScriptCommand_ConfigOverwriteAndQueueLifetime 의 큐 수명: 예약만으로는 아무것도 바뀌지 않고 입력 순서대로 소비되며 해제된 토큰은 거부된다', () => {
    const queue = new InputQueue();
    const strings = new ScriptStringRing();
    const hooks = { lookup: () => SCRIPT_CONFIG };
    queueScriptCommand('Config,DoneTutorial1=1', queue, strings, hooks);
    queueScriptCommand('Config,DoneTutorial2=1', queue, strings, hooks);
    const resolved = [];
    // 실제 입력 순서로 예약 값을 꺼낸다.
    while (!queue.empty) {
        const event = queue.pop(false);
        assert.equal(event.code, SCRIPT_CONFIG);
        resolved.push(strings.resolve(event.x >>> 0));
    }
    assert.deepEqual(resolved, ['DoneTutorial1=1', 'DoneTutorial2=1']);
    assert.throws(() => strings.resolve(0), RangeError);
});

test('웹 계약: 연결 누락 거부와 인자 변환', () => {
    const strings = new ScriptStringRing();
    const queue = new InputQueue();
    assert.throws(() => queueScriptCommand('a,b', queue, strings, {}), /표 조회 미연결/u);
    assert.throws(() => queueScriptCommand('a,b', queue, strings, undefined), TypeError);
    // 코드 0 은 진단 뒤 계속하고 진단이 없으면 거부한다. 0xffffffff 도 같다.
    assert.throws(() => queueScriptCommand('a,b', queue, strings, { lookup: () => 0 }), /진단 미연결/u);
    assert.throws(() => queueScriptCommand('a,b', queue, strings, { lookup: () => 0xffffffff }), /진단 미연결/u);
    // 코드 0: 진단 뒤 코드 0 인 명령 사건을 예약한다(원본 흐름).
    let diagnosed = 0;
    queueScriptCommand('a,5', queue, strings, { lookup: () => 0, unknown: () => { diagnosed += 1; } });
    assert.equal(diagnosed, 1);
    assert.deepEqual(queue.pop(false), { code: 0x02000000, x: 5, y: 5 });
    // 0xffffffff: 인자를 문자열이면 고리 칸을 쓰고 예약은 하지 않는다.
    const before = strings.index;
    queueScriptCommand('x,text', queue, strings, { lookup: () => 0xffffffff, unknown: () => { diagnosed += 1; } });
    assert.equal(diagnosed, 2);
    assert.equal(strings.index, before + 1);
    assert.equal(queue.empty, true);
    // 숫자 인자는 atol(32비트 감김)이고 부호로 시작하면 문자열이다.
    const hooks = { lookup: () => 0x02000200 };
    queueScriptCommand('n,4294967297', queue, strings, hooks);
    queueScriptCommand('n,-5', queue, strings, hooks);
    queueScriptCommand('n,2147483648', queue, strings, hooks);
    queueScriptCommand('n', queue, strings, hooks);
    assert.deepEqual(queue.pop(false), { code: 0x02000200, x: 1, y: 1 });
    const minus = queue.pop(false);
    assert.equal(strings.resolve(minus.x >>> 0), '-5');
    assert.deepEqual(queue.pop(false), { code: 0x02000200, x: -2147483648, y: -2147483648 });
    const empty = queue.pop(false);
    assert.equal(strings.resolve(empty.x >>> 0), '');
});

test('웹 계약: 문자열 고리는 같은 문자열도 새 칸을 쓰고 NUL 뒤를 보지 않고 첫 토큰은 101 이며 80칸 뒤에 덮인다', () => {
    const ring = new ScriptStringRing();
    const first = ring.store('same');
    const second = ring.store('same');
    assert.notEqual(first, second);
    assert.equal(ring.index, 2);
    assert.equal(ring.store('abc\0hidden') > second, true);
    assert.equal(ring.resolve(ring.store('x\0y')), 'x');
    // 첫 토큰은 101 이고 0 이나 존재하지 않는 값은 거부한다.
    assert.equal(first, 101);
    assert.throws(() => ring.resolve(0), RangeError);
    assert.throws(() => ring.resolve(999999), RangeError);
    // 80개를 더 쓰면 첫 토큰의 칸이 덮인다.
    for (let index = 0; index < 80; index += 1) ring.store(`fill${index}`);
    assert.throws(() => ring.resolve(first), RangeError);
});

test('웹 계약: 입력 큐가 가득 차면 새 명령 사건을 버리고 기존 순서는 유지한다', () => {
    const queue = new InputQueue();
    const strings = new ScriptStringRing();
    const hooks = { lookup: () => 0x02000300 };
    // 39개까지 담긴다.
    for (let index = 0; index < 45; index += 1) queueScriptCommand(`c,${index}`, queue, strings, hooks);
    assert.equal(queue.size, 39);
    for (let index = 0; index < 39; index += 1) assert.equal(queue.pop(false).x, index);
    assert.equal(queue.empty, true);
});
