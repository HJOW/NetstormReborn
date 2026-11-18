// 현재 Player/Factory 목록/TYPE/TotalMade 허용표로 원본 생산 Deck 을 재계산하는 client 연결이다(cpppj `client/RawCampaignProductionBindings.cpp/.h`, 10.78 전용).
// 사용:
//   const production = new RawCampaignProductionBindings(core, player, carrier, create, deck, allowed, contained, state, { configInt(키) → 정수, modeAssertion() });
//   production.refreshCurrent() 는 현재 templateOwner 의 Deck 을, production.refresh(owner) 는 명시 owner 의 Deck 을 원문(0044f7c0) 순서로 재계산하고 같은 토큰을 돌려준다.
//   production.appendFactoryProducts(factory, deckToken) 는 004752b0(mode 0)의 실제 생산 포함과 distinct 종류별 남은 자리 반환이다(허용표 조회도 생략하지 않는다).
//   production.factoryCapacity(factory) · production.contains(type)(0044f970 의 현재 Deck 조회, 재계산 없음) · production.indicateTechnology(type)(004929e0) · production.allowed(type) · production.pool · production.deck.
// core 는 월드 Core 뷰의 { pool, types(188칸), bookkeeping(공통 장부의 현재 `currentFactories()` 목록), ownerMode({ fort, battle }) } 이고, state 는 사용 때마다 현재 값을 읽는 { mission, fullDeck, freeTechnologyType, templeProduct }
//   (PATCH 00594fa0·0054db14·005412bc·0054126c; 속성 getter 로 장기 소유자의 현재 값을 빌릴 수 있다). 호출자가 판정값을 복사해 두지 않는다.
// Player/Deck/Carrier/Create 는 같은 core.pool 을 공유해야 하며 모든 참조는 연결보다 오래 살아야 한다. GUI 표시나 설정 결과를 상수 성공으로 대체하지 않는다.
// 기준선 뒤 cpppj 변경 없음(기준선 `225553df` 와 HEAD 소스 동일, 2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E522: 4-D C 구획 RawCampaignProductionBindings 이식. E215 에서 이월한 재계산/Factory 상품 추가 시험 3건을 함께 대조)
import { RawContainedFinder } from './raw-contained-finder.js';
import { PRODUCTION_ENTRY_BYTES, PRODUCTION_ENTRY_CAPACITY, RawIndicateTechnology } from './raw-production-deck.js';

// Deck 의 개수(+0x14c)·가중합(+0x150) 위치와 내용물의 kind WORD(+18)·mask WORD(+20) 위치이다. (2026-11-15 WB04-E522)
const DECK_COUNT = 0x14c;
const DECK_CHANCE = 0x150;
const CONTENT_KIND = 18;
const CONTENT_FLAGS = 20;
// 생산 Factory 의 두 가상 표 주소(+b4 가 같은 12BYTE 몸체 00453f60)이다. (2026-11-15 WB04-E522)
const FACTORY_VTABLES = Object.freeze([0x509148, 0x509210]);
// 신전 genus 와 Factory flags1 비트이다. (2026-11-15 WB04-E522)
const TEMPLE_GENUS = 0x200;
const FACTORY_FLAG = 0x04000000;
// 표시된 내용물의 mask 비트와 distinct 종류 표 크기이다. (2026-11-15 WB04-E522)
const CONTENT_DISPLAYED = 0x10;
const SEEN_KINDS = 255;

// 비정렬 little endian 필드의 필요한 폭만 읽는다. (2026-11-15 WB04-E522)
function read(raw, offset, width = 4) {
    let value = 0;
    // BYTE/WORD/DWORD 뒤의 다른 원본 필드를 섞지 않는다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// 실제 Deck 의 DWORD 만 쓴다. 다른 항목과 물리 꼬리는 보존한다. (2026-11-15 WB04-E522)
function put(raw, offset, value) {
    // 정확한 네 바이트를 낮은 쪽부터 기록한다.
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// signed 음수 개수는 빈 순회이며 양수의 물리 66개 한계만 검사한다. (2026-11-15 WB04-E522)
function count(raw) {
    const value = read(raw, DECK_COUNT) | 0;
    if (value > PRODUCTION_ENTRY_CAPACITY) throw new RangeError('생산재계산Deck물리범위오류');
    return value;
}

// 현재 Player/Factory 목록/허용표로 생산 Deck 을 재계산한다. 실제 계산 몸체를 공유하며 표시/설정 결과를 상수 성공으로 대체하지 않는다. (2026-11-15 WB04-E522)
export class RawCampaignProductionBindings {
    // 단일 Pool/TYPE/공통 Factory 목록을 빌리는 Core 뷰, 현재 두 owner 와 9 Player 원문, Deck 포인터와 실제 블록이다.
    #core;
    #player;
    #deck;
    // 튜토리얼/메뉴와 공유하는 금지 DWORD 표의 실제 몸체, 같은 현재 containedType/권한/진단 전역, 현재 생산 모드·타입 전역과 설정/진단의 실제 소비자이다.
    #allowed;
    #contained;
    #state;
    #effects;
    // 동일 기술 부모/Factory/ContentHost 의 전체 하위 조합이다.
    #indicate;

    // 같은 실제 월드/Player/Deck 연결을 최초 사용 전에 확인한다. (2026-11-15 WB04-E522)
    constructor(core, player, carrier, create, deck, allowed, contained, state, effects) {
        if (core?.pool !== player?.pool || core.pool !== deck?.pool || player !== deck.player || core.pool.edition !== 'patch1078' || core.types?.length !== 188) {
            throw new TypeError('생산재계산의같은실제월드/Player/Deck연결누락');
        }
        this.#core = core;
        this.#player = player;
        this.#deck = deck;
        this.#allowed = allowed;
        this.#contained = contained;
        this.#state = state;
        this.#effects = effects ?? {};
        this.#indicate = new RawIndicateTechnology(player, carrier, create);
    }

    // 0044f970 자체는 dirty 나 현재 건물 수를 검사하지 않는다. (2026-11-15 WB04-E522)
    contains(type) { return this.#deck.contains(type); }

    // 원본 004929e0 의 실제 kind WORD/생성 체인을 사용한다. 표시 Gump 갱신은 별도 실제 명령 소비자의 책임이다. (2026-11-15 WB04-E522)
    indicateTechnology(type) { this.#indicate.indicate(type); }

    // 조회 시 현재 templateOwner 를 다시 읽는다. (2026-11-15 WB04-E522)
    refreshCurrent() { return this.refresh(this.#player.state.templateOwner); }

    // 현재 Factory DWORD 가 손상됐을 때 호스트 주소를 만들기 전에 거부한다. (2026-11-15 WB04-E522)
    #factory(value) {
        if (!(value > 0) || value >= this.#core.pool.capacity || value > 0xffff) throw new RangeError('생산Factory실제SID범위오류');
        return value;
    }

    // 실제 두 Factory 가상 표의 +b4 는 같은 12BYTE 몸체 00453f60 이고 +80 은 BYTE 상위 3비트이다. 미복원 가상 표는 거부한다. (2026-11-15 WB04-E522)
    factoryCapacity(factory) {
        const raw = this.#core.pool.slot(factory);
        const table = read(raw, 0);
        if (!FACTORY_VTABLES.includes(table)) throw new Error('생산Factory가상+b4의현재실제몸체미복원');
        return (raw[11] >> 5) + 2;
    }

    // mode 0 의 빈 Mana 표식 문자열은 004735c0 에서도 빈 문자열 그대로이며 표시 Gump 효과는 발생하지 않는다. deckToken 이 0 이면 Deck 추가 없이 남은 자리만 센다. (2026-11-15 WB04-E522)
    appendFactoryProducts(factory, deckToken) {
        const core = this.#core;
        let remaining = this.factoryCapacity(factory);
        const seen = new Uint8Array(SEEN_KINDS);
        const finder = new RawContainedFinder(core.pool, this.#contained);
        // 기존 커서가 후보 반환 전에 저장한 next 를 보존하며 매 후보의 현재 WORD 조건을 읽는다.
        for (let content = finder.begin(factory); content; content = finder.next()) {
            const raw = core.pool.slot(content);
            if (!(raw[CONTENT_FLAGS] & CONTENT_DISPLAYED)) continue;
            const kind = read(raw, CONTENT_KIND, 2);
            if (kind >= core.types.length || kind >= seen.length) throw new RangeError('생산Factory의현재TYPE/중복표범위오류');
            // 원본 mode 0 도 허용표는 조회하지만 Deck 추가를 금지표로 거르지는 않는다.
            this.#allowed.allowed(kind);
            if (deckToken) this.#deck.activate(deckToken, kind, 1);
            if (!seen[kind]) {
                remaining -= 1;
                seen[kind] = 1;
            }
        }
        return remaining;
    }

    // 0044f7c0 은 별도 Factory 목록의 현재 owner/type/내용물과 같은 Deck 을 재계산한다. fullDeck 모드는 진단 뒤 현재 값을 다시 읽어 자연 반환을 고른다. (2026-11-15 WB04-E522)
    refresh(owner) {
        const core = this.#core;
        const state = this.#state;
        const token = this.#deck.get(owner);
        if (!core.ownerMode.fort && !core.ownerMode.battle) {
            if (typeof this.#effects.modeAssertion !== 'function') throw new Error('생산재계산실제Fort/Battle진단미연결');
            this.#effects.modeAssertion();
        }
        if (!state.mission && state.fullDeck) return token;
        let raw = this.#deck.bytes(token);
        // 기존 항목의 power 만 -1 로 바꾸며 기존 chance/remaining/order 를 유지한다.
        for (let index = 0; index < count(raw); index += 1) raw[index * PRODUCTION_ENTRY_BYTES + 2] = 255;
        put(raw, DECK_CHANCE, 0);
        if (typeof this.#effects.configInt !== 'function') throw new Error('생산재계산getTeleportTechFree실제설정미연결');
        if (this.#effects.configInt('getTeleportTechFree')) this.#deck.activate(token, state.freeTechnologyType, 1);
        const factories = core.bookkeeping.currentFactories();
        // 같은 공통 PostPop/삭제가 변경하는 현재 Factory 개수와 DWORD 목록을 매 반복 읽는다.
        for (let index = 0; index < factories.readCount(); index += 1) {
            const current = this.#factory(factories.storage()[index]);
            const object = core.pool.slot(current);
            if (object[34] !== (owner >>> 0)) continue;
            const type = object[10];
            if (type >= core.types.length) throw new RangeError('생산Factory현재TYPE범위오류');
            if ((core.types[type].flags2 >>> 0) & TEMPLE_GENUS) {
                if (this.#allowed.allowed(state.templeProduct)) this.#deck.activate(token, state.templeProduct, 1);
            } else if ((core.types[type].flags1 >>> 0) & FACTORY_FLAG) {
                this.appendFactoryProducts(current, token);
            }
        }
        this.#deck.compact(token);
        raw = this.#deck.bytes(token);
        let total = 0;
        // signed power 가 양수인 항목의 BYTE chance 만 DWORD 감김 합산한다.
        for (let index = 0; index < count(raw); index += 1) {
            const offset = index * PRODUCTION_ENTRY_BYTES;
            if (((raw[offset + 2] << 24) >> 24) > 0) total = (total + raw[offset + 1]) >>> 0;
        }
        put(raw, DECK_CHANCE, total);
        return token;
    }

    // 같은 현재 허용표를 표시 소비자에게 공급한다. 금지된 일반 Factory 생산을 Deck 에서 임의 제외하지 않는다. (2026-11-15 WB04-E522)
    allowed(type) { return this.#allowed.allowed(type | 0); }

    // 단일 월드 조립을 확인하는 실제 풀이다. (2026-11-15 WB04-E522)
    get pool() { return this.#core.pool; }

    // 실제 Player+68 과 같은 수명 저장소이다. (2026-11-15 WB04-E522)
    get deck() { return this.#deck; }
}
