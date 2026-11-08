// 데빌메이커 80바이트 본체 raw-wind-aviary-state.js(RawWindAviaryState)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: wind-aviary-state-x86.tsv 1894행(생성자 'C' 18 / 쓰기 'W' 1108 / 읽기 'R' 768) — 입력 전체 80바이트·64바이트 패킷·부모 출력·반환 길이를 인수(서버/시각/dirty/부모/강제)별로 만들어
//    전체 80바이트 결과·패킷 64바이트·부모 출력 DWORD·반환 길이 WORD 가 같다(미작성 주변 padding 포함).
//  - 계약: 손상 host 길이를 첫 쓰기 전에 거부하고 기본 프로토콜은 실제 PE 의 01/74 를 따른다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E121: 데빌메이커 상태 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { RawWindAviaryState, WindAviaryProcessFields } from '../www/core/raw-wind-aviary-state.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E121)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'wind-aviary-state-x86.tsv')) ?? [];
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 예외가 나는지 본다. (2026-11-08 WB04-E121)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 64비트 불투명 필드를 두 DWORD 로 준비한다. 기대 결과를 계산하지 않는다. (2026-11-08 WB04-E121)
function input64(raw, offset, bits) {
    put(raw, offset, Number(bits & 0xffffffffn));
    put(raw, offset + 4, Number(bits >> 32n));
}

// 같은 입력 바이트만 준비하고 지정한 종류의 행을 모두 대조한다. 기대 출력은 원본 fixture 에서 읽는다. (2026-11-08 WB04-E121)
function check(kind, expected) {
    let count = 0;
    // 각 고유 입력의 전체 결과와 미작성 주변을 함께 대조한다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        if (row[0] !== kind) continue;
        const args = row[1].split(',').map(Number);
        const fields = new WindAviaryProcessFields();
        fields.bytes.fill(0xa5);
        const raw = fields.bytes;
        const time = new DataView(new ArrayBuffer(8));
        time.setFloat64(0, 123.25, true);
        input64(raw, 16, time.getBigUint64(0, true));
        put(raw, 24, 7);
        input64(raw, 32, 0x7ff0000000000001n);
        input64(raw, 40, 0x8000000000000000n);
        put(raw, 48, 0xfedcba98);
        put(raw, 60, 77);
        put(raw, 64, 50);
        const packet = new Uint8Array(64).fill(0xa7);
        let parent = 0xdadadada;
        let result = 0;
        if (kind === 'C') {
            assert.equal(args.length, 3);
            raw.fill(args[2] & 0xff);
            RawWindAviaryState.initialize(fields, args[0] !== 0, [0, 100.25, -5.25][args[1]]);
        } else if (kind === 'W') {
            assert.equal(args.length, 5);
            raw.fill(args[4] & 0xff, 0, 8);
            raw[8] = args[0];
            put(raw, 64, args[3]);
            result = RawWindAviaryState.writeProcess(fields, packet, args[1] !== 0, args[2] !== 0, 0x43, 0x49);
        } else {
            assert.equal(args.length, 2);
            raw.fill(args[1] & 0xff);
            // 원본 호출자가 공급한 불투명 패킷 바이트를 같은 순서로 준비한다.
            for (let index = 0; index < packet.length; index += 1) packet[index] = (index * 17 + 3) & 0xff;
            packet[8] = args[0];
            parent = RawWindAviaryState.readProcess(fields, packet);
        }
        assert.deepEqual([hex(raw), hex(packet), parent, result], [row[2], row[3], Number(row[4]), Number(row[5])], `데빌메이커 상태 ${kind} 행 ${count} ${row[1]}`);
        count += 1;
    }
    assert.equal(count, expected);
}

test('WindAviaryState: 생성자가 초기화하지 않는 서버별 상태와 padding 까지 원문 18행과 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => check('C', 18));
test('WindAviaryState: 모든 dirty BYTE 와 별도 부모 경계의 원문 1108행이 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => check('W', 1108));
test('WindAviaryState: 수신 부분 갱신과 원문 768행의 미작성 필드가 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => check('R', 768));

test('WindAviaryState: 손상 host 길이를 첫 쓰기 전에 거부하고 기본 프로토콜은 실제 PE 의 01/74 를 따른다', () => {
    const fields = new WindAviaryProcessFields();
    fields.bytes.fill(0xa5);
    const before = fields.bytes.slice();
    const packet = new Uint8Array(64);
    assert.ok(throws(() => RawWindAviaryState.writeProcess(fields, packet.subarray(0, 2), true, true)));
    assert.deepEqual(fields.bytes, before);
    assert.ok(throws(() => RawWindAviaryState.readProcess(fields, packet.subarray(0, 8))));
    assert.deepEqual(fields.bytes, before);
    packet[8] = 0x3f;
    assert.ok(throws(() => RawWindAviaryState.readProcess(fields, packet.subarray(0, 44))));
    assert.deepEqual(fields.bytes, before);
    RawWindAviaryState.writeProcess(fields, packet, false);
    assert.ok(packet[0] === 1 && packet[1] === 0x74);
    // PID NULL: 선택 필드가 없으면 빈 뷰로도 부모만 반환하고, 선택 필드가 있으면 객체 미연결로 거부한다.
    const received = new Uint8Array(64);
    received[8] = 1;
    received[9] = 0x34;
    assert.equal(RawWindAviaryState.readProcess(new Uint8Array(0), received), 0x34);
    received[8] = 3;
    assert.ok(throws(() => RawWindAviaryState.readProcess(new Uint8Array(0), received)));
});
