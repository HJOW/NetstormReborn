// Carrier 삭제 준비(00426890)와 내용물 검색의 타입·WORD 조건 조회(004ac9f0 FindContained)를 옮긴 모듈이다(cpppj `o/RawCarrierPreDestroy.cpp/.h`, 10.78 전용).
// preDestroy(sid, flags): 할당 슬롯·자산 TYPE 사전 검사 → 하위 Damageable 사전 검사 → Damageable 삭제 준비(0044b4b0) → 그 효과 뒤 현재 boss 를 다시 읽어
//   참이고 FindContained(1, 0, 0) 이 있으면 전역 후처리(00485e40)를 부른다. Damageable 에 넘긴 flags 는 조회 인자에 영향을 주지 않는다.
// findContained(parent, mask, kind, index, allowDead): 공용 내용물 커서(raw-contained-finder.js)로 현재 contained 타입 후보 중 +0x12 WORD == kind(0 이면 생략),
//   +0x14 WORD & mask(0 이면 생략)인 것을 찾고 index 개의 일치를 건너뛴다. 후보 상태/extra 는 따로 거르지 않는다.
// hooks(모두 필수): validateDamageable(sid, flags), damageablePre(sid, flags), dependentFound(). 상태는 ContainedFinderState(Carrier·Damageable·검색 공유)이다.
// 이 모듈은 DOM 을 쓰지 않는다. CD 10.72 분기는 옮기지 않았다. (2026-11-08 WB04-E56: Carrier 삭제 준비·내용물 조회 이식으로 추가)
import { ContainedFinderState, RawContainedFinder } from './raw-contained-finder.js';

// 공통 raw 타입·상태·종류/조건 WORD 위치와 free|contained 비트, 자산 TYPE 범위이다. (2026-11-08 WB04-E56)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const KIND_OFFSET = 18;
const MASK_OFFSET = 20;
const FREE_CONTAINED = 9;
const FIRST_ASSET_TYPE = 70;
const TYPE_COUNT = 188;

// 필수 하위 효과 이름이다. (2026-11-08 WB04-E56)
const REQUIRED_HOOKS = Object.freeze(['validateDamageable', 'damageablePre', 'dependentFound']);

// 비정렬 little endian WORD 를 읽는다. (2026-11-08 WB04-E56)
function word(raw, offset) {
    return raw[offset] | (raw[offset + 1] << 8);
}

// Carrier 삭제 준비와 내용물 조회 몸체이다. 가상 지원 범위를 넓히지 않는 직접 호출용이며 풀·상태·훅 대상은 더 오래 살아야 한다.
// 사용: const carrier = new RawCarrierPreDestroy(pool, state, hooks); carrier.findContained(parent, 4, type, 0). (2026-11-08 WB04-E56)
export class RawCarrierPreDestroy {
    // 같은 실제 SID 풀이다.
    #pool;
    // Carrier·Damageable·내용물 검색이 공유하는 현재 권한/contained 타입/checking 상태이다.
    #state;
    // Damageable 사전 검사·삭제 준비와 전역 후처리의 실제 연결이다.
    #hooks;

    // 전체 Damageable 와 전역 후처리 경계를 공급해야 직접 호출할 수 있다.
    constructor(pool, state, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('Carrier preDestroy 는 10.78 SID 풀만 받습니다.');
        if (!(state instanceof ContainedFinderState)) throw new TypeError('Carrier preDestroy 상태가 필요합니다.');
        // 세 하위 효과가 모두 함수인지 확인한다.
        for (const name of REQUIRED_HOOKS) {
            if (typeof hooks?.[name] !== 'function') throw new TypeError(`Carrier preDestroy 하위 효과 누락: ${name}`);
        }
        this.#pool = pool;
        this.#state = state;
        this.#hooks = { ...hooks };
    }

    // 연결 어댑터가 같은 풀인지 확인한다.
    get pool() { return this.#pool; }

    // 진단 전에 독립 커서를 시작해야 하는 Dais 가 같은 현재 검색 전역을 빌린다(사본이 아니다).
    get finderState() { return this.#state; }

    // 새 가상 지원이나 공간 해제 없이 순수 경계 검사만 한다: 할당 슬롯·free/contained 아님·자산 TYPE·하위 Damageable 사전 검사.
    validate(sid, flags) {
        const raw = this.#pool.allocated(sid);
        if ((raw[STATE_OFFSET] & FREE_CONTAINED) || raw[TYPE_OFFSET] < FIRST_ASSET_TYPE || raw[TYPE_OFFSET] >= TYPE_COUNT) throw new Error('Carrier preDestroy raw 자산 상태/타입 오류');
        this.#hooks.validateDamageable(sid, flags);
    }

    // 00426890: Damageable → 현재 boss → FindContained(1, 0, 0) → 전역 후처리 순서이다.
    preDestroy(sid, flags) {
        this.validate(sid, flags);
        this.#hooks.damageablePre(sid, flags);
        if (this.#state.currentBoss && this.findContained(sid, 1, 0, 0)) this.#hooks.dependentFound();
    }

    // 004ac9f0: 공용 커서는 가장 최근 종속부터 진행하며 다음 링크를 먼저 저장한다. 찾지 못하면 0 이다.
    findContained(parent, mask, kind, index, allowDead = false) {
        const pool = this.#pool;
        const wantedMask = mask >>> 0;
        const wantedKind = kind >>> 0;
        let skip = index >>> 0;
        const finder = new RawContainedFinder(pool, this.#state);
        // 현재 contained 타입 후보만 두 WORD 조건으로 검사하고 index 개의 일치를 건너뛴다.
        for (let current = finder.begin(parent, allowDead); current; current = finder.next()) {
            const candidate = pool.slot(current);
            if ((!wantedKind || word(candidate, KIND_OFFSET) === wantedKind) && (!wantedMask || (word(candidate, MASK_OFFSET) & wantedMask))) {
                if (!skip) return current;
                skip -= 1;
            }
        }
        return 0;
    }
}

// 사제 삭제 준비의 Carrier 경계(validateCarrier·carrierPre)를 같은 풀의 실제 몸체로 바꾼 훅 묶음을 돌려준다(나머지 훅은 보존). (2026-11-08 WB04-E56)
export function makeCarrierPreDestroyHooks(pool, carrier, hooks = {}) {
    if (carrier?.pool !== pool) throw new TypeError('사제/Carrier preDestroy 의 SID 풀이 다릅니다');
    return { ...hooks, validateCarrier: (sid, flags) => carrier.validate(sid, flags), carrierPre: (sid, flags) => carrier.preDestroy(sid, flags) };
}
