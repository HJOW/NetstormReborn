// 다리 낙하 진단 부모를 실제 SquidProcessHost/SidPool/현재 TYPE 표에 연결한 scheduleBridgeFallChecked 를 검사한다
// (cpppj `BridgeFallParentTests.cpp` 의 BridgeFallParent_ActualHostChecksBeforeRegistrationAndKeepsChild 이식, WB04-E244: RawBridgeFallParent 마감).
//  - 진단 실패는 등록 전에 멈춘다(Kernel 크기·풀 빈 슬롯 수 불변). 정상 예약과 복귀 뒤 예약은 기존 좌표 포장(payload 5396 = (20&255)|(21<<8))과 Regular 부착을 쓴다.
//  - checking 이 0 이면 genus 를 읽지 않아 표 밖 TYPE 이어도 예약하고, 비0 이면 매 호출 같은 현재 TYPE 의 flags2 를 다시 진단한다.
//  - 계약: Host/풀 불일치는 TypeError, genus 의 TYPE 번호가 표 밖이면 RangeError, 콜백 없는 기본 진단은 조건/파일/줄을 담은 오류이다.
// 원본 10.78 관측 200행(작은 부모 runBridgeFallParent)은 small-parents.test.mjs 가 대조한다. cpppj 의 세 번째 시험(RawSurfaceStorage 의 Checking 뷰 수명)은 client 저장소 단위(4-B)에서 대조한다. (2026-11-10 WB04-E244)
import assert from 'node:assert/strict';
import test from 'node:test';
import { BRIDGE_FALL_EVENT } from '../www/core/bridge-lifecycle.js';
import { Kernel } from '../www/core/kernel.js';
import { scheduleBridgeFallChecked } from '../www/core/raw-bridge-fall-parent.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';

// 다리 타입 번호(raw 타입 82)와 다리 genus 비트(flags2 0x4)이다. (2026-11-10 WB04-E244)
const BRIDGE_TYPE = 82;
const BRIDGE_GENUS = 0x4;

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-10 WB04-E244)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 같은 풀의 공간 해제·삭제기·Kernel·상태·호스트와 다리 타입(82) 부모 슬롯 하나를 만든다. checking 은 단어 상자를 읽는 getter 이다. (2026-11-10 WB04-E244)
function scene({ now = 9 } = {}) {
    const pool = new SidPool(32768, { server: true });
    const types = formTypeInputs();
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types);
    const kernel = new Kernel();
    const state = new SquidProcessState();
    state.now = now;
    const host = new SquidProcessHost(pool, types, kernel, destroy, state);
    const parent = pool.allocate();
    const raw = pool.allocated(parent);
    raw[10] = BRIDGE_TYPE;
    raw[11] = 0;
    raw[40] = 0;
    const word = { value: 0 };
    return { pool, types, kernel, host, parent, word, checking: () => word.value >>> 0 };
}

// 진단 실패가 예외인지 확인한다(빈 예약 성공으로 오인하지 않음). (2026-11-10 WB04-E244)
function failed(action) {
    try {
        action();
    } catch {
        return true;
    }
    return false;
}

test('scheduleBridgeFallChecked: 진단은 등록 전에 멈추고 복귀·정상 예약은 기존 포장과 Regular 부착을 쓴다', () => {
    const s = scene();
    const first = scheduleBridgeFallChecked(s.host, s.pool, s.types, s.parent, 20.75, 21.9, s.checking);
    assert.ok(first);
    assert.deepEqual([first.event, first.payload, first.time, first.count, s.kernel.size], [BRIDGE_FALL_EVENT, 5396, 9, 0, 1]);
    // checking 이 비0 이면 같은 현재 TYPE(flags2 0)을 다시 진단해 등록 전에 실패한다.
    s.word.value = 1;
    const free = s.pool.freeCount;
    assert.ok(failed(() => scheduleBridgeFallChecked(s.host, s.pool, s.types, s.parent, 20, 21, s.checking)));
    assert.deepEqual([s.kernel.size, s.pool.freeCount], [1, free]);
    // 다리 genus 가 있으면 진단 없이 예약한다.
    s.types[BRIDGE_TYPE].flags2 = BRIDGE_GENUS;
    const second = scheduleBridgeFallChecked(s.host, s.pool, s.types, s.parent, 20.75, 21.9, s.checking);
    assert.ok(second);
    assert.deepEqual([second.payload, s.kernel.size], [first.payload, 2]);
    // 진단 콜백이 복귀하면 같은 SID 와 원래 좌표로 예약까지 이어진다(콜백이 TYPE 을 고쳐도 진단은 한 번이다).
    s.types[BRIDGE_TYPE].flags2 = 0;
    s.word.value = 0x80000000;
    let assertions = 0;
    const resumed = scheduleBridgeFallChecked(s.host, s.pool, s.types, s.parent, 20.75, 21.9, s.checking, (condition, source, line) => {
        assert.deepEqual([condition, source, line], ['s.getGenus() & gBRIDGE', '\\Ns\\O\\Bridge.cpp', 0x323]);
        assertions += 1;
        s.types[BRIDGE_TYPE].flags2 = BRIDGE_GENUS;
    });
    assert.ok(resumed);
    assert.deepEqual([assertions, s.kernel.size, resumed.payload], [1, 3, 5396]);
    // 프레임/낙하 행동은 실행하지 않고 검사가 소유한 프로세스만 반납한다.
    s.kernel.clear();
    assert.equal(s.kernel.size, 0);
});

// fixture 가 구별하지 못하는 연결 경계: 풀 불일치, 표 밖 TYPE, checking 0 의 genus 미요구, 콜백 없는 기본 진단의 원문 조건/파일/줄이다.
test('scheduleBridgeFallChecked: Host/풀 불일치·표 밖 TYPE·checking 0·기본 진단 계약', () => {
    const s = scene();
    // 다른 풀을 넘기면 등록 전에 거부하고 아무것도 만들지 않는다.
    assert.throws(() => scheduleBridgeFallChecked(s.host, new SidPool(32768, { server: true }), s.types, s.parent, 20, 21, s.checking), TypeError);
    assert.equal(s.kernel.size, 0);
    // TYPE 번호가 표 밖이면 checking 비0 의 genus 읽기에서 RangeError 이고 등록하지 않는다. checking 0 은 genus 를 읽지 않아 예약한다.
    s.word.value = 1;
    // 표 길이(188)와 같은 경계 번호와 그보다 큰 번호가 모두 표 밖이다.
    for (const outside of [188, 200]) {
        s.pool.allocated(s.parent)[10] = outside;
        assert.throws(() => scheduleBridgeFallChecked(s.host, s.pool, s.types, s.parent, 20, 21, s.checking), RangeError, `TYPE ${outside}`);
        assert.equal(s.kernel.size, 0);
    }
    s.word.value = 0;
    assert.ok(scheduleBridgeFallChecked(s.host, s.pool, s.types, s.parent, 20, 21, s.checking));
    assert.equal(s.kernel.size, 1);
    // 콜백이 없으면 원문 조건/파일/줄을 담은 오류로 멈춘다.
    s.pool.allocated(s.parent)[10] = BRIDGE_TYPE;
    s.word.value = 1;
    assert.throws(() => scheduleBridgeFallChecked(s.host, s.pool, s.types, s.parent, 20, 21, s.checking), /원문 다리 낙하 진단 \\Ns\\O\\Bridge\.cpp:803: s\.getGenus\(\) & gBRIDGE/);
    assert.equal(s.kernel.size, 1);
    s.kernel.clear();
});
