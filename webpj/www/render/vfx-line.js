// 원본 VFX 일반 선(단색 8비트 경로)을 8비트 표면에 그리는 모듈이다. 스크롤바 화살표의 삼각형(대각선 두 개)이 쓴다.
// 수평·수직선은 양 끝을 포함해 채우고, 대각선은 주축을 한 칸씩 가며 0.32 고정소수 분수 위상(시작 0.5)이 넘칠 때 부축을 한 칸 옮긴다.
// 기울기 = (짧은 변 << 32) / 긴 변(두 변이 같으면 0xffffffff), 위상은 매 걸음 기울기를 더하며 32비트를 넘으면 올림(carry)이다.
// 원본은 클립 경계와 교차하는 선을 잘라 그릴 때도 원래 시작점 기준의 위상을 유지하므로, 여기서는 자르지 않고 전체 선을 걸으며 클립 밖 픽셀만 건너뛴다(결과 픽셀은 같다).
// 원본 주소: 004011f1(VFX 선 mode 0). 복원 기준: cpppj VFXLine.cpp(10.78 경로).
// (2026-10-30 WB03-G: 스크롤바 화살표 삼각형을 원본과 같은 픽셀로 그리기 위해 추가)
import { clipRect } from './indexed-surface.js';

// 0.32 고정소수의 절반(0.5)이다. 선 위상의 시작값이다. (2026-10-30 WB03-G)
const HALF = 0x80000000;

// 32비트 부호 없는 상한 + 1 이다. 위상 덧셈의 넘침(carry) 판정에 쓴다. (2026-10-30 WB03-G)
const WRAP = 0x100000000;

// 선 하나를 8비트 표면에 그린다. first·last 는 { x, y } 이고 color 는 색 번호(BYTE 로 잘림)이며 clip 은 출력 제한 사각형(생략 시 표면 전체, right/bottom 은 포함하지 않음)이다.
// 수평·수직선은 양 끝을 포함해 클립과 교차한 부분만 채운다. 그 밖의 선은 분수 위상 순회로 픽셀을 찍는다.
// 사용: drawVfxLine(surface, { x: 10, y: 20 }, { x: 14, y: 16 }, 1) (2026-10-30 WB03-G)
export function drawVfxLine(surface, first, last, color, clip = surface.bounds) {
    const area = clipRect(clip, surface.bounds);
    if (area.left >= area.right || area.top >= area.bottom) return;
    const ink = color & 255;
    const dx = last.x - first.x;
    const dy = last.y - first.y;
    const pixels = surface.pixels;
    const width = surface.width;
    // 수평·수직선(한 점 포함)은 양 끝을 포함해 클립과 교차한 부분을 채운다.
    if (dx === 0 || dy === 0) {
        const left = Math.max(area.left, Math.min(first.x, last.x));
        const right = Math.min(area.right, Math.max(first.x, last.x) + 1);
        const top = Math.max(area.top, Math.min(first.y, last.y));
        const bottom = Math.min(area.bottom, Math.max(first.y, last.y) + 1);
        if (left >= right || top >= bottom) return;
        // 선이 덮는 각 행을 색 번호로 채운다.
        for (let y = top; y < bottom; y += 1) pixels.fill(ink, y * width + left, y * width + right);
        return;
    }
    const ax = Math.abs(dx);
    const ay = Math.abs(dy);
    const majorX = ax >= ay;
    // 기울기(0.32 분수)이다. 두 변이 같으면 항상 부축도 움직이도록 최댓값을 쓴다.
    const slope = ax === ay ? 0xffffffff : Number((BigInt(Math.min(ax, ay)) << 32n) / BigInt(Math.max(ax, ay)));
    const stepX = dx < 0 ? -1 : 1;
    const stepY = dy < 0 ? -1 : 1;
    const count = Math.max(ax, ay) + 1;
    let phase = HALF;
    let x = first.x;
    let y = first.y;
    // 주축 픽셀 수만큼 걸으며 클립 안의 픽셀만 찍는다.
    for (let index = 0; index < count; index += 1) {
        if (x >= area.left && x < area.right && y >= area.top && y < area.bottom) pixels[y * width + x] = ink;
        const next = phase + slope;
        const carry = next >= WRAP;
        phase = carry ? next - WRAP : next;
        if (majorX) {
            x += stepX;
            if (ax === ay || carry) y += stepY;
        } else {
            y += stepY;
            if (carry) x += stepX;
        }
    }
}
