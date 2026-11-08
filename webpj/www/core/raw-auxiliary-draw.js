// 메타 배율 표시(00479340)와 고정 추상색 추가 Draw(00485f90)의 원문 필드/출력 순서를 옮긴 모듈이다(cpppj `o/RawAuxiliaryDraw.cpp/.h`, 10.78 전용). 원문 마지막 VFX 경계는 훅이 받는다.
//  - drawScaledMetadisplay(raw, x, y, hooks): 슬롯 +26 DWORD 의 숨김 비트 0x4000 이 있으면 프레임/SHP 장치 없이 조기 반환한다. 아니면 정수 배율 `(+26 & 0x3fff) << 16` 에 분수 WORD(+8)를 더하고 0 이면 1배(0x10000)로 바꾼 뒤,
//    현재 프레임 DWORD(+36)를 읽고 현재 TYPE(+10)의 SHP 를 조회해 원색 배율 출력(abstract false, scale = signed 16.16 값)을 요청한다.
//  - drawAbstractAdditional(raw, x, y, hooks): 소유자/dead/visible 관문 없이 현재 프레임과 TYPE 의 SHP 를 읽어 고정 추상색 치환표(abstract true, scale 0)로 한 번 출력한다.
// 훅 = { shape(TYPE)→SHP 토큰, draw({ shape, frame, x, y, abstract, scale }) } 이며 도달한 출력 분기에서만 요구한다. 슬롯과 SHP 는 이 동기 출력 동안 같은 주소로 유지되어야 하고 슬롯을 바꾸지 않는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E84: 메타 배율/추상색 추가 Draw 이식으로 추가)

// 슬롯의 분수 WORD(+8)·플래그/정수 배율 DWORD(+26)·TYPE(+10)·프레임(+36) 위치, 숨김 비트, 정수 배율 마스크, 1배 값이다. (2026-11-08 WB04-E84)
const FRACTION = 8;
const FLAGS = 26;
const TYPE = 10;
const FRAME = 36;
const HIDDEN = 0x4000;
const INTEGER_SCALE_MASK = 0x3fff;
const ONE = 0x10000;

// 원문 little endian 필드의 하위값을 읽는다(WORD 또는 DWORD, 범위 밖은 거부). (2026-11-08 WB04-E84)
function read(raw, offset, bytes = 4) {
    if (offset > raw.length || bytes > raw.length - offset) throw new RangeError('추가 Draw raw 필드 범위 오류');
    let result = 0;
    // WORD 또는 DWORD 만 원래 순서로 조합한다.
    for (let index = 0; index < bytes; index += 1) result += raw[offset + index] * 2 ** (8 * index);
    return result >>> 0;
}

// 도달한 실제 출력 분기의 필수 장치를 확인한다. (2026-11-08 WB04-E84)
function require(hooks) {
    if (typeof hooks?.shape !== 'function' || typeof hooks.draw !== 'function') throw new TypeError('추가 Draw SHP/출력 경계 필요');
}

// hidden 4000 조기 반환 → 현재 16.16 배율(0 은 1배) → 현재 프레임/TYPE/SHP → 원색 배율 출력이다. (2026-11-08 WB04-E84)
export function drawScaledMetadisplay(raw, x, y, hooks) {
    const flags = read(raw, FLAGS);
    if (flags & HIDDEN) return;
    require(hooks);
    // 원문 정수 배율과 분수 WORD 를 합산하며 0 만 1배로 바꾼다.
    let scale = (((flags & INTEGER_SCALE_MASK) << 16) + read(raw, FRACTION, 2)) >>> 0;
    if (!scale) scale = ONE;
    // 원문은 프레임을 준비한 뒤 현재 TYPE 의 SHP 를 조회한다.
    const frame = read(raw, FRAME);
    const shape = hooks.shape(raw[TYPE]);
    hooks.draw({ shape, frame, x, y, abstract: false, scale: scale | 0 });
}

// 현재 프레임/TYPE/SHP 를 읽어 고정 치환표로 한 번 출력한다. 별도 visible/dead/owner 필터를 추가하지 않는다. (2026-11-08 WB04-E84)
export function drawAbstractAdditional(raw, x, y, hooks) {
    require(hooks);
    const frame = read(raw, FRAME);
    const shape = hooks.shape(raw[TYPE]);
    hooks.draw({ shape, frame, x, y, abstract: true, scale: 0 });
}
