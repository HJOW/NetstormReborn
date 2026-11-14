// 일반 입력 명령 이름표/우선순위 계열의 원본 10.78 독립 관측 대조: AI 일반 명령 이름표(30행)·MakeAi 이름표(12행)·AI 진단 이름표(옛 별칭 21행+실제 Toggle 이름 63행)·WinMain 스레드 우선순위 계획(96행)을
// cpppj `AiUtilityCommandInputTests`·`MakeAiCommandInputTests`·`AiDebugMenuTests`·`ConfigurationCommandInputTests`(PriorityLoopTests.inc)와 같은 입력 구성으로 비교한다.
// 이름표 시험은 원본의 명령 큐 넣기(004c9090)를 실제 큐·문자열 고리로 재생해 큐 사건·고리 위치·진단 횟수를 대조하고, 관측 열은 비교에만 쓴다. 원본 게임을 실행하지 않는다. (2026-11-15 WB04-E479)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { InputQueue } from '../www/core/input-queue.js';
import { ScriptStringRing, queueScriptCommand } from '../www/ui/script-command.js';
import { AI_PARAM_SET_COMMAND, BRAIN_CRANK_MONEY_COMMAND, CREATE_INITIAL_GEYSERS_COMMAND, aiUtilityCommandScriptCode } from '../www/core/client-input-commands.js';
import { AI_REPORTING_COMMAND, DEBUG_SWITCH_COMMAND, aiDebugCommandScriptCode } from '../www/core/ai-debug-command-input.js';
import { MAKE_AI_COMMAND, makeAiCommandScriptCode } from '../www/core/script-command-inputs.js';
import { planNativeThreadPriority } from '../www/core/configuration-command-input.js';

// 탭 구분 fixture 의 머리글/주석 줄을 건너뛴 행 목록이다(빈 열 보존). headerPrefix 로 시작하는 줄도 건너뛴다.
function rows(name, headerPrefix) { return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#') && !(headerPrefix && line.startsWith(headerPrefix))).map((line) => line.split('\t')); }
// 16진 문자열을 한 글자 한 바이트의 원문으로 복원한다(`-` 는 빈 값).
function bytesOf(encoded) { return encoded === '-' ? '' : Buffer.from(encoded, 'hex').toString('latin1'); }

// 이름표 한 행(원문 + 기대 큐 사건 + 기대 고리 위치 + 사건 목록)을 실제 큐/고리에 재생하고 어긋난 항목 이름을 돌려준다. lookup 은 해당 모듈의 이름 조회, expectCode 는 큐 사건 코드의 기대 목록이다.
function replayNameRow(row, lookup, commands) {
    assert.equal(row.length, 9);
    const problems = [];
    const queue = new InputQueue(); const strings = new ScriptStringRing(); const diagnostics = [];
    queueScriptCommand(bytesOf(row[1]), queue, strings, { lookup, unknown() { diagnostics.push(strings.index); } });
    if (strings.index !== Number(row[7])) problems.push('ring');
    const events = row[8] === '-' ? [] : row[8].split(';');
    // 진단 'L' 줄 수와 새 고리 문자열 'New' 줄 수가 관측과 같아야 한다.
    if (diagnostics.length !== events.filter((event) => event.startsWith('L,')).length) problems.push('diagnostics');
    if (events.filter((event) => event.startsWith('New,')).length !== strings.index) problems.push('new-count');
    if (row[6] === '-') {
        // 미등록 이름은 큐를 비운 채 진단을 남긴다.
        if (!queue.empty || diagnostics.length === 0) problems.push('rejected');
    } else {
        const split = row[6].indexOf(',');
        const code = Number(row[6].slice(0, split)); const expected = row[6].slice(split + 1);
        if (queue.empty) { problems.push('queue-empty'); return problems; }
        const actual = queue.pop(false);
        if (actual.code !== code || actual.x !== actual.y || !queue.empty || diagnostics.length !== 0 || !commands.includes(actual.code)) problems.push('event');
        if (expected[0] === 's') { if (strings.resolve(actual.x >>> 0) !== bytesOf(expected.slice(1))) problems.push('string'); } else if ((actual.x >>> 0) !== Number(expected.slice(1))) problems.push('number');
    }
    return problems;
}

test('AiUtilityCommandInput 원본 10.78 이름표 30행: 원래/대소문자·숫자/문자열 인자와 미등록의 큐 사건·고리 위치·진단이 일치한다', () => {
    const table = rows('ai-utility-names-x86.tsv'); const failures = [];
    assert.equal(table.length, 30);
    for (const [index, row] of table.entries()) { const problems = replayNameRow(row, aiUtilityCommandScriptCode, [CREATE_INITIAL_GEYSERS_COMMAND, BRAIN_CRANK_MONEY_COMMAND, AI_PARAM_SET_COMMAND]); if (problems.length) failures.push(`${index}: ${problems.join(',')}`); }
    assert.deepEqual(failures.slice(0, 20), []);
    // 원문 보호 PE 초기값과 이름표(cpppj 의 GameMenuCommandValue 확인에 해당).
    assert.equal(aiUtilityCommandScriptCode('createinitialgeysers'), CREATE_INITIAL_GEYSERS_COMMAND);
    assert.equal(aiUtilityCommandScriptCode('BrainCrankMoney'), BRAIN_CRANK_MONEY_COMMAND);
    assert.equal(aiUtilityCommandScriptCode('AIPARAMSET'), AI_PARAM_SET_COMMAND);
});

test('MakeAiCommandInput 원본 10.78 이름표 12행: 큐 사건·고리 위치·진단이 일치하고 첫 NUL 까지만 비교한다', () => {
    const table = rows('make-ai-names-x86.tsv'); const failures = [];
    assert.equal(table.length, 12);
    for (const [index, row] of table.entries()) { const problems = replayNameRow(row, makeAiCommandScriptCode, [MAKE_AI_COMMAND]); if (problems.length) failures.push(`${index}: ${problems.join(',')}`); }
    assert.deepEqual(failures.slice(0, 20), []);
    // 원문 stricmp 의 첫 NUL 종료 계약을 이름 조회에서도 고정한다(cpppj 직접 확인에 해당).
    assert.equal(makeAiCommandScriptCode('MakeAi\0ignored'), MAKE_AI_COMMAND);
    assert.equal(makeAiCommandScriptCode('MakeAiX'), 0xffffffff);
});

test('AiDebugCommandInput 원본 10.78 이름표 84행(옛 별칭 21행+실제 Toggle 이름 63행): 숫자/문자열/첫 NUL/큐·고리·진단이 일치한다', () => {
    const aliases = rows('ai-debug-names-x86.tsv'); const current = rows('ai-debug-toggle-names-x86.tsv'); const failures = [];
    assert.equal(aliases.length, 21); assert.equal(current.length, 63);
    // 옛 별칭 21행은 전부 미등록이어야 하고, 실제 이름 63행에는 등록 32행과 철자/공백이 어긋난 미등록 31행이 섞여 있다.
    for (const row of aliases) assert.equal(row[6], '-');
    assert.equal(current.filter((row) => row[6] !== '-').length, 32);
    for (const [index, row] of [...aliases, ...current].entries()) { const problems = replayNameRow(row, aiDebugCommandScriptCode, [AI_REPORTING_COMMAND, DEBUG_SWITCH_COMMAND]); if (problems.length) failures.push(`${index}: ${problems.join(',')}`); }
    assert.deepEqual(failures.slice(0, 20), []);
    // 원문 stricmp 의 첫 NUL 종료를 공개 조회에서 고정한다(cpppj 직접 확인에 해당).
    assert.equal(aiDebugCommandScriptCode('ToggleDebug\0ignored'), DEBUG_SWITCH_COMMAND);
    assert.equal(aiDebugCommandScriptCode('ToggleReporting\0ignored'), AI_REPORTING_COMMAND);
});

test('이름표 독립 확인: 등록 이름과 한 글자라도 다른 근접 이름은 어떤 모듈의 조회에도 걸리지 않고 첫 NUL 앞만 비교한다', () => {
    // 실제 이름표에 없는 접두/접미/별칭/공백 변형이다(관측 fixture 가 담지 않은 근접 입력).
    const near = ['', ' ', 'Debug', 'Reporting', 'Crank', 'CrankMoney', 'AiParam', 'AiParamSets', 'CreateInitialGeyser', 'CreateInitialGeysersX', 'MakeAiX', 'XMakeAi', 'ToggleDebugX', ' ToggleDebug', 'ToggleReport'];
    for (const name of near) {
        assert.equal(aiUtilityCommandScriptCode(name), 0xffffffff, name);
        assert.equal(makeAiCommandScriptCode(name), 0xffffffff, name);
        assert.equal(aiDebugCommandScriptCode(name), 0xffffffff, name);
    }
    // 앞쪽 NUL 은 빈 이름(미등록)이 되고, 뒤쪽 NUL 뒤는 무시되며 세 모듈이 서로의 이름을 받지 않는다.
    assert.equal(makeAiCommandScriptCode('\0MakeAi'), 0xffffffff);
    assert.equal(aiDebugCommandScriptCode('\0ToggleDebug'), 0xffffffff);
    assert.equal(aiUtilityCommandScriptCode('MakeAi'), 0xffffffff);
    assert.equal(aiDebugCommandScriptCode('AiParamSet'), 0xffffffff);
    assert.equal(makeAiCommandScriptCode('ToggleDebug'), 0xffffffff);
});

test('ConfigurationCommandInput 원본 10.78 WinMain 우선순위 루프 96행: 요청 원값·OS 등급 정규화·설정 여부·선택 우선순위가 일치한다', () => {
    const table = rows('priority-loop-x86.tsv', 'name\t'); const failures = [];
    assert.equal(table.length, 96);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 17, `행 ${index}`);
        const problems = [];
        // 비MP 도전 0 의 active 입력으로 caller 가 선택한 raw 요청만 공급한다.
        if (Number(row[1]) !== 0) problems.push('challenge');
        const requested = Number(row[3]) | 0; const wanted = Number(row[2]) ? requested : 0;
        const plan = planNativeThreadPriority(wanted, Number(row[4]) | 0);
        if ((plan.wanted >>> 0) !== Number(row[8]) || plan.currentLevel !== (Number(row[9]) | 0)) problems.push('level');
        if (plan.change !== (Number(row[10]) !== 0)) problems.push('change');
        if (plan.change && plan.priority !== (Number(row[11]) | 0)) problems.push('priority');
        if (row[13] !== '1' || row[14] !== '0') problems.push('completion');
        if (problems.length) failures.push(`${index}/${row[0]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});
