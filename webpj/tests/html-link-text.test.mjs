// 링크 목적지 표(LinkTable)가 원문 HtmlLinkText(0046bfc0, cpppj `client/HtmlLinkText.cpp`)의 인용 검색·복사 규칙과 같은지 확인한다. (2026-11-22 WB04-E1212)
// 원문은 목적지를 저장소 바이트 위치(토큰)로 돌려주고 웹은 1부터 순번으로 돌려준다. 번호 값 자체는 의미가 없으므로 등록된 목적지와 0(링크 아님)만 비교한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { LinkTable } from '../www/ui/html-text.js';

test('HtmlLinkText: 인용 안의 목적지를 대소문자·공백·프로토콜 그대로 보존하고 맨 앞 # 하나만 건너뛴다', () => {
    const links = new LinkTable();
    assert.equal(links.text('a href="#Top">', false), '~[T1]');
    assert.equal(links.text('a href="HTTP://X y">', false), '~[T2]');
    assert.equal(links.text('a href="##x">', false), '~[T3]');
    assert.deepEqual(links.destinations, ['Top', 'HTTP://X y', '#x']);
    assert.equal(links.resolve(2), 'HTTP://X y'); assert.equal(links.resolve(0), null); assert.equal(links.resolve(4), null);
});

test('HtmlLinkText: 닫는 태그·인용 없음·꺾쇠 뒤의 인용은 번호 0(링크 아님)이고 저장소를 쓰지 않는다', () => {
    const links = new LinkTable();
    assert.equal(links.text('a href="x">', true), '~[T0]');
    assert.equal(links.text('a name>', false), '~[T0]');
    assert.equal(links.text('a>x "y"', false), '~[T0]');
    assert.deepEqual(links.destinations, []);
});

test('HtmlLinkText: 목적지는 첫 닫힘 인용·꺾쇠·NUL·끝 앞까지이고 빈 목적지도 번호를 받는다', () => {
    const links = new LinkTable();
    assert.equal(links.text('a href="abc', false), '~[T1]'); // 닫는 인용 없이 끝까지
    assert.equal(links.text('a href="x>y"', false), '~[T2]'); // 인용 안의 꺾쇠에서 끝난다
    assert.equal(links.text('a href="ab\0cd">', false), '~[T3]'); // NUL 뒤는 읽지 않는다
    assert.equal(links.text('a href="">', false), '~[T4]'); // 빈 목적지도 한 칸을 쓴다
    assert.deepEqual(links.destinations, ['abc', 'x', 'ab', '']);
});
