// 채팅 대상 이름 검사(ChatRecipients, 350행)·실제 Identity 접합(48행)·State 의 채팅 기록 전체 정리(RawChatChannels.resetOccupied, 108입력×두 정밀도 216행)·입력줄 전환/Enter(ChatInput 216+162행)를 원본 10.78 독립 관측 fixture 에 대조한다.
// 입력 열만 읽고 관측 열은 비교에만 쓴다(cpppj `ChatRecipientsTests`·`ChatStateClearTests` 와 같은 방식). 원본게임을 실행하지 않는다. (2026-11-15 WB04-E475)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { checkChatRecipients } from '../www/ui/chat-recipients.js';
import { RawChatChannels } from '../www/core/raw-chat-channels.js';
import { RawPlayerIdentity, PLAYER_IDENTITY_RECORD_SIZE } from '../www/core/raw-player-identity.js';
import { GumpCreateNode } from '../www/ui/gump-create.js';
import { HudChatNode, HudChatGeometry, dispatchChatInputKey, consumeChatInputToggle } from '../www/ui/hud-chat-geometry.js';

// 빈 마지막 열도 보존하는 TSV 행(주석/머리글 제외)이다. header 가 true 면 첫 줄을 머리글로 건너뛴다.
function rows(name, skipPrefix = null) {
    const lines = fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0);
    return lines.slice(1).filter((line) => !(skipPrefix && line.startsWith(skipPrefix))).map((line) => line.split('\t'));
}
const hexBytes = (bytes) => Buffer.from(bytes).toString('hex');
const hexText = (text) => Buffer.from(text, 'latin1').toString('hex');
// 레코드의 +8 이름을 첫 NUL 앞까지 바이트 문자열로 읽는다.
function recordName(record) { let end = 8; while (end < record.length && record[end] !== 0) end += 1; return Buffer.from(record.subarray(8, end)).toString('latin1'); }
class RecipientStop extends Error { constructor(where) { super(where); this.where = where; } }

test('ChatRecipients 원본 10.78 350행: 대상 이름 검사의 반환·지역 60바이트·외부 순서(조회/번역)와 현재 번역·선택 중단이 모두 일치한다', () => {
    const table = rows('chat-recipients-x86.tsv'); const failures = []; let stops = 0;
    assert.equal(table.length, 350);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 8, `행 ${index}`);
        const manager = Number(row[0]); const mode = Number(row[2]); const missing = row[1] === '~'; const text = missing ? null : Buffer.from(row[1] === '-' ? '' : row[1], 'hex').toString('latin1');
        const scratch = new Uint8Array(PLAYER_IDENTITY_RECORD_SIZE).fill(0xa5); const trace = []; let resolutions = 0;
        const hooks = {
            resolve: (record) => {
                const name = recordName(record); trace.push(`R:${hexText(name)}`); resolutions += 1; if (mode === 5 && resolutions === 2) throw new RecipientStop('resolve');
                if (mode === 4 && name === 'Unknown') { const replacement = 'EVERYONE'; for (let i = 0; i < replacement.length; i += 1) record[8 + i] = replacement.charCodeAt(i); record[8 + replacement.length] = 0; }
                return mode === 2 || (mode !== 1 && (name === 'Alice' || name === 'ALICE'));
            },
            translate: (key) => { trace.push(`T:${hexText(key)}`); if (mode === 6) throw new RecipientStop('translate'); return mode === 3 && key === 'EVERYONE' ? 'Tous' : key; },
        };
        let status = 'normal'; let result = false;
        try { result = checkChatRecipients(manager, text, scratch, hooks); } catch (error) { if (!(error instanceof RecipientStop)) throw error; status = error.where; stops += 1; }
        const problems = [];
        if (status !== row[3]) problems.push('status'); if (status === 'normal' && String(result ? 1 : 0) !== row[4]) problems.push('result'); if (row[5] !== '-' && hexBytes(scratch) !== row[5]) problems.push('scratch'); if ((trace.join(';') || '-') !== row[6]) problems.push('trace');
        if (problems.length) failures.push(`${index}/${mode}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []); assert.ok(stops > 0);
});

test('ChatRecipients 실제 Identity 원본 48행: 이름 길이와 모드가 다른 입력이 실제 RawPlayerIdentity 조회와 접합해 같은 반환·지역·호출 순서를 낸다', () => {
    const table = rows('chat-recipient-identity-x86.tsv'); const failures = [];
    assert.equal(table.length, 48);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 6, `행 ${index}`);
        const scene = Number(row[0]); const text = Buffer.from(row[1] === '-' ? '' : row[1], 'hex').toString('latin1');
        const players = new Uint8Array(1620); const chat = new Uint8Array(4800);
        // 원문 DWORD 와 NUL 포함 이름만 쓰고 인접 필드는 보존한다.
        const put = (bytes, at, value) => { for (let i = 0; i < 4; i += 1) bytes[at + i] = (value >>> (i * 8)) & 0xff; };
        const name = (bytes, at, value) => { for (let i = 0; i < value.length; i += 1) bytes[at + i] = value.charCodeAt(i); bytes[at + value.length] = 0; };
        name(players, 180 + 12, 'LocalUser'); put(players, 180 + 44, 100); name(players, 360 + 12, 'Alice'); put(players, 360 + 44, 200); name(chat, 0, 'Bob'); put(chat, 44, 300); name(chat, 48, 'B'.repeat(40)); put(chat, 48 + 44, 400);
        const reads = { localOwner: () => 1, subscriber: () => 100, address: () => 0, name: () => 'LocalUser', battle: () => scene === 1, editor: () => scene === 2, metamap: () => false };
        const identity = new RawPlayerIdentity(players, new Uint8Array(0), chat, reads); const scratch = new Uint8Array(PLAYER_IDENTITY_RECORD_SIZE).fill(0xa5); const trace = [];
        const event = (label, value) => { trace.push(`${label}${hexText(value)}`); };
        const hooks = { resolve: (record) => { event('R:', recordName(record)); return identity.resolve(record); }, translate: (key) => { event('T:', key); return key; } };
        const result = checkChatRecipients(1, text, scratch, hooks); const problems = [];
        if (String(result ? 1 : 0) !== row[2]) problems.push('result'); if (hexBytes(scratch) !== row[3]) problems.push('scratch'); if ((trace.join(';') || '-') !== row[4]) problems.push('trace');
        if (problems.length) failures.push(`${index}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

// Native 도구와 같은 비영 초기 패턴/명시 점유 입력이다(기대 정리는 별도 after 칼럼에서만 읽는다).
function chatClearInput(seed, occupied) {
    const bytes = new Uint8Array(4800); for (let i = 0; i < bytes.length; i += 1) bytes[i] = (seed + i * 37) & 255;
    const put = (at, value) => { for (let i = 0; i < 4; i += 1) bytes[at + i] = (value >>> (i * 8)) & 0xff; };
    // 점유 입력을 쓰기 전 각 Subscriber 만 0 으로 둔다.
    for (let i = 0; i < 100; i += 1) put(i * 48 + 44, 0);
    if (occupied !== '-') for (const item of occupied.split(',')) { const colon = item.indexOf(':'); assert.ok(colon >= 0); const index = Number(item.slice(0, colon)); assert.ok(index < 100); put(index * 48 + 44, Number.parseInt(item.slice(colon + 1), 16) >>> 0); }
    return bytes;
}

test('채팅 기록 전체 정리 원본 10.78 216행: 108입력의 두 정밀도 관측이 resetOccupied 의 전체 4800바이트와 모두 일치하고 mask 정리·가드 바이트는 보존된다', () => {
    const lines = fs.readFileSync(new URL('../../cpppj/tests/fixtures/chat-state-clear-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('id\t'));
    const failures = []; let defaultPrecision = 0; let extendedPrecision = 0; const names = new Set();
    for (const [index, line] of lines.entries()) {
        const f = line.split('\t'); assert.equal(f.length, 7, `행 ${index}`);
        const records = chatClearInput(Number(f[2]), f[3]); const expected = Buffer.from(f[4], 'hex'); assert.equal(expected.length, 4800);
        const channels = new RawChatChannels(records, () => { throw new Error('Chat 정리가 현재 My를 조회함'); }, () => { throw new Error('Chat 정리가 원문 없는 이름/Refresh 효과를 호출함'); });
        channels.resetOccupied();
        const problems = []; if (!Buffer.from(records).equals(expected)) problems.push('bytes'); if (!(Number(f[5]) % 12 === 0 && Number(f[6]) > 0)) problems.push('writes');
        if (f[1] === '027f') defaultPrecision += 1; else { if (f[1] !== '037f') problems.push('control'); extendedPrecision += 1; } names.add(f[0]);
        if (problems.length) failures.push(`${index}/${f[0]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []); assert.equal(lines.length, 216); assert.equal(names.size, 108); assert.equal(defaultPrecision, 108); assert.equal(extendedPrecision, 108);
    // 빈 행의 stale/padding 과 경계는 보존하며 mask 정리로 전체 행 정리를 대신할 수 없다(기록 양쪽의 별도 가드 바이트).
    const guard = 16; const physical = new Uint8Array(4800 + guard * 2).fill(0xa7); const source = chatClearInput(19, '0:80000000,99:ffffffff'); const records = physical.subarray(guard, guard + 4800); records.set(source);
    const channels = new RawChatChannels(records, () => { throw new Error('전체 Chat 정리의 불필요 My 조회'); }, () => { throw new Error('전체 Chat 정리의 불필요 통지'); });
    channels.clear(0xff); assert.ok(records[0] === source[0] && records[44] === 0 && records[47] === 0x80 && records[40] === 0);
    channels.resetOccupied(); const once = Uint8Array.from(physical); channels.resetOccupied(); assert.deepEqual(Uint8Array.from(physical), once);
    assert.ok(records.subarray(0, 48).every((byte) => byte === 0)); assert.ok(records.subarray(4800 - 48).every((byte) => byte === 0)); assert.deepEqual(Array.from(records.subarray(48, 4800 - 48)), Array.from(source.subarray(48, 4800 - 48)));
    assert.ok(physical.subarray(0, guard).every((byte) => byte === 0xa7)); assert.ok(physical.subarray(physical.length - guard).every((byte) => byte === 0xa7));
});

// Enter 부모 162행과 전환/조회/프레임말 216행(ChatInput)의 독립 관측: 가상 Push 의 재진입 변경과 원본의 다른 플래그/입력/루트 크기 보존을 대조한다. (2026-11-15 WB04-E475)
function floatBitsOf(value) { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); }

test('ChatInput 원본 10.78 216행: 입력줄 표시 조회·전환·프레임말 요청 소비의 플래그·현재 Chat·요청·반환·호출 순서가 모두 일치한다', () => {
    const table = rows('chat-input-toggle-x86.tsv'); const failures = [];
    assert.equal(table.length, 216);
    for (const [index, f] of table.entries()) {
        assert.equal(f.length, 16, `행 ${index}`);
        const first = new HudChatNode(); const second = new HudChatNode(); const consoles = [new GumpCreateNode(), new GumpCreateNode()]; let current = Number(f[2]) ? 1 : 0; const swap = Number(f[5]); const trace = [];
        first.console = 2; second.console = 4; consoles[0].flags = Number(f[3]); consoles[1].flags = Number(f[4]); consoles[0].x = 12; consoles[0].y = -14; consoles[1].x = 14; consoles[1].y = -16;
        const nodeOf = (token) => (token === 1 ? first : token === 3 ? second : token === 2 ? consoles[0] : token === 4 ? consoles[1] : null);
        const geometry = new HudChatGeometry({
            chat: (token) => (token === 1 ? first : token === 3 ? second : null), node: nodeOf, current: () => current,
            push: (token) => { trace.push(`push:${token}`); if (swap === 1) current = 3; else if (swap === 2) first.console = 4; },
            attach: (token, x, y, parent, mode) => { trace.push(`attach:${token}:${floatBitsOf(x)}:${floatBitsOf(y)}:${parent}:${mode}`); },
        });
        const request = { value: Number(f[6]) | 0 }; let result = 0;
        if (f[1] === 'visible') result = geometry.inputVisible(); else if (f[1] === 'toggle') geometry.toggleInputVisible(); else { assert.equal(f[1], 'frame'); consumeChatInputToggle(request, () => { geometry.toggleInputVisible(); }); }
        const problems = [];
        if (!(f[7] === '1' && f[14] === '0' && Number(f[15]) > 0)) problems.push('range'); if (consoles[0].flags !== Number(f[8]) || consoles[1].flags !== Number(f[9])) problems.push('flags');
        if (current !== Number(f[10]) || (request.value >>> 0) !== Number(f[11])) problems.push('state'); if ((result >>> 0) !== Number(f[12])) problems.push('result'); if ((trace.join(';') || '-') !== f[13]) problems.push('trace');
        if (problems.length) failures.push(`${index}/${f[1]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
    // 실패한 프레임 callback 은 요청을 보존하고 성공한 한 번만 0 으로 소비한다.
    const box = { value: 0 }; consumeChatInputToggle(box, undefined); box.value = -1; assert.throws(() => consumeChatInputToggle(box, () => { throw new Error('명시 프레임 장치 실패'); }), /명시 프레임 장치 실패/); assert.equal(box.value, -1);
    let calls = 0; consumeChatInputToggle(box, () => { calls += 1; }); consumeChatInputToggle(box, () => { calls += 1; }); assert.equal(box.value, 0); assert.equal(calls, 1);
});

test('ChatInput 원본 10.78 162행: Guide/Gump/모달 관문 뒤 Enter 의 표시·열기 호출 순서가 원문 부모 관측과 일치한다', () => {
    const table = rows('chat-input-key-x86.tsv'); const failures = [];
    assert.equal(table.length, 162);
    for (const [index, f] of table.entries()) {
        assert.equal(f.length, 18, `행 ${index}`);
        // 명시 장치 세 개의 실제 호출만 추출한다.
        const expected = f[15].split(';').filter((item) => item === 'visible' || item === 'show:1' || item === 'open').join(';'); const actual = [];
        // 정지 상태는 표에 남기며 Enter 앞에 추가 관문을 만들지 않는다.
        if (!Number(f[2]) && !Number(f[3]) && !Number(f[5])) dispatchChatInputKey({ code: Number(f[1].split(':')[0]), x: 71, y: 83 }, () => { actual.push('visible'); return Number(f[9]); }, () => { actual.push('show:1'); }, () => { actual.push('open'); });
        const problems = []; if (actual.join(';') !== expected) problems.push('trace'); if (!(f[10] === '1' && f[16] === '0' && Number(f[17]) > 0)) problems.push('range');
        if (problems.length) failures.push(`${index}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});
