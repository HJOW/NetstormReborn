// State 정리의 작은 몸체를 Client 장기 전역과 같은 실제 표면 저장소에 연결한다.
// 사용: resetClientStateWorldStorage({ gameplay, local, storage, currentStorage, gameTime }, effect). currentStorage는 아직 없으면 null이다.
import { resetClockSynchronization } from './raw-clock-sync-reset.js';
import { resetStateZone } from './raw-state-zone-reset.js';
import { resetStateOwnerColors, resetStateBridgeDecayScan } from './raw-state-bridge-reset.js';
import { resetRegisteredStateSidLists } from './raw-state-sid-lists.js';
import { resetTotalMadeCounts } from './raw-total-made.js';
import { resetStatePlayerNoMakeWarnings, resetStateSpotMap, StateWorldCleanupEffect } from './raw-state-world-cleanup.js';
import { resetRawIslandRecords } from './raw-island-runtime.js';
import { resetRawGraphTable } from './raw-graph.js';
import { resetSquidWorldPool } from './raw-squid-pool-reset.js';
import { clearMetaZoneMarker, clearMetaZoneRecords } from './raw-meta-zone-cleanup.js';
import { releaseRuntimePin } from './runtime-pin.js';

// 같은 Client 물리의 PATCH 표면과 현재 선택된 저장소만 허용한다.
function validateStorage(context) {
    const { gameplay, storage, currentStorage } = context;
    if (storage.pool.edition !== 'patch1078' || storage.authority !== gameplay.authority() || storage.gameAux !== gameplay.gameAux() || (currentStorage && currentStorage !== storage)) {
        throw new TypeError('State 물리 정리의 실제 Client/표면 owner 불일치');
    }
}

// 같은 Client의 세 동기화 DWORD만 초기화한다.
export function resetClientClockSynchronization(local) { resetClockSynchronization(local.clockSynchronization()); }
// 같은 Client의 Zone 앞 6바이트만 초기화한다.
export function resetClientStateZone(local) { resetStateZone(local.zone()); }

// 선택된 저장소 정리만 실행하며 대여 중에는 소유자 색 외의 물리를 건드리지 않는다.
export function resetClientStateWorldStorage(context, effect) {
    const { gameplay, local, storage, gameTime } = context; validateStorage(context);
    const E = StateWorldCleanupEffect;
    if (effect === E.Clear43b540) { resetStateOwnerColors(storage.links.ownerColors); return; }
    if (storage.borrowed) throw new Error('State 물리 정리 전 실제 World 대여 반납 필요');
    switch (effect) {
    case E.Clear472830: resetRegisteredStateSidLists(storage.sidLists.registeredHeaders, (header) => storage.sidLists.resetCount(header, gameplay.selection)); return;
    case E.Clear4c2450: resetTotalMadeCounts(storage.totalMade); return;
    case E.Clear4b2b50: storage.hash.reset(); return;
    case E.Clear4328a0: resetStatePlayerNoMakeWarnings(local.playerNoMakeWarnings()); return;
    case E.Clear4ab940: resetStateSpotMap(storage.spots); return;
    case E.Reset568b60: resetRawIslandRecords(storage.islands); return;
    case E.Reset54c8a0: storage.chunks.reset(); return;
    case E.ResetGraph: resetRawGraphTable(storage.graph); return;
    case E.Reset422490: resetStateBridgeDecayScan(storage.bridgeDecayScan, storage.bridgeDecayPeriod, gameTime()); return;
    default: throw new RangeError('State 물리 정리의 미지원 실제 Effect');
    }
}

// World가 반납된 같은 장기 풀의 하위 reset과 실패점·삭제 이력을 순서대로 초기화한다.
export function resetClientSquidPool(context, capacity = 0) {
    validateStorage(context); const { storage } = context;
    if (storage.borrowed) throw new Error('State Pool 정리 전 실제 World 대여 반납 필요');
    resetSquidWorldPool(storage.pool, storage.boardTargetState.invalid, capacity);
}

// 메타 구역의 비영 마커만 현재 Runtime을 고정해 실제 가상 삭제한다.
export function clearClientMetaZoneRecords(context) {
    validateStorage(context); const { gameplay, local, storage } = context, globals = local.metaZones();
    clearMetaZoneRecords(globals, { clearMarker: (index) => clearMetaZoneMarker(globals.records, index, { destroy: (sid, flags) => {
        const session = gameplay.requireSession(), pin = gameplay.pinCurrentRuntime(session);
        try {
            const world = session.world();
            if (world.storage !== storage) throw new Error('메타구역 삭제의 현재 Session/Pool 불일치');
            if (sid >= storage.pool.capacity || sid > 0xffff) throw new RangeError('메타구역 DWORD SID의 현재 Pool 물리 범위');
            world.destroy(sid, flags);
        } finally { releaseRuntimePin(pin); }
    } }) });
}
