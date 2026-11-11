// 같은 물리의 ProcessState/Host/Kernel 과 짧은 현재 World 처리기 대여를 분리한 장기 저장소이다(cpppj `client/RawProcessHostStorage.cpp/.h`, 10.78 전용).
// 사용: const storage = new RawProcessHostStorage(pool, types, authority, gameAux);
//   storage.bindDevices(clientKernel, clientClock) 로 실제 Client 의 Kernel 과 프레임 시각(`{ game }`)을 pin 한다(없으면 `storage.kernel` 이 독립 Kernel 을 한 번 만든다).
//   const use = storage.bind(runtime) 로 현재 World 의 처리기 묶음(`createProcessHostRuntime`)을 단일 대여한다 → `use.clearForState()` 는 실제 Kernel.clear 한 번이 정상 복귀했을 때만 `ClearPass`,
//   `use.returnForState(pass)` 는 같은 대여의 증빙·진행 중 호출 없음·Regular 이외 잔류 없음을 확인한 뒤 대여를 반납하고 `Returned`(같은 Kernel 보유)를 돌려준다.
// 웹에는 C++ 소멸자가 없으므로 대여(`Use`)/레거시 회수 대여는 `release()` 로, 저장소 최종 소멸은 `dispose()` 로 명시한다(둘 다 멱등). 이 저장소는 장기 Host(`SquidProcessHost.borrowing`)를 첫 사용에 한 번만 만든다.
// 잔류 Regular 는 old World 반납과 Pool reset 뒤에도 같은 Kernel 에 남고, 새 World 대여의 처리기를 그 위치에서 조회한다. CD(10.72) 독립 계약은 옮기지 않았다.
// 기준선 뒤 cpppj 에서 늘어난 장기 기본 장치 보관(BindLifecycleDefaults/BindFormFactoryDefaults/BindFormTransmissionDefaults)과 fire/rotate 애니메이션 TYPE 참조는 Session 조립(4-D)에서 반영한다(3.6절 ③).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E297: 4-B RawProcessHostStorage 이식)
import { Kernel, PROCESS_CAPACITY } from './kernel.js';
import { SQUID_DESTROY_EFFECT } from './squid-destroy.js';
import { SquidProcessHost, SquidProcessState } from './squid-process.js';

// 증빙 객체를 이 모듈 밖에서 만들지 못하게 하는 비공개 표식이다(C++ 의 private 생성자). (2026-11-11 WB04-E297)
const TOKEN = Symbol('RawProcessHostStorage evidence');

// 실제 단일 Kernel.clear 정상 복귀의 이동 전용 증빙이다. 같은 저장소/대여 정체성을 가리키며 returnForState 가 소비한다. (2026-11-11 WB04-E297)
export class ClearPass {
    // 발급한 저장소와 대여 묶음이다(소비되면 null).
    owner;
    runtime;

    constructor(token, owner, runtime) {
        if (token !== TOKEN) throw new TypeError('ClearPass 는 Use.clearForState 만 발급합니다.');
        this.owner = owner;
        this.runtime = runtime;
    }
}

// 현재 처리기 반납 뒤 Kernel 을 pin 하는 이동 전용 결과이다. 공개 성공 setter 는 만들지 않는다. (2026-11-11 WB04-E297)
export class Returned {
    // 반납 뒤에도 잔류가 old World 보다 오래 살 수 있게 같은 Kernel 소유권을 유지한다.
    kernel;

    constructor(token, kernel) {
        if (token !== TOKEN) throw new TypeError('Returned 는 Use.returnForState 만 발급합니다.');
        this.kernel = kernel;
    }
}

// 현재 World 처리기만 빌리는 단일 대여이며 실제 반납 증빙의 발급 위치이다. (2026-11-11 WB04-E297)
export class Use {
    // 실제 대여 소유권(반납하면 null)과 대여한 처리기 묶음이다.
    #owner;
    #runtime;

    constructor(token, owner, runtime) {
        if (token !== TOKEN) throw new TypeError('Use 는 RawProcessHostStorage.bind 만 발급합니다.');
        this.#owner = owner;
        this.#runtime = runtime;
        owner._setCurrent(runtime);
    }

    // 대여한 처리기 묶음이다(소비되었으면 null).
    get runtime() {
        return this.#runtime;
    }

    // 현재 자기 처리기만 반납한다(Kernel 을 실행하거나 비우지 않음). 다른 바인딩의 현재 슬롯은 건드리지 않고 멱등이다. (C++ ~Use)
    release() {
        if (this.#owner && this.#owner._current() === this.#runtime) this.#owner._setCurrent(null);
    }

    // 현재 World 의 생성 실패/일반 종료에서 자기 대여가 만든 객체만 반납한다. 이전 Native 잔류의 다른 대여는 건드리지 않는다.
    drainOwnedForShutdown() {
        if (this.#owner && this.#owner._current() === this.#runtime) this.#owner.host.drainRuntimeBindings(this.#runtime);
    }

    // 실제 같은 Kernel.clear 한 번이 정상 복귀했을 때만 증빙을 만든다. 예외이면 증빙을 발급하지 않으며 이미 수행된 부분 효과는 되돌리지 않는다.
    clearForState() {
        if (!this.#owner || this.#owner._current() !== this.#runtime) throw new Error('현재 ProcessHost 대여 없는 Native Clear');
        const runtime = this.#runtime;
        // Clear 의 가상 소멸 효과가 진행되는 동안 중첩 반납이 이 호출을 소비하지 못하게 한다.
        runtime.activeCalls += 1;
        try {
            this.#owner.kernel.clear();
        } finally {
            runtime.activeCalls -= 1;
        }
        return new ClearPass(TOKEN, this.#owner, runtime);
    }

    // 같은 대여의 증빙과 현재 잔류 종류를 확인하고 실제 바인딩을 반납한다. 모든 거부는 증빙/대여를 소비하기 전에 한다.
    returnForState(pass) {
        if (!(pass instanceof ClearPass) || !this.#owner || pass.owner !== this.#owner || pass.runtime !== this.#runtime || this.#owner._current() !== this.#runtime) {
            throw new TypeError('Native Clear의 같은 ProcessHost 대여 증빙 필요');
        }
        if (this.#runtime.activeCalls) throw new Error('진행 중 ProcessHost 효과의 현재 대여 반납 금지');
        if (!this.#owner.host.hasOnlyRegularProcesses()) throw new Error('Regular 이외 잔류의 현재 모듈 수명 이행 미완료');
        const kernel = this.#owner.kernel;
        this.#owner._setCurrent(null);
        this.#owner = null;
        this.#runtime = null;
        pass.owner = null;
        pass.runtime = null;
        return new Returned(TOKEN, kernel);
    }
}

// raw 외부 Kernel 의 현재 World 가 가진 SID 소진 회수 callable 단일 대여이다. release 이후에는 조회를 거부한다. (2026-11-11 WB04-E297)
class LegacyReclaim {
    // 회수 함수와 해제 여부이다.
    fn;
    alive = true;

    constructor(token, fn) {
        if (token !== TOKEN) throw new TypeError('LegacyReclaim 은 bindLegacyAllocationReclaim 만 발급합니다.');
        this.fn = fn;
    }

    // 마지막 대여 반납 뒤에는 조회를 거부하게 한다(멱등). (2026-11-11 WB04-E297)
    release() {
        this.alive = false;
    }
}

// 같은 물리의 ProcessState/Host/Kernel 장기 소유자이다. 실행 모듈을 복제하지 않고 실제 공통 Host 의 수명만 소유한다. (2026-11-11 WB04-E297)
export class RawProcessHostStorage {
    // Pool/TYPE 표는 외부 Storage 가 소유한 실제 물리 배열이다(이 저장소보다 오래 산다).
    #pool;
    #types;
    // 실제 권한/게임 보조 TYPE 소유자이다(현재 두 권한 bool 과 같은 긴 수명).
    #authority;
    #gameAux;
    // now fallback/typeRotation/pollType 와 실제 외부 참조를 한 번 소유하는 스칼라 상태이다.
    #state = new SquidProcessState();
    // 현재 짧은 대여의 처리기 묶음(없으면 null)과 raw 외부 Kernel 의 현재 회수 대여(없으면 null)이다.
    #currentRuntime = null;
    #legacy = null;
    // 첫 사용에 만드는 안정된 장기 Host 와 Kernel(독립 또는 외부 pin), 외부 프레임 시각(`{ game }`)이다.
    #host = null;
    #kernel = null;
    #clock = null;
    // 같은 명시 외부 장치와 독립 최종 종료 정책만 구별한다.
    #external = false;

    // authority 는 RawAuthorityState(없으면 독립 값), gameAux 는 GameAuxState(없으면 독립 값; pollType 은 gameAux.geyserType 별칭이다)이다. (2026-11-11 WB04-E297)
    constructor(pool, types, authority = null, gameAux = null) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types)) throw new TypeError('ProcessHost 저장소는 10.78 풀과 TYPE 표를 받습니다.');
        this.#pool = pool;
        this.#types = types;
        this.#authority = authority;
        this.#gameAux = gameAux;
        this.#state.bindAuthority(authority);
        if (gameAux) this.#state.bindGameAux(gameAux);
    }

    // 현재 대여의 처리기 묶음을 읽고 쓰는 내부 접근이다(Use 만 호출). (2026-11-11 WB04-E297)
    _current() {
        return this.#currentRuntime;
    }

    _setCurrent(runtime) {
        this.#currentRuntime = runtime;
    }

    // 같은 실제 Client 장치를 pin 하며 바인딩 이후 다른 장치로 교체하지 않는다. 같은 주소의 재확인은 값을 바꾸지 않는다. 현재 대여/레거시 대여가 있거나 다른 장치이면 거부한다. (2026-11-11 WB04-E297)
    bindDevices(kernel, clock) {
        if (this.#external && this.#kernel === kernel && this.#clock === clock) return;
        if (!kernel || !clock || typeof clock.game !== 'number' || this.#currentRuntime !== null || (this.#legacy && this.#legacy.alive) || (this.#kernel && (this.#kernel !== kernel || this.#clock !== clock))) {
            throw new TypeError('ProcessHost의 같은 장기 Kernel/시각 장치 계약');
        }
        this.#kernel = kernel;
        this.#clock = clock;
        this.#external = true;
        this.#state.bindNow(() => clock.game);
    }

    // raw 입력이 이미 pin 한 같은 실제 Kernel 인지 조회한다(현재 실제 Kernel 주소만 비교). (2026-11-11 WB04-E297)
    usesKernel(kernel) {
        return this.#kernel !== null && this.#kernel === kernel;
    }

    // 명시 장기 외부 장치가 연결됐는지 읽는다. (2026-11-11 WB04-E297)
    hasDevices() {
        return this.#external;
    }

    // 외부 장치가 없으면 같은 저장소의 독립 Kernel 을 한 번만 만든다(독립 판본의 실제 capacity 유지). (2026-11-11 WB04-E297)
    get kernel() {
        if (!this.#kernel) this.#kernel = new Kernel(PROCESS_CAPACITY);
        return this.#kernel;
    }

    // 첫 사용에 안정된 Host 를 만들고 같은 객체를 유지한다. 현재 World 가 없어도 Host 자체를 소유할 수 있다. (2026-11-11 WB04-E297)
    get host() {
        if (!this.#host) this.#host = SquidProcessHost.borrowing(this.#pool, this.#types, this.kernel, this.#state, () => this.#currentRuntime);
        return this.#host;
    }

    // 전체 World/Regular 가 같은 실제 스칼라 저장소를 빌린다. (2026-11-11 WB04-E297)
    get state() {
        return this.#state;
    }

    // Pool 이 긴 수명을 유지해도 종료된 World 주소는 쓰지 않고 현재 모듈 반납을 호출 중 금지한다: SID 소진의 flags 0 가상 삭제(DestroyDependent)를 현재 World 에 요청한다.
    // raw 외부 Kernel 의 레거시 회수 대여가 있으면 그것을 호출하고 끝난다. (2026-11-11 WB04-E297)
    reclaimForAllocation(sid) {
        if (this.#legacy && this.#legacy.alive) {
            this.#legacy.fn(sid);
            return;
        }
        const runtime = this.#currentRuntime;
        if (!runtime || !runtime.asset?.emit) throw new Error('SID 소진 복구의 현재 World 가상 삭제 미연결');
        // 호출이 끝나기 전에는 현재 대여를 반납할 수 없다.
        runtime.activeCalls += 1;
        try {
            runtime.asset.emit({ effect: SQUID_DESTROY_EFFECT.destroyDependent, sid, parent: 0, flags: 0 });
        } finally {
            runtime.activeCalls -= 1;
        }
    }

    // raw 외부 Kernel 의 소유권은 바꾸지 않고 같은 현재 World 의 회수 callable 에만 단일 대여를 발급한다(release 로 반납). (2026-11-11 WB04-E297)
    bindLegacyAllocationReclaim(reclaim) {
        if (typeof reclaim !== 'function' || this.#currentRuntime !== null || (this.#legacy && this.#legacy.alive)) throw new TypeError('SID legacy 회수의 단일 현재 World 대여 계약');
        this.#legacy = new LegacyReclaim(TOKEN, reclaim);
        return this.#legacy;
    }

    // 중복/다른 Pool 은 현재 바인딩을 바꾸기 전에 거부한다. 같은 Host 를 한 번 확보한 뒤 대여를 발급한다. (2026-11-11 WB04-E297)
    bind(runtime) {
        if (!runtime || !runtime.destroy || runtime.destroy.pool !== this.#pool || this.#currentRuntime !== null || (this.#legacy && this.#legacy.alive)) {
            throw new TypeError('ProcessHost의 실제 현재 World 단일 대여 계약');
        }
        if (runtime.factory && runtime.factory.pool !== this.#pool) throw new TypeError('ProcessHost 현재 공장의 다른 Pool 대여');
        void this.host;
        return new Use(TOKEN, this, runtime);
    }

    // 최종 물리 소멸(C++ 소멸자)에서만 잔류를 반납하며 Host/Pool 은 그때까지 유지한다: 독립 Kernel 은 모두, 외부 Kernel 은 이 Host 소유 프로세스만 반납한다. (2026-11-11 WB04-E297)
    dispose() {
        if (this.#kernel && this.#host) {
            if (!this.#external) this.#kernel.drainOwnedForShutdown();
            else this.#kernel.drainOwnedForShutdown((process) => this.#host.owns(process));
        }
        this.#kernel = null;
        this.#host = null;
        this.#currentRuntime = null;
        if (this.#legacy) this.#legacy.release();
    }
}
