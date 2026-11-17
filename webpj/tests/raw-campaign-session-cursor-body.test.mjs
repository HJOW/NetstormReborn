// 실제 Session의 커서 본체·팔레트 조립을 검사한다. 유닛 생성·포획·희생은 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';

// 실제 선행 설치를 사용하며 저장 미션과 게임은 시작하지 않는다.
function scene(extra = {}, front = true) {
    const o = campaignOwners({ carrierRequest: () => {}, deletion: { selected: () => 0 }, ...extra });
    try {
        o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) });
        o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction(); o.connectBuilding(); o.connectMotion(); o.connectDevastation(); o.connectCarrierDrop(); o.connectPriestPrayer(); o.connectBuilder(); o.connectGameplayPlacement(); if (front) o.connectCursorFront();
        return o;
    } catch (error) { o.dispose(); throw error; }
}

// 실제 같은 내용물·공유 커서 전역과 파일 팔레트를 설치한다.
test('Session 커서 본체: 같은 게임 자료와 파일 팔레트', () => {
    const o = scene();
    try {
        assert.throws(() => o.connectCursorBody(), /순서/); o.connectCursorPlacement(); o.connectCursorBody();
        assert.equal(o.cursorModules.bodyReady, true); assert.equal(o.cursor.contents, o.contained);
        assert.equal(o.cursor.snapToGrid, o.gameplayGlobals.cursorGlobals().snapToGrid());
        assert.equal(o.cursor.placement, o.gameplayGlobals.cursorGlobals().placement()); assert.equal(o.cursor.lifecycle, o.gameplayGlobals.cursorGlobals().lifecycle());
        assert.equal(o.cursor.display.textColors().length, 2); assert.equal(o.loaded, false); assert.throws(() => o.connectCursorBody(), /중복/);
    } finally { o.dispose(); }
    assert.throws(() => o.connectCursorBody(), /반납/);
});

// 원문 전체 설치가 같은 앞단·배치·본체를 순서대로 한 번 만든다.
test('Session 커서 전체 설치: 앞단과 본체의 같은 소유권', () => {
    const o = scene({}, false);
    try { o.connectCursor(); assert.equal(o.cursorModules.frontReady, true); assert.equal(o.cursorModules.placementReady, true); assert.equal(o.cursorModules.bodyReady, true); assert.equal(o.cursor.contents, o.contained); assert.equal(o.loaded, false); assert.throws(() => o.connectCursor(), /중복/); }
    finally { o.dispose(); }
});

// 화면 팔레트와 ColorSort 입력은 함께만 사용하고 부분 실패 뒤 본체를 보존한다.
test('Session 커서 본체: 화면 색 서비스의 현재 입력과 부분 실패', () => {
    const partial = scene({ screenPalette: () => new Uint8Array(1024) });
    try { partial.connectCursorPlacement(); assert.throws(() => partial.connectCursorBody(), /ColorSort/); assert.ok(partial.cursor); assert.equal(partial.cursorModules.bodyReady, false); assert.throws(() => partial.connectCursorBody(), /중복/); }
    finally { partial.dispose(); }
    const palette = new Uint8Array(1024); palette.set([255, 255, 255, 0], 255 * 4); let reads = 0;
    const o = scene({ screenPalette: () => { reads += 1; return palette; }, colorSortInput: { logical: new Uint32Array(256), mapsReady: false } });
    try { o.connectCursorPlacement(); o.connectCursorBody(); assert.equal(reads, 1); assert.equal(o.cursorModules.bodyReady, true); }
    finally { o.dispose(); }
});
