// 썬더 캐논(TYPE 131)의 방향별 기본 프레임과 현재 프레임 추가 그림을 실제 캡처에 제공하는 모듈이다(cpppj `o/RawThunderCannonCapture.cpp/.h`, 10.78 전용).
//  - isThunderCannonCapture(raw): 보호 10.78 의 TYPE 131 / 가상 표 0x50bad0 쌍인지 본다(raw 는 50바이트 이상의 슬롯 바이트). 다른 파생 캐논을 추정하지 않는다.
//  - thunderCannonCaptureMainFrame(raw, frames): 0046ae10. 현재 프레임(+36)의 코드 방향 글자가 차지하는 TYPE 프레임 구간의 첫 번호를 돌려준다.
//    frames 는 TYPE 131 의 FrameCodeTable(`assets/terrain-tiles.js`, codes[].side·run(글자))이다. 현재 프레임이 코드 표 밖이면 RangeError 이며 단일 TYPE 기본 프레임으로 바꾸지 않는다.
//  - drawThunderCannonCapture(raw, draw, x, y): 0046ae30. 현재 TYPE/프레임을 draw(type, frame, x, y) 에 한 번 제출한다. extra·출력 WORD·기본 프레임 일치로 생략하지 않는다.
//  - ShapeToBuffer(`render/raw-shape-to-buffer.js`)의 state.mainFrame 과 state.additional 에 각각 연결한다. 다른 TYPE/가상 표·소비자 누락은 그리기 전에 거부한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E202: 그림·캡처 묶음의 썬더 캐논 캡처 이식으로 추가)

// 보호 TYPE 로딩 순서의 썬더 캐논 번호와 실제 생성자 46ae50 이 슬롯에 쓰는 가상 표이다. (2026-11-09 WB04-E202)
const THUNDER_CANNON_TYPE = 131;
const THUNDER_CANNON_VTABLE = 0x50bad0;

// 슬롯 크기·TYPE·현재 프레임 위치이다. (2026-11-09 WB04-E202)
const SLOT_SIZE = 50;
const TYPE_AT = 10;
const FRAME_AT = 36;

// 정렬되지 않은 원문 DWORD 를 이웃 바이트와 섞지 않고 부호 없이 읽는다. (2026-11-09 WB04-E202)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 원본이 확인되지 않은 타입/가상 표는 그림 요청 전에 거부한다. (2026-11-09 WB04-E202)
function validate(raw) {
    if (!isThunderCannonCapture(raw)) throw new TypeError('썬더 캐논 캡처의 다른 TYPE/가상 표');
}

// 가상 표와 타입 둘 다 일치하는 실제 50바이트 슬롯인지 본다. (2026-11-09 WB04-E202)
export function isThunderCannonCapture(raw) {
    return raw.length >= SLOT_SIZE && raw[TYPE_AT] === THUNDER_CANNON_TYPE && readDword(raw, 0) === THUNDER_CANNON_VTABLE;
}

// 현재 프레임을 유효한 방향 코드로 조회한다(부호 없는 DWORD 비교라 음수 프레임도 범위 밖이다). (2026-11-09 WB04-E202)
export function thunderCannonCaptureMainFrame(raw, frames) {
    validate(raw);
    const current = readDword(raw, FRAME_AT);
    if (current >= frames.codes.length) throw new RangeError('썬더 캐논 캡처의 현재 프레임 범위');
    return frames.run(frames.codes[current].side).first;
}

// 원래 497280 의 shape0/frame-1 은 현재 raw 타입/프레임을 뜻한다. (2026-11-09 WB04-E202)
export function drawThunderCannonCapture(raw, draw, x, y) {
    validate(raw);
    if (typeof draw !== 'function') throw new TypeError('썬더 캐논 캡처의 실제 그림 소비자 누락');
    draw(THUNDER_CANNON_TYPE, readDword(raw, FRAME_AT), x, y);
}
