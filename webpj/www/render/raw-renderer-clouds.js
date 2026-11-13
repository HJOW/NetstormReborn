// 원본 Renderer 의 구름 사각형 저장·시차 갱신·배경 선택(10.78 4977f0/497860 과 499bfd..499cfe)을 옮긴 모듈이다(cpppj `client/RawRendererClouds.cpp/.h`, 10.78 전용). 일반 화면 dirty 표와 별도인 원문 구름 처리 본체이다.
//  - appendRendererCloudDirty(queue, rect, checking, hooks): 4977f0. 진단(checking)이 켜져 있고 현재 개수가 4 이상이면 진단 행(0x40c)을 알린 뒤 count 를 다시 읽는다. 4 이상이면 저장하지 않고 끝내며 음수 개수는 4칸 앞의
//    미제공 물리 영역 쓰기라 오류이다. 그 밖에는 사각형을 count 번째 칸에 복사하고 count 를 1 올린다(진단 효과가 count 를 바꿀 수 있어 부분 store 를 그대로 둔다).
//  - refreshRendererCloudBackground(queue, cameraX, cameraY, denominator, hooks): 497860. 현재 count(signed)만큼 칸을 돌며 매 칸 현재 카메라와 분모를 다시 읽어 Y→X 순서의 IDIV 몫으로 별도 시차 표면(draw alternate=true)을 요청하고,
//    정상 종료에만 count 를 0 으로 쓴다. 분모 0 이나 INT_MIN÷−1 은 실제 IDIV 오류(RendererCloudDivideError)이고 count 가 4 를 넘는 칸 읽기는 미제공 물리 영역(RangeError)으로 구별한다.
//  - drawRendererBackdrop(clipped, dirtyFlags, covered, globals, hooks): 499bfd..499cfe. debug 묶음 숨김이면 검정(black)으로 먼저 지우고, UI 전용(dirty 비트 4)이거나 배경이 덮여 있으면 끝낸다(효과 뒤 현재 관문을 다시 읽음).
//    구름 표시 비트(8)가 없으면 날씨색(tint)으로 지우고, 시차+플래그 조합(16·1 켜짐, 2 꺼짐)이면 색 255 로 지우며, 그 밖에는 시차 비트(16)가 있으면 카메라÷분모(Y→X)로, 없으면 카메라 원점 그대로 일반 구름(draw alternate=false)을 그린다.
//    사각형은 left/top 만 호출 직전의 현재 값으로 읽고 right/bottom 은 먼저 보존한 값을 쓴다(원문 EBP/EDI).
//  - queue = { rectangles: 네 칸 { left, top, right, bottom }, count: { value } }(0059a940..97f 와 0059a8d4 의 실제 저장소를 빌림). 현재 참조(dirtyFlags·covered·globals 칸·카메라·분모)는 `{ value }` 상자이다.
//  - 훅: assertion(행)·clear(rect, color)·draw(rect, x, y, alternate). 선택된 위치에서만 요구하며 없으면 Error 로 거부한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E383: 5-A 렌더러 프레임 앞단으로 추가, 기준선 뒤 cpppj 변경 없음)

// 구름 dirty 사각형 칸 수이다. (2026-11-13 WB04-E383)
export const RENDERER_CLOUD_RECT_COUNT = 4;
// 진단 단언이 알리는 4977f0 의 행 번호이다(cloudDirtyRectCount < NUM_CLOUD_DIRTY_RECT). (2026-11-13 WB04-E383)
const CLOUD_ASSERT_LINE = 0x40c;

// 실제 IDIV 오류(분모 0 또는 INT_MIN÷−1)이다. 물리 영역 밖 접근(RangeError)과 구별한다. (2026-11-13 WB04-E383)
export class RendererCloudDivideError extends RangeError {}

// 선택된 위치에서만 장치를 요구한다. (2026-11-13 WB04-E383)
function need(callback) {
    if (typeof callback !== 'function') throw new Error('Renderer 구름의 선택된 실제 장치 미연결');
    return callback;
}

// 원문 DWORD 비트를 signed 로 읽는다. (2026-11-13 WB04-E383)
const signed = (value) => value | 0;

// signed IDIV 의 몫이다(0 방향 절삭). 분모 0 과 INT_MIN÷−1 은 실제 오류로 거부한다. (2026-11-13 WB04-E383)
function divide(value, denominator) {
    if (denominator === 0 || (value === -0x80000000 && denominator === -1)) throw new RendererCloudDivideError('Renderer 구름의 원문 IDIV 오류');
    return Math.trunc(value / denominator) | 0;
}

// 호출 직전 left/top 은 현재 값이고 right/bottom 은 먼저 보존한 값인 사각형이다. (2026-11-13 WB04-E383)
const currentRect = (clipped, right, bottom) => ({ left: clipped.left, top: clipped.top, right, bottom });

// 구름 사각형 4칸에 하나를 저장한다(4칸 한계/진단 뒤 count 재조회·부분 store 보존). checking 은 호출 시점의 진단 DWORD 이다. (2026-11-13 WB04-E383)
export function appendRendererCloudDirty(queue, rect, checking, hooks = {}) {
    if ((checking >>> 0) && signed(queue.count.value) >= RENDERER_CLOUD_RECT_COUNT) need(hooks.assertion)(CLOUD_ASSERT_LINE);
    const index = signed(queue.count.value);
    if (index >= RENDERER_CLOUD_RECT_COUNT) return;
    if (index < 0) throw new RangeError('Renderer 구름 4칸 앞의 미제공 물리 영역 쓰기');
    queue.rectangles[index] = { left: rect.left, top: rect.top, right: rect.right, bottom: rect.bottom };
    queue.count.value = (queue.count.value + 1) >>> 0;
}

// 저장된 구름 사각형마다 시차 표면을 요청하고 count 를 0 으로 쓴다. 각 표시 효과 뒤 현재 count 를 다시 읽고 카메라·분모는 매 반복에서 조회한다. (2026-11-13 WB04-E383)
export function refreshRendererCloudBackground(queue, cameraX, cameraY, denominator, hooks = {}) {
    // signed 0/음수 개수는 표시 없이 마지막 count 0 만 수행한다.
    for (let index = 0; index < signed(queue.count.value); index += 1) {
        if (index >= RENDERER_CLOUD_RECT_COUNT) throw new RangeError('Renderer 구름 4칸 뒤의 미제공 물리 영역 읽기');
        const y = signed(cameraY.value);
        const divisor = signed(denominator.value);
        const dy = divide(y, divisor);
        const dx = divide(signed(cameraX.value), divisor);
        const slot = queue.rectangles[index];
        need(hooks.draw)({ left: slot.left, top: slot.top, right: slot.right, bottom: slot.bottom }, dx, dy, true);
    }
    queue.count.value = 0;
}

// 화면 배경 선택이다. 수집/스프라이트/정리는 호출자가 소유한다. clipped 는 호출자의 사각형 객체(훅 효과가 바꿀 수 있음)이다. (2026-11-13 WB04-E383)
export function drawRendererBackdrop(clipped, dirtyFlags, covered, g, hooks = {}) {
    const right = clipped.right;
    const bottom = clipped.bottom;
    if (g.hideGroups.value >>> 0) need(hooks.clear)(currentRect(clipped, right, bottom), g.black.value >>> 0);
    if ((dirtyFlags.value >>> 0) & 4 || (covered.value >>> 0)) return;
    const flags = (g.mode.value >>> 0) & 255;
    if (!(flags & 8)) {
        need(hooks.clear)(currentRect(clipped, right, bottom), g.tint.value >>> 0);
        return;
    }
    if ((flags & 16) && (flags & 1) && !(flags & 2)) {
        need(hooks.clear)(currentRect(clipped, right, bottom), 255);
        return;
    }
    let x;
    let y;
    // 두 경로 모두 원문의 Y→X 읽기/나눗셈 순서로 채운다.
    if (flags & 16) {
        const currentY = signed(g.cameraY.value);
        const divisor = signed(g.denominator.value);
        y = divide(currentY, divisor);
        x = divide(signed(g.cameraX.value), divisor);
    } else {
        y = signed(g.cameraY.value);
        x = signed(g.cameraX.value);
    }
    need(hooks.draw)(currentRect(clipped, right, bottom), x, y, false);
}
