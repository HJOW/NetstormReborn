// 팔레트 색 번호 표 palette-colors.js(buildPaletteColorTable)를 원본 10.78 파일 로더 관측과 계약으로 대조한다.
//  - palettecolors-x86.tsv 의 originals 8행(파일 크기 0x308 .COL / 0x400 RGBQUAD × 재사용 여부): 파일 → GamePalette.parse → buildScreenPalette(0번 검정·255번 흰색 고정) 한 논리 팔레트의 RGB 가 원본 논리 팔레트와 같고
//    (원본 플래그 바이트는 0/255 가 0, 나머지가 4 이며 색 검색은 쓰지 않는다), 저장 팔레트는 같은 RGB 의 BGR 순서이고, 기본 9색·날씨 4색·전체 58개 (R,G,B,색 번호) 표가 원본 최종 전역과 같다.
//  - 계약: 길이 거부·동결·날씨 별칭(노랑·파랑·빨강·갈색)·중복 RGB 항목의 개별 보존·합성 회색 팔레트에서 독립 최근접 검색과 같음·58개 호출 순서.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E376: 5-A PaletteColors 이식과 함께 추가. originalCD/original1037 행은 10.72/10.37 이라 사용하지 않는다)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { GamePalette, buildScreenPalette } from '../www/assets/palette.js';
import { PATCH_PALETTE_COLOR_COUNT, buildPaletteColorTable } from '../www/core/palette-colors.js';

// 원본 관측 행이다(주석 제외). 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E376)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'palettecolors-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 실제 관찰의 byte hex 를 채널 재배치 없이 읽는다. (2026-11-13 WB04-E376)
const bytes = (hex) => Uint8Array.from(Buffer.from(hex, 'hex'));

test('PaletteColors: 원본 10.78 파일 로더 8행의 논리 팔레트 RGB·기본 9색·날씨 4색·전체 58색 표가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 입력과 기대값을 모두 fixture 에서 읽는다(검색 구현으로 기대값을 만들지 않는다).
    for (const row of rows) {
        assert.equal(row.length, 10);
        const palette = buildScreenPalette(GamePalette.parse(bytes(row[3])));
        const saved = bytes(row[5]);
        const logical = bytes(row[8]);
        const basic = Buffer.from(bytes(row[6]));
        const weather = Buffer.from(bytes(row[7]));
        // 논리 팔레트의 RGB(하위 24비트)는 원본과 같다. 원본 플래그는 0/255 번만 0 이고 나머지는 4 이다.
        for (let index = 0; index < 256; index += 1) {
            const word = Buffer.from(logical).readUInt32LE(index * 4);
            assert.equal(palette.logical[index], word & 0xffffff, `행 ${count} 논리 ${index}`);
            assert.equal(word >>> 24, index === 0 || index === 255 ? 0 : 4, `행 ${count} 플래그 ${index}`);
            // 저장 팔레트는 같은 RGB 의 BGR 순서(예약 바이트는 화면 상태라 비교하지 않는다).
            assert.equal(saved[index * 4], (palette.logical[index] >>> 16) & 255, `행 ${count} 저장 B ${index}`);
            assert.equal(saved[index * 4 + 1], (palette.logical[index] >>> 8) & 255, `행 ${count} 저장 G ${index}`);
            assert.equal(saved[index * 4 + 2], palette.logical[index] & 255, `행 ${count} 저장 R ${index}`);
        }
        const table = buildPaletteColorTable(palette.logical);
        // 기본 배열과 날씨 별칭은 실제 전역의 최종 DWORD 를 비교한다(원소 순서: 바람·비·천둥·해).
        table.basic.forEach((value, index) => assert.equal(value, basic.readUInt32LE(index * 4), `행 ${count} 기본 ${index}`));
        table.weather.forEach((value, index) => assert.equal(value, weather.readUInt32LE(index * 4), `행 ${count} 날씨 ${index}`));
        const items = row[9].split('|');
        assert.equal(items.length, PATCH_PALETTE_COLOR_COUNT);
        assert.equal(table.named.length, PATCH_PALETTE_COLOR_COUNT);
        // 실제 검색에서 관찰한 RGB 인자와 반환 색 번호를 원본 순서로 비교한다.
        items.forEach((item, index) => {
            const [red, green, blue, color] = item.split(',').map(Number);
            assert.deepEqual({ ...table.named[index] }, { red, green, blue, index: color }, `행 ${count} 항목 ${index}`);
        });
        count += 1;
    }
    assert.equal(count, 8);
});

// 합성 팔레트: 색 i 를 (i, (i * 5) & 255, (i * 11) & 255) 로 만든다(대부분 서로 다른 RGB). (2026-11-13 WB04-E376)
const synthetic = () => Uint32Array.from({ length: 256 }, (_, index) => (index | (((index * 5) & 255) << 8) | (((index * 11) & 255) << 16)) >>> 0);

test('PaletteColors: 합성 팔레트에서 독립 최근접 검색과 같고 기본/날씨 별칭·중복 RGB 항목·결과 동결을 지킨다', () => {
    const logical = synthetic();
    const table = buildPaletteColorTable(logical);
    assert.ok(Object.isFrozen(table) && Object.isFrozen(table.basic) && Object.isFrozen(table.weather) && Object.isFrozen(table.named));
    assert.equal(table.basic.length, 9);
    assert.equal(table.weather.length, 4);
    assert.equal(table.named.length, 58);
    // 독립 구현: 번호 순서로 채널 차이 제곱 합이 엄격히 더 작을 때만 바꾼다(float32 최소 저장).
    const nearest = (red, green, blue) => {
        let best = Math.fround(10000000);
        let chosen = 0;
        logical.forEach((color, index) => {
            const distance = ((color & 255) - red) ** 2 + (((color >>> 8) & 255) - green) ** 2 + (((color >>> 16) & 255) - blue) ** 2;
            if (distance < best) { best = Math.fround(distance); chosen = index; }
        });
        return chosen;
    };
    table.named.forEach((item) => assert.equal(item.index, nearest(item.red, item.green, item.blue), `${item.red},${item.green},${item.blue}`));
    // 기본 9색은 앞 아홉 항목, 날씨는 [노랑(5), 파랑(3), 빨강(1), 갈색(named 9)] 의 복사이다.
    assert.deepEqual([...table.basic], table.named.slice(0, 9).map((item) => item.index));
    assert.deepEqual([...table.weather], [table.basic[5], table.basic[3], table.basic[1], table.named[9].index]);
    // 원본 순서의 일부 RGB 확인: 0번 검정, 1번 빨강(255,22,22), 9번 갈색(181,140,111), 중복 항목(26/27, 34/7, 35/8).
    assert.deepEqual([0, 1, 9].map((i) => [table.named[i].red, table.named[i].green, table.named[i].blue]), [[0, 0, 0], [255, 22, 22], [181, 140, 111]]);
    assert.deepEqual([26, 27].map((i) => [table.named[i].red, table.named[i].green, table.named[i].blue]), [[17, 66, 60], [17, 66, 60]]);
    assert.equal(table.named[26].index, table.named[27].index);
    assert.deepEqual([table.named[34].red, table.named[34].green, table.named[34].blue], [255, 255, 255]);
    assert.equal(table.named[34].index, table.named[7].index);
    assert.deepEqual([table.named[35].red, table.named[35].green, table.named[35].blue], [128, 128, 128]);
    assert.equal(table.named[35].index, table.named[8].index);
    assert.deepEqual([table.named[37].red, table.named[37].green, table.named[37].blue], [55, 51, 54]);
    assert.deepEqual([table.named[57].red, table.named[57].green, table.named[57].blue], [238, 144, 41]);
    // 입력 팔레트는 바뀌지 않는다.
    assert.deepEqual(logical, synthetic());
    // 길이가 256 이 아니거나 없으면 거부한다.
    assert.throws(() => buildPaletteColorTable(new Uint32Array(255)), RangeError);
    assert.throws(() => buildPaletteColorTable(new Uint32Array(257)), RangeError);
    assert.throws(() => buildPaletteColorTable(null), RangeError);
});
