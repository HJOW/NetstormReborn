// AI 다리 후보 준비/시작점 raw-ai-bridge-candidates.js(RawAiBridgeCandidates)를 실제 PE 관측 fixture 와 cpppj 계약으로 대조한다.
//  - 후보 준비: ai-bridge-candidates-x86.tsv 의 10.78 210장면/220반환(24칸 예산·최대 6개·연결비트·재개·전체 본체 0xb08 바이트). 열 = 지도 모드, 지면 TYPE, 프레임 코드 side, 시작 x, y, 단계, count, 방향, 변 길이, graph, owner, 반복 호출 수, 본체 채움 BYTE, 반환, 전체 본체 hex, 방문 수.
//  - 시작점: ai-bridge-start-x86.tsv 의 399행(같은 거리 동률/분수 경계/현재 전체 DWORD 비교). 열 = 지도 모드, TYPE, side, 경계 float 비트 4개, 목표 float 비트 2개, graph, owner, 현재 다리 TYPE, 결과 XY 비트(8바이트 hex).
//  - 실제 입력(지도/TYPE/슬롯)만 설치하고 결과는 모듈이 계산하며 본체 불변(시작점)과 입력 지도 보존을 확인한다. cpppj 의 Path 와 조립하는 두 시험(초기화+예산 Step, 시작점+Advance)은 RawAiBridgePath 단위에서 닫는다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E176: AI 다리 후보 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiBridgeCandidates } from '../www/core/raw-ai-bridge-candidates.js';

// 실제 PE 관측 행 수와 열 수이다. (2026-11-09 WB04-E176)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const candidateRows = readRows(path.join(FIXTURES, 'ai-bridge-candidates-x86.tsv')) ?? [];
const startRows = readRows(path.join(FIXTURES, 'ai-bridge-start-x86.tsv')) ?? [];

// 예외가 나는지 본다. (2026-11-09 WB04-E176)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E176)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원본에 공급한 지도/TYPE/슬롯 입력만 만들고 후보 결과는 제품 몸체가 계산한다. (2026-11-09 WB04-E176)
class CandidateScene {
    // 실제 슬롯을 확보하고 caller 입력을 각 필드에만 적용한다.
    constructor(row) {
        this.memory = new RawAiBrainMemory(0x55000000, (raw) => raw.fill(0));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.pool = new SidPool(32768, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0 }));
        this.frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
        this.surfaces = new Uint16Array(65536);
        this.spots = new Uint8Array(65536);
        this.bridge = 82;
        this.fencemark = 90;
        this.windArcher = 140;
        // 원본 격리 입력과 같은 source 50 을 확보한다.
        for (let number = 5; number <= 50; number += 1) assert.equal(this.pool.allocate(2), number);
        const numbers = row.map((value) => (value === '' ? 0 : Number(value)));
        const mode = numbers[0];
        const kind = numbers[1];
        const x = numbers[3] | 0;
        const y = numbers[4] | 0;
        this.memory.bytes(this.token).fill(numbers[12] & 0xff);
        put(this.memory.bytes(this.token), 0x5f8, numbers[9]);
        put(this.memory.bytes(this.token), 0x5fc, numbers[10]);
        put(this.memory.bytes(this.token), 0x690, numbers[6]);
        put(this.memory.bytes(this.token), 0x694, numbers[7]);
        put(this.memory.bytes(this.token), 0x698, numbers[5]);
        put(this.memory.bytes(this.token), 0x69c, numbers[8]);
        put(this.memory.bytes(this.token), 0x6a0, numbers[3]);
        put(this.memory.bytes(this.token), 0x6a4, numbers[4]);
        // 원본 관측 도구와 같은 입력 지도만 행·열 순서로 구성한다.
        for (let cy = 0; cy < 256; cy += 1) {
            for (let cx = 0; cx < 256; cx += 1) {
                let present = mode !== 0 && (mode !== 4 || (cx === x && cy === y));
                if (mode === 3) present = (cx + cy) % 3 !== 0;
                const at = cy * 256 + cx;
                if (present) this.surfaces[at] = 50;
                if (mode === 2) this.spots[at] = 1;
                else if (mode === 3) this.spots[at] = (cx + cy) % 3 === 0 ? 0x80 : 0;
                else if (mode === 5) this.spots[at] = ((cx === x && cy === y - 1) || (cx === x + 1 && cy === y)) ? 0x40 : 0;
            }
        }
        const raw = this.pool.allocated(50);
        raw.fill(0);
        raw[10] = kind;
        raw[30] = 7;
        raw[34] = 2;
        this.types[kind].flags1 = 0x800;
        this.frames[kind] = new FrameCodeTable([{ side: numbers[2], variant: 65, flags: 0 }]);
    }

    // 같은 현재 월드/본체를 참조하는 실제 준비 모듈이다. (2026-11-09 WB04-E176)
    candidates() {
        return new RawAiBridgeCandidates(this.memory, this.pool, this.types, this.frames, this.surfaces, this.spots, { bridge: () => this.bridge, fencemark: () => this.fencemark, windArcher: () => this.windArcher });
    }
}

test('AI 다리 후보: 실제 PE 210장면/220반환의 24칸 예산·최대 6개·연결비트·재개·전체 본체를 대조한다', () => {
    assert.equal(candidateRows.length, 210);
    let returns = 0;
    // 독립 caller 입력만 설치하고 원문 결과를 재계산하지 않는다.
    candidateRows.forEach((row, index) => {
        assert.equal(row.length, 16);
        const scene = new CandidateScene(row);
        const candidates = scene.candidates();
        let result = false;
        // 같은 상태를 실제 다음 호출에 이어 재개한다.
        for (let repeat = 0; repeat < Number(row[11]); repeat += 1) {
            result = candidates.prepare(scene.token);
            returns += 1;
        }
        assert.equal(result, Number(row[13]) !== 0, `반환 행 ${index}`);
        assert.equal(hex(scene.memory.bytes(scene.token)), row[14], `전체 본체 행 ${index}`);
    });
    assert.equal(returns, 220);
});

test('AI 다리 시작점: 실제 PE 399행의 같은 거리 동률/분수 경계/현재 전체 DWORD 비교와 본체 불변을 대조한다', () => {
    assert.equal(startRows.length, 399);
    startRows.forEach((row, index) => {
        assert.equal(row.length, 13);
        const scene = new CandidateScene(['0', row[1], row[2], '50', '50', '2', '0', '0', '1', '7', '2', '1', '165', '0', '', '0']);
        const mode = Number(row[0]);
        // 원본 관측의 현재 지도 입력만 행 순서로 구성한다(비대칭 지형/점유로 X/Y 전도를 검출한다).
        for (let y = 0; y < 256; y += 1) {
            for (let x = 0; x < 256; x += 1) {
                const at = y * 256 + x;
                scene.surfaces[at] = mode && (mode !== 3 || (x + 3 * y) % 5 !== 0) ? 50 : 0;
                scene.spots[at] = mode === 2 ? 0x80 : (mode === 3 && (x + 3 * y) % 5 === 0 ? 0x40 : 0);
            }
        }
        scene.bridge = Number(row[11]);
        const body = scene.memory.bytes(scene.token);
        put(body, 0x298, Number(row[9]));
        put(body, 0x260, Number(row[10]));
        const before = hex(body);
        const bounds = { left: bitsToFloat(Number(row[3])), top: bitsToFloat(Number(row[4])), right: bitsToFloat(Number(row[5])), bottom: bitsToFloat(Number(row[6])) };
        const point = scene.candidates().start(scene.token, { x: bitsToFloat(Number(row[7])), y: bitsToFloat(Number(row[8])) }, bounds);
        const result = new Uint8Array(8);
        put(result, 0, floatToBits(point.x));
        put(result, 4, floatToBits(point.y));
        assert.equal(hex(result), row[12], `시작점 행 ${index}`);
        assert.equal(hex(body), before, `본체 불변 행 ${index}`);
    });
});

test('AI 다리 후보 협력자 접근: 같은 월드를 공유하고 본체 필드 쓰기는 주변을 보존하며 물리 밖을 거부한다', () => {
    const scene = new CandidateScene(['0', '97', '65', '50', '50', '2', '0', '0', '1', '7', '2', '1', '165', '0', '', '0']);
    const candidates = scene.candidates();
    assert.ok(candidates.pool === scene.pool && candidates.surfaces === scene.surfaces && candidates.frames === scene.frames && candidates.bridgeType === 82);
    const before = Buffer.from(scene.memory.bytes(scene.token));
    candidates.setField(scene.token, 0x6a0, 0x11223344);
    assert.equal(candidates.field(scene.token, 0x6a0), 0x11223344);
    const after = Buffer.from(scene.memory.bytes(scene.token));
    assert.ok(after.subarray(0, 0x6a0).equals(before.subarray(0, 0x6a0)) && after.subarray(0x6a4).equals(before.subarray(0x6a4)));
    assert.ok(throws(() => candidates.setField(scene.token, 0xb08 - 3, 1)) && throws(() => candidates.field(scene.token, 0xb08)));
    // 65536 BYTE 지도 밖 spot 은 거부하고 행 경계 인접 주소는 다음 행을 읽는다.
    scene.spots[256] = 9;
    assert.ok(candidates.spot(256, 0) === 9 && candidates.spot(0, 1) === 9 && throws(() => candidates.spot(0, 256)) && throws(() => candidates.spot(-1, 0)));
});

test('AI 다리 후보 생성자: 본체 소유자/PATCH188 월드/전역 조회기가 없으면 거부하고 가짜 본체/표면 타입 오류를 거부한다', () => {
    const scene = new CandidateScene(['0', '97', '65', '50', '50', '2', '0', '0', '1', '7', '2', '1', '165', '0', '', '0']);
    const globals = { bridge: () => 82, fencemark: () => 90, windArcher: () => 140 };
    const make = (overrides) => () => new RawAiBridgeCandidates(...Object.values({ memory: scene.memory, pool: scene.pool, types: scene.types, frames: scene.frames, surfaces: scene.surfaces, spots: scene.spots, globals, ...overrides }));
    assert.ok(throws(make({ memory: undefined })) && throws(make({ pool: {} })) && throws(make({ types: scene.types.slice(0, 10) })) && throws(make({ frames: scene.frames.slice(0, 10) }))
        && throws(make({ surfaces: new Uint16Array(10) })) && throws(make({ spots: new Uint8Array(10) })) && throws(make({ globals: { bridge: () => 82 } })));
    const candidates = scene.candidates();
    assert.ok(throws(() => candidates.prepare(scene.memory.allocate(16))));
    // surface flag 도 없고 fence/windArcher 도 아닌 TYPE 는 graph getter 가 거부한다.
    scene.pool.allocated(50)[10] = 120;
    assert.ok(throws(() => candidates.graph(50)));
});
