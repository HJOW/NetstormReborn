// 전체 Objects 부모004be430의 현재 manager·version·Temp 선택/이름 교체를 보존한다(225553df, WB04-E631).
// 사용: saveFortObjects(slot, liveGlobals, hooks). liveGlobals.templateOwner/version은 현재 참조 getter/setter다.

// Template.cpp957/958와 MemStream36의 진단 종류다. 복귀하면 현재 상태로 계속한다.
export const FORT_OBJECTS_SAVE_ASSERTION = Object.freeze({ missingTemplate: 'MissingTemplate', notMine: 'NotMine', writeCapacity: 'WriteCapacity' });
// 선택된 실제 장치만 그 위치에서 요구한다.
function need(device) { if (typeof device !== 'function') throw new Error('Objects 저장 부모의 실제 경계 미연결'); return device; }
// 같은 실제 stream의 DWORD만 읽는다.
function read(bytes, at) { if (at > bytes.length || bytes.length - at < 4) throw new RangeError('Objects stream DWORD 물리 범위'); return new DataView(bytes.buffer, bytes.byteOffset + at, 4).getUint32(0, true); }
// 같은 실제 stream의 DWORD만 쓰고 주변 필드를 보존한다.
function put(bytes, at, value) { if (at > bytes.length || bytes.length - at < 4) throw new RangeError('Objects stream DWORD 쓰기 범위'); new DataView(bytes.buffer, bytes.byteOffset + at, 4).setUint32(0, value >>> 0, true); }
// version BYTE는 진단 전에 캡처한다. 진단 뒤 현재 pos/data로 쓰고 출력 뒤 현재 pos/end를 다시 읽는다.
function version(stream, value, hooks) {
    need(hooks.access)(stream, (fields, memory) => {
        if ((read(fields, 12) | 0) < ((read(fields, 4) + 1) | 0)) need(hooks.assertion)(FORT_OBJECTS_SAVE_ASSERTION.writeCapacity);
        const position = read(fields, 4); const bytes = need(memory.buffer)(read(fields, 0)); if (position >= bytes.length) throw new RangeError('Objects version 출력 BYTE 물리 범위');
        bytes[position] = value; const next = (read(fields, 4) + 1) >>> 0; put(fields, 4, next); if ((read(fields, 8) | 0) < (next | 0)) put(fields, 8, next);
    });
}
// 현재 선택한 Temp의 data/end만 확인한다. pos나 추가 최소 길이 조건을 넣지 않는다.
function hasData(stream, hooks) { let result = false; need(hooks.access)(stream, (fields) => { result = read(fields, 0) !== 0 && read(fields, 8) !== 0; }); return result; }
// 고정 this의20개 Territory→현재 fort일 때 Chaff 순서다. 전체 snapshot/Notify/파일 쓰기는 caller의 경계다.
export function saveFortObjects(slot, globals, hooks) {
    slot |= 0;
    if (!need(hooks.manager)(slot)) need(hooks.assertion)(FORT_OBJECTS_SAVE_ASSERTION.missingTemplate);
    if ((slot >>> 0) !== (globals.templateOwner >>> 0)) need(hooks.assertion)(FORT_OBJECTS_SAVE_ASSERTION.notMine);
    globals.version = 2;
    // 없는 Terr를 먼저 생성하고 매 영역마다 새 Temp·현재 version BYTE·전체 영역 저장을 실행한다.
    for (let region = 0; region < 20; region++) {
        const name = 'Terr' + (region < 10 ? '0' : '') + region;
        if (!need(hooks.find)(need(hooks.manager)(slot), name)) need(hooks.create)(need(hooks.manager)(slot), name);
        const stream = need(hooks.create)(need(hooks.manager)(slot), 'Temp'); version(stream, globals.version & 255, hooks);
        need(hooks.territory)(stream, slot >= 1 && slot <= 8 ? slot : 0, region);
        const selected = need(hooks.select)(need(hooks.manager)(slot), 'Temp', 1);
        if (hasData(selected, hooks)) need(hooks.rename)(need(hooks.manager)(slot), name, 'Temp');
    }
    if (need(globals.fort)()) { const stream = need(hooks.create)(need(hooks.manager)(slot), 'Chaff'); version(stream, globals.version & 255, hooks); need(hooks.chaff)(stream); }
}
