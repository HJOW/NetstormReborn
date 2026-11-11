// 초기 지형 context(cpppj `o/RawTerrainContext`, 패치 004c0390/004c09a0/004c0ad0)의 웹 대응 `core/raw-terrain-context.js` 를 검사한다(4-B, WB04-E290).
//  - 고정 관측 `terraincontext-x86.tsv` originals 112행: constructor 의 모든 고유 입력(owner·전투·noRegion·청크 네 값·region·타입 두 개)과 앞뒤 문자 변환(Edge 20행: 이웃 분류, Normalize 20행: 도장 뒤 정규화)을
//    같은 입력에서 재생해 makePredictable·lastSid·excludedFlags·region·청크·칸 bounds·타입 대응표 256·alias 256·배열 균일성·X 열 우선 16×16 창 두 개·assertion 횟수가 같다. CD 행은 웹 목표(10.78) 밖이다.
//  - 계약: 배열 공유(map 과 접근자 동일 객체)·필수 진단 연결·판본 거부·전역/입력 종류 거부·bounds 제자리 갱신(int32 감김)·assertion 순서와 경계(16 이하는 보고 없음)·
//    범위 밖 지도 접근 거부(음수 칸/256 이상)·signed 문자 주소 거부·군도 상자 공유(uint32 감김).
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E290)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { RawTerrainContext, TerrainContextAssertion, TerrainContextGlobals } from '../www/core/raw-terrain-context.js';

// 원본 관측 파일(originals 행만)이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-11 WB04-E290)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'terraincontext-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 값 목록을 원본 DWORD 비트(부호 없는 수)로 쉼표 문자열로 만든다(signed 값도 실제 비트로 비교). (2026-11-11 WB04-E290)
const dwords = (values) => Array.from(values, (value) => (value >>> 0).toString()).join(',');

// BYTE 열을 소문자 16진 문자열로 만든다. (2026-11-11 WB04-E290)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// 검증 입력의 X 열 우선 16×16 창(x 48..63, y 48..63)을 원본 관찰 순서로 이어 붙인다. (2026-11-11 WB04-E290)
function windowHex(bytes) {
    let result = '';
    // native 가 관찰한 X 열 순서로 같은 창을 이어 붙인다.
    for (let x = 48; x < 64; x += 1) result += hex(bytes.subarray(x * 256 + 48, x * 256 + 64));
    return result;
}

// 한 행의 입력으로 context 를 만들고 도장 5×5 입력까지 넣은 뒤 변환을 실행한다. 반환 값은 context 와 assertion 횟수 상자이다. (2026-11-11 WB04-E290)
function replay(row) {
    const input = row[2].split(',').map(Number);
    const globals = new TerrainContextGlobals({ battle: input[1] !== 0, noRegion: input[2] | 0, islandType: input[8], edgeType: input[9] });
    const counter = { assertions: 0, kinds: [] };
    const context = new RawTerrainContext('patch1078', input[0], input[7] | 0,
        { left: input[3] | 0, top: input[4] | 0, right: input[5] | 0, bottom: input[6] | 0 }, globals,
        (kind) => { counter.assertions += 1; counter.kinds.push(kind); });
    if (row[1] !== 'Ctor') {
        const profile = input[10];
        // 같은 5×5 의 문자/지역 입력만 넣고 실제 모듈에 변환을 맡긴다.
        for (let x = 49; x < 54; x += 1) {
            // 각 X 열은 Y 순서로 원본 입력과 같은 byte 를 쓴다.
            for (let y = 51; y < 56; y += 1) {
                let value = 0x78;
                if (profile === 1 && x === 49) value = 0x58;
                if (profile === 2 && y === 51) value = 0x6f;
                if (profile === 3 && x === 52) value = 0x3d;
                if (profile === 4 && x === 51 && y === 53) value = 0x2e;
                context.codes[x * 256 + y] = value;
                context.regions[x * 256 + y] = profile === 3 && y === 54 ? 6 : 5;
            }
        }
        if (row[1] === 'Edge') context.markEdges(0x78, 0x3d);
        else context.normalizeAfterStamp();
    }
    return { context, counter };
}

// 실제 constructor 모든 고유 입력과 앞뒤 문자 변환·정상/assert 계속 반환을 10.78 판본에서 전수 대조한다. (2026-11-11 WB04-E290)
test('초기 지형 context 는 관측한 constructor 와 문자 변환을 그대로 재생한다', { skip: ROWS === null }, () => {
    let checked = 0;
    // native 와 같은 값형 입력만 준비하며 출력·branch·지도 값을 기대와 함께 다시 계산하지 않는다.
    for (const row of ROWS) {
        assert.equal(row.length, 13);
        const { context, counter } = replay(row);
        const map = context.map;
        const chunks = context.chunks();
        const codes = context.codes;
        const regions = context.regions;
        const uniform = [codes.every((value) => value === codes[0]) ? 1 : 0, codes[0], regions.every((value) => value === regions[0]) ? 1 : 0, regions[0]];
        assert.equal(context.owner, Number(row[3]));
        assert.equal(dwords([map.makePredictable, map.lastSid.value, map.excludedFlags, context.region]), row[4]);
        assert.equal(dwords([chunks.left, chunks.top, chunks.right, chunks.bottom]), row[5]);
        assert.equal(dwords(map.bounds), row[6]);
        assert.equal(dwords(map.typeMapping), row[7]);
        assert.equal(dwords(map.aliases), row[8]);
        assert.equal(dwords(uniform), row[9]);
        assert.equal(windowHex(codes), row[10]);
        assert.equal(windowHex(regions), row[11]);
        assert.equal(counter.assertions, Number(row[12]));
        checked += 1;
    }
    assert.equal(checked, 112);
});

// 초기 stamp·성장이 같은 owned 배열을 참조하고, 잘못된 입력과 범위 밖 지도 접근을 묵시로 고쳐 쓰지 않는다. (2026-11-11 WB04-E290)
test('초기 지형 context 는 배열을 공유하고 잘못된 연결/입력/범위를 거부한다', () => {
    const globals = new TerrainContextGlobals();
    const noop = () => {};
    const context = new RawTerrainContext('patch1078', 1, 17, { left: 1, top: 1, right: 15, bottom: 15 }, globals, noop);
    assert.equal(context.map.codes, context.codes);
    assert.equal(context.map.regions, context.regions);
    assert.equal(context.map.aliases, context.aliases);
    assert.equal(context.map.typeMapping.length, 256);
    context.aliases[0x68] = 0x78;
    assert.equal(context.map.aliases[0x68], 0x78);
    // 필수 진단 연결 누락, 다른 판본(CD), 전역 종류/입력 종류 오류는 거부한다.
    const bounds = { left: 1, top: 1, right: 15, bottom: 15 };
    assert.throws(() => new RawTerrainContext('patch1078', 1, 17, bounds, globals, undefined), /진단 연결/);
    assert.throws(() => new RawTerrainContext('cd1072', 1, 17, bounds, globals, noop), /patch1078/);
    assert.throws(() => new RawTerrainContext('patch1078', -1, 17, bounds, globals, noop), /owner/);
    assert.throws(() => new RawTerrainContext('patch1078', 1, 1.5, bounds, globals, noop), /region/);
    assert.throws(() => new RawTerrainContext('patch1078', 1, 17, bounds, {}, noop), /전역/);
    assert.throws(() => new RawTerrainContext('patch1078', 1, 17, { left: 1, top: 1, right: 15 }, globals, noop), /bounds/);
    assert.throws(() => new TerrainContextGlobals({ islandType: -1 }), /전역/);
    assert.throws(() => new TerrainContextGlobals({ battle: 1 }), /전역/);
    // 칸 bounds 는 먼저 16 배 저장(int32 감김)하고 같은 배열 객체를 제자리에서 갱신한다.
    const held = context.map.bounds;
    context.setBounds({ left: -1, top: 0, right: 1, bottom: 1 });
    assert.equal(context.map.bounds, held);
    assert.equal(held[0], -16);
    context.setBounds({ left: 0x10000000, top: 0x08000001, right: 2, bottom: 3 });
    assert.deepEqual([...held], [0, -2147483632, 32, 48]); // 0x08000001 << 4 = 0x80000010 (int32 감김)
    // 음수 칸 범위는 가장자리로 끌어당기지 않고 거부한다.
    context.setBounds({ left: -1, top: 0, right: 1, bottom: 1 });
    assert.throws(() => context.markEdges(0x78, 0x3d), RangeError);
    assert.throws(() => context.normalizeAfterStamp(), RangeError);
    assert.throws(() => context.markEdges(0x78, 256), TypeError);
    assert.throws(() => context.markEdges(0x78, 1.5), TypeError);
    assert.throws(() => context.setBounds({ left: 1, top: 1, right: 15, bottom: 'x' }), TypeError);
});

// 청크 right/bottom 의 최대 16 제한과 두 assertion 의 순서/경계를 검사한다(16 이하는 보고 없음). (2026-11-11 WB04-E290)
test('초기 지형 context 는 right/bottom 이 16 을 넘으면 순서대로 보고하고 청크만 16 으로 줄인다', () => {
    const kinds = [];
    const globals = new TerrainContextGlobals();
    const context = new RawTerrainContext('patch1078', 3, 0, { left: 1, top: 1, right: 16, bottom: 16 }, globals, (kind) => kinds.push(kind));
    assert.deepEqual(kinds, []);
    context.setBounds({ left: 1, top: 1, right: 17, bottom: 18 });
    assert.deepEqual(kinds, [TerrainContextAssertion.RightChunkLimit, TerrainContextAssertion.BottomChunkLimit]);
    assert.deepEqual(context.chunks(), { left: 1, top: 1, right: 16, bottom: 16 });
    // 칸 bounds 는 제한하지 않고 입력 그대로 16 배로 저장한다.
    assert.deepEqual([...context.map.bounds], [16, 16, 272, 288]);
    // 한쪽만 넘으면 그쪽만 보고한다.
    kinds.length = 0;
    context.setBounds({ left: 1, top: 1, right: 5, bottom: 17 });
    assert.deepEqual(kinds, [TerrainContextAssertion.BottomChunkLimit]);
    kinds.length = 0;
    context.setBounds({ left: 1, top: 1, right: 17, bottom: 5 });
    assert.deepEqual(kinds, [TerrainContextAssertion.RightChunkLimit]);
    assert.deepEqual(context.chunks(), { left: 1, top: 1, right: 16, bottom: 5 });
    // 반환한 청크 사본을 바꿔도 내부 값은 그대로이다.
    context.chunks().left = 99;
    assert.equal(context.chunks().left, 1);
});

// 이웃 검사가 배열 바깥 칸과 signed 문자 주소를 묵시로 처리하지 않는지 검사한다. (2026-11-11 WB04-E290)
test('초기 지형 context 의 이웃 분류는 지도 바깥/음수 문자 주소를 거부한다', () => {
    const globals = new TerrainContextGlobals();
    const noop = () => {};
    // right 17 청크 = 칸 272 이므로 x 256 에서 지도 범위를 벗어난다.
    const wide = new RawTerrainContext('patch1078', 1, 0, { left: 15, top: 1, right: 17, bottom: 2 }, globals, noop);
    assert.throws(() => wide.normalizeAfterStamp(), /지도 범위/);
    // 이웃이 음수(signed) 문자 주소이면 대응표 주소를 거부한다.
    const narrow = new RawTerrainContext('patch1078', 1, 0, { left: 3, top: 3, right: 4, bottom: 4 }, globals, noop);
    narrow.codes[50 * 256 + 50] = 0x78;
    narrow.codes[50 * 256 + 49] = 0x80; // 북쪽 이웃(첫 검사)이 음수 문자이다.
    assert.throws(() => narrow.markEdges(0x78, 0x58), /signed 문자/);
    // 불일치 이웃이 없는 칸(주변 모두 같은 alias/지역)은 edge 로 바뀌지 않는다.
    const inner = new RawTerrainContext('patch1078', 1, 0, { left: 3, top: 3, right: 4, bottom: 4 }, globals, noop);
    for (let x = 47; x <= 65; x += 1) for (let y = 47; y <= 65; y += 1) inner.codes[x * 256 + y] = 0x78;
    inner.markEdges(0x78, 0x58);
    assert.equal(inner.codes[50 * 256 + 50], 0x78);
    // 경계 칸 자체(왼쪽 위 모서리)는 bounds 밖 이웃이 있으므로 edge 가 된다.
    assert.equal(inner.codes[48 * 256 + 48], 0x58);
});

// 군도 DWORD 상자는 한 개를 공유하고 값은 uint32 로 감긴다. (2026-11-11 WB04-E290)
test('초기 지형 context 의 군도 상자는 같은 DWORD 를 공유한다', () => {
    const context = new RawTerrainContext('patch1078', 1, 0, { left: 1, top: 1, right: 15, bottom: 15 }, new TerrainContextGlobals(), () => {});
    const box = context.sharedArchipelagoMode();
    assert.equal(box, context.sharedArchipelagoMode());
    assert.equal(box.value, 0);
    box.value = -1;
    assert.equal(context.sharedArchipelagoMode().value, 0xffffffff);
    box.value = 0x1_0000_0003;
    assert.equal(box.value, 3);
    assert.throws(() => { box.value = '1'; }, TypeError);
    assert.equal(context.edition, 'patch1078');
});

// 지도를 'x' 로 채운 칸 범위(양 끝 포함)를 만든 context 를 돌려준다. bounds 는 청크 {3,3,4,4} = 칸 [48,48,64,64] 이다. (2026-11-11 WB04-E290)
function filledContext() {
    const context = new RawTerrainContext('patch1078', 1, 0, { left: 3, top: 3, right: 4, bottom: 4 }, new TerrainContextGlobals(), () => {});
    // 순회 범위 바깥 한 칸씩까지 같은 문자/지역으로 채운다.
    for (let x = 46; x <= 66; x += 1) for (let y = 46; y <= 66; y += 1) context.codes[x * 256 + y] = 0x78;
    return context;
}

// 순회 bounds 는 반열린(right/bottom 칸 제외)이고 이웃 판정 bounds 는 양 끝을 포함하는지, edge 값이 경계 분기에도 그대로 쓰이는지 검사한다. (2026-11-11 WB04-E290)
test('초기 지형 context 의 이웃 분류는 반열린 순회와 양 끝 포함 이웃 판정을 구별한다', () => {
    const context = filledContext();
    context.markEdges(0x78, 0x3d);
    // x == 48 또는 y == 48 인 칸만 bounds 밖 이웃(47) 때문에 edge 가 되고, 63 칸은 64 이웃이 포함되어 그대로 남는다.
    for (let x = 48; x < 64; x += 1) {
        for (let y = 48; y < 64; y += 1) assert.equal(context.codes[x * 256 + y], x === 48 || y === 48 ? 0x3d : 0x78, `${x},${y}`);
    }
    // 순회 바깥(right/bottom 칸 포함)은 건드리지 않는다.
    for (let index = 48; index <= 64; index += 1) {
        assert.equal(context.codes[64 * 256 + index], 0x78);
        assert.equal(context.codes[index * 256 + 64], 0x78);
    }
    // signed 문자(0x80)는 음수 code 로만 선택하고 불일치 이웃이 있으면 edge 로 바꾼다.
    const signed = filledContext();
    signed.codes[50 * 256 + 50] = 0x80;
    signed.aliases[0x78] = 0x78;
    signed.markEdges(-128, 0x58);
    assert.equal(signed.codes[50 * 256 + 50], 0x58);
    assert.equal(signed.codes[51 * 256 + 51], 0x78);
});

// 첫 불일치 뒤에는 나머지 이웃을 읽지 않고 멈춘다(뒤 이웃이 지도 밖이어도 거부하지 않음), 마지막 열/행의 바깥 이웃은 거부한다. (2026-11-11 WB04-E290)
test('초기 지형 context 의 이웃 분류는 첫 불일치에서 멈추고 지도 끝의 바깥 이웃을 거부한다', () => {
    const globals = new TerrainContextGlobals();
    const noop = () => {};
    // 북쪽 이웃이 달라 edge 로 바뀌면 지도 밖인 북동쪽 이웃(x 256)은 읽지 않는다.
    const stop = new RawTerrainContext('patch1078', 1, 0, { left: 15, top: 3, right: 16, bottom: 4 }, globals, noop);
    stop.codes[255 * 256 + 50] = 0x78;
    stop.markEdges(0x78, 0x58);
    assert.equal(stop.codes[255 * 256 + 50], 0x58);
    // x == 255 칸의 북동쪽(x 256)과 y == 255 칸의 남동쪽(y 256)은 지도 밖이므로 각각 거부한다.
    const east = new RawTerrainContext('patch1078', 1, 0, { left: 15, top: 3, right: 16, bottom: 4 }, globals, noop);
    for (let y = 49; y <= 51; y += 1) { east.codes[255 * 256 + y] = 0x78; east.codes[254 * 256 + y] = 0x78; }
    assert.throws(() => east.markEdges(0x78, 0x58), /지도 범위/);
    const south = new RawTerrainContext('patch1078', 1, 0, { left: 3, top: 15, right: 4, bottom: 16 }, globals, noop);
    for (let x = 50; x <= 52; x += 1) { south.codes[x * 256 + 255] = 0x78; south.codes[x * 256 + 254] = 0x78; }
    assert.throws(() => south.markEdges(0x78, 0x58), /지도 범위/);
    // code 는 정수여야 한다.
    assert.throws(() => stop.markEdges(1.5, 0x58), TypeError);
});

// 도장 뒤 정규화는 순회 범위 안에서만 =/o/X 를 x 로 바꾸고 지역 BYTE 와 그 밖의 문자는 보존한다. (2026-11-11 WB04-E290)
test('초기 지형 context 의 정규화는 반열린 범위 안의 =/o/X 만 x 로 바꾼다', () => {
    const context = new RawTerrainContext('patch1078', 1, 0, { left: 3, top: 3, right: 4, bottom: 4 }, new TerrainContextGlobals(), () => {});
    const sample = [[48, 48, 0x3d], [50, 50, 0x6f], [63, 63, 0x58], [51, 50, 0x78], [52, 50, 0x2e], [64, 50, 0x3d], [50, 64, 0x6f]];
    // 대표 칸마다 문자와 서로 다른 지역 BYTE 를 넣는다.
    for (const [x, y, code] of sample) { context.codes[x * 256 + y] = code; context.regions[x * 256 + y] = x & 7; }
    context.normalizeAfterStamp();
    assert.deepEqual(sample.map(([x, y]) => context.codes[x * 256 + y]), [0x78, 0x78, 0x78, 0x78, 0x2e, 0x3d, 0x6f]);
    assert.deepEqual(sample.map(([x, y]) => context.regions[x * 256 + y]), sample.map(([x]) => x & 7));
});

// owner 와 region 은 정수 그대로 보관한다(부호 없는 owner 상한, 음수 region). (2026-11-11 WB04-E290)
test('초기 지형 context 의 owner/region 은 입력 그대로 보관된다', () => {
    const context = new RawTerrainContext('patch1078', 0xffffffff, -5, { left: 1, top: 1, right: 15, bottom: 15 }, new TerrainContextGlobals(), () => {});
    assert.equal(context.owner, 0xffffffff);
    assert.equal(context.region, -5);
    assert.equal(context.map.excludedFlags, 0);
});
