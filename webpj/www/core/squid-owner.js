// Squid 의 소유자 지정(vtable +0x74)을 raw SID 풀과 소유자별 작업장 목록에 연결하는 처리기이다(cpppj `o/SquidOwner.cpp/.h`).
// 요새(편집)/전투 모드에서 genus 가 vortex·factory(0x4200)이면 이전 소유자의 작업장 목록에서 이 번호를 모두 빼고, void·buried 가 아니면 새 소유자의 목록 끝에 넣는다(가득 차면 넣지 않음).
// 소유자 0 은 목록이 없다. 범위 밖 번호는 실제 원문 진단이 정상 복귀하면 하위 BYTE 를 쓰며 목록 물리(소유자 9 이상)는 선택 시 확인한다.
// 공통 몸체는 raw-common-owner.js, 목록과 장부는 squid-postpop-state.js 이다. 타입 표는 생성 때 복사하고(bindDiagnostics 의 types 를 주면 빌려서 매번 현재 값을 읽음) 판본 길이(188)를 확인한다.
// 근거: cpppj `o/SquidOwner.cpp/.h`(기준선 1f00a25a). 원본 관측 owner-x86.tsv(tests/squid-owner.test.mjs). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E2: 소유자 지정 이식으로 추가)
import { setCommonSquidOwner } from './raw-common-owner.js';

// 소유자 지정이 읽는 원본 모드 전역이다(10.78 주소): 요새(편집) 00594fb8, 전투 00594fbc(둘 중 하나가 켜져 있어야 작업장 목록을 고침), challenge 00594fc8(켜져 있으면 유효한 소유자 번호가 1~39). (2026-11-08 WB04-E2)
export class SquidOwnerMode {
    fort = false;
    battle = false;
    challenge = false;
}

// 소유 플레이어 변경을 객체 필드와 장부에 반영하는 처리기이다.
// 사용: const owner = new SquidOwner(pool, types, bookkeeping, mode); owner.set(sid, 3). (2026-11-08 WB04-E2)
export class SquidOwner {
    // 같은 풀·복사한 타입 표·장부·모드이다.
    #pool;
    #types;
    #bookkeeping;
    #mode;
    // 같은 World 의 TYPE 와 Client 진단을 선택 위치에서 빌린다(비어 있으면 생성 시 사본을 쓴다).
    #diagnostics = { types: null, assertion: null };

    // 풀·장부·모드는 이 객체보다 오래 살아야 한다. 타입 표는 복사한다.
    constructor(pool, types, bookkeeping, mode) {
        if (pool?.edition !== 'patch1078') throw new TypeError('소유자 지정은 10.78 SID 풀만 받습니다.');
        if (!Array.isArray(types) || types.length !== 188) throw new RangeError('소유자 지정 타입 판본/크기 오류');
        this.#pool = pool;
        this.#types = types.map((record) => ({ ...record }));
        this.#bookkeeping = bookkeeping;
        this.#mode = mode;
    }

    // 같은 실제 풀이다(가상 효과 어댑터가 같은 풀의 소유자를 변경하는지 확인).
    get pool() { return this.#pool; }

    // 현재 TYPE 길이를 확인하고 실제 진단 장치를 빌린다. types 는 188칸 현재 표(없으면 null), assertion 은 (expression, file, line).
    bindDiagnostics({ types = null, assertion = null } = {}) {
        if (types !== null && types.length !== this.#types.length) throw new RangeError('공통 Owner 현재 TYPE 판본/길이 오류');
        this.#diagnostics = { types, assertion };
    }

    // 선택된 위치에서 현재 TYPE 의 물리 범위를 확인하고 그 genus(flags2)를 돌려준다.
    #genus(type) {
        const current = this.#diagnostics.types ?? this.#types;
        if (type >= current.length) throw new RangeError('공통 Owner 현재 TYPE 물리 범위');
        return current[type].flags2 >>> 0;
    }

    // 소유자 번호에 해당하는 작업장 목록을 고른다. 소유자 0 은 목록이 없고 9 이상은 현재 Player 물리 범위 밖이다.
    #list(owner, action) {
        if (!owner) return null;
        if (owner >= this.#bookkeeping.ownerFactories.length) throw new RangeError(`공통 Owner ${action}의 현재 Player 물리 범위`);
        return this.#bookkeeping.ownerFactories[owner];
    }

    // 004adf00 ↔ CD 004aefa0: 객체의 소유자 바이트를 바꾼다.
    set(sid, player) {
        setCommonSquidOwner(this.#pool, sid, player, {
            fort: () => this.#mode.fort,
            battle: () => this.#mode.battle,
            challenge: () => this.#mode.challenge,
            genus: (type) => this.#genus(type),
            assertion: this.#diagnostics.assertion,
            // 이전 소유자의 목록에서 같은 번호를 모두 빼고 나머지를 앞으로 당긴다(개수 밖의 값은 건드리지 않음).
            remove: (owner, value) => this.#list(owner, '제거')?.removeAll(value),
            // 가득 찬 목록에는 넣지 않고 기존 항목과 남은 메모리를 그대로 둔다.
            append: (owner, value) => this.#list(owner, '추가')?.append(value),
        });
    }
}
