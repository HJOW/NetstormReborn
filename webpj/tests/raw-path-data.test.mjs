// 경로 패킷 raw-path-data.js(RawPathData.write/writeCurrent/read·observePathData·makePathFormTransmission)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: pathdata-x86.tsv 의 `originals` 384행 — 12개 경로 프로필(0~11) × 부모 genus × 헤더 변형으로 typed 작성기(004895b0)가 쓴 패킷 길이와 2048바이트 공유 버퍼 전체(반환 길이 밖의 stale 바이트 보존 포함)가 같고 이미지는 바뀌지 않는다.
//    같은 입력을 Native 본문(writeCurrent)으로 재생해 정상 범위에서는 typed 와 같은 바이트·길이임을 교차 확인한다(진단/로그 훅은 별도 시험).
//  - cpppj 시험 이식: 서버 flags 0 부착이 같은 풀·공유 버퍼에서 송신하고(25바이트·Form/부모 연결·삭제 전파) 필수 경계(없는 transport·다른 풀·다른 프로세스·빈 경로)를 거부하는 장면.
//  - 웹 계약: Native writer 의 진단 순서(`i < nVert` 줄 0x1f/0x24/0x29)·로그 서식·DWORD 감김·WORD 증가·물리 범위, 수신 복원(방향 run 모서리 이중 추가·직접 좌표·홀수 nodeCount 진단·의식 스테이지 보정·배열 용량 진단·Prepare/5105 삭제),
//    송신 observer, 직렬화 분배기(writeFullData·packet·transmit 선택 공유).
// 10.72(CD) 행은 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E350: 6-P RawPathData 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPop, RAW_POP_RESULT } from '../www/core/squid-pop.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { RegularProcess, SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { PATH_LAYOUT, RawPathRuntime } from '../www/core/raw-path-runtime.js';
import { RawPathProcess, RawPathProcessState } from '../www/core/raw-path-process.js';
import { PATH_PACKET_PAYLOAD, RawPathData, createPathDataHeader, makePathFormTransmission, observePathData } from '../www/core/raw-path-data.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E350)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'pathdata-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 패킷 머리 원문 파일 이름과 줄이다. (2026-11-13 WB04-E350)
const PATH_HEADER_FILE = 'c:\\ns\\o\\path.h';

// 바이트 배열의 소문자 16진 문자열이다. (2026-11-13 WB04-E350)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// fixture 의 QWORD 와 같은 raw 필드를 주변 여백 변경 없이 쓴다. (2026-11-13 WB04-E350)
function put64(bytes, offset, value) {
    const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    view.setBigUint64(offset, BigInt(value), true);
}

// 단정도 좌표로 줄인 점이다. (2026-11-13 WB04-E350)
const point = (x, y) => ({ x: Math.fround(x), y: Math.fround(y) });

// native 와 같은 독립 꼭짓점/방향 입력이다(cpppj Inputs). 예상 압축 패킷은 계산하지 않는다. (2026-11-13 WB04-E350)
function inputs(profile) {
    switch (profile) {
    case 0: return [[point(10, 20)], [0]];
    case 1: return [[point(10, 20), point(10, 20), point(10, 20)], [0, 7, 2]];
    case 2: return [[point(10, 20), point(11, 20)], [0, 0]];
    case 3: return [[point(10, 20), point(41, 20)], [0, 0]];
    case 4: return [[point(10, 20), point(42, 20)], [0, 0]];
    case 5: return [[point(10, 20), point(73, 20)], [0, 0]];
    case 6: return [[point(10, 20), point(100, 110)], [0, 1]];
    case 7: return [[point(10, 20), point(10, 40), point(80, 40), point(20, 100)], [0, 2, 4, 6]];
    case 8: return [[point(10.9, 20.1), point(10.01, 20.9), point(11.9, 20.1)], [0, 1, 8]];
    case 9: return [[point(-1.9, -2.1), point(-32.9, 20.9), point(255.9, 260.1)], [0, 7, 15]];
    case 10: return [[point(255.9, 256.9), point(1.9, -1.9)], [0, -1]];
    case 11: return [[point(10, 20), point(10, 80), point(30, 80)], [0, 3, 0]];
    default: throw new Error('Path fixture 프로필');
    }
}

// 명령 꼬리 위치(패치 전용 WORD/BYTE)이다. (2026-11-13 WB04-E350)
const TAIL = 0x186e4;

// 프로필의 경로 이미지를 만든다: 꼭짓점·방향은 입력에서, Form/부모 번호·첫 도착 시각·helper·명령 꼬리는 fixture 입력에서 쓴다. (2026-11-13 WB04-E350)
function imageOf(profile, timeBits) {
    const runtime = new RawPathRuntime({ priest: (profile & 1) !== 0 });
    const [points, directions] = inputs(profile);
    runtime.setPath(points, directions);
    const image = Uint8Array.from(runtime.bytes);
    put(image, 4, 32700 + profile);
    put(image, 8, 5);
    put64(image, PATH_LAYOUT.times, timeBits);
    put(image, PATH_LAYOUT.helper, 0x12340000 + profile * 65537);
    put(image, TAIL, 0x87654321 + profile);
    put(image, TAIL + 4, 0xa5 + profile);
    return image;
}

// 행 하나의 머리와 버퍼 입력이다. (2026-11-13 WB04-E350)
function rowInputs(row) {
    const variant = Number(row[3]);
    return {
        header: { packetId: 0x30 + variant, packetFlags: variant ? 0x81 : 0xfe, packetLength: variant ? 0 : 0x8123 },
        buffer: new Uint8Array(2048).fill(variant ? 0xa5 : 0x5a),
    };
}

test('RawPathData.write: 원본 10.78 pathdata 384행 — 패킷 길이와 2048바이트 공유 버퍼 전체(stale 바이트 포함)가 같고 이미지는 불변이다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 모든 고유 입력을 재생하고 반환 길이 밖의 stale buffer 까지 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 8);
        const profile = Number(row[1]);
        const image = imageOf(profile, row[5]);
        const before = Uint8Array.from(image);
        const { header, buffer } = rowInputs(row);
        assert.equal(new RawPathData().write(image, 32700 + profile, 5, Number(row[2]), header, buffer), Number(row[6]), `행 ${count} 길이`);
        assert.equal(hex(buffer), row[7], `행 ${count} 버퍼`);
        assert.deepEqual(image, before);
        count += 1;
    }
    assert.equal(count, 384);
});

test('RawPathData.writeCurrent: 같은 384행에서 Native 본문도 typed 와 같은 길이·버퍼를 쓰고 점/run 로그를 원문 서식으로 낸다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 정상 범위 입력에서는 typed 와 Native 가 같은 바이트를 만든다(진단은 한 번도 나지 않아야 한다).
    for (const row of rows) {
        const profile = Number(row[1]);
        const runtime = new RawPathRuntime({ priest: (profile & 1) !== 0 });
        runtime.bytes.set(imageOf(profile, row[5]));
        const { header, buffer } = rowInputs(row);
        const logs = [];
        const length = new RawPathData().writeCurrent(runtime, 32700 + profile, 5, header, buffer, {
            parentGenus: (sid) => { assert.equal(sid, 5); return Number(row[2]); },
            assertion: () => assert.fail('정상 입력에서 진단이 났다'),
            trace: (text) => { logs.push(text); },
        });
        assert.equal(length, Number(row[6]), `행 ${count} 길이`);
        assert.equal(hex(buffer), row[7], `행 ${count} 버퍼`);
        // 직접 좌표(genus 0x120000)가 아니면 꼭짓점마다 점 로그가 있고, 그 사이 run 로그는 쓴 BYTE 개수와 같다.
        if ((Number(row[2]) & 0x120000) === 0) {
            const [points] = inputs(profile);
            assert.equal(logs.filter((text) => text.startsWith('[')).length, points.length, `행 ${count} 점 로그`);
            assert.equal(logs.filter((text) => text.startsWith('n[')).length, Number(row[6]) - PATH_PACKET_PAYLOAD, `행 ${count} run 로그`);
        } else {
            assert.deepEqual(logs, [], `행 ${count} 직접 좌표는 로그가 없다`);
        }
        count += 1;
    }
    assert.equal(count, 384);
});

// 로그 서식과 run 분할의 실제 문자열이다(프로필 7: 방향 0 으로 (10,20)→(10,40)). (2026-11-13 WB04-E350)
test('RawPathData.writeCurrent: 점 로그는 "[i] (x,y) , 방향", run 로그는 "n[현재 WORD] 방향 | 거리" 이다', () => {
    const runtime = new RawPathRuntime();
    runtime.setPath([point(10, 20), point(10, 80)], [0, 4]);
    const logs = [];
    const buffer = new Uint8Array(64);
    const length = new RawPathData().writeCurrent(runtime, 7, 5, createPathDataHeader(), buffer, { parentGenus: () => 0, assertion: () => assert.fail('진단'), trace: (text) => { logs.push(text); } });
    // 거리 60 은 31 + 29 로 쪼개지고 방향 4 는 짝수라 두 축 합(0+60)이다.
    assert.deepEqual(logs, ['[0] (10,20) , 0\n', '[1] (10,80) , 4\n', 'n[0] 4 | 31\n', 'n[1] 4 | 29\n']);
    assert.equal(length, 26);
    assert.deepEqual(Array.from(buffer.subarray(24, 26)), [(31 << 3) | 4, (29 << 3) | 4]);
    assert.equal(get(buffer, 13, 2), 2);
});

test('RawPathData.writeCurrent: 직접 좌표 genus(0x120000)는 나머지 꼭짓점의 절삭 좌표 두 BYTE 를 순서대로 쓰고 로그가 없다', () => {
    const runtime = new RawPathRuntime();
    runtime.setPath([point(10, 20), point(11.9, 22.5), point(255, 0)], [0, 3, 3]);
    const buffer = new Uint8Array(64).fill(0x77);
    const length = new RawPathData().writeCurrent(runtime, 9, 5, createPathDataHeader(), buffer, { parentGenus: () => 0x20000 | 0x100000, assertion: () => assert.fail('진단') });
    assert.equal(length, 28);
    assert.deepEqual(Array.from(buffer.subarray(24, 28)), [11, 22, 255, 0]);
    assert.equal(get(buffer, 13, 2), 4);
    // 같은 입력의 typed 결과와 같다.
    const typed = new Uint8Array(64).fill(0x77);
    assert.equal(new RawPathData().write(runtime, 9, 5, 0x120000, createPathDataHeader(), typed), 28);
    assert.equal(hex(typed), hex(buffer));
});

test('RawPathData.writeCurrent: 진단 순서 — 꼭짓점이 없으면 첫 읽기에서 i<nVert 0x1f, 정상 복귀 뒤에도 같은 물리를 계속 읽고 줄 0x1f/0x1f/0x24/0x1f/0x1f 순서로 반복한다', () => {
    const runtime = new RawPathRuntime();
    const lines = [];
    const buffer = new Uint8Array(64).fill(0x55);
    // 꼭짓점 개수 0: 모든 진단이 정상 복귀하고 첫 점(이미지 앞 여백)을 그대로 읽는다.
    const length = new RawPathData().writeCurrent(runtime, 1, 2, createPathDataHeader(), buffer, {
        parentGenus: () => 0x120000,
        assertion: (expression, file, line) => { assert.equal(expression, 'i < nVert'); assert.equal(file, PATH_HEADER_FILE); lines.push(line); },
    });
    assert.deepEqual(lines, [0x1f, 0x1f, 0x24, 0x1f, 0x1f]);
    assert.equal(length, 24);
    // 진단 장치가 없으면 첫 지점에서 거부하고 앞선 머리 쓰기는 남는다.
    const fresh = new Uint8Array(64).fill(0x55);
    assert.throws(() => new RawPathData().writeCurrent(runtime, 1, 2, createPathDataHeader(), fresh, { parentGenus: () => 0 }), /현재 진단 장치 미연결/);
    assert.deepEqual(Array.from(fresh.subarray(0, 8)), [1, 0x30, 4, 0, 1, 0, 2, 0]);
    // 부모 genus 장치가 없으면 고정 prefix 뒤에서 거부한다.
    const runtime2 = new RawPathRuntime();
    runtime2.setPath([point(1, 2)], [0]);
    assert.throws(() => new RawPathData().writeCurrent(runtime2, 1, 2, createPathDataHeader(), new Uint8Array(64), {}), /부모 genus 장치 미연결/);
    // 비직접 분기에서 trace 장치가 없으면 점 로그에서 거부한다.
    assert.throws(() => new RawPathData().writeCurrent(runtime2, 1, 2, createPathDataHeader(), new Uint8Array(64), { parentGenus: () => 0 }), /실제 로그 장치 미연결/);
});

test('RawPathData.writeCurrent: 송신 버퍼 물리 범위 — 쓰기 폭이 버퍼 끝을 넘으면 거부하고, 부모 genus 는 현재 packet+6 의 SID 로 한 번만 묻는다', () => {
    const runtime = new RawPathRuntime();
    runtime.setPath([point(1, 2)], [0]);
    assert.throws(() => new RawPathData().writeCurrent(runtime, 1, 2, createPathDataHeader(), new Uint8Array(23), { parentGenus: () => 0, trace: () => {} }), /현재 송신 예약 범위/);
    const asked = [];
    new RawPathData().writeCurrent(runtime, 1, 4321, createPathDataHeader(), new Uint8Array(32), { parentGenus: (sid) => { asked.push(sid); return 0; }, trace: () => {} });
    assert.deepEqual(asked, [4321]);
    // 이미지 크기가 다르면 거부한다.
    assert.throws(() => new RawPathData().writeCurrent(new Uint8Array(10), 1, 2, createPathDataHeader(), new Uint8Array(32), {}), RangeError);
});

test('RawPathData.write: 입력 검사 — 꼭짓점 0/5001 개·버퍼 24바이트 미만·이미지 크기·거리 초과는 거부하고 밖 좌표는 0 쪽 절삭한다', () => {
    const data = new RawPathData();
    const empty = new RawPathRuntime();
    assert.throws(() => data.write(empty, 1, 2, 0, createPathDataHeader(), new Uint8Array(64)), /i < nVert/);
    const runtime = new RawPathRuntime();
    runtime.setPath([point(1, 2)], [0]);
    assert.throws(() => data.write(runtime, 1, 2, 0, createPathDataHeader(), new Uint8Array(23)), RangeError);
    assert.throws(() => data.write(new Uint8Array(10), 1, 2, 0, createPathDataHeader(), new Uint8Array(64)), RangeError);
    // 꼭짓점 개수 필드가 용량(5000)을 넘으면 거부한다.
    const big = Uint8Array.from(runtime.bytes);
    put(big, 0x10, 5001);
    assert.throws(() => data.write(big, 1, 2, 0, createPathDataHeader(), new Uint8Array(64)), /i < nVert/);
    // float 좌표가 signed32 밖이면 typed 는 거부한다(Native 는 낮은 DWORD 0).
    const wild = new RawPathRuntime();
    wild.setPath([point(1, 2), { x: 3e9, y: 2 }], [0, 0]);
    assert.throws(() => data.write(wild, 1, 2, 0, createPathDataHeader(), new Uint8Array(64)), /_ftol 범위/);
    const nativeBuffer = new Uint8Array(64);
    data.writeCurrent(wild, 1, 2, createPathDataHeader(), nativeBuffer, { parentGenus: () => 0x120000 });
    assert.equal(nativeBuffer[24], 0);
    // 헤더는 최소 4 의 낮은 WORD 만 쓰고 플래그는 하위 7비트만 쓴다.
    const buffer = new Uint8Array(64);
    data.write(runtime, 1, 2, 0, { packetId: 0x44, packetFlags: 0xff, packetLength: 0x12345 }, buffer);
    assert.deepEqual(Array.from(buffer.subarray(0, 4)), [0x7f, 0x44, 0x45, 0x23]);
});

// 조립된 서버 월드 한 벌: 풀·해시·Pop/Unpop/삭제·Kernel·호스트와 부모(SID 5, 골렘 134)이다. (2026-11-13 WB04-E350)
function serverWorld() {
    const pool = new SidPool(32768, { server: true });
    const hash = new SquidHash();
    const types = Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
    types[134].footX = types[134].footY = 1;
    const spots = new Uint8Array(65536);
    const book = new SquidPostPopState();
    book.suppressed = true;
    const unpop = new SquidUnpop(pool, hash, spots);
    const pop = new SquidPop(pool, hash, spots, { postPop: new SquidPostPop(pool, types, book) });
    const kernel = new Kernel(32);
    const clock = new SquidProcessState();
    clock.boss = true;
    const deletion = [];
    const host = new SquidProcessHost(pool, types, kernel, new RawSquidDestroy(pool, unpop, types), clock, {
        asset: { selected: () => 0, emit: (event) => { assert.equal(event.effect, SQUID_DESTROY_EFFECT.transmit); deletion.push(event); } },
    });
    const parent = new SquidFactory(pool, types).create(134, 2);
    assert.equal(parent, 5);
    assert.equal(pop.pop(parent, types[134], 1, 1, 10, 20, 0), RAW_POP_RESULT.registered);
    return { pool, hash, types, spots, unpop, pop, kernel, clock, host, deletion, parent };
}

test('RawPathData 서버 flags 0 부착: 같은 풀·공유 버퍼로 25바이트를 송신하고 Form/부모 연결·삭제 전파·필수 경계 거부를 지킨다', () => {
    const world = serverWorld();
    const { pool, hash, types, spots, unpop, pop, kernel, clock, host, deletion, parent } = world;
    const header = createPathDataHeader();
    const buffer = new Uint8Array(2048).fill(0xa5);
    let sent = 0;
    let cleanup = 0;
    let sentForm = 0;
    const transport = (size) => {
        assert.equal(size, 25);
        assert.equal(get(buffer, 6, 2), parent);
        sentForm = get(buffer, 4, 2);
        assert.ok(sentForm >= pool.layout.serverFirst);
        assert.equal(get(pool.slot(parent), 6, 2), 0);
        assert.equal(get(pool.slot(sentForm), 16, 2), 0);
        assert.ok(buffer[8] === 0xa5 && buffer[9] === 0xa5 && buffer[10] === 0xa5);
        sent += 1;
    };
    host.setFormTransmit(makePathFormTransmission(pool, types, header, buffer, transport));
    const hooks = {
        unpop: (sid, flags) => { unpop.unpop(sid, types[pool.slot(sid)[10]], flags); },
        pop: (sid, at, flags) => { assert.equal(pop.pop(sid, types[pool.slot(sid)[10]], 1, 1, at.x, at.y, flags), RAW_POP_RESULT.registered); },
        repop: (sid, flags) => {
            const raw = pool.slot(sid);
            assert.equal(pop.pop(sid, types[raw[10]], 1, 1, bitsToFloat(get(raw, 14)), bitsToFloat(get(raw, 18)), flags), RAW_POP_RESULT.registered);
        },
        halt: () => 10,
        arrival: () => 1000,
        soundId: () => 0,
        stepSound: () => assert.fail('부착 검사에서 발소리는 미실행'),
        clearBlocked: () => assert.fail('부착 검사에서 겹침 정리는 미실행'),
        cleanup: (runtime, sid, flags) => { assert.ok(sid === parent && flags === 0); cleanup += 1; },
    };
    const frames = types.map(() => new FrameCodeTable([]));
    const next = new RawPathProcess(pool, hash, types, frames, types.map(() => ({ frames: [] })), spots, new RawPathProcessState(clock), hooks, {});
    next.runtime.setPath([point(10, 20), point(11, 20)], [0, 0]);
    next.prepare(0, 1, 90);
    const process = RawPathProcess.attach(host, next, parent, 0);
    try {
        assert.ok(process);
        assert.equal(sent, 1);
        assert.equal(process.form, sentForm);
        assert.equal(get(pool.slot(parent), 6, 2), sentForm);
        assert.equal(get(pool.slot(sentForm), 16, 2), parent);
        assert.equal(pool.slot(sentForm)[11] & 12, 8);
        assert.equal(get(process.runtime.bytes, 4), sentForm);
        assert.equal(get(process.runtime.bytes, 8), parent);
        // 다른 풀의 직렬화기는 송신 전에 거부한다.
        const foreign = new SidPool(32768, { server: true });
        const rejected = makePathFormTransmission(foreign, types, header, buffer, () => assert.fail('다른 풀은 송신 전 거부'));
        assert.throws(() => rejected.writeData(process, false), /풀\/부착 불일치/);
        host.kill(process, 0);
        assert.equal(cleanup, 1);
        assert.equal(kernel.size, 0);
        assert.equal(deletion.length, 1);
        assert.equal(deletion[0].sid, sentForm);
        assert.equal(get(pool.slot(parent), 6, 2), 0);
        assert.notEqual(pool.slot(sentForm)[11] & 1, 0);
        // 필수 경계: 송신 장치 없음·잘못된 판본/버퍼·다른 프로세스·빈 경로.
        assert.throws(() => makePathFormTransmission(pool, types, header, buffer, null), TypeError);
        assert.throws(() => makePathFormTransmission(pool, types.slice(1), header, buffer, () => {}), TypeError);
        assert.throws(() => makePathFormTransmission(pool, types, header, new Uint8Array(23), () => {}), TypeError);
        const adapter = makePathFormTransmission(pool, types, header, buffer, () => assert.fail('송신 검사 경계'));
        assert.throws(() => adapter.writeData(new RegularProcess(1, 0), false), /다른 프로세스의 실제 WriteData 미복원 경계/);
        assert.throws(() => new RawPathData().write(new RawPathRuntime(), 6, parent, 0, header, buffer), /i < nVert/);
    } finally {
        kernel.clear();
    }
});

test('makePathFormTransmission: Path 와 다른 프로세스 선택이 packet·writeFullData·transmit 에 공유되고 Native writer 를 force 와 함께 쓴다', () => {
    const world = serverWorld();
    const { pool, hash, types, spots, clock, parent } = world;
    const buffer = new Uint8Array(64).fill(1);
    const header = createPathDataHeader();
    const calls = [];
    const other = {
        writeData: (process, changed) => { calls.push(`other:${changed}`); return 7; },
        packet: (size) => { calls.push(`otherPacket:${size}`); return new Uint8Array(size); },
        writeFullData: (process, force) => { calls.push(`otherFull:${force}`); return 9; },
        transmit: (size) => { calls.push(`otherSend:${size}`); },
    };
    const sent = [];
    const nativeCalls = [];
    const native = makePathFormTransmission(pool, types, header, buffer, (size) => { sent.push(size); }, other, (process, target, changed, force) => { nativeCalls.push([process instanceof RawPathProcess, target === buffer, changed, force]); return 30; });
    const make = () => new RawPathProcess(pool, hash, types, types.map(() => new FrameCodeTable([])), types.map(() => ({ frames: [] })), spots, new RawPathProcessState(clock), {
        unpop: () => {}, repop: () => {}, pop: () => {}, halt: () => 10, arrival: () => 1000, soundId: () => 0, stepSound: () => {}, clearBlocked: () => {}, cleanup: () => {},
    }, {});
    const path = make();
    const regular = new RegularProcess(1, 0);
    // 다른 프로세스를 먼저 골랐으면 packet/transmit 은 other 로 간다.
    assert.equal(native.writeData(regular, true), 7);
    assert.equal(native.packet(3).length, 3);
    native.transmit(3);
    assert.deepEqual(calls, ['other:true', 'otherPacket:3', 'otherSend:3']);
    // Path 를 고르면 같은 버퍼(앞 size 바이트)와 transport 로 간다. Native writer 는 부착 전 프로세스에도 쓰인다.
    assert.equal(native.writeData(path, true), 30);
    assert.equal(native.packet(5).length, 5);
    native.transmit(5);
    assert.deepEqual(sent, [5]);
    assert.equal(native.writeFullData(path, 0xcafe), 30);
    // 다시 다른 프로세스의 전체 writer 를 고르면 선택이 갱신된다.
    assert.equal(native.writeFullData(regular, 3), 9);
    native.transmit(2);
    assert.deepEqual(calls.slice(-2), ['otherFull:3', 'otherSend:2']);
    assert.deepEqual(nativeCalls, [[true, true, true, 0], [true, true, false, 0xcafe]]);
    // Path 가 선택된 상태에서 버퍼보다 큰 크기를 빌리면 거부한다.
    native.writeData(path, false);
    assert.throws(() => native.packet(65), RangeError);
    // other 가 없으면 다른 프로세스는 모두 거부한다.
    const bare = makePathFormTransmission(pool, types, header, buffer, () => {});
    assert.throws(() => bare.writeFullData(regular, 1), /전체 writer 미연결/);
    assert.throws(() => bare.packet(1), /작성 버퍼 미연결/);
    assert.throws(() => bare.transmit(1), /송신 장치 미연결/);
    // typed 경로의 writeFullData 는 force 와 무관하게 부착된 현재 전체 경로를 쓴다.
    assert.equal(parent, 5);
});

test('observePathData: 수신은 무동작이고 송신은 nodeCount+24 의 하위 WORD 로 길이를 확정하며 -1 은 진단(정상 복귀 시 계속)이다', () => {
    const packet = new Uint8Array(32).fill(0x11);
    put(packet, 13, 7, 2);
    observePathData(packet, 0, null);
    assert.deepEqual(Array.from(packet.subarray(2, 4)), [0x11, 0x11]);
    observePathData(packet, 1, null);
    assert.deepEqual(Array.from(packet.subarray(2, 4)), [31, 0]);
    // -1 이면 진단 뒤(정상 복귀) 길이는 23 이다. 진단 장치가 없으면 거부한다.
    put(packet, 13, 0xffff, 2);
    assert.throws(() => observePathData(packet, 1, null), /진단 미연결/);
    const seen = [];
    observePathData(packet, 1, (expression, file, line) => { seen.push([expression, file, line]); });
    assert.deepEqual(seen, [['nodeCount != (short)-1', '\\Ns\\O\\_p_packets.cpp', 0x716]]);
    assert.deepEqual(Array.from(packet.subarray(2, 4)), [23, 0]);
    // 음수 nodeCount 는 signed 로 더한다(-30+24 의 하위 WORD).
    put(packet, 13, 0xffe2, 2);
    observePathData(packet, 1, () => assert.fail('진단 없음'));
    assert.deepEqual(Array.from(packet.subarray(2, 4)), [0xfa, 0xff]);
    assert.throws(() => observePathData(new Uint8Array(14), 1, null), RangeError);
});

// 수신 시험의 입력: 부모(SID 5, 골렘 134)가 있는 풀과 필수 훅이다. (2026-11-13 WB04-E350)
function receiveWorld(flags2 = 0) {
    const pool = new SidPool(32768, { server: false });
    assert.equal(pool.allocate(2), 5);
    pool.allocated(5)[10] = 134;
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    types[134].flags2 = flags2;
    const log = [];
    const hooks = {
        stagePoint: (sid, stage) => { log.push(`stage:${sid},${stage}`); return { x: 100 + stage, y: 200 + stage }; },
        metadata: (sid) => { log.push(`meta:${sid}`); return [3, 90]; },
        now: () => 50,
        clearAnimation: (sid) => { log.push(`clear:${sid}`); },
        trace: (text) => { log.push(text); },
        assertion: (expression, file, line) => { log.push(`assert:${expression}:${line}`); },
    };
    return { pool, types, hooks, log };
}

// 헤더만 채운 24바이트 + payload 수신 패킷이다. (2026-11-13 WB04-E350)
function packetOf({ parent = 5, x = 10, y = 20, count = 0, helper = 0, tail = 0, stage = 0, time = 0, payload = [] }) {
    const packet = new Uint8Array(24 + payload.length);
    put(packet, 6, parent, 2);
    put(packet, 11, helper, 2);
    put(packet, 13, count, 2);
    packet[15] = x;
    packet[16] = y;
    put(packet, 17, time >>> 0);
    put(packet, 21, tail, 2);
    packet[23] = stage;
    packet.set(payload, 24);
    return packet;
}

test('RawPathData.read: 방향 run 은 방향이 바뀌는 모서리를 이전/새 방향으로 두 번 추가하고 끝에 마지막 점을 더하며 Prepare·5105 삭제를 실행한다', () => {
    const { pool, types, hooks, log } = receiveWorld();
    const runtime = new RawPathRuntime();
    // 북(0)으로 10칸, 동(2)으로 10칸, 남동(3)으로 3칸: n 은 3, 각 BYTE = 거리<<3 | 방향.
    const packet = packetOf({ x: 10, y: 20, count: 3, helper: 0x1234, tail: 0xbeef, stage: 9, time: 80, payload: [(10 << 3) | 0, (10 << 3) | 2, (3 << 3) | 3] });
    new RawPathData().read(runtime, packet, pool, types, hooks);
    const expected = [[10, 20, 0], [10, 10, 0], [10, 10, 2], [20, 10, 2], [20, 10, 3], [23, 13, 3]];
    assert.equal(runtime.count, expected.length);
    expected.forEach(([x, y, direction], index) => {
        assert.deepEqual(runtime.point(index), { x, y });
        assert.equal(runtime.direction(index), direction);
    });
    assert.equal(runtime.helper, 0x1234);
    assert.equal(get(runtime.bytes, 0x186e4), 0xbeef);
    assert.equal(get(runtime.bytes, 0x186e8), 9);
    assert.ok(!runtime.finished && runtime.targetA === 0 && runtime.targetB === 0);
    // 시작 시각은 DWORD 80 의 0.125 배(10)이고 현재 시각 50 과의 간격이 2 초를 넘어 늦은 시작이라 첫 시각이 startTime 이다.
    assert.equal(runtime.time(0), 10);
    assert.deepEqual(log.filter((entry) => !entry.startsWith('stage')), [
        '[0] (10,20) , 0\n', 'n[0] 0 | 10\n', 'n[1] 2 | 10\n', '[1] (10,10) , 0\n', '[2] (10,10) , 2\n', 'n[2] 3 | 3\n', '[3] (20,10) , 2\n', '[4] (20,10) , 3\n', '[5] (23,13) , 3\n',
        'meta:5', 'meta:5', 'clear:5',
    ]);
});

test('RawPathData.read: 직접 좌표(genus 0x120000)는 시작점 뒤 좌표쌍을 방향 0 으로 추가하고 홀수 nodeCount 는 진단(정상 복귀)이다', () => {
    const { pool, types, hooks, log } = receiveWorld(0x120000);
    const runtime = new RawPathRuntime();
    new RawPathData().read(runtime, packetOf({ x: 1, y: 2, count: 4, payload: [30, 40, 250, 255] }), pool, types, hooks);
    assert.equal(runtime.count, 3);
    assert.deepEqual([runtime.point(0), runtime.point(1), runtime.point(2)], [{ x: 1, y: 2 }, { x: 30, y: 40 }, { x: 250, y: 255 }]);
    assert.deepEqual([0, 1, 2].map((index) => runtime.direction(index)), [0, 0, 0]);
    assert.ok(!log.some((entry) => entry.startsWith('assert')));
    // 홀수 nodeCount: 진단 줄 0x1ca 가 먼저 나고 정상 복귀하면 쌍 읽기는 index 0 만 돈다(한 쌍은 count 3 에서 index 2 를 건너뜀).
    const odd = new RawPathRuntime();
    const world = receiveWorld(0x120000);
    new RawPathData().read(odd, packetOf({ x: 1, y: 2, count: 3, payload: [30, 40, 50, 60] }), world.pool, world.types, world.hooks);
    assert.ok(world.log.includes('assert:(zwp->nodeCount & 1) == 0:458'));
    assert.equal(odd.count, 3);
});

test('RawPathData.read: 의식 스테이지 — 대상이 살아 있고 스테이지 1..4 면 시작점을 1.5 칸 안에서만 보정하고 마지막 점은 현재 스테이지 코너로 바꾼다', () => {
    // [스테이지, 부모 좌표(시작점과의 거리), 시작점 보정 기대, 마지막 점 코너 기대]
    const cases = [[2, { x: 100.5, y: 201 }, true, 2], [2, { x: 150, y: 201 }, false, 2], [0, { x: 100, y: 200 }, false, 0], [5, { x: 100, y: 200 }, false, null]];
    for (const [stage, parentAt, snapped, lastStage] of cases) {
        const { pool, types, hooks, log } = receiveWorld();
        const raw = pool.allocated(5);
        put(raw, 14, floatToBits(parentAt.x));
        put(raw, 18, floatToBits(parentAt.y));
        pool.allocate(2);
        const runtime = new RawPathRuntime();
        // 대상(SID 6, 장비가 아닌 살아 있는 슬롯)을 꼬리 WORD 로 가리킨다.
        new RawPathData().read(runtime, packetOf({ x: 7, y: 8, count: 1, tail: 6, stage, payload: [(4 << 3) | 2] }), pool, types, hooks);
        const stages = log.filter((entry) => entry.startsWith('stage')).map((entry) => entry.split(':')[1]);
        const expectedStages = [];
        if (stage > 0 && stage < 5) expectedStages.push(`6,${stage - 1}`);
        if (stage >= 0 && stage < 5) expectedStages.push(`6,${stage}`);
        assert.deepEqual(stages, expectedStages, `스테이지 ${stage}`);
        const first = runtime.point(0);
        assert.deepEqual(first, snapped ? { x: 100 + stage - 1, y: 200 + stage - 1 } : { x: 7, y: 8 }, `시작점 ${stage}`);
        const last = runtime.point(runtime.count - 1);
        assert.deepEqual(last, lastStage === null ? { x: 11, y: 8 } : { x: 100 + lastStage, y: 200 + lastStage }, `마지막 점 ${stage}`);
    }
    // 대상이 free 이면 스테이지를 읽지 않는다.
    const { pool, types, hooks, log } = receiveWorld();
    pool.allocate(2);
    pool.physical(6)[11] |= 1;
    new RawPathData().read(new RawPathRuntime(), packetOf({ x: 7, y: 8, count: 0, tail: 6, stage: 2 }), pool, types, hooks);
    assert.ok(!log.some((entry) => entry.startsWith('stage')));
});

test('RawPathData.read: 배열 용량 진단과 입력 검사 — 마지막 칸 append 는 nVert<MAX 진단 뒤 다음 append 가 거부되고, 판본/크기/부모/타입/payload/훅 부족을 거부한다', () => {
    const data = new RawPathData();
    const world = receiveWorld();
    // 4999 개가 이미 있으면 다음 append 가 용량 5000 에 닿아 진단(정상 복귀), 그다음은 배열 범위 거부이다.
    const full = new RawPathRuntime();
    full.setPath(Array.from({ length: 4999 }, () => point(1, 1)), Array.from({ length: 4999 }, () => 0));
    assert.throws(() => data.read(full, packetOf({ x: 1, y: 2, count: 1, payload: [(1 << 3) | 0] }), world.pool, world.types, world.hooks), /append의 실제 배열 범위/);
    assert.ok(world.log.includes('assert:nVert < MAX_PATH_LENGTH:22'));
    assert.equal(full.count, 5000);
    const fresh = receiveWorld();
    const runtime = new RawPathRuntime();
    assert.throws(() => data.read(runtime, new Uint8Array(23), fresh.pool, fresh.types, fresh.hooks), /24BYTE 머리 부족/);
    assert.throws(() => data.read(runtime, packetOf({ parent: 0xffff }), fresh.pool, fresh.types, fresh.hooks), /부모 물리 범위/);
    assert.throws(() => data.read(runtime, packetOf({ count: 2, payload: [8] }), fresh.pool, fresh.types, fresh.hooks), /payload 부족/);
    assert.throws(() => data.read(new Uint8Array(10), packetOf({}), fresh.pool, fresh.types, fresh.hooks), TypeError);
    assert.throws(() => data.read(runtime, packetOf({}), fresh.pool, fresh.types.slice(1), fresh.hooks), TypeError);
    // 부모 TYPE 이 표 밖이면 거부한다.
    fresh.pool.allocated(5)[10] = 200;
    assert.throws(() => data.read(new RawPathRuntime(), packetOf({}), fresh.pool, fresh.types, fresh.hooks), /TYPE 범위/);
    fresh.pool.allocated(5)[10] = 134;
    // 선택 훅이 없으면 해당 위치에서 거부한다(앞선 쓰기는 남는다).
    for (const name of ['trace', 'metadata', 'now', 'clearAnimation']) {
        const partial = receiveWorld();
        const hooks = { ...partial.hooks };
        delete hooks[name];
        assert.throws(() => data.read(new RawPathRuntime(), packetOf({ count: 1, payload: [8] }), partial.pool, partial.types, hooks), /선택된 장치 미연결/, name);
    }
});

test('RawPathData: 송신한 패킷을 수신하면 같은 경로(모서리 이중 추가를 제외한 꼭짓점·방향)가 복원된다', () => {
    const [points, directions] = [[point(10, 20), point(10, 10), point(20, 10), point(23, 13)], [0, 0, 2, 3]];
    const sender = new RawPathRuntime();
    sender.setPath(points, directions);
    const buffer = new Uint8Array(256);
    const length = new RawPathData().write(sender, 7, 5, 0, createPathDataHeader(), buffer);
    observePathData(buffer, 1, () => assert.fail('진단'));
    assert.equal(get(buffer, 2, 2), get(buffer, 13, 2) + 24);
    const { pool, types, hooks } = receiveWorld();
    const receiver = new RawPathRuntime();
    new RawPathData().read(receiver, buffer.subarray(0, length), pool, types, hooks);
    const received = Array.from({ length: receiver.count }, (_, index) => `${receiver.point(index).x},${receiver.point(index).y},${receiver.direction(index)}`);
    assert.deepEqual(received, ['10,20,0', '10,10,0', '10,10,2', '20,10,2', '20,10,3', '23,13,3']);
});

test('RawPathData.read: 수신 전에 남은 완료 표시·두 목표는 0 으로 지우고, 시간표는 시작 시각·현재 시각·속도/회전 속도로 Prepare 한 값과 같다', () => {
    const { pool, types, hooks } = receiveWorld();
    const runtime = new RawPathRuntime();
    runtime.setTargets(61, 62);
    runtime.setFinished(true);
    const packet = packetOf({ x: 10, y: 20, count: 2, time: 80, payload: [(5 << 3) | 2, (4 << 3) | 4] });
    new RawPathData().read(runtime, packet, pool, types, hooks);
    assert.ok(!runtime.finished && runtime.targetA === 0 && runtime.targetB === 0);
    // 같은 꼭짓점·방향에 startTime 10, 현재 50, 속도 3, 회전 90, 첫 방향 2 로 Prepare 한 시각표와 같아야 한다.
    const expected = new RawPathRuntime();
    expected.setPath(Array.from({ length: runtime.count }, (_, index) => runtime.point(index)), Array.from({ length: runtime.count }, (_, index) => runtime.direction(index)));
    expected.prepare(50, 10, 3, 90, 2);
    assert.deepEqual(Array.from({ length: runtime.count }, (_, index) => runtime.time(index)), Array.from({ length: expected.count }, (_, index) => expected.time(index)));
    assert.ok(runtime.count >= 3 && runtime.time(runtime.count - 1) > runtime.time(0));
});

test('RawPathData.read: 방향 run 이 없으면(nodeCount 0) 시작점 하나만 추가하고 점 로그를 두 번(시작·마지막) 낸다', () => {
    const { pool, types, hooks, log } = receiveWorld();
    const runtime = new RawPathRuntime();
    new RawPathData().read(runtime, packetOf({ x: 10, y: 20, count: 0 }), pool, types, hooks);
    assert.equal(runtime.count, 1);
    assert.deepEqual(runtime.point(0), { x: 10, y: 20 });
    assert.deepEqual(log.filter((entry) => entry.startsWith('[')), ['[0] (10,20) , 0\n', '[0] (10,20) , 0\n']);
});

test('RawPathData.read: 시작점 보정은 거리 1.5 미만에서만 하고(정확히 1.5 는 보정 없음) 거리는 x/y 모두 반영한다', () => {
    // 스테이지 2 의 코너(101,201)와 부모의 거리: 1.5 → 보정 없음, 1.49 → 보정, y 만 1.49 벗어남 → 보정.
    for (const [parentAt, snapped] of [[{ x: 102.5, y: 201 }, false], [{ x: 102.49, y: 201 }, true], [{ x: 101, y: 202.49 }, true], [{ x: 101, y: 202.5 }, false]]) {
        const { pool, types, hooks } = receiveWorld();
        const raw = pool.allocated(5);
        put(raw, 14, floatToBits(parentAt.x));
        put(raw, 18, floatToBits(parentAt.y));
        pool.allocate(2);
        const runtime = new RawPathRuntime();
        new RawPathData().read(runtime, packetOf({ x: 7, y: 8, count: 1, tail: 6, stage: 2, payload: [(4 << 3) | 2] }), pool, types, hooks);
        assert.deepEqual(runtime.point(0), snapped ? { x: 101, y: 201 } : { x: 7, y: 8 }, JSON.stringify(parentAt));
    }
});

test('RawPathData.write: 꼭짓점 5000 개는 허용하고, helper 는 WORD 로 쓰며, 쓴 BYTE 32767 개는 허용하지만 32768 개는 signed 길이 상한으로 거부한다', () => {
    const data = new RawPathData();
    // 꼭짓점 개수 필드가 정확히 용량(5000)이면 허용한다(나머지 꼭짓점은 0 좌표라 쓸 BYTE 가 없다).
    const runtime = new RawPathRuntime();
    runtime.setPath([point(1, 2)], [0]);
    const image = Uint8Array.from(runtime.bytes);
    put(image, 0x10, 5000);
    const buffer = new Uint8Array(64);
    // 둘째 꼭짓점(0,0)만 첫 점(1,2)과 달라 거리 3 의 BYTE 하나가 나가고, 나머지는 같은 좌표라 쓰지 않는다.
    assert.equal(data.write(image, 1, 2, 0, createPathDataHeader(), buffer), 24 + 1);
    // helper 는 WORD 로 쓴다(상위 바이트까지).
    put(image, 0x10, 1);
    put(image, PATH_LAYOUT.helper, 0xabcd);
    data.write(image, 1, 2, 0, createPathDataHeader(), buffer);
    assert.deepEqual(Array.from(buffer.subarray(11, 13)), [0xcd, 0xab]);
    // 한 구간의 거리 31×32767 = 1,015,777 칸이면 BYTE 32767 개이고 허용, 31×32768 이면 32768 개라 거부한다.
    const long = (distance) => {
        const path = new RawPathRuntime();
        path.setPath([point(0, 0), point(distance, 0)], [0, 2]);
        return path;
    };
    const wide = new Uint8Array(24 + 32768);
    assert.equal(data.write(long(31 * 32767), 1, 2, 0, createPathDataHeader(), wide), 24 + 32767);
    assert.equal(get(wide, 13, 2), 32767);
    assert.throws(() => data.write(long(31 * 32768), 1, 2, 0, createPathDataHeader(), new Uint8Array(24 + 40000)), /signed 길이 범위/);
});

test('RawPathData.writeCurrent: 비직접 분기의 첫 점 로그 앞에도 진단 0x29 가 있고, 현재 WORD 가 0x8000 이 되면 반환이 음수 signed 이며, 부모 SID 는 패킷의 WORD 로 줄어든 값으로 묻는다', () => {
    const data = new RawPathData();
    const lines = [];
    data.writeCurrent(new RawPathRuntime(), 1, 2, createPathDataHeader(), new Uint8Array(64), {
        parentGenus: () => 0,
        assertion: (expression, file, line) => { lines.push(line); },
        trace: () => {},
    });
    // 꼭짓점 0: 머리 진단 다섯 개 뒤 첫 점 로그 앞 0x29 가 하나 더 난다.
    assert.deepEqual(lines, [0x1f, 0x1f, 0x24, 0x1f, 0x1f, 0x29]);
    // 쓴 BYTE 가 정확히 32768 개이면 WORD 가 0x8000 이 되어 반환은 signed -32768 + 24 이다(32769 번째 쓰기부터 음수 주소라 거부).
    const path = new RawPathRuntime();
    path.setPath([point(0, 0), point(31 * 32768, 0)], [0, 2]);
    const buffer = new Uint8Array(24 + 40000);
    assert.equal(data.writeCurrent(path, 1, 2, createPathDataHeader(), buffer, { parentGenus: () => 0, trace: () => {} }), -32768 + 24);
    assert.equal(get(buffer, 13, 2), 0x8000);
    const beyond = new RawPathRuntime();
    beyond.setPath([point(0, 0), point(31 * 32769, 0)], [0, 2]);
    assert.throws(() => data.writeCurrent(beyond, 1, 2, createPathDataHeader(), new Uint8Array(24 + 40000), { parentGenus: () => 0, trace: () => {} }), /현재 송신 예약 범위/);
    // 부모 번호가 WORD 를 넘으면 패킷에 쓰인 낮은 WORD 로 묻는다.
    const asked = [];
    const one = new RawPathRuntime();
    one.setPath([point(1, 2)], [0]);
    data.writeCurrent(one, 1, 0x12345, createPathDataHeader(), new Uint8Array(32), { parentGenus: (sid) => { asked.push(sid); return 0; }, trace: () => {} });
    assert.deepEqual(asked, [0x2345]);
});

test('RawPathData.read: 서쪽 이동으로 좌표가 음수가 되면 signed 로 복원하고, 첫 방향은 하위 3비트 전체이며, 시작 시각은 signed DWORD 이다', () => {
    const { pool, types, hooks } = receiveWorld();
    const runtime = new RawPathRuntime();
    // 방향 6(서)으로 20칸: 시작 (10,20) 에서 (-10,20) 까지.
    new RawPathData().read(runtime, packetOf({ x: 10, y: 20, count: 1, time: 0xfffffff0, payload: [(20 << 3) | 6] }), pool, types, hooks);
    assert.equal(runtime.count, 2);
    assert.deepEqual([runtime.point(0), runtime.point(1)], [{ x: 10, y: 20 }, { x: -10, y: 20 }]);
    assert.deepEqual([runtime.direction(0), runtime.direction(1)], [6, 6]);
    // 시작 시각 -16 × 0.125 = -2, 현재 50 과의 간격이 2 초를 넘어 첫 시각이 startTime 이다.
    assert.equal(runtime.time(0), -2);
});

test('RawPathData.read: nodeCount 홀짝 진단은 비트 0 만 본다(5 는 진단, 2 는 진단 없음)', () => {
    for (const [count, diagnosed] of [[5, true], [2, false], [4, false], [3, true]]) {
        const world = receiveWorld(0x120000);
        const payload = Array.from({ length: 6 }, (_, index) => 10 + index);
        new RawPathData().read(new RawPathRuntime(), packetOf({ x: 1, y: 2, count, payload }), world.pool, world.types, world.hooks);
        assert.equal(world.log.some((entry) => entry.startsWith('assert:(zwp->nodeCount & 1) == 0')), diagnosed, `count ${count}`);
    }
});

test('observePathData: 길이의 상위 바이트까지 확정한다(nodeCount 200 → 224 = 0xe0, 0x1f4 → 0x20c)', () => {
    const packet = new Uint8Array(32);
    put(packet, 13, 200, 2);
    observePathData(packet, 1, null);
    assert.deepEqual(Array.from(packet.subarray(2, 4)), [0xe0, 0]);
    put(packet, 13, 0x1f4, 2);
    observePathData(packet, 1, null);
    assert.deepEqual(Array.from(packet.subarray(2, 4)), [0x0c, 0x02]);
});

test('makePathFormTransmission: 부착되지 않은 Path 는 거부하고, 부모 TYPE 의 genus 0x120000 이면 직접 좌표 패킷을 만든다', () => {
    const world = serverWorld();
    const { pool, hash, types, spots, clock, host, kernel, parent } = world;
    types[134].flags2 = 0x120000;
    const header = createPathDataHeader();
    const buffer = new Uint8Array(64).fill(0xee);
    const sizes = [];
    const transmission = makePathFormTransmission(pool, types, header, buffer, (size) => { sizes.push(size); });
    const hooks = { unpop: () => {}, repop: () => {}, pop: () => {}, halt: () => 10, arrival: () => 1000, soundId: () => 0, stepSound: () => {}, clearBlocked: () => {}, cleanup: () => {} };
    const make = () => new RawPathProcess(pool, hash, types, types.map(() => new FrameCodeTable([])), types.map(() => ({ frames: [] })), spots, new RawPathProcessState(clock), hooks, {});
    // 부착되기 전(Form/부모 0)에는 직렬화하지 않는다.
    const loose = make();
    loose.runtime.setPath([point(10, 20), point(30, 40)], [0, 0]);
    assert.throws(() => transmission.writeData(loose, false), /풀\/부착 불일치/);
    host.setFormTransmit(transmission);
    const next = make();
    next.runtime.setPath([point(10, 20), point(30, 40)], [0, 0]);
    try {
        const process = RawPathProcess.attach(host, next, parent, 0);
        assert.ok(process);
        assert.deepEqual(sizes, [26]);
        assert.deepEqual(Array.from(buffer.subarray(24, 26)), [30, 40]);
        assert.equal(get(buffer, 13, 2), 2);
    } finally {
        kernel.clear();
    }
});

test('RawPathData.writeCurrent: helper·명령 꼬리·꼬리 단계를 typed 와 같은 폭(WORD·WORD·BYTE)으로 쓴다', () => {
    const runtime = new RawPathRuntime();
    runtime.setPath([point(1, 2)], [0]);
    put(runtime.bytes, PATH_LAYOUT.helper, 0xabcd);
    put(runtime.bytes, 0x186e4, 0x1234);
    put(runtime.bytes, 0x186e8, 0x7f);
    const buffer = new Uint8Array(64);
    new RawPathData().writeCurrent(runtime, 1, 2, createPathDataHeader(), buffer, { parentGenus: () => 0, trace: () => {} });
    assert.deepEqual(Array.from(buffer.subarray(11, 13)), [0xcd, 0xab]);
    assert.deepEqual(Array.from(buffer.subarray(21, 24)), [0x34, 0x12, 0x7f]);
});
