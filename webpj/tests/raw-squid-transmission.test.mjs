// 공통 Squid 전송(cpppj `o/RawSquidTransmission`, 10.78 004ac260·004ad170·00432fe0·00433100)의 웹 대응 `core/raw-squid-transmission.js` 를 검사한다(4-C, WB04-E313).
// 이 몸체의 35BYTE 갱신 쓰기와 압축 인코더에는 cpppj 에도 전용 원본 관측 fixture 가 없다(독립 프레임 검사가 NULL 연결 표로 간접 통과). 그래서
//  - 35BYTE 갱신 패킷은 슬롯 필드 위치를 손으로 계산한 기대 바이트로 대조하고,
//  - 압축 인코더는 손 계산 기대 바이트(마스크 비트·가변 데이터 순서·길이)와, 수신 쪽 복호 규칙(cpppj `RawSquidCompressedReceive` 의 필드 순서를 시험 안에 독립 구현한 복호기)으로 되돌린 왕복 일치를 확인한다.
//  - 송신 선택(`Transmit`)의 진단·선택자·대상·mutable 경로, 같은 owner 모드, 누적 로그를 계약으로 검사한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E313)
import assert from 'node:assert/strict';
import test from 'node:test';
import { SidPool } from '../www/core/sid-pool.js';
import { SQUID_BASE_VTABLE, SQUID_FORM_VTABLES } from '../www/core/squid-vtables.js';
import { CRAZY_PLAYER_ID, RawSquidTransmission, TRANSMIT_BUFFER_SIZE, createSquidTransmissionState } from '../www/core/raw-squid-transmission.js';

// Native 진단 장치가 받는 원문 파일 문자열(`\Ns\O\Squid.cpp`)이다. (2026-11-11 WB04-E313)
const TRANS_FILE = '\\Ns\\O\\Squid.cpp';

// 바이트를 16진 문자열로 바꾼다. (2026-11-11 WB04-E313)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// 단정도 float 의 little endian 바이트 네 개이다. (2026-11-11 WB04-E313)
function floatBytes(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return Array.from(new Uint8Array(view.buffer));
}

// 풀의 물리 슬롯에 원문 필드를 쓴다: vt(0..3)·type(10)·state(11)·quantity(12..13)·x(14..17)·y(18..21)·hp(26..27)·owner(34)·z(35)·extra32(36..39)·extra(40)·tail(42..49). (2026-11-11 WB04-E313)
function setRaw(pool, sid, fields) {
    const raw = pool.physical(sid);
    const put = (offset, value, width) => { for (let index = 0; index < width; index += 1) raw[offset + index] = (value >>> (8 * index)) & 0xff; };
    raw.fill(0);
    put(0, fields.vt ?? SQUID_BASE_VTABLE, 4);
    raw[10] = fields.type ?? 82;
    raw[11] = fields.state ?? 0;
    put(12, fields.quantity ?? 0, 2);
    raw.set(floatBytes(fields.x ?? 0), 14);
    raw.set(floatBytes(fields.y ?? 0), 18);
    put(26, fields.hp ?? 0, 2);
    raw[34] = fields.owner ?? 0;
    raw[35] = fields.z ?? 0;
    put(36, fields.extra32 ?? 0, 4);
    raw[40] = fields.extra ?? 0;
    raw.set(fields.tail ?? new Array(8).fill(0), 42);
}

// 장면: 풀·전역·장치 기록을 한곳에 만든다. 타입 레코드는 번호별 flags1/zOrder, 최대 체력은 번호별 표이다. (2026-11-11 WB04-E313)
function makeScene({ compress = 0 } = {}) {
    const pool = new SidPool(24000, { server: true });
    const state = createSquidTransmissionState();
    state.compress = compress;
    const log = [];
    const scene = {
        pool, state, log,
        types: new Map(), maxHp: new Map(), connected: new Set([3]), processSize: 12,
    };
    scene.hooks = {
        type: (number) => scene.types.get(number) ?? { flags1: 0, zOrder: 0 },
        defaultType: () => 82,
        maxHp: (number) => scene.maxHp.get(number) ?? 0,
        broadcast: (packet) => { log.push(['broadcast', Array.from(packet)]); },
        trace: (level, text) => { log.push(['trace', level, text]); },
        assertion: (expression, file, line) => { log.push(['assert', expression, file, line]); },
        connected: (target) => scene.connected.has(target),
        send: (target, packet) => { log.push(['send', target, Array.from(packet)]); },
        processPacket: (sid, buffer, argument) => { log.push(['process', sid, argument]); buffer[0] = 0x99; return scene.processSize; },
    };
    scene.wire = () => new RawSquidTransmission(pool, state, scene.hooks);
    return scene;
}

// 35BYTE 갱신: 헤더 [1, 0x2c, 27, 0] + 슬롯 필드 위치(+4 SID, +6 x,y, +14 수량, +16 타입, +17 상태, +18 owner, +19 꼬리 8, +27 HP, +29 z, +30 extra32, +34 extra)이며 반환 35 뒤 버퍼는 보존한다. (2026-11-11 WB04-E313)
test('Squid 전송: 35BYTE 갱신 패킷의 필드 위치와 버퍼 보존', () => {
    const scene = makeScene({ compress: 0 });
    setRaw(scene.pool, 20000, { type: 82, state: 0x31, quantity: 0x0123, x: 12.5, y: 200, hp: 0x0456, owner: 5, z: 0xfd, extra32: 0x01020304, extra: 0x77, tail: [1, 2, 3, 4, 5, 6, 7, 8] });
    scene.state.buffer.fill(0xcc);
    scene.wire().pop(20000, 0);
    const expected = '012c1b00' + '204e' + '0000484100004843' + '2301' + '52' + '31' + '05' + '0102030405060708' + '5604' + 'fd' + '04030201' + '77';
    assert.deepEqual(scene.log.map((entry) => entry[0]), ['broadcast']);
    assert.equal(hex(scene.log[0][1]), expected);
    assert.equal(scene.log[0][1].length, 35);
    // 반환 크기 뒤의 옛 버퍼는 보존한다.
    assert.ok(scene.state.buffer.subarray(35).every((byte) => byte === 0xcc));
    // 슬롯은 바뀌지 않는다.
    assert.equal(scene.pool.slot(20000)[10], 82);
});

// 수신 쪽 복호 규칙(cpppj RawSquidCompressedReceive 의 순서)을 시험 안에 독립 구현한 복호기이다: 마스크 비트 0..15 를 순서대로 소비하고 생략 필드는 기본값을 채운다. (2026-11-11 WB04-E313)
function decodeCompressed(packet, ctx) {
    const count = packet[8];
    const data = packet.subarray(9, 9 + count);
    const mask = packet[6] | (packet[7] << 8);
    const image = new Uint8Array(35);
    image.set([ctx.update.protocol & 0x7f, ctx.update.message, ctx.update.length & 0xff, ctx.update.length >> 8]);
    image[4] = packet[4];
    image[5] = packet[5];
    image.set([0x00, 0x40, 0x1c, 0xc6, 0x00, 0x40, 0x1c, 0xc6], 6);
    image[16] = ctx.defaultType;
    image[18] = ctx.lastOwner & 0xff;
    let index = 0;
    // 선택 필드의 바이트를 순서대로 소비한다.
    const copy = (destination, width) => {
        assert.ok(index + width <= count, '복호기의 미정의 스택 읽기');
        image.set(data.subarray(index, index + width), destination);
        index += width;
    };
    for (let field = 0; field < 16; field += 1) {
        if (mask & (1 << field)) {
            switch (field) {
            case 0: {
                copy(6, 1);
                const x = image[6];
                image.set(floatBytes(x), 6);
                copy(10, 1);
                const y = image[10];
                image.set(floatBytes(y), 10);
                break;
            }
            case 1: copy(6, 8); break;
            case 2: copy(14, 1); break;
            case 3: copy(14, 2); break;
            case 4: copy(16, 1); break;
            case 5: copy(17, 1); break;
            case 6: image[17] |= 0x10; break;
            case 7: image[18] = ctx.lastOwner & 0xff; break;
            case 8: copy(18, 1); ctx.lastOwner = image[18]; break;
            case 9: copy(27, 1); break;
            case 10: copy(27, 2); break;
            case 11: copy(29, 1); break;
            case 12: copy(30, 1); break;
            case 13: copy(34, 1); break;
            default: break;
            }
        }
        // 비트 4 가 생략되어도 같은 default TYPE 의 HP/zOrder 를 채운다.
        if (field === 4) {
            const selected = ctx.types.get(image[16]) ?? { flags1: 0, zOrder: 0 };
            if (selected.flags1 & 0x10) {
                const hp = ctx.maxHp.get(image[16]) ?? 0;
                image[27] = hp & 0xff;
                image[28] = (hp >> 8) & 0xff;
            }
            image[29] = selected.zOrder & 0xff;
        }
    }
    assert.equal(index, count, '복호기가 모든 데이터를 소비');
    return image;
}

// 손 계산 기대 바이트: 바이트 좌표·수량 0·기본 타입·상태 0·새 owner·기본 HP/z·extra32 0 이면 마스크 0x2101, 데이터 [x, y, owner, extra] 이다. (2026-11-11 WB04-E313)
test('Squid 전송: 압축 갱신의 손 계산 바이트와 길이', () => {
    const scene = makeScene({ compress: 1 });
    setRaw(scene.pool, 20000, { type: 82, x: 12, y: 200, owner: 5, extra: 0x77 });
    scene.wire().pop(20000, 0);
    // 압축 헤더 길이는 DWORD 입력(초기 0)이라 4 미만이면 4 로 쓰고, 가변 데이터 길이를 미리 채우지 않는다.
    assert.equal(hex(scene.log[0][1]), '017a0400' + '204e' + '0121' + '04' + '0cc80577');
    assert.deepEqual([scene.state.lastOwner, scene.state.compressedCount, scene.state.compressedBytes], [5, 1, 13]);
    // 큰 버퍼의 나머지 영역에서는 반환 길이(13)만 방송한다.
    assert.equal(scene.log[0][1].length, 13);
});

// 모든 필드 비트를 지나가는 왕복: 인코더 출력 → 독립 복호기 → 원본 35BYTE(꼬리 19..26 은 압축에 실리지 않음). (2026-11-11 WB04-E313)
test('Squid 전송: 압축 인코더 출력은 수신 규칙으로 되돌려 원본과 같음', () => {
    const scene = makeScene({ compress: 1 });
    scene.types.set(82, { flags1: 0, zOrder: 0 });
    scene.types.set(20, { flags1: 0x10, zOrder: -3 });
    scene.maxHp.set(20, 500);
    // 기본 HP 의 높은 바이트가 0 인 타입: 1바이트 HP 는 수신 쪽이 높은 바이트를 기본값에서 가져오므로 왕복이 이 경우에만 같다.
    scene.types.set(21, { flags1: 0x10, zOrder: -3 });
    scene.maxHp.set(21, 200);
    scene.state.update = { message: 0x2c, protocol: 1, length: 27 };
    scene.hooks.broadcast = (packet) => { scene.last = Uint8Array.from(packet); };
    const cases = [
        { name: '바이트 좌표·기본 타입', fields: { x: 12, y: 200, owner: 5, extra: 7 } },
        { name: '실수 좌표', fields: { x: 12.5, y: 0.25, owner: 6, extra: 1 } },
        { name: '음수·범위 밖 좌표', fields: { x: -1, y: 256, owner: 6, extra: 1 } },
        { name: '수량 1바이트', fields: { x: 1, y: 2, quantity: 200, owner: 6, extra: 1 } },
        { name: '수량 2바이트', fields: { x: 1, y: 2, quantity: 300, owner: 6, extra: 1 } },
        { name: '다른 타입·HP 기본·z 기본', fields: { type: 20, x: 1, y: 2, hp: 500, z: 0xfd, owner: 7, extra: 1 } },
        { name: 'HP 1바이트', fields: { type: 21, x: 1, y: 2, hp: 100, z: 0xfd, owner: 7, extra: 1 } },
        { name: 'HP 2바이트·z 다름', fields: { type: 20, x: 1, y: 2, hp: 0x1234, z: 5, owner: 7, extra: 1 } },
        { name: '음수 HP', fields: { type: 20, x: 1, y: 2, hp: 0xfff0, z: 0xfd, owner: 7, extra: 1 } },
        { name: '상태 일반 비트', fields: { x: 1, y: 2, state: 0x25, owner: 6, extra: 1 } },
        { name: '상태 0x10 만', fields: { x: 1, y: 2, state: 0x10, owner: 6, extra: 1 } },
        { name: '상태 0x35', fields: { x: 1, y: 2, state: 0x35, owner: 6, extra: 1 } },
        { name: 'extra32 낮은 BYTE', fields: { x: 1, y: 2, extra32: 0x2a, owner: 6, extra: 1 } },
    ];
    // 사례마다 압축 출력을 만들고 같은 규칙으로 복호한 뒤 원본 35BYTE 와 같은지 본다.
    for (const { name, fields } of cases) {
        const sid = 20001;
        setRaw(scene.pool, sid, fields);
        scene.state.sameOwnerEnabled = 0;
        scene.state.lastOwner = 0;
        const owner = scene.pool.slot(sid)[34];
        // 비압축 기준 이미지(캐시 상태 0 에서의 같은 시작 owner)이다.
        const reference = makeScene({ compress: 0 });
        setRaw(reference.pool, sid, fields);
        reference.hooks.broadcast = (packet) => { reference.last = Uint8Array.from(packet); };
        reference.wire().pop(sid, 0);
        scene.wire().pop(sid, 0);
        const decoded = decodeCompressed(scene.last, { update: scene.state.update, defaultType: 82, lastOwner: 0, types: scene.types, maxHp: scene.maxHp });
        const original = reference.last;
        // 꼬리(19..26)는 압축에 실리지 않고, 헤더 길이는 복호기가 갱신 헤더로 채운다.
        for (let index = 0; index < 35; index += 1) {
            if (index >= 19 && index <= 26) continue;
            assert.equal(decoded[index], original[index], `${name} 바이트 ${index}`);
        }
        assert.equal(scene.state.lastOwner, owner, name);
    }
});

// 같은 owner 모드: 모드가 켜져 있고 마지막 owner 와 같으면 마스크 0x80 으로 생략하고, 다르면 owner 를 싣고 캐시를 갱신한다. 모드 변경은 캐시를 -99 로 되돌린다. (2026-11-11 WB04-E313)
test('Squid 전송: 같은 owner 모드의 생략과 캐시 초기화', () => {
    const scene = makeScene({ compress: 1 });
    const wire = scene.wire();
    scene.hooks.broadcast = (packet) => { scene.last = Uint8Array.from(packet); };
    setRaw(scene.pool, 20000, { x: 1, y: 2, owner: 5, extra: 9 });
    wire.sameOwnerMode(1);
    assert.deepEqual([scene.state.sameOwnerEnabled, scene.state.lastOwner], [1, CRAZY_PLAYER_ID]);
    // 첫 송신은 캐시가 -99 라 owner 를 싣고 캐시를 5 로 만든다(마스크 bit 8).
    wire.pop(20000, 0);
    assert.equal(scene.last[6] | (scene.last[7] << 8), 0x2101);
    assert.equal(scene.state.lastOwner, 5);
    // 둘째 송신은 같은 owner 라 bit 7(0x80)만 켜고 owner 바이트는 싣지 않는다: 데이터 [1, 2, 9].
    wire.pop(20000, 0);
    assert.equal(scene.last[6] | (scene.last[7] << 8), 0x2081);
    assert.equal(scene.last[8], 3);
    // 다른 owner 는 싣고 캐시를 갱신한다.
    setRaw(scene.pool, 20000, { x: 1, y: 2, owner: 6, extra: 9 });
    wire.pop(20000, 0);
    assert.equal(scene.last[6] | (scene.last[7] << 8), 0x2101);
    assert.equal(scene.state.lastOwner, 6);
    // 모드가 같아도 캐시는 항상 초기화한다. 모드 0 이면 같은 owner 도 항상 싣는다.
    wire.sameOwnerMode(1);
    assert.equal(scene.state.lastOwner, CRAZY_PLAYER_ID);
    wire.sameOwnerMode(0);
    wire.pop(20000, 0);
    wire.pop(20000, 0);
    assert.equal(scene.last[6] | (scene.last[7] << 8), 0x2101);
});

// 255 번째마다 누적 압축 비율(%)을 수준 4 로그로 남기고, 그 밖은 로그가 없다. (2026-11-11 WB04-E313)
test('Squid 전송: 압축 누적 비율 로그는 255 번째마다', () => {
    const scene = makeScene({ compress: 1 });
    setRaw(scene.pool, 20000, { x: 12, y: 200, owner: 5, extra: 0x77 });
    scene.state.compressedCount = 253;
    scene.state.compressedBytes = 1200;
    const wire = scene.wire();
    wire.pop(20000, 0);
    // 254 번째: 로그 없음.
    assert.deepEqual(scene.log.filter((entry) => entry[0] === 'trace'), []);
    wire.pop(20000, 0);
    // 255 번째: 누적 바이트 1200 + 13*2 = 1226, 비율 1226*100 / (255*35) = 13.7 → 13(정수 나눗셈이며 반올림이 아님, 분모 255*34 였다면 14).
    assert.deepEqual(scene.log.filter((entry) => entry[0] === 'trace'), [['trace', 4, 'compressedUpdateSquid 13%\n']]);
    assert.deepEqual([scene.state.compressedCount, scene.state.compressedBytes], [255, 1226]);
    // 로그 장치가 없으면 도달한 위치에서 거부한다(그 전의 누적은 이미 갱신됨).
    const bare = makeScene({ compress: 1 });
    setRaw(bare.pool, 20000, { x: 12, y: 200, owner: 5, extra: 0x77 });
    bare.state.compressedCount = 254;
    delete bare.hooks.trace;
    assert.throws(() => bare.wire().pop(20000, 0), /선택된 실제 장치 미연결/);
    // 압축 모드 0 이면 압축 출력과 누적은 없다.
    const off = makeScene({ compress: 0 });
    setRaw(off.pool, 20000, { x: 12, y: 200, owner: 5, extra: 0x77 });
    off.wire().pop(20000, 0);
    assert.deepEqual([off.state.compressedCount, off.state.compressedBytes, off.log[0][1].length], [0, 0, 35]);
});

// 송신 선택: 클라이언트 SID 진단(임시 허용 관문), 선택자, 대상 -1/연결 관문, mutable 경로, 장치 누락 위치를 확인한다. (2026-11-11 WB04-E313)
test('Squid 전송: 선택자·대상·진단과 mutable 경로', () => {
    const scene = makeScene({ compress: 0 });
    setRaw(scene.pool, 20000, { x: 1, y: 2 });
    setRaw(scene.pool, 100, { x: 1, y: 2 });
    const wire = scene.wire();
    // 클라이언트 SID(5..14999)는 진단 장치가 불린 뒤(반환하면) 계속 전송한다.
    wire.pop(100, 0);
    assert.deepEqual(scene.log.map((entry) => entry[0]), ['assert', 'broadcast']);
    assert.deepEqual(scene.log[0], ['assert', '!isClientSid()', '\\Ns\\O\\Squid.cpp', 0x2d8]);
    // 임시 허용 관문이 켜져 있으면 진단이 없다. 서버 영역과 0..4 도 진단이 없다.
    scene.log.length = 0;
    scene.state.clientSidAllowed = 1;
    wire.pop(100, 0);
    scene.state.clientSidAllowed = 0;
    setRaw(scene.pool, 4, { x: 1, y: 2 });
    wire.pop(4, 0);
    wire.pop(20000, 0);
    assert.deepEqual(scene.log.map((entry) => entry[0]), ['broadcast', 'broadcast', 'broadcast']);
    // 대상 지정 송신: 연결 관문(3 만 연결)이 참이면 send, 거짓이면 아무 일도 없다. 방송으로 대체하지 않는다.
    scene.log.length = 0;
    wire.sendTo(20000, 3, 5);
    wire.sendTo(20000, 4, 5);
    assert.deepEqual(scene.log.map((entry) => [entry[0], entry[1]]), [['send', 3]]);
    assert.equal(scene.log[0][2].length, 35);
    // 선택자 -2 는 현재 +58(공통 삭제 8바이트)이고 flags 상위 WORD 를 쓴다. target 은 첫 인자 그대로이다.
    scene.log.length = 0;
    wire.transmit(20000, 3, -2, 99, 0x12340000);
    assert.equal(hex(scene.log[0][2]), '012e0800' + '204e' + '3012');
    // mutable 장치가 있으면 전체 2048BYTE 버퍼를 대상별로 넘긴다(방송/송신 장치 대신).
    scene.log.length = 0;
    scene.hooks.broadcastMutable = (buffer) => { scene.log.push(['broadcastMutable', buffer.length, buffer]); };
    scene.hooks.sendMutable = (target, buffer) => { scene.log.push(['sendMutable', target, buffer.length]); };
    wire.pop(20000, 0);
    wire.sendTo(20000, 3, 0);
    wire.sendTo(20000, 5, 0);
    assert.deepEqual(scene.log.map((entry) => entry.slice(0, 3).map((value) => (value instanceof Uint8Array ? 'buffer' : value))), [['broadcastMutable', 2048, 'buffer'], ['sendMutable', 3, 2048]]);
    assert.equal(scene.log[0][2], scene.state.buffer);
    // 선택한 장치(연결 관문/송신)가 없으면 도달한 위치에서 거부한다.
    const bare = makeScene({ compress: 0 });
    setRaw(bare.pool, 20000, { x: 1, y: 2 });
    delete bare.hooks.connected;
    assert.throws(() => bare.wire().sendTo(20000, 3, 0), /선택된 실제 장치 미연결/);
    delete bare.hooks.broadcast;
    assert.throws(() => bare.wire().pop(20000, 0), /선택된 실제 장치 미연결/);
    delete bare.hooks.assertion;
    assert.throws(() => bare.wire().pop(100, 0), /선택된 실제 장치 미연결/);
});

// 현재 +54 의 분배: 빈 writer 표는 0(송신 없음), ProcessForm 은 실제 writer 장치(인자 전달), 두 Form 은 Form writer, 미복원 표는 거부한다. 2048 이상 크기는 진단이다. (2026-11-11 WB04-E313)
test('Squid 전송: 가상 표별 writer 분배와 크기 진단', () => {
    const scene = makeScene({ compress: 0 });
    const wire = scene.wire();
    // 보호 83 VT 중 +54 가 실제 0 인 표: 아무것도 보내지 않는다.
    setRaw(scene.pool, 20000, { vt: 0x5092d0 });
    wire.pop(20000, 0);
    assert.deepEqual(scene.log, []);
    // ProcessForm: 실제 writer 장치가 같은 버퍼에 쓰고 반환 크기만큼 방송한다(작성 전에 writer 인자를 받는다).
    setRaw(scene.pool, 20000, { vt: SQUID_FORM_VTABLES.process });
    wire.sendTo(20000, 3, 7);
    assert.deepEqual(scene.scratch ?? null, null);
    assert.deepEqual(scene.log.map((entry) => entry[0]), ['process', 'send']);
    assert.deepEqual(scene.log[0], ['process', 20000, 7]);
    assert.equal(scene.log[1][2].length, 12);
    assert.equal(scene.log[1][2][0], 0x99);
    // 장치가 없으면 도달한 위치에서 거부한다.
    delete scene.hooks.processPacket;
    assert.throws(() => scene.wire().pop(20000, 0), /선택된 실제 장치 미연결/);
    // 내용물 Form 은 18바이트 Form writer 를 쓴다(기본 헤더 0x2d/1/18).
    scene.log.length = 0;
    setRaw(scene.pool, 20000, { vt: SQUID_FORM_VTABLES.content });
    scene.pool.physical(20000)[16] = 20101 & 0xff;
    scene.pool.physical(20000)[17] = 20101 >> 8;
    wire.pop(20000, 0);
    assert.equal(scene.log[0][1].length, 18);
    assert.deepEqual(scene.log[0][1].slice(0, 4), [1, 0x2d, 18, 0]);
    // 미복원 표(Gump 등)는 Form writer 의 명시 실패를 유지한다.
    setRaw(scene.pool, 20000, { vt: SQUID_FORM_VTABLES.gump });
    assert.throws(() => wire.pop(20000, 0), /미복원/);
    // 삭제 선택자도 미복원 표는 거부하지만 네 Form 표는 공통 삭제를 쓴다.
    setRaw(scene.pool, 20000, { vt: 0x12345678 });
    assert.throws(() => wire.transmit(20000, -1, -2, 0, 0), /삭제 가상\+58 미복원/);
    scene.log.length = 0;
    for (const vt of Object.values(SQUID_FORM_VTABLES)) {
        setRaw(scene.pool, 20000, { vt });
        wire.transmit(20000, -1, -2, 0, 0);
    }
    assert.equal(scene.log.length, 4);
    assert.ok(scene.log.every((entry) => entry[0] === 'broadcast' && entry[1].length === 8));
    // 2048 바이트 이상은 진단이다(정상 복귀하면 이어서 진행): 2048 은 버퍼에 들어가 그대로 방송되고, 2049 이상은 버퍼 범위 검사로 거부한다.
    scene.log.length = 0;
    scene.hooks.processPacket = () => 2048;
    setRaw(scene.pool, 20000, { vt: SQUID_FORM_VTABLES.process });
    wire.pop(20000, 0);
    assert.deepEqual(scene.log[0], ['assert', 'transmitSize < TRANSMIT_BUFFER_SIZE', TRANS_FILE, 0x2e3]);
    assert.deepEqual([scene.log[1][0], scene.log[1][1].length], ['broadcast', 2048]);
    scene.log.length = 0;
    scene.hooks.processPacket = () => 2049;
    assert.throws(() => wire.pop(20000, 0), RangeError);
    assert.deepEqual(scene.log, [['assert', 'transmitSize < TRANSMIT_BUFFER_SIZE', TRANS_FILE, 0x2e3]]);
    // 크기 0 이하는 송신이 없다.
    scene.log.length = 0;
    scene.hooks.processPacket = () => -1;
    wire.pop(20000, 0);
    scene.hooks.processPacket = () => 0;
    wire.pop(20000, 0);
    assert.deepEqual(scene.log, []);
});

// 구성: 10.78 풀과 2048BYTE 공통 버퍼가 필요하고, 전역 초기값은 원본 PE 값이다. (2026-11-11 WB04-E313)
test('Squid 전송: 구성과 전역 초기값', () => {
    const state = createSquidTransmissionState();
    assert.equal(state.buffer.length, TRANSMIT_BUFFER_SIZE);
    assert.deepEqual(state.update, { message: 0x2c, protocol: 1, length: 27 });
    assert.deepEqual(state.compressed, { message: 0x7a, protocol: 1, length: 0 });
    assert.deepEqual(state.sharedRegular, { message: 0x7e, protocol: 1, length: 28 });
    assert.deepEqual(state.forms.deletion, { message: 0x2e, protocol: 1, length: 8 });
    assert.deepEqual([state.compress, state.clientSidAllowed, state.sameOwnerEnabled, state.lastOwner, state.compressedCount, state.compressedBytes, state.syncTarget], [1, 0, 0, 0, 0, 0, 0]);
    const pool = new SidPool(24000, { server: true });
    assert.doesNotThrow(() => new RawSquidTransmission(pool, state, {}));
    assert.throws(() => new RawSquidTransmission(null, state, {}), TypeError);
    assert.throws(() => new RawSquidTransmission({ edition: 'cd1072' }, state, {}), TypeError);
    assert.throws(() => new RawSquidTransmission(pool, null, {}), TypeError);
    assert.throws(() => new RawSquidTransmission(pool, { buffer: new Uint8Array(100) }, {}), TypeError);
});

// 압축 인코더의 "기본값이면 생략" 판단은 왕복만으로는 잡히지 않으므로(불필요하게 실어도 복호 결과가 같음) 마스크와 데이터를 손 계산으로 정확히 대조한다. (2026-11-11 WB04-E313)
test('Squid 전송: 압축 마스크·데이터의 정확한 생략 판단', () => {
    const scene = makeScene({ compress: 1 });
    scene.types.set(21, { flags1: 0x10, zOrder: -3 });
    scene.maxHp.set(21, 200);
    scene.types.set(22, { flags1: 0x10, zOrder: 0 });
    scene.maxHp.set(22, -16);
    scene.hooks.broadcast = (packet) => { scene.last = Uint8Array.from(packet); };
    // 기본 필드: 좌표(비트 0)·owner(비트 8)·extra(비트 13)는 항상 실린다.
    const base = { owner: 7, extra: 1 };
    const table = [
        // [이름, 슬롯 필드, 기대 마스크, 기대 데이터 hex]
        ['상태 0x10 만은 비트 6 만', { ...base, x: 1, y: 2, state: 0x10 }, 0x2141, '01020701'],
        ['상태 일반 비트는 비트 5 와 데이터', { ...base, x: 1, y: 2, state: 0x25 }, 0x2121, '0102250701'],
        ['HP 가 최대 HP(음수 int16)와 같으면 생략', { ...base, type: 22, x: 1, y: 2, hp: 0xfff0 }, 0x2111, '01021607' + '01'],
        ['HP 가 최대 HP·z 가 타입 z(음수)와 같으면 둘 다 생략', { ...base, type: 21, x: 1, y: 2, hp: 200, z: 0xfd }, 0x2111, '01021507' + '01'],
        ['extra32 의 가장 높은 바이트만 있어도 낮은 바이트(0)를 싣는다', { ...base, x: 1, y: 2, extra32: 0x01000000 }, 0x3101, '0102' + '07' + '00' + '01'],
        ['수량 255 는 1바이트(비트 2)', { ...base, x: 1, y: 2, quantity: 255 }, 0x2105, '0102ff0701'],
        ['수량 256 은 2바이트(비트 3)', { ...base, x: 1, y: 2, quantity: 256 }, 0x2109, '01020001' + '0701'],
        ['좌표 0 과 255 는 바이트 좌표', { ...base, x: 0, y: 255 }, 0x2101, '00ff0701'],
        ['한쪽만 실수면 8바이트 좌표(비트 1)', { ...base, x: 12, y: 0.5 }, 0x2102, '00004041' + '0000003f' + '0701'],
    ];
    // 사례마다 같은 시작 상태(캐시 -99/모드 0)에서 압축해 마스크와 데이터를 대조한다.
    for (const [name, fields, mask, data] of table) {
        setRaw(scene.pool, 20000, fields);
        scene.state.sameOwnerEnabled = 0;
        scene.state.lastOwner = 0;
        scene.wire().pop(20000, 0);
        const packet = scene.last;
        assert.equal(packet[6] | (packet[7] << 8), mask, `${name} 마스크`);
        assert.equal(hex(packet.subarray(9)), data, `${name} 데이터`);
        assert.equal(packet[8], data.length / 2, `${name} 개수`);
        assert.equal(packet.length, 9 + data.length / 2, `${name} 길이`);
    }
});

// 헤더의 프로토콜 bit 7 은 지우고, 클라이언트/서버 SID 경계(14999/15000)와 CRAZY 값, 방송 대상 분기를 확인한다. (2026-11-11 WB04-E313)
test('Squid 전송: 헤더 bit7·SID 경계·상수', () => {
    assert.equal(CRAZY_PLAYER_ID, 0xffffff9d);
    const scene = makeScene({ compress: 0 });
    setRaw(scene.pool, 14999, { x: 1, y: 2 });
    setRaw(scene.pool, 15000, { x: 1, y: 2 });
    const wire = scene.wire();
    // 14999 는 client SID 라 진단이 있고 15000 은 서버 영역 첫 번호라 진단이 없다.
    wire.pop(14999, 0);
    wire.pop(15000, 0);
    assert.deepEqual(scene.log.map((entry) => entry[0]), ['assert', 'broadcast', 'broadcast']);
    // 프로토콜 bit 7 은 지우고 선언 길이는 WORD 로 쓴다(갱신 0x81 → 1, 압축 0x82 → 2).
    scene.state.update.protocol = 0x81;
    scene.state.compressed.protocol = 0x82;
    scene.log.length = 0;
    wire.pop(15000, 0);
    assert.equal(scene.log[0][1][0], 1);
    scene.state.compress = 1;
    scene.state.compressed.length = 0x12345;
    scene.log.length = 0;
    wire.pop(15000, 0);
    assert.deepEqual(scene.log[0][1].slice(0, 4), [2, 0x7a, 0x45, 0x23]);
    scene.state.compressed.length = 3;
    scene.log.length = 0;
    wire.pop(15000, 0);
    assert.deepEqual(scene.log[0][1].slice(2, 4), [4, 0]);
    // 방송 대상 -1 만 방송하고 0 은 정상 연결 대상(관문 뒤 송신)이다.
    scene.state.compress = 0;
    scene.connected.add(0);
    scene.log.length = 0;
    wire.transmit(15000, 0, -1, 0, 0);
    assert.deepEqual(scene.log.map((entry) => entry[0]), ['send']);
    // 공통 삭제는 진단 장치(client SID)를 RawFormPacket 에 넘겨 정상 복귀하면 계속한다(장치 없이 두면 기존 명시 오류).
    scene.state.clientSidAllowed = 1;
    scene.log.length = 0;
    wire.transmit(14999, -1, -2, 0, 0);
    assert.deepEqual(scene.log.map((entry) => entry[0]), ['assert', 'broadcast']);
    assert.equal(scene.log[0][3], 0x2f6);
    const bare = makeScene({ compress: 0 });
    bare.state.clientSidAllowed = 1;
    setRaw(bare.pool, 14999, { x: 1, y: 2 });
    delete bare.hooks.assertion;
    assert.throws(() => bare.wire().transmit(14999, -1, -2, 0, 0), /client SID 원본 assert/);
    // pop 은 ProcessForm 에 writer 인자 0 을 넘기고 sendTo 는 현재 인자를 넘긴다.
    const process = makeScene({ compress: 0 });
    setRaw(process.pool, 15000, { vt: SQUID_FORM_VTABLES.process });
    process.wire().pop(15000, 0);
    process.wire().sendTo(15000, 3, 9);
    assert.deepEqual(process.log.filter((entry) => entry[0] === 'process'), [['process', 15000, 0], ['process', 15000, 9]]);
});
