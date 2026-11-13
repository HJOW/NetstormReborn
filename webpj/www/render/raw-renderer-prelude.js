// 원본 Renderer 의 프레임 앞단(10.78 004994b0..00499758 구간의 화면 flags/override/구름 행 스캔과 전체 dirty 준비 순서)을 옮긴 모듈이다(cpppj `client/RawRendererPrelude.cpp/.h`, 10.78 전용).
// Draw 의 월드 좌표/수집/표시 앞에서 한 번 호출한다. 실제 Frame 연결·표시·정리는 호출자가 따로 소유하고, 이 모듈은 게임 계산과 실제 화면/구름 장치를 분리한다.
//  - prepareRendererFrame(state, globals, hooks) 가 하는 일(원문 순서 그대로):
//      ① 현재 화면 flags(mode)가 이전 flags(scan[18])와 다르면 구름 dirty 표시(markCloudDirty)와 전체 화면 구름 dirty 4칸 저장(queueCloudDirty)을 하고 이전 flags 를 현재 값으로 갱신한다.
//      ② 같은 세계 redraw 요청(force)이 있으면 같은 구름 dirty 두 효과를 한 번 더 한다(요청 자체는 지우지 않는다).
//      ③ mode 에 구름 표시 비트(8)가 있고 override(모달 위 모드)가 꺼져 있으면: 구름 갱신 비트(32)에서는 updateClouds 후 하위 BYTE 를 다시 읽고, 시차 비트(16)가 아니면
//         16행 표(scan[1..16])의 현재 index 행부터 30행 구간에서 첫 dirty 행(rowDirty)을 찾아 지도 clip 으로 자른 30행 사각형을 일반 화면 dirty 에 추가(invalidate)하고 index 를 (index+1)%16 으로 옮긴다.
//         표는 최초 1번만(scan[17] 비트 0) 초기 높이로 구성한다. 마지막에 구름 dirty 표시를 하고, 시차+플래그 조합(16·1 켜짐, 2 꺼짐)이면 별도 시차 배경 dirty(invalidateBackground)와 backgroundRedraw=1 을 쓴다.
//      ④ 별도 구름 dirty 사각형 개수(drawCounter)를 0 으로 쓰고, 세계 redraw 요청이 있으면 일반 dirty 개수(dirtyCount)를 0 으로 쓴 뒤(호스트 표를 쓰면 clearDirty) 새 전체 사각형을 추가한다.
//  - 인접 전역: scan 은 0059a984 부터 이어지는 19 DWORD(index · 16행 · 초기화 · 이전 flags)의 물리 배열이라 index-1/16/17 의 인접 읽기를 그대로 둔다(임의로 16칸에 정규화하지 않는다). 범위 밖은 오류다.
//  - 현재 참조: 원본은 효과(훅) 뒤에도 같은 저장소를 다시 읽으므로 globals·state 의 칸은 `{ value }` 상자(실제 저장소를 빌린 것)이고 매번 현재 값을 읽는다. 값은 DWORD/signed 어느 쪽이어도 되며 signed 비교는 `|0`, 비트 검사는 `>>>0` 으로 읽는다.
//  - 훅은 실제로 도달한 위치에서만 요구한다(생략 경로에는 연결을 요구하지 않고, 선택됐는데 없으면 Error 로 거부 — 앞선 효과는 남는다).
// 사용: prepareRendererFrame({ scan, backgroundRedraw, dirtyCount, drawCounter }, { mode, force, width, height, initialHeight, cameraY, left, top, right, bottom }, hooks).
//   훅: markCloudDirty()·queueCloudDirty(rect)·overrideActive()→DWORD·updateClouds()·rowDirty(row, cameraY)→0/1·invalidate(rect, flags)·invalidateBackground()·clearDirty()(선택).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E383: 5-A 렌더러 프레임 앞단으로 추가, 기준선 뒤 cpppj 변경 없음)
import { clipRect } from './indexed-surface.js';

// scan 의 index 뒤 인접 DWORD 위치: 최초 구성 표식(비트 0)과 이전 화면 flags 이다. (2026-11-13 WB04-E383)
const INITIALIZED = 17;
const PREVIOUS_MODE = 18;
// scan 의 크기(index 1 + 16행 + 표식 + 이전 flags)이다. (2026-11-13 WB04-E383)
export const RENDERER_PRELUDE_SCAN_SIZE = 19;

// 실제로 도달한 하위 장치만 요구한다. 선택됐는데 없으면 그 위치에서 거부한다. (2026-11-13 WB04-E383)
function need(callback) {
    if (typeof callback !== 'function') throw new Error('Renderer 앞단의 선택된 현재 장치 미연결');
    return callback;
}

// DWORD 비트를 signed 로 읽는다. (signed CMP/IDIV 대상) (2026-11-13 WB04-E383)
const signed = (value) => value | 0;
// ADD/LEA 의 하위 32비트 결과이다. (2026-11-13 WB04-E383)
const add = (value, delta) => (value + delta) | 0;

// 행 표의 index 번째 행 값이다. 물리 위치는 index+1 이고 인접 alias(index-1/16/17)는 허용하며 배열 밖은 오류이다. (2026-11-13 WB04-E383)
function rowAt(scan, index) {
    const physical = index + 1;
    if (physical < 0 || physical >= scan.length) throw new RangeError('Renderer 행 표의 현재 인접 물리 범위');
    return signed(scan[physical]);
}

// 호출 직전 현재 화면 전체 사각형이다. (2026-11-13 WB04-E383)
const fullRect = (g) => ({ left: 0, top: 0, right: signed(g.width.value), bottom: signed(g.height.value) });
// 행 조회 뒤 바뀐 현재 지도 clip 도 반영하는 사각형이다. (2026-11-13 WB04-E383)
const viewRect = (g) => ({ left: signed(g.left.value), top: signed(g.top.value), right: signed(g.right.value), bottom: signed(g.bottom.value) });

// 행 표 최초 구성: 원문은 0번 행을 쓰지 않으며 기존 값과 실제 store 순서를 유지하고 초기 높이를 한 번만 참조한다. (2026-11-13 WB04-E383)
function initializeRows(scan, height) {
    scan[INITIALIZED] |= 1;
    scan[2] = (height - 30) >>> 0; scan[4] = (height - 150) >>> 0; scan[6] = (height - 60) >>> 0; scan[8] = (height - 180) >>> 0;
    scan[10] = (height - 90) >>> 0; scan[12] = (height - 210) >>> 0;
    scan[3] = 120; scan[5] = 30; scan[7] = 150; scan[9] = 60; scan[11] = 180; scan[13] = 90; scan[15] = 210;
    scan[14] = (height - 120) >>> 0; scan[16] = (height - 240) >>> 0;
}

// Draw 의 수집/표시 전에 현재 화면 모드·세계 요청·override 와 구름 행 스캔을 순서대로 처리한다. 사용: prepareRendererFrame(state, globals, hooks). (2026-11-13 WB04-E383)
export function prepareRendererFrame(state, g, h) {
    const scan = state.scan;
    if (scan.length !== RENDERER_PRELUDE_SCAN_SIZE) throw new RangeError('Renderer 앞단 스캔은 19 DWORD 여야 합니다');
    const mode = () => g.mode.value >>> 0;
    if (mode() !== scan[PREVIOUS_MODE]) {
        need(h.markCloudDirty)();
        need(h.queueCloudDirty)(fullRect(g));
        scan[PREVIOUS_MODE] = mode();
    }
    if (g.force.value >>> 0) {
        need(h.markCloudDirty)();
        need(h.queueCloudDirty)(fullRect(g));
    }
    if ((mode() & 8) && !(need(h.overrideActive)() >>> 0)) {
        // override 조회 효과 뒤 현재 하위 BYTE 를 캡처한다.
        let flags = mode() & 255;
        if (flags & 32) {
            need(h.updateClouds)();
            flags = mode() & 255;
            if (!(flags & 16)) {
                if (!(scan[INITIALIZED] & 1)) initializeRows(scan, signed(g.initialHeight.value) >>> 0);
                let index = signed(scan[0]);
                let row = rowAt(scan, index);
                if (row < add(row, 30)) {
                    // 원문은 현재 30행 구간의 첫 dirty 에서 멈추고 매 콜백 뒤 현재 index/상한을 다시 조회한다.
                    for (;;) {
                        const dirty = need(h.rowDirty)(row, signed(g.cameraY.value));
                        index = signed(scan[0]);
                        if (dirty) {
                            const start = rowAt(scan, index);
                            need(h.invalidate)(clipRect({ left: 0, top: start, right: signed(g.width.value), bottom: add(start, 30) }, viewRect(g)), 0);
                            break;
                        }
                        row = add(row, 1);
                        if (row >= add(rowAt(scan, index), 30)) break;
                    }
                    flags = mode() & 255;
                }
                // signed 나머지이며 음수 index 를 양수로 바꾸지 않는다(DWORD 저장 때 감긴다).
                scan[0] = add(index, 1) % 16;
            }
        }
        need(h.markCloudDirty)();
        if ((flags & 16) && (flags & 1) && !(flags & 2)) {
            need(h.invalidateBackground)();
            state.backgroundRedraw.value = 1;
        }
    }
    state.drawCounter.value = 0;
    if (g.force.value >>> 0) {
        state.dirtyCount.value = 0;
        if (h.clearDirty) h.clearDirty();
        need(h.invalidate)(fullRect(g), 0);
    }
}
