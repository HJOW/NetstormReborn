// 의식 보상 선택 6b 수신 raw-ritual-choice.js(00448e90)의 계약 검사이다.
// cpppj 에도 이 함수 전용 관측 fixture 가 없어(CMake 주석상 MP 6b 사전 구현) 소스 계약을 그대로 옮긴 검사이다:
//  - 입력 거부(판본·동일 풀·10바이트), 부모 free 관문(finder·삭제·생성 모두 건너뜀), mask 4 후보만 +22=1 표식 후 destroy(sid,0),
//  - 삭제 효과 뒤에 종류 DWORD 를 읽어 create(parent, kind, 0, 4, 0, 1), 삭제 경계 누락, 순회 중 삭제 효과(다음 링크 보존), 교체 흐름(두 번 수신).
// 실제 풀·실제 finder·실제 RawContainedCreate 를 쓰며 삭제/부착/생성만 기록 훅이다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E366)
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { RawContainedCreate, INDEPENDENT_CONTAINED_STATE } from '../www/core/contained-create.js';
import { receiveRitualChoice, RITUAL_CHOICE_MIN_PACKET } from '../www/core/raw-ritual-choice.js';

// 슬롯 WORD/DWORD 를 읽고 쓰는 보조이다. (2026-11-13 WB04-E366)
const word = (raw, at) => raw[at] | (raw[at + 1] << 8);
const putWord = (raw, at, value) => { raw[at] = value & 0xff; raw[at + 1] = (value >>> 8) & 0xff; };

// 6b 패킷을 만든다: 앞 4바이트는 무관한 쓰레기 값, [4..5] 부모, [6..9] 종류이다. (2026-11-13 WB04-E366)
function packetOf(parent, kind, size = 10) {
    const packet = new Uint8Array(size).fill(0xa5);
    putWord(packet, 4, parent);
    packet[6] = kind & 0xff; packet[7] = (kind >>> 8) & 0xff; packet[8] = (kind >>> 16) & 0xff; packet[9] = (kind >>> 24) & 0xff;
    return packet;
}

// 실제 풀·finder 상태·contained 생성기와 호출 기록을 묶은 장면이다. 부모 하나와 종속 목록(types/flags 배열 순서)을 만든다.
// children 항목은 { type, flags, state } 이고 기본은 contained TYPE 6·flags 4 이다. 생성기는 실제 풀 할당 + 머리 삽입 부착을 흉내 낸다. (2026-11-13 WB04-E366)
function scene(children = []) {
    const pool = new SidPool(32768, { server: true });
    const events = [];
    const state = new ContainedFinderState();
    const parent = pool.allocate(0);
    putWord(pool.physical(parent), 0, 0x1111);
    const sids = children.map(() => pool.allocate(0));
    // 종속 체인을 raw 로 이어 붙인다: 부모 head(+6) → 첫 자식 → … → 0.
    children.forEach((child, index) => {
        const raw = pool.physical(sids[index]);
        raw[10] = child.type ?? 6;
        if (child.state !== undefined) raw[11] = child.state;
        raw[20] = child.flags ?? 4;
        raw[21] = child.high ?? 0;
        raw[22] = 0xaa;
        putWord(raw, 4, index + 1 < sids.length ? sids[index + 1] : 0);
    });
    putWord(pool.physical(parent), 6, sids[0] ?? 0);
    const create = new RawContainedCreate(pool, INDEPENDENT_CONTAINED_STATE, {
        create: (type, flags) => { events.push(`C:${type}:${flags}`); const sid = pool.allocate(0); pool.physical(sid)[10] = type; return sid; },
        notifySurface: () => events.push('S'),
        attach: (sid, owner, flags) => {
            events.push(`B:${sid}:${owner}:${flags}`);
            putWord(pool.allocated(sid), 4, word(pool.slot(owner), 6));
            putWord(pool.physical(owner), 6, sid);
        },
    });
    const destroy = (sid, flags) => {
        events.push(`D:${sid}:${flags}`);
        // 가상 삭제는 부모 체인에서 해당 SID 를 풀고 슬롯을 비운다(다음 링크도 지운다).
        let previous = parent;
        let at = 6;
        // 체인에 없는 SID 면 무한 반복 대신 거부한다(최대 64단계).
        for (let step = 0; word(pool.slot(previous), at) !== sid; step += 1) {
            if (step >= 64 || !word(pool.slot(previous), at)) throw new Error(`삭제 대상이 체인에 없음: ${sid}`);
            previous = word(pool.slot(previous), at);
            at = 4;
        }
        putWord(pool.physical(previous), at, word(pool.slot(sid), 4));
        pool.physical(sid).fill(0);
    };
    return { pool, state, create, destroy, events, parent, sids };
}

// 예외 종류와 문구를 함께 본다. (2026-11-13 WB04-E366)
const rejects = (action, type, pattern) => assert.throws(action, (error) => error instanceof type && pattern.test(error.message));

// 이 부모의 현재 종속 목록 SID 배열이다. (2026-11-13 WB04-E366)
function chain(pool, parent) {
    const result = [];
    for (let sid = word(pool.slot(parent), 6); sid; sid = word(pool.slot(sid), 4)) result.push(sid);
    return result;
}

test('최소 길이 상수는 10바이트이다', () => {
    assert.equal(RITUAL_CHOICE_MIN_PACKET, 10);
});

test('판본·동일 풀·길이 계약을 어기면 아무 것도 건드리기 전에 거부한다', () => {
    const world = scene([{}]);
    const before = world.pool.bytes();
    const other = scene();
    // 다른 풀에서 만든 생성기, 생성기 없음, 9바이트, 빈 패킷, 패킷 없음은 모두 거부한다.
    rejects(() => receiveRitualChoice(world.pool, world.state, other.create, packetOf(world.parent, 7), world.destroy), TypeError, /의식 선택 수신의 판본\/동일Pool\/10BYTE 계약/);
    rejects(() => receiveRitualChoice(world.pool, world.state, undefined, packetOf(world.parent, 7), world.destroy), TypeError, /계약/);
    rejects(() => receiveRitualChoice(world.pool, world.state, world.create, packetOf(world.parent, 7, 9), world.destroy), TypeError, /계약/);
    rejects(() => receiveRitualChoice(world.pool, world.state, world.create, new Uint8Array(0), world.destroy), TypeError, /계약/);
    rejects(() => receiveRitualChoice(world.pool, world.state, world.create, null, world.destroy), TypeError, /계약/);
    // 10.78 풀이 아닌 입력은 거부한다.
    const foreign = { edition: 'cd1072', slot: () => new Uint8Array(32), capacity: 32768 };
    rejects(() => receiveRitualChoice(foreign, world.state, { pool: foreign }, packetOf(world.parent, 7), world.destroy), TypeError, /계약/);
    rejects(() => receiveRitualChoice(null, world.state, world.create, packetOf(world.parent, 7), world.destroy), TypeError, /계약/);
    assert.deepEqual(world.events, []);
    assert.deepEqual(world.pool.bytes(), before);
});

test('정확히 10바이트와 더 긴 패킷은 받는다', () => {
    for (const size of [10, 11, 64]) {
        const world = scene();
        receiveRitualChoice(world.pool, world.state, world.create, packetOf(world.parent, 0x1234, size), world.destroy);
        assert.equal(world.events[0], 'C:6:0');
    }
});

test('부모가 free 이면 finder·삭제·생성을 모두 건너뛴다', () => {
    const world = scene([{}, {}]);
    // free 비트(상태 +11 의 비트 0)만 세운다. 다른 비트는 free 로 취급하지 않는다.
    world.pool.physical(world.parent)[11] = 0x01;
    const before = world.pool.bytes();
    receiveRitualChoice(world.pool, world.state, world.create, packetOf(world.parent, 9), undefined);
    assert.deepEqual(world.events, []);
    assert.deepEqual(world.pool.bytes(), before);
    // free 비트가 다른 비트와 함께 서 있어도 free 이다(상태 비트 마스크 검사).
    for (const state of [0x03, 0x05, 0x81, 0xff]) {
        world.pool.physical(world.parent)[11] = state;
        receiveRitualChoice(world.pool, world.state, world.create, packetOf(world.parent, 9), undefined);
        assert.deepEqual(world.events, []);
    }
    // free 비트가 없는 부모(dead 비트 등만 선 것)는 free 가 아니므로 계속 진행한다.
    world.pool.physical(world.parent)[11] = 0x04;
    receiveRitualChoice(world.pool, world.state, world.create, packetOf(world.parent, 9), world.destroy);
    assert.ok(world.events.some((event) => event.startsWith('D:')));
});

test('mask 4 가 선 현재 contained 후보만 표식 후 삭제하고 순서는 체인 순서이다', () => {
    const world = scene([
        { flags: 4 }, { flags: 1 }, { type: 46, flags: 4 }, { flags: 5 }, { flags: 0x0a }, { flags: 4, high: 4 }, { flags: 0 },
    ]);
    const marks = [];
    const destroy = (sid, flags) => { marks.push([sid, world.pool.slot(sid)[22]]); world.destroy(sid, flags); };
    receiveRitualChoice(world.pool, world.state, world.create, packetOf(world.parent, 0x00c0ffee), destroy);
    // 삭제 대상: 첫째(flags 4)·넷째(flags 5)·여섯째(+20=4)만, 순서대로 destroy(sid, 0) 이고 호출 시점에 +22 는 이미 1 이다.
    const [a, b, c, d, e, f, g] = world.sids;
    assert.deepEqual(marks, [[a, 1], [d, 1], [f, 1]]);
    assert.deepEqual(world.events.filter((event) => event.startsWith('D:')), [`D:${a}:0`, `D:${d}:0`, `D:${f}:0`]);
    // 남은 후보는 표식(+22=0xaa)이 그대로이다: mask 없음, 다른 TYPE, 0x0a(비트 4 없음), 상위 바이트만 4, 0.
    for (const sid of [b, c, e, g]) assert.equal(world.pool.slot(sid)[22], 0xaa);
    // 생성은 모든 삭제 뒤 한 번이며 새 Form 이 머리에 들어간다.
    const created = world.events.filter((event) => event.startsWith('C:'));
    assert.equal(created.length, 1);
    assert.equal(world.events.findIndex((event) => event.startsWith('C:')) > world.events.lastIndexOf(`D:${f}:0`), true);
    assert.deepEqual(chain(world.pool, world.parent).slice(1), [b, c, e, g]);
});

test('새 선택 Form 은 같은 부모에 종류·인자 0·flags 4·부착 0·품질 1 로 만든다', () => {
    const world = scene();
    receiveRitualChoice(world.pool, world.state, world.create, packetOf(world.parent, 0x0007a1b2), world.destroy);
    const [sid] = chain(world.pool, world.parent);
    const raw = world.pool.slot(sid);
    assert.deepEqual(world.events, ['C:6:0', `B:${sid}:${world.parent}:0`]);
    // 종류는 +18 WORD(하위 16비트)로 들어가고 인자 +12=0, 품질 +30=1, flags +20=4 이다.
    assert.equal(word(raw, 18), 0xa1b2);
    assert.equal(word(raw, 12), 0);
    assert.equal(raw[30], 1);
    assert.equal(word(raw, 20), 4);
});

test('생성 인자는 종류 DWORD 전체를 부호 없이 넘긴다', () => {
    const world = scene();
    const calls = [];
    const spy = { pool: world.pool, create: (...args) => { calls.push(args); } };
    receiveRitualChoice(world.pool, world.state, spy, packetOf(world.parent, 0xfffffffe), world.destroy);
    assert.deepEqual(calls, [[world.parent, 0xfffffffe, 0, 4, 0, 1]]);
    // 바이트마다 다른 값이 제자리에 들어가는지 본다.
    receiveRitualChoice(world.pool, world.state, spy, packetOf(world.parent, 0x04030201), world.destroy);
    assert.deepEqual(calls[1], [world.parent, 0x04030201, 0, 4, 0, 1]);
    // 종류 0 도 그대로 만든다(원문은 종류를 검사하지 않는다).
    receiveRitualChoice(world.pool, world.state, spy, packetOf(world.parent, 0), world.destroy);
    assert.deepEqual(calls[2], [world.parent, 0, 0, 4, 0, 1]);
});

test('부모 SID 는 패킷 [4..5] 의 리틀엔디언 16비트이고 앞 4바이트는 보지 않는다', () => {
    const world = scene();
    const calls = [];
    const spy = { pool: world.pool, create: (...args) => { calls.push(args); } };
    // 상위 바이트가 있는 부모 번호 0x0101 이 읽히는지 본다. 슬롯 상태를 0 으로 비워 free 비트를 끈다.
    const packet = packetOf(0x0101, 3);
    world.pool.physical(0x0101).fill(0);
    receiveRitualChoice(world.pool, world.state, spy, packet, world.destroy);
    assert.equal(calls[0][0], 0x0101);
    // 앞 4바이트는 무엇이든 같다.
    packet.fill(0, 0, 4);
    receiveRitualChoice(world.pool, world.state, spy, packet, world.destroy);
    assert.equal(calls[1][0], 0x0101);
    // 풀 밖 부모는 슬롯 접근에서 거부한다.
    rejects(() => receiveRitualChoice(world.pool, world.state, spy, packetOf(0xffff, 1), world.destroy), RangeError, /SID 번호/);
    assert.equal(calls.length, 2);
});

test('종류는 삭제 효과가 끝난 뒤에 읽는다', () => {
    const world = scene([{}]);
    const packet = packetOf(world.parent, 0x11111111);
    // 삭제 경계가 패킷의 종류 바이트를 바꾸면 바뀐 값이 생성에 쓰인다(원문 순서).
    const destroy = (sid, flags) => { packet[6] = 0x77; packet[9] = 0x66; world.destroy(sid, flags); };
    const calls = [];
    const spy = { pool: world.pool, create: (...args) => { calls.push(args); } };
    receiveRitualChoice(world.pool, world.state, spy, packet, destroy);
    assert.equal(calls[0][1], 0x66111177);
});

test('삭제 후보가 있는데 삭제 경계가 없으면 표식만 쓰고 거부하며 생성하지 않는다', () => {
    const world = scene([{ flags: 1 }, { flags: 4 }]);
    const calls = [];
    const spy = { pool: world.pool, create: (...args) => { calls.push(args); } };
    rejects(() => receiveRitualChoice(world.pool, world.state, spy, packetOf(world.parent, 5), undefined), Error, /의식 선택의 실제 가상 삭제 미연결/);
    // 원문은 삭제 표식을 쓴 뒤에 삭제 경계를 확인한다.
    assert.equal(world.pool.slot(world.sids[0])[22], 0xaa);
    assert.equal(world.pool.slot(world.sids[1])[22], 1);
    assert.deepEqual(calls, []);
    // 함수가 아닌 값도 같다.
    rejects(() => receiveRitualChoice(world.pool, world.state, spy, packetOf(world.parent, 5), 'x'), Error, /미연결/);
    // 삭제 후보가 없으면 삭제 경계 없이도 생성한다.
    const clean = scene([{ flags: 1 }, { type: 46, flags: 4 }]);
    const cleanCalls = [];
    receiveRitualChoice(clean.pool, clean.state, { pool: clean.pool, create: (...args) => cleanCalls.push(args) }, packetOf(clean.parent, 5), undefined);
    assert.equal(cleanCalls.length, 1);
});

test('삭제 효과가 후보 슬롯과 다음 링크를 지워도 저장한 다음 후보로 계속 돈다', () => {
    const world = scene([{}, {}, {}]);
    receiveRitualChoice(world.pool, world.state, world.create, packetOf(world.parent, 5), world.destroy);
    assert.deepEqual(world.events.filter((event) => event.startsWith('D:')), world.sids.map((sid) => `D:${sid}:0`));
    // 세 후보 모두 지워지고 새 Form 하나만 남는다.
    assert.equal(chain(world.pool, world.parent).length, 1);
});

test('현재 contained TYPE 전역이 바뀌면 후보 판정도 따라간다', () => {
    const world = scene([{ type: 6 }, { type: 7 }]);
    world.state.containedType = 7;
    // 생성기는 INDEPENDENT 상태(6)를 쓰므로 새 Form 의 TYPE 은 6 이다. 삭제는 TYPE 7 후보만 한다.
    receiveRitualChoice(world.pool, world.state, world.create, packetOf(world.parent, 5), world.destroy);
    assert.deepEqual(world.events.filter((event) => event.startsWith('D:')), [`D:${world.sids[1]}:0`]);
});

test('할당 상태가 아닌(free) 삭제 후보는 표식 쓰기에서 거부하고 삭제하지 않는다', () => {
    const world = scene([{ state: 1 }]);
    rejects(() => receiveRitualChoice(world.pool, world.state, world.create, packetOf(world.parent, 5), world.destroy), Error, /할당되지 않은 SID 쓰기/);
    assert.deepEqual(world.events, []);
});

test('권한 없는 dead 부모는 checking 이 켜졌으면 finder 진단을 먼저 부른다', () => {
    const world = scene([{}]);
    world.pool.physical(world.parent)[11] = 0x02;
    world.state.boss = false;
    world.state.checkingDead = 1;
    const seen = [];
    world.state.assertion = (expression, file, line) => seen.push([expression, file, line]);
    receiveRitualChoice(world.pool, world.state, world.create, packetOf(world.parent, 5), world.destroy);
    assert.equal(seen.length, 1);
    assert.equal(seen[0][2], 0x13c);
    // 진단 장치가 없으면 순회 시작에서 거부하고 아무 삭제도 하지 않는다.
    const bare = scene([{}]);
    bare.pool.physical(bare.parent)[11] = 0x02;
    bare.state.boss = false;
    bare.state.checkingDead = 1;
    rejects(() => receiveRitualChoice(bare.pool, bare.state, bare.create, packetOf(bare.parent, 5), bare.destroy), Error, /dead 부모/);
    assert.deepEqual(bare.events, []);
});

test('같은 부모에 두 번 받으면 이전 선택 Form 을 교체한다', () => {
    const world = scene();
    receiveRitualChoice(world.pool, world.state, world.create, packetOf(world.parent, 0x21), world.destroy);
    const [first] = chain(world.pool, world.parent);
    assert.equal(word(world.pool.slot(first), 18), 0x21);
    receiveRitualChoice(world.pool, world.state, world.create, packetOf(world.parent, 0x22), world.destroy);
    const after = chain(world.pool, world.parent);
    // 이전 Form 은 삭제 경계에서 풀렸고 새 Form 하나만 종류 0x22 로 남는다.
    assert.equal(after.length, 1);
    assert.equal(word(world.pool.slot(after[0]), 18), 0x22);
    assert.deepEqual(world.events.filter((event) => event.startsWith('D:')), [`D:${first}:0`]);
    // 세 번째도 같다.
    receiveRitualChoice(world.pool, world.state, world.create, packetOf(world.parent, 0x23), world.destroy);
    assert.equal(chain(world.pool, world.parent).length, 1);
    assert.equal(word(world.pool.slot(chain(world.pool, world.parent)[0]), 18), 0x23);
});
