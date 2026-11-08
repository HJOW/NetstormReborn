// 구름 배경의 갱신(00470c50)·표시 요청(00470c80)·표시 행 정리(00470ca0)·행 조회(00470ce0)와 두 표시 부모(004711d0/00471410)를 옮긴 모듈이다(cpppj `o/RawKencloudRender.cpp/.h`, 10.78 전용).
// 기존 Kencloud 소유자의 packed 상태 배열(2054 DWORD: 이전 행 512 + 캐시 주소 + 현재 행 512 + 표시 요청 flags)을 복제하지 않고 그대로 읽고 쓰며, 화면 장치는 호출자가 훅으로 제공한다. 계산·반복·쓰기 순서는 원문 몸체다.
//  - updateKencloud(globals, hooks): 캐시가 0 이면 build 훅으로 생성한 뒤 현재 speed 를 다시 읽고(기존 캐시는 최초 speed 만 캡처), 캡처한 횟수만큼 step 훅을 부른다(하위가 speed 를 바꿔도 이미 캡처한 반복 수는 유지).
//  - markKencloudDirty(state): 전체 표시 요청 1 → 부분 0 순서이다. cleanKencloudDirty(state): 이전 표시 행만 지우고(현재 행 표시도 0, 전체 0) 부분 요청이 남았으면 마지막에 전체/부분을 0 으로 정리한다.
//  - kencloudRowDirty(state, first, second): 두 값의 합을 signed 나머지(음수는 앞선 같은 물리 DWORD)로 행을 구해 그 행이나 전체 표시 요청이 켜져 있으면 1 이다.
//  - drawKencloud(globals, area, hooks, alternate): 면적(곱의 하위 32비트)이 0 이면 장치도 조회하지 않는다. 아니면 캐시가 없을 때 생성하고 현재 clip 을 저장·교체한 뒤 잠금(alternate 로 선택)하고, 512 타일 격자를 세로로 훑으며
//    전체 표시 요청이 없으면 켜진 행마다 가로 타일별로 한 줄 blit, 있으면 타일 전체 blit 뒤 부분 요청을 1 로 둔다. 잠금 해제와 clip 복구는 예외 때 가짜로 실행하지 않는다.
// globals = { state: Uint32Array(2054), get speed() }, area = [left, top, right, bottom, x, y](uint32), 훅 { build(), step(), clip()→[4], setClip([4]), lock(alt), unlock(alt), blit([8]) } 는 도달한 지점에서만 요구한다(없으면 Error).
// 모든 덧셈/뺄셈은 x86 uint32 감김을 따르고 비교는 signed 이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E119: RawKencloudRender 이식으로 추가)

// 원문 연속 전역의 실제 DWORD 위치(캐시 주소, 현재 행 표, 전체/부분 표시 요청)이다. (2026-11-08 WB04-E119)
const CACHE = 515;
const ROWS = 516;
const ALL = 2051;
const PART = 2052;

// 물리 범위를 확인한 위치를 돌려준다. 이전 쓰기를 예외로 되돌리지 않는다. (2026-11-08 WB04-E119)
function checked(state, index) {
    if (index < 0 || index >= state.length) throw new RangeError('구름 표시 현재 DWORD 물리 범위');
    return index;
}

// 원문 signed 비교용 변환과 음수 나머지(512)의 uint32 표현이다. (2026-11-08 WB04-E119)
const signed = (value) => value | 0;
const remainder = (value) => ((value | 0) % 512) >>> 0;

// 자연 생략 경로에는 장치를 요구하지 않고, 선택된 장치가 없으면 거부한다. (2026-11-08 WB04-E119)
function need(callback) {
    if (typeof callback !== 'function') throw new Error('구름 표시 선택된 실제 장치 누락');
    return callback;
}

// 00470c50 의 cache 관문과 캡처한 count 루프이다. 사용: updateKencloud({ state, get speed() }, { build, step }). (2026-11-08 WB04-E119)
export function updateKencloud(globals, hooks) {
    // 기존 캐시는 최초 속도만 캡처한다.
    let count = globals.speed >>> 0;
    if (!globals.state[checked(globals.state, CACHE)]) {
        need(hooks.build)();
        count = globals.speed >>> 0;
    }
    // 하위가 현재 speed 를 바꿔도 이미 캡처한 반복 수는 유지한다.
    for (; count; count = (count - 1) >>> 0) need(hooks.step)();
}

// 00470c80 의 all 1 → partial 0 순서이다(첫 store 뒤 중단은 그대로 남는다). (2026-11-08 WB04-E119)
export function markKencloudDirty(state) {
    state[checked(state, ALL)] = 1;
    state[checked(state, PART)] = 0;
}

// 00470ca0: 실제 이전 표시 행만 순서대로 지우고, 전체 표시가 남긴 부분 플래그도 마지막에 정리한다. (2026-11-08 WB04-E119)
export function cleanKencloudDirty(state) {
    // 실제 이전 표시 행만 지운다.
    for (let index = 0; index < 512; index += 1) {
        if (state[checked(state, index)]) {
            state[checked(state, ROWS + index)] = 0;
            state[checked(state, index)] = 0;
            state[checked(state, ALL)] = 0;
        }
    }
    if (state[checked(state, PART)]) {
        state[checked(state, ALL)] = 0;
        state[checked(state, PART)] = 0;
    }
}

// 00470ce0 의 signed 나머지/인접 alias 를 보존한다(음수 행은 앞선 같은 물리 DWORD 를 읽는다). 반환은 0 또는 1 이다. (2026-11-08 WB04-E119)
export function kencloudRowDirty(state, first, second) {
    const row = signed(remainder((first + second) >>> 0));
    const rowValue = state[checked(state, ROWS + row)];
    return rowValue || state[checked(state, ALL)] ? 1 : 0;
}

// 004711d0/00471410 의 clip/잠금/전체·부분 복사/복구이다. alternate 가 true 이면 두 번째 잠금/해제 장치를 쓴다. 사용: drawKencloud(globals, [l,t,r,b,x,y], hooks, alternate). (2026-11-08 WB04-E119)
export function drawKencloud(globals, area, hooks, alternate = false) {
    const [left, top, right, bottom, x, y] = area.map((value) => value >>> 0);
    // 원문 곱의 하위 32비트가 0 이면 장치도 조회하지 않는다.
    if (Math.imul((right - left) >>> 0, (bottom - top) >>> 0) === 0) return;
    const state = globals.state;
    if (!state[checked(state, CACHE)]) need(hooks.build)();
    const previous = need(hooks.clip)();
    need(hooks.setClip)([left, top, right, bottom]);
    // x86 덧셈/뺄셈의 실제 감김이다.
    const leftX = (left + x) >>> 0;
    const topY = (top + y) >>> 0;
    const firstX = (leftX - ((remainder(leftX) + x) >>> 0)) >>> 0;
    const firstY = (topY - ((remainder(topY) + y) >>> 0)) >>> 0;
    const rightX = (x + 511 + right) >>> 0;
    const bottomY = (bottom + 511 + y) >>> 0;
    const lastX = (rightX - ((remainder(rightX) + x) >>> 0)) >>> 0;
    const lastY = (bottomY - ((remainder(bottomY) + y) >>> 0)) >>> 0;
    need(hooks.lock)(alternate);
    // 각 세로 타일에서 현재 allDirty 를 다시 읽는다.
    for (let tileY = firstY, rowBottom = (1 - firstY) >>> 0; signed(tileY) < signed(lastY); tileY = (tileY + 512) >>> 0, rowBottom = (rowBottom - 512) >>> 0) {
        if (!state[checked(state, ALL)]) {
            // dirty 값은 각 실제 행 접근 때 조회한다.
            for (let row = 0; row < 512; row += 1) {
                if (!state[checked(state, ROWS + row)]) continue;
                for (let tileX = firstX; signed(tileX) < signed(lastX); tileX = (tileX + 512) >>> 0) {
                    const blit = need(hooks.blit);
                    // 각 복사 전에 현재 cache 를 조회한다.
                    blit([tileX, (tileY + row) >>> 0, state[checked(state, CACHE)], 512, 0, row, 512, (rowBottom + tileY + row) >>> 0]);
                }
                // 가로 복사가 없어도 선택된 행의 이전 표시 요청을 남긴다.
                state[checked(state, row)] = 1;
            }
        } else {
            for (let tileX = firstX; signed(tileX) < signed(lastX); tileX = (tileX + 512) >>> 0) {
                const blit = need(hooks.blit);
                // 전체 경로도 callback 뒤 현재 cache 를 읽는다.
                blit([tileX, tileY, state[checked(state, CACHE)], 512, 0, 0, 512, 512]);
            }
            // 현재 세로 타일의 가로 복사가 끝난 뒤에만 저장한다.
            state[checked(state, PART)] = 1;
        }
    }
    // 예외 때 이 후속 효과를 가짜로 실행하지 않는다.
    need(hooks.unlock)(alternate);
    need(hooks.setClip)(previous);
}
