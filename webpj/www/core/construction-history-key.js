// Tab 의 최근 건설 위치 순환과 최초 5칸 초기화(원문 0043994c·0040e800, Tab 키 부모), 그리고 U/u 의 최근 손실 위치 이동(004d98e2..004d990d)을 옮긴 모듈이다(cpppj `client/ConstructionHistoryKey.cpp/.h`, `client/LostPositionKey.cpp/.h`, `o/RawConstructionHistory.h`, 10.78 전용).
// 이력(history = { points: [{x, y} × 5](단정도 좌표, NaN/무효 비트도 보존), index })은 Client 장기의 같은 값형 장부이며 Session 교체 때 재생성하지 않는다.
//  - initializeConstructionHistory: 다섯 좌표 40 BYTE 만 (-10000, -10000) 으로 쓰고 현재 순번은 보존한다.
//  - isConstructionHistoryPointValid(point, worldCells): 0 < 각 축 < float(signed 세계 크기) 를 그대로 판정한다(0/상한/NaN 은 무효).
//  - dispatchConstructionHistoryKey(event, history, worldCells(), hooks): 뗌이거나 키가 Tab(9)이 아니면 소비하지 않는다. 정지(paused)면 효과 없이 소비. 아니면 현재 순번의 좌표를 먼저 복사하고 무효이면 순번 0 과 첫 칸으로 복귀한 뒤
//    유효하면 center(x, y) 로 즉시 카메라 이동 후 카메라가 바꾼 현재 순번을 다시 읽어 (순번 + 1)(32비트 감김)을 signed 로 5 로 나눈 나머지를 쓴다. 순번이 5 이상이면 범위 오류로 거부한다(임의 modulo 보정 없음).
//  - dispatchLostPositionKey(event, lostX(), lostY(), worldCells(), hooks): U/u (수정키·outside 는 거부 조건이 아님)이고 뗌이 아니면 정지 판정 후 정지가 아니면, 정지 callback 이 바꾼 현재 좌표/세계 크기로 유효점일 때만 center 를 호출한다.
// 선택된 정지/카메라 장치가 없으면 그 위치에서 거부하며 앞선 쓰기(첫 칸 복귀 등)는 보존한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E22: 건설 위치 순환·손실 위치 키 이식으로 추가)
import { InputCode } from './input-queue.js';

// 이력 칸 수, Tab 코드, 수정키 제거 마스크, 'U'/'u' 코드, 무효 초기 좌표이다. (2026-11-08 WB04-E22)
export const CONSTRUCTION_HISTORY_COUNT = 5;
const TAB = 9;
const KEY_MASK = 0x03ffffff;
const UPPER_U = 0x55;
const LOWER_U = 0x75;
const INVALID_POINT = -10000;

// 이력을 만든다(최초 명시 초기화 전에는 BSS 0). (2026-11-08 WB04-E22)
export function createConstructionHistory() {
    return { points: Array.from({ length: CONSTRUCTION_HISTORY_COUNT }, () => ({ x: 0, y: 0 })), index: 0 };
}

// 0043994c: 다섯 좌표 40 BYTE 만 -10000 두 좌표로 쓴다. (2026-11-08 WB04-E22)
export function initializeConstructionHistory(history) {
    for (const point of history.points) { point.x = INVALID_POINT; point.y = INVALID_POINT; }
}

// 0040e800: 0 < 각 축 < float(signed 세계 크기). NaN 은 무효이다. (2026-11-08 WB04-E22)
export function isConstructionHistoryPointValid(point, worldCells) {
    const maximum = Math.fround(worldCells | 0);
    return 0 < point.x && point.x < maximum && 0 < point.y && point.y < maximum;
}

// Tab 키 부모이다. hooks = { paused(), center(x, y) }. (2026-11-08 WB04-E22)
export function dispatchConstructionHistoryKey(event, history, worldCells, hooks) {
    if ((event.code & InputCode.release) !== 0 || ((event.code & KEY_MASK) >>> 0) !== TAB) return false;
    if (typeof hooks?.paused !== 'function') throw new Error('최근 건설 위치 키의 실제 정지 판정 미연결');
    if (hooks.paused()) return true;
    if (history.index >= CONSTRUCTION_HISTORY_COUNT) throw new RangeError('최근 건설 위치 순번의 물리 5칸 범위');
    // 실제 두 DWORD 를 현재 순번에서 먼저 복사한다.
    let point = { ...history.points[history.index] };
    if (!isConstructionHistoryPointValid(point, worldCells())) {
        history.index = 0;
        point = { ...history.points[0] };
    }
    if (isConstructionHistoryPointValid(point, worldCells())) {
        if (typeof hooks.center !== 'function') throw new Error('최근 건설 위치의 실제 즉시 카메라 미연결');
        hooks.center(point.x, point.y);
        // 카메라가 바꾼 현재 순번을 다시 읽고 INC 의 32비트 감김/signed IDIV 나머지를 보존한다.
        history.index = ((((history.index + 1) | 0) % CONSTRUCTION_HISTORY_COUNT)) >>> 0;
    }
    return true;
}

// U/u 키 부모이다. lostX()/lostY()/worldCells() 는 호출 위치마다 현재 값을 읽는 getter, hooks = { paused(), center(x, y) }. (2026-11-08 WB04-E22)
export function dispatchLostPositionKey(event, lostX, lostY, worldCells, hooks) {
    const key = (event.code & KEY_MASK) >>> 0;
    if ((event.code & InputCode.release) !== 0 || (key !== UPPER_U && key !== LOWER_U)) return false;
    if (typeof hooks?.paused !== 'function') throw new Error('최근 손실 위치 키의 실제 정지 판정 미연결');
    if (hooks.paused()) return true;
    // 0040e800 의 strict 양축 범위이며 0/상한/NaN 은 무효이다.
    if (isConstructionHistoryPointValid({ x: lostX(), y: lostY() }, worldCells())) {
        if (typeof hooks.center !== 'function') throw new Error('최근 손실 위치의 실제 즉시 카메라 미연결');
        hooks.center(lostX(), lostY());
    }
    return true;
}
