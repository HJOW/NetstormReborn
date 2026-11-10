// 공통 postPop 장부의 자료 구조를 옮긴 모듈이다(cpppj `o/SquidPostPop.h` 의 SquidPostPopList·RawTotalMadeStorage·SquidPostPopState).
// postPop 처리기(SquidPostPop) 자체와 종류별 후처리는 이후 단위에서 이 장부 위에 올라간다. 소유자 지정(squid-owner.js)과 삭제 장부가 같은 장부 객체를 공유한다.
//  - SquidPostPopList: 원본 미리 확보한 DWORD 배열(entries)과 활성 개수(count)의 목록. 실제 Player 배열은 현재 +0/+4/+8(저장소·용량·개수)을 매번 읽는 binding 으로 연결하고
//    (bindCurrent), 연결하지 않은 독립 목록만 entries/count 를 소유한다. 추가(append)는 용량이 남을 때 배열 store 뒤 count 를 올리고, 전체 제거(removeAll)는 중복 전체를 안정 압축하며
//    count 밖의 stale 꼬리는 보존한다. items() 는 활성 항목만 읽으며 손상된 count(용량 초과)는 거부한다.
//  - RawTotalMadeStorage: Totalmade 의 로컬 수·누적 로컬 생산 수·전역 수 세 256칸 DWORD 표.
//  - SquidPostPopState: 억제/Graph/보류 배치/로컬 소유자/무효 Graph 번호, AI 부착 표시(9칸), 전역 비용(signed), 공급/작업장 재계산 알림과 Activate 깊이,
//    세 Totalmade 표, 공급원/공장 목록과 소유자별 공장 목록(9칸). 실제 World 는 bindTotalMade/bindTotalCost/bindProviders/bindFactories 로 장기 저장소를 빌려 사본 동기화를 하지 않는다.
// 근거: cpppj `o/SquidPostPop.h`(기준선 1f00a25a, 목록 메서드는 `o/SquidPostPop.cpp`). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E2: 소유자 지정과 함께 장부 구조 이식)

// 플레이어 수(번호 1..8, 0 은 소유자 없음)이다. 소유자별 목록은 PLAYER_COUNT + 1 칸이다. (2026-11-08 WB04-E2)
export const PLAYER_COUNT = 8;

// Totalmade 표의 칸 수이다(TYPE BYTE 로 인덱싱). (2026-11-08 WB04-E2)
export const TOTAL_MADE_SIZE = 256;

// 객체 후처리 Pop 이 등록한 대상과 그 개수를 보관하는 목록이다.
// 사용: const list = new SquidPostPopList(); list.entries = [0, 0, 0]; list.append(50); list.removeAll(50). (2026-11-08 WB04-E2)
export class SquidPostPopList {
    // 원본 미리 확보한 DWORD 배열이다. entries.length 가 capacity 이다(독립 목록).
    entries = [];

    // 활성 항목 수이다(독립 목록).
    count = 0;

    // 실제 Player 배열의 현재 +0/+4/+8 연결이다: { storage() → 배열형, capacity() → 용량, readCount() → 개수, writeCount(값), sourceOwner?, sourceSlot? }. 연결하지 않으면 null.
    // sourceOwner/sourceSlot 은 원본 값이 아닌 실제 저장소/행의 정체성이며 명시 Native 재사용만 matchesCurrentSource 로 비교한다. (2026-11-10 WB04-E258: cpppj 기준선 뒤 SquidPostPopList::MatchesCurrentSource 재반영)
    #current = null;

    // 같은 실제 저장소/행의 기존 getter 묶음인지 확인한다(정체성은 null 이 아니어야 하며 물리 조회/쓰기는 실행하지 않는다). (2026-11-10 WB04-E258)
    matchesCurrentSource(owner, slot) {
        return Boolean(owner) && this.#current !== null && this.#current.sourceOwner === owner && this.#current.sourceSlot === slot;
    }

    // 빈 독립 목록에 같은 실제 저장소의 조회/쓰기를 연결한다. 이미 자료가 있거나 이미 연결됐거나 장치가 빠졌으면 숨은 사본을 남기지 않고 거부한다.
    bindCurrent(binding) {
        if (this.#current !== null || this.entries.length !== 0 || this.count !== 0 || typeof binding?.storage !== 'function' ||
            typeof binding.capacity !== 'function' || typeof binding.readCount !== 'function' || typeof binding.writeCount !== 'function') {
            throw new Error('postPop 현재 목록 연결의 소유/장치 오류');
        }
        this.#current = binding;
    }

    // 현재 활성 개수이다(연결된 경우 현재 Player+8 을 매번 읽는다).
    readCount() { return this.#current ? this.#current.readCount() >>> 0 : this.count; }

    // 현재 용량이다(연결된 경우 현재 Player+4 를 매번 읽는다).
    capacity() { return this.#current ? this.#current.capacity() : this.entries.length; }

    // 현재 배열 전체를 빌린다. 재할당 전 주소를 보관하지 않는다.
    storage() { return this.#current ? this.#current.storage() : this.entries; }

    // 실제 개수만 쓰며 꼬리 값은 지우지 않는다.
    setCount(value) {
        if (this.#current) this.#current.writeCount(value >>> 0);
        else this.count = value >>> 0;
    }

    // 용량이 남을 때 원문처럼 배열 store 뒤 count 를 증가시킨다. 가득 찬 목록에서는 배열을 건드리지 않는다.
    append(value) {
        const used = this.readCount();
        if (used >= this.capacity()) return;
        this.storage()[used] = value >>> 0;
        this.setCount(used + 1);
    }

    // 중복 전체를 안정 압축하고 비활성 꼬리는 보존한다. 압축 중에도 원문처럼 현재 count/배열을 같은 목록에서 읽는다.
    removeAll(value) {
        this.items();
        let removed = 0;
        for (let index = 0; index < this.readCount(); index += 1) {
            const entry = this.storage()[index];
            if (entry === (value >>> 0)) removed += 1;
            else this.storage()[index - removed] = entry;
        }
        this.setCount(this.readCount() - removed);
    }

    // 활성 항목의 읽기 사본이다. 손상된 count(용량 초과)는 거부한다.
    items() {
        const used = this.readCount();
        if (used > this.capacity()) throw new Error('postPop 목록 count 범위 오류');
        return Array.from(this.storage()).slice(0, used);
    }
}

// Totalmade 의 실제 세 256DWORD 표만 보관한다. 목록·현재 소유자·로딩/삭제 정책과 수명을 분리한다.
export class RawTotalMadeStorage {
    // 005c94d0 의 현재 로컬 수(최초 BSS 0), 005c98d0 의 누적 로컬 생산 수(일반 삭제에서 줄지 않음), 005c9cd0 의 현재 전역 수(모든 타입의 256번째 물리 칸 포함)이다.
    localCounts = new Uint32Array(TOTAL_MADE_SIZE);
    localSecondaryCounts = new Uint32Array(TOTAL_MADE_SIZE);
    globalCounts = new Uint32Array(TOTAL_MADE_SIZE);
}

// 객체 후처리 Pop 처리 중 조회·갱신하는 상태 묶음이다.
// 사용: const state = new SquidPostPopState(); state.ownerFactories[3].append(50); state.depth. (2026-11-08 WB04-E2)
export class SquidPostPopState {
    // 표시·등록 후처리 억제 여부, Graph 활성 여부, 보류 배치 여부이다.
    suppressed = false;

    // 실제 Client 의 현재 metamap 관문(00594fc8)을 돌려주는 함수이다. 없으면 null(독립 입력은 suppressed 를 쓴다). (2026-11-10 WB04-E258: cpppj 기준선 뒤 sharedSuppressed 재반영)
    sharedSuppressed = null;

    // 공통 후처리 진입이 읽는 현재 억제 값이다(공유 getter 가 있으면 그 값). (2026-11-10 WB04-E258)
    readSuppressed() { return this.sharedSuppressed ? Boolean(this.sharedSuppressed()) : this.suppressed; }
    graphsEnabled = false;
    pendingPlacement = false;

    // 로컬 소유자와 원본 invalidGraphNum 의 두 판본 초기값(254)이다.
    localOwner = 0;
    invalidGraph = 254;

    // AI 부착 표시(번호 0..8)이다. AI 부착 경로는 아직 복원하지 않았다.
    aiAttached = new Array(PLAYER_COUNT + 1).fill(false);

    // 공급/작업장 재계산 알림과 Activate/postPop 깊이이다.
    productionDirty = 0;
    depth = 0;

    // 독립 장부의 기존 값형 전역 비용(signed)과 세 표, 공급원/공장 목록, 소유자별 공장 목록이다.
    #totalCost = 0;
    #ownTotalMade = new RawTotalMadeStorage();
    providers = new SquidPostPopList();
    factories = new SquidPostPopList();
    ownerFactories = Array.from({ length: PLAYER_COUNT + 1 }, () => new SquidPostPopList());

    // 실제 World 가 빌리는 장기 저장소이다(없으면 null 이고 위의 독립 값을 쓴다).
    #sharedTotalMade = null;
    #sharedTotalCost = null;
    #sharedProviders = null;
    #sharedFactories = null;

    // 실제 World 의 장기 Totalmade 세 표를 빌린다(RawTotalMadeStorage).
    bindTotalMade(storage) {
        if (storage !== null && !(storage instanceof RawTotalMadeStorage)) throw new TypeError('Totalmade 저장소가 필요합니다.');
        this.#sharedTotalMade = storage;
    }

    // 0059508c 의 실제 장기 signed 비용을 빌린다({ value } 객체).
    bindTotalCost(owner) {
        if (owner !== null && typeof owner?.value !== 'number') throw new TypeError('전역 비용 소유자는 { value } 이어야 합니다.');
        this.#sharedTotalCost = owner;
    }

    // 장기 공급원/공장 목록을 빌린다. null 이면 독립 목록으로 돌아간다.
    bindProviders(list) { this.#sharedProviders = list; }

    bindFactories(list) { this.#sharedFactories = list; }

    // 현재 선택된 세 표(같은 현재 물리를 직접 읽고 쓰며 사본 동기화를 하지 않는다)이다.
    localCounts() { return (this.#sharedTotalMade ?? this.#ownTotalMade).localCounts; }

    localSecondaryCounts() { return (this.#sharedTotalMade ?? this.#ownTotalMade).localSecondaryCounts; }

    globalCounts() { return (this.#sharedTotalMade ?? this.#ownTotalMade).globalCounts; }

    // 전역 비용(signed 32비트)이다. 빌린 소유자가 있으면 그 현재 값을 읽고 쓴다. Player 의 현재 SP 차감과 구별한다.
    get totalCost() { return this.#sharedTotalCost ? this.#sharedTotalCost.value | 0 : this.#totalCost; }

    set totalCost(value) {
        if (this.#sharedTotalCost) this.#sharedTotalCost.value = value | 0;
        else this.#totalCost = value | 0;
    }

    // 현재 공급원/공장 목록이다(빌린 장기 목록 또는 독립 목록).
    currentProviders() { return this.#sharedProviders ?? this.providers; }

    currentFactories() { return this.#sharedFactories ?? this.factories; }
}
