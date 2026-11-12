// 경로 탐색 raw-path-search.js(RawPathSearch)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: pathsearch-x86.tsv 의 `originals` 322행(profile 0~9 의 5×5/3×3/1×1 섬 장면 × flags·avoid·policy·owner·legacy·guard 변형): 같은 raw 입력으로 Search 를 실행해
//    결과(성공/거절/진단)·꼭짓점 좌표·방향·QWORD 시각 문자열·거리표 131,072바이트 FNV32·큐 32,768바이트 FNV32·방향 3 선행 비트를 모두 같게 한다. `originalCD` 행(10.72)은 사용하지 않는다.
//  - cpppj 시험 이식: 표면 alias(해시를 바꾸면 SurfaceAt/GraphAt/Search 가 즉시 바뀜), 공유 진단값을 매 호출 다시 읽음(정책 6/5/1/0/0xffffffff).
//  - 웹 계약: 생성자 연결 검사, 공유 소유자의 지연 생성·`initialized` 시점, 큐 넘침(고정 읽기 head·감김 tail), 경로 길이 5000 경계, Graph 번호 0 이 유효, 방향 전역 범위 진단, 프레임 범위 진단, 같은 SID 접합.
// 경로 프로세스(Init/Kernel)를 쓰는 cpppj 시험 둘과 pathinit 32행은 raw-path-process-search.test.mjs 로 옮겼다(WB04-E349, SearchScene 은 path-search-scene.mjs 로 분리). 실제 게임·브라우저 실행이 아니다. (2026-11-12 WB04-E334: 6-P RawPathSearch 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, fnv32 } from './raw-support.mjs';
import { SearchScene } from './path-search-scene.mjs';
import { RawPathRuntime } from '../www/core/raw-path-runtime.js';
import { RawPathSearch, RawPathSearchState, RawPathSearchStorage, RawPathSearchWork } from '../www/core/raw-path-search.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-12 WB04-E334)
const searchRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'pathsearch-x86.tsv'));

// 예외가 나는지 본다. (2026-11-12 WB04-E334)
function throws(action, kind) {
    try { action(); } catch (error) { return kind ? error instanceof kind : true; }
    return false;
}

// 전체 signed WORD 거리표와 큐를 little endian 바이트 관측의 FNV32 로 비교한다. (2026-11-12 WB04-E334)
function checkWork(work, row) {
    assert.equal(fnv32(new Uint8Array(work.distance.buffer, work.distance.byteOffset, work.distance.byteLength)), Number(row[11]), '거리표');
    assert.equal(fnv32(work.queue), Number(row[12]), '큐');
}

test('pathsearch 원본 관측 322행: 결과·꼭짓점·거리표/큐·방향 3 선행 비트', { skip: !searchRows }, () => {
    const rows = searchRows.filter((row) => row[0] === 'originals');
    assert.equal(rows.length, 322);
    // 입력마다 실제 raw 풀/hash0/spot 을 새로 준비한다.
    for (const row of rows) {
        const scene = new SearchScene();
        scene.setup(Number(row[1]), Number(row[5]), Number(row[3]) !== 0, Number(row[4]), Number(row[6]) !== 0, Number(row[7]));
        const search = scene.searcher();
        if (Number(row[9]) !== 0) {
            assert.ok(throws(() => search.search(scene.path, 5, scene.start, scene.destination, Number(row[2]))), `진단 행 ${row.join(' ')}`);
        } else {
            assert.equal(search.search(scene.path, 5, scene.start, scene.destination, Number(row[2])), Number(row[8]) !== 0, `결과 ${row.join(' ')}`);
        }
        assert.equal(scene.points(), row[10], `꼭짓점 ${row.join(' ')}`);
        checkWork(search.work, row);
        assert.equal(scene.state.directionPrerequisite[3], Number(row[13]));
    }
});

test('실제 표면 alias: 해시를 바꾸면 후속 SurfaceAt/GraphAt/Search 가 즉시 바뀌고 사본이 남지 않는다', () => {
    const scene = new SearchScene();
    scene.setup(0, 2, true, 6, false, 0);
    const search = scene.searcher();
    assert.equal(search.surfaceAt(scene.start), 6);
    assert.equal(search.graphAt(scene.start), 3);
    scene.hash.setCell(0, 10, 10, 0);
    assert.equal(search.surfaceAt(scene.start), 0);
    assert.equal(search.graphAt(scene.start), null);
    assert.equal(search.search(scene.path, 5, scene.start, scene.destination, 0), false);
    assert.equal(scene.path.count, 0);
});

test('공유 진단값은 매 호출 다시 읽고(6/5/1/0/0xffffffff) 정책 선택값 5 에서만 방향 3 선행 비트가 5 가 된다', () => {
    const scene = new SearchScene();
    scene.setup(0, 2, true, 5, false, 0);
    const diagnostic = { value: 6 };
    scene.state.sharedPolicySelector = diagnostic;
    const search = scene.searcher();
    // 초기 6·변경 5·카메라 관문 1·그림자 0·비영 전체 비트를 그대로 공급한다.
    for (const value of [6, 5, 1, 0, 0xffffffff]) {
        diagnostic.value = value;
        scene.state.directionPrerequisite[3] = 20;
        search.searchCells(scene.path, 10, 10, 14, 14, false);
        assert.equal(scene.state.directionPrerequisite[3], value === 5 ? 5 : 20);
        // 공유 상자가 있으면 독립 정책 입력(5)은 읽지 않는다.
        assert.equal(scene.state.policySelector, 5);
    }
    // 상자가 없으면 signed 32비트로 줄인 독립 입력만 비교한다(-1 과 0xffffffff 는 5 가 아님, 5 와 2^32+5 는 5).
    scene.state.sharedPolicySelector = null;
    for (const [value, expected] of [[5, 5], [-1, 20], [6, 20], [0xffffffff, 20], [0x100000005, 5]]) {
        scene.state.policySelector = value;
        scene.state.directionPrerequisite[3] = 20;
        search.searchCells(scene.path, 10, 10, 14, 14, false);
        assert.equal(scene.state.directionPrerequisite[3], expected);
    }
});

test('생성자: spot 65,536바이트와 타입/프레임 표 길이가 맞아야 한다', () => {
    const scene = new SearchScene();
    assert.ok(throws(() => new RawPathSearch(scene.pool, scene.hash, new Uint8Array(65535), scene.types, scene.frames, scene.state), RangeError));
    assert.ok(throws(() => new RawPathSearch(scene.pool, scene.hash, new Uint8Array(65537), scene.types, scene.frames, scene.state), RangeError));
    assert.ok(throws(() => new RawPathSearch(scene.pool, scene.hash, scene.spots, scene.types, scene.frames.slice(1), scene.state), RangeError));
    assert.equal(scene.searcher().pool, scene.pool);
});

test('공유 소유자: 작업 배열을 지연 생성해 등록하고 initialized 는 실제 거리표 쓰기에 도달한 뒤에만 true 이다', () => {
    const scene = new SearchScene();
    scene.setup(0, 2, true, 6, false, 0);
    const storage = new RawPathSearchStorage();
    assert.equal(storage.work, null);
    assert.equal(storage.initialized, false);
    const first = scene.searcher(storage);
    assert.ok(storage.work instanceof RawPathSearchWork);
    assert.equal(first.work, storage.work);
    // 두 번째 검색기는 같은 소유자의 같은 배열을 공유한다.
    const second = scene.searcher(storage);
    assert.equal(second.work, storage.work);
    // 범위 밖 칸은 거리표를 쓰기 전에 거부한다.
    assert.ok(throws(() => first.searchCells(scene.path, -1, 0, 14, 14, true), RangeError));
    assert.ok(throws(() => first.searchCells(scene.path, 10, 10, 256, 0, true), RangeError));
    assert.equal(storage.initialized, false);
    // Graph 비허용 거절은 BFS 에 이르기 전이다.
    scene.state.graphsEnabled = false;
    assert.equal(first.search(scene.path, 5, scene.start, scene.destination, 0), false);
    assert.equal(storage.initialized, false);
    scene.state.graphsEnabled = true;
    assert.equal(first.search(scene.path, 5, scene.start, scene.destination, 0), true);
    assert.equal(storage.initialized, true);
    // 이미 만든 소유자 배열을 새 검색기가 다시 만들지 않는다.
    const kept = storage.work;
    scene.searcher(storage);
    assert.equal(storage.work, kept);
    // 독립 검색기는 소유자와 별개의 배열을 쓴다.
    assert.notEqual(scene.searcher().work, storage.work);
});

test('큐 넘침: head 는 감기지 않고 tail 만 0x3fff 로 감겨 첫 항목이 덮인 뒤 읽기 넘침 진단이다', () => {
    const scene = new SearchScene();
    scene.setup(0, 2, true, 6, false, 0);
    // 전체 보드를 같은 표면 6 번의 걷는 칸으로 채운다(같은 SID 는 바로 연결).
    scene.hash.view(0).fill(6);
    scene.spots.fill(6);
    const search = scene.searcher();
    assert.ok(throws(() => search.searchCells(scene.path, 0, 0, 255, 255, true), RangeError));
    assert.notDeepEqual([search.work.queue[0], search.work.queue[1]], [0, 0]);
    assert.equal(scene.path.count, 0);
});

test('경로 길이 5000 경계: 4999 개가 되면 통과, 5000 개가 되면 진단이다(기존 꼭짓점 뒤에 이어 붙임)', () => {
    const preload = (count) => {
        const scene = new SearchScene();
        scene.setup(0, 2, true, 6, false, 0);
        scene.path.setPath(Array.from({ length: count }, (_, index) => ({ x: index % 200, y: 1 })), new Array(count).fill(2), Array.from({ length: count }, (_, index) => index));
        return scene;
    };
    // 시작 칸 == 목적 칸이면 꼭짓점 하나(방향 0)만 이어 붙인다.
    const passing = preload(4998);
    passing.searcher().searchCells(passing.path, 10, 10, 10, 10, true);
    assert.equal(passing.path.count, 4999);
    assert.equal(passing.path.direction(4998), 0);
    assert.equal(passing.path.time(4998), 0);
    assert.equal(passing.path.time(4997), 4997);
    const failing = preload(4999);
    assert.ok(throws(() => failing.searcher().searchCells(failing.path, 10, 10, 10, 10, true), RangeError));
    assert.equal(failing.path.count, 4999);
});

test('Graph 번호 0 은 유효한 번호이며(없음은 254 뿐) 두 번호가 같아야 검색한다', () => {
    const scene = new SearchScene();
    scene.setup(0, 2, true, 6, false, 0);
    // 모든 표면의 Graph 번호를 0 으로 쓰면 둘 다 0 이며 같다.
    for (let sid = 6; sid < 31; sid += 1) scene.pool.allocated(sid)[30] = 0;
    const search = scene.searcher();
    assert.equal(search.graphAt(scene.start), 0);
    assert.equal(search.graphAt(scene.destination), 0);
    assert.equal(search.search(scene.path, 5, scene.start, scene.destination, 0), true);
    // 목적 표면만 0 이고 시작이 다르면 거절한다.
    scene.pool.allocated(6)[30] = 7;
    assert.equal(search.search(scene.path, 5, scene.start, scene.destination, 0), false);
    // 시작 표면의 Graph 가 없으면(254) 거절한다.
    scene.pool.allocated(6)[30] = 254;
    assert.equal(search.search(scene.path, 5, scene.start, scene.destination, 0), false);
});

test('graphAt: 표면 타입이 아니면 진단이고 울타리 표식/바람 궁수 번호는 예외로 허용한다', () => {
    const scene = new SearchScene();
    scene.setup(0, 2, true, 6, false, 0);
    const search = scene.searcher();
    // 타입 번호가 표 밖이면 진단이다.
    scene.pool.allocated(6)[10] = 200;
    assert.ok(throws(() => search.graphAt(scene.start)));
    // 표면이 아닌 일반 타입(flags1 에 표면 비트 없음)은 진단이다.
    scene.pool.allocated(6)[10] = 50;
    assert.ok(throws(() => search.graphAt(scene.start)));
    // 울타리 표식(90)·바람 궁수(140)는 표면 비트 없이도 Graph BYTE 를 읽는다.
    scene.pool.allocated(6)[10] = 90;
    assert.equal(search.graphAt(scene.start), 3);
    scene.pool.allocated(6)[10] = 140;
    assert.equal(search.graphAt(scene.start), 3);
    // 예외 타입 번호는 전역에서 바꿀 수 있다.
    scene.state.windArcherType = 141;
    assert.ok(throws(() => search.graphAt(scene.start)));
    scene.state.fenceMarkType = 140;
    assert.equal(search.graphAt(scene.start), 3);
    // Graph 비허용이면 표면을 읽지 않고 null 이다.
    scene.state.graphsEnabled = false;
    scene.pool.allocated(6)[10] = 200;
    assert.equal(search.graphAt(scene.start), null);
});

test('search: 부모 타입이 표 밖이면 진단이고 flags 의 비트 0 만 cardinalOnly 이다', () => {
    const scene = new SearchScene();
    scene.setup(0, 2, true, 6, false, 0);
    const search = scene.searcher();
    scene.pool.allocated(5)[10] = 188;
    assert.ok(throws(() => search.search(scene.path, 5, scene.start, scene.destination, 0), RangeError));
    scene.pool.allocated(5)[10] = 158;
    // 일반 사제(flags 비트 0 없음)는 대각선 축약을 쓰고 cardinalOnly 는 직선만 쓴다.
    const diagonal = new RawPathRuntime();
    assert.equal(search.search(diagonal, 5, scene.start, scene.destination, 0), true);
    const straight = new RawPathRuntime();
    assert.equal(search.search(straight, 5, scene.start, scene.destination, 1), true);
    assert.ok(diagonal.count < straight.count);
    // 비트 0 이 아닌 상위 비트는 무시한다.
    const other = new RawPathRuntime();
    assert.equal(search.search(other, 5, scene.start, scene.destination, 2), true);
    assert.equal(other.count, diagonal.count);
    const both = new RawPathRuntime();
    assert.equal(search.search(both, 5, scene.start, scene.destination, 3), true);
    assert.equal(both.count, straight.count);
    // kenGo 비활성은 진단(같은 Graph·허용 지면)이다.
    scene.state.searchEnabled = false;
    assert.ok(throws(() => search.search(new RawPathRuntime(), 5, scene.start, scene.destination, 0)));
});

test('방향 전역 범위 진단: BFS 는 앞 네 항목만, 역추적은 cardinalOnly 일 때 네 항목만, 아니면 여덟 항목을 검사한다', () => {
    const scene = new SearchScene();
    scene.setup(0, 2, true, 6, false, 0);
    const search = scene.searcher();
    scene.state.directionOrder[5] = 9;
    // 앞 네 항목만 쓰는 BFS 와 cardinalOnly 역추적은 영향이 없다.
    assert.equal(search.searchCells(new RawPathRuntime(), 10, 10, 14, 14, true), true);
    assert.ok(throws(() => search.searchCells(new RawPathRuntime(), 10, 10, 14, 14, false), RangeError));
    scene.state.directionOrder[5] = 3;
    scene.state.directionOrder[2] = -1;
    assert.ok(throws(() => search.searchCells(new RawPathRuntime(), 10, 10, 14, 14, true), RangeError));
});

test('프레임 코드 범위 진단과 같은 SID 접합', () => {
    const scene = new SearchScene();
    scene.setup(0, 2, true, 6, false, 0);
    const search = scene.searcher();
    // 0 번 SID 는 어느 쪽이든 연결되지 않는다.
    assert.equal(search.connects(0, 6, 0), false);
    assert.equal(search.connects(6, 0, 0), false);
    // 같은 SID 는 프레임을 조회하지 않고 연결된다.
    scene.pool.allocated(6)[36] = 99;
    assert.equal(search.connects(6, 6, 0), true);
    // 다른 SID 는 현재 프레임을 읽으므로 표 밖 프레임은 진단이다.
    assert.ok(throws(() => search.connects(6, 7, 0), RangeError));
    scene.pool.allocated(6)[36] = 0;
    scene.pool.allocated(6)[39] = 0x80;
    assert.ok(throws(() => search.connects(6, 7, 0), RangeError));
    scene.pool.allocated(6)[39] = 0;
    assert.equal(search.connects(6, 7, 0), true);
    // 타입 표 밖 SID 는 프레임 조회에서 진단이다.
    scene.pool.allocated(7)[10] = 200;
    assert.ok(throws(() => search.connects(6, 7, 0), RangeError));
});

test('searchCells: 시작 표면이 0 이면 거리표 시작 칸만 쓰고 false, 큐가 마르면 false', () => {
    const scene = new SearchScene();
    scene.setup(0, 2, true, 6, false, 0);
    const search = scene.searcher();
    scene.hash.setCell(0, 10, 10, 0);
    assert.equal(search.searchCells(scene.path, 10, 10, 14, 14, true), false);
    assert.equal(search.work.distance[10 * 256 + 10], 1);
    assert.equal(scene.path.count, 0);
    // 목적 칸 둘레를 막으면(spot 0) 큐가 마르고 false 이다. 거리표 자체는 이후 호출에서 다시 지워진다.
    scene.hash.setCell(0, 10, 10, 6);
    scene.spots[13 * 256 + 14] = 0;
    scene.spots[14 * 256 + 13] = 0;
    assert.equal(search.searchCells(scene.path, 10, 10, 14, 14, true), false);
    scene.spots[13 * 256 + 14] = 6;
    scene.spots[14 * 256 + 13] = 6;
    assert.equal(search.searchCells(scene.path, 10, 10, 14, 14, true), true);
    assert.equal(search.work.distance[9 * 256 + 9], 0);
});

test('여덟 방향 모두: 중앙에서 두 칸 떨어진 목적까지 첫 방향이 같고(대각선 포함) 끝점이 목적 칸이다', () => {
    const delta = [[0, -1], [1, -1], [1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1]];
    // 방향 번호마다 새 장면에서 열린 5×5 섬의 중앙(12,12)에서 출발한다.
    for (let direction = 0; direction < 8; direction += 1) {
        for (const flags of direction % 2 === 0 ? [0, 1] : [0]) {
            const scene = new SearchScene();
            scene.setup(0, 2, true, 6, false, 0);
            const destination = { x: 12 + 2 * delta[direction][0], y: 12 + 2 * delta[direction][1] };
            assert.equal(scene.searcher().search(scene.path, 5, { x: 12, y: 12 }, destination, flags), true);
            assert.deepEqual([scene.path.count, scene.path.direction(0), scene.path.direction(scene.path.count - 1)], [2, direction, direction], `방향 ${direction} flags ${flags}`);
            assert.deepEqual(scene.path.point(0), { x: 12, y: 12 });
            assert.deepEqual(scene.path.point(scene.path.count - 1), destination);
        }
    }
});

test('비대칭 입력: 가로/세로가 다른 시작과 목적에서 X/Y 가 바뀌지 않는다', () => {
    const scene = new SearchScene();
    scene.setup(0, 2, true, 6, false, 0);
    assert.equal(scene.searcher().search(scene.path, 5, { x: 10, y: 10 }, { x: 14, y: 12 }, 1), true);
    // 동쪽 네 칸 뒤 남쪽 두 칸이다(같은 방향 직진 선호).
    assert.deepEqual([...Array(scene.path.count).keys()].map((index) => [scene.path.point(index).x, scene.path.point(index).y, scene.path.direction(index)]),
        [[10, 10, 2], [14, 10, 2], [14, 10, 4], [14, 12, 4]]);
});

test('같은 방향 직진 선호: 남은 후보가 있어도 이전 방향을 이어 간다(북쪽이 막힌 시작에서 서쪽 직진 뒤 북쪽)', () => {
    const scene = new SearchScene();
    scene.setup(0, 2, true, 6, false, 0);
    // 시작(14,12)의 북쪽 칸(14,11)을 막아 첫 걸음이 서쪽이 되게 한다.
    scene.spots[11 * 256 + 14] = 0;
    assert.equal(scene.searcher().search(scene.path, 5, { x: 14, y: 12 }, { x: 10, y: 10 }, 1), true);
    assert.deepEqual([...Array(scene.path.count).keys()].map((index) => [scene.path.point(index).x, scene.path.point(index).y, scene.path.direction(index)]),
        [[14, 12, 6], [10, 12, 6], [10, 12, 0], [10, 10, 0]]);
});

test('회피 flag: raw+12 의 &6 이 정확히 2 인 후보에만 30 비용을 더하고 avoid 전역이 꺼지면 더하지 않는다', () => {
    const run = (flag, avoid) => {
        const scene = new SearchScene();
        scene.setup(0, 2, avoid, 6, false, 0);
        // 칸 (11,10) 의 표면 SID 는 7 이다(행 우선 6 부터).
        scene.pool.allocated(7)[12] = flag;
        assert.equal(scene.searcher().search(scene.path, 5, { x: 10, y: 10 }, { x: 11, y: 11 }, 1), true);
        return scene.path.direction(0);
    };
    // 동쪽 우선(2)이 기본이며 비용이 오른 동쪽 칸은 남쪽(4)으로 돌아간다.
    assert.equal(run(0, true), 2);
    assert.equal(run(2, true), 4);
    assert.equal(run(3, true), 4);
    assert.equal(run(2, false), 2);
    // &6 이 2 가 아닌 값(6·4·1·7)은 회피하지 않는다.
    for (const flag of [1, 4, 6, 7]) assert.equal(run(flag, true), 2, `flag ${flag}`);
});

test('spot 비트: &6 이 걸을 수 있음이고 &8 은 섬 안쪽 표시로 막는다', () => {
    const run = (spot) => {
        const scene = new SearchScene();
        scene.setup(0, 2, true, 6, false, 0);
        // 가운데 한 줄만 열고 나머지는 막는다. 가운데 칸의 spot 만 바꿔 본다.
        scene.spots.fill(0);
        for (const x of [10, 11, 12]) scene.spots[10 * 256 + x] = 6;
        scene.spots[10 * 256 + 11] = spot;
        return scene.searcher().search(scene.path, 5, { x: 10, y: 10 }, { x: 12, y: 10 }, 1);
    };
    for (const spot of [2, 4, 6, 7, 22]) assert.equal(run(spot), true, `spot ${spot}`);
    for (const spot of [0, 1, 8, 9, 14, 15]) assert.equal(run(spot), false, `spot ${spot}`);
});

test('선행 허용 비트: 직선 방향의 선행 비트도 적용하고(BFS), 허용 비트는 방향마다 누적되며, 역추적 후보가 없으면 진단이다', () => {
    // 동쪽 이동(2)이 북쪽 허용(1)을 요구하면 북쪽이 막힌 한 줄 통로의 동쪽 진행은 거절된다.
    const corridor = (prerequisite) => {
        const scene = new SearchScene();
        scene.setup(0, 2, true, 6, false, 0);
        scene.spots.fill(0);
        for (const x of [10, 11, 12]) scene.spots[10 * 256 + x] = 6;
        for (const [index, value] of prerequisite) scene.state.directionPrerequisite[index] = value;
        return scene.searcher().search(scene.path, 5, { x: 12, y: 10 }, { x: 10, y: 10 }, 1);
    };
    assert.equal(corridor([]), true);
    assert.equal(corridor([[2, 1]]), false);
    // 동쪽(1)·남쪽(5)·서쪽(21) 요구를 북→동→남→서 순서로 누적해 만족하면 네 이웃 모두 거리 2 를 받는다.
    const scene = new SearchScene();
    scene.setup(0, 2, true, 6, false, 0);
    scene.state.directionPrerequisite[2] = 1;
    scene.state.directionPrerequisite[4] = 5;
    scene.state.directionPrerequisite[6] = 21;
    const search = scene.searcher();
    try { search.searchCells(scene.path, 12, 12, 14, 14, true); } catch { /* 역추적 진단은 이 검사의 관심 밖이다 */ }
    assert.deepEqual([search.work.distance[11 * 256 + 12], search.work.distance[12 * 256 + 13], search.work.distance[13 * 256 + 12], search.work.distance[12 * 256 + 11]], [2, 2, 2, 2]);
    // 서쪽 이동이 북쪽 허용을 요구하면 역추적에서 후보가 없어 진단이다.
    const missing = new SearchScene();
    missing.setup(0, 2, true, 6, false, 0);
    missing.state.directionPrerequisite[6] = 1;
    assert.throws(() => missing.searcher().searchCells(missing.path, 10, 10, 14, 12, true), /역추적 후보 없음/);
});

test('searchCells 경계: 보드 밖 Y, 시작 표면 0 의 같은 칸, 소진은 false, 허용 지면의 표면 0 은 진단', () => {
    const scene = new SearchScene();
    scene.setup(0, 2, true, 6, false, 0);
    const search = scene.searcher();
    assert.throws(() => search.searchCells(scene.path, 10, 256, 14, 14, true), RangeError);
    assert.throws(() => search.searchCells(scene.path, 10, 10, 14, 256, true), RangeError);
    assert.throws(() => search.searchCells(scene.path, 10, -1, 14, 14, true), RangeError);
    assert.throws(() => search.searchCells(scene.path, 10, 10, 14, -1, true), RangeError);
    // 시작 칸 == 목적 칸이어도 표면 SID 가 0 이면 곧바로 false 이다.
    scene.hash.setCell(0, 10, 10, 0);
    assert.equal(search.searchCells(scene.path, 10, 10, 10, 10, true), false);
    assert.equal(scene.path.count, 0);
    scene.hash.setCell(0, 10, 10, 6);
    // 목적 둘레를 막으면 큐가 마르며 false 이다(0,0 에 표면이 있어도 빈 큐를 더 읽지 않는다).
    scene.hash.setCell(0, 0, 0, 6);
    scene.spots[10 * 256 + 11] = 0;
    scene.spots[11 * 256 + 10] = 0;
    assert.equal(search.searchCells(scene.path, 14, 14, 10, 10, true), false);
    // 걸을 수 있는 칸인데 표면이 0 이면 진단이다.
    scene.spots[10 * 256 + 11] = 6;
    scene.spots[11 * 256 + 10] = 6;
    scene.hash.setCell(0, 11, 10, 0);
    assert.throws(() => search.searchCells(scene.path, 14, 14, 10, 10, true), /표면0/);
});

test('search 거절: 시작 칸이 걸을 수 없으면 BFS 에 이르지 않고, 목적 칸 spot 은 보지 않는다', () => {
    const scene = new SearchScene();
    scene.setup(0, 2, true, 6, false, 0);
    const storage = new RawPathSearchStorage();
    const search = scene.searcher(storage);
    scene.spots[10 * 256 + 10] = 0;
    assert.equal(search.search(scene.path, 5, scene.start, scene.destination, 0), false);
    assert.equal(storage.initialized, false);
    scene.spots[10 * 256 + 10] = 6;
    scene.spots[14 * 256 + 14] = 0;
    assert.equal(search.search(scene.path, 5, scene.start, scene.destination, 0), true);
    assert.deepEqual(scene.path.point(scene.path.count - 1), { x: 14, y: 14 });
});

test('Graph 번호 254 는 없음이며 양쪽 모두 254 여도 검색하지 않는다', () => {
    const scene = new SearchScene();
    scene.setup(0, 2, true, 6, false, 0);
    for (let sid = 6; sid < 31; sid += 1) scene.pool.allocated(sid)[30] = 254;
    const search = scene.searcher();
    assert.equal(search.graphAt(scene.start), null);
    assert.equal(search.search(scene.path, 5, scene.start, scene.destination, 0), false);
    assert.equal(scene.path.count, 0);
});

test('surfaceAt 는 X/Y 를 구별한다(가로 11.2·세로 10.2 는 칸 (12,11))', () => {
    const scene = new SearchScene();
    scene.setup(0, 2, true, 6, false, 0);
    const search = scene.searcher();
    assert.equal(search.surfaceAt({ x: 11.2, y: 10.2 }), scene.hash.cell(0, 12, 11));
    assert.notEqual(scene.hash.cell(0, 12, 11), scene.hash.cell(0, 11, 12));
});

test('진단 메시지: 표면 타입·프레임·경로 길이·출력 이미지 크기·큐 넘침·방향 전역', () => {
    const scene = new SearchScene();
    scene.setup(0, 2, true, 6, false, 0);
    const search = scene.searcher();
    scene.pool.allocated(6)[10] = 200;
    assert.throws(() => search.graphAt(scene.start), /Graph getter 표면 타입/);
    scene.pool.allocated(6)[10] = 83;
    // 프레임 번호가 코드 개수와 같으면(표 끝) 진단이다.
    scene.pool.allocated(6)[36] = 6;
    assert.throws(() => search.connects(6, 7, 0), { name: 'RangeError', message: /표면 프레임/ });
    scene.pool.allocated(6)[36] = 0;
    // 기존 경로가 4999 개이면 이어 붙이는 순간 최대 길이 진단이다.
    const full = new RawPathRuntime();
    full.setPath(Array.from({ length: 4999 }, () => ({ x: 1, y: 1 })), new Array(4999).fill(0), []);
    assert.throws(() => search.searchCells(full, 10, 10, 10, 10, true), /최대 경로 길이/);
    // 출력 이미지 크기가 경로 이미지와 다르면 같은 진단이다.
    const fake = { bytes: new Uint8Array(10), count: 0, setPath() {} };
    assert.throws(() => search.searchCells(fake, 10, 10, 10, 10, true), /최대 경로 길이/);
    // 열린 전체 보드에서 목적에 이르기 전 head 가 큐 끝에 닿으면 읽기 넘침 진단이다.
    scene.hash.view(0).fill(6);
    scene.spots.fill(6);
    assert.throws(() => search.searchCells(new RawPathRuntime(), 0, 0, 255, 255, true), /큐 읽기 넘침/);
    // 방향 전역이 0..7 을 벗어나면 BFS(앞 네 항목)와 역추적(여덟 항목)이 각자 진단한다.
    const bad = new SearchScene();
    bad.setup(0, 2, true, 6, false, 0);
    bad.state.directionOrder[0] = 8;
    assert.throws(() => bad.searcher().searchCells(new RawPathRuntime(), 10, 10, 14, 14, true), /방향 전역/);
    bad.state.directionOrder[0] = 0;
    bad.state.directionOrder[5] = 8;
    assert.throws(() => bad.searcher().searchCells(new RawPathRuntime(), 10, 10, 14, 14, false), /역추적 방향/);
    bad.state.directionOrder[5] = -1;
    assert.throws(() => bad.searcher().searchCells(new RawPathRuntime(), 10, 10, 14, 14, false), /역추적 방향/);
    // 원본 kenGo 비활성은 진단이다.
    bad.state.directionOrder[5] = 3;
    bad.state.searchEnabled = false;
    assert.throws(() => bad.searcher().search(new RawPathRuntime(), 5, bad.start, bad.destination, 0), /kenGo/);
});

test('RawPathSearchState 기본값: PE 초기 전역과 방향 표', () => {
    const state = new RawPathSearchState();
    assert.equal(state.graphsEnabled, true);
    assert.equal(state.searchEnabled, true);
    assert.equal(state.avoidFlagTwo, true);
    assert.equal(state.policySelector, 6);
    assert.deepEqual(Array.from(state.directionPrerequisite), [0, 5, 0, 20, 0, 80, 0, 65]);
    assert.deepEqual(Array.from(state.directionOrder), [0, 2, 4, 6, 1, 3, 5, 7]);
    assert.equal(state.fenceMarkType, 90);
    assert.equal(state.windArcherType, 140);
    assert.equal(state.sharedPolicySelector, null);
    const work = new RawPathSearchWork();
    assert.equal(work.distance.length, 65536);
    assert.equal(work.queue.length, 32768);
});
