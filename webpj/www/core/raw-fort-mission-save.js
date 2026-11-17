// 현재 Guide/Mission의 이름·sequence·flags를 같은 stream에 저장한다(225553df, WB04-E633).
// 사용: saveFortMission(stream, hooks). name은 NUL을 포함한 살아 있는 Uint8Array 또는 null이다.
// null 이름의 원본 Template.cpp155 진단이며 일반 쓰기 진단과 구별한다.
export const FORT_MISSION_SAVE_ASSERTION = Object.freeze({ nullName: 'NullName' });
// 선택된 callback만 도달한 위치에서 요구한다.
function need(device) { if (typeof device !== 'function') throw new Error('Mission 저장의 실제 경계 미연결'); return device; }
// callback 전에 보관한 DWORD의 낮은 지정 폭만 little endian으로 쓴다.
function word(stream, value, width, hooks) { const bytes = new Uint8Array(4); new DataView(bytes.buffer).setUint32(0, value >>> 0, true); need(hooks.write)(stream, bytes.subarray(0, width)); }
// 004bcbf0: 최초 길이의 하위 WORD 뒤 같은 원문 주소의 최초 전체 길이를 쓴다.
export function saveFortMissionName(stream, name, hooks) {
    if (name == null) { need(hooks.assertion)(FORT_MISSION_SAVE_ASSERTION.nullName); name = new Uint8Array(1); }
    if (!(name instanceof Uint8Array)) throw new TypeError('Mission 이름의 실제 BYTE 범위');
    const length = name.indexOf(0); if (length < 0) throw new RangeError('Mission 이름의 실제 NUL 물리 범위 누락');
    word(stream, length, 2, hooks); need(hooks.write)(stream, name.subarray(0, length));
}
// 00482c40: 첫 쓰기 뒤 Guide가 바뀌어도 선택한 Mission의 현재 flags를 다시 읽는다.
export function saveFortMissionState(stream, mission, hooks) {
    word(stream, need(hooks.field)(mission, 0x58), 4, hooks); word(stream, need(hooks.field)(mission, 0x54), 4, hooks);
}
// 00463a80: 현재 Mission이 있을 때 이름을 쓴 뒤 현재 Mission을 다시 선택한다.
export function saveFortMission(stream, hooks) {
    if (!need(hooks.current)()) return;
    saveFortMissionName(stream, need(hooks.name)(), hooks); saveFortMissionState(stream, need(hooks.current)(), hooks);
}
