// 전용 PE fixture가 없는 HTML $Input 확인(ConfirmHtmlInput: 편집 문자열 복사·첫 LF 절단·NUL 공백 검사·선택 atol·서식/Tell)을 기준선 소스 계약으로 검증한다. (2026-11-14 WB04-E435)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 호출 순서/조건은 기준선 소스(HtmlInputDialog.cpp, 원문 004cfae0)에서 옮겼다. 편집 자식/서식/Tell은 호출 기록 대역이다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { confirmHtmlInput } from '../www/ui/html-input-dialog.js';

const bytesOf = (text) => Uint8Array.from(Buffer.from(text, 'latin1'));
// 편집 문자열을 공급하고 모든 호출을 기록하는 장치다. space 는 기본이 isspace(0)=0 이다.
function rig(text, options = {}) {
    const log = []; const hooks = {
        copyText: (buffer) => { log.push(['copy']); buffer.fill(0x7a); const source = typeof text === 'string' ? bytesOf(text) : text; buffer.set(source.subarray(0, buffer.length)); if (source.length < buffer.length) buffer[source.length] = 0; options.onCopy?.(); },
        space: (value) => { log.push(['space', value]); return options.space?.(value) ?? 0; }, parseLong: (input) => { log.push(['parse', input]); return 99; },
        format: (section, input) => { log.push(['format', section, input]); return `${section}(${input})`; }, tell: (section) => log.push(['tell', section]) };
    for (const key of options.omit ?? []) delete hooks[key]; return { log, hooks };
}

test('기본 경로: CopyText→NUL 공백 검사 한 번→(mode 가 1 이 아니면) atol→"섹션(입력)" 서식→Tell 한 뒤 1 을 반환한다', () => {
    const r = rig('abc'); assert.equal(confirmHtmlInput({ value: 2 }, 'Next\0', r.hooks), 1);
    assert.deepEqual(r.log, [['copy'], ['space', 0], ['parse', 'abc'], ['format', 'Next', 'abc'], ['tell', 'Next(abc)']]);
    // 모드 1 은 atol 을 부르지 않는다. 숫자 입력도 재서식하지 않는다.
    const one = rig('007'); confirmHtmlInput({ value: 1 }, 'Go\0', one.hooks); assert.deepEqual(one.log.map((entry) => entry[0]), ['copy', 'space', 'format', 'tell']); assert.deepEqual(one.log.at(-1), ['tell', 'Go(007)']);
    // 현재 mode 는 CopyText 뒤에 읽는다(편집 콜백이 mode 를 바꿔도 반영).
    const mode = { value: 2 }; const late = rig('x', { onCopy: () => { mode.value = 1; } }); confirmHtmlInput(mode, 'A\0', late.hooks); assert.equal(late.log.some((entry) => entry[0] === 'parse'), false);
});

test('첫 LF 절단·강제 NUL: 첫 줄바꿈 앞만 입력이고 256칸이 꽉 차면 마지막 칸을 NUL 로 바꾸며 NUL 이 없는 입력도 255바이트로 자른다', () => {
    const lf = rig('ab\ncd'); confirmHtmlInput({ value: 1 }, 'S\0', lf.hooks); assert.deepEqual(lf.log.at(-1), ['tell', 'S(ab)']); assert.deepEqual(lf.log[1], ['space', 0]);
    const first = rig('\nrest'); confirmHtmlInput({ value: 1 }, 'S\0', first.hooks); assert.deepEqual(first.log.at(-1), ['tell', 'S()']); assert.equal(first.log.some((entry) => entry[0] === 'space'), false);
    const full = rig(bytesOf('q'.repeat(300))); confirmHtmlInput({ value: 1 }, 'S\0', full.hooks); assert.equal(full.log.at(-1)[1].length, 'S()'.length + 255); assert.deepEqual(full.log[1], ['space', 0]);
    // 바이트 값은 그대로(라틴-1)다. 빈 입력은 공백 검사를 하지 않는다.
    const high = rig(Uint8Array.of(0xe9, 0xff, 0)); confirmHtmlInput({ value: 1 }, 'S\0', high.hooks); assert.deepEqual(high.log.at(-1), ['tell', 'S(\xe9\xff)']); const empty = rig(''); confirmHtmlInput({ value: 2 }, 'S\0', empty.hooks); assert.deepEqual(empty.log.map((entry) => entry[0]), ['copy', 'parse', 'format', 'tell']);
});

test('공백 검사: 종료 NUL 부터 앞쪽으로 isspace 가 참인 동안 지우고 첫 거짓에서 멈추며 맨 앞 칸은 검사하지 않는다', () => {
    // 모든 값을 공백으로 보면 (맨 앞 글자를 뺀) 종료 NUL 부터 앞쪽 글자를 모두 지운다.
    const all = rig('abc', { space: () => 1 }); confirmHtmlInput({ value: 1 }, 'S\0', all.hooks); assert.deepEqual(all.log.filter((entry) => entry[0] === 'space'), [['space', 0], ['space', 99], ['space', 98]]); assert.deepEqual(all.log.at(-1), ['tell', 'S(a)']);
    // 0 만 공백이면 NUL 하나만 지우고 다음 글자에서 멈춘다. 음수 바이트는 int8 로 전달한다.
    const nulOnly = rig('ab', { space: (value) => (value === 0 ? 1 : 0) }); confirmHtmlInput({ value: 1 }, 'S\0', nulOnly.hooks); assert.deepEqual(nulOnly.log.at(-1), ['tell', 'S(ab)']); assert.deepEqual(nulOnly.log.filter((entry) => entry[0] === 'space'), [['space', 0], ['space', 98]]);
    const high = rig(Uint8Array.of(0x80, 0xff), { space: () => 0 }); confirmHtmlInput({ value: 1 }, 'S\0', high.hooks); assert.deepEqual(high.log[1], ['space', 0]); const signed = rig(Uint8Array.of(0x41, 0xe9, 0), { space: (value) => (value === 0 ? 1 : 0) }); confirmHtmlInput({ value: 1 }, 'S\0', signed.hooks); assert.deepEqual(signed.log.filter((entry) => entry[0] === 'space'), [['space', 0], ['space', -23]]);
});

test('후속 섹션: NUL 앞까지(바이트 문자열/Uint8Array)를 쓰고 NUL 이 없으면 입력을 확인하기 전에 거부하지 않고 서식 전에 거부한다', () => {
    const bytes = rig('v'); confirmHtmlInput({ value: 1 }, bytesOf('Sec\0junk'), bytes.hooks); assert.deepEqual(bytes.log.at(-1), ['tell', 'Sec(v)']);
    const nul = rig('v'); assert.throws(() => confirmHtmlInput({ value: 2 }, 'NoTerminator', nul.hooks), (error) => error instanceof RangeError && /80바이트 NUL 범위/.test(error.message)); assert.deepEqual(nul.log.map((entry) => entry[0]), ['copy', 'space', 'parse']);
    assert.throws(() => confirmHtmlInput({ value: 2 }, bytesOf('abc'), rig('v').hooks), /NUL 범위/); const lead = rig('v'); confirmHtmlInput({ value: 1 }, '\0abc', lead.hooks); assert.deepEqual(lead.log.at(-1), ['tell', '(v)']);
});

test('필수 장치: 도달한 경계가 없으면 거부하고 부분 효과를 성공으로 감추지 않는다', () => {
    for (const omit of ['copyText', 'format', 'tell']) assert.throws(() => confirmHtmlInput({ value: 1 }, 'S\0', rig('v', { omit: [omit] }).hooks), (error) => error.constructor === Error && /HTML 입력 확인의 실제 장치 미연결/.test(error.message), omit);
    assert.throws(() => confirmHtmlInput({ value: 1 }, 'S\0', rig('v', { omit: ['space'] }).hooks), /실제 장치 미연결/); assert.doesNotThrow(() => confirmHtmlInput({ value: 1 }, 'S\0', rig('', { omit: ['space', 'parseLong'] }).hooks));
    assert.throws(() => confirmHtmlInput({ value: 2 }, 'S\0', rig('', { omit: ['parseLong'] }).hooks), /실제 장치 미연결/); assert.doesNotThrow(() => confirmHtmlInput({ value: 1 }, 'S\0', rig('v', { omit: ['parseLong'] }).hooks));
});
