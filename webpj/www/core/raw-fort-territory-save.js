// 캠페인 공유 Territory snapshot·현재 섬 검색/record·빈 seed를 원문 순서로 수행한다(225553df, WB04-E632).
// 사용: snapshotFortTerritory(slot, hooks). 기존120BYTE·현재 World·명시 stream은 같은 호출 수명으로 빌린다.

// 원문 Template521/MemStream36의 실제 진단 종류다. 복귀하면 현재 필드를 다시 읽는다.
export const FORT_TERRITORY_SAVE_ASSERTION = Object.freeze({ missingManager: 'MissingManager', writeCapacity: 'WriteCapacity' });
// 선택된 경계만 그 위치에서 요구하며 앞선 기록/난수/생성 효과를 유지한다.
function need(device) { if (typeof device !== 'function') throw new Error('Territory 저장의 실제 장치 누락'); return device; }
// 지정 폭의 현재 비정렬 원문만 읽으며 실제 물리 밖은 거부한다.
function read(bytes, at, width = 4) { if (at < 0 || at > bytes.length || width > bytes.length - at) throw new RangeError('Territory 현재 필드의 물리 범위'); const view = new DataView(bytes.buffer, bytes.byteOffset + at, width); return width === 1 ? view.getUint8(0) : view.getUint32(0, true); }
// 현재 stream의 같은 DWORD만 쓰고 주변 필드를 보존한다.
function put(bytes, at, value) { if (at > bytes.length || bytes.length - at < 4) throw new RangeError('Territory stream DWORD 물리 범위'); new DataView(bytes.buffer, bytes.byteOffset + at, 4).setUint32(0, value >>> 0, true); }
// 원문 MSVC signed char의 낮은 BYTE 확장이다.
function signedByte(value) { return (value << 24) >> 24; }
// 기존 비트와 현재 소유권을 함께 사용한다. special은 load 뒤 한 번, noRegion/manager는 소비 위치마다 읽는다.
export function snapshotFortTerritory(slot, hooks) {
    slot |= 0; const records = new Uint8Array(120); need(hooks.load)(slot, records); const special = need(hooks.special)() !== 0; let missing = 0;
    // 같은 로컬120BYTE의 여섯 BYTE씩을 지역0..19 순서로 처리한다.
    for (let region = 0; region < 20; region++) {
        const record = records.subarray(region * 6, region * 6 + 6);
        if (!special) {
            const island = need(hooks.island)(slot >= 1 && slot <= 8 ? slot : 0, region);
            if (island !== (need(hooks.noRegion)() | 0) && (record[1] & 1)) record.set(need(hooks.record)(island));
        } else {
            const island = need(hooks.anyIsland)(region);
            if (island === (need(hooks.noRegion)() | 0)) need(hooks.seed)(record, missing++); else record.set(need(hooks.record)(island));
        }
    }
    if (!need(hooks.manager)(slot)) need(hooks.assertion)(FORT_TERRITORY_SAVE_ASSERTION.missingManager);
    need(hooks.create)(need(hooks.manager)(slot)); const stream = need(hooks.current)(need(hooks.manager)(slot));
    need(hooks.access)(stream, (fields, memory) => {
        if ((read(fields, 12) | 0) < ((read(fields, 4) + 120) | 0)) need(hooks.assertion)(FORT_TERRITORY_SAVE_ASSERTION.writeCapacity);
        const position = read(fields, 4); const buffer = need(memory.buffer)(read(fields, 0));
        if (position > buffer.length || buffer.length - position < records.length) throw new RangeError('Territory 현재 출력120BYTE 물리 범위');
        buffer.set(records, position); const next = (read(fields, 4) + 120) >>> 0; put(fields, 4, next); if ((read(fields, 8) | 0) < (next | 0)) put(fields, 8, next);
    });
}
// 진입 count를 보관하고 첫 존재 일치에서 반환한다. 잘못된 큰 count도 실제 접근 위치에서만 거부한다.
export function findFortTerritoryIsland(islands, fort, owner, region) {
    const count = read(islands, 0) | 0; owner |= 0; region |= 0;
    // 현재3204BYTE의 존재/소유/지역 필드만 읽으며 별도 유효성 조건을 더하지 않는다.
    for (let index = 0; index < count; index++) {
        const at = 4 + index * 32; if (!read(islands, at + 4)) continue; const current = read(islands, at + 8);
        if (fort ? (current && current !== (owner >>> 0)) : (signedByte(read(islands, at + 13, 1)) !== owner || current !== (owner >>> 0))) continue;
        if (signedByte(read(islands, at + 12, 1)) === region) return index;
    }
    return 127;
}
// Editor 검색 본체도 현재 소유자/원래 소유자를 읽지 않고 존재하는 첫 signed 지역만 고른다.
export function findAnyFortTerritoryIsland(islands, region) {
    const count = read(islands, 0) | 0; region |= 0;
    // count 전체에 선행 물리 검사를 추가하지 않는다.
    for (let index = 0; index < count; index++) { const at = 4 + index * 32; if (read(islands, at + 4) && signedByte(read(islands, at + 12, 1)) === region) return index; }
    return 127;
}
// 같은 현재 ChunkMap을 y 바깥/x 안쪽으로 검색하고 첫 일치 record6BYTE를 값으로 복사한다.
export function fortTerritoryIslandRecord(chunks, island) {
    // 현재 행/열 크기를 각 조건 위치에서 읽으며 과거 Fort 파일의 기록을 대용하지 않는다.
    for (let y = 0; y < chunks.yLength; y++) {
        // 같은 행에서는 x를 먼저 진행한다.
        for (let x = 0; x < chunks.xLength; x++) { const entry = chunks.at(x, y); if (signedByte(entry.island) === island) return entry.record.slice(); }
    }
    return new Uint8Array(6);
}
// 첫 두 BYTE는 난수 호출 전에 변경하고 반환 뒤 byte3만 쓴다. 중단 뒤 이미 바꾼 필드는 복구하지 않는다.
export function seedFortTerritoryRecord(record, index, random) {
    if (!(record instanceof Uint8Array) || record.length !== 6) throw new RangeError('Territory seed 실제6BYTE 범위');
    record[0] = (record[0] & 0xc0) | (index & 0x3f); record[1] = (record[1] & 0xf7) | 3; const value = need(random)(254); record[3] = (value + 1) & 255;
}
