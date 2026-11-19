// 10.78 위치 음향의 화면 투영·가시 범위·좌우·감쇠를 원본 정수 폭으로 계산한다.
// 사용: soundToScreen(view, x, y) → soundOnScreen(view, point), soundPan(view, point, swap), soundVolume(view, point).
import { projectCameraCoordinates } from '../render/camera-view.js';

// signed DWORD 범위의 사각형 길이를 원본처럼 감은 뒤 0쪽으로 절삭해 반으로 나눈다.
function half(low, high) { const span = (high - low) | 0; return ((span - (span >> 31)) | 0) >> 1; }
// 원본 IDIV와 같은 0쪽 절삭이며 0 나누기와 INT_MIN/-1 넘침은 거부한다.
function divide(value, divisor) {
    if (!divisor || (value === -2147483648 && divisor === -1)) throw new RangeError('소리 위치 계산의 화면 폭이 너무 작습니다');
    return Math.trunc(value / divisor) | 0;
}
// DirectSound의 원본 좌우·음량 범위로 자른다.
function limit(value) { return Math.max(-10000, Math.min(10000, value)); }

// 단정도 월드 좌표를 16×11 화면 좌표로 투영하고 현재 카메라 원점을 뺀다.
export function soundToScreen(view, x, y) {
    const [px, py] = projectCameraCoordinates(view.cameraX, view.cameraY, x, y); return { x: px, y: py };
}
// 지도 영역 바깥 좌우 40·상하 30픽셀까지 원본처럼 끝점을 포함한다.
export function soundOnScreen(view, point) {
    return point.x >= ((view.left - 40) | 0) && point.x <= ((view.right + 40) | 0) && point.y >= ((view.top - 30) | 0) && point.y <= ((view.bottom + 30) | 0);
}
// 가운데에서의 부호 있는 거리 제곱을 화면 절반 폭의 제곱으로 나눈다. swap은 좌우를 바꾼다.
export function soundPan(view, point, swap) {
    const width = half(view.left, view.right); let distance = (point.x - view.left - width) | 0;
    if (swap) distance = (-distance) | 0;
    if (!distance) return 0;
    const scaled = Math.imul(Math.imul(distance, distance), distance < 0 ? -10000 : 10000);
    return limit(divide(scaled, Math.imul(width, width)));
}
// 화면 중심까지 두 축의 거리 제곱을 가로 절반 폭의 제곱으로 나누어 음량을 줄인다.
export function soundVolume(view, point) {
    const width = half(view.left, view.right), height = half(view.top, view.bottom);
    const dx = (point.x - view.left - width) | 0, dy = (point.y - height - view.top) | 0;
    const scaled = Math.imul((Math.imul(dy, dy) + Math.imul(dx, dx)) | 0, -1000);
    return limit(divide(scaled, Math.imul(width, width)));
}
