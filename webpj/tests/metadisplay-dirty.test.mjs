// 미니맵 dirty 큐(RawMetadisplayDirty)를 원본 10.78 관측과 대조한다(WB04-D2).
//  - cpppj/tests/fixtures/metadisplaydirty-x86.tsv 의 10.78(originals) 행 전체: 고유 등록·활성 표시·카메라/사각형 비교·전체/부분 셀 그림 순서와
//    그림 중 count/전체 갱신 변경의 재조회. 결과는 count·전체 갱신·비트맵 폭·초기화 비트·프레임·저장 view 와 큐 100칸·비트맵·사건의 FNV-32 이다.
//    입력 구성과 장치 동작은 cpppj tests/MetadisplayDirtyTests.cpp 와 같다(기대값은 웹이 만들지 않는다).
// (2026-11-05 WB04-D2: 미니맵 dirty 큐 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, floatToBits, fnv32, dwordBytes, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawMetadisplayDirty, createMetadisplayDirtyState } from '../www/core/metadisplay-dirty.js';

// 원본 관측 파일이다. (2026-11-05 WB04-D2)
const FIXTURE = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'metadisplaydirty-x86.tsv');
const rows = readRows(FIXTURE)?.filter((row) => row[0] === 'originals') ?? null;

// 원본 큐 100칸 전체(count 밖 포함)를 x/y/unused DWORD 로 직렬화한다. (2026-11-05 WB04-D2)
function cellBytes(state) {
    return dwordBytes(state.cells.flatMap((cell) => [cell.x, cell.y, cell.unused]));
}

// cpppj MetadisplayScene 과 같은 입력 장면이다. input 은 fixture 13칸이다. (2026-11-05 WB04-D2)
class MetadisplayScene {
    // fixture 입력으로 상태·현재 view·장치 변경 번호를 만든다.
    constructor(input) {
        // client 5번을 가진 실제 풀이다(서버 아님).
        this.pool = new SidPool(24000, { server: false });
        assert.equal(this.pool.allocate(2), 5);
        // 원본 상태와 현재 view 이다.
        this.state = createMetadisplayDirtyState();
        this.current = { camera: [0, 0], rectangle: [0, 0, 0, 0] };
        // 장치 호출 기록 DWORD 와 변경 번호이다.
        this.events = [];
        this.mutation = input[10];
        this.mutated = false;
        const state = this.state;
        state.displayPresent = input[1] !== 0;
        state.displayFlags = input[2] & 0xff;
        state.count = input[3] | 0;
        state.fullRefresh = input[4];
        state.bitmapWidth = input[5] ? 128 : 0;
        if (input[5]) state.bitmap = new Uint8Array(16384).fill(0xa5);
        state.initialized = input[6];
        state.frames = input[7];
        // count 밖 옛 자료도 원본 입력과 같이 채운다.
        for (let index = 0; index < 100; index += 1) state.cells[index] = { x: index, y: index + 10, unused: 0xa5a5a5a5 };
        const twelveHalf = floatToBits(12.5), twentyTwo = floatToBits(22.75);
        if (input[8] === 1) this.current.camera[0] = 0x80000000;
        else if (input[8] === 2) { state.cached.camera[0] = 0x7fc00001; this.current.camera[0] = 0x7fc00001; }
        else if (input[8] === 3) this.current.camera = [twelveHalf, twentyTwo];
        else if (input[8] === 4) { state.cached.camera[0] = 0x7fc00001; this.current.camera[0] = 0x7fc00002; }
        else if (input[8] === 5) { state.cached.camera = [twelveHalf, twentyTwo]; this.current.camera = [twelveHalf, twentyTwo]; }
        if (input[9] === 2) state.cached.rectangle = [11, 22, 33, 44];
        if (input[9]) this.current.rectangle = [11, 22, 33, 44];
    }

    // 재생성/셀 그림 장치이다. 관찰 경계 뒤 현재 full/count 재조회를 검사하도록 입력을 바꾼다.
    hooks() {
        return {
            rebuild: () => {
                this.events.push(1, 0, 0);
                if (this.mutation === 1) this.state.fullRefresh = 0;
                else if (this.mutation === 2) this.state.fullRefresh = 1;
                else if (this.mutation === 3) this.state.count = 2;
            },
            redrawCell: (x, y) => {
                this.events.push(2, x, y);
                if (this.mutation === 4 && !this.mutated) {
                    this.mutated = true;
                    this.state.cells[this.state.count] = { x: 40, y: 41, unused: 0 };
                    this.state.count += 1;
                }
            },
        };
    }

    // 원본 snapshot 필드 순서와 같은 문자열이다.
    snapshot() {
        const state = this.state;
        const output = [state.count, state.fullRefresh, state.bitmapWidth, state.initialized, state.frames, ...state.cached.camera,
            ...state.cached.rectangle.map((value) => value >>> 0), fnv32(cellBytes(state)), state.bitmap ? fnv32(state.bitmap) : 0,
            this.events.length / 3, fnv32(dwordBytes(this.events))];
        return output.map((value) => String(value >>> 0)).join(',');
    }
}

// 원본 10.78 행 전체: 등록(입력 0)과 Refresh(그 외)의 모든 분기와 장치 변경 뒤 재조회가 같다.
test('RawMetadisplayDirty: 원본10.78 미니맵 dirty 행과 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    // 행마다 독립 장면을 만든다.
    for (const row of rows) {
        assert.equal(row.length, 3);
        const input = row[1].split(',').map((value) => Number(value) >>> 0);
        const scene = new MetadisplayScene(input);
        const dirty = new RawMetadisplayDirty(scene.pool, scene.state, scene.hooks());
        if (input[0] === 0) dirty.enqueue(bitsToFloat(input[11]), bitsToFloat(input[12]));
        else dirty.refresh(scene.current);
        assert.equal(scene.snapshot(), row[2], row[1]);
    }
    assert.equal(rows.length, 520);
});

// 같은 풀의 현재 슬롯 좌표로 등록하고 비유한 좌표는 거부한다(signed /4 절삭).
test('RawMetadisplayDirty: 현재 슬롯 좌표와 같은 풀만 쓴다', () => {
    const pool = new SidPool(24000, { server: false });
    assert.equal(pool.allocate(2), 5);
    const state = createMetadisplayDirtyState();
    state.displayPresent = true;
    state.displayFlags = 1;
    const dirty = new RawMetadisplayDirty(pool, state);
    const raw = pool.allocated(5);
    put(raw, 14, floatToBits(7.99));
    put(raw, 18, floatToBits(-7.99));
    dirty.enqueueSid(5);
    assert.equal(state.count, 1);
    assert.deepEqual([state.cells[0].x, state.cells[0].y], [1, -1]);
    put(raw, 14, floatToBits(8));
    dirty.enqueueSid(5);
    dirty.enqueue(8.25, -4);
    assert.equal(state.count, 2);
    assert.deepEqual([state.cells[1].x, state.cells[1].y], [2, -1]);
    assert.throws(() => dirty.enqueue(Infinity, 1), RangeError);
    const unpop = new SquidUnpop(new SidPool(24000), new SquidHash(), new Uint8Array(65536));
    assert.throws(() => unpop.setMetadisplayDirty(dirty), /풀 불일치/);
});

// 비활성 표시의 자연 반환과 필수 그림 장치 누락을 구별하고 비트맵 최초 확보 시점을 지킨다.
test('RawMetadisplayDirty: 비활성 반환·최초 확보·필수 장치', () => {
    const pool = new SidPool(24000, { server: false });
    const state = createMetadisplayDirtyState();
    state.count = 1;
    state.cells[0] = { x: 3, y: 4, unused: 0 };
    const dirty = new RawMetadisplayDirty(pool, state);
    dirty.enqueue(Infinity, 1);
    assert.equal(state.count, 1, '비활성이면 좌표 변환도 하지 않는다');
    dirty.refresh();
    assert.equal(state.bitmap.length, 16384);
    assert.equal(state.bitmapWidth, 128);
    assert.equal(state.fullRefresh, 1);
    assert.equal(state.count, 1);
    assert.equal(state.frames, 0);
    state.displayPresent = true;
    state.displayFlags = 1;
    assert.throws(() => dirty.refresh(), /재생성/);
    const missingCell = new RawMetadisplayDirty(pool, state, { rebuild: () => {} });
    assert.throws(() => missingCell.refresh(), /셀 그림/);
});

// 공유 전체 갱신 소유자({ value })를 지역 값 대신 직접 읽고 쓴다. 재생성 장치가 바꾼 값도 같은 Refresh 에서 다시 읽는다.
test('RawMetadisplayDirty: 공유 전체 갱신 소유자', () => {
    const owner = { value: 13 };
    const pool = new SidPool(24000, { server: false });
    const state = createMetadisplayDirtyState({ fullRefreshOwner: owner });
    state.displayPresent = true;
    state.displayFlags = 1;
    state.bitmap = new Uint8Array(16384);
    state.bitmapWidth = 128;
    state.initialized = 3;
    state.fullRefresh = 0xa5a5a5a5;
    let rebuilds = 0, cells = 0, ordered = true;
    const dirty = new RawMetadisplayDirty(pool, state, {
        rebuild: () => { rebuilds += 1; },
        redrawCell: (x, y) => { ordered = ordered && x === cells % 64 && y === Math.floor(cells / 64); cells += 1; },
    });
    dirty.refresh();
    assert.deepEqual([owner.value, state.fullRefresh, cells, ordered, rebuilds, state.frames], [0, 0xa5a5a5a5, 4096, true, 1, 1]);
    // 소유자 값이 0 이면 지역 값이 비0 이어도 부분 큐만 그린다.
    state.count = 1;
    state.cells[0] = { x: 3, y: 4, unused: 0 };
    const partial = [];
    new RawMetadisplayDirty(pool, state, { rebuild: () => { rebuilds += 1; }, redrawCell: (x, y) => partial.push([x, y]) }).refresh();
    assert.deepEqual(partial, [[3, 4]]);
    assert.deepEqual([owner.value, state.count, state.frames, rebuilds], [0, 0, 2, 2]);
    // 재생성 장치가 소유자 값을 1 로 바꾸면 같은 Refresh 가 전체 갱신을 한다.
    let after = 0;
    const view = { camera: [0, 0], rectangle: [1, 2, 3, 4] };
    new RawMetadisplayDirty(pool, state, { rebuild: () => { owner.value = 1; }, redrawCell: () => { after += 1; } }).refresh(view);
    assert.deepEqual([after, owner.value, state.frames], [4096, 0, 3]);
    assert.deepEqual(state.cached.rectangle, [1, 2, 3, 4]);
    assert.throws(() => createMetadisplayDirtyState({ fullRefreshOwner: { value: 'x' } }), TypeError);
});
