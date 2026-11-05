// 공간 해시(SquidHash)와 일반 사각형 탐색기(RawSquidFinder)를 원본 10.78 관측과 대조한다.
//  - cpppj/tests/fixtures/hash-x86.tsv: 초기화 4·버킷 주소 807(At 100·정수 100·float 607)·ObjectLevel 605. Genus 행(spot 점유)은 WB04-E 범위라 쓰지 않는다.
//  - cpppj/tests/fixtures/rawfinder-x86.tsv: 10.78(originals) Find 584행의 커서 전체(사각형·버킷 범위·단계·x/y·next·current·플래그)와 첫 반환 뒤 입력 변경.
//    Pre/Post(다리 삭제 훅) 행은 WB04-D/E 범위라 쓰지 않는다. CD/10.37 행은 웹 범위 밖이다.
// 읽기 전용 원본 관측은 cpppj 가 보호된 원본 함수를 격리 실행해 만든 것이며 없으면 해당 대조만 건너뛴다.
// (2026-11-05 WB04-B: cpppj HashTests.cpp·RawFinderTests.cpp 의 10.78 대조와 계약 검사를 옮김)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SQUID_HASH_SCALES, SquidHash } from '../www/core/squid-hash.js';
import { RawSquidFinder } from '../www/core/raw-squid-finder.js';

// 원본 관측 파일 경로이다. (2026-11-05 WB04-B)
const HASH_FIXTURE = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'hash-x86.tsv');
const FINDER_FIXTURE = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'rawfinder-x86.tsv');

// 탭 구분 행을 읽는다(주석·빈 줄 제외, 빈 마지막 필드 보존). 파일이 없으면 null 이다. (2026-11-05 WB04-B)
function readRows(file) {
    if (!fs.existsSync(file)) return null;
    return fs.readFileSync(file, 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && line[0] !== '#').map((line) => line.split('\t'));
}

const hashRows = readRows(HASH_FIXTURE);
const finderRows = readRows(FINDER_FIXTURE);

// x86 에 넣은 단정도 비트를 JS 수로 되돌린다. (2026-11-05 WB04-B)
function float(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, Number(bits) >>> 0, true);
    return view.getFloat32(0, true);
}

// JS 수를 단정도 비트로 바꾼다. (2026-11-05 WB04-B)
function bits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 슬롯 쓰기 대여(Uint8Array view)에 little endian 값을 쓴다. (2026-11-05 WB04-B)
function write(raw, offset, value, width) {
    // 필요한 바이트만 갱신한다.
    for (let index = 0; index < width; index += 1) raw[offset + index] = (value >>> (8 * index)) & 0xff;
}

// 해시 초기화·주소: 원본 초기화 4개와 반환 주소 807개를 같은 배열 인덱스로 비교한다.
test('SquidHash: 원본10.78 초기화4·버킷 주소807 과 같다', { skip: !hashRows && '원본 fixture 없음' }, () => {
    const hash = new SquidHash();
    const counts = { Init: 0, At: 0, IntBucket: 0, FloatBucket: 0 };
    // 행마다 원본 결과와 비교한다.
    for (const row of hashRows) {
        if (row[0] === 'Init') {
            hash.select(Number(row[1]));
            hash.setAt(0, 0, 7);
            hash.reset();
            assert.equal(hash.selected, Number(row[2]));
            assert.equal(SQUID_HASH_SCALES[hash.selected], Number(row[3]));
            assert.equal(SquidHash.side(hash.selected), Number(row[4]));
            // 원본이 전체 배열을 지운 결과와 같다.
            for (let level = 0; level < 4; level += 1) assert.ok(hash.entries(level).every((value) => value === 0));
        } else if (row[0] === 'At') {
            hash.select(Number(row[1]));
            // At 은 선택 단계의 인덱스이다. 쓰고 읽어 같은 칸임을 확인한다.
            const index = SquidHash.cellIndex(hash.selected, Number(row[2]), Number(row[3]));
            assert.equal(index, Number(row[4]), row.join(' '));
            hash.setAt(Number(row[2]), Number(row[3]), 99);
            assert.equal(hash.entries(hash.selected)[index], 99);
            hash.reset();
        } else if (row[0] === 'IntBucket') {
            assert.equal(SquidHash.cellIndex(Number(row[1]), Number(row[2]), Number(row[3])), Number(row[4]), row.join(' '));
        } else if (row[0] === 'FloatBucket') {
            assert.equal(SquidHash.bucketIndex(Number(row[1]), float(row[2]), float(row[3])), Number(row[4]), row.join(' '));
        } else {
            continue;
        }
        counts[row[0]] += 1;
    }
    assert.deepEqual(counts, { Init: 4, At: 100, IntBucket: 100, FloatBucket: 607 });
});

// 객체 해시 단계: SHP 프레임 크기와 섬/다리 플래그로 원본이 저장한 단계 바이트를 재현한다(두 판본 결과 열이 같다).
test('SquidHash.objectLevel: 원본10.78 단계 선택605와 같다', { skip: !hashRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 2/4 경계 바로 앞뒤와 섬/다리 행을 비교한다.
    for (const row of hashRows.filter((entry) => entry[0] === 'Level')) {
        const value = SquidHash.objectLevel(Number(row[1]) >>> 0, float(row[2]), float(row[3]));
        assert.equal(value, Number(row[4]), row.join(' '));
        assert.equal(value, Number(row[5]), row.join(' '));
        count += 1;
    }
    assert.equal(count, 605);
});

// 단계 배열의 독립성·전체 초기화·선택 보존과 범위 밖 입력 거부(원본 포인터 산술로 진행하지 않음).
test('SquidHash: 단계 독립·초기화와 범위 밖 거부', () => {
    const hash = new SquidHash(3);
    // 각 단계의 첫/마지막 버킷을 다른 값으로 채운다.
    for (let level = 0; level < 4; level += 1) {
        const last = SquidHash.side(level) - 1;
        hash.setCell(level, 0, 0, level + 1);
        hash.setCell(level, last, last, level + 11);
    }
    assert.equal(hash.at(0, 0), 4);
    assert.equal(hash.bucket(2, 255.9, 255.9), 13);
    // 사본을 바꿔도 해시는 그대로다.
    const copy = hash.entries(0);
    copy[0] = 500;
    assert.equal(hash.cell(0, 0, 0), 1);
    hash.reset();
    assert.equal(hash.selected, 3);
    for (let level = 0; level < 4; level += 1) assert.ok(hash.entries(level).every((value) => value === 0));
    // 정수 절삭: 20.25 는 20, 단계2 는 4칸씩.
    assert.equal(SquidHash.bucketIndex(0, 20.25, 20.25), 20 * 256 + 20);
    assert.equal(SquidHash.bucketIndex(2, 255.99, 7.5), 1 * 64 + 63);
    // float 인자: 255.99999999 는 float32 로 256 이 되어 보드 밖이다.
    assert.throws(() => SquidHash.bucketIndex(0, 255.99999999, 0), RangeError);
    assert.throws(() => hash.select(4), RangeError);
    assert.equal(hash.selected, 3);
    assert.throws(() => hash.cell(3, 16, 0), RangeError);
    assert.throws(() => hash.at(-1, 0), RangeError);
    assert.throws(() => hash.bucket(0, 256, 0), RangeError);
    assert.throws(() => hash.bucket(0, Number.NaN, 0), RangeError);
    assert.throws(() => hash.setCell(0, 0, 0, 65536), RangeError);
    assert.throws(() => SquidHash.objectLevel(0, -1, 1), RangeError);
    assert.equal(SquidHash.objectLevel(0x4, Number.NaN, -1), 0, '다리는 크기를 보지 않는다');
});

// 원본 관측 장면: 같은 풀(24000, 서버 아님)의 client 번호 5~64 를 미리 확보하고, 장면마다 체인/좌표/타입/상태를 다시 입력한다(cpppj Scene 과 같음). (2026-11-05 WB04-B)
class Scene {
    // 풀·해시·슬롯 쓰기 대여·타입 표·높이 확장 표를 만든다.
    constructor() {
        // 실제 SID 풀(WB04-A)이다.
        this.pool = new SidPool(24000, { server: false });
        // 실제 공간 해시이다.
        this.hash = new SquidHash();
        // 번호 → 슬롯 쓰기 대여이다.
        this.slots = [];
        // 타입 번호 → 발자국이다.
        this.types = Array.from({ length: 256 }, () => ({ footX: 0, footY: 0 }));
        // 타입 번호 → 높이 확장이다.
        this.heights = new Array(256).fill(0);
        // 번호 5~64 를 차례로 확보한다.
        for (let expected = 5; expected <= 64; expected += 1) {
            const sid = this.pool.allocate(2);
            assert.equal(sid, expected);
            this.slots[sid] = this.pool.allocated(sid);
        }
    }

    // fixture 의 객체 설명(`sid,type,state,extra,xBits,yBits,footX,footY,height,level;…`)을 버킷 머리 앞에 차례로 끼워 넣는다.
    setup(description) {
        this.hash.reset();
        this.heights.fill(0);
        // 객체마다 슬롯과 체인을 입력한다.
        for (const object of description.split(';')) {
            const v = object.split(',').map((field) => Number(field) >>> 0);
            assert.equal(v.length, 10);
            const raw = this.slots[v[0]];
            raw.fill(0);
            raw[10] = v[1];
            raw[11] = v[2];
            raw[40] = v[3];
            write(raw, 14, v[4], 4);
            write(raw, 18, v[5], 4);
            this.types[v[1]].footX = v[6];
            this.types[v[1]].footY = v[7];
            this.heights[v[1]] = v[8];
            const x = float(v[4]);
            const y = float(v[5]);
            write(raw, 4, this.hash.bucket(v[9], x, y), 2);
            this.hash.setBucket(v[9], x, y, v[0]);
        }
    }
}

// 원본 finder +4..+60 의 DWORD 순서로 상태를 적는다. 확장 높이 플래그의 원래 값은 4 이다. (2026-11-05 WB04-B)
function snapshot(finder) {
    const s = finder.state;
    return [s.area.left, s.area.top, s.area.right, s.area.bottom, s.firstX, s.firstY, s.lastX, s.lastY, s.level, s.x, s.y, s.next, s.current, s.skipSurface ? 1 : 0, s.extendedHeight ? 4 : 0].join(',');
}

// 탐색 커서: 10.78 Find 584행에서 첫 반환 뒤 원본과 같은 시점에 입력을 바꾸고(1 next 끊기·2 buried·3 좌표 이동·4 타입 교체) 이후 커서 전체를 비교한다.
test('RawSquidFinder: 원본10.78 Find584 커서와 동적 입력이 같다', { skip: !finderRows && '원본 fixture 없음' }, () => {
    const scene = new Scene();
    let count = 0;
    const failures = [];
    // 10.78 행만 재생한다.
    for (const row of finderRows.filter((entry) => entry[0] === 'Find' && entry[1] === 'originals')) {
        assert.equal(row.length, 8);
        count += 1;
        scene.setup(row[6]);
        const [left, top, right, bottom] = row[4].split(',').map(Number);
        const finder = new RawSquidFinder(scene.pool, scene.hash, scene.types, scene.heights);
        let current = finder.begin({ left, top, right, bottom }, Number(row[3]));
        let output = snapshot(finder);
        let index = 0;
        // 원본과 같은 시점에 입력만 바꾸고 다음 커서를 비교한다.
        while (current !== 0 && index < 32) {
            if (index === 0) {
                const mutation = Number(row[5]);
                if (mutation === 1) write(scene.slots[current], 4, 0, 2);
                else if (mutation === 2) scene.slots[63][40] = 8;
                else if (mutation === 3) {
                    write(scene.slots[63], 14, bits(100.25), 4);
                    write(scene.slots[63], 18, bits(100.75), 4);
                } else if (mutation === 4) scene.slots[63][10] = Number(row[6].split(';')[0].split(',')[1]);
            }
            current = finder.next();
            output += `;${snapshot(finder)}`;
            index += 1;
        }
        assert.ok(index < 32);
        if (output !== row[7] && failures.length < 5) failures.push({ row: row.slice(0, 6).join(' '), expected: row[7], actual: output });
    }
    assert.deepEqual(failures, []);
    assert.equal(count, 584);
});

// 다리 삭제 훅 검사의 입력과 같은 장면(다리 50 과 같은 칸 20.75/21.9 의 5개 객체)을 만든다(cpppj Lifecycle(false,false) 의 입력부). (2026-11-05 WB04-B)
function lifecycleScene() {
    const scene = new Scene();
    const description = [50, 60, 61, 62, 63, 64].map((sid) => {
        const type = sid === 60 || sid === 63 ? 75 : 74;
        return [sid, type, sid === 61 ? 2 : 0, 0, bits(20.75), bits(21.9), 1, 1, 0, sid === 50 ? 0 : 1].join(',');
    }).join(';');
    scene.setup(description);
    return scene;
}

// 시작 전 Next·지원하지 않는 플래그·확장 높이 표 부족·같은 버킷 순환·풀 밖 머리를 거부한다(원본 assert/무한 루프 대신 오류).
test('RawSquidFinder: 시작 전·잘못된 플래그·순환·풀 밖 번호를 거부한다', () => {
    const scene = lifecycleScene();
    const finder = new RawSquidFinder(scene.pool, scene.hash, scene.types);
    assert.throws(() => finder.next(), Error);
    assert.throws(() => finder.begin({}, 8), RangeError);
    assert.throws(() => finder.begin({}, 4), RangeError);
    assert.throws(() => new RawSquidFinder({ edition: 'cd1072' }, scene.hash, scene.types), TypeError);
    // 자기 순환은 반환 다음 호출에서 거부한다.
    write(scene.slots[50], 4, 50, 2);
    assert.equal(finder.begin({ left: 20, top: 21, right: 20, bottom: 21 }), 50);
    assert.throws(() => finder.next(), /순환/);
    scene.hash.setCell(0, 20, 21, 65535);
    assert.throws(() => finder.begin({ left: 20, top: 21, right: 20, bottom: 21 }), RangeError);
});

// 필터: next 는 필터보다 먼저 저장되므로 필터가 현재 next 를 지워도 뒤 후보를 방문하고, 아직 읽지 않은 버킷 머리는 다음 호출에서 새 값을 쓴다.
test('RawSquidFinder: 필터는 미리 읽은 next 와 현재 후보 값을 쓴다', () => {
    const scene = lifecycleScene();
    scene.hash.setCell(0, 20, 21, 60);
    write(scene.slots[60], 4, 63, 2);
    write(scene.slots[63], 4, 0, 2);
    const finder = new RawSquidFinder(scene.pool, scene.hash, scene.types);
    const visited = [];
    const result = finder.begin({ left: 20, top: 21, right: 20, bottom: 21 }, 0, (sid) => {
        visited.push(sid);
        if (sid === 60) {
            write(scene.slots[60], 4, 0, 2);
            return false;
        }
        return true;
    });
    assert.equal(result, 63);
    assert.deepEqual(visited, [60, 63]);
    scene.hash.setCell(0, 21, 21, 62);
    assert.equal(finder.next(), 62);
});

// 읽기 전용 계약: 정상 탐색은 풀 바이트와 네 단계 해시 머리를 바꾸지 않는다. 10.78 은 0 좌표 후보를 건너뛴다. state 는 읽기 사본이다.
test('RawSquidFinder: 풀·해시를 바꾸지 않고 0 좌표 후보를 건너뛴다', () => {
    const scene = lifecycleScene();
    const before = scene.pool.bytes();
    const heads = [0, 1, 2, 3].map((level) => scene.hash.entries(level));
    const finder = new RawSquidFinder(scene.pool, scene.hash, scene.types);
    // Fort 로더의 같은 풀 계약(finder.pool === pool)과 같은 해시 저장소를 공개한다.
    assert.equal(finder.pool, scene.pool);
    assert.equal(finder.hash, scene.hash);
    const found = [];
    // 읽기 전용 탐색을 끝까지 한다.
    for (let sid = finder.begin({ left: 20, top: 21, right: 20, bottom: 21 }); sid !== 0; sid = finder.next()) found.push(sid);
    assert.ok(found.includes(50) && found.length >= 2, found.join(','));
    assert.deepEqual(scene.pool.bytes(), before);
    for (let level = 0; level < 4; level += 1) assert.deepEqual(scene.hash.entries(level), heads[level]);
    // 상태 사본을 바꿔도 탐색기는 그대로다.
    const state = finder.state;
    state.area.left = 999;
    assert.notEqual(finder.state.area.left, 999);
    // 0 좌표는 10.78 에서 후보가 아니다.
    write(scene.slots[50], 14, 0, 4);
    assert.notEqual(finder.begin({ left: 20, top: 21, right: 20, bottom: 21 }), 50);
});
