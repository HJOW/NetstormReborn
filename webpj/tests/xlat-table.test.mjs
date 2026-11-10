// 번역표(cpppj Xlat.cpp XlatTable, 원본 004de260 읽기·004de1e0 조회·004de9a0 번역 함수)의 웹 대응 `assets/xlat.js` 를 검사한다(4-A Xlat, WB04-E270).
//  - 형식: CR 제거, 줄의 첫 글자만 구분(`*` 원문 시작, `-` 번역 시작, `=` 끝), 원문·번역문 끝의 줄바꿈 하나만 뗌, 끝나지 않은 마지막 블록은 버림, 중복 원문은 덮어씀.
//  - 조회: 대소문자 구분 완전 일치, 번역이 없으면 원문, 언어 0·1 은 표를 보지 않고 원문.
// 10.78 일곱 파일 버퍼·언어별 장부(RawXlat)는 xlat-runtime.test.mjs(E242). 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E270)
import test from 'node:test';
import assert from 'node:assert/strict';
import { XlatTable } from '../www/assets/xlat.js';

test('XlatTable: CR 을 모두 지우고 줄의 첫 글자만 구분 문자로 본다', () => {
    const table = new XlatTable('*\r\nKey\r\n-\r\nValue\r\n=\r\n');
    assert.equal(table.translate('Key'), 'Value');
    // 줄 앞에 공백이 있거나 구분 문자가 줄 중간에 있으면 구분이 아니다.
    const indented = new XlatTable(' *\nKey\n-\nValue\n=\n');
    assert.equal(indented.size, 0);
    const middle = new XlatTable('x*\nKey\n-\nValue\n=\n');
    assert.equal(middle.size, 0);
});

test('XlatTable: 끝나지 않은 마지막 블록은 버리고 LF 없는 마지막 구분 줄은 처리한다', () => {
    assert.equal(new XlatTable('*\nA\n-\nB').size, 0);
    assert.equal(new XlatTable('*\nA\n-').size, 0);
    // '=' 줄이 LF 없이 끝나거나 뒤에 글자가 붙어도 완성된 블록이다.
    assert.equal(new XlatTable('*\nKey\n-\nValue\n=').translate('Key'), 'Value');
    assert.equal(new XlatTable('*\nKey\n-\nValue\n=tail').translate('Key'), 'Value');
    // 구분 문자가 순서대로 나오지 않으면 무시한다('-' 가 먼저 나오면 상태 0 에서는 아무것도 하지 않는다).
    assert.equal(new XlatTable('-\nX\n=\n*\nA\n-\nB\n=\n').translate('A'), 'B');
});

test('XlatTable: 원문·번역문 끝의 줄바꿈은 하나만 떼고 중복 원문은 덮어쓴다', () => {
    const table = new XlatTable('*\nKey\n\n-\nValue\n\n=\n*\nOther\n-\nOne\n=\n*\nOther\n-\nTwo\n=\n');
    // 끝의 줄바꿈 하나만 떼므로 빈 줄 하나가 글자로 남는다.
    assert.equal(table.translate('Key\n'), 'Value\n');
    assert.equal(table.has('Key'), false);
    // 같은 원문은 뒤의 번역문이 이기고 고유 원문 수만 센다.
    assert.equal(table.translate('Other'), 'Two');
    assert.equal(table.size, 2);
    // 여러 줄 원문·번역문은 줄바꿈을 그대로 보존한다.
    assert.equal(new XlatTable('*\nA\nB\n-\nC\nD\n=\n').translate('A\nB'), 'C\nD');
});

test('XlatTable: 조회는 대소문자를 구분하고 번역이 없으면 원문이며 언어 0·1 은 표를 보지 않는다', () => {
    const table = new XlatTable('*\nKey\n-\nValue\n=\n');
    assert.equal(table.translate('key'), 'key');
    assert.equal(table.translate('Key'), 'Value');
    assert.equal(table.translate('Missing'), 'Missing');
    // 언어 번호를 생략하면 우회하지 않고, 0·1 은 우회하며, 그 밖의 번호는 표를 쓴다.
    assert.equal(table.translate('Key', undefined), 'Value');
    assert.equal(table.translate('Key', 0), 'Key');
    assert.equal(table.translate('Key', 1), 'Key');
    assert.equal(table.translate('Key', 2), 'Value');
    assert.equal(table.translate('Key', 3), 'Value');
    assert.equal(new XlatTable(undefined).size, 0);
    assert.equal(new XlatTable('').translate('x'), 'x');
});

// 블록 안에서 다시 나오는 '*' 줄은 구분이 아니고(상태 1 에서는 '-' 만 구분), 소문자 키는 대문자 조회와 다른 키이다(변형 확인으로 추가).
test('XlatTable: 원문 안의 * 줄은 구분이 아니고 소문자 키는 대문자 조회에 걸리지 않는다', () => {
    const nested = new XlatTable('*\nA\n*\nB\n-\nC\n=\n');
    assert.equal(nested.translate('A\n*\nB'), 'C');
    assert.equal(nested.size, 1);
    const lower = new XlatTable('*\nkey\n-\nv\n=\n');
    assert.equal(lower.translate('key'), 'v');
    assert.equal(lower.translate('KEY'), 'KEY');
});
