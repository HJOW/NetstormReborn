// 훨리베이스(TYPE 72)의 생산 상태별 기본/추가 가상 그림을 실제 픽셀 캡처에 제공하는 모듈이다(cpppj `o/RawSunAviaryCapture.cpp/.h`, 10.78 전용).
//  - isSunAviaryCapture(raw): 보호 10.78 의 TYPE 72 / 가상 표 0x509930 쌍인지 본다(raw 는 50바이트 이상의 슬롯 바이트). 다른 홈의 파생 그림을 추정하지 않는다.
//  - sunAviaryCaptureMainFrame(raw): 0045b530. extra(+40)&9 이면 현재 프레임, 아니면 signed 현재 프레임 20..26 → 10, 11..19 → 현재−9, 그 밖은 기본 그림 없음(-1)이다.
//  - drawSunAviaryCapture(raw, draw, x, y): 0045b560. extra&9 이면 소비자 없이 생략하고, 나머지는 현재 TYPE/프레임을 draw(type, frame, x, y) 에 한 번 제출한다.
//  - ShapeToBuffer(`render/raw-shape-to-buffer.js`)의 state.mainFrame 과 state.additional 에 각각 연결한다. 다른 TYPE/가상 표는 그리기 전에 거부한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E202: 그림·캡처 묶음의 훨리베이스 캡처 이식으로 추가)

// 원본 TYPE 로딩 순서의 훨리베이스 번호와 원본 생성자·홈 통지가 공유하는 실제 가상 표이다. (2026-11-09 WB04-E202)
const SUN_AVIARY_TYPE = 72;
const SUN_AVIARY_VTABLE = 0x509930;

// 슬롯 크기·TYPE·현재 프레임·extra 위치와 abstract+buried 생략 비트이다. (2026-11-09 WB04-E202)
const SLOT_SIZE = 50;
const TYPE_AT = 10;
const FRAME_AT = 36;
const EXTRA_AT = 40;
const ABSTRACT_BURIED = 9;

// 정렬되지 않은 DWORD 를 해당 네 바이트에서만 부호 없이 읽는다. (2026-11-09 WB04-E202)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 확인되지 않은 타입/가상 표는 실제 그림 요청 전에 거부한다. (2026-11-09 WB04-E202)
function validate(raw) {
    if (!isSunAviaryCapture(raw)) throw new TypeError('훨리베이스 캡처의 다른 TYPE/가상 표');
}

// 판본의 실제 50바이트 슬롯과 두 식별자를 함께 확인한다. (2026-11-09 WB04-E202)
export function isSunAviaryCapture(raw) {
    return raw.length >= SLOT_SIZE && raw[TYPE_AT] === SUN_AVIARY_TYPE && readDword(raw, 0) === SUN_AVIARY_VTABLE;
}

// 생산 앞구간은 이동한 기본 그림, 뒷구간은 10번이며 extra 관문은 현재 그림(signed)을 돌려준다. (2026-11-09 WB04-E202)
export function sunAviaryCaptureMainFrame(raw) {
    validate(raw);
    const current = readDword(raw, FRAME_AT) | 0;
    if (raw[EXTRA_AT] & ABSTRACT_BURIED) return current;
    if (current >= 20 && current <= 26) return 10;
    return current >= 11 && current <= 19 ? current - 9 : -1;
}

// 생략 경로는 소비자를 요구하지 않으며 실제 497280 의 shape0/frame-1 을 현재 값(부호 없는 DWORD)으로 전달한다. (2026-11-09 WB04-E202)
export function drawSunAviaryCapture(raw, draw, x, y) {
    validate(raw);
    if (raw[EXTRA_AT] & ABSTRACT_BURIED) return;
    if (typeof draw !== 'function') throw new TypeError('훨리베이스 캡처의 실제 그림 소비자 누락');
    draw(SUN_AVIARY_TYPE, readDword(raw, FRAME_AT), x, y);
}
