// 레인 신전의 기본 그림·현재 물결·상태별 추가 그림을 원본 호출 순서로 제출하는 모듈이다(cpppj `o/RawRainVortexDraw.cpp/.h`, 10.78 전용).
//  - isRainVortex(raw): 실제 레인 신전 가상 표(10.78 0x5196f0)인지 본다. extra 의 abstract/특수 Draw 선택은 호출자 몫이다.
//  - drawRainVortex(raw, frames, hooks): 004dcd00. 0번과 현재 프레임(+36 signed DWORD)을 항상 hooks.frame(번호) 로 내고, WORD 상태(+12)가 있으면
//    현재 프레임이 'C' 구간 개수보다 작을 때 현재 코드 방향 구간의 첫 번호와 같으면 hooks.ascendSound()(templeAscend.wav)를 부른 뒤 'C' 첫 번호 + 현재 프레임(32비트 감김)을 낸다.
//    frames 는 레인 신전 TYPE 의 FrameCodeTable(`assets/terrain-tiles.js`)이다. 콜백 뒤 현재 raw 를 다시 읽어 원본의 조회 시점을 보존하므로 raw 는 살아 있는 슬롯 바이트를 넘긴다.
//    가상 표가 다르거나 두 소비자 중 하나라도 없으면 TypeError, 방향 프레임 범위 밖이면 RangeError 다.
//  - cpppj 의 판본 인자는 10.78 만 다루므로 받지 않는다(CD 0x501d28·BYTE 프레임 분기는 범위 밖).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E203: 신전·마나 추가 그림 이식으로 추가)

// 10.78 레인 신전 가상 표이다. (2026-11-09 WB04-E203)
const RAIN_TEMPLE_VTABLE = 0x5196f0;

// 슬롯의 WORD 상태·현재 프레임 위치와 상승 구간 글자이다. (2026-11-09 WB04-E203)
const STATE_AT = 12;
const FRAME_AT = 36;
const RISE_LETTER = 0x43;

// little endian 값을 정렬되지 않은 원본 슬롯에서 지정 폭만 부호 없이 읽는다. (2026-11-09 WB04-E203)
function read(raw, offset, width = 4) {
    if (offset + width > raw.length) throw new RangeError('레인 신전 Draw raw 범위 오류');
    let value = 0;
    // 지정한 폭만 읽는다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (index * 8);
    return value;
}

// 10.78 DWORD 프레임을 signed 로 읽는다. (2026-11-09 WB04-E203)
function frameOf(raw) {
    return read(raw, FRAME_AT) | 0;
}

// 같은 이름을 가진 다른 객체를 레인 신전 Draw 로 처리하지 않는다. (2026-11-09 WB04-E203)
export function isRainVortex(raw) {
    return read(raw, 0) === RAIN_TEMPLE_VTABLE;
}

// 0번과 현재 그림을 항상 내고 WORD 상태가 있을 때 C 구간 추가 그림/소리를 판정한다. (2026-11-09 WB04-E203)
export function drawRainVortex(raw, frames, hooks) {
    if (!isRainVortex(raw) || typeof hooks?.frame !== 'function' || typeof hooks?.ascendSound !== 'function') throw new TypeError('레인 신전 Draw 연결 누락');
    hooks.frame(0);
    hooks.frame(frameOf(raw));
    if (!read(raw, STATE_AT, 2)) return;
    const current = frameOf(raw);
    const rise = frames.run(RISE_LETTER);
    if (current >= rise.count) return;
    if (current < 0 || current >= frames.codes.length) throw new RangeError('레인 신전 Draw 방향 프레임 범위 오류');
    const side = frames.codes[current].side;
    if (current === frames.run(side).first) hooks.ascendSound();
    hooks.frame((rise.first + frameOf(raw)) | 0);
}
