// 위치(float 좌표)에서 표면 해시 0단계의 머리 SID 를 읽는 원본 BFS 보조 `RawPathSurfaceAt` 를 옮긴 모듈이다(cpppj `o/RawPathSearch.cpp` 의 RawPathSurfaceAt/Cell/Inside, 10.78 전용).
// 좌표는 float32 로 반올림한 값에 단정도 0.9999 를 배정도로 더한 뒤 0 쪽으로 절삭해 칸을 만든다(x87 중간 덧셈 보존). 칸이 256×256 밖이면 SID 0 이다.
// 이 모듈은 DOM 을 쓰지 않는다. 삭제 Graph(`raw-graph.js`)가 이 함수를 먼저 쓰고, 길 찾기 본체(`raw-path-search.js`)가 같은 칸 계산(`pathCell`)과 표면 조회를 재사용한다.
// (2026-11-08 WB04-E40: RawGraph 삭제 경로의 위치→표면 조회로 추가. 2026-11-12 WB04-E334: RawPathSearch 가 재사용하도록 주석 갱신)

// 지도 한 변의 칸 수와 표면 근사 올림 보정값(float 0.9999)이다. (2026-11-08 WB04-E40)
const BOARD_SIDE = 256;
const CELL_BIAS = Math.fround(0.9999);

// 보드 칸 범위 판정이다. 범위 밖 칸은 원본 포인터 산술 밖에서 SID 0 으로 처리한다. (2026-11-08 WB04-E40)
function inside(x, y) {
    return x >= 0 && y >= 0 && x < BOARD_SIDE && y < BOARD_SIDE;
}

// 좌표 하나를 칸 번호로 바꾼다. 비유한/32비트 정수 밖 값은 RangeError 이다. (2026-11-08 WB04-E40)
export function pathCell(value) {
    const adjusted = Math.fround(value) + CELL_BIAS;
    if (!Number.isFinite(adjusted) || adjusted < -2147483648 || adjusted > 2147483647) throw new RangeError('BFS 표면 좌표');
    return Math.trunc(adjusted) | 0;
}

// 해시 0단계 지도에서 (x, y) 칸의 머리 SID 를 읽는다. 사용: rawPathSurfaceAt(hash, 20, 20). 지도 밖은 0 이다. (2026-11-08 WB04-E40)
export function rawPathSurfaceAt(hash, x, y) {
    const cellX = pathCell(x);
    const cellY = pathCell(y);
    if (!inside(cellX, cellY)) return 0;
    return hash.cell(0, cellX, cellY);
}
