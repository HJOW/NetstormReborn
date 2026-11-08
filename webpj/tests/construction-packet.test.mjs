// 건설 패킷 raw-construction-packet.js(ConstructionPacketView·initializeConstructionPacket·writeConstructionPacket·finalizeConstructionPacket)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: construction-packet-x86.tsv 86행 — Write 72행(통지 10개 값·헤더 3개 값·SID 목록 → 실제 512바이트 전체)과 Finalize 14행(송신 여부·최종화 전 512바이트 →
//    진단 호출 여부와 최종화 뒤 512바이트 전체)이다. 512바이트 뒤의 호출자 버퍼(0xcc 로 채운 5바이트)는 건드리지 않는다.
//  - 계약: 512바이트 미만 버퍼·SID 20개는 효과 전에 거부(버퍼 보존), 최종화는 sending 0 이면 아무것도 읽지 않고 sending 비영이면 27바이트 미만 버퍼를 거부,
//    sidCount 255 의 진단이 연결되지 않으면 거부, 뷰는 필드를 소비 위치마다 현재 바이트에서 읽고 물리 범위 밖을 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E134: 건설 패킷 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, floatToBits, bitsToFloat } from './raw-support.mjs';
import { ConstructionPacketView, createConstructionPacketHeader, finalizeConstructionPacket, initializeConstructionPacket, writeConstructionPacket } from '../www/core/raw-construction-packet.js';

// 원본 관측 행이다(첫 줄은 열 이름). 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E134)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'construction-packet-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'Write' || row[0] === 'Finalize');

// 전체 패킷 크기와 관측 행 수이다. (2026-11-08 WB04-E134)
const PACKET = 512;
const ROWS = 86;

// 16진수 문자열을 바이트로 바꾼다. (2026-11-08 WB04-E134)
const bytesOf = (text) => Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16));
// 바이트를 소문자 16진수로 만든다. (2026-11-08 WB04-E134)
const hex = (bytes) => Buffer.from(bytes).toString('hex');
// 예외가 나는지 본다. (2026-11-08 WB04-E134)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

test('패킷: 원본 10.78 관측 86행(Write 72·Finalize 14)의 512바이트 전체가 같다', { skip: rows.length === 0 }, () => {
    assert.equal(rows.length, ROWS);
    let writes = 0;
    let finalizes = 0;
    // 실제 Factory 가 원본 실행에 돌려준 SID 순서만 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        const bytes = new Uint8Array(517).fill(0xcc);
        if (row[0] === 'Write') {
            const values = row[1].split(',').map(Number);
            const header = row[2].split(',').map(Number);
            assert.equal(values.length, 10);
            assert.equal(header.length, 3);
            const notice = {
                type: values[0] & 0xff, xBits: values[1], yBits: values[2], argument: values[3] & 0xff, direction: values[4] & 0xff, player: values[5] & 0xff,
                flags: values[6] & 0xff, quality: values[7] & 0xff, timeLow: values[8], timeHigh: values[9],
                sids: row[3] === '-' ? [] : row[3].split(',').map((sid) => Number(sid) & 0xffff),
            };
            assert.equal(writeConstructionPacket(notice, { message: header[0] & 0xff, protocol: header[1] & 0xff, declaredLength: header[2] & 0xffff }, bytes), PACKET);
            writes += 1;
        } else {
            const before = bytesOf(row[3]);
            assert.equal(before.length, PACKET);
            bytes.set(before);
            let diagnostics = 0;
            finalizeConstructionPacket(bytes, Number(row[1]) !== 0, () => { diagnostics += 1; });
            assert.equal(diagnostics, row[5] === 'Assert' ? 1 : 0, row.join(' '));
            finalizes += 1;
        }
        assert.equal(hex(bytes.subarray(0, PACKET)), row[4], row.join(' '));
        // 512바이트 뒤의 호출자 버퍼는 그대로이다.
        assert.ok(bytes.subarray(PACKET).every((byte) => byte === 0xcc));
    }
    assert.equal(writes, 72);
    assert.equal(finalizes, 14);
});

test('패킷: 좌표는 수로 주어도 같은 비트로 쓰이고 비트가 있으면 NaN 페이로드를 보존한다', () => {
    const header = createConstructionPacketHeader();
    assert.deepEqual(header, { message: 0x3f, protocol: 1, declaredLength: 0 });
    const base = { type: 1, argument: 2, direction: 3, player: 4, flags: 5, quality: 6, timeLow: 7, timeHigh: 8, sids: [9] };
    const byNumber = new Uint8Array(PACKET);
    writeConstructionPacket({ ...base, x: 20.5, y: -3.25 }, header, byNumber);
    // 20.5 = 0x41a40000, -3.25 = 0xc0500000 의 리틀엔디언이다.
    assert.equal(floatToBits(20.5), 0x41a40000);
    assert.equal(floatToBits(-3.25), 0xc0500000);
    assert.equal(hex(byNumber.subarray(5, 13)), '0000a441000050c0');
    const byBits = new Uint8Array(PACKET);
    writeConstructionPacket({ ...base, xBits: 0x7fc00034, yBits: 0xffc00099 }, header, byBits);
    assert.equal(hex(byBits.subarray(5, 13)), '3400c07f9900c0ff');
    const view = new ConstructionPacketView(byBits);
    assert.equal(view.xBits(), 0x7fc00034);
    assert.equal(view.yBits(), 0xffc00099);
    assert.ok(Number.isNaN(view.x()) && Number.isNaN(view.y()));
});

test('패킷: 512바이트 미만 버퍼와 SID 20개는 효과 전에 거부하고 버퍼를 보존한다', () => {
    const bytes = new Uint8Array(511).fill(0x11);
    const header = createConstructionPacketHeader();
    const notice = { type: 0, x: 0, y: 0, argument: 0, direction: 0, player: 0, flags: 0, quality: 0, timeLow: 0, timeHigh: 0, sids: [] };
    assert.ok(throws(() => writeConstructionPacket(notice, header, bytes)));
    assert.ok(bytes.every((byte) => byte === 0x11));
    const full = new Uint8Array(PACKET).fill(0x11);
    assert.ok(throws(() => writeConstructionPacket({ ...notice, sids: new Array(20).fill(1) }, header, full)));
    assert.ok(full.every((byte) => byte === 0x11));
    assert.ok(throws(() => initializeConstructionPacket(header, bytes)));
    assert.ok(bytes.every((byte) => byte === 0x11));
    // 선언 길이는 최소 4 이고 protocol 의 상위 비트는 지운다.
    initializeConstructionPacket({ message: 0x3f, protocol: 0xff, declaredLength: 1 }, full);
    assert.equal(hex(full.subarray(0, 5)), '7f3f0400ff');
    assert.ok(full.subarray(4).every((byte) => byte === 0xff));
});

test('패킷: 최종화는 sending 0 이면 읽지 않고, 비영이면 27바이트 미만을 거부하며 255 진단이 없으면 거부한다', () => {
    finalizeConstructionPacket(new Uint8Array(0), false);
    assert.ok(throws(() => finalizeConstructionPacket(new Uint8Array(0), true)));
    assert.ok(throws(() => finalizeConstructionPacket(new Uint8Array(26), true)));
    const packet = new Uint8Array(PACKET).fill(0xff);
    // sidCount 255 에 진단 장치가 없으면 길이를 쓰기 전에 거부한다.
    assert.ok(throws(() => finalizeConstructionPacket(packet, true)));
    assert.equal(hex(packet.subarray(2, 4)), 'ffff');
    let diagnostics = 0;
    finalizeConstructionPacket(packet, true, () => { diagnostics += 1; });
    // 255*2+27 = 537 = 0x0219 이다.
    assert.equal(diagnostics, 1);
    assert.equal(hex(packet.subarray(2, 4)), '1902');
    packet[26] = 3;
    finalizeConstructionPacket(packet, true);
    assert.equal(hex(packet.subarray(2, 4)), '2100');
});

test('패킷: 뷰는 필드를 소비 위치마다 현재 바이트에서 읽고 물리 범위를 넘으면 거부한다', () => {
    const packet = new Uint8Array(PACKET);
    writeConstructionPacket({ type: 100, x: 20, y: 21, argument: 7, direction: 6, player: 3, flags: 1, quality: 9, timeLow: 0x12345678, timeHigh: 0x40208000, sids: [50, 51] },
        createConstructionPacketHeader(), packet);
    const view = new ConstructionPacketView(packet);
    assert.equal(view.type(), 100);
    assert.equal(view.x(), 20);
    assert.equal(view.y(), 21);
    assert.equal(view.argument(), 7);
    assert.equal(view.direction(), 6);
    assert.equal(view.player(), 3);
    assert.equal(view.flags(), 1);
    assert.equal(view.quality(), 9);
    assert.equal(view.timeLow(), 0x12345678);
    assert.equal(view.timeHigh(), 0x40208000);
    assert.equal(view.sidCount(), 2);
    assert.deepEqual([view.sidAt(0), view.sidAt(1)], [50, 51]);
    // 호출 경계가 버퍼를 바꾸면 다음 읽기는 현재 값을 본다.
    packet[15] = 5;
    packet[27] = 60;
    assert.equal(view.player(), 5);
    assert.equal(view.sidAt(0), 60);
    // 선언 개수와 무관하게 요청한 칸만 읽고, 버퍼 끝을 넘는 칸은 거부한다.
    assert.equal(view.sidAt(2), 0xffff);
    assert.ok(throws(() => view.sidAt(243)));
    const tiny = new ConstructionPacketView(new Uint8Array(6));
    assert.equal(tiny.type(), 0);
    assert.ok(throws(() => tiny.x()));
    assert.ok(throws(() => tiny.sidCount()));
    assert.equal(bitsToFloat(floatToBits(1.5)), 1.5);
});
