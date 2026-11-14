// 원본 HtmlTextParser 기본 변환의 실제 10.78 관측(본문 408행·배경 80BYTE 648행·그림 138행)과 현재 상태/외부 효과 경계/콜백 위치 재조회/StyleText 전달을 검사한다.
// 원본게임을 실행하지 않는다. CD 10.72(originalCD) 행은 웹 범위 밖이라 읽지 않는다. (2026-11-14 WB04-E460)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {
    HtmlTextParser, HTML_BACKGROUND_SIZE, HTML_TEXT_CAPACITY, bindHtmlBackground, bindHtmlPictureText, createHtmlTextParserGlobals, createHtmlTextParserState,
    htmlBackgroundName, htmlPictureText,
} from '../www/ui/html-text-parser.js';
import { StyleText, StyleTextState, styleTextField } from '../www/ui/style-text-fragments.js';

// 원본 fixture 의 hex 열을 원문 BYTE 문자열로 바꾼다. 기대 변환 자체는 계산하지 않는다.
const unhex = (value) => Buffer.from(value, 'hex').toString('latin1');
// 80BYTE 저장소를 원문 hex 로 읽는다.
const hex = (bytes) => Buffer.from(bytes).toString('hex');
// 탭 구분 fixture 의 주석/빈 줄을 건너뛰고 열 목록으로 읽는다. 빈 입력 열도 보존한다.
function rows(name) {
    return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line && !line.startsWith('#')).map((line) => line.split('\t'));
}
// fixture 한 행의 시작 상태를 만든다(shown/hiddenDepth/P 억제).
function seeded(shown, hiddenDepth, suppress) {
    const state = createHtmlTextParserState(); state.shown = Number(shown); state.hiddenDepth = Number(hiddenDepth);
    const globals = createHtmlTextParserGlobals(); globals.suppressParagraph = Number(suppress); return { state, globals };
}
// 변환 뒤 다섯 순회 필드와 P 억제 DWORD 의 기대값 열을 모두 대조한다. columns 는 [inputOffset, shown, hiddenDepth, inTag, textAllowed, afterHeading, suppress] 열 값이다.
function assertCurrent(state, globals, columns, label) {
    assert.deepEqual([state.inputOffset, state.shown, state.hiddenDepth, state.inTag, state.textAllowed, state.afterHeading, globals.suppressParagraph], columns.map(Number), label);
}

test('HTML 기본 변환: 실제 10.78 본문 408행의 출력·커서·조건 상태·P 억제 전부 일치한다', () => {
    const table = rows('htmltext-x86.tsv').filter((row) => row[0] === 'originals'); assert.equal(table.length, 408);
    // 원본 관측은 linkTape=0 이라 링크 helper 는 실제 null 저장소처럼 빈 문자열을 공급한다.
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 14, `행 ${index}`);
        const { state, globals } = seeded(row[3], row[4], row[5]); const parser = new HtmlTextParser(state, globals, { link: () => '' });
        assert.equal(parser.parse(unhex(row[1]), Number(row[2])), unhex(row[6]), `행 ${index} 출력 입력 ${row[1]}`);
        assertCurrent(state, globals, row.slice(7), `행 ${index} 상태`);
    }
});

test('HTML 기본 변환: 원본 배경 80BYTE 648행의 출력·상태와 짧은 덮어쓰기의 뒤쪽 보존이 일치한다', () => {
    const table = rows('html-background-x86.tsv'); assert.equal(table.length, 648);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 14, `행 ${index}`);
        const seed = Buffer.from(row[0], 'hex'); assert.equal(seed.length, HTML_BACKGROUND_SIZE);
        const { state, globals } = seeded(row[2], row[3], row[4]); globals.background.set(seed); const hooks = {}; bindHtmlBackground(hooks, globals);
        assert.equal(new HtmlTextParser(state, globals, hooks).parse(unhex(row[1])), unhex(row[5]), `행 ${index} 출력`);
        assertCurrent(state, globals, [row[6], row[7], row[8], row[9], row[10], row[11], row[12]], `행 ${index} 상태`);
        assert.equal(hex(globals.background), row[13], `행 ${index} 배경 80BYTE`);
    }
});

test('HTML 기본 변환: 새 파서/빈 이름/첫 NUL/숨긴 태그에서도 같은 장기 배경을 쓰고 원문 오버런만 거부한다', () => {
    const globals = createHtmlTextParserGlobals(); const first = {}; const second = {}; bindHtmlBackground(first, globals); bindHtmlBackground(second, globals);
    const a = createHtmlTextParserState(); const b = createHtmlTextParserState();
    assert.equal(htmlBackgroundName(globals), ''); new HtmlTextParser(a, globals, first).parse('<background="nsstart">First'); assert.equal(htmlBackgroundName(globals), 'nsstart');
    const saved = globals.background.slice();
    new HtmlTextParser(b, globals, second).parse('Next page'); assert.deepEqual(globals.background, saved);
    b.shown = 0; new HtmlTextParser(b, globals, second).parse('<background="hidden">'); assert.deepEqual(globals.background, saved);
    b.shown = 1; second.background(b, 'ab\0ignored'); assert.equal(htmlBackgroundName(globals), 'ab'); assert.equal(globals.background[3], 't'.charCodeAt(0));
    const shortened = globals.background.slice();
    assert.throws(() => new HtmlTextParser(b, globals, second).parse(`<background="${'x'.repeat(80)}">`), RangeError); assert.deepEqual(globals.background, shortened);
    new HtmlTextParser(b, globals, second).parse('<background="">'); assert.equal(htmlBackgroundName(globals), ''); assert.equal(globals.background[1], 'b'.charCodeAt(0));
    globals.background.fill('x'.charCodeAt(0)); assert.throws(() => htmlBackgroundName(globals), /NUL 없음/);
});

test('HTML 기본 변환: 구현하지 않은 실제 효과가 선택되면 거부하고 숨긴 비조건 태그는 호출하지 않는다', () => {
    const state = createHtmlTextParserState(); const globals = createHtmlTextParserGlobals(); const parser = new HtmlTextParser(state, globals);
    // 각각 실제 원본에서 외부 효과를 요구하는 분기와 그 이름이다.
    for (const [source, name] of [['<$cmd 1>', 'command'], ['<?condition>', 'condition'], ['<!"picture">', 'picture'], ['<a href="target">', 'link-open'], ['</a>', 'link-close'], ['<background="cloud">', 'background']]) {
        assert.throws(() => parser.parse(source), new Error(`HTML 실제 효과 미연결: ${name}`), source);
    }
    state.shown = 0; assert.equal(parser.parse('<$cmd><!"picture"><background="cloud">'), '\n');
    state.shown = 1; assert.equal(parser.parse('<unknown>text'), 'text\n'); assert.equal(parser.parse('<background>text'), 'text\n');
    assert.throws(() => parser.parse('<b><b><b>', 2), RangeError);
});

test('HTML 기본 변환: 조건과 명령 callback 은 현재 상태를 직접 바꾸고 그 뒤 분기부터 새 값을 읽는다', () => {
    const state = createHtmlTextParserState(); const globals = createHtmlTextParserGlobals(); const calls = [];
    const hooks = {
        // 별도 조건 계산기 대신 명시한 효과만 공급한다.
        condition: (current, raw, close) => { calls.push(raw); current.shown = close ? 1 : 0; current.hiddenDepth = close ? 0 : 1; },
        // 큐 경계는 '$' 가 제거된 나머지 원문을 받는다.
        command: (current, raw) => { calls.push(raw); current.output += 'Q'; },
    };
    const parser = new HtmlTextParser(state, globals, hooks); assert.equal(parser.parse('a<?off>hidden</?off>b<$raw 7>c'), 'abQc\n');
    assert.deepEqual(calls, ['?off>hidden</?off>b<$raw 7>c', '?off>b<$raw 7>c', 'raw 7>c']);
    assert.deepEqual([state.shown, state.hiddenDepth, state.textAllowed], [1, 0, 1]);
    assert.equal(parser.parse('next'), 'next\n'); assert.deepEqual([state.shown, state.hiddenDepth], [1, 0]);
});

test('HTML 기본 변환: 파생 슬롯은 현재 커서/문자열을 관찰하고 다음 조회 위치를 바꿀 수 있다', () => {
    const state = createHtmlTextParserState(); const globals = createHtmlTextParserGlobals(); const calls = [];
    const hooks = {
        // virtual0: 태그 안 일반 문자에 도달한다.
        insideTag: (current, source) => { calls.push(`in:${current.inputOffset}`); assert.notEqual(source[current.inputOffset], '<'); },
        // virtual1: 닫힘에서 '/' 로 이동한 직후다.
        beforeClose: (current, source) => { calls.push(`close:${current.inputOffset}`); assert.equal(source[current.inputOffset], '/'); },
        // virtual2: '>' 에서 inTag=0 을 쓴 뒤다.
        afterTag: (current, source) => { calls.push(`after:${current.inputOffset}`); assert.ok(source[current.inputOffset] === '>' && current.inTag === 0); },
    };
    assert.equal(new HtmlTextParser(state, globals, hooks).parse('<i>x</i>'), '~N~jx~.\n');
    assert.deepEqual(calls, ['in:1', 'after:2', 'close:5', 'in:6', 'after:7']);
    // 원본 raw 복사처럼 callback 뒤 포인터를 다시 읽는지 검사한다.
    const moved = { afterTag: (current) => { current.inputOffset += 2; } };
    assert.equal(new HtmlTextParser(state, globals, moved).parse('<b>12X'), '~N~BX\n');
});

test('HTML 기본 변환: Early1 본문의 i/b 는 줄을 나누지 않고 색/굵기 제어로 StyleText 조각에 전달된다', () => {
    const state = createHtmlTextParserState(); const globals = createHtmlTextParserGlobals();
    const converted = new HtmlTextParser(state, globals).parse('walk <i>edge</i> and <b>open</b>.'); assert.equal(converted, 'walk ~N~jedge~. and ~N~Bopen~N.\n');
    const styleState = new StyleTextState(); const style = new StyleText(styleState, { color: (code) => { assert.equal(code, 'j'); return 77; } }); // 색만 공급하며 i 폰트 치환은 하지 않는다.
    const head = styleTextField({ head: 0 }, 'head'); const context = Uint32Array.from([0, 0, 17, 0, 0, 0, 0, 0]); style.parse(head, converted, context);
    let plain = ''; let edge = false; let open = false;
    // 실제 생성된 조각의 본문과 저장 문맥을 순회해 색/스타일/원문 줄 수를 확인한다.
    for (let token = head.value; token; token = styleState.fragments.get(token).next) {
        const node = styleState.fragments.get(token); const text = String.fromCharCode(...node.text.subarray(0, node.text.indexOf(0))); plain += text;
        if (text === 'edge') { edge = true; assert.deepEqual([node.context[2], node.context[3]], [77, 0]); }
        if (text === 'open') { open = true; assert.equal(node.context[3], 2); }
    }
    assert.ok(plain === 'walk edge and open.\n' && edge && open);
});

test('HTML 그림 helper: 실제 10.78 그림 138행의 바이트·커서·현재 상태가 일치한다', () => {
    const table = rows('htmlpicture-x86.tsv').filter((row) => row[0] === 'originals'); assert.equal(table.length, 138);
    // 그림 helper 는 실제 출력만 공급하며 자산 이름 해석은 StyleTextMarkup 의 책임이다.
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 14, `행 ${index}`);
        const { state, globals } = seeded(row[3], row[4], row[5]); const hooks = {}; bindHtmlPictureText(hooks);
        assert.equal(new HtmlTextParser(state, globals, hooks).parse(unhex(row[1]), Number(row[2])), unhex(row[6]), `행 ${index} 출력 입력 ${row[1]}`);
        assertCurrent(state, globals, row.slice(7), `행 ${index} 상태`);
    }
});

// 원본 관측이 닿지 않는 기본값/경계/수명을 독립 입력으로 고정한다(변형 확인에서 생존한 지점). (2026-11-14 WB04-E460)
// 새 파서 한 번 실행의 결과와 상태를 돌려준다.
function run(source, { capacity, shown, hooks = {}, state = createHtmlTextParserState(), globals = createHtmlTextParserGlobals() } = {}) {
    if (shown !== undefined) state.shown = shown;
    const output = capacity === undefined ? new HtmlTextParser(state, globals, hooks).parse(source) : new HtmlTextParser(state, globals, hooks).parse(source, capacity);
    return { output, state, globals };
}

test('HTML 기본 변환: 기본 상태/전역 값과 기본 용량 8192의 평문 논리 한도(용량-256)가 원본과 같다', () => {
    assert.deepEqual(createHtmlTextParserState(), { shown: 1, hiddenDepth: 0, inTag: 0, textAllowed: 0, afterHeading: 0, inputOffset: 0, output: '' });
    assert.deepEqual(createHtmlTextParserGlobals(), { suppressParagraph: 0, background: new Uint8Array(80) });
    assert.equal(HTML_TEXT_CAPACITY, 8192); assert.equal(HTML_BACKGROUND_SIZE, 80);
    // 출력이 용량-256 을 넘는 순간 평문 처리가 멈추고(7937글자) 마지막 LF 만 붙는다.
    assert.equal(run('a'.repeat(9000)).output.length, 7938); assert.equal(run('a'.repeat(9000), { capacity: 8192 }).output.length, 7938);
    assert.equal(run('a'.repeat(9000), { capacity: 8193 }).output.length, 7939); assert.equal(run('a'.repeat(9000), { capacity: 600 }).output.length, 346);
    // 시작 상태의 지난 값은 지우고 shown/hiddenDepth 는 유지하며 textAllowed 는 shown 에서 시작한다.
    const state = createHtmlTextParserState(); Object.assign(state, { shown: 3, hiddenDepth: 5, inTag: 1, textAllowed: 9, afterHeading: 1, inputOffset: 9, output: 'zz' });
    assert.equal(run('x', { state }).output, 'x\n'); assert.deepEqual(state, { shown: 3, hiddenDepth: 5, inTag: 0, textAllowed: 3, afterHeading: 0, inputOffset: 1, output: 'x\n' });
    // 마지막 LF/NUL 을 담을 물리 버퍼가 없으면 거부하고, 정확히 맞는 용량은 허용한다.
    assert.throws(() => run('', { capacity: 1 }), RangeError); assert.throws(() => run('', { capacity: 0 }), RangeError); assert.equal(run('', { capacity: 2 }).output, '\n');
    // 평문은 논리 한도(용량-256)가 먼저 막아 작은 용량에서는 본문 없이 LF 만 남고, 태그 출력만 물리 한도(출력+길이 >= 용량)에서 거부된다.
    assert.equal(run('a', { capacity: 2 }).output, '\n'); assert.equal(run('<b>', { capacity: 6 }).output, '~N~B\n'); assert.throws(() => run('<b>', { capacity: 5 }), /물리 출력 버퍼 초과/); assert.throws(() => run('<b>', { capacity: 4 }), /물리 출력 버퍼 초과/);
});

test('HTML 기본 변환: 첫 NUL 앞만 처리하고 입력 첫 글자의 NUL 도 빈 본문이다', () => {
    assert.equal(run('\0abc').output, '\n'); assert.equal(run('ab\0cd').output, 'ab\n'); assert.equal(run('<b>x\0<i>').output, '~N~Bx\n');
    assert.equal(htmlPictureText('\0"x"'), '~[I]'); assert.equal(htmlPictureText('!"a\0b">'), '~[Ia]');
    const globals = createHtmlTextParserGlobals(); const hooks = {}; bindHtmlBackground(hooks, globals); hooks.background({}, '\0abc'); assert.equal(htmlBackgroundName(globals), ''); assert.ok(globals.background.every((byte) => byte === 0));
});

test('HTML 그림 helper: 첫 따옴표 안만 옮기고 인용 전/안의 꺾쇠·두 번째 따옴표에서 멈춘다', () => {
    assert.equal(htmlPictureText('"abc">'), '~[Iabc]'); assert.equal(htmlPictureText('!"a">'), '~[Ia]'); assert.equal(htmlPictureText('x>"a"'), '~[I]');
    assert.equal(htmlPictureText('!"a"b">'), '~[Ia]'); assert.equal(htmlPictureText('!"a>b"'), '~[Ia]'); assert.equal(htmlPictureText('!x'), '~[I]'); assert.equal(htmlPictureText(''), '~[I]');
    assert.equal(htmlPictureText('!"'), '~[I]'); assert.equal(htmlPictureText('!" '), '~[I ]'); assert.equal(htmlPictureText('!"A.1'), '~[IA.1]');
});

test('HTML 배경 helper: 79BYTE 이름은 허용하고 80BYTE 나 BYTE 가 아닌 글자는 아무것도 쓰기 전에 거부한다', () => {
    const globals = createHtmlTextParserGlobals(); const hooks = {}; bindHtmlBackground(hooks, globals);
    globals.background.fill(0x7a); hooks.background({}, 'k'.repeat(79)); assert.equal(globals.background[78], 0x6b); assert.equal(globals.background[79], 0); assert.equal(htmlBackgroundName(globals).length, 79);
    const saved = globals.background.slice(); assert.throws(() => hooks.background({}, 'k'.repeat(80)), /NUL 범위 초과/); assert.deepEqual(globals.background, saved);
    assert.throws(() => hooks.background({}, 'bĀ'), /BYTE 문자열 필요/); assert.deepEqual(globals.background, saved); assert.throws(() => hooks.background({}, 'Āb'), /BYTE 문자열 필요/); assert.deepEqual(globals.background, saved);
    hooks.background({}, 'aÿ'); assert.equal(globals.background[1], 0xff); assert.equal(htmlBackgroundName(globals), 'aÿ');
});

test('HTML 기본 변환: 제목 닫힘 뒤 공백만 출력한 상태와 br 복귀·P 빈 줄 규칙', () => {
    const result = (source) => { const { output, state } = run(source); return [output, state.afterHeading]; };
    assert.deepEqual(result('</h2>'), ['~*~N~.\n\n', 1]); assert.deepEqual(result('</h2> '), ['~*~N~.\n \n', 1]); assert.deepEqual(result('</h2> x'), ['~*~N~.\n x\n', 0]); assert.deepEqual(result('</h2>x'), ['~*~N~.\nx\n', 0]);
    assert.deepEqual(result('</h2><br>'), ['~*~N~.\n\n\n', 0]); assert.deepEqual(result('</h2><p>'), ['~*~N~.\n~*\n\n', 0]); assert.deepEqual(result('</h2><br><p>'), ['~*~N~.\n\n~*\n \n\n', 0]);
    assert.deepEqual(result('x<p>'), ['x~*\n \n\n', 0]);
});

test('HTML 기본 변환: 소스 줄바꿈은 앞 글자가 공백/LF 가 아닐 때만 공백 하나가 되고 맨 앞 LF 는 버려진다', () => {
    assert.equal(run('\nabc').output, 'abc\n'); assert.equal(run('a\nb').output, 'a b\n'); assert.equal(run('a \nb').output, 'a b\n'); assert.equal(run('a\n\nb').output, 'a b\n'); assert.equal(run('a<br>\nb').output, 'a\nb\n');
    assert.equal(run('a\x01b\x1fc').output, 'abc\n'); assert.equal(run('a b\x20c').output, 'a b c\n');
});

test('HTML 기본 변환: callback 이 입력/출력 위치를 버퍼 밖으로 옮기면 그 지점에서 거부하고 끝 위치까지는 허용한다', () => {
    // afterTag 는 '<b>x' 의 '>'(위치 2)에서 호출된다. 입력 길이 4 는 허용하고 5 는 거부한다.
    assert.equal(run('<b>x', { hooks: { afterTag: (state) => { state.inputOffset = 4; } } }).output, '~N~B\n');
    assert.throws(() => run('<b>x', { hooks: { afterTag: (state) => { state.inputOffset = 5; } } }), /현재 버퍼 위치 오류/);
    // 출력 길이가 용량과 같아지면 위치 오류이고 하나 모자라면 이 검사를 통과한다(이후 마지막 LF 의 물리 초과는 별도 문구다).
    assert.throws(() => run('<b>x', { capacity: 600, hooks: { afterTag: (state) => { state.output = 'y'.repeat(600); } } }), /현재 버퍼 위치 오류/);
    assert.throws(() => run('<b>x', { capacity: 600, hooks: { afterTag: (state) => { state.output = 'y'.repeat(599); } } }), /물리 출력 버퍼 초과/);
    // 닫힘 직전/평문 순회 끝/각 태그 효과 뒤의 같은 검사다.
    assert.throws(() => run('<b></b>', { hooks: { beforeClose: (state) => { state.inputOffset = 8; } } }), /현재 버퍼 위치 오류/);
    assert.throws(() => run('<b>x', { hooks: { insideTag: (state) => { state.inputOffset = 9; } } }), /현재 버퍼 위치 오류/);
    assert.throws(() => run('<$c>', { hooks: { command: (state) => { state.inputOffset = 9; } } }), /현재 버퍼 위치 오류/);
    assert.throws(() => run('<?c>', { hooks: { condition: (state) => { state.inputOffset = 9; } } }), /현재 버퍼 위치 오류/);
    // 조건 효과가 숨김으로 바꾼 뒤의 위치 이동도 같은 곳에서 검사된다.
    assert.equal(run('a<$c>b', { hooks: { command: (state) => { state.inputOffset = 6; } } }).output, 'a\n');
});

test('HTML 기본 변환: 훅 표는 생성 때 값복사하고 조건/닫힘/태그 안 slot 은 선택된 분기에서만 호출한다', () => {
    const calls = []; const hooks = { command: () => calls.push('a') }; const parser = new HtmlTextParser(createHtmlTextParserState(), createHtmlTextParserGlobals(), hooks);
    hooks.command = () => calls.push('b'); parser.parse('<$x>'); delete hooks.command; parser.parse('<$y>'); assert.deepEqual(calls, ['a', 'a']);
    // 숨김 상태에서는 일반 태그의 slot 을 부르지 않고, 닫힘 조건 태그만 숨김 깊이가 있을 때 처리된다.
    const seen = []; const slots = { condition: () => seen.push('condition'), beforeClose: () => seen.push('close'), afterTag: () => seen.push('after'), insideTag: () => seen.push('inside') };
    run('<b></b>', { shown: 0, hooks: slots }); assert.deepEqual(seen, ['inside', 'inside', 'inside']);
    const state = createHtmlTextParserState(); state.shown = 0; state.hiddenDepth = 1; seen.length = 0; run('</?x>', { state, hooks: slots }); assert.deepEqual(seen, ['close', 'condition', 'inside', 'inside']);
    const second = createHtmlTextParserState(); second.shown = 0; second.hiddenDepth = 1; seen.length = 0; run('</b>', { state: second, hooks: slots }); assert.deepEqual(seen, ['inside', 'inside']);
});

test('HTML 기본 변환: a 링크 열림/닫힘이 평문 허용 카운터를 올리고 내리며 숨김 깊이 안에서는 올리지 않는다', () => {
    const links = []; const hooks = { link: (state, raw, closing) => { links.push([raw, closing]); return ''; } };
    const { state } = run('<a href="x">t</a>', { hooks }); assert.deepEqual(links, [['a href="x">t</a>', false], ['', true]]); assert.equal(state.textAllowed, 1);
    const hidden = createHtmlTextParserState(); hidden.hiddenDepth = 1; run('<a>', { state: hidden, hooks }); assert.equal(hidden.textAllowed, 1);
    // 카운터가 1 미만이면 평문을 더 처리하지 않는다(음수는 signed 비교).
    const below = createHtmlTextParserState(); below.shown = 0xffffffff; assert.equal(run('abc', { state: below }).output, '\n'); assert.equal(below.textAllowed, 0xffffffff);
    assert.equal(run('</a>t', { hooks }).output, '~N~.\n'); assert.equal(run('<a>t</a>t', { hooks }).output, 't~N~.t\n');
});

test('HTML 기본 변환: 위치 검사는 태그 효과 전/출력 추가 전에 일어나고 숨김 깊이의 닫힘 조건만 숨김 상태에서 처리된다', () => {
    // beforeClose 가 위치를 밖으로 옮기면 뒤따르는 조건 효과를 부르기 전에 거부한다.
    let called = 0; assert.throws(() => run('</?x>', { hooks: { beforeClose: (state) => { state.inputOffset = 99; }, condition: () => { called += 1; } } }), /현재 버퍼 위치 오류/); assert.equal(called, 0);
    // 링크 효과가 위치를 밖으로 옮기면 태그 출력(~N~.)을 추가하기 전에 거부한다.
    const state = createHtmlTextParserState(); assert.throws(() => run('</a>', { state, hooks: { link: (current) => { current.inputOffset = 99; return ''; } } }), /현재 버퍼 위치 오류/); assert.equal(state.output, '');
    // 숨김(shown=0)에서는 깊이가 0 이면 닫힘 조건도, 깊이가 있어도 열림/일반 태그도 처리하지 않는다.
    const seen = []; const hooks = { condition: (current, raw, closing) => seen.push([raw, closing]) };
    assert.equal(run('</?x>', { shown: 0, hooks }).output, '\n'); assert.deepEqual(seen, []);
    const deep = createHtmlTextParserState(); deep.shown = 0; deep.hiddenDepth = 1; assert.equal(run('<b?>x', { state: deep, hooks }).output, '\n'); assert.deepEqual(seen, []);
    const closing = createHtmlTextParserState(); closing.shown = 0; closing.hiddenDepth = 1; assert.equal(run('</?x>', { state: closing, hooks }).output, '\n'); assert.deepEqual(seen, [['?x>', true]]);
    // 변환 시작은 지난 afterHeading 을 지우고 int32 용량 인자는 32비트로 감긴다.
    const heading = createHtmlTextParserState(); heading.afterHeading = 1; assert.equal(run('<p>', { state: heading }).output, '~*\n \n\n');
    assert.throws(() => run('<b>', { capacity: 2 ** 32 + 4 }), /물리 출력 버퍼 초과/); assert.equal(run('<b>', { capacity: 2 ** 32 + 6 }).output, '~N~B\n');
});
