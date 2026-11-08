// AI 배치 스캔 raw-ai-drop-scan.js(RawAiDropScan)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: J 는 위쪽 세 칸 우선과 기준 Y 쓰기 뒤 X 에 공용 난수 3, 전체 BYTE 0 검사(고위 spot 비트도 위쪽을 막음)와 실패 때 난수 무소비,
//    K 의 왼쪽 우선/오른쪽 대체와 선형 행 이웃(좌표 clamp 없음), 다른 TYPE 는 프레임을 읽지 않고 J/K 이외 코드는 난수 무소비, 스캔은 X 선증가와 inclusive 우측/하단 소진 뒤 좌상단 복귀, 빈 지도에서 일반 1000/공급 500 예산과 재개 위치,
//    다리 후보의 5+125+1 과 승인 실패도 125 차감(공용 난수 전진 횟수), 방향별 동맹 행(표면 owner*9 + owner)과 자기 발자국 실패의 추가 10, flags/spot/2c8 관문, 호출별 경계/flags snapshot, 엄격한 개선은 목록 재시작/동률은 추가(미사용 꼬리 보존),
//    가득 찬 30개 동률의 같은 공용 난수 한 칸 교체, 승인 거부 하위가 바꾼 점수/count 를 되돌리지 않음, 후보 하위 누락 시 선증가 진행/난수 보존, 제거의 count 선감소·안정 압축·꼬리 보존과 진단 뒤 계속.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E173: AI 배치 스캔 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiDropScan } from '../www/core/raw-ai-drop-scan.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E173)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 프레임 코드 side 글자를 만든다. (2026-11-09 WB04-E173)
const code = (letter) => ({ side: letter.charCodeAt(0), variant: 0, flags: 0 });

// 실제 본체/슬롯·전체 지도와 원본 공용 난수에 명시 하위 효과를 연결한다. (2026-11-09 WB04-E173)
class DropScanScene {
    // SID 5 는 J/K 다리, SID 6 은 일반 자기 표면이며 원본 진행은 10/10 이다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x52000000, (raw) => raw.fill(0));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.pool = new SidPool(32768, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        this.frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
        this.spots = new Uint8Array(65536);
        this.surfaces = new Uint16Array(65536);
        this.allies = new Uint32Array(81);
        this.random = new GameRandom(0x12345678);
        this.bridge = 82;
        this.bounds = { left: 10, top: 10, right: 246, bottom: 246 };
        this.free = true;
        this.accepted = false;
        this.values = [];
        this.during = null;
        this.frees = 0;
        this.considers = 0;
        this.points = [];
        this.diagnostics = [];
        assert.ok(this.pool.allocate(2) === 5 && this.pool.allocate(2) === 6);
        const first = this.pool.allocated(5);
        const second = this.pool.allocated(6);
        first.fill(0);
        second.fill(0);
        first[10] = 82;
        first[34] = 1;
        second[34] = 1;
        second[10] = 94;
        this.frames[82] = new FrameCodeTable([code('J'), code('K'), code('A')]);
        this.field(0x260, 1);
        this.field(0x2c8, 1);
        this.field(0xaf8, 10);
        this.field(0xafc, 10);
        this.allies[1 * 9 + 1] = 1;
        this.hooks = {
            free: (type) => { assert.equal(type, 80); this.frees += 1; return this.free; },
            consider: (brain, type, point, direction, best, target) => {
                assert.ok(brain === this.token && type === 80 && direction === 2 && !target.value);
                this.points.push(point);
                const call = this.considers;
                this.considers += 1;
                if (this.during) this.during(call);
                if (!this.accepted) return false;
                best.x = point.x;
                best.y = point.y;
                target.value = 9;
                const value = this.values.length === 0 ? 10 : this.values.at(Math.min(call, this.values.length - 1));
                this.field(0x3e8, value >>> 0);
                return true;
            },
            assertion: (text, line) => { assert.ok(text === 'totalChoices > 0' && line === 0x1ac); this.diagnostics.push(line); }
        };
    }

    // 현재 본체 DWORD 를 읽거나(값 없음) 같은 현재 본체 DWORD 만 입력한다. (2026-11-09 WB04-E173)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), at);
        put(this.memory.bytes(this.token), at, value >>> 0);
        return undefined;
    }

    // 현재 다리의 물리 프레임을 선택한다. (2026-11-09 WB04-E173)
    frame(value) { put(this.pool.allocated(5), 36, value); }

    // 명시 좌표의 실제 surface/spot 을 입력한다. (2026-11-09 WB04-E173)
    cell(x, y, surface, spot = 2) {
        this.surfaces[y * 256 + x] = surface;
        this.spots[y * 256 + x] = spot;
    }

    // 현재 선택 배열의 한 항목을 명시 입력한다. (2026-11-09 WB04-E173)
    choice(index, x, y, target) {
        this.field(0x408 + index * 8, floatToBits(x));
        this.field(0x40c + index * 8, floatToBits(y));
        this.field(0x4f8 + index * 4, target);
    }

    // 같은 실제 저장소·난수를 빌린 처리기를 만든다. (2026-11-09 WB04-E173)
    actions() {
        return new RawAiDropScan(this.memory, this.pool, this.types, this.frames, this.spots, this.surfaces, this.allies, this.random, { bridgeType: () => this.bridge, bounds: () => this.bounds }, this.hooks);
    }
}

test('AI 배치 스캔: 다리 J 는 위쪽 세 칸을 우선하며 기준 Y 쓰기 뒤 X 에 공용 난수 3 을 더한다', () => {
    const scene = new DropScanScene();
    const expected = new GameRandom(scene.random.state);
    const draw = expected.next(3);
    const cell = { x: 20, y: 30 };
    assert.ok(scene.actions().adjustBridge(5, cell) && cell.x === 20 + draw && cell.y === 29 && scene.random.state === expected.state);
});

test('AI 배치 스캔: 전체 BYTE 0 을 검사하므로 고위 spot 비트도 위쪽을 막고 아래쪽을 선택하며 실패 때 난수를 쓰지 않는다', () => {
    const scene = new DropScanScene();
    scene.spots[29 * 256 + 20] = 0x40;
    const expected = new GameRandom(scene.random.state);
    const draw = expected.next(3);
    const cell = { x: 20, y: 30 };
    assert.ok(scene.actions().adjustBridge(5, cell) && cell.x === 20 + draw && cell.y === 33);
    const state = scene.random.state;
    scene.spots[31 * 256 + 20] = 1;
    cell.x = 20;
    cell.y = 30;
    assert.ok(!scene.actions().adjustBridge(5, cell) && cell.x === 20 && cell.y === 30 && scene.random.state === state);
});

test('AI 배치 스캔: 다리 K 는 왼쪽 우선/오른쪽 대체와 세로 난수이며 선형 행 이웃을 좌표 clamp 하지 않는다', () => {
    const scene = new DropScanScene();
    scene.frame(1);
    const expected = new GameRandom(scene.random.state);
    const draw = expected.next(3);
    const cell = { x: 0, y: 30 };
    assert.ok(scene.actions().adjustBridge(5, cell) && cell.x === -1 && cell.y === 30 + draw);
    scene.spots[30 * 256 + 19] = 1;
    cell.x = 20;
    cell.y = 30;
    const next = expected.next(3);
    assert.ok(scene.actions().adjustBridge(5, cell) && cell.x === 23 && cell.y === 30 + next);
});

test('AI 배치 스캔: 다른 TYPE 는 프레임을 읽지 않고 J/K 이외 코드는 난수를 쓰지 않는다', () => {
    const scene = new DropScanScene();
    const cell = { x: 20, y: 30 };
    put(scene.pool.allocated(6), 36, 0xffffffff);
    assert.ok(!scene.actions().adjustBridge(6, cell));
    scene.frame(2);
    const state = scene.random.state;
    assert.ok(!scene.actions().adjustBridge(5, cell) && scene.random.state === state);
    scene.frame(0);
    scene.bridge = 83;
    assert.ok(!scene.actions().adjustBridge(5, cell) && scene.random.state === state);
});

test('AI 배치 스캔: X 를 먼저 증가시키고 inclusive 우측/하단 소진 뒤 X/Y 를 좌상단으로 되돌린다', () => {
    const scene = new DropScanScene();
    scene.bounds = { left: 10, top: 10, right: 13, bottom: 10 };
    scene.cell(10, 10, 5);
    assert.ok(!scene.actions().scan(scene.token, 80, 2) && scene.field(0xaf8) === 10 && scene.field(0xafc) === 10 && !scene.considers);
    scene.field(0xaf8, 12);
    scene.cell(13, 10, 5);
    assert.ok(!scene.actions().scan(scene.token, 80, 2) && scene.considers === 1 && scene.points[0].y === 9);
});

test('AI 배치 스캔: 빈 지도에서 일반 1000/공급 500 예산을 정확히 차감하며 재개 위치를 보존한다', () => {
    const normal = new DropScanScene();
    assert.ok(normal.actions().scan(normal.token, 80, 2) && normal.field(0xaf8) === 62 && normal.field(0xafc) === 14);
    const supply = new DropScanScene();
    supply.types[80].flags2 = 0x800;
    assert.ok(supply.actions().scan(supply.token, 80, 2) && supply.field(0xaf8) === 36 && supply.field(0xafc) === 12);
});

test('AI 배치 스캔: 다리 후보 한 번은 5+125+1 을 쓰며 승인 실패도 125 를 차감한다(공용 난수도 그 횟수만큼 전진)', () => {
    // 일반은 8회/공급은 4회 뒤 예산이 소진되며 같은 공용 난수도 그 횟수만큼 전진한다.
    for (const supply of [false, true]) {
        const scene = new DropScanScene();
        scene.surfaces.fill(5);
        if (supply) scene.types[80].flags2 = 0x800;
        const expected = new GameRandom(scene.random.state);
        const count = supply ? 4 : 8;
        for (let index = 0; index < count; index += 1) expected.next(3);
        assert.ok(scene.actions().scan(scene.token, 80, 2) && scene.considers === count && !scene.frees && scene.field(0xaf8) === 10 + count && scene.random.state === expected.state);
    }
});

test('AI 배치 스캔: 동맹 행은 surfaceOwner*9+owner 이며 자기 발자국 실패도 추가 10 을 쓴다', () => {
    const own = new DropScanScene();
    own.surfaces.fill(6);
    own.spots.fill(2);
    own.types[80].flags2 = 0x800;
    own.free = false;
    assert.ok(own.actions().scan(own.token, 80, 2) && own.frees === 32 && !own.considers && own.field(0xaf8) === 42);
    const ally = new DropScanScene();
    ally.surfaces.fill(6);
    ally.spots.fill(2);
    ally.types[80].flags2 = 0x800;
    ally.pool.allocated(6)[34] = 2;
    ally.allies[2 * 9 + 1] = 1;
    assert.ok(ally.actions().scan(ally.token, 80, 2) && !ally.frees && !ally.considers && ally.field(0xaf8) === 42);
    const reverse = new DropScanScene();
    reverse.surfaces.fill(6);
    reverse.spots.fill(2);
    reverse.types[80].flags2 = 0x800;
    reverse.pool.allocated(6)[34] = 2;
    reverse.allies[1 * 9 + 2] = 1;
    assert.ok(reverse.actions().scan(reverse.token, 80, 2) && !reverse.frees && reverse.field(0xaf8) === 94);
});

test('AI 배치 스캔: flags/spot/2c8 관문이 막히면 전체 발자국과 후보 승인을 호출하지 않는다', () => {
    const scene = new DropScanScene();
    scene.bounds = { left: 10, top: 10, right: 13, bottom: 10 };
    scene.types[80].flags2 = 0x800;
    scene.cell(11, 10, 6, 0x82);
    scene.cell(12, 10, 6, 0x42);
    scene.cell(13, 10, 6, 0x40);
    assert.ok(!scene.actions().scan(scene.token, 80, 2) && scene.frees === 2 && scene.considers === 2);
    scene.field(0x2c8, 0);
    scene.frees = 0;
    scene.considers = 0;
    assert.ok(!scene.actions().scan(scene.token, 80, 2) && !scene.frees && !scene.considers);
});

test('AI 배치 스캔: 같은 호출의 경계/시설 flags 는 하위 효과 뒤에도 최초값이며 진행은 본체에 남는다', () => {
    const scene = new DropScanScene();
    scene.bounds = { left: 10, top: 10, right: 13, bottom: 10 };
    scene.types[80].flags2 = 0x800;
    scene.cell(11, 10, 6);
    scene.cell(12, 10, 6);
    scene.cell(13, 10, 6);
    scene.during = (call) => { if (!call) { scene.bounds = { left: 100, top: 100, right: 102, bottom: 102 }; scene.types[80].flags2 = 0; } };
    assert.ok(!scene.actions().scan(scene.token, 80, 2) && scene.frees === 3 && scene.considers === 3 && scene.field(0xaf8) === 10 && scene.field(0xafc) === 10);
});

test('AI 배치 스캔: 엄격한 점수 개선은 목록을 다시 시작하고 동률은 추가한다(미사용 꼬리는 지우지 않는다)', () => {
    const scene = new DropScanScene();
    scene.bounds = { left: 10, top: 10, right: 13, bottom: 10 };
    scene.types[80].flags2 = 0x800;
    scene.accepted = true;
    scene.values = [20, 20, 30];
    scene.field(0x570, 2);
    scene.field(0x3e8, 10);
    scene.choice(0, 1, 2, 7);
    scene.choice(1, 3, 4, 8);
    scene.cell(11, 10, 6);
    scene.cell(12, 10, 6);
    scene.cell(13, 10, 6);
    assert.ok(!scene.actions().scan(scene.token, 80, 2) && scene.field(0x570) === 1 && scene.field(0x3e8) === 30 && bitsToFloat(scene.field(0x408)) === 13 && scene.field(0x4f8) === 9);
    assert.ok(bitsToFloat(scene.field(0x410)) === 12 && scene.field(0x4fc) === 9);
});

test('AI 배치 스캔: 가득 찬 30개 동률 목록은 같은 공용 난수로 한 칸만 교체하고 count 를 증가시키지 않는다', () => {
    const scene = new DropScanScene();
    scene.bounds = { left: 10, top: 10, right: 11, bottom: 10 };
    scene.types[80].flags2 = 0x800;
    scene.cell(11, 10, 6);
    scene.accepted = true;
    scene.field(0x570, 30);
    scene.field(0x3e8, 10);
    // 미선택 29칸의 원문도 보존되는지 각각 비교한다.
    for (let index = 0; index < 30; index += 1) scene.choice(index, 100 + index, 200, 1000 + index);
    const expected = new GameRandom(scene.random.state);
    const selected = expected.next(30);
    assert.ok(!scene.actions().scan(scene.token, 80, 2) && scene.field(0x570) === 30 && scene.random.state === expected.state);
    // 선택된 칸 이외에는 좌표와 표적 어느 것도 재작성하지 않는다.
    for (let index = 0; index < 30; index += 1) {
        assert.equal(bitsToFloat(scene.field(0x408 + index * 8)), index === selected ? 11 : 100 + index);
        assert.equal(scene.field(0x4f8 + index * 4), index === selected ? 9 : 1000 + index);
    }
});

test('AI 배치 스캔: 승인 거부 하위가 변경한 실제 점수/count 도 사본으로 되돌리지 않는다', () => {
    const scene = new DropScanScene();
    scene.bounds = { left: 10, top: 10, right: 11, bottom: 10 };
    scene.types[80].flags2 = 0x800;
    scene.cell(11, 10, 6);
    scene.during = () => { scene.field(0x3e8, 123); scene.field(0x570, 3); };
    assert.ok(!scene.actions().scan(scene.token, 80, 2) && scene.field(0x3e8) === 123 && scene.field(0x570) === 3);
});

test('AI 배치 스캔: 후보 하위가 없으면 선증가된 진행과 이미 소비한 다리 난수는 보존한다', () => {
    const scene = new DropScanScene();
    scene.cell(11, 10, 5);
    scene.hooks.consider = undefined;
    const expected = new GameRandom(scene.random.state);
    expected.next(3);
    assert.ok(throws(() => scene.actions().scan(scene.token, 80, 2)) && scene.field(0xaf8) === 11 && scene.random.state === expected.state && !scene.field(0x570));
});

test('AI 배치 스캔: 선택 제거는 count 선감소 후 XY/표적을 안정 압축하고 꼬리/점수/진행을 보존한다', () => {
    const scene = new DropScanScene();
    scene.field(0x570, 3);
    scene.field(0x3e8, 99);
    scene.choice(0, 1, 2, 7);
    scene.choice(1, 3, 4, 8);
    scene.choice(2, 5, 6, 9);
    scene.actions().remove(scene.token, 1);
    assert.ok(scene.field(0x570) === 2 && bitsToFloat(scene.field(0x410)) === 5 && scene.field(0x4fc) === 9 && bitsToFloat(scene.field(0x418)) === 5 && scene.field(0x500) === 9 && scene.field(0x3e8) === 99 && scene.field(0xaf8) === 10);
    scene.actions().remove(scene.token, 999);
    assert.ok(scene.field(0x570) === 1 && bitsToFloat(scene.field(0x408)) === 1);
});

test('AI 배치 스캔: count 0 진단이 돌아오면 원본처럼 감소를 계속하며 누락 진단은 앞에서 실패한다', () => {
    const scene = new DropScanScene();
    scene.actions().remove(scene.token, 0);
    assert.ok(scene.field(0x570) === 0xffffffff && JSON.stringify(scene.diagnostics) === '[428]');
    scene.field(0x570, 0);
    scene.hooks.assertion = undefined;
    assert.ok(throws(() => scene.actions().remove(scene.token, 0)) && !scene.field(0x570));
    scene.field(0x570, 2);
    assert.ok(throws(() => scene.actions().remove(scene.token, -1)) && scene.field(0x570) === 1);
});

test('AI 배치 스캔 생성자: 본체 소유자/PATCH 월드 크기/난수/전역 조회기가 없으면 거부하고 현재 TYPE 범위 밖을 거부한다', () => {
    const scene = new DropScanScene();
    const globals = { bridgeType: () => 82, bounds: () => scene.bounds };
    assert.ok(throws(() => new RawAiDropScan(undefined, scene.pool, scene.types, scene.frames, scene.spots, scene.surfaces, scene.allies, scene.random, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiDropScan(scene.memory, scene.pool, scene.types.slice(0, 187), scene.frames, scene.spots, scene.surfaces, scene.allies, scene.random, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiDropScan(scene.memory, scene.pool, scene.types, scene.frames, new Uint8Array(10), scene.surfaces, scene.allies, scene.random, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiDropScan(scene.memory, scene.pool, scene.types, scene.frames, scene.spots, new Uint16Array(10), scene.allies, scene.random, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiDropScan(scene.memory, scene.pool, scene.types, scene.frames, scene.spots, scene.surfaces, new Uint32Array(80), scene.random, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiDropScan(scene.memory, scene.pool, scene.types, scene.frames, scene.spots, scene.surfaces, scene.allies, undefined, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiDropScan(scene.memory, scene.pool, scene.types, scene.frames, scene.spots, scene.surfaces, scene.allies, scene.random, { bridgeType: () => 82 }, scene.hooks)));
    assert.ok(throws(() => scene.actions().scan(scene.token, 188, 2)) && scene.actions().pool === scene.pool);
    assert.ok(throws(() => scene.actions().scan(scene.memory.allocate(16), 80, 2)));
});
