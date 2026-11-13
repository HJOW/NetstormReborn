// 원본 VFX 변환 커널의 회전 없는 16.16 확대·축소·반전을 8비트 표면에 복원한 모듈이다(cpppj `client/VFXTransform.cpp/.h`, 원문 004028cb·0040351d, 10.78 전용).
//  - drawScaledVfxImage(pixels, pitch, height, image, frame, position, scaleX, scaleY, clip, colors): 일반 VFX 변환 출력이다. 배율이 정확히 1.0(0x10000)이면 기존 단위 배율 직접 출력
//    (render/indexed-surface.js drawIndexedImage)으로 보내 255 도 불투명 색으로 둔다. 그 밖의 배율은 꼭짓점을 변환해 임시 표면처럼 255 를 투명으로 취급하며 출력한다.
//  - drawTranslatedVfxImage(pixels, pitch, height, image, frame, position, scaleX, scaleY, clip, colors): 0040351d 의 angle=0 이다. 소스 그림은 255 가 아닌 불투명 마스크로만 쓰고,
//    마스크 아래의 현재 대상 픽셀 색을 colors[현재 색] 으로 바꿔 쓴다. 단위 배율도 변환 꼭짓점/한 행 생략을 거치며 변환 결과 255 도 그대로 저장한다.
//  - 배율은 signed 16.16 이고 음수는 축 반전이다. clip 은 반개방 사각형 { left, top, right, bottom } 이며 표면 전체와 교차한다. position 은 프레임 기준점이다.
//  - 프레임/변환 끝점은 signed WORD(−32768~32767), 각 축 간격은 32767 이하만 지원하고 그 밖은 RangeError 로 거부한다(원본 고정소수점 감김으로 버퍼 밖을 읽는 극단 입력).
//  - 모든 좌표 누산은 32비트 DWORD 감김이며 첫 픽셀이 클리핑돼도 같은 기울기로 전진한 위치에서 시작한다. 이 모듈은 DOM 을 쓰지 않는다.
// image = { width, height, indices, opacity }(ShapeDatabase.decode 결과), frame = { rect: { left, top, right, bottom }, special }(ShapeDatabase.frame 결과).
// (2026-11-13 WB04-E378: 5-A VFXTransform 이식으로 추가, 기준선 뒤 cpppj 변경 없음)
import { clipRect, drawIndexedImage } from './indexed-surface.js';

// 원본 배율 1 과 텍스처 좌표의 한 픽셀은 같은 16.16 단위(0x10000)다. (2026-11-13 WB04-E378)
const FIXED_UNIT = 0x10000;
// 텍스처 좌표의 반 픽셀 편향이다. 꼭짓점 변환의 64비트 덧셈에는 같은 비트를 따로 쓴다. (2026-11-13 WB04-E378)
const TEXTURE_BIAS = 0x8000;
// 변환 임시 표면은 255 로 채워지며 색 치환 후 같은 값도 투명으로 남는다. (2026-11-13 WB04-E378)
const SCRATCH_TRANSPARENT = 255;
// x86 고정소수점 좌표와 기울기의 정상 signed WORD 범위이다. (2026-11-13 WB04-E378)
const WORD_MINIMUM = -32768;
const WORD_MAXIMUM = 32767;

// 기준점 이동의 하위 DWORD 를 보존하는 32비트 감김 덧셈이다. (2026-11-13 WB04-E378)
const add = (left, right) => (left + right) | 0;

// 꼭짓점의 원본 IMUL/ADD 8000/ADC 를 계산한다. 좌표를 16비트 올린 값에 배율을 곱한 64비트 곱의 하위 DWORD 에만 반 픽셀 편향을 더하고 상위 DWORD(몫, 음수는 아래 정수로 내림)를 돌려준다.
// 반 픽셀을 더하는 일반 반올림과 다르다 — 곱의 하위 16비트가 항상 0 이라 편향 0x8000 은 상위 DWORD 로 올림을 내지 않아 결과는 곱의 내림 몫과 같다(원문 명령 순서를 보존하려고 더한다).
// 사용: scaleCoordinate(10, 0x8000) === 5. 64비트 곱이 Number 정밀도를 넘으므로 BigInt 로 계산한다. (2026-11-13 WB04-E378)
export function scaleCoordinate(coordinate, scale) {
    const shifted = coordinate << 16;
    const biased = BigInt(shifted) * BigInt(scale) + BigInt(TEXTURE_BIAS);
    return Number(BigInt.asIntN(32, biased >> 32n));
}

// 원본 signed IDIV 의 0 방향 절삭 기울기(16.16)이다. 분모는 양수이며 검증한 간격(1~32767)만 전달한다. 정밀도는 |delta × 65536| ≤ 2^31 이라 Number 로 충분하다. (2026-11-13 WB04-E378)
export function textureStep(delta, span) {
    return Math.trunc(delta * FIXED_UNIT / span);
}

// 원본 좌표 누산의 DWORD 감김을 유지한다. 클리핑된 첫 픽셀까지도 같은 기울기로 전진한다(부호 없는 32비트 값을 돌려준다). (2026-11-13 WB04-E378)
const advance = (value, step, count) => (value + Math.imul(step, count)) >>> 0;

// 픽셀 중심 좌표(인덱스 << 16 + 반 픽셀)를 만든다. 호출자는 0~32767 의 원본 픽셀 인덱스만 넘긴다. (2026-11-13 WB04-E378)
const textureStart = (index) => ((index << 16) + TEXTURE_BIAS) >>> 0;

// 원본 WORD 좌표를 정규화 없이 해석할 수 있는 범위인지 검사한다. (2026-11-13 WB04-E378)
const isWord = (value) => value >= WORD_MINIMUM && value <= WORD_MAXIMUM;

// 두 원문 변환 커널의 같은 꼭짓점/텍스처 순회를 공유하되 픽셀 연산(일반 출력 / 대상 색 변환)은 translated 로 구별한다. (2026-11-13 WB04-E378)
function drawVfxImage(pixels, pitch, height, image, frame, position, scaleX, scaleY, clip, colors, translated) {
    if (pitch < 1 || height < 1 || pitch * height > pixels.length) throw new RangeError('VFX 변환 표면 용량 오류');
    if (image.width < 1 || image.height < 1 || image.width > WORD_MAXIMUM + 1 || image.height > WORD_MAXIMUM + 1
        || image.width * image.height !== image.indices.length || image.opacity.length !== image.indices.length) {
        throw new RangeError('VFX 변환 이미지 용량 오류');
    }
    const rect = frame.rect;
    if (frame.special || rect.right - rect.left + 1 !== image.width || rect.bottom - rect.top + 1 !== image.height) {
        throw new RangeError('VFX 변환 실제 프레임 영역 오류');
    }
    // 원본의 조기 일반 출력 분기에서는 255 도 불투명 색으로 유지한다.
    if (!translated && scaleX === FIXED_UNIT && scaleY === FIXED_UNIT) {
        drawIndexedImage(pixels, pitch, height, image, add(position.x, rect.left), add(position.y, rect.top), clip, colors);
        return;
    }
    if (!isWord(rect.left) || !isWord(rect.right) || !isWord(rect.top) || !isWord(rect.bottom)) {
        throw new RangeError('VFX 변환 프레임 WORD 좌표 범위 초과');
    }
    const firstX = add(position.x, scaleCoordinate(rect.left, scaleX));
    const lastX = add(position.x, scaleCoordinate(rect.right, scaleX));
    const firstY = add(position.y, scaleCoordinate(rect.top, scaleY));
    const lastY = add(position.y, scaleCoordinate(rect.bottom, scaleY));
    if (!isWord(firstX) || !isWord(lastX) || !isWord(firstY) || !isWord(lastY)) {
        throw new RangeError('VFX 변환 화면 WORD 좌표 범위 초과');
    }
    // 최저/최고 y 가 같으면 원본은 행을 그리지 않는다. 한 행 그림의 비단위 변환도 같다.
    if (firstY === lastY) return;
    const left = Math.min(firstX, lastX);
    const right = Math.max(firstX, lastX);
    const top = Math.min(firstY, lastY);
    const bottom = Math.max(firstY, lastY);
    const spanX = right - left;
    const spanY = bottom - top;
    if (spanX > WORD_MAXIMUM || spanY > WORD_MAXIMUM) throw new RangeError('VFX 변환 고정소수점 간격 범위 초과');
    const visible = clipRect({ left, top, right: right + 1, bottom: bottom + 1 }, clipRect(clip, { left: 0, top: 0, right: pitch, bottom: height }));
    if (visible.left >= visible.right || visible.top >= visible.bottom) return;
    const sourceX = image.width - 1;
    const sourceY = image.height - 1;
    const stepX = spanX ? textureStep(lastX > firstX ? sourceX : -sourceX, spanX) : 0;
    const stepY = textureStep(lastY > firstY ? sourceY : -sourceY, spanY);
    // 동일 x 의 두 변 중 오른쪽 변을 먼저 선택하는 <= 비교와 최저 y 의 마지막 꼭짓점을 유지한다.
    const startX = spanX ? (lastX > firstX ? 0 : sourceX) : (lastY > firstY ? sourceX : 0);
    const firstU = advance(textureStart(startX), stepX, visible.left - left);
    let v = advance(textureStart(lastY > firstY ? 0 : sourceY), stepY, visible.top - top);
    const { indices, opacity } = image;
    // 마지막 y 도 포함한다. 잘린 행은 원본 기울기를 누적한 위치에서 시작한다.
    for (let y = visible.top; y < visible.bottom; y += 1) {
        const sourceRow = (v >>> 16) * image.width;
        const targetRow = y * pitch;
        let u = firstU;
        // 마지막 x 도 포함한다. 일반 소스의 결과 255 와 translated 마스크의 255 는 각각 투명이다.
        for (let x = visible.left; x < visible.right; x += 1) {
            const input = sourceRow + (u >>> 16);
            if (translated && opacity[input] && indices[input] !== SCRATCH_TRANSPARENT) {
                // 0040351d 는 결과 255 도 불투명 대상으로 저장한다.
                pixels[targetRow + x] = colors[pixels[targetRow + x]];
            } else if (!translated && opacity[input]) {
                const ink = colors ? colors[indices[input]] : indices[input];
                if (ink !== SCRATCH_TRANSPARENT) pixels[targetRow + x] = ink;
            }
            u = advance(u, stepX, 1);
        }
        v = advance(v, stepY, 1);
    }
}

// 일반 VFX 변환 출력이다. 기존 단위 배율의 직접 출력과 비단위 임시 255 표면 동작을 유지한다. position = { x, y }, scaleX/scaleY = signed 16.16, colors 는 선택 색 변환표(없으면 null).
// 사용: drawScaledVfxImage(surface.pixels, surface.width, surface.height, db.decode(0, 0), db.frame(0, 0), { x: 12, y: 10 }, 0x8000, 0x8000, clip). (2026-11-13 WB04-E378)
export function drawScaledVfxImage(pixels, pitch, height, image, frame, position, scaleX, scaleY, clip, colors = null) {
    drawVfxImage(pixels, pitch, height, image, frame, position, scaleX, scaleY, clip, colors, false);
}

// 현재 대상 픽셀을 실제 표로 변환하는 출력이다(0040351d). 소스 그림은 255 투명 마스크로만 쓰고 colors 는 필수(256개)다. 원문의 마스크/기하 순서를 함께 사용한다.
// 사용: drawTranslatedVfxImage(surface.pixels, surface.width, surface.height, image, frame, { x, y }, 0x10000, 0x10000, clip, colorMap). (2026-11-13 WB04-E378)
export function drawTranslatedVfxImage(pixels, pitch, height, image, frame, position, scaleX, scaleY, clip, colors) {
    if (!colors) throw new TypeError('VFX 대상 색 변환에는 색 변환표가 필요합니다.');
    drawVfxImage(pixels, pitch, height, image, frame, position, scaleX, scaleY, clip, colors, true);
}
