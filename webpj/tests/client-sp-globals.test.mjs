// Client 수명 SP 전역(client-sp-globals.js)과 Client 시각 공급원(client-time-source.js)을 원본 10.78 관측과 cpppj 시험, 웹 계약으로 확인한다.
//  - 원본 관측: sp-initialization-x86.tsv 48행(시드 0/42/4294967295 × 이전 완전 초기화 0/1 × 중단 지점 0/1/16/32 번째 할당 × x87 정밀도 모드 두 가지): 할당 호출 수·초기화 완료 여부·비트 offset·추적 offset·게임 난수 상태·첫 배열 확보 여부와
//    실제 1152BYTE 분산 배열 전체를 대조한다(중단된 부분 초기화도 되돌리지 않음). 웹은 x87 모드를 구별하지 않으며 두 모드 행이 같은 값을 내야 한다.
//  - cpppj 시험 이식: ClientSpGlobalsTests 의 명시 시작 시험(같은 난수와 장기 추적 offset), ClientTimeSourceTests 2건(기본 OS 시계·고정 시각 모드). 실제 Client 생성 시험은 Client 조립(4-D E)에서 옮긴다.
//  - 웹 계약: 생성자 거부, 숨은 초기화 없음, 소유자 정체성, 기본 할당 입력(0), 모드 0 에서도 재시드는 시각을 읽고 모드를 되돌림, 공급자 범위 오류, 두 소유자 독립, 고정 시각 기본값/범위/time32 경계.
// 실제 게임·브라우저 실행이 아니다. (2026-11-12 WB04-E336: 4-D ClientSpGlobals 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { ClientSpGlobals } from '../www/core/client-sp-globals.js';
import { ClientTimeSource } from '../www/core/client-time-source.js';
import { ClientRandomGlobals, GameRandom, GameRandomSeed } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-12 WB04-E336)
const initRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'sp-initialization-x86.tsv'))?.filter((row) => row[0] !== 'seed') ?? null;

// 예외가 나는지 본다. (2026-11-12 WB04-E336)
function throws(action, kind) {
    try { action(); } catch (error) { return kind ? error instanceof kind : true; }
    return false;
}

// 원본 operator_new 정상 반환 전에 중단하는 명시 검사 입력이다. (2026-11-12 WB04-E336)
class SpInitStop extends Error {}

// 정수 배열을 리틀 엔디언 바이트 16진 문자열로 만든다. (2026-11-12 WB04-E336)
function poolsHex(pools) {
    return Array.from(new Uint8Array(pools.buffer, pools.byteOffset, pools.byteLength), (byte) => byte.toString(16).padStart(2, '0')).join('');
}

test('SP 초기화 원본 관측 48행: 할당 수·offset·추적 offset·난수 상태·첫 배열·1152BYTE 분산 배열(중단 포함)', { skip: !initRows }, () => {
    assert.equal(initRows.length, 48);
    // 시드·이전 초기화 횟수·중단 지점 조합마다 새 저장소를 만든다.
    for (const row of initRows) {
        assert.equal(row.length, 12);
        const seed = Number(row[0]);
        const before = Number(row[1]);
        const stop = Number(row[2]);
        let allocations = 0;
        let stopped = false;
        const tracking = { value: 0 };
        const random = new GameRandom();
        const store = new ScrambledSpStore(random, () => seed, (bytes) => {
            assert.equal(bytes.length, 36);
            allocations += 1;
            // 원문 operator_new 에 제공했던 같은 명시 입력이다.
            for (let byte = 0; byte < bytes.length; byte += 1) bytes[byte] = (allocations * 13 + byte * 7) & 255;
            if (stop && allocations === before * 32 + stop) throw new SpInitStop();
        });
        store.bindTrackingOffset(tracking);
        try {
            // 이전 완전 초기화를 거친 뒤 마지막 초기화에서 중단할 수 있다.
            for (let pass = 0; pass <= before; pass += 1) store.initialize();
        } catch (error) {
            if (!(error instanceof SpInitStop)) throw error;
            stopped = true;
        }
        const label = row.slice(0, 5).join(' ');
        assert.equal(stopped, row[4] === 'stop', label);
        assert.equal(allocations, Number(row[5]), label);
        assert.equal(store.initialized, Number(row[6]) !== 0, label);
        assert.equal(store.offset, Number(row[7]), label);
        assert.equal(tracking.value >>> 0, Number(row[8]), label);
        assert.equal(random.state, Number(row[9]), label);
        assert.equal(store.firstPoolReady, Number(row[10]) !== 0, label);
        assert.equal(row[11].length, 288 * 8);
        assert.equal(poolsHex(store.pools()), row[11], label);
    }
});

test('ClientSpGlobals 명시 시작: 같은 난수와 장기 추적 offset 을 쓰고 생성/접근자는 초기화하지 않는다', () => {
    const random = new ClientRandomGlobals();
    random.gameMode = 7;
    let timeCalls = 0;
    let allocations = 0;
    const owner = new ClientSpGlobals(random, () => { timeCalls += 1; return 946684800; }, (bytes) => {
        assert.equal(bytes.length, 36);
        allocations += 1;
        bytes.fill(allocations & 255);
    });
    const store = owner.store();
    assert.equal(store.initialized, false);
    assert.equal(store.firstPoolReady, false);
    assert.equal(store.offset, 0);
    assert.equal(owner.tracking().value, 0);
    assert.equal(timeCalls, 0);
    assert.equal(allocations, 0);
    assert.equal(random.game.state, 55);
    store.initialize();
    assert.equal(store.initialized, true);
    assert.equal(store.firstPoolReady, true);
    assert.equal(timeCalls, 1);
    assert.equal(allocations, 32);
    assert.equal(random.gameMode, 7);
    assert.notEqual(random.crt.state, 1);
    // 저장소가 읽는 추적 offset 은 소유자의 같은 상자이다.
    assert.equal(store.trackingOffset, owner.tracking().value);
    assert.equal(owner.tracking().value, store.offset | 0);
    store.set(1, 0x44960000);
    const pools = store.pools();
    const state = random.game.state;
    // 읽기 접근자와 소유자 교체는 재시드나 재초기화를 추가하지 않는다.
    assert.equal(owner.store(), store);
    assert.equal(owner.randomGlobals(), random);
    assert.deepEqual(store.pools(), pools);
    assert.equal(random.game.state, state);
    assert.equal(store.get(1), 0x44960000);
    assert.equal(timeCalls, 1);
    assert.equal(allocations, 32);
    // 소유자를 거치지 않고 직접 조립한 같은 입력의 저장소(같은 GameRandom 을 씀)와 offset·난수 상태·분산 배열이 같다(소유자의 난수가 저장소의 난수이다).
    const reference = new ClientRandomGlobals();
    reference.gameMode = 7;
    let referenceAllocations = 0;
    const referenceSeed = new GameRandomSeed(reference, () => 946684800);
    const referenceStore = new ScrambledSpStore(reference.game, () => referenceSeed.reseed(), (bytes) => { referenceAllocations += 1; bytes.fill(referenceAllocations & 255); });
    referenceStore.bindTrackingOffset({ value: 0 });
    referenceStore.initialize();
    referenceStore.set(1, 0x44960000);
    assert.equal(referenceStore.offset, store.offset);
    assert.deepEqual(referenceStore.pools(), store.pools());
    assert.equal(reference.game.state, random.game.state);
    assert.equal(reference.crt.state, random.crt.state);
});

test('ClientSpGlobals 생성: 난수 소유자·시각 공급자가 없으면 거부하고 기본 할당 입력은 새 할당의 0 이다', () => {
    assert.ok(throws(() => new ClientSpGlobals(null, () => 1), TypeError));
    assert.ok(throws(() => new ClientSpGlobals({ crt: {}, game: {} }, () => 1), TypeError));
    assert.throws(() => new ClientSpGlobals({ crt: {}, game: {} }, () => 1), /난수 소유자 누락/);
    assert.throws(() => new ClientSpGlobals(null, () => 1), /난수 소유자 누락/);
    assert.ok(throws(() => new ClientSpGlobals(new ClientRandomGlobals(), null), TypeError));
    assert.ok(throws(() => new ClientSpGlobals(new ClientRandomGlobals(), 12), TypeError));
    // 할당 입력을 주지 않으면 36BYTE 할당마다 0 이 들어간다(웹 적응, 비결정적 힙을 읽지 않음).
    const random = new ClientRandomGlobals();
    const owner = new ClientSpGlobals(random, () => 946684800);
    owner.store().initialize();
    assert.ok(owner.store().pools().every((word) => word === 0));
    assert.equal(owner.store().get(3), 0);
});

test('ClientSpGlobals 재시드: 난수 모드가 0 이어도 시각을 읽고 모드 DWORD 를 되돌리며, 공급자 범위 오류도 모드를 되돌린다', () => {
    const random = new ClientRandomGlobals();
    let calls = 0;
    const owner = new ClientSpGlobals(random, () => { calls += 1; return 123456; });
    assert.equal(random.gameMode, 0);
    owner.store().initialize();
    assert.equal(calls, 1);
    assert.equal(random.gameMode, 0);
    // 같은 시각이면 같은 난수 상태를 만든다(결정적 재시드).
    const other = new ClientRandomGlobals();
    new ClientSpGlobals(other, () => 123456).store().initialize();
    assert.equal(other.game.state, random.game.state);
    // signed 32비트를 벗어난 초는 거부하고 모드 DWORD 를 되돌린다.
    const bad = new ClientRandomGlobals();
    bad.gameMode = 5;
    const failing = new ClientSpGlobals(bad, () => 2 ** 31);
    assert.ok(throws(() => failing.store().initialize(), RangeError));
    assert.equal(bad.gameMode, 5);
    assert.equal(failing.store().initialized, false);
});

test('두 ClientSpGlobals 는 서로 독립이며 한 소유자의 초기화가 다른 소유자의 난수/저장소/추적 offset 을 바꾸지 않는다', () => {
    const randomA = new ClientRandomGlobals();
    const randomB = new ClientRandomGlobals();
    const a = new ClientSpGlobals(randomA, () => 111);
    const b = new ClientSpGlobals(randomB, () => 222);
    a.store().initialize();
    assert.equal(b.store().initialized, false);
    assert.equal(randomB.game.state, 55);
    assert.equal(b.tracking().value, 0);
    assert.notEqual(a.tracking(), b.tracking());
    assert.notEqual(a.store(), b.store());
    b.store().initialize();
    assert.notEqual(randomA.game.state, randomB.game.state);
});

// ───── ClientTimeSource ─────

test('ClientTimeSource 기본: 꺼져 있고 전진은 아무 일도 하지 않으며 호스트 시계를 32비트 범위로 읽는다', () => {
    const source = new ClientTimeSource({ hostMilliseconds: () => 4294967296 + 5, hostUnixSeconds: () => 1234567890 });
    assert.equal(source.fixedEnabled, false);
    source.advanceFixed();
    assert.equal(source.unixSeconds(), 1234567890);
    // 호스트 밀리초는 부호 없는 32비트로 감긴다(원본 timeGetTime).
    assert.equal(source.milliseconds(), 5);
    // 주입하지 않은 기본 호스트 시계도 읽힌다.
    const real = new ClientTimeSource();
    const before = Math.floor(Date.now() / 1000);
    const seconds = real.unixSeconds();
    assert.ok(seconds >= before && seconds <= Math.floor(Date.now() / 1000));
    assert.ok(Number.isInteger(real.milliseconds()) && real.milliseconds() >= 0 && real.milliseconds() <= 0xffffffff);
});

test('ClientTimeSource 고정 모드: 원점에서 시작해 프레임마다 같은 밀리초만 전진하고 유닉스 초는 경과 초를 내림해 더한다', () => {
    const source = new ClientTimeSource();
    source.enableFixed({ frameMilliseconds: 25, originMilliseconds: 4294967000, unixSeconds: 946684800 });
    assert.equal(source.fixedEnabled, true);
    assert.equal(source.milliseconds(), 4294967000);
    assert.equal(source.milliseconds(), 4294967000);
    assert.equal(source.unixSeconds(), 946684800);
    // 40프레임(1000ms) 전진 뒤 값과 부호 없는 32비트 감김을 확인한다.
    for (let frame = 0; frame < 40; frame += 1) source.advanceFixed();
    assert.equal(source.milliseconds(), (4294967000 + 1000) >>> 0);
    assert.equal(source.milliseconds(), 704);
    assert.equal(source.unixSeconds(), 946684801);
    // 두 번째 켜기는 거부하고 상태를 바꾸지 않는다.
    assert.ok(throws(() => source.enableFixed({ frameMilliseconds: 25 }), Error));
    assert.equal(source.milliseconds(), 704);
    source.disableFixedForTests();
    assert.equal(source.fixedEnabled, false);
    // 범위 밖 설정은 거부하고 켜지 않는다.
    assert.ok(throws(() => source.enableFixed({ frameMilliseconds: 0 }), RangeError));
    assert.ok(throws(() => source.enableFixed({ frameMilliseconds: 60001 }), RangeError));
    assert.ok(throws(() => source.enableFixed({ frameMilliseconds: 25, unixSeconds: -1 }), RangeError));
    assert.ok(throws(() => source.enableFixed({ frameMilliseconds: 2.5 }), RangeError));
    assert.ok(throws(() => source.enableFixed({ frameMilliseconds: 25, unixSeconds: 1.5 }), RangeError));
    assert.equal(source.fixedEnabled, false);
    // 경계 값(1, 60000, 유닉스 초 0)은 허용한다.
    source.enableFixed({ frameMilliseconds: 1, unixSeconds: 0 });
    assert.equal(source.unixSeconds(), 0);
    source.disableFixedForTests();
    source.enableFixed({ frameMilliseconds: 60000 });
    assert.equal(source.fixedEnabled, true);
});

test('ClientTimeSource 기본 호스트 시계: performance.now 의 정수부와 Date.now 초의 정수부를 읽는다', () => {
    const realNow = Object.getOwnPropertyDescriptor(performance, 'now');
    const realDate = Date.now;
    try {
        // 기본 공급 함수가 호출 때 전역 시계를 읽으므로 임시로 소수 값을 돌려준다.
        Object.defineProperty(performance, 'now', { value: () => 1234.9, configurable: true, writable: true });
        Date.now = () => 1234567890999;
        const source = new ClientTimeSource();
        assert.equal(source.milliseconds(), 1234);
        assert.equal(source.unixSeconds(), 1234567890);
    } finally {
        if (realNow) Object.defineProperty(performance, 'now', realNow);
        else delete performance.now;
        Date.now = realDate;
    }
});

test('ClientTimeSource 고정 모드: 경과 초는 반올림하지 않고 내림한다(600ms 는 0초, 1000ms 는 1초)', () => {
    const source = new ClientTimeSource();
    source.enableFixed({ frameMilliseconds: 300, unixSeconds: 100 });
    source.advanceFixed();
    source.advanceFixed();
    assert.equal(source.milliseconds(), 1000600);
    assert.equal(source.unixSeconds(), 100);
    source.advanceFixed();
    source.advanceFixed();
    assert.equal(source.unixSeconds(), 101);
    assert.equal(Number.isInteger(source.unixSeconds()), true);
});

test('ClientTimeSource 고정 모드 기본값과 끄기: 원점 1000000·2000-01-01 이며 끄면 경과가 0 으로 돌아간다', () => {
    const source = new ClientTimeSource();
    source.enableFixed({ frameMilliseconds: 40 });
    assert.equal(source.milliseconds(), 1000000);
    assert.equal(source.unixSeconds(), 946684800);
    // 3프레임(120ms)은 초를 바꾸지 않고 25프레임(1000ms)은 초를 하나 올린다.
    for (let frame = 0; frame < 3; frame += 1) source.advanceFixed();
    assert.equal(source.milliseconds(), 1000120);
    assert.equal(source.unixSeconds(), 946684800);
    for (let frame = 0; frame < 22; frame += 1) source.advanceFixed();
    assert.equal(source.unixSeconds(), 946684801);
    // 끄면 경과와 설정이 처음으로 돌아가고 호스트 시계를 읽는다.
    const host = new ClientTimeSource({ hostMilliseconds: () => 77, hostUnixSeconds: () => 99 });
    host.enableFixed({ frameMilliseconds: 10, originMilliseconds: 5, unixSeconds: 50 });
    host.advanceFixed();
    host.disableFixedForTests();
    assert.equal(host.milliseconds(), 77);
    assert.equal(host.unixSeconds(), 99);
    host.enableFixed({ frameMilliseconds: 10 });
    assert.equal(host.milliseconds(), 1000000);
});

test('ClientTimeSource time32 범위: 호스트/고정 초가 0 미만이거나 2^31-1 을 넘으면 거부한다', () => {
    let seconds = 0;
    const source = new ClientTimeSource({ hostUnixSeconds: () => seconds });
    for (const [value, ok] of [[0, true], [2147483647, true], [-1, false], [2147483648, false], [Number.NaN, false]]) {
        seconds = value;
        if (ok) assert.equal(source.unixSeconds(), value);
        else assert.ok(throws(() => source.unixSeconds(), RangeError), String(value));
    }
    // 고정 모드도 경과 초가 time32 상한을 넘으면 거부한다.
    const fixed = new ClientTimeSource();
    fixed.enableFixed({ frameMilliseconds: 60000, unixSeconds: 2147483647 - 60 });
    assert.equal(fixed.unixSeconds(), 2147483587);
    fixed.advanceFixed();
    assert.equal(fixed.unixSeconds(), 2147483647);
    fixed.advanceFixed();
    assert.ok(throws(() => fixed.unixSeconds(), RangeError));
});
