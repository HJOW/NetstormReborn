// 절벽 fringe 모듈이다: 지면 칸마다 고른 `isle` 클러스터에 `fringe` 플래그(비트 1)가 있으면, 그 칸의 한 줄 아래(y + 4칸)에 `fringe` 타입 오브젝트를 하나 더 만든다. 지면 아래로 늘어진 절벽 바위가 이것이다.
// 복원 기준: cpppj `o/RawTerrainStamp.cpp` 의 `Create`(원본 Terrainbuilder 004bfe50, 패치 10.78)·`o/RawFrameChoose.cpp`(ChooseTerrainFrame, 원본 0049aa90)·`o/RawTerrainContext.cpp`(제외 플래그 004c0390)이며
// 근거 문서는 docs/exe/terrain-and-bridges.md(절벽 fringe 절)와 docs/exe/cpp-initial-terrain-stamp-audit-20261013.md 이다.
// 규칙 요약:
//  - 지면 하나를 만든 직후(첫 도장 루프의 칸마다) 고른 클러스터의 코드(방향 글자·변형 글자·플래그)를 읽는다. 플래그 비트 1(`fringe`)이 없으면 아무것도 하지 않고 난수도 쓰지 않는다.
//  - 있으면 `fringe` 타입의 프레임 코드 표에서 그 코드로 후보를 찾는다(정확히 → 정규화 → (방향, 'A') → 방향만, 지면 선택과 같은 4단계 검색). 후보 전체에서 난수 한 번(theme −1)으로 하나를 고른다.
//  - 고른 후보의 플래그가 "제외 플래그"와 겹치면 후보를 버리고 **난수를 다시 소비해** 다시 고른다(겹치지 않을 때까지 반복). 전투 맥락은 0x10(`unlit` 제외), 맥락 소유자가 0 이면 0x18(`lit`·`unlit` 모두 제외)이다.
//    후보가 하나도 없으면(-1) 원본은 후보 코드 배열 바로 앞 바이트를 플래그로 읽는데(prefixFlags 입력, 기본 0) 겹치지 않아 그대로 끝나고 절벽은 만들지 않는다.
//  - 만들어진 절벽의 프레임은 고른 클러스터 번호이고 위치는 지면과 같은 x, y + 4칸(실수 4.0)이다. 소유자는 없다(0).
//  - 지역 도장(신전을 세워 소유자가 정해질 때 다시 칠함)은 절벽을 다시 만들지 않는다. 절벽은 첫 도장이 정한 클러스터로만 정해진다.
// 이 모듈은 DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다.
// (2026-11-02 WB03-Y: 정적 맵 화면(M2) 3단계(c)로 추가)
import { WORLD_CELLS } from './terrain-mask.js';
import { terrainFrameCandidates } from './terrain-tiles.js';

// 지면 클러스터 코드 플래그에서 "절벽을 만든다"를 뜻하는 비트(`fringe`)이다. (2026-11-02 WB03-Y)
export const FRINGE_FLAG = 0x01;

// 절벽 오브젝트를 지면 아래로 내리는 칸 수이다(원본 0x502314 의 실수 4.0). 칸의 화면 높이 11픽셀을 곱하면 44픽셀이다. (2026-11-02 WB03-Y)
export const FRINGE_Y_OFFSET = 4;

// 전투 맥락의 제외 플래그이다: `unlit` 클러스터(비트 0x10)가 있는 후보는 쓰지 않는다. (2026-11-02 WB03-Y)
export const EXCLUDE_UNLIT = 0x10;

// 맥락 소유자가 0 인 경로의 제외 플래그이다: `lit`(0x08)·`unlit`(0x10) 클러스터 후보를 모두 쓰지 않는다. (2026-11-02 WB03-Y)
export const EXCLUDE_LIT_AND_UNLIT = 0x18;

// 허용 후보가 있는데도 난수를 이만큼 다시 소비해도 못 고르면 오류로 본다(원본은 끝없이 반복한다; 허용 후보가 하나라도 있으면 사실상 일어나지 않는다). (2026-11-02 WB03-Y)
const RETRY_LIMIT = 1_000_000;

// 후보 클러스터의 플래그가 제외 플래그와 겹치는지 본다(원본은 플래그 바이트를 부호 있는 char 로 읽어 32비트로 확장한 값과 비교한다). (2026-11-02 WB03-Y)
function overlapsExcluded(flags, excludedFlags) {
    return (excludedFlags & ((flags << 24) >> 24)) !== 0;
}

// 방향·변형 글자(숫자 코드)를 한 글자 문자열로 바꾼다. (2026-11-02 WB03-Y)
function letter(code) {
    return String.fromCharCode(code);
}

// 지면 클러스터 코드 하나에 대해 제외 플래그를 지킨 절벽 후보 목록(프레임 번호, 저장 순서)을 돌려준다. 난수는 쓰지 않는다.
// code 는 { side, variant }(문자 코드)이고, fringeFrames 는 `fringe` 타입의 FrameCodeTable 이다. 반환: { stage, candidates, allowed }(stage 는 검색 단계, candidates 는 제외 전 후보, allowed 는 제외 뒤 후보).
// 사용: fringeCandidates(fringeFrames, isleFrames.codes[40], EXCLUDE_UNLIT). (2026-11-02 WB03-Y)
export function fringeCandidates(fringeFrames, code, excludedFlags = EXCLUDE_UNLIT) {
    const { stage, candidates } = terrainFrameCandidates(fringeFrames, letter(code.side), letter(code.variant));
    return { stage, candidates, allowed: candidates.filter((frame) => !overlapsExcluded(fringeFrames.codes[frame].flags, excludedFlags)) };
}

// 절벽 클러스터 하나를 고른다(원본 004bfe50 의 선택 반복): 후보 전체에서 random.next(후보 수) 한 번으로 하나를 고르고, 플래그가 제외 플래그와 겹치면 난수를 다시 소비해 다시 고른다.
// 후보가 하나도 없으면 -1 이다(난수를 쓰지 않는다). 후보는 있지만 모두 제외 플래그와 겹치면 원본은 끝없이 반복하므로 RangeError 로 알린다.
// options: { excludedFlags(기본 EXCLUDE_UNLIT), prefixFlags(후보가 없을 때 원본이 읽는 앞 바이트 플래그, 기본 0) }. 사용: chooseFringeFrame(fringeFrames, code, random). (2026-11-02 WB03-Y)
export function chooseFringeFrame(fringeFrames, code, random, { excludedFlags = EXCLUDE_UNLIT, prefixFlags = 0 } = {}) {
    const { candidates, allowed } = fringeCandidates(fringeFrames, code, excludedFlags);
    if (candidates.length === 0) {
        if (overlapsExcluded(prefixFlags, excludedFlags)) throw new RangeError('절벽 후보가 없고 앞 바이트 플래그가 제외 플래그와 겹친다(원본은 끝없이 반복)');
        return -1;
    }
    if (allowed.length === 0) throw new RangeError('절벽 후보가 모두 제외 플래그와 겹친다(원본은 끝없이 반복)');
    // 겹치지 않는 후보가 나올 때까지 난수를 다시 소비한다.
    for (let tries = 0; tries < RETRY_LIMIT; tries += 1) {
        const selected = candidates[random.next(candidates.length)];
        if (!overlapsExcluded(fringeFrames.codes[selected].flags, excludedFlags)) return selected;
    }
    throw new RangeError('절벽 선택 반복 한도 초과');
}

// 첫 도장 훅(terrain-tiles.js `buildTerrainFrames` 의 options.onStamp)에 붙여 쓰는 절벽 생성기를 만든다. 반환: { cells, onStamp, count, positionOf }.
//  - cells: Int32Array(65536), 인덱스 = 지면 칸의 y × 256 + x, 값 = 그 칸 아래에 만든 절벽의 `fringe` 클러스터 번호(없으면 -1). 절벽의 월드 위치는 (x, y + FRINGE_Y_OFFSET)이다.
//  - onStamp(x, y, frame, random): 칸의 지면 프레임을 고른 직후 부른다. 프레임의 코드에 fringe 플래그가 있을 때만 chooseFringeFrame 으로 절벽을 고른다.
//  - count(): 만든 절벽의 수. positionOf(x, y): 절벽의 월드 칸 좌표 { x, y: y + 4 }.
// isleFrames·fringeFrames 는 각각 `isle`·`fringe` 타입의 FrameCodeTable 이다. 지면 프레임이 표 밖이면 원본의 범위 검사 오류처럼 RangeError 이다.
// 사용: const fringe = createFringeStamper(isleFrames, fringeFrames); buildTerrainFrames(mask, isleFrames, { onStamp: fringe.onStamp }); fringe.cells. (2026-11-02 WB03-Y)
export function createFringeStamper(isleFrames, fringeFrames, options = {}) {
    const cells = new Int32Array(WORLD_CELLS * WORLD_CELLS).fill(-1);
    let created = 0;
    return {
        cells,
        onStamp(x, y, frame, random) {
            const code = isleFrames.codes[frame];
            if (!code) throw new RangeError('초기 도장 생성 프레임 범위 오류');
            if ((code.flags & FRINGE_FLAG) === 0) return;
            const selected = chooseFringeFrame(fringeFrames, code, random, options);
            if (selected === -1) return;
            cells[y * WORLD_CELLS + x] = selected;
            created += 1;
        },
        count() {
            return created;
        },
        positionOf(x, y) {
            return { x, y: y + FRINGE_Y_OFFSET };
        },
    };
}
