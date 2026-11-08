// 다이스(Dais) 165 의 별도 TYPE 169 추가 그림 요청(004484b0)을 원본 현재 프레임으로 계산하는 모듈이다(cpppj `o/RawDaisDraw.cpp/.h`, 10.78 전용).
//  - isPatchDais(raw): 슬롯의 가상 표가 0x507950 이고 TYPE 이 165 일 때만 참이다(다른 자산에 추가 그림을 적용하지 않음). 슬롯이 4바이트보다 짧으면 거부한다.
//  - drawDaisAdditional(raw, extraFrameCount, draw): 현재 프레임 DWORD(+36)를 signed 로 읽어 TYPE 169 의 현재 전체 프레임 수로 나눈 signed 나머지를 draw(169, 프레임)에 넘긴다(다이스 165 의 프레임 수로 대체하지 않음).
//    추상/로컬/dead 관문은 이 함수에 없으며 497280 소비자(draw)가 처리한다. 다이스가 아니거나 프레임 수가 0 이하이거나 소비자가 없으면 거부한다. 슬롯을 바꾸지 않는다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E84: 다이스 추가 그림 이식으로 추가)

// 원본 005412fc 의 고정 자산 로딩 번호(daisExtraFrames)와 다이스의 가상 표/TYPE, 슬롯의 TYPE(+10)·프레임(+36) 위치이다. (2026-11-08 WB04-E84)
export const PATCH_DAIS_EXTRA_TYPE = 169;
const DAIS_TABLE = 0x507950;
const DAIS_TYPE = 165;
const TYPE = 10;
const FRAME = 36;

// 정렬되지 않은 원본 DWORD 를 필요한 위치에서만 읽는다. (2026-11-08 WB04-E84)
function read(raw, offset) {
    if (offset + 4 > raw.length) throw new RangeError('다이스 그림 raw 범위 오류');
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 생성자 주소가 확인된 실제 다이스만 별도 그림으로 분배한다. (2026-11-08 WB04-E84)
export function isPatchDais(raw) {
    return read(raw, 0) === DAIS_TABLE && raw.length > TYPE && raw[TYPE] === DAIS_TYPE;
}

// TYPE 169 의 현재 전체 프레임 수를 사용하며 signed 나머지(`-0` 없음)를 소비자에 넘긴다. (2026-11-08 WB04-E84)
export function drawDaisAdditional(raw, extraFrameCount, draw) {
    if (!isPatchDais(raw) || !(extraFrameCount > 0) || typeof draw !== 'function') throw new TypeError('다이스 추가 그림 TYPE/프레임 수/소비자 오류');
    // 원본 idiv 의 signed DWORD 프레임이다.
    const frame = read(raw, FRAME) | 0;
    draw(PATCH_DAIS_EXTRA_TYPE, (frame % (extraFrameCount | 0)) | 0);
}
