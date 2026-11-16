// 단일 객체/청크 저장의 필터·현재 필드·고정 count 위치를 보존한다(225553df, WB04-E630).
// 사용: saveFortObject(stream, sid, originX, originY, hooks), saveFortChunk(stream, chunkX, chunkY, hooks).
import { writeMemStream } from './raw-mem-stream.js';

// 원문이 각 조건에서 다시 읽는 전역 키다. localOnlyType은 실제005412ec 다이스 종류다.
export const FORT_OBJECT_SAVE_GLOBAL = Object.freeze({ battle: 'battle', mission: 'mission', editor: 'editor', scope: 'scope', version: 'version', localOwner: 'localOwner', bridgeType: 'bridgeType', localOnlyType: 'daisType', battleMask: 'battleMask', ownerMask: 'ownerMask' });
// Template.cpp의 위치/소유자 진단 종류다. 진단이 복귀하면 뒤 쓰기를 계속한다.
export const FORT_OBJECT_SAVE_ASSERTION = Object.freeze({ xOffset: 'XOffset', yOffset: 'YOffset', owner: 'Owner' });
// 실제 masked __ftol의 signed64 불확정값 경계다.
const INT64_BOUNDARY = 9223372036854775808;
// 선택된 외부 장치만 그 위치에서 요구한다.
function need(device) { if (typeof device !== 'function') throw new Error('Fort 객체 저장의 실제 장치 누락'); return device; }
// 같은 raw 필드의 지정 BYTE/WORD/DWORD만 읽고 물리 범위를 확인한다.
function read(bytes, at, width = 4) {
    if (at > bytes.length || width > bytes.length - at) throw new RangeError('Fort 객체 저장의 raw 필드 범위');
    let value = 0;
    // 낮은 BYTE부터 지정 폭만 조합한다.
    for (let byte = 0; byte < width; byte++) value |= bytes[at + byte] << (byte * 8);
    return value >>> 0;
}
// 현재 stream의 지정 DWORD만 바꾸고 인접 필드는 보존한다.
function put(bytes, at, value) {
    if (at > bytes.length || bytes.length - at < 4) throw new RangeError('Fort 청크 저장의 stream 필드 범위');
    new DataView(bytes.buffer, bytes.byteOffset + at, 4).setUint32(0, value >>> 0, true);
}
// raw float32 비트를 JS 숫자로 정확히 넓힌다.
function numberFromBits(bits) { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits, true); return view.getFloat32(0, true); }
// 외부 Write 전에 같은 BYTE/WORD 값을 캡처한다.
function write(stream, value, width, hooks) { const bytes = new Uint8Array([value, value >>> 8]); need(hooks.write)(stream, bytes.subarray(0, width)); }
// 입력 float32→signed64 절삭/불확정값→하위 DWORD의 signed 재해석이다.
export function fortSaveCoordinate(input) {
    const value = Math.fround(input); if (!Number.isFinite(value) || value >= INT64_BOUNDARY || value < -INT64_BOUNDARY) return 0;
    return Math.trunc(value) | 0;
}
// 004bd940: 초기 좌표만 고정하고 이후 virtual/TYPE/필드는 원문 소비 위치마다 다시 읽는다.
export function saveFortObject(stream, sid, originX, originY, hooks) {
    const raw = () => need(hooks.form)(sid); const global = (key) => need(hooks.global)(key); const g = FORT_OBJECT_SAVE_GLOBAL;
    const x = fortSaveCoordinate(numberFromBits(read(raw(), 14))); const y = fortSaveCoordinate(numberFromBits(read(raw(), 18)));
    if (x >= ((originX + 16) | 0) || y >= ((originY + 16) | 0) || (read(raw(), 40, 1) & 9)) return 0;
    if (global(g.battle) && !global(g.mission) && !global(g.editor)) { const flags = need(hooks.flags2)(sid); if (!(flags & global(g.battleMask))) return 0; }
    // 가상60 뒤의 현재 두 float 비트를 같은 raw 대여에서 캡처한다.
    const island = () => { const current = raw(); return need(hooks.island)(read(current, 14), read(current, 18)); };
    let selected = false;
    if (global(g.scope) === 1) selected = !!need(hooks.robust)(sid) && !!island();
    if (global(g.scope) === 2) { if (!need(hooks.robust)(sid) || island()) return 0; } else if (!selected) return 0;
    if (read(raw(), 10, 1) === global(g.localOnlyType) && read(raw(), 34, 1) !== global(g.localOwner)) return 0;
    const cx = (x - originX) | 0; const cy = (y - originY) | 0;
    if (cx < 0 || cx > 15) need(hooks.assertion)(FORT_OBJECT_SAVE_ASSERTION.xOffset);
    if (cy < 0 || cy > 15) need(hooks.assertion)(FORT_OBJECT_SAVE_ASSERTION.yOffset);
    write(stream, (cx << 4) | (cy & 255), 1, hooks);
    const kind = read(raw(), 10, 1); write(stream, kind === global(g.bridgeType) ? ~read(raw(), 34, 1) : kind, 1, hooks);
    if (need(hooks.flags1)(sid) & 0x20) write(stream, read(raw(), 36, 1), 1, hooks);
    if (need(hooks.flags1)(sid) & 0x1000) write(stream, read(raw(), 12, 1), 1, hooks);
    if (need(hooks.flags1)(sid) & 0x2000) write(stream, read(raw(), 12, 2), 2, hooks);
    if (need(hooks.flags2)(sid) & 4) write(stream, read(raw(), 36, 1), 1, hooks);
    if (!global(g.version) && need(hooks.hasLevel)(sid)) write(stream, need(hooks.level)(sid), 1, hooks);
    if ((global(g.version) | 0) > 1 && (need(hooks.flags2)(sid) & 0x4000)) write(stream, need(hooks.level)(sid), 1, hooks);
    if ((global(g.version) | 0) > 0) { const flags = need(hooks.flags2)(sid); if (flags & global(g.ownerMask)) { if (read(raw(), 34, 1) > 8) need(hooks.assertion)(FORT_OBJECT_SAVE_ASSERTION.owner); write(stream, read(raw(), 34, 1), 1, hooks); } }
    if (need(hooks.flags1)(sid) & 0x20000) need(hooks.contents)(sid, stream); return 1;
}
// 004be020: Begin→marker99→미초기화 countWORD 예약→후보→원래 data+offset에 count를 쓴다.
export function saveFortChunk(stream, chunkX, chunkY, hooks) {
    const x = chunkX << 4; const y = chunkY << 4;
    const area = { left: Math.fround(x), top: Math.fround(y), right: Math.fround((x + 15) | 0), bottom: Math.fround((y + 15) | 0) };
    const originX = fortSaveCoordinate(area.left); const originY = fortSaveCoordinate(area.top); const cursor = need(hooks.begin)(area);
    need(hooks.access)(stream, (fields, memory) => writeMemStream(fields, new Uint8Array([99]), 1, memory));
    let data; let offset;
    need(hooks.access)(stream, (fields) => { data = read(fields, 0); offset = read(fields, 4); const next = (offset + 2) >>> 0; put(fields, 4, next); if ((read(fields, 8) | 0) < (next | 0)) put(fields, 8, next); });
    let count = 0;
    // 현재 후보가0 또는 물리 상한 밖이 될 때만 원래 count 위치를 확정한다.
    for (;;) { const candidate = need(cursor.current)(); if (!candidate || candidate >= need(hooks.poolLimit)()) break; count = (count + need(hooks.object)(stream, candidate, originX, originY)) & 0xffff; need(cursor.next)(); }
    need(hooks.access)(stream, (_fields, memory) => { const buffer = need(memory.buffer)(data); if (offset > buffer.length || buffer.length - offset < 2) throw new RangeError('Fort 청크 countWORD의 고정 출력 범위'); buffer[offset] = count; buffer[offset + 1] = count >>> 8; });
}
