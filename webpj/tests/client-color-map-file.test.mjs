// 색표 캐시 파일 연결 client-color-sort-input.js 의 initializeCache 와 웹 적응 저장소 color-map-files.js(MemoryColorMapFiles)를 cpppj ClientColorMapFileTests 의 장면으로 대조한다.
//  - colorsort-fixed-input-x86.tsv 의 기본 53비트 첫 관측(최초 논리 RGB·현재 빨강·원문 주소 순서 9표)을 입력/기대로만 쓴다: 캐시 miss 가 계산해 파일(원문 9표 순서 2304바이트)을 남기고, 새 소유자의 hit 는
//    다른 빨강으로 다시 계산하지 않고 같은 표를 읽으며, 소유자 8행 출력(ownerSolid=소유자 색, ownerHighlight=명도 > 0.8 이면 소유자 색)이 4096바이트 전부 같다.
//  - 계약: 알려진 표 파일 hit(후행 바이트 무시·파일 불변·이후 rebuild 는 같은 표 객체 갱신), 짧은 파일은 첫 목적 표 접두만 바꾸고 나머지는 0, 쓸 수 없는 저장소는 계산한 표를 남기고 열 수 없음으로 실패, 최초 초기화 계약, 저장소 규칙.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E375: 5-A ClientColorMapFile 웹 적응과 함께 추가. 암호화 TAFF 등록 snapshot 시험은 웹에 TAFF 캐시 경로가 없어 옮기지 않았다)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { COLOR_SORT_PRIMARY_BYTES } from '../www/core/raw-color-sort-primary.js';
import { ClientColorSortInput } from '../www/core/client-color-sort-input.js';
import { MemoryColorMapFiles } from '../www/core/color-map-files.js';
import { ColorCacheUnableToOpen } from '../www/core/raw-color-map-cache.js';

// 원문 43c4e0 의 아홉 read/write 순서이며 주소 순서와 다르다. 소유자 초기 8색은 최초 PE 의 값이다. (2026-11-13 WB04-E375)
const FILE_ORDER = [2, 7, 6, 4, 3, 5, 1, 0, 8];
const OWNER_COLORS = [84, 249, 184, 140, 150, 112, 156, 222];
const PATH = 'cache/!color.dat';

// 16진 문자열을 바이트 배열로 읽는다. (2026-11-13 WB04-E375)
const bytes = (hex) => Uint8Array.from(Buffer.from(hex, 'hex'));

// 최초 53비트 관측의 입력(논리 RGB·현재 빨강)과 아홉 출력이다. 없으면 null 이다. (2026-11-13 WB04-E375)
function nativeInput() {
    const all = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'colorsort-fixed-input-x86.tsv'));
    const row = all?.find((candidate) => candidate[0] === '639');
    if (!row) return null;
    const raw = Buffer.from(row[3], 'hex');
    return { red: Number(row[2]), logical: Uint32Array.from({ length: 256 }, (_, index) => raw.readUInt32LE(index * 4)), expected: row[6].split(':').map(bytes) };
}
const native = nativeInput();

// 명시 파일 순서만 적용하며 각 표의 내용은 다시 계산하지 않는다. (2026-11-13 WB04-E375)
const pack = (maps) => Uint8Array.from(FILE_ORDER.flatMap((index) => [...maps[index]]));

// 미작성 padding 도 명시 원시 입력으로 주고 최초 캡처를 한다. (2026-11-13 WB04-E375)
function capture(owner) {
    owner.captureWithPrimary(native.logical, new Uint8Array(COLOR_SORT_PRIMARY_BYTES).fill(0xa5));
    return owner;
}

// 아홉 표가 기대 배열(주소 순서)과 같은지 본다. (2026-11-13 WB04-E375)
function sameMaps(actual, expected) {
    return actual.length === 9 && actual.every((map, index) => Buffer.compare(map, expected[index]) === 0);
}

test('ClientColorMapFile: 캐시 miss 가 원문 9표를 계산해 남기고 새 소유자의 hit 가 같은 표와 소유자 8행 출력을 유지한다', { skip: !native && '원본 fixture 없음' }, () => {
    const files = new MemoryColorMapFiles();
    const first = capture(new ClientColorSortInput());
    const redraw = { value: 0 };
    let calls = 0;
    first.initializeCache(files, PATH, native.red, () => { calls += 1; first.ownerTables.initialize(first.primary, 188, 82, redraw); });
    assert.ok(first.mapsReady && sameMaps(first.maps, native.expected) && calls === 1 && first.ownerTables.ready);
    const expected = pack(native.expected);
    assert.equal(expected.length, 2304);
    assert.deepEqual(files.read(PATH), expected);
    // 최초 PE 의 8색을 각 물리 행에 대조한다(hit 와 무관한 부모 접두 전체 4096바이트).
    for (let row = 0; row < 8; row += 1) {
        for (let color = 0; color < 256; color += 1) {
            assert.equal(first.ownerSolid[row * 256 + color], OWNER_COLORS[row]);
            assert.equal(first.ownerHighlight[row * 256 + color], first.primary.lightness(color) > 0.800000011920929 ? OWNER_COLORS[row] : color);
        }
    }
    // 다른 빨강으로 다시 계산하면 원문 destroy 표와 달라지므로 hit 는 계산하지 않는다.
    const second = capture(new ClientColorSortInput());
    second.initializeCache(files, PATH, 255, () => { calls += 1; });
    assert.ok(calls === 2 && sameMaps(second.maps, native.expected));
    assert.deepEqual(files.read(PATH), expected);
});

test('ClientColorMapFile: 알려진 표 파일 hit 는 후행 바이트를 무시하고 파일을 다시 쓰지 않으며 이후 rebuild 는 같은 표 객체를 갱신한다', { skip: !native && '원본 fixture 없음' }, () => {
    const files = new MemoryColorMapFiles();
    // 계산으로 우연히 일치하지 않는 명시 행이다.
    const sample = Array.from({ length: 9 }, (_, row) => Uint8Array.from({ length: 256 }, (__, color) => (row * 17 + color) & 0xff));
    const content = Uint8Array.from([...pack(sample), 0x77]);
    files.put(PATH, content);
    const owner = capture(new ClientColorSortInput());
    let calls = 0;
    owner.initializeCache(files, PATH, native.red, () => { calls += 1; });
    assert.ok(sameMaps(owner.maps, sample) && calls === 1);
    assert.deepEqual(files.read(PATH), content);
    const same = owner.maps;
    owner.rebuild(native.red);
    assert.ok(owner.maps === same && sameMaps(owner.maps, native.expected));
    assert.deepEqual(files.read(PATH), content);
});

test('ClientColorMapFile: 짧은 실제 읽기는 첫 목적 표 접두만 바꾸고 나머지 표를 재생성하거나 파일을 고치지 않는다', { skip: !native && '원본 fixture 없음' }, () => {
    const files = new MemoryColorMapFiles();
    const content = Uint8Array.from([7, 11, 19]);
    files.put(PATH, content);
    const owner = capture(new ClientColorSortInput());
    let calls = 0;
    owner.initializeCache(files, PATH, native.red, () => { calls += 1; });
    assert.ok(calls === 1 && owner.mapsReady);
    assert.deepEqual(files.read(PATH), content);
    // 첫 목적 표(주소 순서 2)의 앞 3바이트만 파일 내용이고 나머지는 처음 상태의 0 이다.
    for (let row = 0; row < 9; row += 1) for (let color = 0; color < 256; color += 1) assert.equal(owner.maps[row][color], row === 2 && color < content.length ? content[color] : 0);
});

test('ClientColorMapFile: 쓸 수 없는 저장소는 계산한 표와 소유자 등록을 남기고 열 수 없음으로 실패하며 성공 파일을 합성하지 않는다', { skip: !native && '원본 fixture 없음' }, () => {
    for (const files of [new MemoryColorMapFiles({ directories: ['cache'] }), new MemoryColorMapFiles({ writable: false })]) {
        const owner = capture(new ClientColorSortInput());
        let calls = 0;
        const target = 'missing/!color.dat';
        assert.throws(() => owner.initializeCache(files, target, native.red, () => { calls += 1; }), ColorCacheUnableToOpen);
        // 소유자 등록(prefix)과 9표 계산(buildMaps)은 writer 실패 전에 끝났다.
        assert.ok(calls === 1 && sameMaps(owner.maps, native.expected));
        assert.equal(files.has(target), false);
    }
});

test('ClientColorSortInput.initializeCache: 최초 초기화 계약(primary 필요·한 번뿐·소유자 등록 필수)을 지킨다', { skip: !native && '원본 fixture 없음' }, () => {
    const files = new MemoryColorMapFiles();
    const bare = new ClientColorSortInput();
    bare.capture(native.logical);
    assert.throws(() => bare.initializeCache(files, PATH, native.red, () => {}), /primary 미연결/u);
    const owner = capture(new ClientColorSortInput());
    assert.throws(() => owner.initializeCache(files, PATH, native.red, null), /최초 초기화 계약/u);
    assert.throws(() => owner.ownerSolid, /미생성/u);
    assert.throws(() => owner.ownerHighlight, /미생성/u);
    owner.initializeCache(files, PATH, native.red, () => {});
    assert.throws(() => owner.initializeCache(files, PATH, native.red, () => {}), /최초 초기화 계약/u);
    assert.equal(owner.ownerSolid.length, 2048);
    assert.equal(owner.ownerHighlight.length, 2048);
});

test('MemoryColorMapFiles: 읽기는 사본이고 쓰기는 즉시 이어 붙으며 쓸 수 없는/상위 폴더 없는 경로는 열 수 없음이다', () => {
    const files = new MemoryColorMapFiles({ directories: ['cache'] });
    assert.equal(files.read(PATH), null);
    assert.equal(files.has(PATH), false);
    const writer = files.createWriter(PATH);
    // 열면 즉시 빈 파일이 생기고 쓰기는 닫기 전에도 보인다(도중 실패해도 부분 파일).
    assert.ok(files.has(PATH));
    assert.equal(files.read(PATH).length, 0);
    const source = Uint8Array.from([1, 2, 3]);
    writer.write(source);
    source[0] = 99;
    assert.deepEqual(files.read(PATH), Uint8Array.from([1, 2, 3]));
    writer.write(Uint8Array.from([4]));
    writer.close();
    const copy = files.read(PATH);
    copy[0] = 55;
    assert.deepEqual(files.read(PATH), Uint8Array.from([1, 2, 3, 4]));
    // 다시 열면 비운다.
    files.createWriter(PATH);
    assert.equal(files.read(PATH).length, 0);
    // 상위 폴더 검사: 최상위 경로는 허용, 목록에 없는 상위는 거부.
    assert.ok(files.createWriter('top.dat'));
    assert.throws(() => files.createWriter('other/x.dat'), ColorCacheUnableToOpen);
    assert.throws(() => new MemoryColorMapFiles({ writable: false }).createWriter(PATH), ColorCacheUnableToOpen);
    const open = new MemoryColorMapFiles();
    assert.ok(open.createWriter('anywhere/deep/x.dat'));
    files.put('x', Uint8Array.from([9]));
    assert.deepEqual(files.read('x'), Uint8Array.from([9]));
});
