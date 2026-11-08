// AI 직접 명령의 본체 쓰기 두 가지(00413c60 CrankAiMoney, 004143b0 SetAiParameter)를 옮긴 모듈이다(cpppj `o/RawAiCommandState.cpp/.h`, 10.78 전용).
// 기존 가산/전략 프레임과 구별한다. 원문 직접 writer 는 지갑 Credit/데모 하한/분산 SP 갱신을 호출하지 않고, 현재 Player9(180바이트×9) 표의 +0xb0 AI 본체 포인터를 방문마다 새로 읽어 실제 생존 0xb08 본체의 DWORD 만 쓴다.
//  - crankAiMoney(): 중립 0 을 방문하지 않고 1..8 번 Player 를 차례로 읽어 비영 본체의 +0x2d8 에 30000 을 쓴다(앞선 실제 경계가 후속 Player 를 바꾸면 다음 방문이 현재 값을 읽는다).
//  - setAiParameter(packed): 소유자는 일반 1..8 이 아니라 packed 의 하위 3비트이고 선택자는 비트 4..11 의 8비트(0..4 만 지원)이다. 값은 signed 상위 16비트이다.
//    선택자 0 은 float 값을 +0x220 과 +0x600 에 쓰며 두 쓰기마다 Player+0xb0 을 다시 읽는다(한 저장 포인터로 합치지 않음). 선택자 1 은 float 을 +0x228, 2·3·4 는 정수 DWORD 를 +0x230/+0x234/+0x24c 에 쓴다.
// 선택하지 않은 하위 경계는 요구하지 않으며(owner 0·본체 없음·미지원 선택자는 body 훅 없이 반환), 도달한 Player9 표 길이와 0xb08 본체 길이가 어긋나면 성공으로 바꾸지 않고 거부한다. 이 모듈은 DOM 을 쓰지 않는다.
// 훅 = { players()→Uint8Array(1620)(방문마다 현재 표), body(token)→Uint8Array(0xb08)(비영 포인터를 같은 실제 생존 본체로 해석) } 이다. (2026-11-08 WB04-E76: AI 직접 명령 이식으로 추가)

// 10.78 Player 한 칸 폭·칸 수와 AI 본체 포인터 위치·본체 물리 폭이다. (2026-11-08 WB04-E76)
const PLAYER_BYTES = 180;
const PLAYER_SLOTS = 9;
const BRAIN_OFFSET = 0xb0;
const BODY_BYTES = 0xb08;
// CrankAiMoney 가 쓰는 위치와 값, 선택자별 기록 위치(1..4)와 선택자 0 의 두 float 위치이다. (2026-11-08 WB04-E76)
const MONEY_OFFSET = 0x2d8;
const MONEY_VALUE = 30000;
const SELECTOR_OFFSETS = Object.freeze([0, 0x228, 0x230, 0x234, 0x24c]);
const SELECTOR_ZERO_OFFSETS = Object.freeze([0x220, 0x600]);

// 선택하지 않은 하위 경계를 미리 요구하지 않고 도달했을 때만 연결 여부를 확인한다. (2026-11-08 WB04-E76)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI 직접 명령의 실제 현재 저장소 미연결');
    return hook;
}

// 단정도 float 의 비트 패턴(uint32)을 만드는 공용 DataView 이다. (2026-11-08 WB04-E76)
const view = new DataView(new ArrayBuffer(4));
const floatBits = (value) => { view.setFloat32(0, value, true); return view.getUint32(0, true); };

export class RawAiCommandState {
    // 현재 Player9 표와 본체 해석 훅이다. 값형 지갑/AI 사본이나 새 생성 성공을 제공하지 않는다.
    #hooks;

    // 훅 묶음을 보관한다(구성 때 연결 여부는 요구하지 않는다). (2026-11-08 WB04-E76)
    constructor(hooks = {}) {
        this.#hooks = hooks ?? {};
    }

    // 현재 Player base 를 각 방문/재조회 위치에서 새로 읽어 owner 의 +0xb0 네 BYTE 를 리틀엔디언 DWORD 로 돌려준다. 표 길이가 다르면 거부한다. (2026-11-08 WB04-E76)
    #brain(owner) {
        const players = need(this.#hooks.players)();
        if (!players || players.length !== PLAYER_BYTES * PLAYER_SLOTS) throw new RangeError('AI 직접 명령의 현재 Player9 누락');
        const at = owner * PLAYER_BYTES + BRAIN_OFFSET;
        return ((players[at]) | (players[at + 1] << 8) | (players[at + 2] << 16) | (players[at + 3] << 24)) >>> 0;
    }

    // 같은 생존 본체만 돌려주며 NULL 재조회/잘못된 폭을 성공으로 바꾸지 않는다. (2026-11-08 WB04-E76)
    #body(token) {
        if (!token) throw new Error('AI 직접 명령의 재조회된 NULL 본체');
        const raw = need(this.#hooks.body)(token);
        if (!raw || raw.length !== BODY_BYTES) throw new RangeError('AI 직접 명령의 실제 0xb08 본체 누락');
        return raw;
    }

    // 한 DWORD 외의 본체 바이트를 보존하도록 낮은 바이트부터 네 BYTE 만 쓴다. (2026-11-08 WB04-E76)
    static #put(raw, at, value) {
        // 원문 little endian 물리 네 BYTE 만 쓴다.
        for (let byte = 0; byte < 4; byte += 1) raw[at + byte] = (value >>> (byte * 8)) & 0xff;
    }

    // 현재 owner 1..8 의 비영 AI 본체 +0x2d8 을 30000 으로 직접 쓴다. mode/데모/SP 는 읽지 않는다. (2026-11-08 WB04-E76)
    crankAiMoney() {
        // 앞선 실제 경계가 후속 Player 를 바꾸더라도 다음 방문은 현재 값을 읽는다.
        for (let owner = 1; owner < PLAYER_SLOTS; owner += 1) {
            const token = this.#brain(owner);
            if (token) RawAiCommandState.#put(this.#body(token), MONEY_OFFSET, MONEY_VALUE);
        }
    }

    // packed 의 하위 3비트 owner·비트 4..11 선택자·signed 상위 16비트 값으로 다섯 직접 writer 중 하나를 수행한다. 선택자 0 은 두 현재 포인터를 재조회한다. (2026-11-08 WB04-E76)
    setAiParameter(packed) {
        const word = packed >>> 0;
        const owner = word & 7;
        if (!owner) return;
        const token = this.#brain(owner);
        if (!token) return;
        const selector = (word >>> 4) & 255;
        if (selector > 4) return;
        // 비트 패턴을 signed 로 보고 산술 오른쪽 이동한다.
        const value = (word | 0) >> 16;
        if (!selector) {
            const bits = floatBits(value);
            // 원문 fst 뒤 Player+b0 를 다시 읽는 두 포인터를 한 저장 포인터로 합치지 않는다.
            for (const offset of SELECTOR_ZERO_OFFSETS) RawAiCommandState.#put(this.#body(this.#brain(owner)), offset, bits);
            return;
        }
        const bits = selector === 1 ? floatBits(value) : value >>> 0;
        // 나머지는 원문 ESI 에 보관한 첫 brain 을 사용한다.
        RawAiCommandState.#put(this.#body(token), SELECTOR_OFFSETS[selector], bits);
    }
}
