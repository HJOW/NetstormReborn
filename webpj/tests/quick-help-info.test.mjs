// 원본 10.78의 전용 도움말 마나 그림 관측과 입력 한계를 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { manaPictureText } from '../www/ui/quick-help-info.js';

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
