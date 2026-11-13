// 원본 VFX 일반 선(단색 8비트 경로)을 8비트 표면에 그리는 모듈이다(cpppj `client/VFXLine.cpp/.h`, 원문 004011f1 mode 0 / CD 00464bd1, 10.78 전용). 스크롤바 화살표 삼각형·VFX 타원의 퇴화 선·불꽃 꼬리 선이 쓴다.
//  - drawVfxLinePixels(pixels, pitch, height, clip, first, last, color): 원문 그대로의 클립 선이다. clip 은 반개방 사각형으로 표면 전체와 교차하고, 색은 하위 BYTE 만 쓴다.
//    수평·수직선(한 점 포함)은 양 끝을 포함해 클립과 교차한 부분을 채운다. 그 밖의 선은 기울기 = (짧은 변 << 32) / 긴 변(두 변이 같으면 0xffffffff)의 0.32 분수와, 시작 위상 0.5(0x80000000)에서
//    걸음마다 기울기를 더해 32비트를 넘으면(carry) 부축을 한 칸 옮기는 순회로 그린다.
//    클립 경계와 교차하는 선은 **원래 시작점 기준**으로 양 끝을 따로 잘라낸다(Cohen-Sutherland 식 분류: 왼쪽 8·오른쪽 4·위 2·아래 1, 우선 순서 왼쪽→오른쪽→위→아래, 경계는 포함 범위).
//    주축 경계는 `high32(거리 × 기울기) + (low32 ≥ 0.5)` 의 반올림 곱, 부축 경계는 `((거리 − 시작이면 1) << 32 | 0.5) / 기울기` 의 DIV 역산이라 시작/끝이 서로 다르게 역산된다(시작은 +1, 나머지가 0 이면 −1).
//    잘린 시작에서의 위상은 `거리 × 기울기 + 0.5` 의 하위 32비트로 다시 계산해 전체 선을 걸을 때와 같은 분수 위상을 유지한다.
//    32비트 몫을 넘는 DIV 와 32번 안에 수렴하지 않는 클립, 31비트를 넘는 선 길이는 호스트 입력으로 거부한다(RangeError/Error).
//  - drawVfxLine(surface, first, last, color, clip): { pixels, width, height, bounds } 표면용 얇은 포장이다(생략 시 clip 은 표면 전체).
// 다른 선 알고리즘(Bresenham 등)으로 원본 선을 대체하지 않는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-10-30 WB03-G: 스크롤바 화살표 삼각형을 원본과 같은 픽셀로 그리기 위해 추가; 2026-11-13 WB04-E379: 전체 선 걷기를 원문 클립 알고리즘(Cut/Inverse/DIV 범위)으로 교체하고 pixels 직접 진입점 추가)
import { clipRect } from './indexed-surface.js';

// 클립 분류 비트의 실제 우선 순서는 왼쪽→오른쪽→위→아래다. 반 픽셀은 0.32 분수의 bit 31 이다. (2026-11-13 WB04-E379)
const LEFT = 8;
const RIGHT = 4;
const TOP = 2;
const BOTTOM = 1;
const HALF = 0x80000000;
const HALF_BIG = 0x80000000n;
// 32비트 부호 없는 상한 + 1 과 상한이다. 위상 덧셈의 넘침(carry) 판정과 DIV 몫 범위에 쓴다. (2026-10-30 WB03-G, 2026-11-13 WB04-E379 정리)
const WRAP = 0x100000000;
const MASK32 = 0xffffffffn;
// signed 32비트 최댓값이다. 선 길이(절댓값)가 넘으면 호스트 입력으로 거부한다. (2026-11-13 WB04-E379)
const INT32_MAX = 0x7fffffff;
// 정상 네 경계 교차 밖에서 수렴하지 않는 호스트 입력을 제한하는 반복 상한이다. (2026-11-13 WB04-E379)
const CLIP_ITERATIONS = 32;

// 원본 DWORD 좌표 덧셈(부호 있는 좌표에 부호 없는 값을 더함)을 signed overflow 없이 32비트로 감싼다. (2026-11-13 WB04-E379)
const add = (a, b) => (a + b) | 0;

// 점이 포함 경계 bounds 밖에 있는 방향의 분류 비트이다. 반 평면 비교는 현재 포함 경계를 쓴다. (2026-11-13 WB04-E379)
const outcode = (point, bounds) => (point.x < bounds.left ? LEFT : 0) | (point.x > bounds.right ? RIGHT : 0) | (point.y < bounds.top ? TOP : 0) | (point.y > bounds.bottom ? BOTTOM : 0);

// 클립 곱의 high DWORD 와 low DWORD bit 31 을 더해 반 픽셀을 반올림한다. 입력은 모두 부호 없는 32비트다. (2026-11-13 WB04-E379)
function roundedProduct(distance, slope) {
    const product = BigInt(distance) * BigInt(slope);
    return Number(BigInt.asUintN(32, (product >> 32n) + ((product & MASK32) >= HALF_BIG ? 1n : 0n)));
}

// 부축 경계의 주축 거리를 DIV 로 역산한다. 시작 끝점은 분자에 거리 − 1(부호 없는 32비트 감김)을 쓰고 몫에 +1, 나머지가 0 이면 −1 한다. 몫이 32비트를 넘으면 원본 DIV 오류라 RangeError 다. (2026-11-13 WB04-E379)
function inverse(distance, slope, start) {
    const numerator = (BigInt(start ? BigInt.asUintN(32, BigInt(distance) - 1n) : BigInt(distance)) << 32n) | HALF_BIG;
    const quotient = numerator / BigInt(slope);
    const remainder = numerator % BigInt(slope);
    if (quotient > MASK32) throw new RangeError('VFX 선 클립 DIV 범위 초과');
    return Number(BigInt.asUintN(32, quotient + (start ? 1n : 0n) - (remainder === 0n ? 1n : 0n)));
}

// 한 끝점을 원래 시작점 기준으로 교차한다. 현재 끝점에서 다시 기울기를 계산하지 않는다. line = { first, last, origin, bounds, slope, majorX, negativeSlope }. (2026-11-13 WB04-E379)
function cut(line, code, start) {
    const xAxis = (code & (LEFT | RIGHT)) !== 0;
    const boundary = xAxis ? (code & LEFT ? line.bounds.left : line.bounds.right) : (code & TOP ? line.bounds.top : line.bounds.bottom);
    const origin = xAxis ? line.origin.x : line.origin.y;
    const delta = boundary - origin;
    // 거리는 부호 없는 32비트로 자른다(원문 static_cast<uint32_t>).
    const distance = Number(BigInt.asUintN(32, BigInt(delta < 0 ? -delta : delta)));
    const offset = xAxis === line.majorX ? roundedProduct(distance, line.slope) : inverse(distance, line.slope, start);
    const negative = (delta < 0) !== line.negativeSlope;
    // 부호 반전은 부호 없는 32비트 감김(0U − offset)이다.
    const other = add(xAxis ? line.origin.y : line.origin.x, negative ? (0 - offset) >>> 0 : offset);
    if (start) line.first = xAxis ? { x: boundary, y: other } : { x: other, y: boundary };
    else line.last = xAxis ? { x: boundary, y: other } : { x: other, y: boundary };
}

// 두 끝점의 바깥 분류를 반복한다. 정상 원본 좌표 밖에서 수렴하지 않는 입력만 거부한다. 선이 보이면 true, 완전히 밖이면 false 다. (2026-11-13 WB04-E379)
function clipLine(line) {
    // 정상 좌표의 교차/거부 순서를 유지하고 비정상 수렴 실패만 상한에서 거부한다.
    for (let count = 0; count < CLIP_ITERATIONS; count += 1) {
        const first = outcode(line.first, line.bounds);
        const last = outcode(line.last, line.bounds);
        if (!(first | last)) return true;
        if (first & last) return false;
        cut(line, first || last, first !== 0);
    }
    throw new Error('VFX 선 클립 수렴 실패');
}

// 선 하나를 8비트 표면 버퍼(pixels, 한 줄 pitch, 높이 height)에 그린다. first·last 는 { x, y }(32비트 정수), color 는 색 번호(BYTE 로 잘림), clip 은 반개방 출력 제한 사각형이다.
// 사용: drawVfxLinePixels(surface.pixels, surface.width, surface.height, clip, { x: 10, y: 20 }, { x: 14, y: 16 }, 1). 원본 pane 원점의 감산/재가산이 상쇄된 절대 좌표를 쓴다. (2026-11-13 WB04-E379)
export function drawVfxLinePixels(pixels, pitch, height, clip, first, last, color) {
    if (pitch < 1 || height < 1 || pitch * height > pixels.length) throw new RangeError('VFX 선 표면 용량 오류');
    const area = clipRect(clip, { left: 0, top: 0, right: pitch, bottom: height });
    if (area.left >= area.right || area.top >= area.bottom) return;
    // 분류용 경계는 마지막 픽셀을 포함하는 닫힌 구간이다.
    const bounds = { left: area.left, top: area.top, right: area.right - 1, bottom: area.bottom - 1 };
    const dx = last.x - first.x;
    const dy = last.y - first.y;
    const ax = Math.abs(dx);
    const ay = Math.abs(dy);
    const ink = color & 0xff;
    // 수평·수직선(한 점 포함)은 양 끝을 포함해 현재 클립과 교차한 부분을 채운다.
    if (dx === 0 || dy === 0) {
        const left = Math.max(area.left, Math.min(first.x, last.x));
        const right = Math.min(area.right, Math.max(first.x, last.x) + 1);
        const top = Math.max(area.top, Math.min(first.y, last.y));
        const bottom = Math.min(area.bottom, Math.max(first.y, last.y) + 1);
        if (left >= right || top >= bottom) return;
        // 선이 덮는 각 행을 색 번호로 채운다.
        for (let y = top; y < bottom; y += 1) pixels.fill(ink, y * pitch + left, y * pitch + right);
        return;
    }
    if (ax > INT32_MAX || ay > INT32_MAX) throw new RangeError('VFX 선 signed 거리 한도 초과');
    // 기울기(0.32 분수)이다. 두 변이 같으면 항상 부축도 움직이도록 최댓값을 쓴다.
    const slope = ax === ay ? 0xffffffff : Number(BigInt.asUintN(32, (BigInt(Math.min(ax, ay)) << 32n) / BigInt(Math.max(ax, ay))));
    const majorX = ax >= ay;
    const line = { first: { x: first.x, y: first.y }, last: { x: last.x, y: last.y }, origin: first, bounds, slope, majorX, negativeSlope: (dx < 0) !== (dy < 0) };
    if (!clipLine(line)) return;
    const startMajor = majorX ? line.first.x : line.first.y;
    const endMajor = majorX ? line.last.x : line.last.y;
    const originMajor = majorX ? first.x : first.y;
    const distance = startMajor - originMajor;
    // 잘린 시작의 분수 위상은 원래 시작점에서의 주축 거리로 다시 계산한다(하위 32비트).
    let phase = (Number(BigInt.asUintN(32, BigInt(Math.abs(distance)) * BigInt(slope))) + HALF) >>> 0;
    let x = line.first.x;
    let y = line.first.y;
    const stepX = dx < 0 ? -1 : 1;
    const stepY = dy < 0 ? -1 : 1;
    const count = Math.abs(endMajor - startMajor) + 1;
    // 원래 방향과 분수 위상을 유지하며 잘린 시작/끝 사이의 실제 주축 픽셀만 방문한다.
    for (let index = 0; index < count; index += 1) {
        if (!outcode({ x, y }, bounds)) pixels[y * pitch + x] = ink;
        const next = phase + slope;
        const carry = next >= WRAP;
        phase = carry ? next - WRAP : next;
        if (majorX) {
            x = add(x, stepX);
            if (ax === ay || carry) y = add(y, stepY);
        } else {
            y = add(y, stepY);
            if (carry) x = add(x, stepX);
        }
    }
}

// 선 하나를 { pixels, width, height, bounds } 표면에 그린다. clip 은 출력 제한 사각형(생략 시 표면 전체, right/bottom 은 포함하지 않음)이다.
// 사용: drawVfxLine(surface, { x: 10, y: 20 }, { x: 14, y: 16 }, 1) (2026-10-30 WB03-G; 2026-11-13 WB04-E379: drawVfxLinePixels 로 위임)
export function drawVfxLine(surface, first, last, color, clip = surface.bounds) {
    drawVfxLinePixels(surface.pixels, surface.width, surface.height, clip, first, last, color);
}
