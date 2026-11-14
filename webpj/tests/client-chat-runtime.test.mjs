// ClientChatRuntimeTests/ChatWindowClientTests의 창 없는 장기 owner 조립과 원본 Identity48행을 검증한다. 실제 메뉴 픽셀/Console입력 조립은 RawMenuControls에 남는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { runtimeScene, bin, name, put, word, z } from './client-chat-runtime-support.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { GameAssets } from '../www/assets/game-assets.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { RawPlayerIdentity } from '../www/core/raw-player-identity.js';
import { ChatManagerDivideError } from '../www/core/raw-chat-manager-prefix.js';
import { checkChatRecipients } from '../www/ui/chat-recipients.js';
import { createRawChatWindowContext } from '../www/ui/raw-chat-window-context.js';
import { makeClientChatPacketHooks } from '../www/core/client-chat-manager-globals.js';
import { relayChatPacket } from '../www/ui/raw-chat-relay.js';
import { dispatchChatPacket } from '../www/ui/raw-chat-receive.js';
import { observeChatPacket } from '../www/ui/raw-chat-packet.js';
import { applyClientChatFormat, bindClientChatMessage, bindClientChatRecipients, bindClientChatSubmit, findClientChatSenderOwner, initializeClientChatManager, initializeClientIssueCommands, loadClientSystemNewsTime, makeClientChatMessageHooks, refreshClientChatRankPrefix, resolveClientChatIdentity, saveClientSystemNewsTime } from '../www/core/client-chat-runtime.js';

test('Chat 문맥: 실제 owner 참조를 공유하고 editor 값 구조체의 필드를 복사한다', () => {
    const editor = { now: () => 1 }; const owner = {}; const context = createRawChatWindowContext({ globals: owner, commands: owner, editor });
    assert.equal(context.globals, owner); assert.equal(context.commands, owner); assert.notEqual(context.editor, editor); assert.equal(context.editor.now, editor.now); context.editor.now = null; assert.equal(editor.now(), 1);
    assert.equal(context.resolve, null); assert.equal(context.message, null); assert.equal(createRawChatWindowContext().editor.complete, null);
});
test('초기화: 실제 명령표·signed 현재ID·두 strncpy와 마지막 강제NUL', () => {
    const r = runtimeScene(); r.state.subscriber = -1; r.config.set('-1chatFormat', 'F'.repeat(70)); r.config.set('-1chatFormat2', 'G'.repeat(520)); initializeClientChatManager(r.client);
    const manager = r.manager.bytes(r.manager.current().value); const table = r.commands.bytes();
    assert.equal(word(manager, 4), 32); assert.ok(word(manager, 8)); assert.equal(manager[0x244], 0); assert.equal(z(manager, 0x14), 'F'.repeat(59)); assert.equal(z(manager, 0x50), 'G'.repeat(499));
    assert.equal(word(table, 0xa04), 4); assert.deepEqual([z(table, 32), z(table, 64), z(table, 96)], ['format', 'back', 'front']); assert.deepEqual([word(table, 0x804), word(table, 0x808), word(table, 0x80c)], [0x42ddf0, 0x42ddf0, 0x42def0]);
});
test('초기화: UTF16 Config의1252 표시 바이트와 누락한 서식 전체 padding', () => {
    const r = runtimeScene(); r.config.set('100chatFormat', 'é'); initializeClientChatManager(r.client); const manager = r.manager.bytes(r.manager.current().value);
    assert.equal(manager[0x14], 0xe9); assert.equal(manager[0x15], 0); assert.ok(manager.subarray(0x50, 0x244).every((byte) => byte === 0));
});
test('재초기화: 실제 이전 manager/캡처 주소와 새 현재ID·추가 등록을 보존한다', () => {
    const r = runtimeScene(); initializeClientChatManager(r.client); const previous = r.manager.current().value; const memory = r.manager.bytes(previous); name(memory, 0x244, 'P'); const capture = r.manager.formatCapture();
    r.state.subscriber = 7; r.config.set('7chatFormat', 'Now'); initializeClientChatManager(r.client);
    assert.notEqual(r.manager.current().value, previous); assert.equal(r.manager.bytes(previous), memory); assert.equal(z(memory, 0x244), 'P'); assert.equal(r.manager.formatCapture(), capture);
    assert.equal(z(r.manager.bytes(r.manager.current().value), 0x14), 'Now'); assert.equal(word(r.commands.bytes(), 0xa04), 7);
});
test('초기화는 두 Config 키를 쓰는 각 위치에서 현재 subscriber를 다시 읽는다', () => {
    const r = runtimeScene(); const original = r.client.configuration; const keys = []; r.client.configuration = () => ({ get: (key) => { keys.push(key); r.state.subscriber = 7; return ''; } });
    initializeClientChatManager(r.client); assert.deepEqual(keys, ['100chatFormat', '7chatFormat2']); r.client.configuration = original;
});
test('표시 codec 미연결/지원 밖 한국어는 손실 변환하지 않고 앞선 실제 할당·등록 뒤 실패한다', () => {
    const r = runtimeScene(); r.config.set('100chatFormat', '한글'); assert.throws(() => initializeClientChatManager(r.client), /Windows-1252/); assert.ok(r.manager.current().value); assert.equal(word(r.commands.bytes(), 0xa04), 4);
    r.client.encodeDisplayText = null; assert.throws(() => initializeClientChatManager(r.client), /미연결/); r.client.encodeDisplayText = () => '잘못된 반환'; assert.throws(() => initializeClientChatManager(r.client), /BYTE 배열/);
});
test('랭크 자연 관문: manager0/active0에서 Core·TYPE를 선행 조회하지 않는다', () => {
    const r = runtimeScene(); r.core.active = 1; refreshClientChatRankPrefix(r.client, null, null); assert.equal(r.core.active, 1);
    initializeClientChatManager(r.client); r.core.active = 0; name(r.manager.bytes(r.manager.current().value), 0x244, 'K'); refreshClientChatRankPrefix(r.client, null, null); assert.equal(z(r.manager.bytes(r.manager.current().value), 0x244), 'K'); assert.equal(r.core.current, null);
});
test('랭크: 실제 보호icon B10/C14와 현재Core 캐시, TYPE교체·나눗셈fault', () => {
    const r = runtimeScene(); initializeClientChatManager(r.client); const { table, database } = realCaptureAssets(); const assets = new GameAssets(table, database); const kind = assets.find('icon').block + 70;
    r.surface.ownerMode.challenge = true; r.surface.frames[kind] = FrameCodeTable.fromDefinition(assets.find('icon').definition); r.core.select(r.gameplay.player, 1, 1, 1); r.core.active = 1; r.core.cached()[4] = 255;
    const selected = r.core.current; refreshClientChatRankPrefix(r.client, assets, r.surface); assert.equal(z(r.manager.bytes(r.manager.current().value), 0x244), '~[Iicon.C12]~[Iicon.B9]'); assert.equal(r.core.current, selected);
    r.core.cached()[4] = 15; refreshClientChatRankPrefix(r.client, assets, r.surface); assert.equal(z(r.manager.bytes(r.manager.current().value), 0x244), '~[Iicon.C0]~[Iicon.B5]');
    r.surface.frames[kind] = new FrameCodeTable([66, 66, 66, 67, 67].map((side) => ({ side, variant: 0, flags: 0 }))); refreshClientChatRankPrefix(r.client, assets, r.surface); assert.equal(z(r.manager.bytes(r.manager.current().value), 0x244), '~[Iicon.C0]~[Iicon.B2]');
    r.surface.frames[kind] = new FrameCodeTable([{ side: 67, variant: 0, flags: 0 }]); assert.throws(() => refreshClientChatRankPrefix(r.client, assets, r.surface), ChatManagerDivideError); assert.equal(z(r.manager.bytes(r.manager.current().value), 0x244), '~[Iicon.C0]~[Iicon.B2]');
});
test('서식: 공통80BYTE캡처·back59BYTE·front·현재ID와 Config1252해독·실패 보존', () => {
    const r = runtimeScene(); initializeClientChatManager(r.client); assert.equal(applyClientChatFormat(r.client, false, '~0~r' + 'X'.repeat(70)), 0);
    assert.equal(r.config.get('100chatFormat'), '~r' + 'X'.repeat(70)); assert.equal(z(r.manager.bytes(r.manager.current().value), 0x14), '~r' + 'X'.repeat(57));
    applyClientChatFormat(r.client, true, 'prefix~rCaf\xe9 suffix'); assert.equal(r.config.get('100chatFormat2'), '~rCafé');
    const manager = r.manager.bytes(r.manager.current().value).slice(); const capture = r.manager.formatCapture().slice(); applyClientChatFormat(r.client, false, 'plain');
    assert.deepEqual(r.manager.bytes(r.manager.current().value), manager); assert.deepEqual(r.manager.formatCapture(), capture); assert.deepEqual(r.trace, [['log', 29, 'Illegal format "plain"\n']]);
});
test('실제 대상Identity 원본48행의 반환·전체60BYTE·외부 호출 순서', () => {
    const rows = fs.readFileSync(new URL('../../cpppj/tests/fixtures/chat-recipient-identity-x86.tsv', import.meta.url), 'utf8').trim().split(/\r?\n/).slice(1); assert.equal(rows.length, 48);
    // 모든 원본 입력은 새 실제 owner와 같은 물리에 공급하며 기대값은 마지막 비교에서만 읽는다.
    for (const line of rows) {
        const [scene, source, result, expected, events] = line.split('\t'); const r = runtimeScene(); r.surface.ownerMode.battle = scene === '1'; r.gameplay.world.editor = Number(scene === '2');
        const context = r.context(); bindClientChatRecipients(context, r.client, r.surface, () => r.state.currentName); const scratch = new Uint8Array(60).fill(0xa5); const trace = [];
        const hex = (value) => Buffer.from(value, 'latin1').toString('hex'); const input = source === '-' ? '' : Buffer.from(source, 'hex').toString('latin1');
        assert.equal(checkChatRecipients(1, input, scratch, { resolve: (record) => { trace.push(`R:${hex(z(record, 8))}`); return context.resolve(record); }, translate: (key) => { trace.push(`T:${hex(key)}`); return key; } }), result === '1', line);
        assert.equal(Buffer.from(scratch).toString('hex'), expected, line); assert.equal(trace.join(';') || '-', events, line);
        r.manager.current().value = 1; assert.equal(context.recipient(input), result === '1', line);
    }
});
test('대상 문맥: 현재이름·주소·Playerbase 변경과 현재 manager0을 즉시 읽는다', () => {
    const r = runtimeScene(); const context = r.context(); bindClientChatRecipients(context, r.client, r.surface, () => r.state.currentName); r.manager.current().value = 1;
    assert.equal(context.recipient('LocalUser ALLIES Bob'), true); assert.equal(context.editor.recipient, context.recipient); assert.equal(context.recipient('Alice'), false);
    r.state.currentName = 'Renamed'; assert.equal(context.recipient('Renamed'), true); assert.equal(context.recipient('LocalUser'), false); r.state.address = 0x81234567;
    const record = new Uint8Array(60); RawPlayerIdentity.initialize(record, 0); put(record, 48, r.state.address); assert.equal(context.resolve(record), true); assert.equal(z(record, 8), 'Renamed');
    const previous = r.core.players; r.core.players = previous.slice(); name(r.core.players, 372, 'Changed'); put(r.core.players, 404, 222); r.gameplay.world.editor = 1; assert.equal(context.recipient('Changed'), true); assert.equal(context.recipient('Alice'), false);
    r.manager.current().value = 0; assert.equal(context.recipient('Renamed'), false);
});
test('자동완성: 새 질의만 현재Chat100행을 읽고NULL순환은 장기BSS를 유지한다', () => {
    const r = runtimeScene(); const context = r.context(); bindClientChatRecipients(context, r.client, r.surface, () => r.state.currentName);
    assert.equal(context.editor.complete(0x42e540, 'Bo', 2, false), 'Bob'); r.state.records = new Uint8Array(4800); name(r.state.records, 0, 'Bobby'); put(r.state.records, 44, 500);
    assert.equal(context.editor.complete(0x42e540, null, 2, false), 'Bob'); assert.equal(context.editor.complete(0x42e540, 'Bo', 2, false), 'Bobby');
    assert.throws(() => context.editor.complete(1, '', 0, false), /미지원/);
});
test('진단: 원문 한 서식과 signed 수준·subscriber를 공유로그에 전달한다', () => {
    const r = runtimeScene(); const context = r.context(); bindClientChatRecipients(context, r.client, r.surface, () => r.state.currentName);
    context.diagnostic(0xffffffff, 'Could not lookup a name for subId == %d\n', 0x80000000); assert.deepEqual(r.trace, [['log', -1, 'Could not lookup a name for subId == -2147483648\n']]); assert.throws(() => context.diagnostic(1, '%d', 1), /미지원/);
});
test('Identity 메타 제외: 로컬 일치는 CS없이 성공하고 미선택 메타표를 선행 조회하지 않는다', () => {
    const r = runtimeScene(); r.surface.ownerMode.challenge = true; const record = new Uint8Array(60); RawPlayerIdentity.initialize(record, 0); put(record, 4, 100);
    assert.equal(resolveClientChatIdentity(r.client, r.surface, 'LocalUser', record), true); RawPlayerIdentity.initialize(record, 0); put(record, 4, 999); assert.throws(() => resolveClientChatIdentity(r.client, r.surface, 'LocalUser', record), /metamap.*미연결/);
});
test('이름 getter: 실패해도SOMEONE을쓰고포인터별칭1이며 현재소유자는flags2로조회한다', () => {
    const r = runtimeScene(); r.surface.ownerMode.battle = true; const hooks = makeClientChatMessageHooks(r.client, r.surface, () => r.state.currentName); const output = new Uint8Array(256).fill(0xa5);
    assert.equal(hooks.nameForSubscriber(output, 200), 1); assert.equal(z(output), 'Alice'); assert.equal(output[6], 0xa5); assert.equal(findClientChatSenderOwner(r.client, r.surface, 'LocalUser', 200), 2); assert.equal(findClientChatSenderOwner(r.client, r.surface, 'LocalUser', 999), 0);
    r.surface.ownerMode.battle = false; assert.equal(hooks.nameForSubscriber(output, 200), 1); assert.equal(z(output), 'SOMEONE'); assert.throws(() => hooks.nameForSubscriber(new Uint8Array(7), 999), RangeError);
});
test('초기 issue등록과 제출은 실제장기cheat를 쓰며Config·모드를변경하지않는다', () => {
    const r = runtimeScene(); initializeClientIssueCommands(r.client); const context = r.submit(); r.gameplay.energy.userCheat = 17; r.config.set('userCheat', '17');
    context.sendChat(r.manager.current().value, 'EVERYONE:.cheatorama wrong'); assert.equal(r.gameplay.energy.userCheat, 17); context.sendChat(r.manager.current().value, 'EVERYONE:.cheatorama "8675309"'); assert.equal(r.gameplay.energy.userCheat, 0x845fed); assert.equal(r.config.get('userCheat'), '17'); assert.equal(r.gameplay.world.editor, 0);
    assert.equal(word(r.commands.bytes(), 0xa04), 8); assert.equal(z(r.commands.bytes(), 32), 'flash'); assert.equal(z(r.commands.bytes(), 64), 'flash');
});
test('제출 명령: flash→소리·서식·clear/quit와 소문자hex 오류가 같은 현재장치를쓴다', () => {
    const r = runtimeScene(); initializeClientIssueCommands(r.client); r.commands.register(bin('clear'), 0x42f680, 1); r.commands.register(bin('quit'), 0x42e520, 1); const context = r.submit(() => 0xfeed);
    context.sendChat(r.manager.current().value, 'EVERYONE:.flash -17;.front ~g;.clear;.quit');
    assert.deepEqual(r.trace, [['flash', -17], ['sound', 'thunderQuietDistant.wav', 0, 0, 0, 0, 0], ['command', 0x42f680], ['command', 0x42e520]]); assert.equal(r.config.get('100chatFormat2'), '~g');
    context.sendChat(r.manager.current().value, 'EVERYONE:ordinary'); assert.deepEqual(r.trace.at(-1), ['print', 'Error in command ".say": code == feed\n']);
});
test('제출은 bind때resolve/번역/message를캡처하고 현재창0에서표시장치를조회하지않는다', () => {
    const r = runtimeScene(); const delivered = []; const context = r.submit((input) => { delivered.push(z(input)); return 0; }); context.resolve = () => { throw new Error('교체됨'); }; context.translate = context.resolve; context.message = context.resolve;
    context.sendChat(r.manager.current().value, 'EVERYONE:hello'); assert.deepEqual(delivered, ['hello']); r.menus.chatGeometryBackend = () => { throw new Error('미선택'); }; context.sendChat(r.manager.current().value, 'EVERYONE:');
    r.chat.window.current = 99; r.menus.chatGeometryBackend = () => ({ setInputVisible: (token, visible) => r.trace.push([token, visible]) }); context.sendChat(r.manager.current().value, 'EVERYONE:'); assert.deepEqual(r.trace, [[99, 0]]);
});
test('미연결 명령/desc/일반본문/표시 장치를 선택한 위치에서 명시적으로 거부한다', () => {
    const r = runtimeScene(); initializeClientIssueCommands(r.client); r.commands.register(bin('unknown'), 0x123456, 1); const context = r.submit(); const submit = (input) => context.sendChat(r.manager.current().value, `EVERYONE:${input}`);
    assert.throws(() => submit('hello'), /미연결/); assert.throws(() => submit('.desc hi'), /선택 장치 미연결/); assert.throws(() => submit('.unknown x'), /callback 미연결/);
    r.chat.window.current = 99; r.menus.chatGeometryBackend = () => null; assert.throws(() => submit(''), /표시 장치 없음/);
    assert.throws(() => bindClientChatRecipients(r.context(), r.client, null, () => ''), TypeError); assert.throws(() => bindClientChatSubmit(r.context(), r.client, r.menus), TypeError); assert.throws(() => makeClientChatMessageHooks(r.client, null, () => ''), TypeError);
});
test('서버뉴스Runtime: 같은뉴스키·시각·Config해석기와 현재표시codec을 공유한다', () => {
    const r = runtimeScene(); r.config.set('ns_example_orgNewsTime', '-1'); loadClientSystemNewsTime(r.client); assert.equal(r.chat.newsTime, 0xffffffff); assert.equal(z(r.chat.newsKey), 'ns_example_orgNewsTime');
    r.chat.newsTime = 0x80000000; saveClientSystemNewsTime(r.client); assert.equal(r.config.get('ns_example_orgNewsTime'), '-2147483648'); r.state.server = 'café'; saveClientSystemNewsTime(r.client); assert.equal(r.config.get('caféNewsTime'), '-2147483648');
});
test('제출→실제본문/패킷→로컬중계→수신필터에서같은owner와무시목록을쓴다', () => {
    const r = runtimeScene(); r.surface.ownerMode.battle = true; initializeClientChatManager(r.client); const context = r.context(); bindClientChatRecipients(context, r.client, r.surface, () => r.state.currentName); r.chat.window.current = 77;
    const queue = []; const logs = []; const packet = makeClientChatPacketHooks(r.manager); packet.currentName = () => bin(r.state.currentName);
    packet.read = (at) => at === 0x540c74 ? r.state.subscriber : at === 0x594fbc ? 1 : at === 0x52edf6 ? 0x41 : 0;
    packet.send = (channel, bytes) => { assert.equal(channel, 9); observeChatPacket(bytes, 1, null); queue.push(bytes.slice()); };
    bindClientChatMessage(context, r.client, r.surface, () => r.state.currentName, packet); packet.send = () => { throw new Error('값 캡처 깨짐'); }; bindClientChatSubmit(context, r.client, r.menus);
    const receive = {
        read: (at) => { if (at === 0x548740) return r.chat.window.current; if (at === 0x548710) return r.chat.receivedMask; if (at === 0x540c74) return r.state.subscriber; if (at === 0x594fbc || at === 0x540c70) return 1; if (at === 0x5b5dd8) return 44; if (at >= 0x595200 && at < 0x595200 + 324) return 1; return 0; },
        ignored: (value) => r.chat.ignored.includes(value), config: () => 1, owner: (value) => findClientChatSenderOwner(r.client, r.surface, r.state.currentName, value),
        wake: () => {}, append: (root, value, style) => { assert.equal(root, 77); assert.deepEqual(style, [0, 5, 44, 0, 0, 0, 1, 1]); logs.push(value); }, show: () => {},
    };
    // 명시 큐 경계는 Poll 전 무표시를 검사하며 실제 transport/Console입력은 해당 허브에서 조립한다.
    const poll = () => { const bytes = queue.shift(); relayChatPacket(9, bytes, { get value() { return r.chat.receivedMask; }, set value(value) { r.chat.receivedMask = value; } }, { address: () => new Uint8Array(20), read: (at) => at === 0x540c74 ? 100 : 1, receive: (bytes) => dispatchChatPacket(bytes, receive) }); };
    // 현행 C++ 송신은 전투 본문 NUL을 길이에서 빼고 중계가 마지막 BYTE를 NUL로 바꾼다. 이 결합 효과를 임의로 고치지 않는다(전용 PE 판정은 없음).
    context.sendChat(r.manager.current().value, 'hello'); assert.equal(logs.length, 0); assert.equal(queue.length, 1); poll(); assert.deepEqual(logs, ['~[Iicon.D41]LocalUser> hell\n']); assert.equal(r.chat.receivedMask, 32);
    r.chat.ignored.push(100); context.sendChat(r.manager.current().value, 'hidden'); poll(); assert.equal(logs.length, 1); r.chat.ignored.length = 0; context.sendChat(r.manager.current().value, '!waves'); poll(); assert.equal(logs.at(-1), '~[Iicon.D41]LocalUser wave\n');
});
