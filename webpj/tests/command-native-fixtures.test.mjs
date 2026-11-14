// 점 명령/HTML 계열의 원본 10.78 독립 관측 대조: 분배(DotCommandDispatch, 512행)·등록(DotCommandRegister, 250행+보충 v2 12행)·HTML 조건(10.78 행)·HTML 입력 확인(89행)을 같은 표·입력·호출 순서·부분 효과에서 비교한다.
// 입력 열만 읽고 관측 열은 비교에만 쓴다(cpppj `DotCommandDispatchTests`·`DotCommandRegisterTests` 와 같은 방식). 원본게임을 실행하지 않는다. (2026-11-15 WB04-E478)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { DotCommandDispatchState, dispatchDotCommand } from '../www/ui/dot-command-dispatch.js';
import { DOT_COMMAND_STATE_BYTES, copyDotCommandName, registerDotCommand } from '../www/ui/dot-command-register.js';
import { applyHtmlCondition, createParserGlobals, createParserState, parseHtml } from '../www/ui/html-text.js';
import { Config, ConfigRegistry } from '../www/assets/config.js';
import { confirmHtmlInput } from '../www/ui/html-input-dialog.js';

// 탭 구분 fixture 의 머리글/주석 줄을 건너뛴 행 목록이다(빈 열 보존).
function rows(name, headerPrefix) { return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#') && !(headerPrefix && line.startsWith(headerPrefix))).map((line) => line.split('\t')); }
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const putWord = (bytes, at, value) => { for (let i = 0; i < 4; i += 1) bytes[at + i] = (value >>> (i * 8)) & 0xff; };
// 원문 관측의 격리 명령 장치 식별자와 명령 표의 오프셋이다.
const NATIVE_CALLBACK = 0x20000100; const COUNT_OFFSET = 0xa04;

// 최초 원문 관측과 독립적으로 지정했던 등록 표 입력을 구성한다(이름 5개·함수·표시 포인터·count).
function initialTable(scene) {
    const table = new Uint8Array(DOT_COMMAND_STATE_BYTES); const names = ['say', 'clear', 'echo', 'front', 'x'.repeat(31)]; if (scene === 3) names[1] = 'echo';
    // 중복 이름과 실제 31BYTE 이름을 원문 입력과 같은 위치에 쓴다.
    for (let i = 0; i < names.length; i += 1) { for (let c = 0; c < names[i].length; c += 1) table[i * 32 + c] = names[i].charCodeAt(c); putWord(table, 0x800 + i * 4, NATIVE_CALLBACK + i * 16); putWord(table, 0x900 + i * 4, 0x531b30 + i * 32); }
    putWord(table, COUNT_OFFSET, scene === 1 ? 0 : scene === 2 ? 0xffffffff : 5); return table;
}
// 선택한 복사 장치에서 앞선 부분 쓰기를 남기는 명시 중단이다.
class DotStop extends Error {}
// 실제 NUL 앞의 장치 인자만 hex 로 만든다.
function argumentHex(bytes) { const end = bytes.indexOf(0); if (end < 0) throw new Error('명령 장치 인자 NUL 없음'); return hex(bytes.subarray(0, end)); }

test('DotCommandDispatch 원본 10.78 512행: 분배의 전체 표·입력·현재 전역(기본 슬롯/구분자 위치)·외부 호출 순서가 모두 일치한다', () => {
    const table = rows('dot-command-dispatch-x86.tsv', 'text\t'); const failures = [];
    assert.equal(table.length, 512);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 9, `행 ${index}`);
        const scene = Number(row[1]); const mode = Number(row[2]); const commands = initialTable(scene);
        const source = Uint8Array.from(Buffer.from(row[0] === '-' ? '' : row[0], 'hex')); const input = new Uint8Array(source.length + 1 + 32).fill(0xd7); input.set(source); input[source.length] = 0;
        const state = new DotCommandDispatchState(); const trace = []; let calls = 0;
        const hooks = {
            invoke: (token, argument) => {
                calls += 1; trace.push(`C:${token - NATIVE_CALLBACK}:${argumentHex(argument)}`);
                if (calls === 1) {
                    if (mode === 3) { const name = 'changed'; for (let i = 0; i < name.length; i += 1) commands[32 + i] = name.charCodeAt(i); commands[32 + name.length] = 0; putWord(commands, 0x808, NATIVE_CALLBACK + 48); }
                    else if (mode === 4) { state.fallbackIndex = 1; const name = 'new-default'; for (let i = 0; i < name.length; i += 1) commands[32 + i] = name.charCodeAt(i); commands[32 + name.length] = 0; }
                    else if (mode === 5 && argument[0]) argument[0] = 0x5a;
                    else if (mode === 6 && state.separatorOffset !== -1) state.separatorOffset = 2;
                }
                return mode === 2 ? 0x80000001 : (mode === 1 || mode === 3 || mode === 4 || mode === 7) ? 0x1234 : 0;
            },
            error: (name, code) => { trace.push(`E:${hex(Buffer.from(name, 'latin1'))}:${(code >>> 0).toString(16)}`); if (mode === 7 && state.separatorOffset !== -1) state.separatorOffset = 2; },
        };
        const result = dispatchDotCommand(commands, state, input, hooks); const problems = [];
        if (String(result) !== row[3]) problems.push('result'); if (hex(commands) !== row[4]) problems.push('table'); if (String(state.fallbackIndex) !== row[5]) problems.push('default'); if (String(state.separatorOffset) !== row[6]) problems.push('separator');
        if (hex(input) !== row[7]) problems.push('input'); if ((trace.join(';') || '-') !== row[8]) problems.push('trace');
        if (problems.length) failures.push(`${index}/${scene}/${mode}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

test('DotCommandRegister 원본 10.78 250행: count 63 끝쓰기와 복사 뒤 count 변경·복사 중단의 인접 전체 표·반환·복사 인자가 일치한다', () => {
    const table = rows('dot-command-register-x86.tsv'); const failures = [];
    assert.equal(table.length, 250);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 6, `행 ${index}`); const fields = row[1].split(','); assert.equal(fields.length, 5);
        const state = new Uint8Array(DOT_COMMAND_STATE_BYTES); for (let i = 0; i < state.length; i += 1) state[i] = (165 + i * 17) & 0xff; putWord(state, COUNT_OFFSET, Number(fields[0]));
        const name = new Uint8Array(Number(fields[1]) + 1).fill(0x51); name[name.length - 1] = 0; let trace = ''; let completion = 'return';
        const hooks = { copy: (destination, source, count) => {
            trace = `COPY:${destination.byteOffset - state.byteOffset}:0:${count}`; copyDotCommandName(destination, source, count);
            if (fields[4].startsWith('copy-count')) putWord(state, COUNT_OFFSET, Number(fields[4].slice(10))); else if (fields[4] === 'copy-stop') throw new DotStop();
        } };
        let result = 0; try { result = registerDotCommand(state, name, Number(fields[3]), Number(fields[2]), hooks); } catch (error) { if (!(error instanceof DotStop)) throw error; completion = 'copy-stop'; }
        const problems = []; if (completion !== row[2]) problems.push('completion'); if (hex(state) !== row[4]) problems.push('table'); if (trace !== row[5]) problems.push('trace'); if (completion === 'return' && result !== Number(row[3])) problems.push('result');
        if (problems.length) failures.push(`${index}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

test('DotCommandRegister 보충 v2 관측: count 64 정상과 count 65 종단 실패 전 전체 표, 음수 count 의 CRT 인자 경계(4행)가 일치한다', () => {
    const table = rows('dot-command-register-v2-x86.tsv'); let compared = 0; let negativeBoundaries = 0; const failures = [];
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 7, `행 ${index}`); const fields = row[1].split(','); assert.equal(fields.length, 5);
        // 원문이 표 앞 CRT 인자에 도달한 기록이며 host 성공/실제 CRT 실행으로 세지 않는다.
        if (row[2] === 'name-copy-stop') { assert.equal(fields[0], '4294967295'); assert.equal(row[5], 'COPY:-32:0:31'); assert.equal(row[6], 'COPY:531b10:31'); negativeBoundaries += 1; continue; }
        const state = new Uint8Array(DOT_COMMAND_STATE_BYTES); for (let i = 0; i < state.length; i += 1) state[i] = (165 + i * 17) & 0xff; putWord(state, COUNT_OFFSET, Number(fields[0]));
        const name = new Uint8Array(Number(fields[1]) + 1).fill(0x51); name[name.length - 1] = 0; let trace = ''; let completion = 'return';
        const hooks = { copy: (destination, source, count) => { trace = `COPY:${destination.byteOffset - state.byteOffset}:0:${count}`; copyDotCommandName(destination, source, count); putWord(state, COUNT_OFFSET, Number(fields[4].slice(10))); } };
        let result = 0; try { result = registerDotCommand(state, name, Number(fields[3]), Number(fields[2]), hooks); } catch (error) { if (!(error instanceof RangeError)) throw error; completion = 'physical-store-stop'; }
        const problems = []; if (completion !== row[2] || hex(state) !== row[4] || trace !== row[5]) problems.push('state');
        if (completion === 'return') { if (result !== Number(row[3]) || row[6] !== '-') problems.push('result'); } else if (!(row[6] === 'STORE:532538:4:0' && state[COUNT_OFFSET] === 66 && state[COUNT_OFFSET + 1] === 0 && state[COUNT_OFFSET + 2] === 0 && state[COUNT_OFFSET + 3] === 0)) problems.push('store');
        compared += 1; if (problems.length) failures.push(`${index}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures, []); assert.equal(compared, 8); assert.equal(negativeBoundaries, 4);
});

// HTML 조건 효과(HtmlConditionEffect): 실제 HTML 조건/토큰/CRT 와 분리된 Config 치환, 현재 상태 효과를 10.78 행(originals)에 전수 대조한다. 같은 fixture 의 originalCD(10.72) 행은 웹 범위 밖이라 제외한다. (2026-11-15 WB04-E478)
test('HtmlConditionEffect 원본 10.78 행: 조건 태그와 전체 순회의 출력·입력 위치·표시/숨김 깊이·상태 비트·Config 치환 호출 순서가 일치한다', () => {
    const all = rows('htmlcondition-x86.tsv'); const table = all.filter((row) => row[0] === 'originals'); const failures = [];
    assert.ok(table.length > 300 && table.length < all.length);
    const decode = (text) => (text === '-' ? '' : Buffer.from(text, 'hex').toString('latin1'));
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 16, `행 ${index}`);
        const configurations = row[6].split(','); let call = 0; const trace = []; const registry = new ConfigRegistry(); const config = new Config(registry);
        // native 의 Config ABI 경계마다 같은 현재 설정을 공급한다.
        const substitute = (token) => { config.assign(decode(configurations[Math.min(call, configurations.length - 1)])); call += 1; const value = config.substitute(token); trace.push(`${Buffer.from(token, 'latin1').toString('hex')}=${Buffer.from(value, 'latin1').toString('hex')}`); return value; };
        const state = createParserState(); state.shown = Number(row[3]); state.hiddenDepth = Number(row[4]); const globals = createParserGlobals();
        if (row[1] === 'Tag') applyHtmlCondition(state, decode(row[2]), Number(row[5]) !== 0, substitute);
        else parseHtml(decode(row[2]), state, globals, { condition: (current, raw, closing) => { applyHtmlCondition(current, raw, closing, substitute); }, link: () => '' });
        const problems = [];
        if (state.output !== decode(row[7])) problems.push('output'); if (state.inputOffset !== Number(row[8])) problems.push('offset'); if (state.shown !== Number(row[9]) || state.hiddenDepth !== Number(row[10])) problems.push('shown');
        if (state.inTag !== Number(row[11]) || state.textAllowed !== Number(row[12]) || state.afterHeading !== Number(row[13])) problems.push('bits'); if (globals.suppressParagraph !== Number(row[14])) problems.push('paragraph');
        if ((trace.join(';') || '-') !== row[15]) problems.push('trace'); config.dispose();
        if (problems.length) failures.push(`${index}/${row[1]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

test('applyHtmlCondition: 숨김 깊이는 원문 DWORD 증가라 0xffffffff 에서 0 으로 감기고 표시 비트는 꺼진다(원본 관측 htmlcondition 행으로 확인한 이식 결함 교정)', () => {
    const state = createParserState(); state.shown = 1; state.hiddenDepth = 0xffffffff; applyHtmlCondition(state, '?', false, () => '');
    assert.equal(state.shown, 0); assert.equal(state.hiddenDepth, 0);
    // 일반 증가는 그대로 1 씩 오른다.
    const normal = createParserState(); normal.shown = 1; normal.hiddenDepth = 3; applyHtmlCondition(normal, '?', false, () => ''); assert.equal(normal.shown, 0); assert.equal(normal.hiddenDepth, 4);
});

// HTML 입력 대화상자: 원문 확인(004cfae0) 89행을 대조한다. 생성/공장(ShowInput) 20행은 SaveFortNameDialog 이식 때 연결한다(이 파일의 confirm 행만 대상). (2026-11-15 WB04-E478)
test('HtmlInputDialog 원본 10.78 확인 89행: 복사·강제 NUL·첫 LF·종료 NUL 부터의 공백 검사·atol·서식/Tell 호출 순서와 반환이 일치한다', () => {
    const table = rows('html-input-dialog-x86.tsv').filter((row) => row[0] !== 'C'); const failures = [];
    assert.equal(table.length, 89);
    const decode = (text) => (text === '-' ? '' : Buffer.from(text, 'hex').toString('latin1'));
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 11, `행 ${index}`);
        const mode = { value: Number(row[1]) }; const key = decode(row[5]); const input = decode(row[6]); const trace = []; const continuation = new Uint8Array(80); for (let i = 0; i < key.length; i += 1) continuation[i] = key.charCodeAt(i);
        const hooks = {
            copyText: (output) => { trace.push('copy'); for (let i = 0; i < Math.min(input.length, output.length); i += 1) output[i] = input.charCodeAt(i); if (Number(row[7])) { mode.value = 1; continuation.fill(0); for (let i = 0; i < 7; i += 1) continuation[i] = 'changed'.charCodeAt(i); } },
            space: (code) => { trace.push(`space:${code}`); return 0; }, parseLong: (text) => { trace.push(`atol:${Buffer.from(text, 'latin1').toString('hex')}`); return 0; },
            format: (section, text) => { trace.push(`format:${Buffer.from(section, 'latin1').toString('hex')}:${Buffer.from(text, 'latin1').toString('hex')}`); return `${section}(${text})`; },
            tell: (text) => { trace.push(`tell:${Buffer.from(text, 'latin1').toString('hex')}`); },
        };
        const result = confirmHtmlInput(mode, continuation, hooks); const problems = [];
        if (result !== Number(row[8])) problems.push('result'); if (trace.join(';') !== row[10]) problems.push('trace');
        if (problems.length) failures.push(`${index}/${row[1]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});
