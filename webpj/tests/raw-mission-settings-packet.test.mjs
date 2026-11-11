// 로그인 마지막의 7DWORD 현재 설정 패킷(0x32, cpppj `o/RawMissionSettingsPacket`)의 웹 대응 `core/raw-mission-settings-packet.js` 를 검사한다(4-C, WB04-E316).
//  - 원본 독립 관측 `mission-settings-packet-x86.tsv` 164행(송신 S 117·수신 R 45·observer O 2 × 현재 설정 프로필 × seed × 대상 × 호출 도중 변경)의 packet 32바이트 전체·현재 설정 7DWORD·읽기/쓰기/송신 효과 순서를 재생한다.
//  - 손 시험(cpppj `MissionSettingsPacketTests.cpp` 이식 + 기준선 뒤 경로): 실제 분배기 등록(길이 32·버전 1·observer), 비로그인 7DWORD 직접 등록, 선택한 getter/setter/send 누락과 물리 부족 전까지의 부분 쓰기 보존.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E316)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { PacketConnection, RawPacketDispatch } from '../www/core/raw-packet-dispatch.js';
import { MISSION_SETTINGS_FIELD, RawMissionSettingsPacket, receiveCurrentMissionSettingsPacket, registerMissionSettingsPacket } from '../www/core/raw-mission-settings-packet.js';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';

// 원본 관측 파일이다. 파일이 없으면 재생 검사를 건너뛴다. (2026-11-11 WB04-E316)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'mission-settings-packet-x86.tsv'));

// 바이트를 소문자 16진 문자열로 바꾼다. (2026-11-11 WB04-E316)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// little endian DWORD 를 읽고 쓴다. (2026-11-11 WB04-E316)
const get = (bytes, offset) => (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
function put(bytes, offset, value) {
    for (let index = 0; index < 4; index += 1) bytes[offset + index] = (value >>> (8 * index)) & 0xff;
}

// 원본에 공급한 명시 현재값만 만들며 packet 기대 출력을 생성하지 않는다: 프로필 0 영, 1 전부 0xffffffff, 2 경계값, 3 필드별 구분값, 4 이상 한 필드만 1/0x80000000/0xffffffff. (2026-11-11 WB04-E316)
function settingsValues(profile, seed) {
    const values = new Uint32Array(7);
    if (profile === 1) values.fill(0xffffffff);
    else if (profile === 2) values.set([0, 1, 0x80000000, 0x7fffffff, 0xffffffff, 3, 16]);
    else if (profile === 3) {
        // 각 현재 필드를 다른 DWORD 로 구분한다.
        for (let index = 0; index < values.length; index += 1) values[index] = (0x12345678 + index * 0x1010101 + seed) >>> 0;
    } else if (profile >= 4) {
        const bits = [1, 0x80000000, 0xffffffff];
        values[Math.floor((profile - 4) / 3)] = bits[(profile - 4) % 3];
    }
    return values;
}

// 명시 효과가 먼저 읽은 값 이외의 현재 원문을 변경한다. (2026-11-11 WB04-E316)
function settingsChange(words, field, step, seed) {
    // 다음 getter/read 의 현재값을 바꾸되 이미 반환한 값은 보존한다.
    for (let index = 0; index < words.length; index += 1) {
        if (index !== field) words[index] = (words[index] ^ ((0x9e3779b9 + seed + step * 17 + index) >>> 0)) >>> 0;
    }
}

// 현재 7DWORD 를 payload 번호 순서의 little endian 바이트로 표시한다. (2026-11-11 WB04-E316)
function settingsHex(words) {
    const bytes = new Uint8Array(28);
    for (let index = 0; index < words.length; index += 1) put(bytes, index * 4, words[index]);
    return hex(bytes);
}

// 실제 분배기 경계를 관찰하는 효과 장치이다(선택되지 않은 효과는 trace 가 비어야 한다). (2026-11-11 WB04-E316)
function dispatchScene() {
    const scene = { trace: '' };
    scene.dispatch = new RawPacketDispatch({
        now: () => { scene.trace += 'N'; return 1; },
        record: () => { scene.trace += 'D'; },
        terminate: () => { scene.trace += 'T'; },
        close: () => { scene.trace += 'C'; },
        release: () => { scene.trace += 'F'; },
    });
    return scene;
}

// 164행 재생: 열은 input(kind,profile,seed,target,mode)/packet/settings/trace 이다. (2026-11-11 WB04-E316)
test('미션 설정 패킷: 원본 관측 164행의 packet·설정·효과 순서', { skip: ROWS === null }, () => {
    let count = 0;
    // 각 행에 새로운 현재 설정과 수신 원문을 설치한다.
    for (const row of ROWS) {
        if (row[0] === 'input') continue;
        assert.equal(row.length, 4);
        const fields = row[0].split(',');
        assert.equal(fields.length, 5);
        const profile = Number(fields[1]);
        const seed = Number(fields[2]);
        const target = Number(fields[3]);
        const mode = Number(fields[4]);
        const current = settingsValues(profile, seed);
        const packet = new Uint8Array(32);
        // 수신/observer 의 원문 헤더도 native 의 무해석 입력을 그대로 보존한다.
        for (let index = 0; index < packet.length; index += 1) packet[index] = (index * 37 + 11) & 0xff;
        // 수신의 기대결과가 아니라 원본에 전달한 현재 packet 입력이다(송신 행은 packet 을 0 으로 시작).
        if (fields[0] === 'S') packet.fill(0);
        else for (let index = 0; index < current.length; index += 1) put(packet, 4 + index * 4, current[index]);
        if (fields[0] === 'R') {
            // 수신 전 현재 전역은 packet 과 독립된 stale DWORD 이다.
            for (let index = 0; index < current.length; index += 1) current[index] = (0xcccc0000 + index) >>> 0;
        }
        const trace = [];
        let step = 0;
        const event = (text) => { trace.push(text); };
        // 현재 효과 입력 변경(모드 1 은 첫 효과에서만, 모드 2 는 매번)은 packet 기대 출력 작성과 구분한다.
        const change = (field, receive) => {
            step += 1;
            if (mode !== 2 && !(mode === 1 && step === 1)) return;
            if (!receive) settingsChange(current, field, step, seed);
            else {
                // callback 이 바꾸는 나머지 현재 payload 를 원문 span 에서 읽어 바꾼 뒤 같은 원문에 되쓴다.
                const words = new Uint32Array(7);
                for (let index = 0; index < words.length; index += 1) words[index] = get(packet, 4 + index * 4);
                settingsChange(words, field, step, seed);
                for (let index = 0; index < words.length; index += 1) put(packet, 4 + index * 4, words[index]);
            }
        };
        const settings = new RawMissionSettingsPacket({
            current: (field) => { const value = current[field]; event(`R,${field},${value}`); change(field, false); return value; },
            setCurrent: (field, value) => { event(`W,${field},${value}`); current[field] = value; change(field, true); },
            send: (recipient, bytes) => { event(`S,${recipient >>> 0}`); assert.equal(bytes.length, packet.length); packet.set(bytes); },
        });
        if (fields[0] === 'S') settings.send(target | 0);
        else if (fields[0] === 'R') settings.receive(packet);
        else {
            const scene = dispatchScene();
            registerMissionSettingsPacket(scene.dispatch, settings);
            scene.dispatch.registration(0x32).observer(mode);
            assert.equal(scene.trace, '');
        }
        assert.equal(hex(packet), row[1], `행 ${count} packet`);
        assert.equal(settingsHex(current), row[2], `행 ${count} 설정`);
        assert.equal(trace.length ? trace.join(';') : '-', row[3], `행 ${count} 효과`);
        count += 1;
    }
    assert.equal(count, 164);
});

// 실제 고정 길이(32)/버전 1/observer 와 같은 현재 설정에 쓰는 0x32 수신을 분배기에 등록한다. (2026-11-11 WB04-E316)
test('미션 설정 패킷: 분배기 등록과 현재 설정 왕복', () => {
    const values = settingsValues(2, 0);
    let packet = new Uint8Array(32);
    const settings = new RawMissionSettingsPacket({
        current: (field) => values[field],
        setCurrent: (field, value) => { values[field] = value; },
        send: (target, bytes) => { assert.equal(target, 9); packet = Uint8Array.from(bytes); },
    });
    settings.send(9);
    const first = Uint8Array.from(packet);
    values.set(settingsValues(3, 7));
    settings.send(9);
    assert.notDeepEqual(Array.from(first), Array.from(packet));
    const expected = Uint32Array.from(values);
    values.fill(0);
    const scene = dispatchScene();
    registerMissionSettingsPacket(scene.dispatch, settings);
    const registration = scene.dispatch.registration(0x32);
    assert.deepEqual([registration.length, registration.version, registration.mismatch, typeof registration.receive, typeof registration.observer], [32, 1, 0, 'function', 'function']);
    registration.observer(0);
    registration.observer(1);
    assert.equal(scene.trace, '');
    // 실제 분배기로 수신하면 같은 현재 설정에 payload 순서로 저장되고 시각과 기록이 한 번씩 불린다.
    const connection = new PacketConnection({ sender: 9 });
    assert.equal(scene.dispatch.dispatch(connection, packet), true);
    assert.deepEqual(Array.from(values), Array.from(expected));
    assert.equal(scene.trace, 'ND');
    assert.deepEqual([connection.received, connection.depth], [1, 0]);
    // 선언 길이가 32 가 아니면 검증이 거부한다.
    packet[2] = 31;
    assert.equal(scene.dispatch.validate(packet), false);
});

// 비로그인(Editor/현재 Session)은 송신 객체 없이 같은 원문 7DWORD 수신을 등록하고 UI/정규화/새 설정 초기화를 추가하지 않는다. (2026-11-11 WB04-E316: 기준선 뒤 52eae6bc0)
test('미션 설정 패킷: 비로그인 7DWORD 직접 등록과 수신', () => {
    const current = new Uint32Array(7);
    const scene = dispatchScene();
    registerMissionSettingsPacket(scene.dispatch, current);
    const registration = scene.dispatch.registration(0x32);
    assert.deepEqual([registration.length, registration.version, registration.mismatch, registration.observer, typeof registration.receive], [32, 1, 0, null, 'function']);
    const packet = new Uint8Array(32);
    packet[0] = 1;
    packet[1] = 0x32;
    packet[2] = 32;
    for (let index = 0; index < 7; index += 1) put(packet, 4 + index * 4, (0x80000000 + index) >>> 0);
    assert.equal(scene.dispatch.dispatch(new PacketConnection({ sender: 3 }), packet), true);
    assert.deepEqual(Array.from(current), [0x80000000, 0x80000001, 0x80000002, 0x80000003, 0x80000004, 0x80000005, 0x80000006]);
    // 물리 부족은 앞선 store 를 보존한 채 거부한다: 12바이트면 DWORD 두 개만 쓴다.
    const partial = new Uint32Array(7).fill(0xdeadbeef);
    assert.throws(() => receiveCurrentMissionSettingsPacket(partial, packet.subarray(0, 12)), RangeError);
    assert.deepEqual(Array.from(partial), [0x80000000, 0x80000001, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef, 0xdeadbeef]);
    // 7칸이 아닌 배열/등록 대상은 거부한다.
    assert.throws(() => receiveCurrentMissionSettingsPacket(new Uint32Array(6), packet), TypeError);
    assert.throws(() => registerMissionSettingsPacket(scene.dispatch, new Uint32Array(6)), TypeError);
    assert.throws(() => registerMissionSettingsPacket(scene.dispatch, null), TypeError);
});

// 선택된 getter/store/send 누락을 가짜 성공으로 채우지 않고 물리 부족 전까지 원본 부분 쓰기를 보존한다. (2026-11-11 WB04-E316)
test('미션 설정 패킷: 선택 장치 누락과 부분 쓰기', () => {
    assert.throws(() => new RawMissionSettingsPacket({}), TypeError);
    assert.throws(() => new RawMissionSettingsPacket(null), TypeError);
    const hooks = { current: () => 0 };
    const missing = new RawMissionSettingsPacket(hooks);
    // 송신 장치가 없으면 읽은 뒤 거부하고, 쓰기 장치가 없으면 첫 store 전에 거부한다.
    assert.throws(() => missing.send(9), /실제 현재 연결 송신 미연결/);
    const packet = new Uint8Array(32);
    assert.throws(() => missing.receive(packet), /실제 현재 설정 쓰기 미연결/);
    const writes = [];
    const partial = new RawMissionSettingsPacket({ ...hooks, setCurrent: (field) => { writes.push(field); } });
    assert.throws(() => partial.receive(packet.subarray(0, 12)), RangeError);
    assert.deepEqual(writes, [0, 1]);
    // DWORD 한 개에 한 바이트가 모자란 15바이트면 셋째 필드를 읽는 위치에서 거부한다(3바이트만 남음).
    writes.length = 0;
    assert.throws(() => partial.receive(packet.subarray(0, 15)), RangeError);
    assert.deepEqual(writes, [0, 1]);
    writes.length = 0;
    assert.throws(() => partial.receive(packet.subarray(0, 16)), RangeError);
    assert.deepEqual(writes, [0, 1, 2]);
    // 실제 연결 gate 가 거부하면 그대로 전파된다.
    const rejected = new RawMissionSettingsPacket({ ...hooks, send: () => { throw new Error('검사용 실제 연결 gate 거부'); } });
    assert.throws(() => rejected.send(9), /연결 gate 거부/);
    // 필드 번호는 payload 순서이다.
    assert.deepEqual(Object.values(MISSION_SETTINGS_FIELD), [0, 1, 2, 3, 4, 5, 6]);
});
