// 원본 497960 의 현재 view/snapshot·dirty·배율 적용 본체를 옮긴다(cpppj client/RawCameraCommit.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 목표(target)의 배율이 현재 배율과 다르면 배율 분기(칸 크기·Q16 float 갱신→두 setter→두 번 Gump 재그리기→worldRedraw=1)만 실행하고, 그 밖에는 이동 분기(커서 잠금→화면 이동 세 조각→시차/DIB 배경 이동→
// 두 setter→마우스 갱신→기존 변경 표를 이동량만큼 옮김→노출 띠 추가→재그리기 요청)를 실행한다. 모든 외부 효과 뒤에는 현재 저장소/전역을 다시 읽는다(콜백이 목표/viewport/표를 바꿀 수 있다).
// Present 제한과 실제 Frame 호출 위치는 호출자가 소유한다. 사용: commitCameraTarget({ renderer, snapshot, target, redraw, worldRedraw, dirty }, globals, hooks). (2026-11-15 WB04-E463)

// 선택된 외부 경계만 요구한다(C++ std::logic_error → Error). 사용하지 않는 분기의 장치는 비어 있어도 된다.
function need(hook) {
    if (typeof hook !== 'function') throw new Error('카메라 적용 현재 장치 미연결');
    return hook;
}
// 원문 ADD/SUB/NEG 의 하위 32비트이다.
const add = (a, b) => (a + b) | 0;
const sub = (a, b) => (a - b) | 0;
// 원문 IDIV 의 두 오류(0 으로 나눔·INT_MIN/-1 넘침)를 C++ 미정의 동작 대신 선택된 위치에서 거부한다(std::domain_error → RangeError). 몫은 0 쪽으로 절삭한다.
function divide(value, denominator) {
    if (!denominator || (value === -0x80000000 && denominator === -1)) throw new RangeError('카메라 시차 원문 IDIV 오류');
    return Math.trunc(value / denominator) | 0;
}
// 원문 IMUL 하위 32비트 뒤 signed SAR 16 이다(Q16 배율을 칸 크기에 적용).
const scale = (base, zoom) => Math.imul(base, zoom >>> 0) >> 16;
// 원문 FILD32/FSTP32 저장(정수 → 단정도 비트)이다.
function floatBits(value) { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); }
// 현재 Q16 배율을 단정도로 바꿔 2^-16 을 곱한 비트이다(원문 FILD 뒤 FMUL 0x1p-16).
const zoomBits = (zoom) => floatBits(Math.fround(Math.fround(zoom | 0) * 2 ** -16));

// 16DWORD 상태(CameraView.data)의 칸별 위치이다: 칸 크기 +08/+0c, 기본 칸 크기 +10/+14, 배율 +18, 현재 칸 크기 float +24/+28, 배율 float +34.
const CELL = [8, 12]; const BASE = [16, 20]; const ZOOM = 24; const CELL_FLOAT = [36, 40]; const ZOOM_FLOAT = 52;
const geti = (view, offset) => view.data.getInt32(offset, true);
const seti = (view, offset, value) => view.data.setInt32(offset, value, true);
const setu = (view, offset, value) => view.data.setUint32(offset, value >>> 0, true);

// 원문의 두 노출 띠를 계산한다. 음수 길이일 때 끝 좌표 자체를 NEG 하는 순서도 보존한다. emit(rect)는 가로 띠, 세로 띠 순으로 호출한다.
function exposed(viewport, dx, dy, emit) {
    const left = dx > 0 ? add(sub(viewport.x, dx), viewport.width) : viewport.x;
    let right = dx > 0 ? add(viewport.x, viewport.width) : sub(viewport.x, dx);
    const top = dy > 0 ? add(sub(viewport.y, dy), viewport.height) : viewport.y;
    let bottom = dy > 0 ? add(viewport.y, viewport.height) : add(viewport.y, dy);
    const horizontal = sub(right, left); const vertical = sub(bottom, top);
    if (horizontal) { if (horizontal < 0) right = sub(0, right); emit({ left, top: viewport.y, right, bottom: add(viewport.y, viewport.height) }); }
    if (vertical) { if (vertical < 0) bottom = sub(0, bottom); emit({ left: viewport.x, top, right: add(viewport.x, viewport.width), bottom }); }
}

// 배율 분기에서 한 view 의 칸 크기/Q16 float/배율을 갱신한다. 칸 크기 X 와 Y 사이에 배율 저장이 끼는 원문 순서는 renderer 와 snapshot 이 다르다(아래 호출부가 순서를 지킨다).
function writeCell(view, axis, zoom) { seti(view, CELL[axis], scale(geti(view, BASE[axis]), zoom)); }
function writeCellFloat(view, axis) { setu(view, CELL_FLOAT[axis], floatBits(geti(view, CELL[axis]))); }

// 497960 본체: state = { renderer(Renderer 앞 64BYTE CameraView), snapshot(전역 0059a900 CameraView), target({limits, pixels, zoom}), redraw({value}, Renderer+5c 전체 출력 요청), worldRedraw({value}, 0059a8b0),
// dirty([{rect:{left,top,right,bottom}, flags}], 같은 현재 변경 표) }, globals = { screenFlags(시차 16/DIB 2 비트), denominator(시차 분모), width, height(논리 화면), viewport({x,y,width,height}), fullViewport({left,top,right,bottom}),
// mouse({x,y}) }(외부 효과 뒤에도 같은 객체의 현재 속성을 다시 읽는다), hooks = { lockCursor(0), unlockCursor(), redrawRoots(), scroll(x,y,w,h,dx,dy), scrollBackground(x,y,w,h,dx,dy), addDirty(rect, flags),
// addBackgroundDirty(rect), updateMouse(point), assertion(문자열, 줄) }(선택된 경계만 요구).
export function commitCameraTarget(state, globals, hooks) {
    const view = state.renderer; const snapshot = state.snapshot; const target = state.target;
    // 배율이 다르면 배율 분기만 실행한다. 첫 재그리기 뒤에 목표 배율을 다시 읽는다.
    if ((target.zoom >>> 0) !== view.data.getUint32(ZOOM, true)) {
        need(hooks.redrawRoots)(); const zoom = target.zoom >>> 0;
        writeCell(view, 0, zoom); setu(view, ZOOM_FLOAT, zoomBits(zoom)); writeCellFloat(view, 0); setu(view, ZOOM, zoom); writeCell(view, 1, zoom); writeCellFloat(view, 1);
        setu(snapshot, ZOOM_FLOAT, zoomBits(zoom)); writeCell(snapshot, 0, zoom); writeCell(snapshot, 1, zoom); setu(snapshot, ZOOM, zoom); writeCellFloat(snapshot, 0); writeCellFloat(snapshot, 1);
        view.setPixels(target.pixels[0], target.pixels[1], hooks.assertion); snapshot.setPixels(target.pixels[0], target.pixels[1], hooks.assertion);
        need(hooks.redrawRoots)(); state.worldRedraw.value = 1; return;
    }
    // 위치가 다르면 커서를 잠그고 현재 Gump 를 먼저 다시 그린다. 이동량은 이 효과 뒤의 목표/현재 값에서 구한다.
    if (target.pixels[0] !== geti(view, 28) || target.pixels[1] !== geti(view, 32)) { need(hooks.lockCursor)(0); need(hooks.redrawRoots)(); }
    const dy = sub(target.pixels[1], geti(view, 32)); const dx = sub(target.pixels[0], geti(view, 28));
    need(hooks.scroll)(globals.viewport.x, globals.viewport.y, globals.viewport.width, globals.viewport.height, dx, dy);
    if (globals.viewport.x > 0) need(hooks.scroll)(0, 0, globals.viewport.x, globals.height, 0, 0);
    if (globals.viewport.height < globals.height) need(hooks.scroll)(globals.viewport.x, globals.viewport.height, sub(globals.width, globals.viewport.x), sub(globals.height, globals.viewport.height), 0, 0);
    if (globals.screenFlags & 16) {
        if (!(globals.screenFlags & 2)) {
            // 시차 배경: 현재/목표 원점을 분모로 나눈 차이만큼 배경을 이동하고 두 노출 띠를 배경 변경 영역으로 더한다.
            const oldX = divide(geti(view, 28), globals.denominator); const newX = divide(target.pixels[0], globals.denominator);
            const oldY = divide(geti(view, 32), globals.denominator); const newY = divide(target.pixels[1], globals.denominator); const sx = sub(newX, oldX); const sy = sub(newY, oldY);
            need(hooks.scrollBackground)(globals.viewport.x, globals.viewport.y, globals.viewport.width, globals.viewport.height, sx, sy);
            exposed(globals.viewport, sx, sy, (rect) => need(hooks.addBackgroundDirty)(rect));
        } else if (dx || dy) need(hooks.addDirty)(globals.fullViewport, 0);
    }
    view.setPixels(target.pixels[0], target.pixels[1], hooks.assertion); snapshot.setPixels(target.pixels[0], target.pixels[1], hooks.assertion);
    if (!dx && !dy) return;
    need(hooks.updateMouse)({ x: globals.mouse.x, y: globals.mouse.y });
    // 마우스 효과 뒤의 현재 표를 매 반복의 현재 개수로 순회한다. 플래그 1(버려짐)은 건너뛰고 2 는 이동 전후의 합집합, 그 밖은 이동한 사각형이다.
    for (let index = 0; index < state.dirty.length; index += 1) {
        const entry = state.dirty[index]; if (entry.flags & 1) continue; const old = entry.rect;
        const shifted = { left: sub(old.left, dx), top: sub(old.top, dy), right: sub(old.right, dx), bottom: sub(old.bottom, dy) };
        if (entry.flags & 2) entry.rect = { left: Math.min(old.left, shifted.left), top: Math.min(old.top, shifted.top), right: Math.max(old.right, shifted.right), bottom: Math.max(old.bottom, shifted.bottom) };
        else entry.rect = shifted;
    }
    exposed(globals.viewport, dx, dy, (rect) => need(hooks.addDirty)(rect, 0));
    need(hooks.unlockCursor)(); need(hooks.redrawRoots)(); state.redraw.value = 1;
}
