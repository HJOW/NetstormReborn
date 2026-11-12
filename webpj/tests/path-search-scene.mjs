// 경로 탐색(path-search.test.mjs)과 경로 프로세스 통합 시험(raw-path-process-search.test.mjs)이 함께 쓰는 입력 장면이다. cpppj `tests/RawPathSearchTests.cpp` 의 SearchScene 에 대응한다.
//  - SearchScene: 5~31 번 raw 슬롯과 hash0 표면·spot 입력을 profile 별로 공급한다. 경로 결과는 만들지 않고 입력만 공급한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E349: path-search.test.mjs 에서 분리)
import assert from 'node:assert/strict';
import { floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { RawPathRuntime } from '../www/core/raw-path-runtime.js';
import { RawPathSearch, RawPathSearchState } from '../www/core/raw-path-search.js';

const fround = Math.fround;

// 동일 입력 프레임 목록이며 연결 결과는 실제 접합 판정에 맡긴다(side, variant 글자 번호). (2026-11-12 WB04-E334)
export const FRAMES = [['A', 'A'], ['A', 'P'], ['M', 'A'], ['P', 'P'], ['K', 'A'], ['N', 'A']].map(([side, variant]) => ({ side: side.charCodeAt(0), variant: variant.charCodeAt(0), number: 1, flags: 0 }));

// 장면: 입력 장면의 type/frame/Graph/owner/spot 를 같은 hash0 배열에 넣는다(cpp SearchScene 10.78). 경로 결과를 만들지 않고 입력만 공급한다. (2026-11-12 WB04-E334)
export class SearchScene {
    // 5~31 번 raw 슬롯을 확보하고 82/83/84 번 표면 타입의 코드 표를 독립으로 공급한다(접합 비트는 미리 계산하지 않음).
    constructor() {
        this.pool = new SidPool(32768, { server: false });
        // 실제 allocated raw 슬롯에만 장면 필드를 공급한다.
        for (let sid = 5; sid < 32; sid += 1) assert.equal(this.pool.allocate(2), sid);
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1 }));
        this.frames = Array.from({ length: 188 }, () => []);
        // 표면 타입 82(4)/83(2)/84(0x40000)의 flags1=0x800, flags2, 프레임 코드 여섯 개이다.
        for (const number of [82, 83, 84]) {
            this.types[number].flags1 = 0x800;
            this.types[number].flags2 = number === 82 ? 4 : number === 83 ? 2 : 0x40000;
            this.frames[number] = FRAMES.map((code) => ({ ...code }));
        }
        this.spots = new Uint8Array(65536);
        this.state = new RawPathSearchState();
        this.path = new RawPathRuntime();
        this.start = null;
        this.destination = null;
    }

    // 입력 profile 별 셀/상태 공급이다(cpp Setup). 경로/거리 결과를 만들지 않는다.
    setup(profile, owner, avoid, policy, legacy, guard) {
        this.state.avoidFlagTwo = avoid;
        this.state.policySelector = policy;
        this.state.graphsEnabled = guard !== 1;
        this.state.searchEnabled = guard !== 2;
        const side = profile === 4 ? 1 : profile === 5 ? 3 : 5;
        let next = 6;
        // 각 raw 표면과 지도 셀을 같은 순서로 등록한다.
        for (let y = 10; y < 10 + side; y += 1) {
            // 동일한 행의 모든 현재 후보 필드를 입력으로 공급한다.
            for (let x = 10; x < 10 + side; x += 1) {
                let type = 83;
                let frame = 0;
                let spot = 6;
                let flag = 0;
                let graph = 3;
                const sid = profile === 5 ? 6 : next;
                if (profile === 1 && ((x === 11 && y === 11) || (x === 12 && y === 12) || (x === 13 && y === 13))) spot = 8;
                if (profile === 2 && x === 12) spot = 0;
                if (profile === 3 && (x + y) % 3 === 0) flag = 2;
                if (profile === 6 && x === 12 && y < 14) { type = 82; frame = 4; }
                if (profile === 7 && x === 12 && y === 12) type = 84;
                if (profile === 8 && x === 14 && y === 14) graph = 254;
                if (profile === 9 && x === 14 && y === 14) graph = 4;
                if (profile !== 5 || next === 6) {
                    const raw = this.pool.allocated(sid);
                    raw.fill(0);
                    raw[10] = type;
                    raw[12] = flag;
                    new DataView(raw.buffer, raw.byteOffset, raw.byteLength).setUint32(14, floatToBits(x), true);
                    new DataView(raw.buffer, raw.byteOffset, raw.byteLength).setUint32(18, floatToBits(y), true);
                    raw[30] = graph;
                    raw[34] = owner;
                    new DataView(raw.buffer, raw.byteOffset, raw.byteLength).setUint32(36, frame, true);
                }
                this.hash.setCell(0, x, y, sid);
                this.spots[y * 256 + x] = spot;
                next += 1;
            }
        }
        this.pool.allocated(5)[10] = owner % 2 === 0 ? 158 : 134;
        this.start = { x: fround(9.2), y: fround(9.3) };
        // cpp 의 `9.2F + side - 1` 은 float 연산마다 단정도로 저장한다.
        this.destination = { x: fround(fround(fround(9.2) + side) - 1), y: fround(fround(fround(9.3) + side) - 1) };
        // 기존 경로 한 꼭짓점(시각 12.5)을 둔 상태에서 이어 붙이는 입력이다.
        if (legacy) this.path.setPath([{ x: 9, y: 9 }], [1], [12.5]);
    }

    // 경로의 꼭짓점/방향/QWORD 시각을 원본 관측과 같은 문자열로 만든다. (2026-11-12 WB04-E334)
    points() {
        const parts = [];
        const view = new DataView(new ArrayBuffer(8));
        // 배열 순서로 직접 저장 필드를 조회한다.
        for (let index = 0; index < this.path.count; index += 1) {
            const point = this.path.point(index);
            view.setFloat64(0, this.path.time(index), true);
            parts.push(`${floatToBits(point.x)},${floatToBits(point.y)},${this.path.direction(index)},${view.getBigUint64(0, true)}`);
        }
        return parts.length === 0 ? '-' : parts.join(';');
    }

    // 새 검색기를 만든다. (2026-11-12 WB04-E334)
    searcher(storage = null) {
        return new RawPathSearch(this.pool, this.hash, this.spots, this.types, this.frames, this.state, storage);
    }
}

