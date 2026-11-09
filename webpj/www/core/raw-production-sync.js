// 생산 Deck 과 현재 기술 부모의 CFF_TELEPORT 내용물을 원본 43df80 순서로 동기화하는 모듈이다(cpppj `o/RawProductionSync.cpp/.h`, 10.78 전용).
// 순서: ① 현재 templateOwner 의 Deck(44f760)을 얻고 기술 부모(48fc60)를 조회해 종속(내용물) 체인을 순회한다. 원본은 다음 후보를 먼저 찾은 뒤 이전 후보의 현재 kind(+18 WORD)/flags(+20 WORD)를 읽는다.
//   flags 비트 8 이면 그 kind 의 BYTE 중복 카운터 seen 을 올린다(삭제되는 기존 내용물도 먼저 올리므로 같은 호출 안에서는 다시 만들지 않으며 255 초과는 BYTE 로 감긴다).
//   kind 의 TYPE flags2 비트 4(신전 상품)이면 로컬 owner 의 신전 포인터(Player+7c)가 없을 때 제거하고, 아니고 flags 비트 8 이면 Deck 의 첫 type BYTE 일치 항목의 signed power 가 없거나 1 미만일 때 fullDeck 이 0 인 경우에만 제거한다.
//   제거 대상이 선택 중(flags 비트 0x100)이면 먼저 446e70 취소를 부르고 가상 destroy(0)를 부른다.
// ② 순회가 끝나면 기술 부모를 다시 조회하고(현재 값), Deck 개수를 매번 다시 읽으며 seen 이 0 인 항목마다 기존 종속 검색(mask 8)이 이미 있으면 진단("!me.findContainedType( CFF_TELEPORT, type )") 뒤에도 계속해 생성(parent, kind, 0, 8, 16, 3)한다.
// 신호 전역은 호출 위치에서 한 번 현재 값을 읽는 getter 이고, 선택된 실제 하위가 없으면 그 위치에서 오류다(빈 성공 금지). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E216: 생산 내용물 동기화 이식으로 추가)

import { RawContainedFinder } from './raw-contained-finder.js';
import { PRODUCTION_DECK_BYTES, PRODUCTION_ENTRY_BYTES, PRODUCTION_ENTRY_CAPACITY } from './raw-production-deck.js';

// 현재 Deck 개수(0x14c)와 내용물 flags/kind(+20/+18 WORD) 위치이다. (2026-11-10 WB04-E216)
const COUNT = 0x14c;
const FLAGS = 20;
const KIND = 18;
const TYPE_COUNT = 188;
// BYTE seen 표의 크기(kind < 255)이다. (2026-11-10 WB04-E216)
const SEEN_SIZE = 255;

// 비정렬 원문의 필요한 폭만 읽는다. 인접한 다른 필드를 섞지 않는다. (2026-11-10 WB04-E216)
function read(raw, offset, width = 4) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// 원본의 signed 개수는 음수이면 빈 순회이다. 호스트 물리 66개 한계는 넘지 못한다. (2026-11-10 WB04-E216)
function count(deck) {
    if (deck.length < PRODUCTION_DECK_BYTES) throw new RangeError('생산 동기화 실제 Deck 크기 부족');
    const value = read(deck, COUNT) | 0;
    if (value > PRODUCTION_ENTRY_CAPACITY) throw new RangeError('생산 동기화 Deck 물리 개수 초과');
    return value;
}

// 실제 44f4d0 처럼 첫 type BYTE 일치 항목의 signed power 만 반환한다(없으면 null). 같은 종류가 중복되어도 첫 물리 항목만 쓴다. (2026-11-10 WB04-E216)
function power(deck, kind) {
    for (let index = 0; index < count(deck); index += 1) {
        const offset = index * PRODUCTION_ENTRY_BYTES;
        if (deck[offset] === kind) return (deck[offset + 2] << 24) >> 24;
    }
    return null;
}

// 호출한 실제 하위의 누락만 그 위치에서 거부한다. (2026-11-10 WB04-E216)
function need(callback) {
    if (typeof callback !== 'function') throw new Error('생산 내용물 동기화 실제 하위 미연결');
    return callback;
}

// 생산 내용물 동기화이다. pool 은 10.78 SidPool, types 는 188칸 TYPE 표(flags2), finderState 는 같은 ContainedFinderState, state = { localOwner(), fullDeck() } 현재 전역 getter,
// hooks = { deck() → 현재 Deck 352 BYTE, technology() → 현재 기술 부모, temple(owner) → 신전 포인터 비0 여부, cancelCursor(), destroy(sid, flags), findKind(parent, mask, kind, index) → 저장 kind WORD(SID 아님),
//   create(parent, kind, amount, flags, attach, quality) → SID, assertion(text) }. 참조와 효과 대상은 동기화 호출보다 오래 살아야 한다.
// 사용: new RawProductionSync(pool, types, finderState, state, hooks).synchronize(). (2026-11-10 WB04-E216)
export class RawProductionSync {
    // 현재 기술 내용물의 유일한 저장소, 각 현재 kind 의 원본 flags2, 같은 현재 TYPE6/권한/진단 전역, 현재 owner/fullDeck 안정 참조, 선택된 실제 하위이다.
    #pool;
    #types;
    #finder;
    #state;
    #hooks;

    // 판본/현재 자료를 확인하며 생성 때는 게임 상태를 바꾸지 않는다. (2026-11-10 WB04-E216)
    constructor(pool, types, finderState, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT) throw new TypeError('생산 내용물 동기화 PATCH 자료 오류');
        this.#pool = pool;
        this.#types = types;
        this.#finder = finderState;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 원문 4b2270 은 SID 0 도 실제 예약 슬롯의 head 를 읽는다. DWORD 절단/물리 밖 접근만 거부한다. (2026-11-10 WB04-E216)
    #parent(value) {
        if (value >= this.#pool.capacity || value > 0xffff) throw new RangeError('생산 동기화 현재 기술 부모 범위 오류');
        return value;
    }

    // 실제 43df80 전체이다. 일반 기술·비활성 teleport·신전·선택 취소/생성 순서를 보존한다. (2026-11-10 WB04-E216)
    synchronize() {
        const hooks = this.#hooks;
        const deck = need(hooks.deck)();
        const seen = new Uint8Array(SEEN_SIZE);
        const finder = new RawContainedFinder(this.#pool, this.#finder);
        let current = finder.begin(this.#parent(need(hooks.technology)()));
        // 원본은 다음 후보를 먼저 찾은 뒤 이전 후보의 현재 kind/flags 를 읽는다.
        while (current) {
            const form = current;
            current = finder.next();
            const raw = this.#pool.slot(form);
            const flags = read(raw, FLAGS, 2);
            const kind = read(raw, KIND, 2);
            if (kind >= this.#types.length || kind >= seen.length) throw new RangeError('생산 동기화 내용물 현재 TYPE 범위 오류');
            if (flags & 8) seen[kind] += 1;
            let remove = false;
            if (this.#types[kind].flags2 & 4) remove = !need(hooks.temple)(this.#state.localOwner());
            else if (flags & 8) {
                const found = power(deck, kind);
                if (found === null || found < 1) remove = this.#state.fullDeck() === 0;
            }
            if (remove) {
                if (flags & 0x100) need(hooks.cancelCursor)();
                need(hooks.destroy)(form, 0);
            }
        }
        const parent = this.#parent(need(hooks.technology)());
        // 같은 Deck 포인터의 현재 개수를 매번 읽고 새 생성 뒤 seen 을 올리지 않는다.
        for (let index = 0; index < count(deck); index += 1) {
            const kind = deck[index * PRODUCTION_ENTRY_BYTES];
            if (kind >= seen.length) throw new RangeError('생산 동기화 Deck type BYTE 범위 오류');
            if (!seen[kind]) {
                if (need(hooks.findKind)(parent, 8, kind, 0)) need(hooks.assertion)('!me.findContainedType( CFF_TELEPORT, type )');
                need(hooks.create)(parent, kind, 0, 8, 16, 3);
            }
        }
    }

    // 단일 월드의 실제 저장소임을 검증하는 풀이다. (2026-11-10 WB04-E216)
    get pool() { return this.#pool; }
}
