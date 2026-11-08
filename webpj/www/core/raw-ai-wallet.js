// AI 본체의 지갑(+0x2d8) 가산과 조회를 옮긴 모듈이다(cpppj `o/RawAiWallet.cpp/.h`, 10.78: 패치 00413bb0 가산·00413c20 조회).
//  - new RawAiWallet(source, players, reads): source 는 E164 `RawAiBrainMemory`(본체 저장소) 또는 `acquire()` 함수(→ `{ memory, owner }`)이다. 함수이면 모드/owner/NULL brain 의 조기 반환을 모두 지난 뒤에만 실제 본체 소유자를 선택하며,
//    선택한 소유자(`owner` 는 호출이 끝날 때까지 보유할 수명 증표, 없어도 된다)는 같은 호출 안의 데모 조회 뒤 재조회까지 그대로 쓴다. players 는 실제 9×180 BYTE Player 기록(Uint8Array) 또는 현재 base 를 매 사용 위치에서 돌려주는 함수이다
//    (재할당 가능한 Client base 는 조기 반환 뒤에만 읽고 span 을 보관하지 않는다). reads = { disabled()(00594fc8 이 비영이면 Player/brain 을 읽기 전에 반환), demo()(가산 뒤 00463b50 의 현재 데모 여부) } 둘 다 필수이다.
//  - credit(owner, amount): 비활성 또는 signed owner < 1 이면 반환, owner ≥ 9 는 거부, 현재 Player+0xb0 이 0 이면 반환한다. 그 밖에는 본체 +0x2d8 에 amount(int32)를 DWORD 감김으로 더하고, 데모이면 본체를 다시 조회해
//    현재 signed 지갑이 1190 이하일 때 1190 으로 올린다(데모 조회가 Player 포인터나 지갑을 바꿔도 처음 저장한 brain 의 현재 지갑을 읽는다). SP/난수/화면은 갱신하지 않는다.
//  - balance(owner): 같은 관문 뒤 현재 brain 의 signed 지갑을 읽기만 한다(데모/SP/본체 바이트 변경 없음). 해제/가짜/크기가 다른 본체는 지갑 성공으로 대신하지 않고 거부한다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E166: AI 지갑 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';

// 원본 Player 한 기록 폭/개수, brain 필드·지갑 필드 위치, 데모 하한(0x4a6, signed)이다. (2026-11-09 WB04-E166)
const PLAYER_BYTES = 180;
const PLAYER_SLOTS = 9;
const BRAIN_OFFSET = 0xb0;
const WALLET_OFFSET = 0x2d8;
const DEMO_FLOOR = 1190;

// 비정렬 DWORD 를 같은 바이트에서 읽는다(범위 밖은 거부한다). (2026-11-09 WB04-E166)
function read(bytes, at) {
    if (at < 0 || bytes.length - at < 4) throw new RangeError('AI 지갑 DWORD 읽기 범위');
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}

// 지갑 DWORD 만 바꾸고 다른 AI 본체 바이트를 보존한다. (2026-11-09 WB04-E166)
function put(bytes, at, value) {
    if (at < 0 || bytes.length - at < 4) throw new RangeError('AI 지갑 DWORD 쓰기 범위');
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (index * 8)) & 0xff;
}

// AI 지갑 처리기이다. 값형 지갑 사본 없이 같은 Player 9기록의 +0xb0 과 같은 0xb08 본체를 사용한다. (2026-11-09 WB04-E166)
export class RawAiWallet {
    #memory = null;
    #acquire = null;
    #players = null;
    #playersRead = null;
    #reads;

    // 모드/Player/본체 연결과 두 현재 조회기를 확인한다. 정적 Player 는 생성 때 크기를 확인하고 함수 Player 는 사용 위치에서 확인한다. (2026-11-09 WB04-E166)
    constructor(source, players, reads) {
        if (typeof reads?.disabled !== 'function' || typeof reads.demo !== 'function') throw new TypeError('AI 지갑의 현재 모드 조회 누락');
        if (typeof source === 'function') this.#acquire = source;
        else if (source && typeof source.bytes === 'function') this.#memory = source;
        else throw new TypeError('AI 지갑의 실제 본체 장치 누락');
        if (typeof players === 'function') this.#playersRead = players;
        else if (players instanceof Uint8Array && players.length === PLAYER_BYTES * PLAYER_SLOTS) this.#players = players;
        else throw new RangeError('AI 지갑의 실제 Player9 누락');
        this.#reads = reads;
    }

    // 조기 반환 뒤 현재 Player 물리를 조회하고 실제 9행 폭을 확인한다(원래 표의 사본이나 새 할당을 만들지 않는다). (2026-11-09 WB04-E166)
    #currentPlayers() {
        const current = this.#playersRead ? this.#playersRead() : this.#players;
        if (!(current instanceof Uint8Array) || current.length !== PLAYER_BYTES * PLAYER_SLOTS) throw new RangeError('AI 지갑의 현재 실제 Player9 물리 누락');
        return current;
    }

    // 선택된 비영 brain 에만 현재 실제 메모리/수명을 요구하며 없는 본체를 새 메모리로 대체하지 않는다. (2026-11-09 WB04-E166)
    #acquireMemory() {
        const selected = this.#acquire ? this.#acquire() : { memory: this.#memory, owner: null };
        if (!selected?.memory) throw new Error('AI 지갑의 선택된 실제 본체 owner 누락');
        return selected;
    }

    // 해제되거나 실제 크기가 다른 brain 을 지갑 사본으로 대신하지 않는다. (2026-11-09 WB04-E166)
    #body(token, selected) {
        const bytes = selected.memory.bytes(token);
        if (bytes.length !== AI_BRAIN_BYTES) throw new RangeError('AI 지갑의 실제0xb08 본체 누락');
        return bytes;
    }

    // 00413bb0: Player 포인터는 진입 시 읽고 데모 조회 뒤에도 저장해 둔 같은 brain 의 현재 지갑을 다시 읽는다. (2026-11-09 WB04-E166)
    credit(owner, amount) {
        if (this.#reads.disabled() || (owner | 0) < 1) return;
        if (owner >>> 0 >= PLAYER_SLOTS) throw new RangeError('AI 지갑의 실제 Player9 범위');
        const token = read(this.#currentPlayers(), (owner >>> 0) * PLAYER_BYTES + BRAIN_OFFSET);
        if (!token) return;
        const selected = this.#acquireMemory();
        let body = this.#body(token, selected);
        put(body, WALLET_OFFSET, (read(body, WALLET_OFFSET) + (amount >>> 0)) >>> 0);
        if (this.#reads.demo()) {
            body = this.#body(token, selected);
            const current = read(body, WALLET_OFFSET) | 0;
            put(body, WALLET_OFFSET, (current > DEMO_FLOOR ? current : DEMO_FLOOR) >>> 0);
        }
    }

    // 00413c20: 현재 모드/Player+0xb0/실제 본체의 signed 지갑을 읽는다. 데모 하한/분산 SP 는 조회하지 않는다. (2026-11-09 WB04-E166)
    balance(owner) {
        if (this.#reads.disabled() || (owner | 0) < 1) return 0;
        if (owner >>> 0 >= PLAYER_SLOTS) throw new RangeError('AI 지갑 조회 실제Player9 범위');
        const token = read(this.#currentPlayers(), (owner >>> 0) * PLAYER_BYTES + BRAIN_OFFSET);
        if (!token) return 0;
        const selected = this.#acquireMemory();
        return read(this.#body(token, selected), WALLET_OFFSET) | 0;
    }
}
