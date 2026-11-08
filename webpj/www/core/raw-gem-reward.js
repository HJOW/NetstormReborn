// 오벨리스크(묻힌 보상)의 현재 QA 보상 또는 실제 가중 Deck 추첨(461f00 선택·4620c0 공개)을 생성·배치·삭제에 연결하는 모듈이다(cpppj `o/RawGemReward.cpp/.h`, 10.78 전용).
// 게임 계산은 이 모듈이 소유하고 현재 시각·진단·실제 큰 월드 호출(생성/배치/삭제)만 훅으로 받는다.
//  - choose(unused = 5): 전달된 5 를 읽지 않고 현재 TYPE 표 전체로 스택 Deck 을 만든다(초기화 전 물리 미정 바이트는 훅이 공급). 현재 TYPE 개수(signed, 반복 끝마다 다시 읽음)만큼 돌며 flags2 에 0x100(보상 후보)이 있고
//    제외 목록(BYTE 항목, 전체 TYPE 인자와 비교)에 없는 TYPE 의 현재 레벨로 가중치 0→20, 1→30, 2→50, 3→100 을 정해 Deck 에 추가한다(레벨 98/99 는 제외, 그 밖은 진단 0xf6 뒤 건너뜀).
//    Deck 개수가 0 이 아니면 열기(seed 는 같은 게임 난수 0..9999)→추첨→닫기로 TYPE 을 골라 0 이 아니면 반환하고, 빈 Deck 이거나 실패면 진단(0x104) 뒤 0 을 반환한다.
//  - reveal(buried): 오벨리스크 TYPE 이 현재 오벨리스크 타입이 아니거나 삭제 비트(bit0)면 진단(0x10a)을 부르고 계속한다. 슬롯의 보상 TYPE(WORD +12)이 0 이거나 보상 후보가 아니면 choose 로 다시 고른다.
//    생성(create(type, 0)) 뒤 원 오벨리스크의 **현재** 좌표(+14/+18)로 배치(pop)하고 원 오벨리스크를 삭제(destroy(buried, 0))한 뒤 생성 결과를 반환한다. 범위 밖 TYPE 은 읽기 전에 거부한다.
// state = GemRewardState({ typeCount(signed, 기본 188), buriedType(기본 112) }), excluded 는 SquidPostPopList, random 은 게임 난수(GameRandom), hooks = { fill(raw), deck(DeckDrawHooks), assertion(line), deckAssertion(kind),
// create(type, flags)→sid, pop(sid, x, y, flags), destroy(sid, flags) }. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E105: 오벨리스크 보상 이식으로 추가)
import { PRODUCTION_DECK_BYTES, RawDeckDraw } from './raw-deck-draw.js';

// 보상 후보 flags2 비트, 레벨별 가중치, 진단 번호, 오벨리스크 슬롯 위치, 판본의 TYPE 수이다. (2026-11-08 WB04-E105)
const CANDIDATE = 0x100;
const CHANCE = Object.freeze({ 0: 20, 1: 30, 2: 50, 3: 100 });
const ASSERT_LEVEL = 0xf6;
const ASSERT_EMPTY = 0x104;
const ASSERT_BURIED = 0x10a;
const TYPE_AT = 10;
const STATE_AT = 11;
const REWARD_AT = 12;
const X_AT = 14;
const Y_AT = 18;
const TYPE_COUNT = 188;
const COUNT_AT = 0x14c;

// 선택된 외부 효과가 없으면 아직 미복원된 호출로 보고한다. (2026-11-08 WB04-E105)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('오벨리스크 보상의 실제 경계 미연결');
    return hook;
}

// 효과 뒤 다시 읽는 원본 타입 개수와 오벨리스크 타입이다. (2026-11-08 WB04-E105)
export class GemRewardState {
    // 541348 의 현재 signed TYPE 개수이다.
    typeCount = 188;
    // 541218 의 현재 오벨리스크 타입이다.
    buriedType = 112;
}

// 오벨리스크 보상 선택/공개 처리기이다. (2026-11-08 WB04-E105)
export class RawGemReward {
    // 오벨리스크/생성 객체의 유일한 슬롯 저장소이다. (2026-11-08 WB04-E105)
    #pool;
    // 효과 뒤 현재 레벨/genus 를 다시 읽는 TYPE 표이다. (2026-11-08 WB04-E105)
    #types;
    // 원본 00532710 의 동일 게임 난수이다. (2026-11-08 WB04-E105)
    #random;
    // 건설 소비자와 같은 현재 TYPE 제외 목록(SquidPostPopList)이다. (2026-11-08 WB04-E105)
    #excluded;
    // 다음 반복 및 공개 변환의 현재 전역이다. (2026-11-08 WB04-E105)
    #state;
    // 선택된 실제 하위가 없으면 명시 거부하는 훅이다. (2026-11-08 WB04-E105)
    #hooks;

    // 다른 판본의 보상 TYPE 나 구조체를 PATCH 와 같은 것으로 추정하지 않는다. (2026-11-08 WB04-E105)
    constructor(pool, types, random, excluded, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !types || types.length !== TYPE_COUNT) throw new Error('오벨리스크 보상의 PATCH/188TYPE 연결 오류');
        this.#pool = pool;
        this.#types = types;
        this.#random = random;
        this.#excluded = excluded;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 같은 실제 월드 조립 검사용 풀이다. (2026-11-08 WB04-E105)
    get pool() { return this.#pool; }

    // QA 밖 TYPE 주소의 원본 무효 역참조는 호스트 범위 계약에서 거부한다. (2026-11-08 WB04-E105)
    #type(type) {
        if (type >= this.#types.length) throw new RangeError('오벨리스크 현재TYPE 범위');
        return this.#types[type];
    }

    // 원본 41c7a0 의 저장 항목은 BYTE 이며 인자 전체 DWORD 와 비교한다. 같은 현재 목록을 호출 때마다 읽는다. (2026-11-08 WB04-E105)
    #isExcluded(type) {
        const list = this.#excluded;
        const used = list.readCount();
        const entries = list.storage();
        // 최초 일치에서 반환한다.
        for (let index = 0; index < used; index += 1) if ((entries[index] & 0xff) === type) return true;
        return false;
    }

    // 실시간 난수는 Deck Draw 안에서만 만들며 전역 난수를 추첨값으로 대신 쓰지 않는다. (2026-11-08 WB04-E105)
    choose(unused = 5) {
        void unused;
        const hooks = this.#hooks;
        const raw = new Uint8Array(PRODUCTION_DECK_BYTES);
        need(hooks.fill)(raw);
        RawDeckDraw.initialize(raw);
        // 현재 타입 개수는 각 반복 끝에 다시 읽는다. 수준 진단이 돌아오면 다음 현재 TYPE 를 계속 처리한다.
        for (let type = 0; type < (this.#state.typeCount | 0); type += 1) {
            if (!(this.#type(type).flags2 & CANDIDATE) || this.#isExcluded(type)) continue;
            const level = this.#type(type).level | 0;
            const chance = CHANCE[level];
            if (level === 98 || level === 99) continue;
            if (chance === undefined) {
                need(hooks.assertion)(ASSERT_LEVEL);
                continue;
            }
            RawDeckDraw.append(raw, type, chance, hooks.deckAssertion);
        }
        let type = 0;
        const used = (raw[COUNT_AT] | (raw[COUNT_AT + 1] << 8) | (raw[COUNT_AT + 2] << 16) | (raw[COUNT_AT + 3] << 24)) >>> 0;
        if (used !== 0) {
            const deck = new RawDeckDraw(raw, this.#random, hooks.deck);
            deck.open(this.#random.next(10000));
            type = deck.draw();
            deck.close();
            if (type) return type;
        }
        need(hooks.assertion)(ASSERT_EMPTY);
        return type;
    }

    // 오벨리스크 QA 의 유효 genus 는 제외 목록/현재 레벨과 무관하게 그대로 사용한다. (2026-11-08 WB04-E105)
    reveal(buried) {
        const hooks = this.#hooks;
        let raw = this.#pool.slot(buried);
        if (raw[TYPE_AT] !== this.#state.buriedType || (raw[STATE_AT] & 1)) need(hooks.assertion)(ASSERT_BURIED);
        const current = this.#pool.slot(buried);
        let type = current[REWARD_AT] | (current[REWARD_AT + 1] << 8);
        if (!type || !(this.#type(type).flags2 & CANDIDATE)) type = this.choose(5);
        const created = need(hooks.create)(type, 0);
        raw = this.#pool.slot(buried);
        const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        need(hooks.pop)(created, view.getFloat32(X_AT, true), view.getFloat32(Y_AT, true), 0);
        need(hooks.destroy)(buried, 0);
        return created;
    }
}
