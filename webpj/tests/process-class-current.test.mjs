// Process 호스트의 공통 생성 부모(현재 DWORD 번호)와 현재 VT 가 정하는 클래스 수명을 검사한다(WB04-D5c, cpppj ProcessCreationCurrentTests·ProcessClassCurrentTests 의 독립 호스트 부분 이식).
//  - 독립 호스트도 고정 VT 대신 원문 공통 Create 와 전체 DWORD 번호를 쓴다: 번호가 0xffffffff 로 바뀌면 TYPE BYTE 로 줄이지 않고 거부하며 할당·Kernel 등록은 되돌리지 않는다.
//  - TYPE 별칭과 콜백의 TYPE 변경에도 실제 Process VT/PID 가 부모 통지·삭제 통지·소멸자·공통 깊이를 결정한다(10.78: 상한 변경 입력 포함 32가지 조합).
//  - 원문 부모 통지의 NULL 생략과 preDestroy 의 필수 PID/선택된 가상 장치(다른 Kernel 클래스)는 구별한다.
// 실제 World(RawSurfaceWorld) 대여의 같은 장면(생성 공장 연속 대여, World 삭제)은 WB05 에서 옮긴다. CD 10.72 입력은 범위 밖이다. (2026-11-08 WB04-D5c: Process 호스트 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { get, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SQUID_FORM_VTABLES } from '../www/core/squid-vtables.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { BaseProcess, Kernel } from '../www/core/kernel.js';
import { SquidFormDispatch } from '../www/core/squid-form-dispatch.js';
import { SquidProcess, SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-08 WB04-D5c)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 오류가 나는지 확인한다(성공을 무조건 인정하지 않음). (2026-11-08 WB04-D5c)
function throws(action) {
    try {
        action();
        return false;
    } catch {
        return true;
    }
}

// 같은 풀의 공장·삭제기 묶음이다. (2026-11-08 WB04-D5c)
function parts(types = formTypeInputs(), { server = true } = {}) {
    const pool = new SidPool(32768, { server });
    const factory = new SquidFactory(pool, types);
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types);
    return { pool, types, factory, destroy };
}

// 독립 입력도 고정 VT 대신 원문 생성자/번호 관문을 사용한다: 번호 7(생성자 주소는 Process)로 만든 Form 의 VT 는 Process 이고, 번호가 전체 DWORD 0xffffffff 가 되면 거부하되 앞선 할당·등록은 남는다.
test('SquidProcessHost: 독립 호스트는 공통 Create 와 현재 전체 DWORD 번호를 쓴다', () => {
    const types = formTypeInputs();
    types[7].constructorAddress = typeConstructorAddress(2);
    const { pool, factory, destroy } = parts(types);
    const kernel = new Kernel();
    const state = new SquidProcessState();
    const aux = new GameAuxState();
    aux.processFormType = 7;
    state.bindGameAux(aux);
    const host = new SquidProcessHost(pool, types, kernel, destroy, state);
    const parent = factory.create(74);
    const process = host.addRegular(parent, 0x123, 1);
    assert.ok(process);
    assert.equal(get(pool.slot(process.form), 0), SQUID_FORM_VTABLES.process);
    // 전체 DWORD 를 2 나 BYTE 로 바꾸지 않는다.
    aux.processFormType = 0xffffffff;
    const free = pool.freeCount;
    const registered = kernel.size;
    assert.ok(throws(() => host.addRegular(parent, 0x124, 1)));
    assert.deepEqual([pool.freeCount, kernel.size], [free - 1, registered + 1]);
    kernel.clear();
});

// 실제 Kernel 제거 안의 가상 콜백/소멸자 시점을 관찰하는 작은 프로세스이다. (2026-11-08 WB04-D5c)
class ClassProbe extends SquidProcess {
    constructor(kernel, events) {
        super();
        this.kernel = kernel;
        this.events = events;
        // 부모 통지가 실행할 명시적인 바이트 변경과 삭제 통지가 실행할 명시적인 바이트 변경이다.
        this.parentEffect = null;
        this.destroyEffect = null;
    }

    runFrame() {}

    // 원본 가상 +10 에서 현재 TYPE 을 바꾸는 입력이다.
    onParentDying(parent) {
        this.events.push(`parent${parent}`);
        if (this.parentEffect) this.parentEffect(parent);
    }

    // 원본 가상 +c 뒤 다음 공통 효과가 현재 클래스를 다시 사용해야 한다.
    onFormDestroy(flags) {
        this.events.push(`pre${flags}`);
        if (this.destroyEffect) this.destroyEffect();
    }

    // Kernel 은 가상 소멸자가 끝난 뒤 슬롯을 지운다.
    destroy() { this.events.push(this.kernel.get(this.pid) === this ? 'destructor-live' : 'destructor-missing'); }
}

// client bare 부모의 실제 공통 깊이만 반납하며 송신은 선택되면 실패하는 자산 경계이다. (2026-11-08 WB04-D5c)
function classAssets(destroy) {
    return {
        selected: () => 0,
        emit: (event) => {
            if (event.effect === SQUID_DESTROY_EFFECT.preDestroy) destroy.completePreDestroy();
            else if (event.effect === SQUID_DESTROY_EFFECT.postDestroy) destroy.completePostDestroy();
            else throw new Error('Process 클래스 검사에서 선택하지 않은 자산/전송 효과');
        },
    };
}

// TYPE 별칭과 콜백의 TYPE 변경에도 실제 Process VT/PID 가 부모/삭제/소멸자·공통 깊이를 결정한다. 상한 변경(narrowEnd)은 이미 생성한 클래스의 삭제에 쓰이지 않는다.
test('SquidProcessHost: 현재 VT 가 TYPE 별칭과 콜백의 TYPE 변경을 무시하고 수명을 정한다', () => {
    let inputs = 0;
    for (const narrowEnd of [false, true]) {
        for (const destroyParent of [false, true]) {
            for (const type of [1, 2, 3, 6, 7, 46, 63, 69]) {
                const { pool, types, factory, destroy } = parts(formTypeInputs(), { server: false });
                const kernel = new Kernel();
                const state = new SquidProcessState();
                const events = [];
                const dispatch = new SquidFormDispatch(pool, destroy, classAssets(destroy));
                const host = new SquidProcessHost(pool, types, kernel, destroy, state, { asset: dispatch.hooks() });
                dispatch.bind(host);
                assert.ok(throws(() => dispatch.bind(host)), '같은 클래스 재등록은 거부');
                const parent = factory.create(74, 2);
                const process = host.attach(new ClassProbe(kernel, events), 10, parent, 0x50);
                assert.ok(process);
                const form = process.form;
                const pid = kernel.find(process);
                if (narrowEnd) state.processTypeEnd = 0;
                pool.allocated(form)[10] = type;
                // 부모 통지는 자식 dead 설정 전에 실행된다.
                process.parentEffect = (actual) => {
                    assert.ok(actual === parent && !(pool.slot(form)[11] & 2));
                    pool.allocated(form)[10] = 6;
                };
                // 같은 VT 의 후처리를 raw TYPE 3 의 Gump 로 바꾸지 않는다.
                process.destroyEffect = () => {
                    assert.ok((pool.slot(form)[11] & 2) && pool.slot(form)[10] === (destroyParent ? 6 : type));
                    pool.allocated(form)[10] = 3;
                };
                const label = `narrowEnd=${narrowEnd} parent=${destroyParent} type=${type}`;
                if (destroyParent) assert.equal(dispatch.destroy(parent, 0x10), true, label);
                else host.kill(process, 0x10);
                assert.deepEqual(events, destroyParent ? [`parent${parent}`, 'pre80', 'destructor-live'] : ['pre16', 'destructor-live'], label);
                assert.deepEqual([kernel.get(pid), kernel.size, pool.slot(form)[11] & 3, pool.slot(form)[10], destroy.preDepth, destroy.postDepth], [null, 0, 3, 3, 0, 0], label);
                if (!destroyParent) {
                    assert.equal(get(pool.slot(parent), 6, 2), 0);
                    assert.equal(dispatch.destroy(parent, 0x10), true);
                }
                inputs += 1;
            }
        }
    }
    assert.equal(inputs, 32);
});

// 원문 부모 통지의 NULL 생략과 preDestroy 의 필수 PID/선택된 가상 장치를 구별한다: PID 0 은 release 통지를 빈 통지로 생략하고, 비어 있지 않은 다른 Kernel 클래스는 거부한다.
// 이어지는 공통 삭제의 preDestroy 는 두 경우 모두 필수 PID 에서 막혀 이미 쓴 dead/깊이를 되돌리지 않는다.
test('SquidProcessHost: 부모 통지의 NULL 생략과 다른 Kernel 클래스는 구별된다', () => {
    for (const foreign of [false, true]) {
        const { pool, types, factory, destroy } = parts(formTypeInputs(), { server: false });
        const kernel = new Kernel();
        const dispatch = new SquidFormDispatch(pool, destroy, classAssets(destroy));
        const host = new SquidProcessHost(pool, types, kernel, destroy, new SquidProcessState(), { asset: dispatch.hooks() });
        dispatch.bind(host);
        const form = factory.create(2, 2);
        const raw = pool.allocated(form);
        raw[10] = foreign ? 46 : 2;
        const pid = foreign ? kernel.add(new BaseProcess()) : 0;
        put(raw, 18, pid);
        const free = pool.freeCount;
        assert.equal(throws(() => dispatch.hooks().emit({ effect: SQUID_DESTROY_EFFECT.releaseDependent, sid: form, parent: 50, flags: 0 })), foreign);
        assert.deepEqual([raw[11], pool.freeCount, destroy.preDepth, destroy.postDepth], [4, free, 0, 0]);
        // PID 0/잘못된 장치를 성공으로 숨기거나 이미 쓴 dead/깊이를 되돌리지 않는다.
        assert.ok(throws(() => dispatch.destroy(form, 0x10)));
        assert.deepEqual([raw[11], pool.freeCount, destroy.preDepth, destroy.postDepth, kernel.size], [6, free, 1, 0, foreign ? 1 : 0]);
    }
});
