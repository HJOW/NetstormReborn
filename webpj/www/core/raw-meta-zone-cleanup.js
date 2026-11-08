// 메타맵 51구역의 고정 기록표 정리 부모(0042b1b0)와 마커 정리(00479520)를 옮긴 모듈이다(cpppj `o/RawMetaZoneCleanup.cpp/.h`). 00546908 의 51×144바이트(7344바이트)이며 State 다섯 블록과 다른 표이다.
//  - clearMetaZoneMarker(records, index, hooks): index 가 51 이상이면 거부한다. 해당 기록(index×144)의 signed 상태 DWORD 가 0 이하이면 반환하고, +76 의 SID DWORD 가 0 이면 반환한다. 아니면 원시 DWORD SID 와 flags 0 으로
//    실제 가상 삭제(hooks.destroy)를 부른 뒤, 삭제가 필드를 바꿨더라도 복귀 뒤 같은 DWORD 한 칸만 0 으로 쓴다(WORD 로 줄이거나 free/dead 필터를 더하지 않으며 삭제 실패 뒤 SID 0 을 앞당기지 않음).
//  - clearMetaZoneRecords(globals, hooks): 진입 때 첫 플래그(firstReset)가 비영이면 표 전체를 0 으로 쓰고 플래그를 0 으로 써 돌아간다(자식 호출 없음). 진입 때 0 이면 index 0..50 의 마커 자식을 51회 모두 부른 뒤 표를 0 으로 쓴다
//    (callback 이 플래그를 바꿔도 진입 때 정한 분기를 유지하며 바뀐 플래그는 보존). 표 쓰기는 낮은 주소부터 DWORD 단위로 하고, 물리가 부족하면 그 DWORD 에서 거부하며 앞서 완료한 쓰기는 남긴다.
// globals = { firstReset(00531760 DWORD, 현재 값을 읽고 쓰는 속성), records(Uint8Array, 00546908 의 고정 물리) } 이고 hooks = { clearMarker(index) } / { destroy(sid, flags) } 이다. 도달한 실제 자식만 요구한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E77: 메타 구역 기록 정리 이식으로 추가)

// 원문 index 0..50 의 구역 수, 기록 한 칸 폭, 전체 표 폭, 기록 안 상태 DWORD(+0)와 SID DWORD(+76) 위치이다. (2026-11-08 WB04-E77)
export const META_ZONE_COUNT = 51;
export const META_ZONE_STRIDE = 144;
export const META_ZONE_BYTES = 7344;
const STATE_AT = 0;
const SID_AT = 76;

// 원문이 실제 읽는 현재 DWORD 만 검사하며 생략되는 SID 필드의 물리를 앞당겨 요구하지 않는다. (2026-11-08 WB04-E77)
function metaWord(records, offset) {
    if (offset > records.length || 4 > records.length - offset) throw new RangeError('메타구역 마커의 현재 DWORD 물리 부족');
    // 정렬되지 않은 실제 필드의 낮은 BYTE 부터 조합한다.
    return (records[offset] | (records[offset + 1] << 8) | (records[offset + 2] << 16) | (records[offset + 3] << 24)) >>> 0;
}

// 실제 rep stosd 의 한 DWORD 폭을 검사하고 앞서 완료한 쓰기는 남긴다. (2026-11-08 WB04-E77)
function zeroMetaRecords(records) {
    // 원문 0x72c DWORD 만 낮은 주소부터 쓴다.
    for (let offset = 0; offset < META_ZONE_BYTES; offset += 4) {
        if (offset > records.length || 4 > records.length - offset) throw new RangeError('메타구역 정리의 실제 DWORD 물리 부족');
        // 현재 DWORD 전체 폭 이외에는 쓰지 않는다.
        for (let byte = 0; byte < 4; byte += 1) records[offset + byte] = 0;
    }
}

// 상태와 SID 를 각 원문 위치에서 한 번 읽고 가상 복귀 뒤에는 같은 물리 필드에만 0 을 쓴다. (2026-11-08 WB04-E77)
export function clearMetaZoneMarker(records, index, hooks) {
    if (!Number.isInteger(index) || index < 0 || index >= META_ZONE_COUNT) throw new RangeError('메타구역 마커의 51기록 범위');
    const offset = index * META_ZONE_STRIDE;
    if ((metaWord(records, offset + STATE_AT) | 0) <= 0) return;
    const sid = metaWord(records, offset + SID_AT);
    if (!sid) return;
    if (typeof hooks?.destroy !== 'function') throw new Error('메타구역 마커의 실제 가상 삭제 미연결');
    hooks.destroy(sid, 0);
    // 실제 삭제가 필드를 바꿔도 복귀 뒤 같은 DWORD 만 지운다.
    for (let byte = 0; byte < 4; byte += 1) records[offset + SID_AT + byte] = 0;
}

// 인자 바인딩은 초기화 효과가 없으며 선택한 하위만 실제 호출 위치에서 요구한다. (2026-11-08 WB04-E77)
export function clearMetaZoneRecords(globals, hooks) {
    if (globals.firstReset !== 0) {
        zeroMetaRecords(globals.records);
        globals.firstReset = 0;
        return;
    }
    // callback 의 플래그 변경에도 진입 때 선택한 51회 분기를 유지한다.
    for (let index = 0; index < META_ZONE_COUNT; index += 1) {
        if (typeof hooks?.clearMarker !== 'function') throw new Error('메타구역 정리의 실제 marker 자식 미연결');
        hooks.clearMarker(index);
    }
    zeroMetaRecords(globals.records);
}
