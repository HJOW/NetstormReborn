// 수신 활동 기록(cpppj `o/RawConnectionActivity`, 10.78 004b6180)의 웹 대응 `core/raw-connection-activity.js` 를 검사한다(4-C, WB04-E309).
// 원본 관측 파일이 없는 단위 계약 검사이다(cpppj `ConnectionActivityTests.cpp` 여섯 건 이식): 벽시각→프레임→벽시각 store→server→프레임 store→상태 삭제 순서와 마지막 효과가 바꾼 표를 복구하지 않음,
// 비서버 분기·프레임 0·음수 벽시각의 비정규화, signed64 절삭 하위 DWORD 변환, 두 표 사이 Zone DWORD 보존, 잘못된 인덱스의 첫 효과 전 거부, 기존 분배기를 통한 0x64 등록(길이 보존).
// 표 수명을 소유하는 `ClientConnectionActivityGlobals` 시험(같은 파일의 표 경계 시험 일부)은 해당 단위에서 옮긴다. 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E309)
import assert from 'node:assert/strict';
import test from 'node:test';
import { PacketConnection, RawPacketDispatch } from '../www/core/raw-packet-dispatch.js';
import { CONNECTION_ACTIVITY_ENTRIES, RawConnectionActivity, registerConnectionActivity } from '../www/core/raw-connection-activity.js';

// 실제 시계/현재 전역/상태 삭제의 호출만 관측하는 독립 장치와 두 표이다. 값은 호출 순서를 바꾸지 않는다. (2026-11-11 WB04-E309)
function makeDevice() {
    const device = {
        frames: new Uint32Array(CONNECTION_ACTIVITY_ENTRIES),
        wallSeconds: new Uint32Array(CONNECTION_ACTIVITY_ENTRIES),
        wall: 100.75,
        frame: 7,
        server: false,
        trace: [],
        erased: [],
    };
    // 네 실제 장치의 기본값이며 각 검사는 필요한 효과만 바꾼다.
    device.hooks = () => ({
        wallSeconds: () => { device.trace.push('wall'); return device.wall; },
        frameNumber: () => { device.trace.push('frame'); return device.frame; },
        server: () => { device.trace.push('server'); return device.server; },
        eraseStatus: (kind, player) => { device.trace.push('erase'); device.erased.push([kind, player]); },
    });
    device.tables = () => ({ frames: device.frames, wallSeconds: device.wallSeconds });
    return device;
}

// 원문 벽시각의 부수 효과 뒤 현재 프레임/server 를 읽고 두 store 사이에서 server 를 조회하며, 마지막 상태 삭제 효과가 바꾼 표를 되돌리지 않는다. (2026-11-11 WB04-E309)
test('연결 활동: 벽시각·프레임 store 와 현재 server 는 원문 순서', () => {
    const device = makeDevice();
    device.frames.fill(0xaaaaaaaa);
    device.wallSeconds.fill(0xbbbbbbbb);
    const hooks = device.hooks();
    hooks.wallSeconds = () => {
        device.trace.push('wall');
        // 벽시각 조회 시점에는 두 표 모두 아직 쓰이지 않았다.
        assert.deepEqual([device.frames[9], device.wallSeconds[9]], [0xaaaaaaaa, 0xbbbbbbbb]);
        device.frame = 0x12345678;
        device.server = true;
        return device.wall;
    };
    hooks.server = () => {
        device.trace.push('server');
        // server 조회는 벽시각 store 뒤, 프레임 store 앞이다.
        assert.deepEqual([device.wallSeconds[9], device.frames[9]], [100, 0xaaaaaaaa]);
        device.frame = 999;
        return device.server;
    };
    const erase = hooks.eraseStatus;
    hooks.eraseStatus = (kind, player) => {
        // 상태 삭제 시점에는 프레임이 벽시각 store 전에 캡처한 값이다.
        assert.deepEqual([device.frames[9], device.wallSeconds[9]], [0x12345678, 100]);
        erase(kind, player);
        device.frames[9] = 0x55555555;
        device.wallSeconds[9] = 0x66666666;
    };
    new RawConnectionActivity(device.tables(), hooks).receive(9);
    assert.deepEqual(device.trace, ['wall', 'frame', 'server', 'erase']);
    assert.deepEqual(device.erased, [[2, 9]]);
    // 마지막 효과가 바꾼 같은 표를 반환 후 사본으로 복구하지 않는다.
    assert.deepEqual([device.frames[9], device.wallSeconds[9]], [0x55555555, 0x66666666]);
});

// 비서버 분기의 player 는 sender 와 무관하게 0 이며 프레임 0 과 음수 벽시각의 하위 DWORD 를 보존한다. (2026-11-11 WB04-E309)
test('연결 활동: 비서버 삭제 인자와 프레임 0 은 정규화하지 않음', () => {
    const device = makeDevice();
    device.wall = -1.5;
    device.frame = 0;
    new RawConnectionActivity(device.tables(), device.hooks()).receive(21);
    assert.deepEqual([device.frames[21], device.wallSeconds[21]], [0, 0xffffffff]);
    assert.deepEqual(device.erased, [[1, 0]]);
});

// 기존 _ftol 관측과 같은 signed64 절삭/하위 DWORD 계약을 double 입력에서 검증한다(float 으로 줄이지 않음). (2026-11-11 WB04-E309)
test('연결 활동: signed64 벽시각 변환은 하위 DWORD 를 보존', () => {
    // 2^63 바로 앞/뒤 double 은 간격 1024/2048 이다.
    const justBelow = 2 ** 63 - 1024;
    const cases = [
        [0, 0], [-0, 0], [0.75, 0], [-0.75, 0], [1.5, 1], [-1.5, 0xffffffff],
        [2147483648.75, 0x80000000], [-2147483649.75, 0x7fffffff], [4294967303.75, 7], [-4294967303.75, 0xfffffff9],
        [justBelow, 0xfffffc00], [-justBelow, 0x400], [2 ** 63, 0], [-(2 ** 63), 0],
        [-(2 ** 63 + 2048), 0], [1e100, 0], [-1e100, 0],
        [Infinity, 0], [-Infinity, 0], [NaN, 0],
    ];
    const device = makeDevice();
    device.frame = 17;
    const activity = new RawConnectionActivity(device.tables(), device.hooks());
    // 변환마다 같은 원문 몸체와 현재 표를 사용하며 예전 반환값을 재활용하지 않는다.
    for (const [value, expected] of cases) {
        device.wall = value;
        activity.receive(9);
        assert.deepEqual([device.wallSeconds[9], device.frames[9]], [expected, 17], `벽시각 ${value}`);
    }
    assert.equal(device.erased.length, cases.length);
});

// 두 표의 경계와 사이의 별개 Zone DWORD 는 건드리지 않고, 다음 몸체에 빌려 준 같은 표를 계속 쓴다. (2026-11-11 WB04-E309)
test('연결 활동: 두 표 사이의 Zone DWORD 와 이웃 칸은 보존', () => {
    // 22 프레임 + Zone 두 DWORD + 22 벽시각 + 이웃 한 DWORD 를 한 메모리에 놓는다(005c84e8..005c85a0 배치).
    const memory = new Uint32Array(22 + 2 + 22 + 1);
    const frames = memory.subarray(0, 22);
    const wallSeconds = memory.subarray(24, 46);
    for (let index = 0; index < 22; index += 1) {
        frames[index] = 0x1000 + index;
        wallSeconds[index] = 0x2000 + index;
    }
    memory[22] = 0x12345678;
    memory[23] = 0x87654321;
    memory[46] = 0xdeadbeef;
    const before = Array.from(memory);
    const device = makeDevice();
    device.frames = frames;
    device.wallSeconds = wallSeconds;
    new RawConnectionActivity(device.tables(), device.hooks()).receive(21);
    // 마지막 정상 인덱스 한 칸씩만 바뀌고 Zone 두 DWORD 와 뒤 이웃은 그대로이다.
    const after = Array.from(memory);
    const changed = after.map((value, index) => (value === before[index] ? -1 : index)).filter((index) => index >= 0);
    assert.deepEqual(changed, [21, 45]);
    assert.deepEqual([memory[22], memory[23], memory[46], frames[21], wallSeconds[21]], [0x12345678, 0x87654321, 0xdeadbeef, 7, 100]);
    // 새 몸체를 만들어도 같은 표(같은 메모리)를 쓴다.
    device.wall = 111.75;
    device.frame = 88;
    new RawConnectionActivity(device.tables(), device.hooks()).receive(0);
    assert.deepEqual([frames[0], wallSeconds[0], frames[21], wallSeconds[21], memory[22], memory[23]], [88, 111, 7, 100, 0x12345678, 0x87654321]);
});

// 원문이 잘못된 주소를 쓰는 범위는 정상 실행으로 가장하지 않고 첫 벽시각 효과 전에 계약 오류로 거부한다. (2026-11-11 WB04-E309)
test('연결 활동: 잘못된 sender 는 효과와 store 전에 거부', () => {
    const device = makeDevice();
    device.frames.fill(0x11111111);
    device.wallSeconds.fill(0x22222222);
    const frames = Array.from(device.frames);
    const wallSeconds = Array.from(device.wallSeconds);
    const activity = new RawConnectionActivity(device.tables(), device.hooks());
    // 정상 22칸의 앞/뒤와 signed 양끝, 비정수/비수치를 따로 확인한다.
    for (const sender of [-1, -22, 22, -2147483648, 2147483647, 1.5, NaN, Infinity, '3', null, undefined]) {
        assert.throws(() => activity.receive(sender), RangeError, `sender ${String(sender)}`);
        assert.deepEqual([device.trace, device.erased], [[], []]);
        assert.deepEqual([Array.from(device.frames), Array.from(device.wallSeconds)], [frames, wallSeconds]);
    }
});

// 구성: 서로 다른 22칸 두 표와 네 필수 장치가 필요하다(같은 주소·부족한 칸·장치 누락은 거부). (2026-11-11 WB04-E309)
test('연결 활동: 구성의 별개 두 표와 필수 장치', () => {
    const device = makeDevice();
    const hooks = device.hooks();
    assert.doesNotThrow(() => new RawConnectionActivity(device.tables(), hooks));
    // 같은 표이거나 같은 메모리 시작의 다른 뷰는 같은 Native 주소로 보고 거부한다.
    assert.throws(() => new RawConnectionActivity({ frames: device.frames, wallSeconds: device.frames }, hooks), TypeError);
    const alias = new Uint32Array(device.frames.buffer, device.frames.byteOffset, 22);
    assert.throws(() => new RawConnectionActivity({ frames: device.frames, wallSeconds: alias }, hooks), TypeError);
    // 22칸이 아닌 표/Uint32Array 가 아닌 표/표 없음은 거부한다.
    assert.throws(() => new RawConnectionActivity({ frames: new Uint32Array(21), wallSeconds: device.wallSeconds }, hooks), TypeError);
    assert.throws(() => new RawConnectionActivity({ frames: device.frames, wallSeconds: new Uint32Array(23) }, hooks), TypeError);
    assert.throws(() => new RawConnectionActivity({ frames: Array(22).fill(0), wallSeconds: device.wallSeconds }, hooks), TypeError);
    assert.throws(() => new RawConnectionActivity(null, hooks), TypeError);
    // 필수 장치를 하나씩 빼면 거부한다.
    for (const name of ['wallSeconds', 'frameNumber', 'server', 'eraseStatus']) {
        assert.throws(() => new RawConnectionActivity(device.tables(), { ...hooks, [name]: undefined }), TypeError, name);
    }
    assert.throws(() => new RawConnectionActivity(device.tables(), null), TypeError);
});

// 기존 분배기의 길이/버전/기록/본문 순서를 사용하며 별도 동기 수신 우회나 중복 기록을 만들지 않는다. 등록은 길이 표를 덮지 않는다. (2026-11-11 WB04-E309)
test('연결 활동: 등록은 기존 분배기를 쓰고 길이 5 를 보존', () => {
    const device = makeDevice();
    device.server = true;
    const activity = new RawConnectionActivity(device.tables(), device.hooks());
    const dispatch = new RawPacketDispatch({
        now: () => { device.trace.push('now'); return 77; },
        record: (connection, bytes, length) => {
            // 기록 장치는 연결의 sender 와 패킷 타입·선언 길이를 그대로 받는다(길이 5 는 기본 길이 표).
            assert.deepEqual([connection.sender, bytes[1], length], [device.expectSender, 0x64, bytes[2]]);
            device.trace.push('record');
        },
        terminate: () => { throw new Error('활동 등록 검사의 미선택 연결 종료'); },
        close: () => { throw new Error('활동 등록 검사의 미선택 연결 소멸'); },
        release: () => { throw new Error('활동 등록 검사의 미선택 연결 해제'); },
    });
    registerConnectionActivity(dispatch, activity);
    const registration = dispatch.registration(0x64);
    assert.deepEqual([registration.version, registration.mismatch, registration.length, typeof registration.receive, typeof registration.observer], [1, 0, 5, 'function', 'function']);
    // 관찰기는 송신/수신 선택과 관계없이 아무 효과도 없다.
    registration.observer(1);
    assert.deepEqual(device.trace, []);
    const connection = new PacketConnection({ sender: 9 });
    device.expectSender = 9;
    assert.equal(dispatch.dispatch(connection, Uint8Array.of(1, 0x64, 5, 0, 0xa5)), true);
    assert.deepEqual(device.trace, ['now', 'record', 'wall', 'frame', 'server', 'erase']);
    assert.deepEqual([connection.received, connection.depth, device.frames[9], device.wallSeconds[9]], [77, 0, 7, 100]);
    // 선언 길이가 다르면 기존 분배기가 본문 앞에서 자연 생략한다.
    device.trace.length = 0;
    assert.equal(dispatch.dispatch(connection, Uint8Array.of(1, 0x64, 4, 0, 0)), true);
    assert.deepEqual([device.trace, device.erased.length], [[], 1]);
    // 원문 정책 0 은 다른 버전도 분배한다.
    assert.equal(dispatch.dispatch(connection, Uint8Array.of(0x7f, 0x64, 5, 0, 0)), true);
    assert.equal(device.erased.length, 2);
    // 다시 등록해도 현재 길이 표를 암묵 초기화하지 않는다.
    dispatch.requireLength(0x64, 7);
    registerConnectionActivity(dispatch, activity);
    assert.equal(dispatch.registration(0x64).length, 7);
    // 서로 다른 sender(음수 비트열)는 signed 로 해석해 범위 밖이면 거부한다.
    const stray = new PacketConnection({ sender: 0xfffffffe });
    device.expectSender = 0xfffffffe;
    assert.throws(() => dispatch.dispatch(stray, Uint8Array.of(1, 0x64, 7, 0, 0, 0, 0)), RangeError);
});
