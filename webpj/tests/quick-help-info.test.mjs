// 원본 10.78의 전용 도움말 마나 그림 관측과 입력 한계를 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { manaPictureText, buildQuickHelpInfoHtml } from '../www/ui/quick-help-info.js';
import { createParserState, createParserGlobals, parseHtml } from '../www/ui/html-text.js';
import { renderHelpTopic } from '../www/ui/help-topic.js';

// 보호 실행 파일에서 독립 캡처한 M행의 출력 바이트와 직접 비교한다.
test('도움말 마나 그림: 원본 관측 24행과 일치한다', () => {
    const fixture = fs.readFileSync(new URL('../../cpppj/tests/fixtures/quick-help-html-x86.tsv', import.meta.url), 'utf8');
    const rows = fixture.split(/\r?\n/).filter((line) => line.startsWith('M\t')); assert.equal(rows.length, 24);
    for (const row of rows) {
        const [, input, expected] = row.split('\t'), [mana, highlight] = JSON.parse(input);
        assert.equal(Buffer.from(manaPictureText(mana, highlight), 'latin1').toString('hex'), expected, input);
    }
});

// 명시 전역 표와 NUL·원본 저장 한계가 적용된다.
test('도움말 마나 그림: 현재 표와 경계', () => {
    const frames = new Int32Array(128); frames[65] = 20;
    assert.equal(manaPictureText('A.A\0w', 1, 7, frames), '~[I7.31]~[I7.20]');
    assert.throws(() => manaPictureText('가'), /비ASCII/u);
    assert.throws(() => manaPictureText('w'.repeat(256)), /256/u);
    assert.throws(() => manaPictureText('w'.repeat(40)), /250/u);
});

// 독립 관측의 여덟 TYPE 입력과 스트림 HTML을 비교한다.
test('도움말 TYPE 정보: 원본 관측의 현재 필드와 HTML이 일치한다', () => {
    const fixture = fs.readFileSync(new URL('../../cpppj/tests/fixtures/quick-help-html-x86.tsv', import.meta.url), 'utf8');
    const rows = fixture.split(/\r?\n/).filter((line) => line.startsWith('D\t') && line.includes('<a name=\\"unitType\\"><info>'));
    assert.equal(rows.length, 16);
    for (const row of rows) {
        const [, input, expected] = row.split('\t'), [, , profile] = JSON.parse(input), value = (x) => profile === 8 ? NaN : profile === 5 ? -x : profile === 3 || profile === 4 ? 0 : x;
        const type = { theme: profile % 6, flags1: profile === 7 ? 0 : 0x10, flags2: profile === 3 ? 0x100 : 0,
            classification: profile === 4 ? '' : 'Class Label', mana: profile === 2 || profile === 5 ? 'w.r.t' : '',
            range: value(23.75), damage: value(3.25), cost: value(17.75) };
        const identity = (value) => value, html = buildQuickHelpInfoHtml({ type: () => type, description: () => 'Unit Label', hitPoints: () => profile === 6 ? 0 : profile === 2 ? 25 : 101,
            translate: identity, substitute: identity, config: identity });
        const fields = expected.split('|'), stream = Buffer.from(fields[3], 'hex').toString('latin1'); assert.ok(stream.startsWith(html), input);
        const pane = fields[4].split(';')[0].slice('Pane,'.length);
        assert.equal(Buffer.from(parseHtml(html, createParserState(), createParserGlobals(), {}, 1024), 'latin1').toString('hex'), pane, input);
        const [source, topic] = JSON.parse(input); const result = renderHelpTopic({ helpText: source, topic, substitute: identity, info: () => html });
        assert.equal(Buffer.from(result.information, 'latin1').toString('hex'), pane, input);
    }
});

// 일치하지 않는 주제나 제공하지 않은 TYPE 정보는 새 패널을 만들지 않는다.
test('도움말 TYPE 정보: 선택된 주제와 info 태그만 호출한다', () => {
    let calls = 0; const info = () => { calls += 1; return '<h2>정보</h2>'; }, substitute = (value) => value;
    const helpText = '<a name="first"><info>A</a><a name="second"><info>B</a>';
    const result = renderHelpTopic({ helpText, topic: 'second', substitute, info });
    assert.equal(calls, 1); assert.ok(result.information.includes('정보')); assert.ok(result.styleText.includes('B')); assert.ok(!result.styleText.includes('A'));
    assert.equal(renderHelpTopic({ helpText, topic: 'second', substitute }).information, null);
});

// 정보 본문의 중첩 링크는 부모 주제의 링크 번호를 소비하지 않는다.
test('도움말 TYPE 정보: 중첩 링크 뒤 부모 링크 번호를 보존한다', () => {
    const result = renderHelpTopic({ helpText: '<a name="unit"><info><a href="#outer">Out</a></a>\n', topic: 'unit',
        substitute: (value) => value, info: () => '<h2><a href="#inner">Title</a></h2>' });
    assert.deepEqual(result.links.destinations, ['outer']); assert.equal(result.links.resolve(1), 'outer'); assert.ok(result.information.includes('Title'));
});
