// HTML 장기owner와 원시문맥의 실제 초기값/값복사·참조수명을 검사한다. (2026-11-14 WB04-E456)
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientHtmlWindowGlobals } from '../www/core/client-html-window-globals.js';
import { bindHtmlBackground, htmlBackgroundName } from '../www/ui/html-text-parser.js';
import { createRawHtmlFactoryServices, createRawHtmlMetadataServices, createRawHtmlWindowContext } from '../www/ui/raw-html-window-context.js';

test('HTML 장기owner: 실제PE초기값과 같은window/Tell256BYTE/80BYTE배경 파서전역을 공유한다', () => {
    const owner = new ClientHtmlWindowGlobals(); const { window, tell, text } = owner;
    assert.deepEqual([window.instances, window.createdSerial, window.transientSerial, window.lastCloseTime, window.kind, window.enterEvent, window.clickEvent, window.tagEvent, window.closedEvent], [0, 0, 0, 0, 27, 0x02000038, 0x0200002c, 0x02000025, 0x020000be]);
    assert.equal(tell.current.length, 256); assert.ok(tell.current.every((byte) => byte === 0)); assert.deepEqual(text, { suppressParagraph: 0, background: new Uint8Array(80) }); assert.equal(htmlBackgroundName(text), '');
    window.instances = 7; tell.current[3] = 0xa5; const hooks = {}; bindHtmlBackground(hooks, text); hooks.background({}, 'clouds'); assert.equal(htmlBackgroundName(owner.text), 'clouds'); text.suppressParagraph = 1; assert.equal(owner.window, window); assert.equal(owner.tell, tell); assert.equal(owner.text, text); assert.equal(owner.tell.current[3], 0xa5);
    const next = new ClientHtmlWindowGlobals(); assert.notEqual(next.window, window); assert.equal(next.window.instances, 0); assert.equal(next.tell.current[3], 0); assert.equal(htmlBackgroundName(next.text), ''); assert.notEqual(next.text.background, text.background); assert.equal(next.text.suppressParagraph, 0);
});
test('HTML 문맥: 장기owner/목록/고리는 공유하고서비스/명령/HTML콜백표는값복사한다', () => {
    const owner = new ClientHtmlWindowGlobals(); const ring = {}; const callback = () => 1; const metadata = { strings: ring, commands: { lookup: callback } }; const factory = { globals: owner.text, html: { command: callback } };
    const context = createRawHtmlWindowContext({ globals: owner, metadata, factory, wallSeconds: callback });
    assert.equal(context.globals, owner); assert.equal(context.wallSeconds, callback); assert.equal(context.metadata.strings, ring); assert.equal(context.factory.globals, owner.text);
    assert.notEqual(context.metadata, metadata); assert.notEqual(context.metadata.commands, metadata.commands); assert.notEqual(context.factory, factory); assert.notEqual(context.factory.html, factory.html);
    metadata.commands.lookup = null; factory.html.command = null; assert.equal(context.metadata.commands.lookup, callback); assert.equal(context.factory.html.command, callback);
    assert.equal(context.read, null); assert.equal(context.metadata.send, null); assert.equal(context.factory.fallback, null); assert.deepEqual(createRawHtmlMetadataServices().commands, {}); assert.deepEqual(createRawHtmlFactoryServices().html, {});
});
