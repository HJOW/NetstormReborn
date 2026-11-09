// 현재 Player 가 소유하는 원본 0x160 BYTE 생산 Deck 과 기술 표시 내용물을 같은 월드에서 조회하는 모듈이다(cpppj `o/RawProductionDeck.cpp/.h`, 10.78 전용).
// 구성: ① RawProductionDeckStorage — 실제 Deck 블록(352 BYTE, 66개의 5 BYTE 항목 + 개수/가중합/남은 수/seed/open 다섯 DWORD)과 opaque 주소(토큰)만 장기 소유한다. 0044f700 의 "현재 Player+68 조회 → 없으면 할당 → 다섯 DWORD 초기화 → 부착".
//   ② RawProductionDeck — 현재 Player(9기록)의 +68 포인터와 이 저장소의 블록을 이어 주는 수명·조회·추가(0044f290)·signed power 활성화(0044f3d0)·압축(0044f430)·Contains(0044f970, 현재 templateOwner)·초기화(0044f200/0044f230)와
//   Player 링크 정리 부모(0048fcd0)의 정상 복귀 증빙(PlayerLinksPass: 같은 모듈이 발급하고 한 번만 소비). 증빙을 소비하면 그 Deck 은 닫히며 새 Session 모듈로 교체해야 한다.
//   ③ RawIndicateTechnology — 004929e0 의 현재 기술 부모·저장 kind WORD 조회·contained 생성 사슬(type, 0, 4, 0x10, 0). kind WORD 가 0 이면 일치하는 종속이 있어도 생성으로 진행한다.
// signed 개수는 그대로 두고(음수는 빈 순회), 호스트 블록을 넘는 양수 개수(66 초과)와 모르는 토큰·Player 9 이상은 쓰기 전에 거부한다. 원본 진단은 선택된 경계가 없으면 성공으로 넘기지 않는다.
// C++ 소멸자의 반납은 `dispose()` 로 옮겼다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E215: 생산 Deck 이식으로 추가)

import { DeckAssertion, PRODUCTION_DECK_BYTES, RawDeckDraw } from './raw-deck-draw.js';
import { cleanupPlayerLinks } from './raw-player-links-cleanup.js';

// 원본 Deck 객체의 전체 물리 크기와 고정된 66개 5 BYTE 항목, 각 필드 위치이다. (2026-11-10 WB04-E215)
export const PRODUCTION_ENTRY_BYTES = 5;
export const PRODUCTION_ENTRY_CAPACITY = 66;
export { PRODUCTION_DECK_BYTES };
const COUNT = 0x14c;
const CHANCE = 0x150;
const REMAINING = 0x154;
const PLAYER_COUNT = 9;
const PLAYER_STRIDE = 180;
const POINTER = 0x68;

// 기존 목록 추가의 원본 진단 위치 이름이다(RawDeckDraw.append 의 DeckAssertion 과 같은 번호). (2026-11-10 WB04-E215)
export const PRODUCTION_DECK_ASSERTION = DeckAssertion;

// 비정렬 little endian DWORD/WORD 를 필요한 폭만 읽는다. 인접 필드나 호스트 정렬을 섞지 않는다. (2026-11-10 WB04-E215)
function read(raw, offset, width = 4) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// 원본 DWORD 필드만 쓰고 항목/미초기화 꼬리는 보존한다. (2026-11-10 WB04-E215)
function put(raw, offset, value) {
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 실제 Deck 블록과 opaque 주소만 장기 소유한다. Player/World 실행기나 반납 증빙은 보관하지 않는다.
// 사용: const storage = new RawProductionDeckStorage(0x30000000); storage.get(playerRecords, owner, fill). (2026-11-10 WB04-E215)
export class RawProductionDeckStorage {
    // 모든 World/HUD 가 공유하는 유일한 실제 할당(토큰 → 352 BYTE)과 다음 주소이다. 원문 크기 352 만큼 진행하며 새 Session 에서 재설정하지 않는다.
    #blocks = new Map();
    #next;

    // 최초 주소만 검증하며 get 호출 전에는 할당하지 않는다. (2026-11-10 WB04-E215)
    constructor(firstToken = 0x30000000) {
        if (!firstToken || firstToken > 0xffffffff - PRODUCTION_DECK_BYTES) throw new RangeError('실제 Deck의 토큰 연결 누락');
        this.#next = firstToken >>> 0;
    }

    // 0044f700: 현재 Player+68 을 먼저 읽고 없는 경우만 실제 352 BYTE 를 할당한다. 미정 꼬리에는 fill 입력을 보존하고 다섯 DWORD 만 초기화한 뒤 Player+68 에 부착한다.
    get(players, owner, fill) {
        if (owner >= PLAYER_COUNT || players.length !== PLAYER_COUNT * PLAYER_STRIDE) throw new RangeError('실제 Deck의 Player 범위 오류');
        const offset = owner * PLAYER_STRIDE + POINTER;
        const current = read(players, offset);
        if (current) {
            this.bytes(current);
            return current;
        }
        if (typeof fill !== 'function') throw new Error('실제 Deck 할당 바이트 입력 누락');
        if (this.#next > 0xffffffff - PRODUCTION_DECK_BYTES) throw new RangeError('실제 Deck 토큰 주소 고갈');
        const token = this.#next;
        const bytes = new Uint8Array(PRODUCTION_DECK_BYTES);
        fill(bytes);
        this.#blocks.set(token, bytes);
        this.#next += PRODUCTION_DECK_BYTES;
        RawDeckDraw.initialize(this.bytes(token));
        put(players, offset, token);
        return token;
    }

    // 불투명 DWORD 를 호스트 주소로 해석하지 않고 현재 생존 블록만 조회한다(살아 있는 352 BYTE 를 빌린다). (2026-11-10 WB04-E215)
    bytes(token) {
        const block = this.#blocks.get(token >>> 0);
        if (!block) throw new RangeError('현재 Player Deck의 실제 할당 블록 누락');
        return block;
    }

    // Player 포인터의 후속 0 쓰기는 원문 정리 부모에 맡기며 이 장치는 블록만 반납한다. 0 은 free(NULL) 처럼 무효과이고 미등록 비영/중복 토큰은 오류이다. (2026-11-10 WB04-E215)
    release(token) {
        if (!token) return;
        if (!this.#blocks.delete(token >>> 0)) throw new RangeError('현재 Player Deck의 실제 반납 블록 누락');
    }

    // 토큰이 이 저장소의 생존 블록인지이다(소멸 정리와 증빙 소비가 쓴다). (2026-11-10 WB04-E215)
    has(token) { return this.#blocks.has(token >>> 0); }

    // 생존 블록이 하나도 없는지이다. (2026-11-10 WB04-E215)
    isEmpty() { return this.#blocks.size === 0; }

    // 모든 블록을 반납한다(다음 토큰은 되감지 않는다). (2026-11-10 WB04-E215)
    clear() { this.#blocks.clear(); }
}

// 증빙 발급을 RawProductionDeck 안으로 제한하는 모듈 내부 열쇠이다. (2026-11-10 WB04-E215)
const PASS_KEY = Symbol('PlayerLinksPass');

// 실제 0048fcd0 전체 정상 복귀를 증명하는 이동 전용 증빙이다. 외부에서 만들 수 없고(열쇠 필요) 한 번만 소비된다. (2026-11-10 WB04-E215)
export class PlayerLinksPass {
    // 발급한 Deck, 그 Deck 의 수명 표식(약한 참조), 소비 여부이다.
    source;
    lifetime;
    consumed = false;

    // 모듈 내부 열쇠가 아니면 임의 완료 상태를 만들지 못하게 거부한다. (2026-11-10 WB04-E215)
    constructor(key, source, lifetime) {
        if (key !== PASS_KEY) throw new TypeError('Player 링크 정상 복귀 증빙은 실제 정리 뒤에만 발급된다');
        this.source = source;
        this.lifetime = new WeakRef(lifetime);
    }
}

// Player+68 의 32비트 토큰에 대응하는 유일한 Deck 수명 저장소이다. 다른 Player 사본은 만들지 않는다.
// player 는 RawPlayerWorld(PATCH 9기록), fill(bytes) 는 할당 직후 미초기화 바이트 공급(0 채움을 묵시적으로 넣지 않음), assertion(where) 은 원본 진단 경계(없으면 도달 시 오류),
// firstToken 은 새 저장소의 첫 주소, storage 를 주면 Client 의 장기 물리를 빌린다. (2026-11-10 WB04-E215)
export class RawProductionDeck {
    // 같은 월드의 Player, 할당 바이트 공급, 원본 진단, 장기 저장소이다.
    #player;
    #fill;
    #assertion;
    #storage;
    // 실제 모듈이 사라진 뒤 이전 증빙의 재사용을 거부하기 위한 수명 표식과, 소비한 증빙·진행 중 링크 호출 수이다.
    #lifetime = {};
    #returnedLinks = null;
    #activeLinkCalls = 0;

    // 판본(PATCH)·할당 바이트 공급·첫 주소를 확인한다. (2026-11-10 WB04-E215)
    constructor(player, fill, assertion = null, firstToken = 0x30000000, storage = null) {
        if (player?.pool?.edition !== 'patch1078' || typeof fill !== 'function' || !firstToken || firstToken > 0xffffffff - PRODUCTION_DECK_BYTES) {
            throw new TypeError('실제 Deck의 PATCH/할당 바이트/토큰 연결 누락');
        }
        this.#player = player;
        this.#fill = fill;
        this.#assertion = assertion;
        this.#storage = storage ?? new RawProductionDeckStorage(firstToken);
    }

    // C++ 소멸자: 증빙을 소비하지 않았다면 이 저장소가 소유한 현재 Player+68 포인터만 지우고 블록을 모두 반납한다. 다른 모듈이 교체한 포인터는 지우지 않는다. (2026-11-10 WB04-E215)
    dispose() {
        if (this.#returnedLinks) return;
        // 동일 9기록의 현재 포인터와 자신이 소유한 토큰을 확인한다.
        for (let owner = 0; owner < PLAYER_COUNT; owner += 1) {
            const records = this.#player.records;
            const offset = this.#pointerOffset(owner);
            if (this.#storage.has(read(records, offset))) put(records, offset, 0);
        }
        this.#storage.clear();
    }

    // 닫힌 소비자는 원문 새 게임의 Deck owner 로 재활용하지 않고 새 Session 모듈로 교체해야 한다. (2026-11-10 WB04-E215)
    #requireLive() {
        if (this.#returnedLinks) throw new Error('Native 반납된 Player 링크의 Deck 소비 금지');
    }

    // 원본은 Player 0 도 사용할 수 있지만 물리 9기록 밖 주소는 호스트 계약에서 거부한다. (2026-11-10 WB04-E215)
    #pointerOffset(owner) {
        this.#requireLive();
        if (owner >= PLAYER_COUNT || this.#player.records.length !== PLAYER_COUNT * PLAYER_STRIDE) throw new RangeError('실제 Deck의 Player 범위 오류');
        return owner * PLAYER_STRIDE + POINTER;
    }

    // 정확한 원본 진단을 전달하며 선택된 경계가 없으면 성공으로 넘기지 않는다. (2026-11-10 WB04-E215)
    #assert(where) {
        if (typeof this.#assertion !== 'function') throw new Error('실제 Deck 원본 진단 경계 누락');
        this.#assertion(where);
    }

    // signed 개수를 유지하고 호스트 블록을 넘는 손상된 양수 개수만 거부한다(음수는 원본처럼 빈 순회). (2026-11-10 WB04-E215)
    #count(raw) {
        const count = read(raw, COUNT) | 0;
        if (count > PRODUCTION_ENTRY_CAPACITY) throw new RangeError('실제 Deck의 66항목 물리 범위 오류');
        return count;
    }

    // 0044f700: 없으면 실제 저장소를 할당하고 다섯 DWORD 만 초기화한 뒤 현재 Player+68 에 부착한다. fill 입력은 보존한다. (2026-11-10 WB04-E215)
    get(owner) {
        this.#pointerOffset(owner);
        return this.#storage.get(this.#player.records, owner, this.#fill);
    }

    // 현재 토큰의 살아 있는 352 BYTE 를 빌린다. 반납하면 그 뷰는 즉시 무효이다. (2026-11-10 WB04-E215)
    bytes(token) {
        this.#requireLive();
        return this.#storage.bytes(token);
    }

    // 0048fcd0 의 선택된 free 인자를 동일한 할당 소유자에 연결한다. Player+68 의 후속 0 쓰기는 부모가 수행한다. 0 은 무효과이다. (2026-11-10 WB04-E215)
    release(token) {
        this.#requireLive();
        this.#storage.release(token);
    }

    // 0044f200: 생성자는 count/seed/chance/remaining/open 의 원본 다섯 DWORD 만 0 으로 쓴다. (2026-11-10 WB04-E215)
    initialize(token) { RawDeckDraw.initialize(this.bytes(token)); }

    // 0044f230: 새 생산 재계산은 목록 개수/가중합/남은 수만 초기화하고 seed/open 은 유지한다. (2026-11-10 WB04-E215)
    clearEntries(token) {
        const raw = this.bytes(token);
        put(raw, COUNT, 0);
        put(raw, CHANCE, 0);
        put(raw, REMAINING, 0);
    }

    // 0044f970: 현재 templateOwner 의 Deck 을 읽는다. 항목의 type BYTE 와 전체 인자 DWORD 를 비교하며 첫 일치 뒤에는 나머지 항목을 읽지 않는다. (2026-11-10 WB04-E215)
    contains(type) {
        const token = this.get(this.#player.state.templateOwner);
        const raw = this.bytes(token);
        for (let index = 0; index < this.#count(raw); index += 1) if (raw[index * PRODUCTION_ENTRY_BYTES] === (type >>> 0)) return true;
        return false;
    }

    // 0044f290: 중복 진단을 먼저 수행한 뒤 원본 5 BYTE 초기화와 DWORD 합계를 기록한다. (2026-11-10 WB04-E215)
    append(token, type, chance) {
        RawDeckDraw.append(this.bytes(token), type >>> 0, chance | 0, (where) => this.#assert(where));
    }

    // 0044f3d0: 기존 항목의 signed power 가 음수이면 0 으로 바꾸고 increment 가 비영일 때만 1 을 더한다. 최초 BYTE 타입 일치만 변경하며 없으면 원본 chance 1/power 1 항목을 추가한다. (2026-11-10 WB04-E215)
    activate(token, type, increment) {
        const raw = this.bytes(token);
        for (let index = 0; index < this.#count(raw); index += 1) {
            const at = index * PRODUCTION_ENTRY_BYTES;
            if (raw[at] !== (type >>> 0)) continue;
            if (((raw[at + 2] << 24) >> 24) < 0) raw[at + 2] = 0;
            if (increment) raw[at + 2] = (raw[at + 2] + 1) & 0xff;
            return;
        }
        this.append(token, type, 1);
    }

    // 0044f430: 원본은 제외할 항목도 먼저 앞으로 복사한 뒤 signed power 가 음수이면 삭제 개수를 늘린다. 앞방향 겹침 복사와 남은 물리 꼬리 보존을 유지한다. (2026-11-10 WB04-E215)
    compact(token) {
        const raw = this.bytes(token);
        const count = this.#count(raw);
        let removed = 0;
        for (let index = 0; index < count; index += 1) {
            const from = index * PRODUCTION_ENTRY_BYTES;
            const to = (index - removed) * PRODUCTION_ENTRY_BYTES;
            const word = read(raw, from);
            const last = raw[from + 4];
            put(raw, to, word);
            raw[to + 4] = last;
            if (((raw[from + 2] << 24) >> 24) < 0) removed += 1;
        }
        put(raw, COUNT, (count - removed) >>> 0);
    }

    // 실제 부모(0048fcd0)와 이 Deck 의 실제 release 만 사용한 정상 복귀 증빙을 발급한다. 같은 현재 Player getter 와 자기 Deck 메모리만 기존 원문 부모에 공급한다. (2026-11-10 WB04-E215)
    cleanupPlayerLinksForState(currentPlayers) {
        this.#requireLive();
        this.#activeLinkCalls += 1;
        try {
            cleanupPlayerLinks({ players: currentPlayers, release: (token) => this.release(token) });
        } finally {
            this.#activeLinkCalls -= 1;
        }
        return new PlayerLinksPass(PASS_KEY, this, this.#lifetime);
    }

    // 실제 부모가 끝나고 같은 현재 물리가 0 인 경우만 old Player 뷰의 재조회 책임을 반납한다. 같은 모듈이 발급한 소비 전 증빙만 받는다. (2026-11-10 WB04-E215)
    returnPlayerLinksForState(pass) {
        if (!(pass instanceof PlayerLinksPass) || pass.source !== this || pass.lifetime.deref() !== this.#lifetime || pass.consumed) {
            throw new TypeError('같은 실제 Deck의 Player 링크 정상 복귀 증빙 필요');
        }
        this.#requireLive();
        if (this.#activeLinkCalls) throw new Error('진행 중 Player 링크 효과의 증빙 소비 금지');
        if (!this.#storage.isEmpty()) throw new Error('현재 실제 Deck 소유 블록 반납 미완료');
        const records = this.#player.records;
        if (records.length !== PLAYER_COUNT * PLAYER_STRIDE) throw new Error('현재 Player 링크 반납의 실제 9기록 누락');
        // 실제 부모가 정리한 세 DWORD 만 확인하며 다른 상태는 바꾸지 않는다.
        for (let owner = 0; owner < PLAYER_COUNT; owner += 1) {
            if (read(records, owner * PLAYER_STRIDE + 0x9c) || read(records, owner * PLAYER_STRIDE + 8) || read(records, owner * PLAYER_STRIDE + POINTER)) {
                throw new Error('현재 Player 링크 세 DWORD 반납 미완료');
            }
        }
        pass.consumed = true;
        this.#returnedLinks = pass;
    }

    // 보관한 실제 증빙의 존재만 조회하며 old Player 뷰는 읽지 않는다. (2026-11-10 WB04-E215)
    playerLinksReturnedForState() { return this.#returnedLinks !== null; }

    // 월드 조립에서 풀 동일성을 확인한다. (2026-11-10 WB04-E215)
    get pool() { return this.#player.pool; }

    // 후속 재계산이 현재 원본 두 owner/Player9 를 다시 읽도록 같은 Player 를 돌려준다. (2026-11-10 WB04-E215)
    get player() { return this.#player; }
}

// 004929e0 의 현재 기술 부모/반환 kind WORD/실제 contained 생성 사슬이다. 타입 assert 를 임의로 추가하지 않는다.
// player 는 RawPlayerWorld, carrier 는 RawCarrierPreDestroy(내용물 검색), create 는 RawContainedCreate 이며 셋은 같은 풀이어야 한다. (2026-11-10 WB04-E215)
export class RawIndicateTechnology {
    // 현재 PlayerKnown/battleOwner 의 실제 Technology DWORD 를 읽는 Player, 조건 순회 Carrier, 생성 사슬이다.
    #player;
    #carrier;
    #create;

    // 기술 표시의 실제 조회/생성은 반드시 같은 풀이어야 한다. (2026-11-10 WB04-E215)
    constructor(player, carrier, create) {
        if (player?.pool !== carrier?.pool || player?.pool !== create?.pool) throw new TypeError('기술 표시의 실제 Player/Carrier/생성 풀이 서로 다릅니다');
        this.#player = player;
        this.#carrier = carrier;
        this.#create = create;
    }

    // CurrentTechnology 의 DWORD 를 WORD 로 미리 자르지 않고 실제 풀 범위를 확인한다. 원본 PlayerKnown 0 반환과 호스트 풀 범위 계약을 구분한다. (2026-11-10 WB04-E215)
    #parent() {
        const sid = this.#player.currentTechnology();
        if (sid > 0xffff || sid >= this.#player.pool.capacity) throw new RangeError('기술 표시의 현재 Technology SID 범위 오류');
        return sid;
    }

    // 004acac0 은 SID 가 아닌 저장 kind WORD 를 돌려주므로 kind 0 일치는 여전히 생성으로 진행한다. (2026-11-10 WB04-E215)
    indicate(type) {
        const found = this.#carrier.findContained(this.#parent(), 4, type, 0);
        const kind = found ? read(this.#player.pool.slot(found), 18, 2) : 0;
        if (!kind) this.#create.create(this.#parent(), type, 0, 4, 0x10, 0);
    }

    // 실제 같은 월드 어댑터 검사용 풀이다. (2026-11-10 WB04-E215)
    get pool() { return this.#player.pool; }
}
