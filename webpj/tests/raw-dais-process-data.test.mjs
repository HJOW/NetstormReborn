// RawDaisProcessData의 선택 폭·부분 효과·실제 Host/Kernel 수명을 기준선 소스 계약으로 대조한다(WB04-E617).
// cpppj 전용 검사/원본 fixture는 없으며 실제 제단 희생·게임 플레이를 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { DaisProcessFields, DaisProcess, initializeDaisProcessFields, destructDaisProcessFields, readDaisProcessData, writeDaisProcessData } from '../www/core/raw-dais-process-data.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';

// 비트/원시 바이트 관찰 helper이며 원문 선택 필드 계산과 별개인 Buffer API로 기대값을 만든다.
const hex = (raw) => Buffer.from(raw).toString('hex');
// 실제40BYTE 이미지의 little endian 필드를 관찰한다.
const view = (fields) => new DataView(fields.bytes.buffer, fields.bytes.byteOffset, fields.bytes.byteLength);

// 생성자의 지정 필드와 쓰지 않은 padding을 전체 이미지로 검증한다.
test('DaisProcess 생성/소멸: 값 복사·원문 가상 표·padding·base 표식', () => {
    const fields = new DaisProcessFields(new Uint8Array(40).fill(0xa5)); // padding을 가진 호출자 이미지다.
    initializeDaisProcessFields(fields);
    assert.equal(hex(fields.bytes), '347850000000000000000000a5a5a5a530785000a5a5a5a5ffa5a5a5a5a5a5a50000000000000000');
    const process = new DaisProcess(fields); // 생성자 값 인자를 받은 별도 소유 이미지다.
    assert.notEqual(process.fields, fields); assert.notEqual(process.fields.bytes, fields.bytes);
    fields.bytes[12] = 0x12; assert.equal(process.fields.bytes[12], 0xa5);
    const before = process.fields.bytes.slice(); // 빈 실행/통지의 전체 이미지 기준이다.
    process.runFrame(); process.notify(1, 2, 3); assert.deepEqual(process.fields.bytes, before);
    destructDaisProcessFields(process.fields);
    assert.equal(hex(process.fields.bytes.subarray(0, 12)), 'ec195000c7cfffffc7cfffff');
    assert.deepEqual(process.fields.bytes.subarray(12), before.subarray(12));
    assert.throws(() => new DaisProcessFields(new Uint8Array(39)), /40BYTE/);
});

// 비영 dirty 상위bit와 64bit 비트는 decoder가 선택한 필드 이외에는 영향을 주지 않는다.
test('DaisProcess 수신: 신규/기존 parent 선택·기한 비트·다른 mask 무시', () => {
    // 원문 mask의 모든 조합을 같은 단독 decoder로 확인한다.
    for (let mask = 0; mask < 256; ++mask) {
        const packet = Buffer.alloc(21, 0xcc); packet[8] = mask; // parent와 기한이 함께 있는 최대 packet이다.
        packet.writeUInt32LE(0xfedcba98, 9); packet.writeBigUInt64LE(0x7ff0000000000001n, mask & 1 ? 13 : 9);
        const fields = new DaisProcessFields(new Uint8Array(40).fill(0x55)); // 다른 모든 바이트의 보존 기준이다.
        const before = fields.bytes.slice(); // 선택 필드 밖의 원시 보존 기준이다.
        assert.equal(readDaisProcessData(fields, packet, true), mask & 1 ? 0xfedcba98 : 0);
        assert.equal(view(fields).getBigUint64(32, true), mask & 2 ? 0x7ff0000000000001n : 0x5555555555555555n);
        assert.deepEqual(fields.bytes.subarray(0, 32), before.subarray(0, 32));
        assert.equal(readDaisProcessData(fields, packet, false), 0);
    }
});

// 기존 parent 영역은 실제로 읽지 않고, 선택된 기한 입력을 먼저 읽은 뒤 NULL 저장을 거부한다.
test('DaisProcess 수신 실패: 건너뛴 parent·입력 우선·NULL/물리 범위', () => {
    const packet = new Uint8Array(9); packet[8] = 1; // 기존 객체라면 없는 parent4BYTE도 건너뛴다.
    assert.equal(readDaisProcessData(null, packet, false), 0);
    assert.throws(() => readDaisProcessData(null, packet, true), RangeError);
    packet[8] = 2; assert.throws(() => readDaisProcessData(null, packet, false), RangeError);
    const wide = new Uint8Array(17); wide[8] = 2; // 입력 기한은 충분하지만 객체가 없다.
    assert.throws(() => readDaisProcessData(null, wide, false), /NULL/);
    assert.throws(() => readDaisProcessData({ bytes: new Uint8Array(39) }, wide, false), RangeError);
    assert.throws(() => readDaisProcessData(null, new Uint8Array(8), false), RangeError);
});

// 같은 deadline을 parent/force/dirty의 모든 조합으로 쓰고 원문 특유의 bit/길이 불일치도 보존한다.
test('DaisProcess writer: 모든 dirty·parent0/비영·force·현재 헤더와 비트', () => {
    // dirty 상위bit가 길이에 기여하지 않는 원문 동작을 함께 확인한다.
    for (let mask = 0; mask < 256; ++mask) {
      // parent0과 비영 payload를 별도로 확인한다.
      for (const parent of [0, 0xabcdef01]) {
       // 같은 dirty에서 일반 작성과 강제 전체 작성을 각각 확인한다.
       for (const force of [false, true]) {
        const fields = new DaisProcessFields(); fields.bytes[24] = mask; view(fields).setBigUint64(32, 0x8000000000000000n, true); // -0 기한의 비트다.
        const packet = Buffer.alloc(26, 0xcd); // 끝 canary는 writer가 건드리면 안 된다.
        const dirty = (force ? 0xff : mask) | (parent ? 1 : 0); // 원문이 마지막 mask에 기록할 비트다.
        const size = 9 + (parent ? 4 : 0) + (dirty & 2 ? 8 : 0); // parent는 dirty1이 아니라 실제 비영 payload로 결정된다.
        const expected = Buffer.alloc(26, 0xcd); // 선택 필드와 무관한 고정 헤더/버퍼의 독립 기대값이다.
        expected[0] = 3; expected[1] = 0xea; expected.writeUInt16LE(size, 2); expected.writeUInt32LE(0x87654321, 4); expected[8] = dirty;
        if (parent) expected.writeUInt32LE(parent, 9);
        if (dirty & 2) expected.writeBigUInt64LE(0x8000000000000000n, parent ? 13 : 9);
        assert.equal(writeDaisProcessData(fields, packet, 0x87654321, parent, force, 3, 0xea), size);
        assert.deepEqual(packet, expected); assert.equal(fields.bytes[24], 0);
       }
      }
    }
});

// 선택 필드가 부족해도 force/parent dirty와 앞선 packet 접두는 원문 순서대로 남는다.
test('DaisProcess writer 실패: 부분 접두·dirty 보존과 슬라이스 canary', () => {
    const fields = new DaisProcessFields(); // dirty0에서 force/parent 효과를 관찰한다.
    const backing = Buffer.alloc(30, 0xcc); const packet = backing.subarray(3, 19); // parent까지 쓰고 deadline에서 실패하는16BYTE다.
    assert.throws(() => writeDaisProcessData(fields, packet, 0x11223344, 0x55667788, true), RangeError);
    assert.equal(fields.bytes[24], 0xff);
    assert.equal(hex(packet), '016acccc44332211cc88776655cccccc');
    assert.equal(hex(backing.subarray(0, 3)), 'cccccc'); assert.equal(hex(backing.subarray(19)), 'cc'.repeat(11));
    fields.bytes[24] = 0; const short = Buffer.alloc(8, 0xcc); // parent dirty는 parent 출력의 범위 실패 전에 켜진다.
    assert.throws(() => writeDaisProcessData(fields, short, 9, 5, false), RangeError);
    assert.equal(fields.bytes[24], 1); assert.equal(hex(short), '016acccc09000000');
});

// 실제 Kernel/Host는 빈 RunFrame을 실행해도 객체를 유지하며 소멸 때 기록 표식만 적용한다.
test('DaisProcess Host: 같은 Form/부모·Kernel 빈 실행·writer·실제 소멸 수명', () => {
    const pool = new SidPool(32768, { server: true }); // 실제 단독 SID 풀이다.
    const types = Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0,
        ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) })); // 공통 Form 생성자 표다.
    const factory = new SquidFactory(pool, types); const unpop = new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)); // 같은 생성/공간 부모다.
    const destroy = new RawSquidDestroy(pool, unpop, types); const kernel = new Kernel(); const state = new SquidProcessState(); // 실제 수명 장치다.
    const host = new SquidProcessHost(pool, types, kernel, destroy, state); // 복사하지 않은 공통 Host다.
    const parent = factory.create(6, 2); const process = new DaisProcess(); // 일반 내용물 부모에 부착하며 실제 제단/희생을 생성하지 않는다.
    assert.equal(host.attach(process, 31, parent, 0x50), process);
    const form = process.form; const pid = process.pid; const packet = new Uint8Array(32); // 실제 Host가 배정한 식별자와 writer 버퍼다.
    assert.ok(form); assert.ok(pid); assert.equal(kernel.get(pid), process);
    assert.equal(process.writePacket(packet, true), 21);
    assert.equal(Buffer.from(packet).readUInt32LE(4), form); assert.equal(Buffer.from(packet).readUInt32LE(9), parent);
    kernel.runFrame(); assert.equal(kernel.get(pid), process); assert.equal(process.form, form);
    assert.equal(process.writePacket(packet, false), 9);
    host.kill(process, 0); assert.equal(kernel.get(pid), null);
    assert.equal(view(process.fields).getUint32(0, true), 0x5019ec); assert.equal(view(process.fields).getUint32(4, true), 0xffffcfc7);
});
