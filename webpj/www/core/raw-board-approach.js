// 원본 Boardcoord 의 사각형 가장자리와 같은 Graph 접근점 탐색(41db70)을 옮긴 모듈이다(cpppj `o/RawBoardApproach.cpp/.h`, 10.78 전용).
// 건설 사제의 Graph 순회이며 운반 명령의 일반 접근점(41d530/41d400, `raw-board-target.js`)과 구별된다. Graph 번호 254 도 원본처럼 일반 동등 비교에 포함한다.
//  - find(rectangle, x, y, flags, checking): 사각형 [left, top, right, bottom](단정도, 제자리 수정)의 중심을 향한 출발점 (x, y)의 방향을 4 칸 돌려 고른 변 쪽 중간점에서 시작한다. 출발점의 Graph 번호(graph(x, y))를 기억하고,
//    flags bit2 이면 사각형을 1 만큼 넓히며, 점을 1~255 로 보정하고 flags bit1 이고 북쪽 방향이면 x 를 중심 기준 좌/우 끝으로 옮긴 뒤 사각형 안으로 다시 보정한다. 이어 사각형 **면적(절삭 DWORD)** 번까지 같은 Graph 번호를 찾을 때까지
//    가장자리를 시계 방향으로 한 칸씩 순회한다(점이 가장자리 밖이면 진단을 부르고 왼쪽 위로 복귀). 찾으면 사각형을 원래 값으로 복구하고 그 점을 돌려준다. 못 찾으면 사각형을 1 씩 안쪽으로 줄인 새 사각형의 면적이 4 초과일 때만 같은 방식으로 재귀 탐색하고,
//    아니면 마지막 점을 돌려준다(사각형은 확장된 채 남는다). 좌표가 유한하지 않거나 사각형이 거꾸로면 거부한다.
// graph(x, y)는 실제 462b80 의 현재 Graph 번호, assertion()은 checking 이 켜져 있을 때 가장자리 순회가 경계에 없으면 요청하는 debug 진단(선택된 경계에서만 필요), checking 은 값/getter(호출 시점마다 현재 값을 다시 읽음)이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E104: 건설 접근점 이식으로 추가)
import { RawSquidNeighbors } from './raw-squid-neighbors.js';

// 실제 _ftol signed64 반환의 하위 DWORD(signed)이다. 비유한·범위 밖의 하위 DWORD 도 0 이다. (2026-11-08 WB04-E104)
function integer(value) {
    if (!Number.isFinite(value) || value < -(2 ** 63) || value >= 2 ** 63) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

const fround = Math.fround;

// 실제 41d400 은 대각선이 아닌 네 방향의 중간점을 최종 float 에 저장한다. (2026-11-08 WB04-E104)
function sidePoint(rectangle, side) {
    if (side === 0 || side === 4) return { x: fround((rectangle[2] - rectangle[0]) * 0.5 + rectangle[0]), y: rectangle[side === 0 ? 1 : 3] };
    if (side === 2 || side === 6) return { x: rectangle[side === 2 ? 2 : 0], y: fround((rectangle[3] - rectangle[1]) * 0.5 + rectangle[1]) };
    throw new Error('접근점 41d400 비정상 방향');
}

// 건설 접근점 탐색기이다. (2026-11-08 WB04-E104)
export class RawBoardApproach {
    // 실제 462b80 의 현재 GraphAt 번호 조회이다. (2026-11-08 WB04-E104)
    #graph;
    // checking 에서 가장자리 순회가 경계에 없을 때만 요청하는 진단이다. (2026-11-08 WB04-E104)
    #assertion;

    // 현재 GraphAt 와 선택된 debug assertion 경계를 연결한다(좌표/Graph 사본을 만들지 않고 호출마다 조회). (2026-11-08 WB04-E104)
    constructor(graph, assertion = null) {
        if (typeof graph !== 'function') throw new Error('접근점 실제 GraphAt 미연결');
        this.#graph = graph;
        this.#assertion = assertion;
    }

    // 유효 게임 사각형에 실제 float 저장 위치와 정수 순회 조건을 적용한다. rectangle 은 4칸 배열이며 제자리로 바뀐다. (2026-11-08 WB04-E104)
    find(rectangle, x, y, flags, checking = false) {
        const isChecking = () => Number(typeof checking === 'function' ? checking() : checking) !== 0;
        const r = rectangle;
        if (!Number.isFinite(x) || !Number.isFinite(y) || r[2] < r[0] || r[3] < r[1]) throw new RangeError('접근점 사각형/좌표 오류');
        const original = [r[0], r[1], r[2], r[3]];
        const centerX = fround((r[0] + r[2]) * 0.5);
        const centerY = fround((r[1] + r[3]) * 0.5);
        const side = ((RawSquidNeighbors.direction(x, y, centerX, centerY) - 4) >>> 0) & 7;
        const destinationGraph = this.#graph(x, y);
        if (flags & 4) {
            r[0] = fround(r[0] - 1);
            r[1] = fround(r[1] - 1);
            r[2] = fround(r[2] + 1);
            r[3] = fround(r[3] + 1);
        }
        let point = sidePoint(r, side);
        // 실제 41d0d0 의 1~255 보정 뒤 사각형 보정이 다시 적용된다.
        if (!(point.x >= 1)) point.x = 1;
        if (!(point.y >= 1)) point.y = 1;
        if (point.x > 255) point.x = 255;
        if (!(point.y <= 255)) point.y = 255;
        if ((flags & 2) && !side) point.x = (r[0] + r[2]) * 0.5 <= x ? r[2] : r[0];
        if (r[2] < point.x) point.x = r[2];
        if (point.x < r[0]) point.x = r[0];
        if (r[3] < point.y) point.y = r[3];
        if (point.y < r[1]) point.y = r[1];
        // 원본 반복 상한은 둘레가 아니라 현재 사각형 면적을 절삭한 DWORD 이다.
        let remaining = integer((r[3] - r[1]) * (r[2] - r[0])) >>> 0;
        while (remaining) {
            if (this.#graph(point.x, point.y) === destinationGraph) {
                r[0] = original[0]; r[1] = original[1]; r[2] = original[2]; r[3] = original[3];
                return point;
            }
            if (integer(point.y) === integer(r[1])) {
                if (integer(r[0]) === integer(point.x)) point.y = fround(point.y + 1);
                else point.x = fround(point.x - 1);
            } else if (integer(point.x) === integer(r[2])) {
                point.y = fround(point.y - 1);
            } else if (integer(point.y) === integer(r[3])) {
                point.x = fround(point.x + 1);
            } else if (integer(point.x) === integer(r[0])) {
                point.y = fround(point.y + 1);
            } else {
                if (isChecking()) {
                    if (typeof this.#assertion !== 'function') throw new Error('접근점 가장자리 assertion 미연결');
                    this.#assertion();
                }
                point = { x: r[0], y: r[1] };
            }
            remaining = (remaining - 1) >>> 0;
        }
        const inner = [fround(original[0] + 1), fround(original[1] + 1), fround(original[2] - 1), fround(original[3] - 1)];
        if ((inner[2] - inner[0]) * (inner[3] - inner[1]) > 4) return this.find(inner, x, y, flags, checking);
        return point;
    }
}
