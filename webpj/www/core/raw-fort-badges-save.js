// 같은 Client의 28x220BYTE 장부와 실제 MemStream16BYTE를 빌려 Badges를 저장한다(225553df, WB04-E633).
// 사용: saveFortBadges(slot, { manager, assertion, create, count, access }). access는 동기 콜백이다.
// 원본 Template/MemStream의 실제 진단 행 번호다.
export const FORT_BADGES_SAVE_ASSERTION = Object.freeze({ missingTemplate: 0x412, writeCapacity: 0x24 });
// 원본 장부의 기록 수·간격·count 위치이며 다른 padding은 건드리지 않는다.
export const FORT_BADGE_RECORD_COUNT = 28, FORT_BADGE_RECORD_STRIDE = 220, FORT_BADGE_COUNT_OFFSET = 216;
// 도달하지 않은 장치를 미리 검사하지 않는다.
function need(device) { if (typeof device !== 'function') throw new Error('Badges 저장의 실제 하위 경계 미연결'); return device; }
// 현재 비정렬 DWORD만 읽거나 쓰며 실제 물리 밖 접근은 거부한다.
function view(bytes, at) { if (at < 0 || at > bytes.length || bytes.length - at < 4) throw new RangeError('Badges DWORD 실제 물리 범위'); return new DataView(bytes.buffer, bytes.byteOffset + at, 4); }
// 현재 DWORD의 unsigned 값을 읽는다.
function read(bytes, at) { return view(bytes, at).getUint32(0, true); }
// 현재 DWORD만 little endian으로 쓴다.
function put(bytes, at, value) { view(bytes, at).setUint32(0, value >>> 0, true); }
// 현재 출력 BYTE만 쓰고 잘못된 물리 주소를 무시하지 않는다.
function writeByte(bytes, position, value) { if (position >= bytes.length) throw new RangeError('Badges 출력 BYTE 실제 물리 범위'); bytes[position] = value; }
// BYTE 쓰기 뒤 현재 pos/end를 다시 읽고 DWORD 감김과 signed 비교로 갱신한다.
function advance(fields) { const next = (read(fields, 4) + 1) >>> 0, end = read(fields, 8); put(fields, 4, next); if ((next | 0) > (end | 0)) put(fields, 8, next); return next; }
// 첫 type 일치의 전체 DWORD를 반환하며 중복과 type255 이상도 원본대로 비교한다.
export function readFortBadgeCount(badges, type) {
    if (!(badges instanceof Uint8Array) || badges.length !== FORT_BADGE_RECORD_COUNT * FORT_BADGE_RECORD_STRIDE) throw new RangeError('Badge getter의 실제28x220BYTE 장부 누락');
    // 같은 장부를 앞에서부터 검색하고 첫 일치에서만 반환한다.
    for (let index = 0; index < FORT_BADGE_RECORD_COUNT; index++) { const at = index * FORT_BADGE_RECORD_STRIDE; if (read(badges, at) === (type >>> 0)) return read(badges, at + FORT_BADGE_COUNT_OFFSET); }
    return 0;
}
// 004be7f0: 최초 count 할당/위치를 끝까지 보관하며 type0..254의 비0 count만 두 BYTE로 쓴다.
export function saveFortBadges(slot, hooks) {
    slot |= 0; if (!need(hooks.manager)(slot)) need(hooks.assertion)(FORT_BADGES_SAVE_ASSERTION.missingTemplate);
    const manager = need(hooks.manager)(slot), stream = need(hooks.create)(manager, 'Badges');
    let allocation, position; // 최초 count BYTE의 실제 살아 있는 할당/주소이며 뒤의 data/pos 변경과 구별한다.
    need(hooks.access)(stream, (fields, memory) => {
        position = read(fields, 4); const data = read(fields, 0), end = read(fields, 8); allocation = need(memory.buffer)(data);
        const next = (position + 1) >>> 0; put(fields, 4, next); if ((next | 0) > (end | 0)) put(fields, 8, next);
        writeByte(allocation, position, 0);
    });
    // type255는 제외하고 각 getter가 반환한 count DWORD를 보관한다.
    for (let type = 0; type < 255; type++) {
        const count = need(hooks.count)(type) >>> 0; if (!count) continue;
        need(hooks.access)(stream, (fields, memory) => {
            if (((read(fields, 4) + 1) | 0) > (read(fields, 12) | 0)) need(hooks.assertion)(FORT_BADGES_SAVE_ASSERTION.writeCapacity);
            const typePosition = read(fields, 4), typeData = read(fields, 0); writeByte(need(memory.buffer)(typeData), typePosition, type & 255);
            const afterType = advance(fields);
            if (((afterType + 1) | 0) > (read(fields, 12) | 0)) need(hooks.assertion)(FORT_BADGES_SAVE_ASSERTION.writeCapacity);
            const countData = read(fields, 0), countPosition = read(fields, 4); writeByte(need(memory.buffer)(countData), countPosition, count & 255);
            advance(fields); writeByte(allocation, position, (allocation[position] + 1) & 255);
        });
    }
}
