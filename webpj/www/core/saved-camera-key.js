// 숫자 키(1..9,0)로 저장 카메라 10칸을 부르고, Shift+숫자 기호로 현재 지도 중심을 저장하는 입력 분배 모듈이다(cpppj `client/SavedCameraKey.cpp/.h`, 10.78 전용, 원문 005caeb0 저장 Point 80바이트·00439928 초기화·497e50 중심·004d5020 유효성).
// 저장 10칸은 Tab 의 최근 배치 5칸과 별도 Client 전역이며 일반 World/Renderer 교체로 초기화하지 않는다. 점은 단정도 float 비트(NaN 페이로드 포함)를 그대로 보관하기 위해 Uint32Array 80바이트 이미지로 둔다.
//  - initializeSavedCameraPositions(saved): 최초 10칸을 모두 무효 Point(−10000,−10000)로 쓴다.
//  - captureSavedCameraCenter(view, viewport): 현재 Renderer 지도 중심 = (뷰포트 L+R 합의 절반 + 픽셀 원점)을 32비트 감김 정수로 구해 현재 칸 크기(float)로 나눈 단정도 값이다(정수를 먼저 float 로 줄이지 않음).
//  - isSavedCameraPositionValid(x, y): 두 축 모두 0 이상 256 미만(세계 크기를 읽지 않음).
//  - dispatchSavedCameraKey(event, saved, hooks): 뗌이거나 세 기호 표/숫자에 속할 수 없는 문자는 소비하지 않는다(false, 장치 비요구). 정지면 소비만. 현재 공개 언어(3 독일어 `!"§$%&/()=`, 1 영어1 `!"£$%^&*()`, 그 밖 `!@#$%^&*()`)로 표를 고르고, Shift 없는 '*' 는 소비만,
//    Shift 면 기호 위치의 칸에 선택음 sound() 뒤 capture() 한 현재 중심을 저장하며, Shift 없으면 숫자('0' 은 10번째)의 칸이 유효할 때만 center(x, y)를 호출한다. Ctrl/Alt 비트는 제거하지 않는다.
// 선택된 장치의 누락은 그 위치에서 거부하고 앞선 변경(예: 선택음 뒤)을 되돌리지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E27: 저장 카메라 숫자 키 이식으로 추가)
import { InputCode } from './input-queue.js';

// 숫자 줄과 언어별 Shift 기호 표이다(보호 PE 005434c0/cc/d8 와 00543504/1c/34 의 실제 바이트; 영어1 의 £ 는 0xa3, 독일어의 § 는 0xa7). (2026-11-08 WB04-E27)
const DIGITS = '1234567890';
const DEFAULT_SYMBOLS = '!@#$%^&*()';
const ENGLISH_ONE_SYMBOLS = '!"£$%^&*()';
const GERMAN_SYMBOLS = '!"§$%&/()=';
// 저장 칸 수, 무효 Point 좌표, 지도 한 변, 수정키 제거 마스크이다. (2026-11-08 WB04-E27)
const SLOT_COUNT = 10;
const INVALID_POINT = -10000;
const WORLD_SIDE = 256;
const KEY_MASK = 0x03ffffff;

// 공개 언어 번호로 Shift 기호 표를 고른다(나머지는 기본 표). (2026-11-08 WB04-E27)
function symbolsOf(language) {
    return language === 3 ? GERMAN_SYMBOLS : language === 1 ? ENGLISH_ONE_SYMBOLS : DEFAULT_SYMBOLS;
}

// 원문 signed ADD 의 하위 DWORD 감김이다. (2026-11-08 WB04-E27)
const add = (first, second) => (first + second) | 0;

// 세 표 중 어디든 속할 수 있는 문자(0..255)인지이다. 언어를 미리 읽지 않고 좁은 키 몸체로 받는다. (2026-11-08 WB04-E27)
function isCandidate(character) {
    if (character > 255) return false;
    const letter = String.fromCharCode(character);
    return DIGITS.includes(letter) || DEFAULT_SYMBOLS.includes(letter) || ENGLISH_ONE_SYMBOLS.includes(letter) || GERMAN_SYMBOLS.includes(letter);
}

// 저장 Point 10칸의 80바이트 이미지이다(칸 i 의 x 비트 bits[2i], y 비트 bits[2i+1]). (2026-11-08 WB04-E27)
export class SavedCameraPositions {
    // 같은 버퍼를 비트와 float 로 함께 본다(읽기만 float 로, NaN 페이로드를 보존하려면 쓰기는 setBits 를 쓴다).
    bits = new Uint32Array(SLOT_COUNT * 2);
    #floats = new Float32Array(this.bits.buffer);

    // 칸 i 의 단정도 값 { x, y } 이다.
    get(index) { return { x: this.#floats[index * 2], y: this.#floats[index * 2 + 1] }; }

    // 칸 i 에 단정도 값을 쓴다.
    set(index, x, y) {
        this.#floats[index * 2] = x;
        this.#floats[index * 2 + 1] = y;
    }
}

// 00439928 의 최초 10칸 무효 Point 초기화이다. (2026-11-08 WB04-E27)
export function initializeSavedCameraPositions(saved) {
    // 열 좌표를 모두 같은 원문 무효 Point 로 쓴다.
    for (let index = 0; index < SLOT_COUNT; index += 1) saved.set(index, INVALID_POINT, INVALID_POINT);
}

// view = { pixels: [x, y], cellFloatBits: [x, y] }(현재 Renderer 픽셀 원점과 칸 크기 float 비트), viewport = [L, T, R, B] signed DWORD 이다. { x, y } 단정도 중심을 돌려준다. (2026-11-08 WB04-E27)
export function captureSavedCameraCenter(view, viewport) {
    const centerX = add(Math.trunc(add(viewport[0], viewport[2]) / 2), view.pixels[0]);
    const centerY = add(Math.trunc(add(viewport[1], viewport[3]) / 2), view.pixels[1]);
    const cell = new DataView(new ArrayBuffer(8));
    cell.setUint32(0, view.cellFloatBits[0] >>> 0, true);
    cell.setUint32(4, view.cellFloatBits[1] >>> 0, true);
    // FILD32 → FDIV F32 → FSTP F32 이며 정수의 조기 float 변환을 하지 않는다.
    return { x: Math.fround(centerX / cell.getFloat32(0, true)), y: Math.fround(centerY / cell.getFloat32(4, true)) };
}

// 004d5020 은 두 축 모두 0 이상 256 미만이며 세계 크기를 읽지 않는다(Tab 의 strict 양수 판정과 합치지 않음). (2026-11-08 WB04-E27)
export function isSavedCameraPositionValid(x, y) {
    return x >= 0 && y >= 0 && x < WORLD_SIDE && y < WORLD_SIDE;
}

// event = { code, x, y }, hooks = { paused(), language(), sound(), capture() → { x, y }, center(x, y) } 이다. 소비하면 true 이다. (2026-11-08 WB04-E27)
export function dispatchSavedCameraKey(event, saved, hooks) {
    if ((event.code & InputCode.release) !== 0 || !isCandidate((event.code & KEY_MASK) >>> 0)) return false;
    if (typeof hooks.paused !== 'function') throw new Error('숫자 저장 카메라의 실제 정지 판정 미연결');
    if (hooks.paused()) return true;
    if (typeof hooks.language !== 'function') throw new Error('숫자 저장 카메라의 현재 공개 언어 미연결');
    const symbols = symbolsOf(hooks.language());
    const character = String.fromCharCode(event.code & 255);
    if (!DIGITS.includes(character) && !symbols.includes(character)) return true;
    const shift = (event.code & InputCode.shift) !== 0;
    if (((event.code & KEY_MASK) >>> 0) === 0x2a && !shift) return true;
    // Ctrl/Alt/PressedSince 비트는 제거하지 않는다.
    const plain = (event.code & ~InputCode.shift) >>> 0;
    let index;
    if (shift) {
        if (plain > 255) return true;
        index = symbols.indexOf(String.fromCharCode(plain));
    } else index = plain === 0x30 ? 9 : plain - 0x31;
    if (index < 0 || index >= SLOT_COUNT) return true;
    if (shift) {
        if (typeof hooks.sound !== 'function') throw new Error('숫자 저장 카메라의 실제 선택음 미연결');
        hooks.sound();
        if (typeof hooks.capture !== 'function') throw new Error('숫자 저장 카메라의 현재 Renderer 중심 미연결');
        // 소리 이후의 현재 중심을 읽은 뒤에만 선택한 칸을 쓴다.
        const point = hooks.capture();
        saved.set(index, point.x, point.y);
    } else {
        const point = saved.get(index);
        if (isSavedCameraPositionValid(point.x, point.y)) {
            if (typeof hooks.center !== 'function') throw new Error('숫자 저장 카메라의 실제 즉시 중심 미연결');
            hooks.center(point.x, point.y);
        }
    }
    return true;
}
