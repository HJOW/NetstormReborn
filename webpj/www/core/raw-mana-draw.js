// 마나(TYPE 103)의 조건부 그림자/현재 프레임 추가 그림을 원문 00473710 의 조회·출력 순서로 옮긴 모듈이다(cpppj `o/RawManaDraw.cpp/.h`).
//  - drawManaAdditional(raw, x, y, hooks): TYPE(+10)을 한 번 고르고, WORD(+12)가 0 이 아니면 그림자를 먼저 낸 뒤 본체를 낸다.
//    hooks.shape(type) → 처음 고른 TYPE 의 현재 SHP 토큰(+dc, 실제 주소로 실행하지 않음)을 매 출력 직전에 읽는다.
//    hooks.draw({ shape, frame, x, y, shadow }) → 실제 VFX 요청이다. frame 은 각 출력 직전에 읽은 +36 DWORD(부호 없음), 그림자는 557738 치환 출력이며 두 축에 10 을 더한다(32비트 감김).
//    raw 는 살아 있는 슬롯 바이트(40바이트 이상)이며, 그림자 출력 뒤 바뀐 프레임/SHP 는 본체에 반영하지만 바뀐 TYPE 으로 다시 고르지 않는다.
//  - 소유자/dead/추상 관문을 덧붙이지 않는다. raw 가 짧거나 소비자가 없으면 TypeError 다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E203: 신전·마나 추가 그림 이식으로 추가)

// 슬롯의 TYPE·WORD 상태·현재 프레임 위치와 최소 길이, 그림자 좌표 이동량이다. (2026-11-09 WB04-E203)
const TYPE_AT = 10;
const STATE_AT = 12;
const FRAME_AT = 36;
const MIN_RAW = 40;
const SHADOW_OFFSET = 10;

// 원문 +36 DWORD 를 각 호출 때 다시 읽는다(부호 없음). (2026-11-09 WB04-E203)
function frameOf(raw) {
    return (raw[FRAME_AT] | (raw[FRAME_AT + 1] << 8) | (raw[FRAME_AT + 2] << 16) | (raw[FRAME_AT + 3] << 24)) >>> 0;
}

// 원문의 LEA+10 하위 DWORD 감김이다. (2026-11-09 WB04-E203)
function shadowCoordinate(value) {
    return (value + SHADOW_OFFSET) | 0;
}

// 화면/선택 출력이 공유하는 원문 몸체이다. raw/TYPE 저장소는 두 출력 동안 같은 객체로 살아 있어야 한다. (2026-11-09 WB04-E203)
export function drawManaAdditional(raw, x, y, hooks) {
    if (!raw || raw.length < MIN_RAW || typeof hooks?.shape !== 'function' || typeof hooks?.draw !== 'function') throw new TypeError('마나 추가 Draw raw/SHP/출력 경계 필요');
    const type = raw[TYPE_AT];
    if ((raw[STATE_AT] | (raw[STATE_AT + 1] << 8)) !== 0) {
        // 원문은 프레임을 먼저 읽고 선택한 TYPE 의 현재 SHP 를 읽는다.
        const frame = frameOf(raw);
        const shape = hooks.shape(type);
        hooks.draw({ shape, frame, x: shadowCoordinate(x), y: shadowCoordinate(y), shadow: true });
    }
    // 그림자 소비자가 바꾼 프레임/SHP 를 본체에 반영한다.
    const frame = frameOf(raw);
    const shape = hooks.shape(type);
    hooks.draw({ shape, frame, x: x | 0, y: y | 0, shadow: false });
}
