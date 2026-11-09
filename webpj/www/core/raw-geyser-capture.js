// 간헐천(TYPE 122)의 고정 본체와 현재 애니메이션을 실제 캡처/표시에 공유하는 모듈이다(cpppj `o/RawGeyserCapture.cpp/.h`, 10.78 전용).
//  - isGeyserCapture(raw): 보호 10.78 의 TYPE 122 / 가상 표 0x50da88 쌍인지 본다. raw 는 50바이트 이상의 슬롯 바이트(Uint8Array)이며 다른 자원 객체를 추정하지 않는다.
//  - drawGeyserCapture(raw, draw, x, y): 00485480 처럼 현재 TYPE/프레임(+36 DWORD)을 497280 그림 소비자 draw(type, frame, x, y) 에 한 번 전달한다.
//    현재 프레임이 본체와 같아도 두 번째 그림을 낸다. 다른 TYPE/가상 표나 소비자 누락은 그리기 전에 거부한다. 추상색 관문은 실제 출력 소비자 몫이다.
//  - ShapeToBuffer(`render/raw-shape-to-buffer.js`)의 state.mainFrame 에는 TYPE 기본 프레임(4acfe0, 정의의 baseFrame)을, state.additional 에는 이 함수를 연결한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E202: 그림·캡처 묶음의 간헐천 캡처 이식으로 추가)

// 보호 10.78 TYPE 로딩 순서의 간헐천 번호와 실제 4857a0 생성자가 슬롯에 설치하는 가상 표이다. (2026-11-09 WB04-E202)
const GEYSER_TYPE = 122;
const GEYSER_VTABLE = 0x50da88;

// 슬롯 크기·TYPE·현재 프레임 위치이다. (2026-11-09 WB04-E202)
const SLOT_SIZE = 50;
const TYPE_AT = 10;
const FRAME_AT = 36;

// 정렬되지 않은 원문 DWORD 를 낮은 바이트부터 부호 없이 읽는다. (2026-11-09 WB04-E202)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 가상 표와 타입 둘 다 일치해야 원본 고정 본체/현재 그림 조합을 적용한다. (2026-11-09 WB04-E202)
export function isGeyserCapture(raw) {
    return raw.length >= SLOT_SIZE && raw[TYPE_AT] === GEYSER_TYPE && readDword(raw, 0) === GEYSER_VTABLE;
}

// 현재 프레임(부호 없는 DWORD 그대로)을 같은 TYPE 로 한 번 제출한다. (2026-11-09 WB04-E202)
export function drawGeyserCapture(raw, draw, x, y) {
    if (!isGeyserCapture(raw)) throw new TypeError('간헐천 캡처의 다른 TYPE/가상 표');
    if (typeof draw !== 'function') throw new TypeError('간헐천 캡처의 실제 그림 소비자 누락');
    draw(GEYSER_TYPE, readDword(raw, FRAME_AT), x, y);
}
