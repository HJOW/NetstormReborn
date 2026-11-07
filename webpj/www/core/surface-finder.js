// flag 8 표면 이웃 탐색(004b23e0/004b1d70/004b1c20/004b1e80)과 두 표면의 프레임/방향 접합 판정(Bridge::Connects 00441e40)을 옮긴 모듈이다(cpppj `o/SquidFinder.cpp/.h`, `o/Bridge.cpp` 의 Connects).
// 일반 사각형/소수 좌표 탐색은 raw-squid-finder.js 에 있다. 이 탐색기는 원본 Squid/Type 에서 읽는 표면 필드만 이름 있는 정수 발자국 스냅샷으로 받는다.
//  - 객체: { id, x, y, width, height, flags1, flags2, frame: { side, variant, number, flags }, dead, buried }. 기준점(x, y)은 발자국의 오른쪽 아래 칸이다.
//  - 지도: 256×256 객체 번호 지도(map)와 spot 바이트(spots). 두 배열은 생성 때 복사한다.
//  - neighbors(id): 내부(spot bit 8 을 발자국 전체 AND)이면 빈 목록. 아니면 발자국을 한 칸 넓힌 사각형을 y/x 순서로 훑되 위·아래 행의 두 모서리를 제외하고, 자기 발자국 안 기준점·표면이 아닌 것·죽은 것·기준점이 내부인 것·접합하지 않는 것을 거르고 중복 없이 번호 순서대로 돌려준다.
//  - connects(from, to): 두 발자국 중심 차(동률이면 세로 방향 먼저)로 방향 0/2/4/6 을 고르고 프레임 문자(짝수 방향은 side, 홀수는 variant)와 flags2 로 보정한 코드의 연결 마스크를 서로 비교한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E12: 표면 이웃 탐색 이식으로 추가)
import { WORLD_CELLS, connectionMask } from '../assets/terrain-mask.js';

// 탐색에 쓰는 원본 비트와 한계값이다: 섬 안쪽 spot 비트(8), 표면 타입 flags1(0x800), 받침 flags2(0x40000), 다리/폭탄(0x104)과 지상 물체(0x1044202) 마스크, 원본 short 비교의 최대 번호. (2026-11-08 WB04-E12)
const INTERIOR_BIT = 8;
export const TYPE_FLAG1_SURFACE = 0x800;
const TYPE_FLAG2_EMPLACEMENT = 0x40000;
const BRIDGE_OR_BOMB_MASK = 0x104;
const LAND_OBJECT_MASK = 0x1044202;
const MAXIMUM_SURFACE_ID = 32767;

// 방향 0..7 별 연결 비트이다(원문 441e40). (2026-11-08 WB04-E12)
const DIRECTION_BITS = Object.freeze([1, 2, 2, 4, 4, 8, 8, 1]);

// 004adea0/0041ce90: 두 발자국 중심 차이의 절댓값 동률이면 세로 방향을 먼저 고른다. 반환은 0(위)/2(오른쪽)/4(아래)/6(왼쪽)이다. (2026-11-08 WB04-E12)
function direction(from, to) {
    const x = to.x - to.width * 0.5 - (from.x - from.width * 0.5);
    const y = to.y - to.height * 0.5 - (from.y - from.height * 0.5);
    if (Math.abs(x) <= Math.abs(y)) return y > 0 ? 4 : 0;
    return x <= 0 ? 6 : 2;
}

// 프레임 코드(side/variant)와 flags2 로 보정한 연결 코드의 마스크를 비교한다. 첫 마스크가 0 이면 두 번째 코드를 조회하지 않는다. (2026-11-08 WB04-E12)
export function bridgeConnects(first, firstFlags2, second, secondFlags2, directionIndex) {
    if (directionIndex < 0 || directionIndex >= 8) throw new RangeError('표면 연결 방향 범위 오류');
    let a = directionIndex % 2 === 0 ? first.side : first.variant;
    let b = directionIndex % 2 === 0 ? second.side : second.variant;
    if (firstFlags2 & TYPE_FLAG2_EMPLACEMENT) a = 0x50;
    if (secondFlags2 & TYPE_FLAG2_EMPLACEMENT) b = 0x50;
    if ((firstFlags2 & BRIDGE_OR_BOMB_MASK) && (secondFlags2 & LAND_OBJECT_MASK)) b = 0x41;
    else if ((secondFlags2 & BRIDGE_OR_BOMB_MASK) && (firstFlags2 & LAND_OBJECT_MASK)) a = 0x41;
    if (a < 0x41 || a > 0x50) throw new RangeError('표면 연결 코드 범위 오류');
    if (!(connectionMask(String.fromCharCode(a)) & DIRECTION_BITS[directionIndex])) return false;
    if (b < 0x41 || b > 0x50) throw new RangeError('표면 연결 코드 범위 오류');
    return (connectionMask(String.fromCharCode(b)) & DIRECTION_BITS[(directionIndex + 4) % 8]) !== 0;
}

// 표면 지도와 점유 비트로 이웃 표면을 찾는 검색기이다. 사용: new SurfaceFinder(objects, map, spots). (2026-11-08 WB04-E12)
export class SurfaceFinder {
    #objects = new Map();
    #map;
    #spots;

    // 발자국이 지도 안에 놓인 정수 스냅샷만 지원한다. 번호·발자국·중복 오류는 어떤 입력도 보존하지 않고 거부한다.
    constructor(objects, map, spots) {
        if (map.length !== WORLD_CELLS * WORLD_CELLS || spots.length !== map.length) throw new RangeError('표면 지도 크기 오류');
        this.#map = Uint16Array.from(map);
        this.#spots = Uint8Array.from(spots);
        // 유효한 배열 번호·발자국·중복 번호를 확인한 뒤 입력 필드를 그대로 보존한다.
        for (const object of objects) {
            if (object.id === 0 || object.id > MAXIMUM_SURFACE_ID || object.width < 1 || object.height < 1 ||
                object.x < object.width - 1 || object.y < object.height - 1 || object.x >= WORLD_CELLS || object.y >= WORLD_CELLS ||
                this.#objects.has(object.id)) throw new RangeError('표면 객체 발자국/번호 오류');
            this.#objects.set(object.id, object);
        }
    }

    // 표면 지도에 살아 있는 객체 배열의 번호가 있어야 한다.
    object(id) {
        const found = this.#objects.get(id);
        if (!found) throw new RangeError('없는 표면 객체');
        return found;
    }

    // 전역 Graph 재구성은 발자국 AND 와 달리 기준점 한 칸의 spot 내부 비트만 읽는다.
    originInterior(id) {
        const object = this.object(id);
        return (this.#spots[object.y * WORLD_CELLS + object.x] & INTERIOR_BIT) !== 0;
    }

    // 열린 방향 검사가 읽는 256×256 표면 번호 지도의 읽기 전용 사본이다(탐색기보다 오래 보관하지 않는다).
    map() { return this.#map; }

    // 일반 삭제 탐색과 flag 8 이웃 탐색이 공유하는 프레임/방향 접합 판정이다.
    static connects(from, to) {
        return bridgeConnects(from.frame, from.flags2, to.frame, to.flags2, direction(from, to));
    }

    // 004ab880: 각 좌표를 1..255 로 자르고 사각형의 모든 spot 비트를 AND 한다(원본은 y 바깥·x 안쪽 순서).
    #interior(object) {
        const clamp = (value) => Math.min(Math.max(value, 1), WORLD_CELLS - 1);
        const left = clamp(object.x - object.width + 1);
        const right = clamp(object.x);
        const top = clamp(object.y - object.height + 1);
        const bottom = clamp(object.y);
        let bits = 0xffff;
        // 각 행의 사각형 전체를 확인한다.
        for (let y = top; y <= bottom; y += 1) for (let x = left; x <= right; x += 1) bits &= this.#spots[y * WORLD_CELLS + x];
        return (bits & INTERIOR_BIT) !== 0;
    }

    // 이웃 지도에는 객체 번호가 들어 있다. 같은 객체가 여러 칸을 차지해도 한 번만 반환한다.
    neighbors(id) {
        const source = this.object(id);
        const result = [];
        if (this.#interior(source)) return result;
        const left = source.x - source.width + 1;
        const top = source.y - source.height + 1;
        // 발자국을 한 칸 넓힌 사각형을 y/x 순서로 훑고 네 모서리는 제외한다.
        for (let y = top - 1; y <= source.y + 1; y += 1) {
            const edge = y === top - 1 || y === source.y + 1;
            // 맨 위/아래 행은 확장 사각형의 두 모서리를 읽지 않는다.
            for (let x = left - 1 + (edge ? 1 : 0); x <= source.x + 1 - (edge ? 1 : 0); x += 1) {
                if (x < 0 || y < 0 || x >= WORLD_CELLS || y >= WORLD_CELLS) continue;
                const candidate = this.#map[y * WORLD_CELLS + x];
                if (candidate === 0) continue;
                const object = this.object(candidate);
                if (left <= object.x && object.x <= source.x && top <= object.y && object.y <= source.y) continue;
                if (((object.flags1 >>> 0) & TYPE_FLAG1_SURFACE) === 0 || object.dead ||
                    (this.#spots[object.y * WORLD_CELLS + object.x] & INTERIOR_BIT) !== 0) continue;
                if (!SurfaceFinder.connects(object, source)) continue;
                if (!result.includes(candidate)) result.push(candidate);
            }
        }
        return result;
    }
}
