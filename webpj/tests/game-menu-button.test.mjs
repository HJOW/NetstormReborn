// 웹 전용 게임 메뉴 버튼의 배치와 논리 폭(logicalInset) 계산을 가짜 DOM 으로 검사한다. 상단 경고 안내가 버튼 뒤로 가려지지 않게 비우는 폭이다. (2026-11-20 WB04-E1097)
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameMenuButton } from '../www/ui/game-menu-button.js';

// 버튼 요소 하나와 루트를 흉내 낸다.
function scene(width = 80) {
    const button = { style: {}, attrs: {}, addEventListener() {}, removeEventListener() {}, setAttribute(name, value) { this.attrs[name] = value; }, remove() { this.removed = true; }, getBoundingClientRect: () => ({ width }) };
    const root = { ownerDocument: { createElement: () => button }, appendChild() {}, clientWidth: 1200 };
    return { button, root, instance: new GameMenuButton(root, { label: 'Menu', title: '메뉴', onClick() {} }) };
}

test('논리 폭: 배치 전·숨김·크기 모름은 0 이고 보이면 (폭+간격 14) 를 논리 픽셀로 환산해 올린다', () => {
    const { instance } = scene();
    assert.equal(instance.logicalInset(1024), 0);
    instance.layout({ left: 100, top: 20, cssWidth: 1000, cssHeight: 750 });
    assert.equal(instance.logicalInset(1024), 0, '아직 숨김');
    instance.show();
    // (80 + 8 + 6) * 1024 / 1000 = 96.256 → 97
    assert.equal(instance.logicalInset(1024), 97);
    // 화면이 CSS 로 절반 크기면 같은 버튼이 논리 픽셀로는 두 배다.
    instance.layout({ left: 0, top: 0, cssWidth: 512, cssHeight: 384 }); assert.equal(instance.logicalInset(1024), 188);
    instance.hide(); assert.equal(instance.logicalInset(1024), 0);
    assert.equal(scene(0).instance.logicalInset(1024), 0);
});

test('배치: 버튼은 Canvas 오른쪽 위 모서리에서 8픽셀 안쪽에 놓인다', () => {
    const { button, instance } = scene();
    instance.layout({ left: 100, top: 20, cssWidth: 1000, cssHeight: 750 }); instance.show();
    assert.equal(button.style.top, '28px'); assert.equal(button.style.right, `${1200 - 1100 + 8}px`); assert.equal(button.style.display, 'block');
    instance.destroy(); assert.equal(button.removed, true);
});
