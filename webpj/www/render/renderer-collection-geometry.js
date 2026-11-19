// Renderer의 dirty·지도 화면 사각형을 같은 카메라 snapshot의 수집 월드 범위로 바꾼다.
// 사용: rendererCollectionViewport(dirty, map, camera), rendererCollectionRowLimit(width, height).

// 카메라의 원본 역투영은 float32 월드 원점과 signed 픽셀 덧셈을 그대로 사용한다.
export function rendererWorldRect(rect, camera) {
    if (!rect || typeof camera?.screenToWorld !== 'function') throw new TypeError('Renderer 수집의 화면·카메라 수명 누락');
    const first = camera.screenToWorld(rect.left, rect.top), last = camera.screenToWorld(rect.right, rect.bottom);
    return { left: first.x, top: first.y, right: last.x, bottom: last.y };
}

// 원본은 화면에서 먼저 잘라내지 않고 두 월드 사각형을 각각 변환한 뒤 좌표별로 제한한다.
export function rendererCollectionViewport(dirty, map, camera) {
    const rect = rendererWorldRect(dirty, camera), bounds = rendererWorldRect(map, camera);
    // 4970c0의 max(low, min(value, high)) 순서를 보존한다.
    const clamp = (value, low, high) => Math.max(low, Math.min(value, high));
    return { left: clamp(rect.left, bounds.left, bounds.right), top: clamp(rect.top, bounds.top, bounds.bottom),
        right: clamp(rect.right, bounds.left, bounds.right), bottom: clamp(rect.bottom, bounds.top, bounds.bottom) };
}

// 화면 폭/16과 높이+10의 DWORD 곱을 signed32로 읽고 11로 0 방향 나눈다.
export function rendererCollectionRowLimit(width, height) {
    if (!Number.isInteger(width) || !Number.isInteger(height) || width < -0x80000000 || width > 0x7fffffff || height < -0x80000000 || height > 0x7fffffff) throw new RangeError('Renderer 수집의 signed 화면 크기 오류');
    return Math.trunc(Math.imul(Math.trunc(width / 16), (height + 10) | 0) / 11) | 0;
}
