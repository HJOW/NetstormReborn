// Form 패킷 writer(cpppj `o/RawFormPacket`, 10.78 004ae560·004aeb40·004ac340)의 웹 대응 `core/raw-form-packet.js` 를 검사한다(4-C, WB04-E312).
//  - 원본 독립 관측 `form-packet-x86.tsv`(10.78 originals 576행: 감시/내용물 Form 작성기와 공통 삭제 작성기, 래퍼 행 포함)의 전체 64바이트 출력·반환 크기·삭제 표식·입력 슬롯 불변을 재생한다.
//  - 손 시험(cpppj `FormPacketTests.cpp` 이식 + 기준선 뒤 b8cadde14 계약): 짧은 삭제 버퍼는 출력/표식 변경 전에 거부, 삭제 writer 의 헤더 선기록과 client SID 진단 정상 복귀, 미복원 가상 표 거부, 구성 검사.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E312)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidForms } from '../www/core/squid-forms.js';
import { SQUID_FORM_VTABLES } from '../www/core/squid-vtables.js';
import { RawFormPacket, createFormDeletionStamp, createFormPacketHeaders } from '../www/core/raw-form-packet.js';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';

// 원본 관측 파일이다. 파일이 없으면 재생 검사를 건너뛴다. (2026-11-11 WB04-E312)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'form-packet-x86.tsv'));

// 16진 문자열을 BYTE 배열로 복원한다(입력과 기대 출력에 따로 적용). (2026-11-11 WB04-E312)
const hexBytes = (text) => Uint8Array.from(text.match(/../g) ?? [], (pair) => Number.parseInt(pair, 16));

// 실제 SID 경계의 큰 풀과 실제 Form 생성자로 서버 영역 SID 슬롯 셋(20000~20002)을 만든다. (2026-11-11 WB04-E312)
function makeScene() {
    const pool = new SidPool(24000, { server: true });
    const forms = new SquidForms(pool);
    const sids = { form: 20000, other: 20001, other2: 20002 };
    for (const sid of Object.values(sids)) forms.take(1, sid);
    // 상대 Form 의 getter 는 실제 생성자 가상 표와 현재 부모 WORD(+16)를 읽는다.
    for (const sid of [sids.other, sids.other2]) {
        const raw = pool.allocated(sid);
        raw[16] = 20101 & 0xff;
        raw[17] = 20101 >> 8;
    }
    return { pool, sids };
}

// 576행 재생: 열은 edition/kind/case/slot/initial/message/protocol/length/-/flags/stamp.sid/stamp.token/size/expected/stamp.sid 결과/wrapper 방송 수이다. (2026-11-11 WB04-E312)
test('Form 패킷: 원본 관측 576행의 전체 출력·표식·입력 불변', { skip: ROWS === null }, () => {
    const { pool, sids } = makeScene();
    // 입력 준비용 쓰기 대여: free 상태 BYTE 도 포함한 전체 native 입력을 받는다.
    const raw = pool.allocated(sids.form);
    let count = 0;
    // 10.78(originals) 행만 재생한다. CD 판본 행은 웹 범위 밖이다.
    for (const row of ROWS) {
        if (row[0] !== 'originals') continue;
        assert.equal(row.length, 16);
        const kind = row[1][0];
        const dependency = kind === 'D' || kind === 'W';
        const deleting = kind === 'X' || kind === 'Z';
        const input = hexBytes(row[3]);
        const initial = hexBytes(row[4]);
        const expected = hexBytes(row[13]);
        assert.equal(input.length, raw.length);
        raw.set(input);
        const headers = createFormPacketHeaders();
        const stamp = { sid: Number(row[10]), token: Number(row[11]) };
        // 선택한 실제 등록 전역(감시/내용물/삭제)의 현재 헤더를 행의 입력으로 바꾼다.
        const header = deleting ? headers.deletion : dependency ? headers.dependency : headers.content;
        Object.assign(header, { message: Number(row[5]), protocol: Number(row[6]), length: Number(row[7]) });
        const packet = Uint8Array.from(initial);
        const writer = new RawFormPacket(pool, headers, stamp);
        const size = deleting ? writer.writeDeletion(sids.form, Number(row[9]), packet) : writer.writeData(sids.form, packet);
        assert.equal(size, Number(row[12]), `행 ${count} 크기`);
        assert.deepEqual(Array.from(packet), Array.from(expected), `행 ${count} 패킷`);
        assert.deepEqual([stamp.sid, stamp.token], [Number(row[14]), Number(row[11])], `행 ${count} 표식`);
        // writer 는 입력 슬롯을 바꾸지 않는다.
        assert.deepEqual(Array.from(raw), Array.from(input), `행 ${count} 슬롯`);
        // wrapper 의 실제 방송 요청 수는 native 관찰 범위이며 W/V/Z 만 1 이다.
        assert.equal(Number(row[15]), kind === 'W' || kind === 'V' || kind === 'Z' ? 1 : 0);
        count += 1;
    }
    assert.equal(count, 576);
});

// 물리 버퍼 부족은 출력/표식 변경 전에 거부하는 호스트 경계이다. (2026-11-11 WB04-E312)
test('Form 패킷: 짧은 삭제 버퍼는 출력과 표식을 바꾸지 않음', () => {
    const { pool } = makeScene();
    const stamp = { sid: 20000, token: 0x1234 };
    const writer = new RawFormPacket(pool, createFormPacketHeaders(), stamp);
    const packet = new Uint8Array(7);
    assert.throws(() => writer.writeDeletion(20000, 0xffffffff, packet), RangeError);
    assert.deepEqual([stamp.sid, stamp.token, Array.from(packet)], [20000, 0x1234, new Array(7).fill(0)]);
    // 감시 Form 13바이트/내용물 Form 18바이트 미만도 쓰기 전에 거부한다.
    const raw = pool.allocated(20000);
    raw[0] = SQUID_FORM_VTABLES.content & 0xff;
    raw[1] = (SQUID_FORM_VTABLES.content >>> 8) & 0xff;
    raw[2] = (SQUID_FORM_VTABLES.content >>> 16) & 0xff;
    raw[3] = (SQUID_FORM_VTABLES.content >>> 24) & 0xff;
    const shortData = new Uint8Array(17).fill(0xa5);
    assert.throws(() => writer.writeData(20000, shortData), RangeError);
    assert.ok(shortData.every((byte) => byte === 0xa5));
});

// 삭제 writer 는 진단보다 먼저 현재 헤더를 실제 버퍼에 쓰고, 진단 장치가 있으면 정상 복귀 뒤 계속하며, 없으면 기존 명시 오류를 낸다. 0..4 는 client SID 로 분류하지 않는다. (2026-11-11 WB04-E312: 기준선 뒤 b8cadde14)
test('Form 패킷: 삭제의 헤더 선기록과 client SID 진단 복귀', () => {
    const { pool } = makeScene();
    const headers = createFormPacketHeaders();
    Object.assign(headers.deletion, { message: 0x2e, protocol: 0x81, length: 2 });
    const stamp = { sid: 100, token: 0xabcd };
    const writer = new RawFormPacket(pool, headers, stamp);
    // 장치가 없으면 기존 명시 오류이고, 그 전에 헤더는 이미 실제 버퍼에 있다(프로토콜 bit 7 은 지우고 길이 4 미만은 4).
    const failing = new Uint8Array(8).fill(0xee);
    assert.throws(() => writer.writeDeletion(100, 0x12345678, failing), /client SID 원본 assert/);
    assert.deepEqual(Array.from(failing), [1, 0x2e, 4, 0, 0xee, 0xee, 0xee, 0xee]);
    assert.deepEqual([stamp.sid, stamp.token], [100, 0xabcd]);
    // 장치가 반환하면 계속 진행해 SID·표식 소비·flags 상위 WORD 를 쓴다.
    const seen = [];
    const packet = new Uint8Array(10).fill(0xdd);
    const size = writer.writeDeletion(100, 0x12345678, packet, (...args) => { seen.push(args); });
    assert.equal(size, 8);
    assert.deepEqual(seen, [['!isClientSid()', '\\Ns\\O\\Squid.cpp', 0x2f6]]);
    // 표식이 일치해 token 낮은 WORD 를 합치고 sid 만 0 으로 지운다: (0x1234 & 0xfff0) | 0xabcd = 0xbbfd.
    assert.deepEqual(Array.from(packet), [1, 0x2e, 4, 0, 100, 0, 0xfd, 0xbb, 0xdd, 0xdd]);
    assert.deepEqual([stamp.sid, stamp.token], [0, 0xabcd]);
    // 서버 영역 SID 와 0..4 는 진단 없이 쓴다. 일치하지 않는 표식은 소비하지 않는다.
    seen.length = 0;
    stamp.sid = 20000;
    stamp.token = 0x7777;
    for (const sid of [0, 4, 15000, 20000]) {
        const out = new Uint8Array(8);
        writer.writeDeletion(sid, 0, out, (...args) => { seen.push(args); });
        assert.deepEqual([out[4] | (out[5] << 8), seen.length], [sid, 0], `sid ${sid}`);
    }
    assert.deepEqual([stamp.sid, stamp.token], [0, 0x7777]);
    // 표식이 없으면 최근 표식 WORD 는 0 이다.
    const plain = new Uint8Array(8);
    writer.writeDeletion(20000, 0xfffffff0, plain);
    assert.deepEqual(Array.from(plain.subarray(6, 8)), [0xf0, 0xff]);
});

// 미복원 가상 표의 Form 은 거부하고, 서버 영역이 아닌 부모/상대 Form 과 감시 Form 의 server assert 도 원문 경로로 거부한다. (2026-11-11 WB04-E312)
test('Form 패킷: 미복원 가상 표와 client 부모·server assert 거부', () => {
    const { pool, sids } = makeScene();
    const writer = new RawFormPacket(pool, createFormPacketHeaders(), createFormDeletionStamp());
    const packet = new Uint8Array(32);
    // 다른 Form 가상+54 writer 는 미복원이다(Gump/Process 와 알 수 없는 표).
    const raw = pool.allocated(sids.form);
    const setTable = (table) => { for (let index = 0; index < 4; index += 1) raw[index] = (table >>> (8 * index)) & 0xff; };
    for (const table of [SQUID_FORM_VTABLES.gump, SQUID_FORM_VTABLES.process, 0x12345678]) {
        setTable(table);
        assert.throws(() => writer.writeData(sids.form, packet), /미복원/);
    }
    // 부모 getter 도 알 수 없는 표를 거부한다.
    assert.throws(() => writer.parent(sids.form), /미복원/);
    // 감시 Form: 부모가 client SID(5..14999)이면 거부한다.
    setTable(SQUID_FORM_VTABLES.depend);
    raw[16] = 100;
    raw[17] = 0;
    assert.throws(() => writer.writeData(sids.form, packet), /client SID 원본 assert/);
    // 부모를 서버 영역으로 돌리면 통과하고, 상대 Form 의 부모(20101)도 서버 영역이다.
    raw[16] = 20101 & 0xff;
    raw[17] = 20101 >> 8;
    raw[18] = sids.other & 0xff;
    raw[19] = sids.other >> 8;
    assert.equal(writer.writeData(sids.form, packet), 13);
    // 상대 Form 의 부모가 client SID 이면 거부한다.
    const other = pool.allocated(sids.other);
    other[16] = 7;
    other[17] = 0;
    assert.throws(() => writer.writeData(sids.form, packet), /client SID 원본 assert/);
    other[16] = 20101 & 0xff;
    other[17] = 20101 >> 8;
    // 상대 Form 자체가 client SID(100)이면 거부한다.
    raw[20] = 100;
    raw[21] = 0;
    assert.throws(() => writer.writeData(sids.form, packet), /client SID 원본 assert/);
    raw[20] = 0;
    raw[21] = 0;
    // 경계: 부모 SID 4 는 client 로 분류하지 않고, 서버 영역 첫 번호 15000 은 서버 번호이다(5..14999 만 거부).
    for (const parent of [0, 4, 15000]) {
        raw[16] = parent & 0xff;
        raw[17] = parent >> 8;
        assert.equal(writer.writeData(sids.form, packet), 13, `부모 ${parent}`);
    }
    raw[16] = 14999 & 0xff;
    raw[17] = 14999 >> 8;
    assert.throws(() => writer.writeData(sids.form, packet), /client SID 원본 assert/);
    raw[16] = 5;
    raw[17] = 0;
    assert.throws(() => writer.writeData(sids.form, packet), /client SID 원본 assert/);
    // 서버 권한이 없는 풀에서는 감시 Form 작성을 거부한다.
    const client = new SidPool(24000, { server: false });
    const clientForms = new SquidForms(client);
    clientForms.take(1, 20000);
    const clientRaw = client.allocated(20000);
    clientRaw[16] = 20101 & 0xff;
    clientRaw[17] = 20101 >> 8;
    assert.throws(() => new RawFormPacket(client, createFormPacketHeaders(), createFormDeletionStamp()).writeData(20000, packet), /server assert/);
});

// 구성: SID 풀·헤더 묶음·표식이 모두 필요하다. 현재 헤더는 호출마다 읽는다(요청 중 변경 반영). (2026-11-11 WB04-E312)
test('Form 패킷: 구성 검사와 현재 헤더 재조회', () => {
    const { pool } = makeScene();
    const headers = createFormPacketHeaders();
    const stamp = createFormDeletionStamp();
    assert.deepEqual(headers, { dependency: { message: 0x65, protocol: 1, length: 13 }, content: { message: 0x2d, protocol: 1, length: 18 }, deletion: { message: 0x2e, protocol: 1, length: 8 } });
    assert.deepEqual(stamp, { sid: 0, token: 0 });
    assert.throws(() => new RawFormPacket(null, headers, stamp), TypeError);
    assert.throws(() => new RawFormPacket({}, headers, stamp), TypeError);
    assert.throws(() => new RawFormPacket(pool, { dependency: {}, content: {} }, stamp), TypeError);
    assert.throws(() => new RawFormPacket(pool, null, stamp), TypeError);
    assert.throws(() => new RawFormPacket(pool, headers, null), TypeError);
    const writer = new RawFormPacket(pool, headers, stamp);
    const first = new Uint8Array(8);
    writer.writeDeletion(20000, 0, first);
    headers.deletion.message = 0x77;
    headers.deletion.length = 0x1234;
    const second = new Uint8Array(8);
    writer.writeDeletion(20000, 0, second);
    assert.deepEqual([first[1], second[1], second[2] | (second[3] << 8)], [0x2e, 0x77, 0x1234]);
    // 선언 길이는 WORD 로 취급하고 4 미만(0 포함)은 4 로 쓴다: 0x10001 은 WORD 1 이라 4.
    for (const [length, wire] of [[0, 4], [3, 4], [4, 4], [5, 5], [0xffff, 0xffff], [0x10001, 4]]) {
        headers.deletion.length = length;
        const out = new Uint8Array(8);
        writer.writeDeletion(20000, 0, out);
        assert.equal(out[2] | (out[3] << 8), wire, `길이 ${length}`);
    }
});
