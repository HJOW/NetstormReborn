// Chat 채널 기록(RawChatChannels: Add/Clear/ResetOccupied)을 원본 10.78 관측으로 대조한다(WB04-E6, cpppj ChatChannelsTests 이식).
//  - chat-channels-x86.tsv 2448행: 자기/0/-1/음수 식별자·만료 flags·이름 경계·기존/신규/가득 찬 100기록의 전체 4800 BYTE 변경과 하위 진입 순서를 대조한다.
//  - 계약: 현재 자기 subscriber 변경 시점의 자기 반환, 이름 정제/두 번 갱신 순서, 저장소·조회기 누락 거부, 점유 기록만 전부 반납.
// cpppj 의 Client 수명/CampaignAiServices 연결 검사는 해당 client 단위를 옮길 때 대조한다. (2026-11-08 WB04-E6: Chat 채널 기록 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { CHAT_CHANNEL_CALL, CHAT_RECORD_BYTES, RawChatChannels } from '../www/core/raw-chat-channels.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E6)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'chat-channels-x86.tsv'));

// fixture 숫자 열을 32비트로 감아 읽는다. (2026-11-08 WB04-E6)
const number = (text) => Number(BigInt(text) & 0xffffffffn);

// 같은 명시 초기 입력만 만든다. 기록 선택이나 기대 결과는 계산하지 않는다(cpppj ChatInput). (2026-11-08 WB04-E6)
function chatInput(scene) {
    const bytes = new Uint8Array(CHAT_RECORD_BYTES);
    // 원본이 건드리지 않아야 하는 stale BYTE 를 전체 저장소에 먼저 넣는다.
    for (let index = 0; index < bytes.length; index += 1) bytes[index] = (index * 37 + scene * 11) & 0xff;
    // 각 입력 기록의 이름/식별자/flags 를 원본 준비와 같은 위치에 쓴다.
    for (let index = 0; index < 100; index += 1) {
        const record = bytes.subarray(index * 48, (index + 1) * 48);
        record.fill(65 + (index % 26), 0, 40);
        const name = 'OldName';
        for (let position = 0; position < name.length; position += 1) record[position] = name.charCodeAt(position);
        record[7] = 0;
        record[40] = (index * 13 + 0x95) & 0xff;
        put(record, 44, scene === 0 || scene === 5 ? 0 : 1000 + index);
    }
    if (scene === 1) put(bytes, 7 * 48 + 44, 77);
    if (scene === 2) { put(bytes, 2 * 48 + 44, 0); put(bytes, 7 * 48 + 44, 77); }
    if (scene === 3) put(bytes, 99 * 48 + 44, 0);
    if (scene === 4) { put(bytes, 7 * 48 + 44, 77); put(bytes, 98 * 48 + 44, 77); }
    if (scene === 5) put(bytes, 44, 0xffffffff);
    return bytes;
}

// 원본 문자열 입력 hex 를 UTF-8 재해석 없이 이진 문자열로 읽는다. (2026-11-08 WB04-E6)
function unhex(hex) {
    let result = '';
    // 두 hex 글자가 한 BYTE 이다.
    for (let index = 0; index < hex.length; index += 2) result += String.fromCharCode(parseInt(hex.slice(index, index + 2), 16));
    return result;
}

// 원본 전체 4800 BYTE 관찰의 변경 위치(offset:byte 목록)를 초기 입력에 적용한다. 나머지 모든 바이트도 그대로 비교된다. (2026-11-08 WB04-E6)
function applyObservation(expected, changes) {
    if (changes === '-') return;
    // 손실 없는 offset:byte 이며 체크섬 충돌을 동일성 근거로 쓰지 않는다.
    for (const change of changes.split(',')) {
        const colon = change.indexOf(':');
        assert.ok(colon > 0, 'Chat 변경 위치 형식');
        const offset = Number(change.slice(0, colon));
        assert.ok(offset < expected.length, 'Chat 변경 위치 범위');
        expected[offset] = parseInt(change.slice(colon + 1), 16);
    }
}

// 자기/0/-1/음수 식별자·만료 flags·이름 경계·기존/신규/가득 찬 100기록을 실제 PE 출력과 비교한다.
test('RawChatChannels: 원본10.78 Add/Clear 2448행', { skip: !rows && '원본 fixture 없음' }, () => {
    let compared = 0;
    for (const row of rows) {
        if (row[0] === 'kind') continue;
        assert.equal(row.length, 9);
        const records = chatInput(Number(row[1]));
        const expected = records.slice();
        let current = number(row[3]);
        const trace = [];
        const channels = new RawChatChannels(records, () => current, (call, subscriber) => {
            trace.push(call === CHAT_CHANNEL_CALL.nameChanged ? `N,${subscriber}` : 'R');
        });
        const label = `행 ${compared} ${row[0]} scene${row[1]} sub${row[2]} cur${row[3]} flags${row[4]} name${row[5]}`;
        if (row[0] === 'A') {
            const name = row[5] === '-' ? null : unhex(row[5]);
            assert.equal(channels.add(number(row[2]), name, number(row[4])), Number(row[6]), label);
        } else if (row[0] === 'C') channels.clear(number(row[4]));
        else assert.fail(`알 수 없는 입력 종류 ${row[0]}`);
        applyObservation(expected, row[7]);
        assert.deepEqual(Buffer.from(records), Buffer.from(expected), `${label} 기록`);
        assert.equal(trace.length ? trace.join(';') : '-', row[8], `${label} 호출 순서`);
        compared += 1;
    }
    assert.equal(compared, 2448);
});

// 같은 클라이언트의 현재 subscriber 변경과 원본 자기 반환·이름 정제/두 번 갱신 시점을 확인한다.
test('RawChatChannels: 현재 자기 subscriber와 효과 순서', () => {
    const records = chatInput(0);
    let current = 77;
    let reads = 0;
    const calls = [];
    const channels = new RawChatChannels(records, () => { reads += 1; return current; }, (call, subscriber) => {
        calls.push(call);
        if (call === CHAT_CHANNEL_CALL.nameChanged) {
            // 이름 정제 진입 시점: 플래그는 이미 쓰였고 이름은 정제 전(공백 포함)이다.
            assert.equal(subscriber, 77);
            assert.equal(records[40], 0x40);
            assert.equal(records[0], 'A'.charCodeAt(0));
            assert.equal(records[1], 0x20);
        } else {
            // 갱신 진입 시점: 이름 정제가 끝났다.
            assert.equal(records[0], 'A'.charCodeAt(0));
            assert.equal(records[1], 'B'.charCodeAt(0));
            assert.equal(records[2], 0);
        }
    });
    const before = records.slice();
    // 자기 subscriber 는 이름·효과를 읽기 전에 -1 이다.
    assert.equal(channels.addToChannel(77, 'A B', 0x40), -1);
    assert.deepEqual(records, before);
    assert.equal(calls.length, 0);
    assert.equal(reads, 1);
    current = 1;
    assert.equal(channels.addToChannel(77, 'A B', 0x140), 0);
    assert.equal(reads, 2);
    assert.deepEqual(calls, [CHAT_CHANNEL_CALL.nameChanged, CHAT_CHANNEL_CALL.refresh, CHAT_CHANNEL_CALL.refresh]);
    channels.clear(0x140);
    assert.equal(records[40], 0);
    assert.equal(calls.length, 3);
});

// 저장소 크기/조회기 누락은 생성 시 거부하며 자기 경로에는 별도 소비자가 필요하지 않다.
test('RawChatChannels: 저장소와 조회기 계약', () => {
    const records = chatInput(0);
    assert.throws(() => new RawChatChannels(records.subarray(0, records.length - 1), () => 1), TypeError);
    assert.throws(() => new RawChatChannels(records, null), TypeError);
    assert.throws(() => new RawChatChannels([], () => 1), TypeError);
    const channels = new RawChatChannels(records, () => 1);
    const before = records.slice();
    assert.equal(channels.add(1, null, 0xffffffff), -1);
    assert.deepEqual(records, before);
});

// 점유 기록은 48 BYTE 전체를 반납하고 빈 기록의 stale 값은 유지하며, 이름 정제는 NUL 뒤를 건드리지 않는다.
test('RawChatChannels: resetOccupied와 이름 정제 경계', () => {
    const records = chatInput(1);
    const channels = new RawChatChannels(records, () => 1);
    const staleEmpty = records.slice(0, 48);
    // scene1 은 모든 기록이 점유(1000+index)이므로 0 번 기록을 비워 둔 상태로 만든다.
    put(records, 44, 0);
    const emptyRow = records.slice(0, 48);
    assert.notDeepEqual(emptyRow, staleEmpty);
    channels.resetOccupied();
    assert.deepEqual(records.slice(0, 48), emptyRow, '빈 기록의 stale 값은 유지');
    for (let index = 1; index < 100; index += 1) assert.ok(records.subarray(index * 48, (index + 1) * 48).every((byte) => byte === 0), `점유 기록 ${index} 반납`);
    // 금지 문자만 있는 이름은 Cheat<하위 BYTE> 로 대체된다.
    const fresh = chatInput(0);
    const named = new RawChatChannels(fresh, () => 1);
    assert.equal(named.add(0x1ff, '..??', 0), 0);
    assert.equal(Buffer.from(fresh.subarray(0, 9)).toString('latin1'), 'Cheat255\0');
    // NULL 이름은 Unknown<8자리 hex> 이다.
    assert.equal(named.add(0xabc, null, 0), 1);
    assert.equal(Buffer.from(fresh.subarray(48, 48 + 16)).toString('latin1'), 'Unknown00000abc\0');
    assert.equal(get(fresh, 48 + 44), 0xabc);
});
