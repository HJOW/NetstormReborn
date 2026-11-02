// 작은 받침 모듈이다: 저장 파일에는 3 × 3 받침의 칸 9개(`noIsland`)만 들어 있고, 받침의 윗면(`island`, 타입 94)과 아래 바위(`islandStalag`, 타입 95)는 저장되지 않고 요새를 읽을 때 만들어진다.
// 원본이 어떤 칸을 윗면의 기준으로 삼는지(= 받침이 몇 개이고 어디에 있는지)를 `noIsland` 오브젝트를 읽는 순서대로 다시 계산한다.
// 복원 기준: cpppj `o/RawIslandPostPop.cpp`(원본 noIsland postPop 004423b0 · 서/북 글자 조회 00442350 · 받침 소유자 전파 0044bd20), 근거 문서 docs/exe/cpp-islandpostpop-reconstruction.md 와 docs/exe/terrain-and-bridges.md(작은 받침 절).
// 규칙 요약(요새를 읽는 중, 칸 하나를 만들 때마다):
//  - 서쪽 칸 (x−1, y) 과 북쪽 칸 (x, y−1) 에 **이미 만든** `noIsland` 칸이 있으면 그 칸의 글자를 읽는다. 서쪽 글자가 C→G, F→C, A→D, B→A, E→H, I→E 이면 그 글자를, 아니면 북쪽 글자가 F→B, B→I 이면 그 글자를,
//    그 밖(칸이 없거나 다른 글자)이면 F 를 자기 글자로 삼는다. 이 글자로 `P` 변형의 번호 1 프레임을 찾아 프레임을 정한다(저장된 프레임은 덮어쓴다).
//  - 그래서 받침 3 × 3 은 행 우선으로 `F C G / B A D / I E H` 가 되고, 왼쪽 위 칸(F)에서 받침 윗면을 만든다: 소유자는 그 칸의 소유자, 위치는 (x + 폭 − 1, y + 높이 − 1) = 받침의 오른쪽 아래 칸 (x + 2, y + 2)(`island` 타입의 발자국 3 × 3).
//    아래 바위도 같은 좌표에 만들어진다. 오른쪽 아래 칸(H)이 만들어지면 3 × 3 칸·윗면·아래 바위의 소유자를 왼쪽 위 칸(F)의 소유자로 맞춘다(저장 게임 읽기 중이 아닐 때).
//  - 칸이 모자라면(받침이 불완전하면) 원본은 진단 문자열만 남기고 계속한다.
// 이 모듈은 칸의 글자·받침 목록까지만 정한다. 윗면·바위의 프레임(소유자 색 번호 1~8 → 윗면 클러스터 0~7, 그 밖은 8)과 그리기는 그림 단계에서 한다. DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다.
// (2026-11-02 WB03-Y: 정적 맵 화면(M2) 3단계(c)로 추가)

// 월드 한 변의 칸 수이다. (2026-11-02 WB03-Y)
const WORLD_CELLS = 256;

// 받침의 한 변 칸 수이다(`island` 타입의 발자국과 같다). (2026-11-02 WB03-Y)
export const SUPPORT_SIZE = 3;

// 받침 3 × 3 칸의 글자이다(행 우선). 왼쪽 위 F 에서 윗면을 만들고 오른쪽 아래 H 에서 소유자를 맞춘다. (2026-11-02 WB03-Y)
export const SUPPORT_ROWS = Object.freeze(['FCG', 'BAD', 'IEH']);

// 받침 윗면(`island`)·아래 바위(`islandStalag`)의 타입 번호이다(10.78 패치판: 70 + 로딩 순서 24·25). (2026-11-02 WB03-Y)
export const ISLAND_TYPE = 94;
export const STALAG_TYPE = 95;

// 받침 칸(`noIsland`)의 타입 번호이다(70 + 로딩 순서 87). (2026-11-02 WB03-Y)
export const NO_ISLAND_TYPE = 157;

// 서쪽 칸의 글자에 따른 자기 글자이다(원본 분기 순서: C→G, F→C, A→D, B→A, E→H, I→E). (2026-11-02 WB03-Y)
const FROM_WEST = Object.freeze({ C: 'G', F: 'C', A: 'D', B: 'A', E: 'H', I: 'E' });

// 북쪽 칸의 글자에 따른 자기 글자이다(F→B, B→I, 그 밖은 F). (2026-11-02 WB03-Y)
const FROM_NORTH = Object.freeze({ F: 'B', B: 'I' });

// 서쪽·북쪽 칸의 글자(없으면 null)로 자기 글자를 정한다(원본 004423b0 의 분기 순서). 사용: supportLetter('C', null) → 'G'. (2026-11-02 WB03-Y)
export function supportLetter(west, north) {
    if (west !== null && Object.hasOwn(FROM_WEST, west)) return FROM_WEST[west];
    if (north !== null && Object.hasOwn(FROM_NORTH, north)) return FROM_NORTH[north];
    return 'F';
}

// 요새 배치(territory.js layoutFort 의 결과)에서 `noIsland` 오브젝트를 **원본이 만드는 순서**로 모은다: 먼저 Chaff(월드 청크 y·x 순서), 이어서 영역 0~19 의 청크 레코드(영역 안의 청크 순서)이며
// 청크 안에서는 파일에 저장된 순서이다. 반환: [{ x, y, storedOwner, storedFrame, territory(영역 번호, Chaff 면 -1) }]. 사용: collectNoIslandObjects(layout). (2026-11-02 WB03-Y)
export function collectNoIslandObjects(layout, noIslandType = NO_ISLAND_TYPE) {
    const found = [];
    // 청크 하나의 오브젝트에서 noIsland 만 월드 칸 좌표로 모은다.
    const collect = (chunkX, chunkY, chunk, territory) => {
        if (!chunk) return;
        for (const object of chunk.objects) {
            if (object.type !== noIslandType) continue;
            found.push({ x: chunkX * 16 + object.cellX, y: chunkY * 16 + object.cellY, storedOwner: object.owner, storedFrame: object.frame, territory });
        }
    };
    // Chaff 의 청크는 월드 y 바깥·x 안쪽 순서이다.
    for (const { x, y, chunk } of layout.chaff) collect(x, y, chunk, -1);
    // 영역마다 놓인 청크 순서대로 저장된 청크 레코드를 읽는다.
    for (const region of layout.regions) for (const { x, y, stored } of region.chunks) collect(x, y, stored, region.territory);
    return found;
}

// `noIsland` 칸을 읽는 순서대로 글자를 정하고 받침을 찾는다. 입력은 collectNoIslandObjects 의 결과이다.
// 반환: { letters(Map: 칸 y × 256 + x → 글자), cells[{ x, y, letter, storedOwner, storedFrame, territory }](읽은 순서), supports[], orphanCells(어느 받침의 3 × 3 글자 배치에도 맞지 않는 칸 수; 불완전한 받침에 맞는 칸은 센다) }.
// supports 의 항목: { x, y(왼쪽 위 F 칸), anchorX, anchorY(윗면·바위의 기준 = 오른쪽 아래 칸), owner(F 칸의 저장 소유자 바이트, 없으면 null), territory, complete(9칸의 글자가 SUPPORT_ROWS 와 같음), missing(없거나 글자가 다른 칸 수) }.
// 같은 칸에 `noIsland` 가 둘 이상이면 처음 것만 지도에 남는다(원본의 표면 지도 머리 하나). 사용: findSupports(collectNoIslandObjects(layout)). (2026-11-02 WB03-Y)
export function findSupports(objects) {
    const letters = new Map();
    const cells = [];
    const supports = [];
    // 읽는 순서대로 칸마다 서쪽·북쪽 글자를 보고 자기 글자를 정한다.
    for (const object of objects) {
        const key = object.y * WORLD_CELLS + object.x;
        // 왼쪽·위쪽 가장자리 칸은 월드 밖 이웃이 없다(한 줄 위의 오른쪽 끝 칸을 서쪽으로 잘못 읽지 않는다).
        const west = object.x > 0 ? (letters.get(key - 1) ?? null) : null;
        const north = object.y > 0 ? (letters.get(key - WORLD_CELLS) ?? null) : null;
        const letter = supportLetter(west, north);
        const duplicate = letters.has(key);
        if (!duplicate) letters.set(key, letter);
        cells.push({ x: object.x, y: object.y, letter, storedOwner: object.storedOwner, storedFrame: object.storedFrame, territory: object.territory });
        // 같은 칸의 두 번째 오브젝트는 표면 지도에 남지 않으므로 받침을 만들지 않는다.
        if (letter === 'F' && !duplicate) {
            supports.push({ x: object.x, y: object.y, anchorX: object.x + SUPPORT_SIZE - 1, anchorY: object.y + SUPPORT_SIZE - 1, owner: object.storedOwner, territory: object.territory, complete: false, missing: 0 });
        }
    }
    const covered = new Set();
    // 받침마다 3 × 3 칸의 글자가 F C G / B A D / I E H 인지 본다.
    for (const support of supports) {
        let missing = 0;
        // 9칸을 행 우선으로 본다.
        for (let dy = 0; dy < SUPPORT_SIZE; dy += 1) {
            for (let dx = 0; dx < SUPPORT_SIZE; dx += 1) {
                const key = (support.y + dy) * WORLD_CELLS + support.x + dx;
                if (support.x + dx < WORLD_CELLS && support.y + dy < WORLD_CELLS && letters.get(key) === SUPPORT_ROWS[dy][dx]) covered.add(key);
                else missing += 1;
            }
        }
        support.missing = missing;
        support.complete = missing === 0;
    }
    return { letters, cells, supports, orphanCells: cells.filter((cell) => !covered.has(cell.y * WORLD_CELLS + cell.x)).length };
}
