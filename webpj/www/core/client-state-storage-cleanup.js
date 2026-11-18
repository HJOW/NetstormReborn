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

// 같은 Client의 세 동기화 DWORD만 초기화한다.
export function resetClientClockSynchronization(local) { resetClockSynchronization(local.clockSynchronization()); }
// 같은 Client의 Zone 앞 6바이트만 초기화한다.
export function resetClientStateZone(local) { resetStateZone(local.zone()); }

// 선택된 저장소 정리만 실행하며 대여 중에는 소유자 색 외의 물리를 건드리지 않는다.
export function resetClientStateWorldStorage(context, effect) {
    const { gameplay, local, storage, currentStorage, gameTime } = context;
    if (storage.pool.edition !== 'patch1078' || storage.authority !== gameplay.authority() || storage.gameAux !== gameplay.gameAux() || (currentStorage && currentStorage !== storage)) {
        throw new TypeError('State 물리 정리의 실제 Client/표면 owner 불일치');
    }
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
