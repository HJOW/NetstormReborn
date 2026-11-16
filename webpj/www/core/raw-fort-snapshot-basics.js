// TypeNames/Deck snapshot의 실제 인라인 writer다(225553df, WB04-E628).
// 사용: snapshotFortTypeNames(slot, liveGlobals, hooks), snapshotFortDeck(slot, hooks).
// liveGlobals.templateOwner/typeCount는 C++ const 참조처럼 getter로 현재 칸을 읽는다.
// 일반 count BYTE 쓰기와 인라인 폭별 쓰기를 구별하며 현재 섹션으로 stream을 대체하지 않는다.
import { nameHash } from '../assets/type-flags.js';

// 원문 진단 종류다. 실제 행 번호 매핑은 외부의 Template/MemStream 진단 장치가 맡는다.
export const FORT_SNAPSHOT_ASSERTION = Object.freeze({ missingNamesTemplate: 'MissingNamesTemplate', namesNotMine: 'NamesNotMine', missingDeckTemplate: 'MissingDeckTemplate', writeCapacity: 'WriteCapacity' });
// 선택한 경계만 도달 위치에서 요구한다. 이미 쓴 접두나 난수 효과를 되돌리지 않는다.
function need(device) { if (typeof device !== 'function') throw new Error('Fort snapshot 실제 경계 미연결'); return device; }
// 비정렬 DWORD의 실제 네 BYTE만 읽으며 물리 범위 밖을0으로 채우지 않는다.
function read(bytes, offset) {
    if (offset > bytes.length || bytes.length - offset < 4) throw new RangeError('Fort snapshot DWORD 물리 범위');
    return new DataView(bytes.buffer, bytes.byteOffset + offset, 4).getUint32(0, true);
}
// 같은 stream의 지정 DWORD만 쓰고 인접 필드를 보존한다.
function put(bytes, offset, value) {
    if (offset > bytes.length || bytes.length - offset < 4) throw new RangeError('Fort snapshot DWORD 쓰기 범위');
    new DataView(bytes.buffer, bytes.byteOffset + offset, 4).setUint32(0, value >>> 0, true);
}
// 인라인 BYTE/DWORD 쓰기는 진단 뒤 현재 data/pos를 다시 읽고, 출력 뒤 현재 pos/end를 다시 갱신한다.
function append(stream, value, width, hooks) {
    need(hooks.access)(stream, (fields, memory) => {
        if ((read(fields, 12) | 0) < ((read(fields, 4) + width) | 0)) need(hooks.assertion)(FORT_SNAPSHOT_ASSERTION.writeCapacity);
        const buffer = need(memory.buffer)(read(fields, 0)); const position = read(fields, 4);
        if (position > buffer.length || width > buffer.length - position) throw new RangeError('Fort snapshot 실제 인라인 버퍼 범위');
        // 이미 읽은 원본 값의 지정 폭만 낮은 BYTE부터 쓴다. 같은 stream 자체를 덮는 입력도 분리하지 않는다.
        for (let byte = 0; byte < width; byte++) buffer[position + byte] = value >>> (byte * 8);
        const next = (read(fields, 4) + width) >>> 0; put(fields, 4, next); if ((read(fields, 8) | 0) < (next | 0)) put(fields, 8, next);
    });
}
// 보관한 같은 덱의 signed count를 반복 조건마다 현재 물리에서 읽는다.
function deckCount(token, hooks) { return read(need(hooks.deck)(token), 0x14c) | 0; }

// 004bd6f0: Create→현재 State+17→count BYTE→현재 이름의 signed hash DWORD 순서다.
export function snapshotFortTypeNames(slot, globals, hooks) {
    slot |= 0;
    if (!need(hooks.manager)(slot)) need(hooks.assertion)(FORT_SNAPSHOT_ASSERTION.missingNamesTemplate);
    if ((slot >>> 0) !== (globals.templateOwner >>> 0)) need(hooks.assertion)(FORT_SNAPSHOT_ASSERTION.namesNotMine);
    const stream = need(hooks.create)(need(hooks.manager)(slot), 'TypeNames'); const state = need(hooks.state)(slot);
    if (state.length < 18) throw new RangeError('Fort snapshot의 현재 State+17 누락'); if (!state[17]) return;
    const count = new Uint8Array([globals.typeCount]); need(hooks.write)(stream, count);
    // 각 이름/진단 뒤 바뀐 현재 signed 타입 수를 다음 조건에 반영한다.
    for (let index = 0; index < (globals.typeCount | 0); index++) {
        const name = need(hooks.typeName)(index); const end = name.indexOf('\0'); append(stream, nameHash(end < 0 ? name : name.slice(0, end)), 4, hooks);
    }
}
// 004befd0: 실제 owner0..8의 덱을 갱신/캡처한 뒤 각5BYTE 항목의 앞4BYTE만 저장한다.
export function snapshotFortDeck(slot, hooks) {
    slot |= 0;
    if (!need(hooks.manager)(slot)) need(hooks.assertion)(FORT_SNAPSHOT_ASSERTION.missingDeckTemplate);
    const owner = slot >= 1 && slot <= 8 ? slot : 0;
    need(hooks.refreshDeck)(owner); const token = need(hooks.getDeck)(owner); const stream = need(hooks.create)(need(hooks.manager)(slot), 'Deck');
    const count = new Uint8Array([deckCount(token, hooks)]); need(hooks.write)(stream, count);
    // 헤더 쓰기나 용량 진단이 바꾼 현재 count를 매번 읽는다.
    for (let index = 0; index < deckCount(token, hooks); index++) {
        // 각 BYTE를 먼저 캡처한 뒤 그 쓰기의 용량 진단을 수행하며 다섯째 런타임 BYTE는 생략한다.
        for (let byte = 0; byte < 4; byte++) {
            const source = need(hooks.deck)(token); const offset = index * 5 + byte;
            if (offset >= source.length) throw new RangeError('Fort snapshot Deck 실제 항목 범위');
            append(stream, source[offset], 1, hooks);
        }
    }
}
