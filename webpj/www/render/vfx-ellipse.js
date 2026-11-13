// 원본 VFX 타원 윤곽(PATCH 00405071 / CD 00468a51 단색 경로)을 8비트 표면에 그리는 모듈이다(cpppj `client/VFXEllipse.cpp/.h`, 10.78 전용). 두 정수 구간·클리핑·퇴화 선을 복원한다.
//  - drawVfxEllipse(pixels, pitch, height, clip, center, radiusX, radiusY, color): 중심은 절대 좌표(pane 원점 감산/재가산이 상쇄됨), clip 은 반개방 사각형(표면 전체와 교차), 색은 하위 BYTE 만 쓴다.
//    하나의 반경이 0 이면 원본 VFX 선 경로(render/vfx-line.js)로 (중심 − 반경) → (중심 + 반경) 선을 그린다. signed 음수 반경도 원문의 정수 분기(32비트 DWORD 감김)로 처리한다.
//    첫 구간은 `Signed(dx − dy) < 0` 인 동안 네 대칭점(중복 쓰기와 순서 유지: +x+y, +x−y, −x+y, −x−y)을 찍고 결정 변수를 갱신하며, 사이에서 `SAR(SAR(a − b) + (a − b) − dx − dy)` 를 더한 뒤
//    두 번째 구간은 do/while 로 한 번은 실행한 뒤 `y ≥ 0`(signed)일 때까지 반복한다. 모든 중간값은 32비트 감김이며 산술 시프트는 부호 비트를 보존한다.
//    호스트의 비수렴/과도한 입력은 100만 번 반복 상한에서 거부한다(정상 작은 표시 반경과 구별).
// 다른 기하 라이브러리로 원본 타원 윤곽을 대체하지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E377: 5-A VFXEllipse 이식으로 추가, 기준선 뒤 cpppj 변경 없음)
import { clipRect } from './indexed-surface.js';
import { drawVfxLine } from './vfx-line.js';

// 비수렴/과도한 호스트 입력의 반복 상한과 부호 비트이다. (2026-11-13 WB04-E377)
const ITERATION_LIMIT = 1000000;
const SIGN = 0x80000000;

// 원문 CMP/JL 의 signed DWORD 해석이다. (2026-11-13 WB04-E377)
const signed = (value) => value | 0;

// 원문 SAR 1 을 호스트 오른쪽 시프트 차이 없이 보존한다(부호 비트 유지, 32비트 부호 없는 값을 받아 돌려준다). 시험이 직접 확인하도록 내보낸다. (2026-11-13 WB04-E377)
export const sar = (value) => ((value >>> 1) | (value & SIGN)) >>> 0;

// 8비트 표면(pixels 배열, 한 줄 pitch, 높이 height)에 단색 타원 윤곽을 그린다. center = { x, y }, clip = { left, top, right, bottom }.
// 사용: drawVfxEllipse(surface.pixels, surface.width, surface.height, clip, { x: 32, y: 24 }, 5, 3, 200). (2026-11-13 WB04-E377)
export function drawVfxEllipse(pixels, pitch, height, clip, center, radiusX, radiusY, color) {
    if (pitch < 1 || height < 1 || pitch * height > pixels.length) throw new RangeError('VFX 타원 표면 용량 오류');
    const area = clipRect(clip, { left: 0, top: 0, right: pitch, bottom: height });
    if (area.left >= area.right || area.top >= area.bottom) return;
    const cx = center.x >>> 0;
    const cy = center.y >>> 0;
    const rx = radiusX >>> 0;
    const ry = radiusY >>> 0;
    // 하나의 반경이 0 이면 기존 원문 VFX 선 경로를 사용한다(표면 객체는 같은 pixels 를 공유하는 얇은 보기).
    if ((radiusX | 0) === 0 || (radiusY | 0) === 0) {
        const surface = { pixels, width: pitch, height, bounds: { left: 0, top: 0, right: pitch, bottom: height } };
        drawVfxLine(surface, { x: signed(cx - rx), y: signed(cy - ry) }, { x: signed(cx + rx), y: signed(cy + ry) }, color, area);
        return;
    }
    // 원문 제곱/차분/결정 변수의 32비트 감김이다.
    const a = Math.imul(rx, rx) >>> 0;
    const b = Math.imul(ry, ry) >>> 0;
    let x = 0;
    let y = ry;
    let dx = 0;
    let dy = Math.imul(Math.imul(a, 2), ry) >>> 0;
    let decision = ((a >>> 2) + b - Math.imul(a, ry)) >>> 0;
    let iterations = 0;
    const ink = color & 0xff;
    // 현재 포함 경계 안의 한 BYTE 만 쓴다.
    const pixel = (px, py) => {
        const sx = signed(px);
        const sy = signed(py);
        if (sx >= area.left && sx < area.right && sy >= area.top && sy < area.bottom) pixels[sy * pitch + sx] = ink;
    };
    // 원문 네 대칭점의 중복 쓰기와 순서를 유지한다.
    const plot = () => {
        iterations += 1;
        if (iterations > ITERATION_LIMIT) throw new RangeError('VFX 타원 반복 범위 초과');
        pixel((cx + x) >>> 0, (cy + y) >>> 0);
        pixel((cx + x) >>> 0, (cy - y) >>> 0);
        pixel((cx - x) >>> 0, (cy + y) >>> 0);
        pixel((cx - x) >>> 0, (cy - y) >>> 0);
    };
    // 첫 구간은 signed 두 차분의 차가 음수일 때만 실행한다.
    while (signed((dx - dy) >>> 0) < 0) {
        plot();
        if (signed(decision) >= 0) {
            y = (y - 1) >>> 0;
            dy = (dy - Math.imul(a, 2)) >>> 0;
            decision = (decision - dy) >>> 0;
        }
        x = (x + 1) >>> 0;
        dx = (dx + Math.imul(b, 2)) >>> 0;
        decision = (decision + dx + b) >>> 0;
    }
    const difference = (a - b) >>> 0;
    decision = (decision + sar((sar(difference) + difference - dx - dy) >>> 0)) >>> 0;
    // 두 번째 구간은 음수 초기 반경이어도 한 번 실행한 뒤 현재 y 를 판단한다.
    do {
        plot();
        if (signed(decision) < 0) {
            x = (x + 1) >>> 0;
            dx = (dx + Math.imul(b, 2)) >>> 0;
            decision = (decision + dx) >>> 0;
        }
        y = (y - 1) >>> 0;
        dy = (dy - Math.imul(a, 2)) >>> 0;
        decision = (decision - ((dy - a) >>> 0)) >>> 0;
    } while (signed(y) >= 0);
}
