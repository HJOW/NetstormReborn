// 윈드·레인·썬더 신전의 실제 기본 프레임과 윈드/썬더 전용 추가 그림 순서를 옮긴 모듈이다(cpppj `o/RawTempleDraw.cpp/.h`, 10.78 전용).
//  - isPatchTemple(raw): 정확한 10.78 세 신전 가상 표(윈드 0x519860·레인 0x5196f0·썬더 0x519918)인지 본다. 이름이나 vortex 플래그가 같은 다른 표는 받지 않는다.
//  - patchTempleMainFrame(raw, baseFrames): 실제 4acfe0 의 현재 TYPE(+10)의 baseFrame 이다(-1 도 그대로). baseFrames 는 TYPE 번호로 찾는 188칸 정수 배열이다.
//    현재 애니메이션이나 가상 +88 을 읽지 않는다. 신전이 아니거나 TYPE 이 표 밖이면 TypeError 다.
//  - drawWindThunderTemple(raw, frames, baseFrame, frame): 실제 4dcf10(윈드)/4dd0f0(썬더) 추가 그림이다. 본체는 일반 SpriteDraw 가 먼저 그린다.
//    썬더는 모든 현재 프레임을, 윈드는 고정 본체(baseFrame)보다 앞의 회오리만 frame(번호) 로 낸다. WORD 상태(+12)가 있으면 현재 프레임이 'C' 구간 개수보다
//    작을 때 'C' 첫 번호 + 현재 프레임(32비트 감김)을 한 번 더 낸다. frames 는 현재 TYPE 의 FrameCodeTable(`assets/terrain-tiles.js`)이다.
//  - 레인 신전의 추가 그림은 `raw-rain-vortex-draw.js` 의 drawRainVortex 이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E203: 신전·마나 추가 그림 이식으로 추가)

// 10.78 세 신전 가상 표이다. (2026-11-09 WB04-E203)
const WIND_TEMPLE_VTABLE = 0x519860;
const RAIN_TEMPLE_VTABLE = 0x5196f0;
const THUNDER_TEMPLE_VTABLE = 0x519918;

// 슬롯의 TYPE·WORD 상태·현재 프레임 위치와 상승 구간 글자이다. (2026-11-09 WB04-E203)
const TYPE_AT = 10;
const STATE_AT = 12;
const FRAME_AT = 36;
const RISE_LETTER = 0x43;

// 비정렬 슬롯의 지정 폭만 부호 없이 읽는다. 범위 밖이면 RangeError 다. (2026-11-09 WB04-E203)
function read(raw, at, width = 4) {
    if (at + width > raw.length) throw new RangeError('신전 Draw raw 범위 오류');
    let value = 0;
    // 원본 little endian 바이트 순서다.
    for (let index = 0; index < width; index += 1) value += raw[at + index] * 2 ** (index * 8);
    return value;
}

// 10.78 프레임은 signed DWORD 이며 WORD 상태와 폭을 섞지 않는다. 호출마다 현재 raw 를 다시 읽는다. (2026-11-09 WB04-E203)
function current(raw) {
    return read(raw, FRAME_AT) | 0;
}

// 정확한 세 신전 가상 표만 받는다. (2026-11-09 WB04-E203)
export function isPatchTemple(raw) {
    const table = read(raw, 0);
    return table === WIND_TEMPLE_VTABLE || table === RAIN_TEMPLE_VTABLE || table === THUNDER_TEMPLE_VTABLE;
}

// 실제 helper 는 TYPE 의 baseFrame 한 필드만 읽는다. (2026-11-09 WB04-E203)
export function patchTempleMainFrame(raw, baseFrames) {
    if (!isPatchTemple(raw) || raw[TYPE_AT] >= baseFrames.length) throw new TypeError('신전 기본 프레임 자료 불일치');
    return baseFrames[raw[TYPE_AT]];
}

// 윈드는 고정 본체보다 앞의 회오리만, 썬더는 모든 현재 프레임을 추가 그림으로 낸다. (2026-11-09 WB04-E203)
export function drawWindThunderTemple(raw, frames, baseFrame, frame) {
    const table = read(raw, 0);
    if ((table !== WIND_TEMPLE_VTABLE && table !== THUNDER_TEMPLE_VTABLE) || typeof frame !== 'function') throw new TypeError('윈드/썬더 전용 Draw 연결 누락');
    if (table === THUNDER_TEMPLE_VTABLE || current(raw) < baseFrame) frame(current(raw));
    if (!read(raw, STATE_AT, 2)) return;
    const rise = frames.run(RISE_LETTER);
    if (current(raw) >= rise.count) return;
    frame((rise.first + current(raw)) | 0);
}
