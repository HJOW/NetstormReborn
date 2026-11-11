// 청크 지도(cpppj `o/ChunkMap`, 원본 00432180·00432360·004323c0)의 웹 대응 `assets/territory.js` ChunkMap 의 원본 물리 순서·mode·reset 을 검사한다(4-B, WB04-E291).
//  - 항목 (x, y) 의 저장 위치는 원본 물리 순서 `entries[x × 16 + y]` 이고 고정 256칸은 현재 가로·세로와 별개이다(가장자리 당김 포함).
//  - mode 는 원문 첫 DWORD(기본 1, uint32 로 감김, 수가 아닌 값은 거부)이다.
//  - reset 은 mode 를 보존하고(0 도 0 으로 복원) 현재 가로·세로 안의 항목만 같은 객체에서 "섬 없음"으로 되돌리며 바깥 물리 칸은 stale 값을 보존한다.
// 청크 지도 자체에는 독립 원본 관측 파일이 없다: 영역 배치 결과는 territory.test.mjs(보호 Fort 465개)가, 이 기능의 상호작용은 cpppj `RawSurfaceStorageTests` 의 계약 문장이 근거이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E291)
import test from 'node:test';
import assert from 'node:assert/strict';
import { ChunkMap, NO_ISLAND } from '../www/assets/territory.js';

// 항목 하나에 모든 필드를 표식 값으로 채운다. (2026-11-11 WB04-E291)
function stain(entry, seed) {
    entry.island = seed;
    entry.side = 'P';
    entry.record.set([1, 2, 3, 4, 5, seed & 0xff]);
    entry.ordinal = 71 + seed;
}

// 항목이 "섬 없음" 초기 상태인지 검사한다. (2026-11-11 WB04-E291)
function assertEmpty(entry, label) {
    assert.equal(entry.island, NO_ISLAND, label);
    assert.equal(entry.side, '', label);
    assert.deepEqual([...entry.record], [0, 0, 0, 0, 0, 0], label);
    assert.equal(entry.ordinal, 0, label);
}

// 저장 순서는 x × 16 + y 이고 서로 다른 256개 객체이며, 범위 밖 좌표는 현재 크기의 가장자리 항목으로 당긴다. (2026-11-11 WB04-E291)
test('청크 지도는 원본 물리 순서 x × 16 + y 로 256개 항목을 보관한다', () => {
    const map = new ChunkMap();
    assert.equal(map.entries.length, 256);
    assert.equal(new Set(map.entries).size, 256);
    // 모든 칸에서 접근자와 물리 위치가 같다.
    for (let x = 0; x < 16; x += 1) for (let y = 0; y < 16; y += 1) assert.equal(map.at(x, y), map.entries[x * 16 + y], `${x},${y}`);
    assert.equal(map.at(-3, 99), map.entries[0 * 16 + 15]);
    assert.equal(map.at(99, -3), map.entries[15 * 16 + 0]);
    // 작은 지도는 현재 크기 안으로 당긴다(물리 위치는 여전히 x × 16 + y).
    const small = new ChunkMap(4, 3);
    assert.equal(small.at(9, 9), small.entries[3 * 16 + 2]);
    assert.equal(small.at(2, 1), small.entries[2 * 16 + 1]);
});

// mode 는 기본 1 이고 uint32 로 감기며, 수가 아닌 값은 거부한다. (2026-11-11 WB04-E291)
test('청크 지도 mode 는 원문 첫 DWORD 로 보관된다', () => {
    const map = new ChunkMap();
    assert.equal(map.mode, 1);
    map.mode = 0x89abcdef;
    assert.equal(map.mode, 0x89abcdef);
    map.mode = -1;
    assert.equal(map.mode, 0xffffffff);
    map.mode = 0x1_0000_0003;
    assert.equal(map.mode, 3);
    map.mode = 0;
    assert.equal(map.mode, 0);
    assert.throws(() => { map.mode = '1'; }, TypeError);
    assert.throws(() => { map.mode = Number.NaN; }, TypeError);
    assert.throws(() => { map.mode = Infinity; }, TypeError);
    assert.equal(map.mode, 0);
    // 독립 객체의 mode 는 섞이지 않는다.
    assert.equal(new ChunkMap().mode, 1);
});

// reset 은 모든 활성 항목을 같은 객체에서 비우고 mode 를 보존한다(0 포함). (2026-11-11 WB04-E291)
test('청크 지도 reset 은 활성 항목을 같은 객체에서 비우고 mode 를 보존한다', () => {
    const map = new ChunkMap();
    const held = [...map.entries];
    // 모든 칸을 서로 다른 값으로 더럽힌다.
    map.entries.forEach((entry, index) => stain(entry, index & 0x3f));
    map.mode = 0x89abcdef;
    map.reset();
    assert.equal(map.mode, 0x89abcdef);
    assert.equal(map.entries.length, 256);
    // 같은 항목 객체가 그대로 비워진다.
    map.entries.forEach((entry, index) => {
        assert.equal(entry, held[index]);
        assertEmpty(entry, `entry ${index}`);
    });
    // mode 0 도 0 으로 복원한다(reset 중의 임시 1 이 남지 않음).
    map.mode = 0;
    map.reset();
    assert.equal(map.mode, 0);
    // 이미 비어 있는 상태에서 다시 reset 해도 같다.
    map.reset();
    map.entries.forEach((entry, index) => assertEmpty(entry, `again ${index}`));
});

// 작은 지도의 reset 은 현재 가로·세로 밖의 물리 항목을 보존한다. (2026-11-11 WB04-E291)
test('청크 지도 reset 은 현재 크기 밖의 고정 물리 항목을 보존한다', () => {
    const map = new ChunkMap(4, 3);
    map.entries.forEach((entry, index) => stain(entry, index & 0x3f));
    map.reset();
    // 활성 4 × 3 안은 비워지고, 그 밖은 stale 값을 그대로 가진다.
    for (let x = 0; x < 16; x += 1) {
        for (let y = 0; y < 16; y += 1) {
            const entry = map.entries[x * 16 + y];
            if (x < 4 && y < 3) assertEmpty(entry, `inside ${x},${y}`);
            else assert.deepEqual([entry.island, entry.side, entry.ordinal], [(x * 16 + y) & 0x3f, 'P', 71 + ((x * 16 + y) & 0x3f)], `outside ${x},${y}`);
        }
    }
    assert.equal(map.mode, 1);
});
