// raw 풀 연결 이웃 탐색(RawSquidNeighbors/RawSquidNeighborWalk)을 원본 10.78 관측으로 대조한다(WB04-E39, cpppj SurfaceNeighborCurrentTests·ProductionMaintenanceTests(전이 이웃 경계) 이식).
//  - surface-x86.tsv 의 Neighbor 125장면(flag 8 표면 지도 순회: 원문 발자국 전체를 지도에 놓고 spot/지도 덮어쓰기를 입력 순서대로 적용), surface-neighbor-current-x86.tsv 10.78 16행(첫 반환 뒤 현재 지도/후보/원천 좌표를 바꾸는 원문 순회),
//    neighbor-boundary-x86.tsv 18행(첫 다리 취득의 0/0 을 포함한 유한 경계의 원본 발자국 사각형 16바이트와 첫 이웃).
//  - 계약: 지원하지 않는 flags 거부, 내부 발자국 원천은 시작하지 않음, 방향 계산의 동률/NaN 은 세로.
// 일반 해시 순회(flag 0/4)의 다리 이벤트 장면(다리 이벤트 관측(neighbor 계열)의 Event 행)은 다리 이벤트 단위에서, 울타리 이웃(fence-neighbors 1904행)은 울타리 단위에서 대조한다. (2026-11-08 WB04-E39: raw 풀 이웃 탐색 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { RawSquidNeighborWalk, RawSquidNeighbors } from '../www/core/raw-squid-neighbors.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E39)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const surfaceRows = fixture('surface-x86.tsv');
const currentRows = fixture('surface-neighbor-current-x86.tsv');
const boundaryRows = fixture('neighbor-boundary-x86.tsv');

// 원문 DWORD 읽기(감김), float 비트 변환이다. (2026-11-08 WB04-E39)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const floatView = new DataView(new ArrayBuffer(4));
const floatBits = (value) => { floatView.setFloat32(0, value, true); return floatView.getUint32(0, true); };
const put = (raw, at, value) => { for (let index = 0; index < 4; index += 1) raw[at + index] = (value >>> (8 * index)) & 0xff; };

// 실제 고정 분할을 만족하는 장면 전용 풀, 단계 0 표면 지도, 현재 내부 bit8 필터 입력, 합성 타입/프레임이다(cpppj SurfaceWalkScene). (2026-11-08 WB04-E39)
class SurfaceWalkScene {
    constructor() {
        this.pool = new SidPool(32768, { server: true });
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
        this.frames = Array.from({ length: 188 }, () => []);
        this.ids = new Map();
    }

    // fixture 한 객체를 할당하고 타입/프레임·현재 raw 필드와 발자국 전체 지도만 작성한다.
    add(id, x, y, width, height, flags1, flags2, side, variant, dead, buried) {
        const sid = this.pool.allocate(2);
        this.ids.set(id, sid);
        const type = 70 + id;
        this.types[type] = { flags1, flags2, footX: width, footY: height };
        this.frames[type] = [{ side, variant, number: 1, flags: 0 }];
        const raw = this.pool.allocated(sid);
        raw.fill(0);
        raw[10] = type;
        raw[11] = dead ? 2 : 0;
        put(raw, 14, floatBits(x));
        put(raw, 18, floatBits(y));
        raw[40] = buried ? 8 : 0;
        // 표면 지도는 원본 입력 준비와 같이 뒤 객체가 앞 객체의 칸을 덮는다.
        for (let row = y - height + 1; row <= y; row += 1) {
            for (let column = x - width + 1; column <= x; column += 1) if (column >= 0 && row >= 0 && column < 256 && row < 256) this.hash.setCell(0, column, row, sid);
        }
    }

    // 이웃 반환을 fixture 번호로 되돌려 원본의 순서/중복을 손실 없이 비교한다.
    original(sid) {
        for (const [id, current] of this.ids) if (current === sid) return id;
        throw new Error('표면 이웃 결과의 원문 번호가 없습니다');
    }

    // 실제 Next 를 끝까지 호출하며 호스트 무한 순회가 성공으로 기록되지 않게 한다.
    walk(walker) {
        const result = [];
        for (let sid = walker.current; sid; sid = walker.next()) {
            assert.ok(result.length <= 128, '표면 이웃 검사 입력의 순회 범위 오류');
            result.push(this.original(sid));
        }
        assert.equal(walker.next(), 0);
        return result;
    }
}
const surfaceIds = (text) => (text === '' || text === '-' ? [] : text.split(',').map(number));

// 기존 두 원본의 공통 125장면을 raw 슬롯/현재 지도에서 다시 대조한다.
test('RawSquidNeighbors: 원본10.78 표면 지도 순회 125장면(발자국·필터·중복 캐시)', { skip: !surfaceRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of surfaceRows) {
        if (row[0] !== 'Neighbor') continue;
        const scene = new SurfaceWalkScene();
        // 입력 객체는 원문 발자국 전체를 지도에 놓으며 결과를 미리 필터하지 않는다.
        for (const entry of row[1].split(';')) {
            const f = entry.split(',');
            scene.add(number(f[0]), Number(f[1]), Number(f[2]), Number(f[3]), Number(f[4]), number(f[5]), number(f[6]), number(f[7]), number(f[8]), f[9] === '1', f[10] === '1');
        }
        // spot 과 명시 지도 덮어쓰기를 입력 순서대로 적용한다. 명시 입력은 x/y/원문 번호 또는 spot 비트이다.
        for (const column of [2, 3]) {
            if (row[column] === '-') continue;
            for (const entry of row[column].split(';')) {
                const f = entry.split(',');
                const value = number(f[2]);
                const x = Number(f[0]);
                const y = Number(f[1]);
                if (column === 2) scene.spots[y * 256 + x] = value & 0xff;
                else scene.hash.setCell(0, x, y, value ? scene.ids.get(value) : 0);
            }
        }
        const neighbors = new RawSquidNeighbors(scene.pool, scene.hash, scene.spots, scene.types, scene.frames);
        const walker = new RawSquidNeighborWalk(neighbors, scene.ids.get(number(row[4])), 8);
        assert.deepEqual(scene.walk(walker), surfaceIds(row[5]), `장면 ${count + 1} source=${row[4]} objects=${row[1]}`);
        count += 1;
    }
    assert.equal(count, 125);
});

// 첫 반환 뒤 현재 지도/후보와 원천 좌표를 바꾸는 독립 원문 행을 그대로 재생한다(source 발자국/코드는 생성 시점, 나머지는 Next 시점).
test('RawSquidNeighbors: 원본10.78 순회 중 변경 16행(현재 지도·후보 상태·원천 좌표)', { skip: !currentRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of currentRows) {
        // 원본 10.78 PE 행만 쓴다. CD 10.72(originalCD)는 범위 밖이다.
        if (row[0] !== 'originals') continue;
        const scene = new SurfaceWalkScene();
        // 원문 실행기의 여섯 입력 위치이다.
        const coordinates = [[20, 20], [20, 19], [19, 20], [21, 20], [20, 21], [19, 20]];
        coordinates.forEach(([x, y], index) => scene.add(index + 1, x, y, 1, 1, 0x800, 4, 0x41, 0x50, false, false));
        scene.hash.setCell(0, 19, 20, scene.ids.get(3));
        const neighbors = new RawSquidNeighbors(scene.pool, scene.hash, scene.spots, scene.types, scene.frames);
        const walker = new RawSquidNeighborWalk(neighbors, scene.ids.get(1), number(row[1]));
        assert.equal(scene.original(walker.current), 2);
        const action = number(row[2]);
        if (action === 1) { scene.hash.setCell(0, 19, 20, scene.ids.get(6)); scene.hash.setCell(0, 20, 19, scene.ids.get(5)); }
        else if (action === 2) { scene.pool.allocated(scene.ids.get(3))[11] = 2; scene.pool.allocated(scene.ids.get(4))[40] = 1; }
        else if (action === 3) { const raw = scene.pool.allocated(scene.ids.get(1)); put(raw, 14, floatBits(80)); put(raw, 18, floatBits(80)); }
        assert.deepEqual(scene.walk(walker), surfaceIds(row[3]), `행 ${count + 1} flags=${row[1]} action=${row[2]}`);
        count += 1;
    }
    assert.equal(count, 16);
});

// 입력된 합성 코드 표를 bridgeevent 도구의 변형과 같게 만든다(cpppj Frames(variant)). 판단 결과는 계산하지 않는다. (2026-11-08 WB04-E39)
function frameTable(variant) {
    const letters = 'JKALMNOJKLBJPPCD';
    return [...letters].map((letter, index) => {
        const hard = variant === 1 ? [0, 1, 7, 8, 11].includes(index) : variant === 2 && (index === 0 || index === 8);
        return { side: letter.charCodeAt(0), variant: 0x50, number: index + 1, flags: hard ? 0x40 : letter === 'L' || letter === 'M' ? 0x20 : 0 };
    });
}

// 첫 다리 취득의 0/0 을 포함한 모든 유한 경계를 원본 발자국/실제 첫 이웃과 대조한다.
test('RawSquidNeighbors: 원본10.78 전이 이웃 경계 18행(발자국 사각형 16바이트·첫 이웃)', { skip: !boundaryRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of boundaryRows) {
        assert.equal(row.length, 5);
        const pool = new SidPool(65535, { server: true });
        const source = pool.allocate(2);
        const raw = pool.allocated(source);
        raw.fill(0);
        raw[10] = 82;
        raw[40] = 8;
        put(raw, 14, number(row[0]));
        put(raw, 18, number(row[1]));
        const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
        types[82] = { flags1: 0x800, flags2: 4, footX: number(row[2]) | 0, footY: number(row[2]) | 0 };
        const frames = Array.from({ length: 188 }, () => []);
        frames[82] = frameTable(0);
        const hash = new SquidHash();
        hash.setCell(0, 20, 21, source);
        const neighbors = new RawSquidNeighbors(pool, hash, new Uint8Array(65536), types, frames);
        const foot = neighbors.footprint(source);
        const bytes = Buffer.alloc(16);
        [foot.left, foot.top, foot.right, foot.bottom].forEach((value, index) => bytes.writeUInt32LE(floatBits(value), index * 4));
        assert.equal(bytes.toString('hex'), row[3], `행 ${count + 1} 발자국`);
        assert.equal(new RawSquidNeighborWalk(neighbors, source, 0).current, number(row[4]), `행 ${count + 1} 첫 이웃`);
        count += 1;
    }
    assert.equal(count, 18);
});

// 지원하지 않는 flags 와 방향 계산의 동률/NaN, 잘못된 연결을 거부한다.
test('RawSquidNeighbors: flags·방향·생성 계약', () => {
    const scene = new SurfaceWalkScene();
    scene.add(1, 20, 20, 1, 1, 0x800, 4, 0x41, 0x50, false, false);
    const neighbors = new RawSquidNeighbors(scene.pool, scene.hash, scene.spots, scene.types, scene.frames);
    assert.throws(() => new RawSquidNeighborWalk(neighbors, scene.ids.get(1), 16), RangeError);
    // 이웃이 없으면 첫 이웃 0, 내부 발자국 원천은 시작하지 않는다.
    assert.equal(neighbors.first(scene.ids.get(1)), 0);
    scene.spots.fill(8);
    assert.equal(new RawSquidNeighborWalk(neighbors, scene.ids.get(1), 8).current, 0);
    // 방향: |세로| < |가로| 가 아니면(같음·NaN 포함) 세로 방향이다.
    assert.equal(RawSquidNeighbors.direction(0, 0, 5, 5), 4);
    assert.equal(RawSquidNeighbors.direction(0, 0, -5, -5), 0);
    assert.equal(RawSquidNeighbors.direction(0, 0, 5, 1), 2);
    assert.equal(RawSquidNeighbors.direction(0, 0, -5, 1), 6);
    assert.equal(RawSquidNeighbors.direction(0, 0, 1, Number.NaN), 0);
    // 크기 계약: 지도 크기/타입·프레임 수 불일치는 조회 전에 거부한다.
    assert.throws(() => new RawSquidNeighbors(scene.pool, scene.hash, new Uint8Array(10), scene.types, scene.frames), TypeError);
    assert.throws(() => new RawSquidNeighbors(scene.pool, scene.hash, scene.spots, scene.types, scene.frames.slice(1)), TypeError);
    // 범위 밖 SID 와 자산이 아닌 원천 타입은 거부한다.
    assert.throws(() => neighbors.source(0), RangeError);
    assert.throws(() => neighbors.readObject({ x: 1, y: 1, type: 69, frame: 0 }, false), RangeError);
});
