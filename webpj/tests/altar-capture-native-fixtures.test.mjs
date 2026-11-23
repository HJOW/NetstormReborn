// 제단 ShapeToBuffer 캡처의 원본 10.78 관측 1824행(cpppj `ShapeToBuffer_AltarNative1824Pixels` 가 읽는 altar-capture-x86.tsv, 약 95MB)을 웹 RawShapeToBuffer 가 같은 그림으로 만드는지 대조한다.
// 제단(TYPE 164)의 90프레임 × 소유자 색 × 추가·추상 비트 × 배율 입력에서 캡처 메타데이터 16바이트와 전체 픽셀을 비교한다. 입력 열만 raw 슬롯과 상태에 쓰고 관측 열은 비교에만 쓴다. 원본 게임을 실행하지 않는다.
// TYPE 기준점은 원문 입력대로 (-2, 3), 추가 그림(+a4)은 원문처럼 빈 반환이다. (2026-11-23 WB04-E1276)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { buildStandardOwnerColorMaps } from '../www/assets/owner-colors.js';
import { RawShapeToBuffer, createShapeToBufferState } from '../www/render/raw-shape-to-buffer.js';
import { put } from './raw-support.mjs';
import { clientPool, hexOrDash as hex, realCaptureAssets } from './capture-assets-support.mjs';

// 제단 TYPE 번호와 원본 제단 클래스 가상 표 기록값이다(원문 리터럴로 설치해 상수 오타를 잡는다).
const ALTAR = 164;
const ALTAR_VTABLE = 0x5076b8;

test('altar-capture 원본 1824행: 제단 캡처의 메타데이터·전체 픽셀·색표/주 프레임/추가 그림 호출 횟수가 같다', () => {
    const lines = fs.readFileSync(new URL('../../cpppj/tests/fixtures/altar-capture-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#') && !line.startsWith('frame\t'));
    assert.equal(lines.length, 1824);
    const assets = realCaptureAssets();
    const types = assets.types.slice(); types[ALTAR] = { ...types[ALTAR], hotspotX: -2, hotspotY: 3 };
    const baseFrame = assets.table.definitions[ALTAR - 70].baseFrame;
    const pool = clientPool(); const maps = buildStandardOwnerColorMaps(ALTAR, 82); const tokens = new Uint32Array(188); tokens[ALTAR] = 1;
    const failures = []; let count = 0;
    for (const [index, line] of lines.entries()) {
        const row = line.split('\t'); assert.equal(row.length, 9, `행 ${index}`); assert.equal(row[5], row[7]); assert.equal(row[6], row[8]);
        const frame = Number(row[0]), owner = Number(row[1]), color = Number(row[2]), zoom = Number(row[3]) | 0, extra = Number(row[4]);
        const raw = pool.allocated(50); raw.fill(0); put(raw, 0, ALTAR_VTABLE); raw[10] = ALTAR; raw[34] = owner; raw[40] = extra; put(raw, 36, frame);
        let mainCalls = 0, additionalCalls = 0, colorCalls = 0;
        const state = createShapeToBufferState({
            zoom, addTypeHotspot: true, ownerTypeMaps: tokens,
            ownerColors: (type, who) => { assert.deepEqual([type, who], [ALTAR, owner]); colorCalls += 1; return maps[color]; },
            // 실제 +78 은 현재 TYPE 의 기본 프레임(TYPE+128)이고 +a4 는 빈 반환이다.
            mainFrame: (sid) => { assert.equal(sid, 50); mainCalls += 1; return baseFrame; },
            additional: (sid) => { assert.equal(sid, 50); additionalCalls += 1; },
        });
        const before = Uint8Array.from(raw);
        let image; try { image = new RawShapeToBuffer(pool, types, assets.shapes, assets.database, state).capture(50); } catch (error) { failures.push(`${index}: ${error.message}`); continue; }
        const metadata = Buffer.alloc(16); [image.width, image.height, image.hotspotX, image.hotspotY].forEach((value, position) => metadata.writeInt32LE(value, position * 4));
        const calls = mainCalls === ((extra & 2) ? 1 : 0) && additionalCalls === mainCalls && colorCalls === (owner ? 1 : 0);
        if (hex(metadata) !== row[5] || hex(image.pixels) !== row[6] || !calls || Buffer.compare(Buffer.from(before), Buffer.from(raw)) !== 0) failures.push(`${index} frame${frame} owner${owner} color${color} zoom${zoom} extra${extra}`);
        else count += 1;
    }
    assert.deepEqual(failures.slice(0, 12), []);
    assert.equal(count, 1824);
});
