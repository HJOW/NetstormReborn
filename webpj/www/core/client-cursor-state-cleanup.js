// Session 없는 첫 메뉴도 같은 커서 데이터에서 Cancel→Activate→Prepare를 실행한다(cpppj client/ClientCursorStateCleanup.cpp/.h).
// makeClientCursorPrepareHooks는 CampaignCursorClientEffects의 해당 공개 API만 앞당긴다. 나머지 GUI/번역/입력 연결은 후속 단위다.
// client 최소 계약: gameplayGlobals() → { camera()→ClientCameraGlobals, cursorGlobals()→ClientCursorGlobals, cursor }, getRenderer()→Renderer.
// surface 최소 계약: {pool, types, hash.view(0)}. 장치는 호출 때 현재 소유자에서 조회하며 이전 Renderer/Session을 보관하지 않는다.
// 이 모듈은 DOM을 쓰지 않는다. (2026-11-14 WB04-E402: 원문 취소와 첫 메뉴 표시 준비 연결)
import { RawCursorLifecycle } from './raw-cursor-lifecycle.js';
import { RawCursorPlacement } from './raw-cursor-placement.js';

// 현재 카메라 투영과 현재 Renderer 변경 표만 제공한다. provider가 없으면 client의 현재 렌더러를 조회한다.
export function makeClientCursorPrepareHooks(client, renderer = null) {
    if (renderer === null) renderer = () => client.getRenderer();
    return {
        project: (x, y) => client.gameplayGlobals().camera().snapshot().project(x, y),
        invalidate: (rect, flags) => renderer().invalidate(rect, flags),
    };
}

// 일반 game.prepare를 호출하지 않고 같은 실제 상태에 원문 Cancel/Prepare 두 몸체만 조립한다.
export function cancelCursorForState(pool, types, drop, state, effects, placement, surfaceMap = []) {
    const prepare = new RawCursorPlacement(pool, types, surfaceMap, drop, state.lifecycle(), state.placement(), placement);
    const selected = { ...effects, prepareCursor: () => prepare.prepare() };
    new RawCursorLifecycle(pool, types, drop, state.lifecycle(), selected).cancel();
}

// 현재 Client 카메라의 같은8바이트 픽셀 원점을 빌리고, 첫 메뉴의 잠금/빈 목록도 실제 Project/dirty 꼬리를 실행한다.
export function cancelClientCursorForState(client, surface, selectedEffects = {}, renderer = null) {
    const gameplay = client.gameplayGlobals(); const state = gameplay.cursorGlobals();
    const snapshot = gameplay.camera().snapshot();
    // CameraView의 bytes는 장기 주소다. pixels 접근자가 돌려주는 값 사본을 공유 포인터로 쓰지 않는다.
    const current = state.placement().sharedScreenOrigin; const offset = snapshot.bytes.byteOffset + 28;
    if (!(current instanceof Int32Array) || current.buffer !== snapshot.bytes.buffer || current.byteOffset !== offset || current.length !== 2) {
        state.placement().sharedScreenOrigin = new Int32Array(snapshot.bytes.buffer, offset, 2);
    }
    cancelCursorForState(surface.pool, surface.types, gameplay.cursor, state, selectedEffects, makeClientCursorPrepareHooks(client, renderer), surface.hash.view(0));
}
