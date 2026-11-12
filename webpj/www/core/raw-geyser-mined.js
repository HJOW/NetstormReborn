// 간헐천 채집 통지(패킷 0x5b)의 현재 수량/cost 차감·게임 난수 소리·고갈 삭제와 emptyGeyser 생성을 옮긴 모듈이다(cpppj `o/RawGeyserMined.cpp/.h`, 10.78 전용, 핸들러 004266c0).
// 경제/운반 상위 상태를 별도 캐시에 보관하지 않고 handler 가 효과 콜백 뒤에도 현재 raw/전역을 다시 읽는다.
//  - handle(geyser): free 객체(상태 bit0)는 모든 효과를 건너뛴다. 아니면 게임 난수로 소리 번호 `(난수(6000)+1000)/1000`(1~6)을 정해 `geyserMined{번호}.wav` 를 현재 좌표에서 flags 0 으로 요청한다(소리마다 난수 정확히 한 번 전진).
//    소리 뒤 **현재** 수량 WORD(+12, unsigned)에서 현재 nugget 타입의 cost(float)를 뺀 값을 0 방향으로 절삭(signed64 하위 WORD, 불확정값은 0)해 수량으로 기록한다. 패치는 기록 뒤 다리(bridge) 수량 num assert(검사 전역이 켜졌고 abstract 비트가 없고
//    수량의 bit3..6 이 7 초과) 위반 시 오류이다. 이어 현재 boss 이고 수량이 0 이면 현재 좌표를 보관해 간헐천을 삭제(flags 0x1000)하고, 삭제 뒤 그 좌표의 현재 표면(정수 칸의 hash0 머리 TYPE 이 isle 타입)이 아니면
//    emptyGeyser 를 생성(create(type, 0))해 보관한 좌표로 배치(pop(sid, x, y, 0))한다. 좌표 정수화는 ftol 하위 DWORD 이며 NaN/무한/범위 밖은 0, 256 밖 칸은 표면 없음이다.
//  - handlePacket(packet): 패킷 +4 WORD 를 SID 로 읽어 handle 한다(짧은 패킷은 거부, 사용하지 않는 첫 두 handler 인자는 해석하지 않음).
// state = { boss, checking(값/getter), nuggetType, emptyGeyserType, isleType, bridgeType } 의 live 속성, hooks = { sound(x, y, name, flags), destroy(sid, flags), create(type, flags)→sid, pop(sid, x, y, flags) }(모두 필수).
//  - registerGeyserMined(dispatch, handler): 원본 초기화처럼 0x5b 에 빈 observer·버전 1/정책 0·실제 handler 를 등록한다(길이 6 은 등록 표 기본값).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E108: 간헐천 채집 통지 이식으로 추가, 2026-11-12 WB04-E323: 분배기 등록 RegisterGeyserMined 추가)

// 소리 난수 구간 크기·최소, 고갈 삭제 flags, 슬롯 위치, ftol 경계, 판본의 TYPE 수이다. (2026-11-08 WB04-E108)
const SOUND_CHOICES = 6000;
const SOUND_MINIMUM = 1000;
const DESTROY_FLAGS = 0x1000;
const TYPE_AT = 10;
const STATE_AT = 11;
const QUANTITY_AT = 12;
const X_AT = 14;
const Y_AT = 18;
const EXTRA_AT = 40;
const TYPE_COUNT = 188;
const INT64_BOUNDARY = 2 ** 63;

// 비정렬 float 를 little endian 으로 읽는다. (2026-11-08 WB04-E108)
function coordinate(raw, offset) {
    return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getFloat32(offset, true);
}

// 두 판본 모두 +12 의 unsigned WORD 를 경제 수량으로 읽는다. (2026-11-08 WB04-E108)
const quantityOf = (raw) => raw[QUANTITY_AT] | (raw[QUANTITY_AT + 1] << 8);

// float cost 와 정확한 WORD 값의 차이를 0 방향으로 절삭한 뒤 하위 16비트를 보존한다(불확정값은 0). (2026-11-08 WB04-E108)
function reducedQuantity(quantity, cost) {
    const result = quantity - cost;
    if (!Number.isFinite(result) || result >= INT64_BOUNDARY || result < -INT64_BOUNDARY) return 0;
    return Number(BigInt.asUintN(16, BigInt(Math.trunc(result))));
}

// 실제 __ftol 반환의 EAX 만 읽는 좌표 caller 를 보존한다. NaN/무한/범위 밖 값은 EAX 0 이다. (2026-11-08 WB04-E108)
function coordinateInteger(value) {
    if (!Number.isFinite(value) || value >= INT64_BOUNDARY || value < -INT64_BOUNDARY) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 간헐천 채집 통지 처리기이다. (2026-11-08 WB04-E108)
export class RawGeyserMined {
    // 간헐천/생성물의 유일한 raw 슬롯 저장소이다. (2026-11-08 WB04-E108)
    #pool;
    // 삭제 콜백이 바꾼 현재 표면 머리를 읽는 해시이다. (2026-11-08 WB04-E108)
    #hash;
    // 소리 뒤 현재 nugget 타입 cost 를 읽는 공유 타입 표이다. (2026-11-08 WB04-E108)
    #types;
    // 소리 하나마다 정확히 한 번 전진하는 전역 게임 난수이다. (2026-11-08 WB04-E108)
    #random;
    // 삭제/소리 콜백 뒤의 현재 권한/타입 번호 전역이다. (2026-11-08 WB04-E108)
    #state;
    // 같은 월드의 필수 음향/가상 삭제/생성/Pop 이다. (2026-11-08 WB04-E108)
    #hooks;

    // 입력 판본/필수 경계가 없으면 raw 수량이나 난수를 바꾸기 전에 거부한다. (2026-11-08 WB04-E108)
    constructor(pool, hash, types, random, state, hooks) {
        if (pool?.edition !== 'patch1078' || !types || types.length !== TYPE_COUNT
            || typeof hooks?.sound !== 'function' || typeof hooks.destroy !== 'function' || typeof hooks.create !== 'function' || typeof hooks.pop !== 'function') {
            throw new Error('간헐천 채집 타입/효과 연결 오류');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#random = random;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 생성/packet 어댑터의 같은 월드 확인용 풀이다. (2026-11-08 WB04-E108)
    get pool() { return this.#pool; }

    // 원본 setter 는 다른 수량/상태/소유자 필드를 변경하지 않는다. 기록 뒤 패치 bridge 수량 assert 를 수행한다. (2026-11-08 WB04-E108)
    #setQuantity(sid, quantity) {
        const bytes = this.#pool.allocated(sid);
        bytes[QUANTITY_AT] = quantity & 0xff;
        bytes[QUANTITY_AT + 1] = (quantity >> 8) & 0xff;
        const checking = typeof this.#state.checking === 'function' ? this.#state.checking() : this.#state.checking;
        if (bytes[TYPE_AT] === this.#state.bridgeType && !(bytes[EXTRA_AT] & 8) && Number(checking) !== 0 && ((bytes[QUANTITY_AT] >> 3) & 15) > 7) {
            throw new Error('Bridge 수량 num 원본 assert');
        }
    }

    // 원본 island getter 의 편향 없는 ftol 좌표와 같은 현재 표면 타입 비교이다. (2026-11-08 WB04-E108)
    #onIsle(x, y) {
        const ix = coordinateInteger(x);
        const iy = coordinateInteger(y);
        if (ix < 0 || iy < 0 || ix >= 256 || iy >= 256) return false;
        const surface = this.#hash.cell(0, ix, iy);
        return surface !== 0 && this.#pool.slot(surface)[TYPE_AT] === this.#state.isleType;
    }

    // 음향 → 현재 수량/cost → setter → 현재 boss/잔량 0 → 삭제 → 현재 표면 검사 → emptyGeyser 생성/Pop 이다. (2026-11-08 WB04-E108)
    handle(geyser) {
        const pool = this.#pool;
        const state = this.#state;
        const raw = pool.slot(geyser);
        if (raw[STATE_AT] & 1) return;
        const choice = Math.trunc((this.#random.next(SOUND_CHOICES) + SOUND_MINIMUM) / 1000);
        this.#hooks.sound(coordinate(raw, X_AT), coordinate(raw, Y_AT), `geyserMined${choice}.wav`, 0);
        if (state.nuggetType >= this.#types.length) throw new RangeError('간헐천 채집 현재 nugget 타입 범위');
        this.#setQuantity(geyser, reducedQuantity(quantityOf(pool.slot(geyser)), this.#types[state.nuggetType].cost));
        if (!state.boss || quantityOf(pool.slot(geyser)) !== 0) return;
        const current = pool.slot(geyser);
        const x = coordinate(current, X_AT);
        const y = coordinate(current, Y_AT);
        this.#hooks.destroy(geyser, DESTROY_FLAGS);
        if (!this.#onIsle(x, y)) {
            const next = this.#hooks.create(state.emptyGeyserType, 0);
            this.#hooks.pop(next, x, y, 0);
        }
    }

    // packet SID 검사는 byte 범위만 수행하고 free 여부는 실제 handler 가 현재 raw 에서 검사한다. (2026-11-08 WB04-E108)
    handlePacket(packet) {
        if (packet.length < 6) throw new RangeError('간헐천 채집 packet SID 범위');
        this.handle(packet[4] | (packet[5] << 8));
    }
}

// 간헐천 채집 통지의 패킷 타입(0x5b)과 원본 초기화가 등록하는 버전 BYTE·불일치 정책 BYTE 이다. 정책 0 은 버전이 달라도 계속 분배한다. (2026-11-12 WB04-E323: 분배기 등록 이식)
export const GEYSER_MINED_MESSAGE = 0x5b;
const GEYSER_MINED_VERSION = 1;
const GEYSER_MINED_MISMATCH = 0;

// 원본 초기화처럼 0x5b 의 observer 를 비우고(원본 observer 는 효과가 없는 빈 함수라 표를 비운다; 단일 분배는 observer 를 부르지 않는다) 버전 1/정책 0 과 실제 handler 를 등록한다.
// 길이는 등록 표 기본값(6)을 그대로 쓴다. 원본 handler 는 sender/연결을 읽지 않고 packet+4 SID 의 현재 raw 슬롯만 읽는다. handler 는 분배기보다 오래 살아야 한다.
// 사용: registerGeyserMined(dispatch, new RawGeyserMined(pool, hash, types, random, state, hooks)). (2026-11-12 WB04-E323: cpppj RegisterGeyserMined 이식)
export function registerGeyserMined(dispatch, handler) {
    if (!(handler instanceof RawGeyserMined)) throw new TypeError('간헐천 채집 등록에는 RawGeyserMined 처리기가 필요합니다');
    dispatch.observe(GEYSER_MINED_MESSAGE, null);
    dispatch.version(GEYSER_MINED_MESSAGE, GEYSER_MINED_VERSION, GEYSER_MINED_MISMATCH);
    dispatch.register(GEYSER_MINED_MESSAGE, (sender, connection, packet) => { handler.handlePacket(packet); });
}
