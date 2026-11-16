// Territory/Chaff의 현재 지도 크기·공유 scope·셀 호출 순서를 보존한다(225553df, WB04-E631).
// 사용: saveFortTerritoryChunks(stream, owner, region, scopeRef, hooks), saveFortChaffChunks(stream, scopeRef, hooks).
// scopeRef.value는 원문 DWORD 참조이며 종료/예외 뒤 원래 값으로 복구하지 않는다.

// 빈 지도에서 선택하지 않은 장치는 요구하지 않고 실제 도달 위치의 누락만 거부한다.
function need(device) { if (typeof device !== 'function') throw new Error('전체 Fort 청크 저장의 실제 경계 누락'); return device; }
// 최초 height는 scope 쓰기 전에 읽고, 행 시작 width는 직전 셀 끝의 값을 그대로 사용한다.
function saveRegionChunks(stream, owner, region, scope, hooks, territory) {
    let height = need(hooks.chunksY)() | 0; scope.value = territory ? 1 : 2; if (height <= 0) return;
    let width = need(hooks.chunksX)() | 0; let y = 0;
    // 행 경계에서 width getter를 추가하지 않는다.
    do {
        let x = 0;
        // 현재 셀의 영역 조회/판정/쓰기 뒤에만 다음 width를 읽는다. Chaff도 셀 조회 자체는 생략하지 않는다.
        if (width > 0) do {
            const byte = need(hooks.chunkRegion)(x, y) & 255;
            if (!territory || need(hooks.allowed)((byte << 24) >> 24, owner, region)) need(hooks.chunk)(stream, x, y);
            width = need(hooks.chunksX)() | 0; x = (x + 1) | 0;
        } while (x < width);
        height = need(hooks.chunksY)() | 0; y = (y + 1) | 0;
    } while (y < height);
}
// 004be230: 같은 고정 owner/region의 선택된 청크를 저장하며 원문 signed BYTE 영역을 전달한다.
export function saveFortTerritoryChunks(stream, owner, region, scope, hooks) { saveRegionChunks(stream, owner, region, scope, hooks, true); }
// 004be350: 영역 반환값/판정만 사용하지 않으며 모든 실제 셀 조회와 변경 효과는 유지한다.
export function saveFortChaffChunks(stream, scope, hooks) { saveRegionChunks(stream, 0, 0, scope, hooks, false); }
