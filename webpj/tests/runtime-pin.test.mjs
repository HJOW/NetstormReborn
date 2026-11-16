// 실제 GameUse 핸들의 호출 범위 반납과 같은 World/Host/PostPop·종속 Unpop 결선이다(WB04-E626).
// 앱 Session/Load·자연 플레이와 공중3종 새 전용 실행은 하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { createRuntimePin, releaseRuntimePin } from '../www/core/runtime-pin.js';
import { RawSquidContainedUnpop, commonContainedParent, commonContainedPrevious, commonContainedSetPrevious } from '../www/core/squid-contained-unpop.js';
import { campaignOwners } from './campaign-session-support.mjs';

// 실제 owner/World와 실제 ClientGameplayGlobals 대여다. 공개 토큰은 pin API만 검사하는 명시 어댑터이며 전체 Session 구현을 가장하지 않는다.
function scene() {
    const o = campaignOwners(); const g = o.gameplayGlobals; const calls = [];
    const session = { bindRuntimePin: (pin) => o.world.bindRuntimePin(pin ? () => { const hold = pin(); calls.push(g.runtimeCallActive(session)); return hold; } : null) };
    const owner = { o, session }; o.gameplayUse.publish(session); g.bindRuntimeOwner(session, owner);
    return { o, g, session, owner, calls,
        // 외곽 C++ 소멸 순서처럼 pin을 먼저 닫고 공개/하위 자료를 반납한다.
        close() { o.world.bindRuntimePin(null); o.gameplayUse.withdraw(session); o.dispose(); },
    };
}

test('Runtime pin: 실제 발급 증표만 한 번 반납하고 일반 강한 owner의 release는 호출하지 않는다', () => {
    let returned = 0; const owner = {}; const pin = createRuntimePin(owner, () => returned++);
    assert.equal(pin.owner, owner); releaseRuntimePin(pin); assert.equal(pin.owner, null); pin.release(); releaseRuntimePin(pin); assert.equal(returned, 1);
    releaseRuntimePin({ release: () => { throw new Error('일반 owner를 닫으면 안 됨'); } }); releaseRuntimePin(null); releaseRuntimePin(1);
    assert.throws(() => createRuntimePin(null, () => {}), /실제 owner/);
});

test('World Runtime pin: 같은 Factory의 정상·예외 반환이 실제 GameUse 호출 수를 남기지 않는다', () => {
    const s = scene(); const { o, g, session, calls } = s;
    try {
        // Create의 가상 PostCreate에는 원문 pin이 없다. 실제 Take 진입 전체를 선택한다.
        assert.equal(g.runtimeCallActive(session), false); const sid = o.world.create(152); assert.equal(o.core.factory.take(152, sid), sid); assert.ok(calls.length > 0); assert.ok(calls.every(Boolean)); assert.equal(g.runtimeCallActive(session), false);
        assert.throws(() => o.core.factory.take(999, sid)); assert.equal(g.runtimeCallActive(session), false);
        const count = calls.length; o.world.bindRuntimePin(null); o.world.create(152); assert.equal(calls.length, count); assert.equal(g.runtimeCallActive(session), false);
        assert.throws(() => o.world.bindRuntimePin(7), /함수 필요/);
    } finally { s.close(); }
    assert.throws(() => o.world.bindRuntimePin(() => s.owner), /반납한 월드/); o.world.bindRuntimePin(null);
});

test('World Runtime pin: 가상/공통 PostPop 통지는 호출 중 공개 반납을 막고 정상·예외 끝에 모두 반납한다', () => {
    const s = scene(); const { o, g, session } = s; let notices = 0;
    try {
        o.playerState.playerKnown = 1;
        o.core.postPop.bindAiNotice(() => { notices++; assert.equal(g.runtimeCallActive(session), true); assert.throws(() => o.gameplayUse.withdraw(session), /배송 종료 전/); });
        o.player.create(2); assert.equal(notices, 1); assert.equal(g.runtimeCallActive(session), false);
        o.core.postPop.bindAiNotice(() => { assert.equal(g.runtimeCallActive(session), true); throw new Error('선택된 통지 실패'); });
        assert.throws(() => o.player.create(3), /선택된 통지 실패/); assert.equal(g.runtimeCallActive(session), false);
    } finally { s.close(); }
});

test('World Runtime pin: 같은 Process 대여의 실제 Regular 부착/조회가 호출 수를 균형 있게 보유한다', () => {
    const s = scene(); const { o, g, session, calls } = s;
    try {
        const parent = o.world.create(85); calls.length = 0; const process = o.core.processes.addRegular(parent, 3, 2.5); assert.ok(process); assert.ok(calls.length > 0); assert.equal(g.runtimeCallActive(session), false);
        assert.equal(o.core.processes.findEvent(parent, 3), process); assert.equal(g.runtimeCallActive(session), false);
    } finally { s.close(); }
});

test('World Runtime pin: 후처리 공개 진입의 pin 실패는 사전 검사와 깊이 쓰기보다 먼저 중단한다', () => {
    const o = campaignOwners(); const failure = new Error('실제 owner 보유 실패'); const bytes = o.core.pool.bytes(); const depth = o.core.bookkeeping.depth;
    try {
        o.world.bindRuntimePin(() => { throw failure; });
        // 실제 세 공개 진입 모두 잘못된 SID를 읽거나 공통 깊이를 쓰기 전에 같은 실패를 전달한다.
        for (const name of ['activate', 'postPopVirtual', 'postPop']) {
            assert.throws(() => o.core.postPop[name](0xffffffff, 0), (error) => error === failure);
            assert.equal(o.core.bookkeeping.depth, depth); assert.deepEqual(o.core.pool.bytes(), bytes);
        }
    } finally { o.world.bindRuntimePin(null); o.dispose(); }
});

test('World Runtime pin: 실제 종속 링크의 getter·Detach/NotifyUi 중첩과 예외 반환이 같은 전체 owner를 보유한다', () => {
    const s = scene(); const { o, g, session, calls } = s; const pool = o.core.pool; let throwNotice = false; let notices = 0;
    try {
        const body = new RawSquidContainedUnpop(pool, { contentFormType: () => 6, gumpFormType: () => 3, uiEvent: () => 0x02000031, checking: () => 0, inMetamapMode: () => false, boss: () => true }, {
            parent: (sid) => commonContainedParent(pool, sid), previous: (sid) => commonContainedPrevious(pool, sid), setPrevious: (sid, previous) => commonContainedSetPrevious(pool, sid, previous),
            parentNotice: () => { notices++; assert.equal(g.runtimeCallActive(session), true); if (throwNotice) throw new Error('선택된 부모 통지 실패'); },
            command: () => { throw new Error('미선택 UI 사건'); }, assertion: () => { throw new Error('미선택 링크 진단'); },
        });
        o.world.bindContainedUnpop(body); calls.length = 0; assert.equal(body.contentFormType, 6); assert.equal(calls.length, 1); assert.equal(g.runtimeCallActive(session), false);
        const parent = o.world.create(152); const child = o.contained.create(parent, 71, 0, 4, 0x10, 0); calls.length = 0;
        body.unpop(child); assert.ok(calls.length >= 3); assert.equal(notices, 1); assert.equal(g.runtimeCallActive(session), false);
        const second = o.contained.create(parent, 71, 0, 4, 0x10, 0); throwNotice = true; assert.throws(() => body.unpop(second), /선택된 부모 통지 실패/); assert.equal(g.runtimeCallActive(session), false);
        assert.throws(() => body.bindRuntimePin(3), /함수 필요/);
    } finally { s.close(); }
});
