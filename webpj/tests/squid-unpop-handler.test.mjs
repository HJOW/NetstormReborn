// Unpop 처리기(SquidUnpop)·표시 갱신 vtable 지원표·미니맵/contained 연결을 검사한다(WB04-D2).
//  - cpppj/tests/fixtures/unpop-vtables.tsv 의 10.78 83행: 실제 PE 가상 표의 update88/update8c 가 공통 경로인지(mask)와 웹 지원표가 같다.
//    따로 복원한 마나(103)·볼트(105)·소용돌이 8c·간헐천 부분 8c 는 cpppj 검사와 같이 추가 허용한다.
//  - 부분 변경 경계: cpppj tests/UnpopTests.cpp 의 Guards 검사를 옮겼다. 그 검사의 Factory Create(82) 두 번은 실제 생성자 주소 표를 넣은 SquidFactory.create(82)로 한다
//    (2026-11-06 WB04-D4: D2 의 allocate(0)+base VT 대체를 실제 Factory 로 교체, 검사는 그 뒤 초기 바이트 대비 변화만 본다).
//  - 미니맵/contained 연결: cpppj MetadisplayDirtyTests 의 Unpop 부분과 SquidContainedUnpopTests 의 Take→Unpop→종속 해제 경로 중 Unpop 부분을 같은 풀로 검사한다.
// (2026-11-05 WB04-D2: Unpop 처리기 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop, supportsUnpopDisplay } from '../www/core/squid-unpop.js';
import { SQUID_ASSET_VTABLES, SQUID_BASE_VTABLE } from '../www/core/squid-vtables.js';
import { RawMetadisplayDirty, createMetadisplayDirtyState } from '../www/core/metadisplay-dirty.js';
import { RawSquidContainedUnpop, commonContainedParent, commonContainedPrevious, commonContainedSetPrevious } from '../www/core/squid-contained-unpop.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { PATCH_TYPE_CONSTRUCTORS } from '../www/core/squid-constructors.js';

// 원본 관측 파일이다. (2026-11-05 WB04-D2)
const VTABLE_FIXTURE = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'unpop-vtables.tsv');
const vtableRows = readRows(VTABLE_FIXTURE)?.filter((row) => row[0] === 'originals') ?? null;

// 188칸 타입 표(발자국 1×1)이다. (2026-11-05 WB04-D2)
function types() {
    return Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1 }));
}

// 원본 83행: 공통 update88(플래그 0)/update8c(0x2000) 지원이 PE 가상 표의 mask 와 같다(따로 복원한 표시는 추가 허용).
test('supportsUnpopDisplay: 원본10.78 vtable 83행과 같다', { skip: !vtableRows && '원본 fixture 없음' }, () => {
    const listed = new Set();
    // 행마다 두 경로의 지원 여부를 비교한다.
    for (const row of vtableRows) {
        assert.equal(row.length, 6);
        const vtable = Number(row[1]) >>> 0;
        const alternate = Number(row[3]) >>> 0;
        const mask = Number(row[5]);
        const mana = vtable === 0x50c3b0 || vtable === 0x509810;
        const vortex = [0x4dcda0, 0x4dcf80, 0x4dd150, 0x4dbcd0].includes(alternate);
        const geyser = vtable === 0x50da88 && alternate === 0x4854b0;
        assert.equal(supportsUnpopDisplay(vtable, 0), (mask & 1) !== 0 || mana, row.join(' '));
        assert.equal(supportsUnpopDisplay(vtable, 0x2000), (mask & 2) !== 0 || vortex || mana || geyser, row.join(' '));
        listed.add(vtable);
    }
    assert.equal(vtableRows.length, 83);
    // 웹 표의 83칸이 원본 행의 가상 표와 정확히 같은 집합이다.
    assert.deepEqual([...listed].sort((a, b) => a - b), SQUID_ASSET_VTABLES.map((row) => row[0]));
    assert.equal(supportsUnpopDisplay(0, 0), false);
    assert.equal(supportsUnpopDisplay(SQUID_BASE_VTABLE, 0), true);
});

// Guards 장면이다: 서버 풀 두 객체(실제 Factory.create(82), 다리 생성자 VT), 대상은 (20,20), spot 0xff. reset 은 같은 초기 바이트·빈 해시·초기 spot 으로 되돌린다.
// (2026-11-05 WB04-D2, 2026-11-06 WB04-D4: 생성을 실제 SquidFactory 로 교체)
function guardScene() {
    const pool = new SidPool(32768);
    const hash = new SquidHash();
    const spots = new Uint8Array(65536).fill(0xff);
    const unpop = new SquidUnpop(pool, hash, spots);
    const list = types();
    // cpppj Types 처럼 실제 생성자 주소 표를 넣은 TYPE 입력으로 두 객체를 만든다.
    const factory = new SquidFactory(pool, list.map((record, number) => ({ ...record, constructorAddress: PATCH_TYPE_CONSTRUCTORS[number] })));
    const sid = factory.create(82);
    const other = factory.create(82);
    const bytes = pool.allocated(sid);
    bytes[11] = 0;
    put(bytes, 14, floatToBits(20));
    put(bytes, 18, floatToBits(20));
    const initial = pool.bytes();
    const originalSpots = spots.slice();
    const stride = pool.layout.stride;
    const reset = () => {
        // 모든 물리 슬롯을 초기 바이트로 되돌린다.
        for (let index = 0; index < pool.capacity; index += 1) pool.physical(index).set(initial.subarray(index * stride, (index + 1) * stride));
        hash.reset();
        spots.set(originalSpots);
    };
    return { pool, hash, spots, unpop, types: list, sid, other, bytes, initial, originalSpots, reset, stride };
}

// 원문 void/지도/체인 변경 뒤 선택된 진단·표시 경계에서 실패해도 부분 효과를 보존한다.
test('SquidUnpop: 선택된 경계의 실패는 앞선 부분 변경을 남긴다', () => {
    const scene = guardScene();
    const { pool, hash, spots, unpop, sid, other, bytes, stride } = scene;
    const type = scene.types[82];
    // 현재 풀 전체의 사본이다.
    const snapshot = () => pool.bytes();
    // found 진단(장치 없음)은 void 쓰기 뒤다.
    let expected = scene.initial.slice();
    expected[sid * stride + 11] |= 4;
    assert.throws(() => unpop.unpop(sid, type), /assertion/);
    assert.deepEqual(pool.bytes(), expected);
    assert.deepEqual(spots, scene.originalSpots);
    // 미복원 표시 거부도 체인 해제 뒤다.
    scene.reset();
    hash.setBucket(1, 20, 20, sid);
    put(bytes, 0, 0x12345678);
    expected = snapshot();
    expected[sid * stride + 11] |= 4;
    assert.throws(() => unpop.unpop(sid, type), /표시 갱신 미복원/);
    assert.deepEqual(pool.bytes(), expected);
    assert.equal(hash.bucket(1, 20, 20), 0);
    // 접두 진단 장치가 없으면 아직 void 쓰기 전이다.
    scene.reset();
    bytes[11] = 8;
    const contained = pool.bytes();
    assert.throws(() => unpop.unpop(sid, type), /assertion/);
    assert.deepEqual(pool.bytes(), contained);
    // 찾은 대상의 next(자기 자신)를 그대로 머리에 쓰며 임의 self-next 거부를 추가하지 않는다.
    scene.reset();
    hash.setBucket(1, 20, 20, sid);
    put(bytes, 4, sid, 2);
    unpop.unpop(sid, type);
    assert.equal(bytes[11], 4);
    assert.equal(hash.bucket(1, 20, 20), sid);
    // 대상에 닿지 않는 무한 순환은 물리 경계로 거부한다.
    scene.reset();
    hash.setBucket(1, 20, 20, other);
    put(pool.allocated(other), 4, other, 2);
    assert.throws(() => unpop.unpop(sid, type), /순환/);
    assert.equal(bytes[11], 4);
    assert.equal(hash.bucket(1, 20, 20), other);
    // 잘못된 좌표도 소비 시점 앞의 void 쓰기는 남긴다.
    scene.reset();
    put(bytes, 14, 0x7fc00000);
    expected = snapshot();
    expected[sid * stride + 11] |= 4;
    assert.throws(() => unpop.unpop(sid, type));
    assert.deepEqual(pool.bytes(), expected);
    assert.deepEqual(spots, scene.originalSpots);
});

// 표시 장치 연결: 지원 VT 이면 현재 슬롯과 플래그로 update 를 부르고, 연결 오류는 생성/등록 때 거부한다.
test('SquidUnpop: 표시 장치·현재 TYPE 표·연결 확인', () => {
    const pool = new SidPool(32768);
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const calls = [];
    const unpop = new SquidUnpop(pool, hash, spots, { update: (raw, flags) => calls.push([get(raw, 0), flags]) });
    const sid = pool.allocate(0);
    const raw = pool.allocated(sid);
    put(raw, 0, SQUID_BASE_VTABLE);
    raw[10] = 74;
    // 할당 직후의 void 를 지워 공간에 놓인 객체로 만든다.
    raw[11] = 0;
    put(raw, 14, floatToBits(30));
    put(raw, 18, floatToBits(40));
    hash.setBucket(0, 30, 40, sid);
    unpop.unpop(sid, { flags1: 0, flags2: 0, footX: 1, footY: 1 }, 0x2000);
    assert.deepEqual(calls, [[SQUID_BASE_VTABLE, 0x2000]]);
    assert.equal(hash.bucket(0, 30, 40), 0);
    // 전체 TYPE 표가 연결되면 현재 BYTE 로 재조회하고 범위 밖은 거부한다.
    assert.throws(() => unpop.bindDiagnostics({ types: [] }), RangeError);
    assert.throws(() => new SquidUnpop(pool, hash, new Uint8Array(10)), RangeError);
    assert.throws(() => new SquidUnpop(pool, hash, spots, {}), TypeError);
    assert.throws(() => unpop.validateSpace(pool, new SquidHash(), spots), TypeError);
    assert.doesNotThrow(() => unpop.validateSpace(pool, hash, spots));
    // 미니맵 대여 반납은 현재 대여자일 때만 끊는다.
    const dirty = new RawMetadisplayDirty(pool, createMetadisplayDirtyState());
    unpop.setMetadisplayDirty(dirty);
    assert.equal(unpop.releaseMetadisplayDirty(new RawMetadisplayDirty(pool, createMetadisplayDirtyState())), false);
    assert.equal(unpop.releaseMetadisplayDirty(dirty), true);
});

// void 직후 섬/다리(genus 2·4) 객체의 기존 좌표를 같은 풀의 미니맵 큐에 넣는다. 매몰·비표면·이미 해제된 객체는 넣지 않는다.
test('SquidUnpop: 미니맵 dirty 큐 통지 시점과 조건', () => {
    const pool = new SidPool(24000, { server: false });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const unpop = new SquidUnpop(pool, hash, spots);
    const state = createMetadisplayDirtyState();
    state.displayPresent = true;
    state.displayFlags = 1;
    const dirty = new RawMetadisplayDirty(pool, state);
    unpop.setMetadisplayDirty(dirty);
    const surface = { flags1: 0, flags2: 2, footX: 1, footY: 1 };
    // 표면(genus 2) 객체를 (30,40) 에 수동 등록한다(Pop 은 WB04-E).
    const place = (x, y, buried) => {
        const sid = pool.allocate(2);
        const raw = pool.allocated(sid);
        put(raw, 0, SQUID_BASE_VTABLE);
        raw[10] = 74;
        raw[11] = 0;
        if (buried) raw[40] |= 8;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        hash.setBucket(0, x, y, sid);
        spots[y * 256 + x] = 2;
        return sid;
    };
    const sid = place(30, 40, false);
    unpop.unpop(sid, surface, 8);
    assert.equal(state.count, 1);
    assert.deepEqual([state.cells[0].x, state.cells[0].y], [7, 10]);
    assert.equal(spots[40 * 256 + 30], 0);
    assert.equal(hash.bucket(0, 30, 40), 0);
    assert.ok(pool.slot(sid)[11] & 4);
    // 이미 void 이면 아무것도 하지 않는다.
    unpop.unpop(sid, surface, 8);
    assert.equal(state.count, 1);
    // 매몰 객체는 미니맵·spot 모두 건너뛴다.
    const buried = place(40, 60, true);
    unpop.unpop(buried, surface, 8);
    assert.equal(state.count, 1);
    // 비표면 genus 는 미니맵에 알리지 않는다.
    const plain = place(80, 100, false);
    unpop.unpop(plain, { flags1: 0, flags2: 0, footX: 1, footY: 1 }, 8);
    assert.equal(state.count, 1);
});

// contained 객체의 Unpop 은 접두 진단 복귀 뒤 void 를 쓰고 공통 종속 해제(실제 VT 링크 getter)·UI 통지로 끝나며 해시 체인은 건드리지 않는다.
test('SquidUnpop: contained 종단을 같은 풀의 공통 종속 해제에 연결한다', () => {
    const pool = new SidPool(32768);
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const unpop = new SquidUnpop(pool, hash, spots);
    const lines = [];
    const assertion = (expression, file, line) => { assert.equal(file, '\\Ns\\O\\Squid.cpp'); lines.push(line); };
    unpop.bindDiagnostics({ assertion, types: types() });
    const source = pool.allocate(0);
    const parent = pool.allocate(2);
    const gump = pool.allocate(2);
    // 자산 두 개(base VT)와 GumpForm 하나를 둔다. source 는 parent 의 첫 종속이다.
    for (const sid of [source, parent]) {
        put(pool.allocated(sid), 0, SQUID_BASE_VTABLE);
        pool.allocated(sid)[10] = 74;
    }
    const raw = pool.allocated(source);
    raw[11] = 8;
    put(raw, 14, parent, 2);
    put(raw, 16, 0, 2);
    put(raw, 6, gump, 2);
    put(pool.allocated(parent), 6, source, 2);
    pool.allocated(gump)[10] = 3;
    let notices = 0;
    const commands = [];
    const contained = new RawSquidContainedUnpop(pool, {
        contentFormType: () => 6, gumpFormType: () => 3, uiEvent: () => 0x02000039, checking: () => 0, inMetamapMode: () => false, boss: () => true,
    }, {
        parent: (sid) => commonContainedParent(pool, sid),
        previous: (sid) => commonContainedPrevious(pool, sid),
        setPrevious: (sid, previous) => commonContainedSetPrevious(pool, sid, previous),
        parentNotice: (p, sid, reason) => { assert.deepEqual([p, sid, reason], [parent, source, 2]); notices += 1; },
        command: (event, object, value) => commands.push([event, object, value]),
        assertion,
    });
    // 종단이 없으면 void 뒤 선택 시점에 오류이다.
    assert.throws(() => unpop.unpop(source, types()[74]), /contained/);
    assert.equal(pool.slot(source)[11], 12);
    pool.allocated(source)[11] = 8;
    lines.length = 0;
    unpop.bindContainedUnpop((sid) => contained.unpop(sid));
    unpop.unpop(source, types()[74]);
    assert.deepEqual(lines, [0x5d9], '!isContained() 진단 복귀 뒤 계속한다');
    assert.equal(pool.slot(source)[11], 4);
    assert.equal(get(pool.slot(parent), 6, 2), 0);
    assert.equal(notices, 1);
    assert.deepEqual(commands, [[0x02000039, source, 0]]);
});
