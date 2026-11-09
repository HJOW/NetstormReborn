// ShapeToBuffer 계열 캡처 검사(WB04-E199 shape-to-buffer·E200 burning-frame)가 공유하는 실제 10.78 자료 장치이다. cpppj ShapeToBufferTests/BurningFrameTests 의 BufferAssets·FrameAssets 에 대응한다:
// 웹 준비 자료(www/data 의 netstorm.tarc·d/_shapes.shp)에서 TypeTable·ShapeDatabase·188칸 TYPE 레코드(플래그·maxHitPoints, 기준점 0)와 Squid 그림 헤더 표를 만든다.
// TYPE 기준점(hotspotX/Y)은 웹 레코드에 아직 없으므로 0 이며, 각 검사는 원문 fixture 가 명시한 값만 덮어쓴다. 실제 게임·브라우저 실행이 아니다.
// (2026-11-09 WB04-E200: E199 shape-to-buffer 시험의 장치를 Burning 프레임 시험과 공유하도록 분리)
import assert from 'node:assert/strict';
import { readData } from './helpers.mjs';
import { TaffArchive } from '../www/assets/taff.js';
import { TypeTable } from '../www/assets/type-table.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { buildTypeRecords } from '../www/assets/type-flags.js';
import { ShapeDatabase } from '../www/assets/shapes.js';
import { SidPool } from '../www/core/sid-pool.js';
import { squidRendererShapes } from '../www/render/squid-renderer.js';

// 정의의 maxHitPoints 속성 값이다(없으면 원본 초기값 0). (2026-11-09 WB04-E200)
function maxHitPointsOf(definition) {
    const property = definition?.properties.find((item) => item.name.toLowerCase() === 'maxhitpoints');
    return property && typeof property.value === 'number' ? Math.trunc(property.value) : 0;
}

// 한 번만 읽은 자료 묶음이다. (2026-11-09 WB04-E200)
let cached = null;

// { table, database, types, shapes } 를 돌려준다. types 는 매 검사가 slice 해 덮어쓸 수 있는 188칸 레코드 배열이다. (2026-11-09 WB04-E200)
export function realCaptureAssets() {
    if (cached) return cached;
    const table = TypeTable.fromArchive(new TaffArchive(readData('netstorm.tarc')), decodeOriginalText);
    const database = new ShapeDatabase(readData('d/_shapes.shp'));
    const records = buildTypeRecords(table.definitions);
    const types = records.map((record, number) => ({ ...record, maxHitPoints: number >= 70 ? maxHitPointsOf(table.definitions[number - 70]) : 0, hotspotX: 0, hotspotY: 0 }));
    cached = { table, database, types, shapes: squidRendererShapes(table, database) };
    return cached;
}

// 원본 격리 장면과 같은 client 50 슬롯까지 확보한 풀이다. (2026-11-09 WB04-E200)
export function clientPool() {
    const pool = new SidPool(32768);
    // client 영역에서 SID 5..50 을 차례로 확보한다.
    for (let sid = 5; sid <= 50; sid += 1) assert.equal(pool.allocate(2), sid);
    return pool;
}

// 바이트의 소문자 16진 문자열이다(빈 버퍼는 fixture 처럼 '-'). (2026-11-09 WB04-E200)
export const hexOrDash = (bytes) => (bytes.length ? Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength).toString('hex') : '-');
