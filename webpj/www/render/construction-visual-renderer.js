// 복원한 가시 표시 수집을 공유 Renderer 정렬 비교와 실제 표시 Draw 에 연결하는 어댑터이다(cpppj `client/ConstructionVisualRenderer.cpp/.h`, 10.78 전용).
//  - drawVisibleConstruction(system, view, work, sort = true): 월드 영역의 가시 건설 표시를 작업 버퍼에 수집하고 선택적으로 깊이/y/x 정렬(원본 비교 키)한 뒤 제출한다.
//    반환은 수집 항목 수이다. 공간이 모자라면 원본처럼 먼저 등록된 항목으로 제한하며 잘린 항목은 정렬에 참여하지 않는다. sort=false 는 정렬 전역이 꺼진 상태(수집 순서 보존)에 해당한다.
//  - drawVisibleScene(system, request, work, collection, state, dispatch, sort = true): 실제 건설 수집/토큰 Draw 를 공통 목록(`draw-submission.js`)에 연결하고 외부 UI/묶음/자산/미리보기 수집기와 함께 정렬·분배한다.
//    system 은 `ConstructionVisualSystem`(그 안의 공통 표시 저장소를 쓴다) 또는 건설/불꽃 꼬리를 포함한 원본 단일 표시 목록 `RenderProcessSystem` 이며 둘 다 같은 kind 3 수집·정렬·Draw 로 처리한다.
//    두 훅 묶음의 construction 은 system 으로 연결한 사본이라 호출자의 훅은 바뀌지 않는다. 반환의 backgroundCovered 는 호출자가 배경 그리기에 사용한다.
// 화면 픽셀/카메라 환산은 개별 Draw 몸체가 수행하며 수집/정렬은 월드 좌표를 쓴다. 정렬은 유한 좌표만 지원한다. 전체 Client/GameWorld 프레임 루프·외부 수집기의 실제 자산/SHP/장치 연결은 후속이다.
// 작업 버퍼·월드·팔레트는 호출 동안 살아야 한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E382: 5-A 시각 효과 소단위로 추가, 기준선 뒤 cpppj 변경 없음)
import { RENDER_ENTRY_BYTES } from '../core/raw-render-collection.js';
import { RenderProcessSystem } from '../core/render-process-system.js';
import { collectDrawEntries, dispatchDrawEntries, sortRenderEntries } from './draw-submission.js';

// 원본 작업 개수는 32비트 signed 이다. 그보다 큰 작업 버퍼는 거부한다. (2026-11-13 WB04-E382)
const MAX_WORK_ENTRIES = 0x7fffffff;

// 단일 표시 목록 하나를 그 목록의 수집기·토큰 Draw 로 처리한다. (2026-11-13 WB04-E382)
function drawScene(system, request, work, collection, state, dispatch, sort) {
    const collectors = { ...collection, construction: (view, output, limit) => system.collect(view, output, limit) };
    const hooks = { ...dispatch, construction: (token) => { system.drawToken(token); } };
    const result = collectDrawEntries(request, work, collectors);
    const entries = work.subarray(0, result.count * RENDER_ENTRY_BYTES);
    if (sort) sortRenderEntries(entries);
    dispatchDrawEntries(entries, state, hooks);
    return result;
}

// 클라이언트의 원본 비교 키를 그대로 쓴다. 건설 표시만 수집해 그리며 수집한 항목 수를 돌려준다. 사용: drawVisibleConstruction(system, { left, top, right, bottom }, new Uint8Array(16 * 5000)). (2026-11-13 WB04-E382)
export function drawVisibleConstruction(system, view, work, sort = true) {
    if (Math.floor(work.length / RENDER_ENTRY_BYTES) > MAX_WORK_ENTRIES) throw new RangeError('건설 표시 작업 버퍼 크기 오류');
    const request = { screen: { left: 0, top: 0, right: 0, bottom: 0 }, visible: view, unclipped: view, dirtyFlags: 0, groupRowLimit: 0 };
    return drawVisibleScene(system, request, work, {}, {}, {}, sort).count;
}

// 수집기는 호출 동안만 system 을 참조한다. ConstructionVisualSystem 이면 안의 공통 표시 저장소를, RenderProcessSystem 이면 그대로 쓴다(C++ 오버로드 둘). (2026-11-13 WB04-E382)
export function drawVisibleScene(system, request, work, collection, state, dispatch, sort = true) {
    const rendering = system instanceof RenderProcessSystem ? system : system.rendering;
    if (!(rendering instanceof RenderProcessSystem)) throw new TypeError('건설 표시 수집 대상 오류');
    return drawScene(rendering, request, work, collection, state, dispatch, sort);
}
