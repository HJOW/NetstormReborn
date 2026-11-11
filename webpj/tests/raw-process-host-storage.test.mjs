// 장기 ProcessState/Host/Kernel 저장소(cpppj `client/RawProcessHostStorage`)의 웹 대응 `core/raw-process-host-storage.js` 를 검사한다(4-B, WB04-E297).
// cpppj `ProcessHostStorageTests` 의 계약 장면을 RawSurfaceWorld 없이 저장소 수준으로 옮겼다(World 대신 `createProcessHostRuntime` 묶음을 대여한다):
//  - Native 단일 Clear 의 잔류 Regular 가 old 대여 반납·Pool reset 뒤에도 같은 PID 로 남고 새 대여의 처리기를 그 위치에서 조회한다(일반/공유 두 factory).
//  - 무효 부모(free/dead/void)와 예약 시각의 기존 관문은 현재 처리기를 요구하는 위치보다 앞에 남는다.
//  - Clear 증빙과 장치 정체성: 다른 저장소의 증빙/대여/장치는 현재 바인딩과 PID 를 그대로 둔 채 거부한다.
//  - 실제 Client 시각/권한(빌린 getter)은 스냅샷을 되쓰지 않는다. 외부 장치는 저장소가 pin 한다. 독립 최종 소멸은 Host/Pool 이 가상 소멸자보다 오래 산다.
//  - Regular 이외 잔류는 현재 대여를 반납하지 않고 부분 효과/PID 를 보존한다. 실행 중 효과는 그 호출의 대여를 반납할 수 없다. 이전 증빙의 재진입은 거부한다.
// 원본 관측 파일이 없는 단위 계약 검사이다. 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E297)
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { RawAuthorityState } from '../www/core/raw-authority-state.js';
import { BaseProcess, Kernel } from '../www/core/kernel.js';
import { RegularProcess, SquidProcess, createProcessHostRuntime } from '../www/core/squid-process.js';
import { ClearPass, RawProcessHostStorage, Returned, Use } from '../www/core/raw-process-host-storage.js';

// 실제 ProcessForm 생성에 필요한 4개 TYPE 의 생성자 주소만 채운 188칸 입력이다. (2026-11-11 WB04-E297)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 실제 Kernel 가상 소멸자에서만 외부 효과를 수행하는 순서 관찰기이다. (2026-11-11 WB04-E297)
class FinalAction extends BaseProcess {
    #action;

    constructor(action) {
        super();
        this.#action = action;
    }

    destroy() {
        this.#action();
    }
}

// 아직 장기 logic 이행을 하지 않은 다른 파생(Regular 가 아닌 SquidProcess)이다. (2026-11-11 WB04-E297)
class OtherProcess extends SquidProcess {
    #action;

    constructor(action = () => {}) {
        super();
        this.#action = action;
    }

    destroy() {
        this.#action();
    }

    runFrame() {}
}

// 같은 Pool 의 공통 삭제기·공장·장기 저장소와 다리 타입(82) 부모 슬롯 하나를 만든다. (2026-11-11 WB04-E297)
function world({ aux = new GameAuxState(), authority = new RawAuthorityState() } = {}) {
    const types = formTypeInputs();
    const pool = new SidPool(32768, { server: false });
    const factory = new SquidFactory(pool, types);
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types);
    const storage = new RawProcessHostStorage(pool, types, authority, aux);
    const parent = pool.allocate(2);
    const raw = pool.allocated(parent);
    raw[10] = 82;
    raw[11] = 0;
    raw[40] = 0;
    const runtime = (extra = {}) => createProcessHostRuntime({ destroy, factory, postPopDepth: () => 0, asset: { emit: () => {}, selected: () => 0 }, ...extra });
    return { types, pool, factory, destroy, storage, parent, runtime, aux, authority };
}

// 오류가 나는지 확인한다(성공을 무조건 인정하지 않음). (2026-11-11 WB04-E297)
function rejects(action) {
    try {
        action();
    } catch {
        return true;
    }
    return false;
}

// 지난 슬롯 재등록은 같은 Clear 에서 남고 old 대여 반납/Pool reset 뒤에도 같은 PID 를 유지하며 새 대여의 처리기를 쓴다(일반/공유 Regular). (2026-11-11 WB04-E297)
test('저장소: Native Clear 의 잔류 Regular 는 새 대여의 처리기로 이어진다', () => {
    for (const shared of [false, true]) {
        const s = world();
        const host = s.storage.host;
        const kernel = s.storage.kernel;
        const state = s.storage.state;
        let residual = null;
        const use = s.storage.bind(s.runtime());
        state.now = 5.25;
        assert.equal(s.storage.host, host);
        assert.equal(s.storage.kernel, kernel);
        assert.equal(kernel.add(new FinalAction(() => {})), 1);
        assert.equal(kernel.add(new FinalAction(() => { residual = shared ? host.addSharedRegular(s.parent, 0x12345678, 9.5) : host.addRegular(s.parent, 0x12345678, 9.5); })), 2);
        const pass = use.clearForState();
        assert.ok(pass instanceof ClearPass);
        assert.ok(residual && kernel.find(residual) === 1 && kernel.size === 1);
        const rotation = state.typeRotation;
        const returned = use.returnForState(pass);
        assert.ok(returned instanceof Returned);
        assert.equal(returned.kernel, kernel);
        // 반납 뒤에도 잔류와 타입 순환은 그대로이고, 풀을 reset 해도 Kernel 슬롯은 그대로이다.
        assert.equal(kernel.get(1), residual);
        assert.equal(state.typeRotation, rotation);
        s.pool.reset(0);
        kernel.runFrame();
        assert.equal(kernel.get(1), residual);
        assert.equal(residual.count, 0);
        assert.equal(residual.time, 5.25);
        // 새 대여는 같은 Host/상태/Kernel 을 쓰고 재할당된 같은 부모에서 새 처리기를 호출한다.
        const parent = s.pool.allocate(2);
        assert.equal(parent, s.parent);
        const reused = s.pool.allocated(parent);
        reused[10] = 82;
        reused[11] = 0;
        reused[40] = 0;
        const calls = [];
        const next = s.storage.bind(s.runtime({ regular: (sid, event, count, payload) => { calls.push([sid, event, count, payload]); return 1.25; } }));
        assert.equal(s.storage.host, host);
        assert.equal(s.storage.state, state);
        assert.equal(s.storage.kernel, kernel);
        state.now = 11;
        kernel.runFrame();
        assert.deepEqual(calls, [[parent, 0x12345678, 0, 9.5]]);
        assert.equal(kernel.get(1), residual);
        assert.equal(residual.count, 1);
        assert.equal(residual.payload, 1.25);
        assert.equal(residual.time, 12.25);
        assert.equal(state.typeRotation, rotation);
        next.release();
        assert.equal(kernel.size, 1);
        s.storage.dispose();
    }
});

// free/dead/void 와 예약 시각의 기존 관문은 현재 처리기를 요구하는 위치보다 앞에 남는다. (2026-11-11 WB04-E297)
test('저장소: 무효 부모와 예약 시각은 반납한 처리기를 요구하지 않는다', () => {
    const s = world();
    const use = s.storage.bind(s.runtime());
    let residual = null;
    const kernel = s.storage.kernel;
    kernel.add(new FinalAction(() => {}));
    kernel.add(new FinalAction(() => { residual = s.storage.host.addRegular(s.parent, 71, 3.0); }));
    use.clearForState();
    assert.ok(residual && kernel.find(residual) === 1);
    // 원본이 현재 BYTE 상태(free=1, dead=2, void=4)를 각 Run 에서 다시 읽는다.
    for (const state of [1, 2, 4]) {
        s.pool.physical(s.parent)[11] = state;
        kernel.runFrame();
        assert.equal(kernel.get(1), residual);
        assert.equal(residual.count, 0);
    }
    s.pool.physical(s.parent)[11] = 0;
    residual.setNextTime(100);
    s.storage.state.now = 3;
    kernel.runFrame();
    assert.equal(residual.time, 100);
    assert.equal(residual.count, 0);
    // 유효 부모의 선택된 현재 가상 사건은 현재 World 대여가 없으면 빈 성공이 아니라 거부한다.
    use.release();
    s.storage.state.now = 101;
    assert.ok(rejects(() => kernel.runFrame()));
    assert.equal(kernel.get(1), residual);
    assert.equal(residual.count, 0);
});

// 같은 Clear 증빙을 다른 저장소/대여에 쓰면 현재 바인딩과 PID 를 그대로 남기고 거부한다. 다른 풀의 처리기/장치도 거부한다. (2026-11-11 WB04-E297)
test('저장소: Clear 증빙과 장치/풀 정체성이 다른 소유자를 거부한다', () => {
    const first = world();
    const second = world();
    const firstUse = first.storage.bind(first.runtime());
    const secondUse = second.storage.bind(second.runtime());
    assert.notEqual(first.storage.kernel, second.storage.kernel);
    assert.notEqual(first.storage.host, second.storage.host);
    assert.notEqual(first.storage.state, second.storage.state);
    first.storage.state.now = 7;
    second.storage.state.now = 19;
    assert.equal(first.storage.state.now, 7);
    assert.equal(second.storage.state.now, 19);
    const pass = firstUse.clearForState();
    assert.ok(rejects(() => secondUse.returnForState(pass)));
    assert.equal(pass.owner, first.storage);
    firstUse.returnForState(pass);
    assert.equal(pass.owner, null);
    // 소비한 증빙은 다시 쓸 수 없다.
    assert.ok(rejects(() => firstUse.returnForState(pass)));
    assert.ok(rejects(() => secondUse.returnForState({ owner: second.storage, runtime: secondUse.runtime })));
    // 다른 풀의 삭제기/공장 묶음과 이미 대여 중인 저장소의 중복 대여는 현재 바인딩을 바꾸기 전에 거부한다.
    const foreign = createProcessHostRuntime({ destroy: second.destroy });
    assert.ok(rejects(() => first.storage.bind(foreign)));
    assert.ok(rejects(() => second.storage.bind(createProcessHostRuntime({ destroy: second.destroy }))));
    assert.ok(rejects(() => first.storage.bind(createProcessHostRuntime({ destroy: first.destroy, factory: second.factory }))));
    assert.ok(rejects(() => first.storage.bind(null)));
    assert.ok(rejects(() => first.storage.bind({})));
    // 대여 중이거나 다른 장치이면 장치 교체를 거부한다.
    assert.ok(rejects(() => second.storage.bindDevices(new Kernel(), { game: 1 })));
    secondUse.release();
    assert.ok(rejects(() => second.storage.bindDevices(null, { game: 1 })));
    assert.ok(rejects(() => second.storage.bindDevices(new Kernel(), null)));
    first.storage.dispose();
    second.storage.dispose();
});

// 실제 Client 의 같은 Kernel/FrameTime/Authority 와 GameAux TYPE owner 를 직접 빌리고, 반납이 시각을 0 으로 바꾸지 않는다. (2026-11-11 WB04-E297)
test('저장소: 외부 Kernel/시각/권한/보조 TYPE 은 직접 빌리며 스냅샷을 되쓰지 않는다', () => {
    const s = world();
    const kernel = new Kernel();
    const clock = { game: 31.75 };
    s.storage.bindDevices(kernel, clock);
    assert.ok(s.storage.hasDevices() && s.storage.usesKernel(kernel) && !s.storage.usesKernel(new Kernel()));
    // 같은 주소의 재확인은 값을 바꾸지 않고 통과한다.
    s.storage.bindDevices(kernel, clock);
    assert.equal(s.storage.kernel, kernel);
    assert.equal(s.storage.state.now, 31.75);
    clock.game = 40.5;
    assert.equal(s.storage.state.now, 40.5);
    // 권한 owner 와 GameAux 의 TYPE 소유자가 같은 현재 값이다.
    s.authority.boss = false;
    s.authority.server = false;
    assert.deepEqual([s.storage.state.boss, s.storage.state.server], [false, false]);
    s.storage.state.boss = true;
    assert.equal(s.authority.boss, true);
    s.aux.regularProcessType = 47;
    s.aux.geyserType = 123;
    assert.deepEqual([s.storage.state.regularType, s.storage.state.pollType], [47, 123]);
    // 처리기 대여 뒤 일반 Regular 를 만들면 현재 시각으로 예약되고 반납 뒤에도 시각이 0 으로 바뀌지 않는다.
    const use = s.storage.bind(s.runtime({ regular: () => 0.75 }));
    const process = s.storage.host.addSharedRegular(s.parent, 19, 2);
    assert.ok(process);
    assert.equal(process.time, 40.5);
    clock.game = 41;
    const pass = use.clearForState();
    use.returnForState(pass);
    assert.equal(s.storage.state.now, 41);
    assert.equal(clock.game, 41);
    // 대여 중에는 장치 교체가 거부되고, 다른 Kernel/시각으로의 교체는 처음 장치 뒤에는 늘 거부된다.
    assert.ok(rejects(() => s.storage.bindDevices(new Kernel(), clock)));
    assert.ok(rejects(() => s.storage.bindDevices(kernel, { game: 1 })));
    s.storage.dispose();
});

// 독립 fallback 의 최종 Kernel 소멸에서도 Host/Pool 이 가상 소멸자보다 오래 산다. 외부 Kernel 의 최종 소멸은 이 Host 소유 프로세스만 반납한다. (2026-11-11 WB04-E297)
test('저장소: 최종 소멸은 Host/Pool 을 유지한 채 자기 프로세스만 반납한다', () => {
    const s = world();
    let alive = false;
    let destroyed = 0;
    const use = s.storage.bind(s.runtime());
    const pass = use.clearForState();
    s.storage.kernel.add(new FinalAction(() => { alive = s.storage.host.pool === s.pool && s.pool.capacity === 32768; destroyed += 1; }));
    use.returnForState(pass);
    s.storage.dispose();
    assert.equal(destroyed, 1);
    assert.ok(alive);
    // 외부 Kernel: Client 소유 프로세스는 남기고 이 Host 가 만든 Regular 만 정리한다.
    const t = world();
    const kernel = new Kernel();
    const clientProcess = new FinalAction(() => {});
    kernel.add(clientProcess);
    t.storage.bindDevices(kernel, { game: 2 });
    const useT = t.storage.bind(t.runtime({ regular: () => 1 }));
    const regular = t.storage.host.addRegular(t.parent, 5, 1);
    assert.ok(regular instanceof RegularProcess && kernel.size === 2);
    useT.release();
    t.storage.dispose();
    assert.equal(kernel.size, 1);
    assert.equal(kernel.find(clientProcess), 1);
});

// 단일 Clear 뒤 다른 파생이 남으면 현재 binding 을 반납하지 않고 부분 효과와 실제 PID 를 보존한다. 일반 종료(drainOwnedForShutdown)는 자기 대여가 만든 것을 정리한다. (2026-11-11 WB04-E297)
test('저장소: Regular 이외의 잔류는 반납을 거부하고 PID 를 보존한다', () => {
    const s = world();
    const use = s.storage.bind(s.runtime());
    let residual = null;
    const kernel = s.storage.kernel;
    kernel.add(new FinalAction(() => {}));
    kernel.add(new FinalAction(() => { residual = s.storage.host.attach(new OtherProcess(), 10, s.parent, 0x50); }));
    const pass = use.clearForState();
    assert.ok(residual && kernel.find(residual) === 1);
    assert.ok(rejects(() => use.returnForState(pass)));
    assert.ok(use.runtime && kernel.get(1) === residual);
    assert.equal(pass.owner, s.storage);
    use.drainOwnedForShutdown();
    assert.equal(kernel.size, 0);
    use.release();
    // 반납을 한 번 거부해도 다른 대여가 이어서 가능하다(현재 슬롯이 비워졌으므로).
    assert.ok(s.storage.bind(s.runtime()));
    s.storage.dispose();
});

// 현재 Regular 효과가 실행 중일 때는 그 호출의 대여를 반납할 수 없고 정상 복귀 후 같은 증빙을 소비한다. 실제 Clear 의 소멸 callback 은 지난 증빙의 재진입을 진행 중으로 거부한다. (2026-11-11 WB04-E297)
test('저장소: 진행 중 효과/Clear 중에는 대여를 반납할 수 없다', () => {
    const s = world();
    const kernel = s.storage.kernel;
    let rejectedInside = false;
    let calls = 0;
    let pass = null;
    const use = s.storage.bind(s.runtime({
        regular: () => {
            // 현재 Regular 효과가 실행 중인 호출의 대여는 반납할 수 없다.
            rejectedInside = rejects(() => use.returnForState(pass));
            calls += 1;
            return 1;
        },
    }));
    pass = use.clearForState();
    const regular = s.storage.host.addRegular(s.parent, 7, 1);
    kernel.runFrame();
    assert.equal(calls, 1);
    assert.ok(rejectedInside && regular);
    assert.ok(use.runtime && kernel.get(1) === regular);
    // 효과가 정상 복귀한 뒤에는 같은 증빙으로 반납된다(남은 것은 Regular 뿐).
    assert.ok(use.returnForState(pass) instanceof Returned);
    assert.equal(use.runtime, null);
    // 지난 증빙의 재진입: 새 Clear 의 소멸 callback 안에서는 '진행 중' 으로 거부된다.
    const again = s.storage.bind(s.runtime());
    const previous = again.clearForState();
    let message = '';
    kernel.add(new FinalAction(() => {
        try {
            again.returnForState(previous);
        } catch (error) {
            message = String(error.message);
        }
    }));
    const current = again.clearForState();
    assert.ok(message.includes('진행 중'));
    assert.equal(kernel.size, 0);
    assert.ok(again.returnForState(current) instanceof Returned);
    assert.equal(again.runtime, null);
    s.storage.dispose();
});

// SID 소진 복구는 현재 World 의 가상 삭제(DestroyDependent, flags 0)를 요청하고 호출 동안 대여를 pin 한다. 레거시 회수 대여는 단일이며 release 뒤에는 조회를 거부한다. (2026-11-11 WB04-E297)
test('저장소: SID 소진 복구와 레거시 회수 대여', () => {
    const s = world();
    // 현재 World 없음/미연결은 거부한다.
    assert.throws(() => s.storage.reclaimForAllocation(7), /미연결/);
    const events = [];
    let inside = -1;
    const runtime = s.runtime({ asset: { emit: (event) => { events.push(event); inside = runtime.activeCalls; }, selected: () => 0 } });
    const use = s.storage.bind(runtime);
    s.storage.reclaimForAllocation(9);
    assert.deepEqual(events, [{ effect: 'destroyDependent', sid: 9, parent: 0, flags: 0 }]);
    assert.equal(inside, 1);
    assert.equal(runtime.activeCalls, 0);
    // emit 이 없는 묶음은 거부한다.
    use.release();
    const bare = s.storage.bind(createProcessHostRuntime({ destroy: s.destroy }));
    assert.throws(() => s.storage.reclaimForAllocation(9), /미연결/);
    bare.release();
    // 레거시 회수: 대여 중이면 거부하고, 단일 대여 동안 회수 함수가 호출되며 release 뒤에는 현재 World 경로로 돌아간다.
    const holding = s.storage.bind(s.runtime());
    assert.throws(() => s.storage.bindLegacyAllocationReclaim(() => {}), /단일 현재 World 대여/);
    holding.release();
    const sids = [];
    const legacy = s.storage.bindLegacyAllocationReclaim((sid) => sids.push(sid));
    assert.throws(() => s.storage.bindLegacyAllocationReclaim(() => {}), /단일 현재 World 대여/);
    assert.throws(() => s.storage.bind(s.runtime()), /단일/);
    assert.throws(() => s.storage.bindLegacyAllocationReclaim(null), /단일 현재 World 대여/);
    s.storage.reclaimForAllocation(12);
    assert.deepEqual(sids, [12]);
    legacy.release();
    legacy.release();
    assert.throws(() => s.storage.reclaimForAllocation(13), /미연결/);
    assert.deepEqual(sids, [12]);
    s.storage.bind(s.runtime()).release();
    s.storage.dispose();
});

// 생성 입력과 증빙 위조를 거부한다. (2026-11-11 WB04-E297)
test('저장소: 구성 입력과 증빙 위조를 거부한다', () => {
    const s = world();
    assert.throws(() => new RawProcessHostStorage({}, s.types, null, null), TypeError);
    assert.throws(() => new RawProcessHostStorage(s.pool, null, null, null), TypeError);
    assert.throws(() => new ClearPass(Symbol('x'), s.storage, {}), TypeError);
    assert.throws(() => new Returned(Symbol('x'), new Kernel()), TypeError);
    // 권한/보조 owner 없이도 독립 값으로 만든다.
    const alone = new RawProcessHostStorage(s.pool, s.types);
    alone.state.now = 3;
    assert.equal(alone.state.now, 3);
    assert.equal(alone.state.regularType, 46);
    assert.ok(!alone.hasDevices());
    // 다른 대여(release 한 Use)로 Clear 하면 거부한다.
    const use = s.storage.bind(s.runtime());
    use.release();
    assert.throws(() => use.clearForState(), /현재 ProcessHost 대여 없는/);
    use.release();
    s.storage.dispose();
    s.storage.dispose();
});

// ───────────── 변형 확인으로 보강한 계약 ─────────────

// 반납한(release) 대여와 지난 증빙은 새 대여의 현재 슬롯과 프로세스에 영향을 주지 않는다. (2026-11-11 WB04-E297)
test('저장소: 낡은 대여/증빙은 새 대여를 바꾸지 못한다', () => {
    const s = world();
    // release 는 자기 현재 슬롯만 비운다: A 를 release 하고 B 가 대여한 뒤 A 를 다시 release 해도 B 는 현재이다.
    const a = s.storage.bind(s.runtime());
    a.release();
    const b = s.storage.bind(s.runtime());
    a.release();
    const passB = b.clearForState();
    assert.ok(b.returnForState(passB) instanceof Returned);
    // drainOwnedForShutdown 도 현재 대여일 때만: A 가 만든 Regular 는 A 를 release 한 뒤의 A.drain 으로 사라지지 않는다.
    const c = s.storage.bind(s.runtime({ regular: () => 1 }));
    const regular = s.storage.host.addRegular(s.parent, 3, 1);
    assert.ok(regular);
    c.release();
    const d = s.storage.bind(s.runtime());
    c.drainOwnedForShutdown();
    assert.equal(s.storage.kernel.find(regular), 1);
    d.release();
    // 지난 대여의 증빙은 새 대여로 반납할 수 없다(같은 저장소, 다른 처리기 묶음).
    const e = s.storage.bind(s.runtime());
    const passE = e.clearForState();
    e.release();
    const f = s.storage.bind(s.runtime());
    assert.ok(rejects(() => f.returnForState(passE)));
    assert.ok(f.runtime && passE.owner === s.storage);
    f.release();
    // release 한 대여는 현재가 아니므로 증빙이 있어도 반납할 수 없다.
    const g = s.storage.bind(s.runtime());
    const passG = g.clearForState();
    g.release();
    assert.ok(rejects(() => g.returnForState(passG)));
    // 같은 Pool 위의 다른 저장소가 같은 처리기 묶음을 빌려도 증빙의 저장소 정체성이 다르면 거부한다.
    const t = world();
    const other = new RawProcessHostStorage(t.pool, t.types, t.authority, t.aux);
    const shared = t.runtime();
    const first = t.storage.bind(shared);
    const second = other.bind(shared);
    const passFirst = first.clearForState();
    assert.ok(rejects(() => second.returnForState(passFirst)));
    assert.equal(passFirst.owner, t.storage);
    other.dispose();
    t.storage.dispose();
    s.storage.dispose();
});

// 장치 pin: 대여 중에도 같은 장치의 재확인은 통과하고, 레거시 회수 대여 중이거나 이미 만든 독립 Kernel 이 있으면 다른 장치를 거부한다. 최종 소멸 뒤 새 Kernel 을 만든다. (2026-11-11 WB04-E297)
test('저장소: 장치 pin 규칙과 독립 Kernel/소멸 수명', () => {
    const s = world();
    assert.equal(s.storage.usesKernel(null), false);
    assert.equal(s.storage.kernel.capacity, 39999);
    s.storage.dispose();
    const second = world();
    const kernel = new Kernel();
    const clock = { game: 4 };
    second.storage.bindDevices(kernel, clock);
    const use = second.storage.bind(second.runtime());
    // 대여 중에도 같은 주소의 재확인은 값을 바꾸지 않고 통과한다.
    second.storage.bindDevices(kernel, clock);
    use.release();
    // 레거시 회수 대여 중에는 새 장치를 거부한다(Kernel 이 아직 없는 새 저장소).
    const fresh = world();
    const legacy = fresh.storage.bindLegacyAllocationReclaim(() => {});
    assert.ok(rejects(() => fresh.storage.bindDevices(new Kernel(), { game: 1 })));
    legacy.release();
    fresh.storage.bindDevices(new Kernel(), { game: 1 });
    // 대여 때 만든 독립 Kernel 이 있으면 다른 장치로의 교체를 거부한다.
    const bound = world();
    bound.storage.bind(bound.runtime()).release();
    assert.ok(rejects(() => bound.storage.bindDevices(new Kernel(), { game: 1 })));
    // 레거시 대여 함수 종류.
    assert.throws(() => world().storage.bindLegacyAllocationReclaim(null), /단일 현재 World 대여/);
    // 최종 소멸 뒤에는 새 독립 Kernel 을 만든다.
    const survivor = world();
    const before = survivor.storage.kernel;
    survivor.storage.dispose();
    assert.notEqual(survivor.storage.kernel, before);
    // 대여 증빙을 밖에서 만들 수 없다.
    assert.throws(() => new Use(Symbol('x'), survivor.storage, survivor.runtime()), TypeError);
});
