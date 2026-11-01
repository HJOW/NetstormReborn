// 본섬 칸 마스크 모듈이다: 영역 배치(territory.js 의 ChunkMap·IslandList)로부터 월드 256 × 256칸 중 어느 칸이 어느 영역의 지면인지를 만든다.
// 복원 기준: cpppj `o/TerrainBuilder.cpp`(원본 Terrainbuilder.cpp 004c10a0: 전체 연결 통로 → 청크별 성장 004c0800 → 두 차례 빈틈 보정 004c0cf0·004c0c30, 정수 난수 004bca20)이며
// 근거 문서는 docs/exe/terrain-and-bridges.md 이다. 두 공식 맵의 마스크 SHA-256 이 그 문서에 있고, 원본 디컴파일에서 따로 옮긴 Python 재현 도구(tools/terrain_mask.py)가 있다.
// 범위: 칸 마스크(영역 번호 바이트)까지이다. 지면 타일(직선/대각선 이웃 → 클러스터) 선택·가장자리 절벽(fringe)·edgeFarm·작은 받침(noIsland)·소유자 색 변환은 옮기지 않았다.
// 규칙 요약:
//  - 청크마다(월드 y 바깥·x 안쪽 순서, 영역 번호 순서가 아니다) 중심 (4 + (시드 & 7), 4 + ((9999 - 시드) & 7))에 4 × 4칸, 방향 글자의 연결 비트(북 1·동 2·남 4·서 8)가 가리키는 쪽으로 폭 2칸 통로를 만들고,
//    영역의 두 번째 청크는 추가로 10 × 10칸을 채운다. 시드는 `Territory` 레코드의 넷째 바이트이다.
//  - 청크별 목표량 = max(20, Next(30) + 50 - 초기 칸 수 × 100 ÷ 256) × 256 ÷ 100 (난수 시드 = 영역 안 순서 × 0x10e3 + 시드).
//  - 모든 통로가 만들어진 뒤 청크마다 목표량만큼 3 × 3 덩어리를 붙인다(같은 섬의 청크 안에서만, 연속 1000회 실패하면 끝). 난수 시드 = (영역 청크 수 + 영역 안 순서) × 0x10e3 + 시드.
//  - 마지막으로 직선 이웃에 지면이 있는 빈 칸을 두 번 훑어, 여덟 이웃에서 같은 영역이 다섯 칸 넘게 이어지고 마지막 방향이 홀수이면 채운다.
// 이 모듈은 DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다.
// (2026-11-01 WB03-W: 정적 맵 화면(M2) 3단계의 첫 부분으로 추가)
import { WORLD_CHUNKS_PER_SIDE, CHUNK_CELLS } from './fort.js';

// 월드 한 변의 칸 수이다(16청크 × 16칸). (2026-11-01 WB03-W)
export const WORLD_CELLS = WORLD_CHUNKS_PER_SIDE * CHUNK_CELLS;

// 마스크에서 지면이 없는 칸의 값이다(검사 스트림의 허공 표시 255). (2026-11-01 WB03-W)
export const EMPTY_TERRAIN = 255;

// 북부터 시계 방향인 여덟 이웃의 (dx, dy) 이다(원본 0052f83c/0052f85c). 짝수 번호는 직선, 홀수 번호는 대각선이다. (2026-11-01 WB03-W)
export const NEIGHBOR_CELLS = Object.freeze([[0, -1], [1, -1], [1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1]].map((cell) => Object.freeze(cell)));

// 방향 글자 A~P 의 연결 비트이다(북 1·동 2·남 4·서 8, 원본 0x52f910). (2026-11-01 WB03-W)
const CONNECTIONS = Object.freeze([15, 7, 14, 13, 11, 6, 12, 9, 3, 5, 10, 4, 8, 1, 2, 0]);

// 연결 비트(0~15) → 방향 글자 표이다(원본 0x52f8fc `PNOILJFBMHKEGDCA`). (2026-11-01 WB03-W)
const ORIENTATION_LETTERS = 'PNOILJFBMHKEGDCA';

// 청크 방문 번호가 시드에 더해지는 증가량이다(원본 004c117e). (2026-11-01 WB03-W)
const CHUNK_SEED_STEP = 0x10e3;

// 성장에서 연속 실패가 이 횟수에 닿으면 그 청크의 성장을 끝낸다(원본 004c0800). (2026-11-01 WB03-W)
const GROWTH_RETRIES = 1000;

// 청크 지역 난수이다(원본 004bca20): 시드 0 은 0x0bad0bad 로 바꾸고, 다음 상태는 `상태 × 0x10003 + 3` 의 32비트 값, 출력은 상위 16비트를 limit 로 나눈 나머지이다.
// 게임 전역 난수를 소비하지 않는다. 사용: const random = new LocalRandom(시드); random.next(30). (2026-11-01 WB03-W)
export class LocalRandom {
    // 시드는 32비트 부호 없는 정수로 다룬다.
    constructor(seed) {
        // 현재 상태이다.
        this.state = seed >>> 0;
    }

    // 상태를 한 번 전진시키고 0 이상 limit 미만의 값을 돌려준다. limit 가 1 미만이면 오류이다(limit 1 도 상태는 전진한다).
    next(limit) {
        if (!(limit > 0)) throw new RangeError('Terrain random limit must be positive');
        if (this.state === 0) this.state = 0x0bad0bad;
        this.state = (Math.imul(this.state, 0x10003) + 3) >>> 0;
        return (this.state >>> 16) % limit;
    }
}

// 방향 글자 'A'~'P' 의 연결 비트(북 1·동 2·남 4·서 8)를 돌려준다. 그 밖의 글자는 오류이다. (2026-11-01 WB03-W)
export function connectionMask(side) {
    if (typeof side !== 'string' || side.length !== 1 || side < 'A' || side > 'P') throw new RangeError('Invalid terrain side');
    return CONNECTIONS[side.charCodeAt(0) - 0x41];
}

// 연결 비트(하위 4비트만 본다)에 대응하는 방향 글자를 돌려준다. connectionMask 의 역이다. (2026-11-01 WB03-W)
export function orientationLetter(mask) {
    return ORIENTATION_LETTERS[mask & 15];
}

// 마스크의 (x, y) 칸 값이다. 월드 밖은 허공이다. (2026-11-01 WB03-W)
function cellAt(land, x, y) {
    return x >= 0 && y >= 0 && x < WORLD_CELLS && y < WORLD_CELLS ? land[y * WORLD_CELLS + x] : EMPTY_TERRAIN;
}

// 비어 있는 칸에만 지면을 넣고 실제로 추가했으면 1, 아니면 0 을 돌려준다(덮어쓰지 않는다; 월드 밖은 0). (2026-11-01 WB03-W)
function putCell(land, x, y, territory) {
    if (x < 0 || y < 0 || x >= WORLD_CELLS || y >= WORLD_CELLS || land[y * WORLD_CELLS + x] !== EMPTY_TERRAIN) return 0;
    land[y * WORLD_CELLS + x] = territory;
    return 1;
}

// 틈 보정이다(원본 004c0cf0·004c0c30): 월드 바깥 한 줄을 뺀 (1..254)를 y·x 순서로 passes 번 훑어, 직선 이웃에 지면이 있는 빈 칸을 채운다.
// 채울 조건은 북에서 시작해 여덟 이웃을 13번(북서·북이 한 번 더) 돌며 같은 영역이 5칸 넘게 이어지고 그 순간의 방향 번호가 홀수인 것이다. 채운 칸은 곧바로 이어지는 검사에 반영된다.
// land(마스크)를 그 자리에서 고친다. 사용: fillMaskGaps(mask). (2026-11-01 WB03-W)
export function fillMaskGaps(land, passes = 2) {
    for (let pass = 0; pass < passes; pass += 1) {
        for (let y = 1; y < WORLD_CELLS - 1; y += 1) {
            // 직선 이웃에 지면이 있는 빈 칸만 조사한다.
            for (let x = 1; x < WORLD_CELLS - 1; x += 1) {
                if (cellAt(land, x, y) !== EMPTY_TERRAIN) continue;
                if (cellAt(land, x, y - 1) === EMPTY_TERRAIN && cellAt(land, x + 1, y) === EMPTY_TERRAIN && cellAt(land, x, y + 1) === EMPTY_TERRAIN && cellAt(land, x - 1, y) === EMPTY_TERRAIN) continue;
                let run = 0;
                let owner = EMPTY_TERRAIN;
                // 북서·북의 경계에 걸친 연속 이웃도 확인하려고 여덟 이웃을 13번 훑는다.
                for (let direction = 0; direction < 13; direction += 1) {
                    const [dx, dy] = NEIGHBOR_CELLS[direction & 7];
                    const value = cellAt(land, x + dx, y + dy);
                    if (value === EMPTY_TERRAIN) {
                        run = 0;
                        continue;
                    }
                    if (run === 0) owner = value;
                    if (value !== owner) {
                        run = 0;
                        continue;
                    }
                    run += 1;
                    if (run > 4 && (direction & 1) !== 0) {
                        putCell(land, x, y, owner);
                        break;
                    }
                }
            }
        }
    }
}

// 영역 청크 배치(map·islands)로 본섬 칸 마스크를 만든다. 반환: 길이 65536 의 Uint8Array(인덱스 = y × 256 + x, 값 = 그 칸이 속한 영역 번호 0~19, 지면이 없으면 255).
// map 은 territory.js 의 ChunkMap, islands 는 IslandList 이다. 영역 번호는 섬 목록의 territory 이다(섬 번호가 아니다).
// trace 가 있으면(선택, 진단용) 성장한 청크마다 { x, y, territory, target, remaining, attempts, sinceSuccess } 를 trace.growth 에 쌓는다 —
// remaining 은 성장이 끝났을 때 못 채운 목표량, attempts 는 후보 시도 횟수, sinceSuccess 는 마지막 성공 뒤의 시도 횟수(성공이 없었으면 attempts)이다.
// 사용: buildIslandMask(layout.map, layout.islands). (2026-11-01 WB03-W)
export function buildIslandMask(map, islands, trace = null) {
    const land = new Uint8Array(WORLD_CELLS * WORLD_CELLS).fill(EMPTY_TERRAIN);

    // 이 마스크의 칸 조회와 쓰기이다(월드 밖은 허공, 이미 있는 칸은 덮어쓰지 않는다).
    const at = (x, y) => cellAt(land, x, y);
    const put = (x, y, territory) => putCell(land, x, y, territory);

    // 반열린 사각형을 채우고 새로 추가된 칸 수를 돌려준다(원본 004c0930: x 바깥·y 안쪽 순서).
    const fill = (left, top, right, bottom, territory) => {
        let count = 0;
        // 열을 먼저 순회한다.
        for (let x = left; x < right; x += 1) {
            for (let y = top; y < bottom; y += 1) count += put(x, y, territory);
        }
        return count;
    };

    // 섬마다 속한 청크 수와 지금까지 만난 청크 수이다.
    const counts = new Array(128).fill(0);
    const indices = new Array(128).fill(0);
    // 통로와 성장에 쓸 영역별 청크 수를 먼저 센다.
    for (let y = 0; y < map.yLength; y += 1) {
        for (let x = 0; x < map.xLength; x += 1) {
            if (islands.exists(map.at(x, y).island)) counts[map.at(x, y).island] += 1;
        }
    }

    // 청크마다의 작업 항목이다: { x, y, island, index(영역 안 순서), count(영역 청크 수), target(성장 목표 칸 수), territory, seed }.
    const work = [];
    // 통로 생성 순서는 월드 y·x 이다(영역 번호 순서가 아니다).
    for (let y = 0; y < map.yLength; y += 1) {
        for (let x = 0; x < map.xLength; x += 1) {
            const chunk = map.at(x, y);
            if (!islands.exists(chunk.island)) continue;
            const territory = islands.islands[chunk.island].territory;
            const index = indices[chunk.island];
            indices[chunk.island] += 1;
            const mask = connectionMask(chunk.side);
            const left = x * CHUNK_CELLS;
            const top = y * CHUNK_CELLS;
            const seed = chunk.record[3];
            // 중심은 청크 시작에 (4 + (시드 & 7), 4 + ((9999 - 시드) & 7))을 더한 칸이다.
            const cx = left + 4 + (seed & 7);
            const cy = top + 4 + ((9999 - seed) & 7);
            // 중심 4 × 4칸, 그리고 연결 비트가 가리키는 쪽으로 청크 가장자리까지 폭 2칸의 통로이다.
            let added = fill(cx - 1, cy - 1, cx + 3, cy + 3, territory);
            added += fill((mask & 8) !== 0 ? left : cx, (mask & 1) !== 0 ? top : cy, (mask & 2) !== 0 ? left + CHUNK_CELLS : cx + 2, (mask & 4) !== 0 ? top + CHUNK_CELLS : cy + 2, territory);
            // 영역의 두 번째 청크는 추가로 10 × 10칸을 채운다.
            if (index === 1) added += fill(left + 3, top + 3, left + 13, top + 13, territory);
            const random = new LocalRandom(Math.imul(index, CHUNK_SEED_STEP) + seed);
            const target = Math.trunc((Math.max(20, random.next(30) + 50 - Math.trunc((added * 100) / 256)) * 256) / 100);
            work.push({ x, y, island: chunk.island, index, count: counts[chunk.island], target, territory, seed });
        }
    }

    // 모든 통로가 있는 상태에서 청크마다 3 × 3 덩어리를 성장시킨다.
    for (const chunk of work) {
        const random = new LocalRandom(Math.imul(chunk.count + chunk.index, CHUNK_SEED_STEP) + chunk.seed);
        let remaining = chunk.target;
        let retries = GROWTH_RETRIES;
        let attempts = 0;
        let lastSuccess = 0;
        // 연속 실패 한도 또는 목표량에서 끝난다.
        while (remaining > 0 && retries > 0) {
            retries -= 1;
            attempts += 1;
            // 후보 칸은 청크 안의 (0~14, 0~14)이다. 난수는 x, y 에 이어 limit 1 호출도 소비한다.
            const x = chunk.x * CHUNK_CELLS + random.next(15);
            const y = chunk.y * CHUNK_CELLS + random.next(15);
            random.next(1);
            if (at(x, y) !== chunk.territory) continue;
            let added = 0;
            // 성장 덩어리의 행을 먼저 순회한다.
            for (let dy = -1; dy <= 1; dy += 1) {
                // 다른 영역의 청크로는 성장시키지 않는다.
                for (let dx = -1; dx <= 1; dx += 1) {
                    const nx = x + dx;
                    const ny = y + dy;
                    if (nx >= 0 && ny >= 0 && nx < WORLD_CELLS && ny < WORLD_CELLS && map.at(nx >> 4, ny >> 4).island === chunk.island) added += put(nx, ny, chunk.territory);
                }
            }
            if (added > 0) {
                remaining -= added;
                retries = GROWTH_RETRIES;
                lastSuccess = attempts;
            }
        }
        if (trace) (trace.growth ??= []).push({ x: chunk.x, y: chunk.y, territory: chunk.territory, target: chunk.target, remaining, attempts, sinceSuccess: attempts - lastSuccess });
    }

    // 두 번의 순회 중 추가한 칸을 즉시 반영하는 틈 보정이다.
    fillMaskGaps(land);
    return land;
}

// 마스크의 영역별 칸 수를 돌려준다: { 영역 번호: 칸 수 } (지면이 없는 칸은 세지 않는다). (2026-11-01 WB03-W)
export function territoryCellCounts(mask) {
    const counts = {};
    // 모든 칸을 훑는다.
    for (let i = 0; i < mask.length; i += 1) {
        if (mask[i] !== EMPTY_TERRAIN) counts[mask[i]] = (counts[mask[i]] ?? 0) + 1;
    }
    return counts;
}
