// 채팅 계열 이식 모듈(RawChatFormat·RawChatMessage·RawChatPacket·RawChatReceive·RawChatSubmit)을 원본 10.78 독립 관측 fixture 에 대조한다. 원본게임을 실행하지 않는다.
// 입력 열만 읽고 관측 열은 비교에만 쓴다(cpppj `ChatFormatTests` 등과 같은 방식). 이전 단위 시험의 "cpppj에도 직접 시험이 없다"는 오기였으며 이 시험이 처음으로 원본 관측 수치에 직접 대조한다. (2026-11-15 WB04-E474)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { applyChatFormat, stripChatFormatSlots, CHAT_FORMAT_CAPTURE_BYTES } from '../www/ui/raw-chat-format.js';
import { copyChatManagerFormat } from '../www/core/raw-chat-manager-prefix.js';
import { formatChatMessage, formatChatInput, sayChatMessage, writeChatNameFormat } from '../www/ui/raw-chat-message.js';
import { sendChatPacket, observeChatPacket } from '../www/ui/raw-chat-packet.js';
import { receiveChatPacket, dispatchChatPacket } from '../www/ui/raw-chat-receive.js';
import { submitChatInput, addChatRecipient } from '../www/ui/raw-chat-submit.js';
import { PLAYER_IDENTITY_RECORD_SIZE } from '../www/core/raw-player-identity.js';

// 빈 마지막 열과 NUL 원시 문자열을 구별하는 TSV 행 목록이다(머리글 한 줄을 건너뛴다).
function rows(name) { return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0).slice(1).map((line) => line.split('\t')); }
// 바이트 배열을 소문자 hex 로, 바이트 문자열(1 문자 = 1 바이트)을 hex 로 기록해 제어문자/미작성 꼬리까지 비교한다.
const hexBytes = (bytes) => Buffer.from(bytes).toString('hex');
const hexText = (text) => Buffer.from(text, 'latin1').toString('hex');
// 원문 입력 hex 를 UTF8 변환 없이 좁은 바이트 문자열로 되돌린다("-" 는 빈 문자열).
const input = (hex) => (hex === '-' ? '' : Buffer.from(hex, 'hex').toString('latin1'));

// 원문 관측의 두 실제 manager 토큰이다.
const FIRST = 0x12001000; const SECOND = 0x12004000;
// 선택한 장치에서 앞선 부분 쓰기를 남기는 명시 중단 입력이다.
class FormatStop extends Error { constructor(reason) { super(reason); this.reason = reason; } }

test('RawChatFormat 원본 10.78 372행: 정규식 캡처·슬롯 제거·두 manager 쓰기·설정 키와 선택 중단의 전체 부분 효과가 manager 메모리·캡처·호출 순서까지 일치한다', () => {
    const table = rows('chat-format-x86.tsv'); const failures = [];
    assert.equal(table.length, 372);
    for (const [index, fields] of table.entries()) {
        assert.equal(fields.length, 10, `행 ${index}`);
        const front = fields[0] === '1'; const source = input(fields[1]); const mode = fields[2];
        // 같은 두 비영 원문 입력만 구성한다.
        const first = new Uint8Array(2712); const second = new Uint8Array(2712);
        for (let i = 0; i < first.length; i += 1) { first[i] = (0xa5 + i * 17) & 255; second[i] = (0xb0 + i * 17) & 255; }
        const capture = new Uint8Array(CHAT_FORMAT_CAPTURE_BYTES).fill(0xd3); const globals = { current: mode.startsWith('assert') ? 0 : FIRST, subscriber: 100 }; const trace = [];
        const alias = (token) => { if (!token) return 0; if (token === FIRST) return 1; if (token === SECOND) return 2; throw new Error('서식 검사 manager 토큰 오류'); };
        const memory = (token) => { if (token === FIRST) return first; if (token === SECOND) return second; throw new RangeError('서식 검사 실제 manager 없음'); };
        const hooks = {
            memory,
            assertion: (number) => { trace.push(`assert:${number}`); if (mode === 'assertStop') throw new FormatStop('assertion'); assert.equal(mode, 'assertRestore'); globals.current = FIRST; },
            stripSlots: (bytes) => { trace.push(`strip:${hexBytes(bytes)}`); stripChatFormatSlots(bytes); },
            copyFormat: (token, offset, text, count) => {
                trace.push(`copy:${alias(token)}:${offset}:${count}`); copyChatManagerFormat(memory(token).subarray(offset), text, count);
                if (mode === 'copySwitch') { globals.current = SECOND; globals.subscriber = 0xffffffff; const next = '~gNEXT'; for (let i = 0; i < next.length; i += 1) capture[i] = next.charCodeAt(i); capture[next.length] = 0; }
                if (mode === 'stopCopy') throw new FormatStop('copy');
            },
            writeConfig: (key, value) => { trace.push(`key:${hexText(key)}`); trace.push(`config:${hexText(key)}:${hexText(value)}`); if (mode === 'stopConfig') throw new FormatStop('config'); },
            illegalFormat: (text) => { trace.push(`error:${hexText(text)}`); },
        };
        let status = 'normal'; let result;
        try { result = applyChatFormat(front, globals, source, capture, hooks); } catch (error) { if (!(error instanceof FormatStop)) throw error; status = error.reason; }
        const problems = [];
        if (status === 'normal' && result !== 0) problems.push('result');
        if (status !== fields[3]) problems.push('status'); if (alias(globals.current) !== Number(fields[4])) problems.push('current'); if (globals.subscriber !== Number(fields[5])) problems.push('subscriber');
        if (hexBytes(first) !== fields[6]) problems.push('managerA'); if (hexBytes(second) !== fields[7]) problems.push('managerB'); if (hexBytes(capture) !== fields[8]) problems.push('capture'); if (trace.join(';') !== fields[9]) problems.push('trace');
        if (problems.length) failures.push(`${index}/${mode}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

// 일반 본문 조합 부모(RawChatMessage)의 524행: 세 호출 부모(format/wrap/say)의 전체 manager·입력·지역 이름 바이트와 선택 중단, 호출 순서와 현재 이름/구독자 읽기를 대조한다. (2026-11-15 WB04-E474)
const RECIPIENTS = 0x12007000;
// 입력 DWORD 를 같은 실제 네 바이트에 쓰는 little endian 도우미이다.
function putWord(bytes, at, value) { for (let i = 0; i < 4; i += 1) bytes[at + i] = (value >>> (8 * i)) & 0xff; }
// 내부 NUL 과 그 뒤 입력까지 그대로 쓰고 마지막 NUL 만 추가한다.
function store(bytes, at, text) { for (let i = 0; i < text.length; i += 1) bytes[at + i] = text.charCodeAt(i) & 0xff; bytes[at + text.length] = 0; }

test('RawChatMessage 원본 10.78 524행: 서식·wrap·say 세 호출 부모의 전체 manager/입력/지역 이름 바이트와 현재 이름·구독자 읽기·선택 중단이 호출 순서까지 일치한다', () => {
    const table = rows('chat-message-x86.tsv'); const failures = [];
    assert.equal(table.length, 524);
    for (const [index, f] of table.entries()) {
        assert.equal(f.length, 20, `행 ${index}`);
        const kind = f[0]; const spec = Number(f[1]); const mask = Number(f[2]); const direct = Number(f[3]); const count = Number(f[4]); const receiver = f[5] === '1' ? SECOND : FIRST; const source = input(f[6]); const name = input(f[7]); const mode = f[8]; const lookup = f[9] === '1';
        const first = new Uint8Array(2712); const second = new Uint8Array(2712); const recipients = new Uint8Array(128); const body = new Uint8Array(256).fill(0xd3); const local = new Uint8Array(256).fill(0xa5);
        for (let i = 0; i < first.length; i += 1) { first[i] = (0xa5 + i * 17) & 255; second[i] = (0xb0 + i * 17) & 255; }
        // 두 manager 의 입력 필드는 원문과 같은 DWORD/문자열만 쓴다.
        for (const memory of [first, second]) { putWord(memory, 0, count); putWord(memory, 4, 32); putWord(memory, 8, RECIPIENTS); putWord(memory, 12, direct); putWord(memory, 16, mask); store(memory, 0x14, '~rBACK'); store(memory, 0x50, '~gFRONT'); }
        if (count === 1) putWord(recipients, 0, 200); else if (count === 2) { putWord(recipients, 0, 100); putWord(recipients, 4, 200); }
        store(body, 0, source);
        const globals = { current: f[10] === '1' ? FIRST : 0, subscriber: 100 }; let currentName = 1; const trace = []; let localUsed = false;
        const memory = (token) => { if (token === FIRST) return first; if (token === SECOND) return second; if (token === RECIPIENTS) return recipients; throw new RangeError('본문 검사 실제 토큰 없음'); };
        const hooks = {
            memory, currentName: () => (currentName === 2 ? 'Changed' : name),
            nameForSubscriber: (destination, target) => {
                localUsed = true; trace.push(`lookup:${target}`); if (mode === 'stopResolve') throw new FormatStop('resolve');
                if (mode === 'resolveSwitch') { currentName = 2; globals.subscriber = 7; putWord(memory(receiver), 12, 0); store(first, 0x14, '~pLateBack'); store(first, 0x50, '~uLateFront'); store(body, 0, 'new~0body'); }
                store(destination, 0, lookup ? 'Ta%s' : 'SOMEONE'); return 1;
            },
            formatName: (destination, pattern, actualName, other) => {
                localUsed = true; trace.push(`format:${hexText(pattern)}:${hexText(actualName)}:${hexText(other)}:${destination.byteOffset - first.byteOffset}`); writeChatNameFormat(destination, pattern, actualName, other);
                if (mode === 'renderSwitch') { putWord(memory(receiver), 12, 0); putWord(first, 16, 32); store(first, 0x50, '~lRender'); store(body, 0, 'render~1changed'); }
                if (mode === 'stopFormat') throw new FormatStop('format');
            },
            stripSlots: (bytes) => { trace.push('strip'); stripChatFormatSlots(bytes); },
            deliver: (manager, target, bytes) => { assert.equal(manager, FIRST); trace.push(`deliver:${target === SECOND ? '2' : '1'}:${bytes.length}:${hexBytes(bytes)}`); if (mode === 'stopDeliver') throw new FormatStop('deliver'); return 0x55aacc33; },
            inputVisible: (visible) => { trace.push(`visible:${visible}`); },
        };
        let status = 'normal'; let result = 0;
        try {
            if (kind === 'format') { const patterns = ['%s> ', '%s ', '%s (to %s)> ', null]; result = formatChatMessage(FIRST, receiver, body, patterns[spec], local, globals, hooks); }
            else if (kind === 'wrap') result = formatChatInput(FIRST, body, local, globals, hooks);
            else { assert.equal(kind, 'say'); result = sayChatMessage(body, local, globals, hooks); }
        } catch (error) { if (!(error instanceof FormatStop)) throw error; status = error.reason; }
        const problems = []; const events = trace.join(';');
        if (status !== f[11]) problems.push('status'); if (status === 'normal' && result !== Number(f[12])) problems.push('result'); if (currentName !== Number(f[13])) problems.push('currentName'); if (globals.subscriber !== Number(f[14])) problems.push('subscriber');
        if (hexBytes(first) !== f[15]) problems.push('managerA'); if (hexBytes(second) !== f[16]) problems.push('managerB'); if (hexBytes(body) !== f[17]) problems.push('input'); if ((localUsed ? hexBytes(local) : '-') !== f[18]) problems.push('local'); if ((events || '-') !== f[19]) problems.push('trace');
        if (problems.length) failures.push(`${index}/${kind}/${mode}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

// 송신 부모(SendChatPacket)와 길이 observer(ObserveChatPacket)의 독립 원문 256행: 단독 9/fallback 부모와 송수신 observer 의 전체 manager/수신자 물리·현재 전역·부분 효과·호출 순서를 대조한다. (2026-11-15 WB04-E474)
const PACKET_MANAGER = 0x12001000; const PACKET_RECIPIENTS = 0x12004000;
// 원문 입력의 비정렬 BYTE/WORD/DWORD 를 같은 little endian 으로 쓰고(폭 밖 BYTE 는 보존) 읽는다.
function putWidth(bytes, at, value, width = 4) { for (let i = 0; i < width; i += 1) bytes[at + i] = (value >>> (i * 8)) & 0xff; }
function readWidth(bytes, at, width = 4) { let result = 0; for (let i = 0; i < width; i += 1) result += bytes[at + i] * 2 ** (i * 8); return result; }

test('RawChatPacket 원본 10.78 256행: 단독 9/fallback 송신 부모와 송수신 observer 의 전체 manager·수신자 물리·현재 전역·부분 효과가 호출 순서까지 일치한다', () => {
    const table = rows('chat-packet-x86.tsv'); const failures = [];
    assert.equal(table.length, 256);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 18, `행 ${index}`);
        const direct = Number(row[2]); const mask = Number(row[3]); const count = Number(row[4]); const mode = row[8];
        const source = Uint8Array.from(Buffer.from(row[6] === '-' ? '' : row[6], 'hex')); const nameInput = Uint8Array.from(Buffer.from(row[7] === '-' ? '' : row[7], 'hex'));
        let battle = Number(row[1]); let subscriber = 100; let result = 0; const trace = []; let packet = new Uint8Array(0);
        const manager = new Uint8Array(2712); const recipients = new Uint8Array(1200).fill(0xa5); const name = new Uint8Array(256);
        // 원문과 같은 미작성 manager 꼬리를 둔다.
        for (let i = 0; i < manager.length; i += 1) manager[i] = (0xa5 + i * 17) & 255;
        putWidth(manager, 0, count); putWidth(manager, 4, 300); putWidth(manager, 8, PACKET_RECIPIENTS); putWidth(manager, 12, direct); putWidth(manager, 16, mask); name.set(nameInput);
        // 중간 자기 항목과 실제 원문 순서를 재현한다.
        for (let i = 0; i < count; i += 1) putWidth(recipients, i * 4, row[5] === '1' && i === Math.floor(count / 2) ? 100 : 200 + i);
        const hooks = {
            memory: (token) => { if (token === PACKET_MANAGER) return manager; if (token === PACKET_RECIPIENTS) return recipients; throw new RangeError('Chat 패킷 검사 물리 토큰 없음'); },
            currentName: () => name,
            read: (at) => { if (at === 0x540c74) return subscriber; if (at === 0x594fbc) return battle; if (at === 0x594fb8 || at === 0x594fc8 || at === 0x544c5c) return 0; if (at === 0x52edf6) return 0x41; throw new RangeError('Chat 패킷 검사 원문 전역 없음'); },
            addRecipient: (target, added) => {
                assert.equal(target, PACKET_MANAGER); trace.push(`add:${added}`); if (mode === 'stopAdd') throw new FormatStop('add');
                const current = readWidth(manager, 0); putWidth(recipients, current * 4, added); putWidth(manager, 0, current + 1);
                if (mode === 'addSwitch') { subscriber = 7; battle = 1 - battle; putWidth(manager, 16, 128); putWidth(manager, 12, 1); for (let i = 0; i < 8; i += 1) name[i] = 'Modified'.charCodeAt(i); name[8] = 0; }
            },
            send: (target, bytes) => { assert.equal(target, 9); packet = Uint8Array.from(bytes); trace.push(`send:9:${hexBytes(bytes)}`); if (mode === 'stopSend') throw new FormatStop('send'); },
            log: (channel, text) => { trace.push(`log:${channel}:${hexText(text)}`); if (mode === 'stopLog') throw new FormatStop('log'); },
            zeroAddress: () => { trace.push('address:0.0.0.0:0'); },
            localReceive: (bytes) => { packet = Uint8Array.from(bytes); trace.push(`receive:${hexBytes(bytes)}`); if (mode === 'stopReceive') throw new FormatStop('receive'); },
        };
        let status = 'normal';
        try {
            if (row[0] === 'send') result = sendChatPacket(PACKET_MANAGER, PACKET_MANAGER, source, hooks);
            else { assert.equal(row[0], 'observe'); packet = new Uint8Array(32).fill(0xd3); putWidth(packet, 10, direct, 2); putWidth(packet, 12, mask, 2); putWidth(packet, 14, count, 1); observeChatPacket(packet, Number(row[9]), (line) => { trace.push(`assert:${line}`); }); }
        } catch (error) { if (!(error instanceof FormatStop)) throw error; status = error.reason; }
        const problems = [];
        if (status !== row[10]) problems.push('status'); if (!(row[11] === '-' || result === Number(row[11]))) problems.push('result'); if (subscriber !== Number(row[12])) problems.push('subscriber'); if (battle !== Number(row[13])) problems.push('battle');
        if (hexBytes(manager) !== row[14]) problems.push('manager'); if (hexBytes(recipients) !== row[15]) problems.push('recipients'); if ((packet.length === 0 ? '-' : hexBytes(packet)) !== row[16]) problems.push('packet'); if ((trace.join(';') || '-') !== row[17]) problems.push('trace');
        if (problems.length) failures.push(`${index}/${row[0]}/${mode}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

// 수신 부모(ReceiveChatPacket)와 현재 Chat wrapper(DispatchChatPacket)의 원문 238행: 버전/ASCII·무시·채널/관계 옵션과 현재값 변경·중단을 전체 패킷과 함께 대조한다. (2026-11-15 WB04-E474)
const CHAT = 0x12008000;
// 독립 입력 비트와 설정 장치를 잇는 원문 키 순서이다(오타 포함).
const RECEIVE_KEYS = ['ShowBattle', 'ShowEveryone', 'ShowAllies', 'ShowObserver', 'ShowWisper', 'ShowMyAllies', 'ShowMyEmenys', 'ShowMyNeutrals', 'ShowChatWindow', 'PlaySoundMessage'];
// 같은 8 DWORD 를 원문 little endian 32 BYTE 문맥으로 변환한 hex 이다(각 현재 DWORD 의 비트를 유지한다).
function contextHex(context) { assert.equal(context.length, 8); const bytes = new Uint8Array(32); for (let i = 0; i < 8; i += 1) putWidth(bytes, i * 4, context[i] >>> 0); return hexBytes(bytes); }

test('RawChatReceive 원본 10.78 238행: 버전/ASCII·무시·채널/관계 옵션·현재값 변경·선택 중단이 전체 패킷 바이트와 호출 순서까지 일치한다', () => {
    const table = rows('chat-receive-x86.tsv'); const failures = [];
    assert.equal(table.length, 238);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 18, `행 ${index}`);
        const number = (at) => Number(row[at]); let battle = number(3); let subscriber = 100; let color = 44;
        const mask = number(4); const sender = number(5); const flags = number(6); const allied = number(7); const neutral = number(8); const ignored = number(9) !== 0; const mode = row[10];
        const source = Uint8Array.from(Buffer.from(row[11], 'hex')); const name = 'Sender'; const packet = new Uint8Array(256).fill(0xd3); const trace = [];
        putWidth(packet, 0, number(1), 1); putWidth(packet, 1, 0x41, 1); putWidth(packet, 2, 15 + name.length + 1 + source.length + 4, 2); putWidth(packet, 5, number(2), 1); putWidth(packet, 6, sender); putWidth(packet, 10, 7, 2);
        putWidth(packet, 12, source.length, 2); putWidth(packet, 14, 1, 1); for (let i = 0; i < name.length; i += 1) packet[15 + i] = name.charCodeAt(i); packet[21] = 0; packet.set(source, 22); putWidth(packet, 22 + source.length, 100);
        const hooks = {
            read: (at) => {
                if (at === 0x540c74) return subscriber; if (at === 0x594fbc) return battle; if (at === 0x594fc8) return 0; if (at === 0x548710) return mask; if (at === 0x548748) return 7; if (at === 0x540c70) return 2; if (at === 0x5ca92c) return 0x1234; if (at === 0x5b5dd8) return color;
                if (at === 0x548740) return row[0] === 'noRoot' ? 0 : CHAT; if (at === 0x595200 + (2 * 9 + 3) * 4) return allied; if (at === 0x52f81e + 3) return neutral; throw new RangeError('Chat 수신 검사 전역 없음');
            },
            ignored: (actual) => { trace.push(`ignored:${actual}`); return ignored; },
            config: (key) => {
                trace.push(`config:${key}`); if (mode === 'stopConfig') throw new FormatStop('config'); if (mode === 'configBattleOff' && key === RECEIVE_KEYS[RECEIVE_KEYS.length - 1]) battle = 0;
                const found = RECEIVE_KEYS.indexOf(key); assert.ok(found >= 0); return (flags & (1 << found)) !== 0 ? 1 : 0;
            },
            owner: (actual) => { trace.push(`owner:${actual}`); return 3; },
            wake: (target) => { trace.push(`wake:${target}`); if (mode === 'wakeSwitch') { subscriber = sender; color = 99; } if (mode === 'stopWake') throw new FormatStop('wake'); },
            sound: (text) => { trace.push(`sound:${text}`); },
            append: (target, text, context) => { assert.equal(target, CHAT); trace.push(`append:${hexText(text)}:${contextHex(context)}`); if (mode === 'stopAppend') throw new FormatStop('append'); },
            show: () => { trace.push('show'); if (mode === 'stopShow') throw new FormatStop('show'); },
            log: (channel, text) => { trace.push(`log:${channel}:${hexText(text)}`); },
        };
        let status = 'normal';
        try { if (row[0] === 'direct') receiveChatPacket(CHAT, packet, hooks); else dispatchChatPacket(packet, hooks); } catch (error) { if (!(error instanceof FormatStop)) throw error; status = error.reason; }
        const problems = [];
        if (status !== row[12]) problems.push('status'); if (hexBytes(packet) !== row[13]) problems.push('packet'); if (row[14] !== '7') problems.push('news'); if (subscriber !== number(15)) problems.push('subscriber'); if (battle !== number(16)) problems.push('battle'); if ((trace.join(';') || '-') !== row[17]) problems.push('trace');
        if (problems.length) failures.push(`${index}/${row[0]}/${mode}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

// 제출(SubmitChatInput)과 수신자 추가(AddChatRecipient)의 독립 원문 383행: 전체 manager/이전·새 배열/식별자 지역/본문 작업 공간/호출 순서를 모든 고정 행과 대조한다. (2026-11-15 WB04-E474)
const SUBMIT_MANAGER = 0x12010000; const SUBMIT_BUFFER = 0x12001000; const SUBMIT_NEW = 0x12070000;
// 선택된 원문 장치 중단의 이름이다.
class SubmitStop extends Error { constructor(where) { super(where); this.where = where; } }
// 기록은 최초 원문 입력과 같은 세 장면만 구성한다(scene 0 은 빈 행, 1 은 6행, 2 는 확장을 요구하는 100행).
function submitRecords(scene) {
    const rowsBytes = new Uint8Array(4800); const subscribers = [11, 22, 11, 0, 33, 44]; const flags = [0x20, 0x40, 0x28, 0xff, 0x80, 0xa0];
    for (let i = 0; i < 100; i += 1) { if (!scene || (scene === 1 && i >= 6)) continue; rowsBytes[i * 48 + 40] = scene === 2 ? 0x20 : flags[i]; putWidth(rowsBytes, i * 48 + 44, scene === 2 ? i + 11 : subscribers[i]); }
    return rowsBytes;
}
// 장치에 전달된 C 문자열만 NUL 앞까지 빌린다(NUL 이 없으면 오류).
function cText(bytes) { const end = bytes.indexOf(0); if (end < 0) throw new Error('검사 장치의 NUL 없음'); return Buffer.from(bytes.subarray(0, end)).toString('latin1'); }

test('RawChatSubmit 원본 10.78 383행: 제출/수신자 추가의 전체 manager·이전/새 수신자 배열·식별자 지역·본문 작업 공간과 호출 순서·선택 중단이 모두 일치한다', () => {
    const table = rows('chat-submit-x86.tsv'); const failures = []; let stops = 0;
    assert.equal(table.length, 383);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 16, `행 ${index}`);
        const scene = Number(row[2]); const mode = Number(row[3]); const initialCount = Number(row[4]); const capacity = Number(row[5]); const subscriber = Number(row[6]);
        const memory = new Map(); const manager = new Uint8Array(2712); memory.set(SUBMIT_MANAGER, manager);
        // 부모가 안 쓰는 꼬리까지 원문 명시 입력으로 정의한다.
        for (let i = 0; i < manager.length; i += 1) manager[i] = (165 + i * 17) & 255;
        putWidth(manager, 0, initialCount); putWidth(manager, 4, capacity); putWidth(manager, 8, SUBMIT_BUFFER); putWidth(manager, 12, 7); putWidth(manager, 16, 0xaabbccdd);
        const old = new Uint8Array(capacity * 4); memory.set(SUBMIT_BUFFER, old); for (let i = 0; i < capacity; i += 1) putWidth(old, i * 4, 100 + i);
        const records = submitRecords(scene); const source = row[1] === '-' ? '' : Buffer.from(row[1], 'hex').toString('latin1'); const identity = new Uint8Array(PLAYER_IDENTITY_RECORD_SIZE).fill(0xa5); const work = Uint8Array.from(Buffer.from(row[13] === '-' ? '' : row[13], 'hex'));
        if (row[11] !== '-') identity.set(Buffer.from(row[11], 'hex'));
        const trace = []; const allocations = []; let next = SUBMIT_NEW; let resolves = 0;
        const hooks = {
            memory: (token) => { const bytes = memory.get(token); if (!bytes) throw new RangeError('검사 장치 토큰 없음'); return bytes; },
            allocate: (size) => { trace.push(`N:${size}`); if (mode === 6) return 0; const token = next; next += (size + 15) & ~15; memory.set(token, new Uint8Array(size).fill(0xd3)); allocations.push(token); return token; },
            release: (token) => { trace.push(`F:${token}`); if (mode === 7) throw new SubmitStop('free'); },
            assertion: (number) => { trace.push(`A:${number}`); throw new SubmitStop('assertion'); },
            resolve: (record) => {
                const name = cText(record.subarray(8)); trace.push(`R:${hexText(name)}`); resolves += 1; if (mode === 5 && resolves === 2) throw new SubmitStop('resolve');
                if (mode === 3 && name === 'Unknown') { const replacement = 'ALLIES'; for (let i = 0; i < replacement.length; i += 1) record[8 + i] = replacement.charCodeAt(i); record[8 + replacement.length] = 0; }
                const accepted = mode === 2 || (mode !== 1 && (name === 'Alice' || name === 'Bob')); if (accepted) putWidth(record, 4, name === 'Bob' ? 22 : 11); return accepted;
            },
            translate: (key) => { trace.push(`T:${hexText(key)}`); return mode === 4 && key === 'EVERYONE' ? 'Tous' : key; },
            records: () => records, inputVisible: (visible) => { trace.push(`V:${visible}`); },
            dispatch: (text) => { trace.push(`D:${hexText(cText(text))}`); },
        };
        let status = 'normal';
        try { if (row[0] === 'submit') submitChatInput(SUBMIT_MANAGER, source, identity, work, hooks); else addChatRecipient(SUBMIT_MANAGER, subscriber, hooks); } catch (error) { if (!(error instanceof SubmitStop)) throw error; status = error.where; stops += 1; }
        const problems = []; const dash = (bytes) => (bytes.length === 0 ? '-' : hexBytes(bytes));
        if (status !== row[7]) problems.push('status'); if (dash(manager) !== row[8]) problems.push('manager'); if (dash(old) !== row[9]) problems.push('old');
        const buffers = allocations.map((token) => `${token}:${dash(memory.get(token))}`).join('|'); if ((buffers || '-') !== row[10]) problems.push('buffers');
        if (row[12] !== '-' && hexBytes(identity) !== row[12]) problems.push('identity'); if (row[14] !== '-' && hexBytes(work) !== row[14]) problems.push('work'); if ((trace.join(';') || '-') !== row[15]) problems.push('trace');
        if (problems.length) failures.push(`${index}/${row[0]}/${mode}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []); assert.ok(stops > 0);
});
