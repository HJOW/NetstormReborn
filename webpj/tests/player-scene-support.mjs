// Player 월드/Regular 검사(WB04-E55)가 공유하는 실제 10.78 자료 장치이다. cpppj DudeSpawnSupport 의 Assets(Patch1078)에 대응한다:
// 웹 준비 자료(www/data 의 netstorm.tarc·d/_shapes.shp)에서 188칸 TYPE 레코드(플래그·발자국·zOrder·생성자 주소)와 SHP 셀 크기를 만든다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E55: Player 월드/Regular 검사와 함께 추가)
import { readData } from './helpers.mjs';
import { adler32 } from './raw-support.mjs';
import { TaffArchive } from '../www/assets/taff.js';
import { TypeTable, FIRST_ASSET_TYPE_NUMBER } from '../www/assets/type-table.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { buildTypeRecords } from '../www/assets/type-flags.js';
import { ShapeDatabase } from '../www/assets/shapes.js';
import { typeDrawDepth } from '../www/assets/terrain-scene.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';

// 한 번만 읽은 실제 자료 묶음이다. (2026-11-08 WB04-E55)
let cached = null;

// 실제 TYPE 188칸과 프레임 크기 조회를 돌려준다. 레코드는 { flags1, flags2, footX, footY, group, maxHitPoints, zOrder, cost, constructorAddress } 이며
// 자료에 없는 maxHitPoints·cost 는 원본 초기값 0 이다. frameSize(type, frame) 은 SquidDisplay::FrameSize(10.78)의 셀 폭/높이(float)이다. (2026-11-08 WB04-E55)
export function realPlayerAssets() {
    if (cached) return cached;
    const archive = new TaffArchive(readData('netstorm.tarc'));
    const table = TypeTable.fromArchive(archive, decodeOriginalText);
    const shapes = new ShapeDatabase(readData('d/_shapes.shp'));
    const records = buildTypeRecords(table.definitions);
    // 번호마다 자산 정의의 zOrder 와 내장 생성자 주소를 붙인다.
    const types = records.map((record, number) => {
        const definition = number >= FIRST_ASSET_TYPE_NUMBER ? table.definitions[number - FIRST_ASSET_TYPE_NUMBER] ?? null : null;
        return {
            ...record,
            maxHitPoints: record.maxHitPoints ?? 0,
            cost: record.cost ?? 0,
            zOrder: definition ? typeDrawDepth(definition) : 0,
            constructorAddress: typeConstructorAddress(number),
        };
    });
    const frames = Array.from({ length: types.length }, () => []);
    // 타입 번호의 SHP 블록에서 셀 폭/높이를 읽는다.
    const frameSize = (type, frame) => {
        const view = table.view(type);
        if (!view || view.block < 0 || frame < 0 || frame >= view.frameCount) throw new RangeError('Player 검사 SHP 프레임 범위 오류');
        const metrics = shapes.squidMetrics(view.block, frame);
        return [metrics.cellWidth, metrics.cellHeight];
    };
    cached = Object.freeze({ types, frames, frameSize, view: (type) => table.view(type) });
    return cached;
}

// 바이트 배열의 소문자 16진 문자열이다. (2026-11-08 WB04-E55)
export const hex = (bytes) => Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength).toString('hex');

// 정수 배열 원소를 지정 폭의 little endian 바이트로 펼친 뒤 Adler-32 를 구한다(cpppj WordAdler<T>). (2026-11-08 WB04-E55)
export function wordAdler(words, width) {
    const bytes = new Uint8Array(words.length * width);
    let at = 0;
    // 각 원소의 낮은 바이트부터 기록한다(큰 해시 배열도 배열 복사 없이 한 번 훑는다).
    for (let index = 0; index < words.length; index += 1) {
        const word = words[index];
        for (let byte = 0; byte < width; byte += 1) bytes[at++] = (word >>> (byte * 8)) & 0xff;
    }
    return adler32(bytes);
}

// 여러 정수 배열을 순서대로 이어 붙인 것의 WordAdler 이다(해시 네 단계 머리 목록용). (2026-11-08 WB04-E55)
export function concatWordAdler(parts, width) {
    const total = parts.reduce((sum, part) => sum + part.length, 0);
    const joined = new Uint32Array(total);
    let at = 0;
    // 각 배열을 원래 순서대로 복사한다.
    for (const part of parts) {
        joined.set(part, at);
        at += part.length;
    }
    return wordAdler(joined, width);
}

// double 의 64비트 비트값(10진 문자열)과 그 역변환이다(cpppj bit_cast<uint64>). (2026-11-08 WB04-E55)
export function doubleBits(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return view.getBigUint64(0, true).toString();
}
export function bitsDouble(text) {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, BigInt(text), true);
    return view.getFloat64(0, true);
}
