// 중앙섬 군도의 실제 지면 연결을 섬 목록의 독립 지역으로 나누는 모듈이다(cpppj `o/RawIslandPartition.cpp/.h`, 원본 10.78 0046f7d0 군도 분리·0046ed50 flood·0046ec00 지역).
// 사용: new RawIslandPartition(pool, surfaces, frames, terrainType, islands, noteWorldRedraw, getRegion, setRegion).split() → 새로 만든 섬(지역) 수.
//  - pool: 실제 SidPool(10.78), surfaces: 해시 0 의 65536칸 Uint16Array(256×256 표면 SID, y 행 우선), frames: 타입별 프레임 코드 배열 표(188칸), terrainType(): 현재 지면 TYPE 전역 getter,
//    islands: 같은 풀의 RawIslandRuntime(100기록 add/setBounds/count), noteWorldRedraw(): flood 한 번이 끝날 때마다 월드 다시 그리기 전역을 1 올리는 함수,
//    getRegion(sid)/setRegion(sid, region): 현재 TYPE 진단·다리·noRegion 관문을 거치는 실제 지역 getter/setter(raw-squid-region.js).
//  - split(): y 행 → x 열 순서로 표면을 훑어 현재 지면 TYPE 이고 지역이 0 인 첫 칸마다 새 섬을 `add(0, 0xff)` 로 만들고, 그 지면의 raw X/Y 를 절삭한 좌표에서 북·동·남·서 연결 마스크를 따라 같은 지역으로 flood 하며 경계를 누적한다.
// 원본의 재귀 flood 는 호스트 작업 스택으로 같은 방문 순서(역순 push 로 북→동→남→서)를 보존한다. 방문 기록은 별도 집합이 아니라 실제 raw 지역 WORD 이다. 새 기록의 Temple/padding 은 초기화하지 않고 add 와 같은 필드만 쓴다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-20 WB04-E1104: 12차 스커미시 중앙섬 군도 분리 이식)
import { connectionMask } from '../assets/terrain-mask.js';

// 지도 한 변의 칸 수와 표면 표의 칸 수이다(256×256). (2026-11-20 WB04-E1104)
const WORLD_SIDE = 256;
const SURFACE_COUNT = WORLD_SIDE * WORLD_SIDE;
// 필수 하위 장치가 없으면 그 위치에서 거부한다. (2026-11-20 WB04-E1104)
function need(value, label) {
    if (typeof value !== 'function') throw new TypeError(`군도 분리의 ${label} 미연결`);
    return value;
}
// 원문 PartitionCoordinate 이다: x86 _ftol 의 부호 있는 64비트 유효 범위 밖(비유한 포함)은 0, 그 안은 절삭한 하위 32비트(signed)이다. (2026-11-20 WB04-E1104)
function partitionCoordinate(value) {
    const bound = 9223372036854775808;
    if (!Number.isFinite(value) || value < -bound || value >= bound) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 군도 분리 부모이다. 생성자는 판본·풀 동일성·표 크기를 확인하고 모든 장치를 보관한다(Build 의 동기 호출 동안만 쓴다). (2026-11-20 WB04-E1104)
export class RawIslandPartition {
    #pool;
    #surfaces;
    #frames;
    #terrainType;
    #islands;
    #noteWorldRedraw;
    #getRegion;
    #setRegion;

    // 원문 생성자의 검사: 10.78 판본, 섬 목록과 같은 풀, 65536칸 표면 표, 두 지역 함수이다. (2026-11-20 WB04-E1104)
    constructor(pool, surfaces, frames, terrainType, islands, noteWorldRedraw, getRegion, setRegion) {
        if (pool?.edition !== 'patch1078' || islands?.pool !== pool || surfaces?.length !== SURFACE_COUNT || typeof getRegion !== 'function' || typeof setRegion !== 'function') {
            throw new TypeError('군도 분리의 실제 10.78 Pool/표면/지역 장치 불일치');
        }
        this.#pool = pool;
        this.#surfaces = surfaces;
        this.#frames = frames;
        this.#terrainType = need(terrainType, '현재 지면 TYPE getter');
        this.#islands = islands;
        this.#noteWorldRedraw = need(noteWorldRedraw, '월드 다시 그리기 통지');
        this.#getRegion = getRegion;
        this.#setRegion = setRegion;
    }

    // 월드 밖이거나 빈 칸은 0 이다. free/dead/owner 필터를 더하지 않는다. (2026-11-20 WB04-E1104)
    #surface(x, y) {
        return x < 0 || y < 0 || x >= WORLD_SIDE || y >= WORLD_SIDE ? 0 : this.#surfaces[y * WORLD_SIDE + x];
    }

    // 지면 SID 의 raw 슬롯에서 float32 필드(오프셋 14: X, 18: Y)나 signed 32비트 필드(오프셋 36: 프레임)를 읽는다. (2026-11-20 WB04-E1104)
    #view(raw) { return new DataView(raw.buffer, raw.byteOffset, raw.byteLength); }

    // (x, y) 에서 시작해 같은 지면 TYPE 의 연결을 북/동/남/서 순서로 지역으로 지정하고 좌표 경계를 누적한다. 한 번 끝날 때마다 월드 다시 그리기를 올린다. (2026-11-20 WB04-E1104)
    #flood(x, y, region, codes) {
        const bounds = { left: 257, top: 257, right: -1, bottom: -1 };
        const work = [[x, y]]; // 호스트 작업 스택(원문 재귀 깊이 우선 순서를 역순 push 로 보존한다)
        // 스택이 빌 때까지 한 칸씩 방문한다.
        while (work.length) {
            const point = work.pop();
            const sid = this.#surface(point[0], point[1]);
            if (!sid) continue;
            let raw = this.#pool.slot(sid);
            if (raw[10] !== this.#terrainType() || this.#getRegion(sid) === region) continue;
            this.#setRegion(sid, region);
            bounds.left = Math.min(bounds.left, point[0]);
            bounds.top = Math.min(bounds.top, point[1]);
            bounds.right = Math.max(bounds.right, point[0]);
            bounds.bottom = Math.max(bounds.bottom, point[1]);
            raw = this.#pool.slot(sid); // 지역 쓰기 뒤의 현재 프레임을 다시 읽는다.
            const frame = this.#view(raw).getInt32(36, true);
            if (frame < 0 || frame >= codes.length) throw new RangeError('군도 flood의 실제 프레임 코드 범위');
            const mask = connectionMask(String.fromCharCode(codes[frame].side & 0xff));
            if (mask & 8) work.push([point[0] - 1, point[1]]);
            if (mask & 4) work.push([point[0], point[1] + 1]);
            if (mask & 2) work.push([point[0] + 1, point[1]]);
            if (mask & 1) work.push([point[0], point[1] - 1]);
        }
        this.#noteWorldRedraw();
        return bounds;
    }

    // 표면을 y 행 → x 열 순서로 훑어 새 섬을 만들고 flood 한다. 만든 섬 수를 돌려준다. 원문은 새 기록의 Temple/padding 을 초기화하지 않는다. (2026-11-20 WB04-E1104)
    split() {
        let count = 0;
        // 원문 바깥 루프는 해시 0 의 각 256칸 행이다.
        for (let y = 0; y < WORLD_SIDE; y += 1) {
            // 같은 행을 왼쪽부터 읽으며 TYPE 일치·현재 지역 0 인 칸만 새 component 후보로 삼는다.
            for (let x = 0; x < WORLD_SIDE; x += 1) {
                const sid = this.#surface(x, y);
                if (!sid) continue;
                const raw = this.#pool.slot(sid);
                const terrainType = this.#terrainType();
                if (raw[10] !== terrainType || this.#getRegion(sid) !== 0) continue;
                if (terrainType >= this.#frames.length) throw new RangeError('군도 분리의 현재 지면 TYPE 범위');
                const codes = this.#frames[terrainType];
                const region = this.#islands.add(0, 0xff);
                const view = this.#view(raw);
                // 원문은 스캔 칸이 아닌 현재 지면의 raw X/Y 를 절삭해 flood 시작점으로 쓴다.
                const bounds = this.#flood(partitionCoordinate(view.getFloat32(14, true)), partitionCoordinate(view.getFloat32(18, true)), region, codes);
                this.#islands.setBounds(region, bounds);
                count += 1;
            }
        }
        return count;
    }
}
