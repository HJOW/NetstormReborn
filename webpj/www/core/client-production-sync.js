// 월드가 없는 메뉴 또는 현재 Session의 같은 Client 물리에서 생산 내용을 동기화한다.
// 사용: synchronizeProductionControlContents({ storage, gameplay, players }). Session이 없을 때 선택된 가상 효과는 실제 Runtime을 요구한다.
import { RawCarrierPreDestroy } from './raw-carrier-pre-destroy.js';
import { RawProductionSync } from './raw-production-sync.js';
import { currentAllocationBytes } from './current-allocation-bytes.js';

// 현재 Player 9기록의 선택한 DWORD를 실제 조회 위치에서 읽는다.
function playerField(context, owner, offset) {
    const bytes = context.players.records(), at = owner * 180 + offset;
    if (!Number.isInteger(owner) || owner < 0 || owner >= 9 || offset < 0 || offset > 176 || at + 4 > bytes.length) throw new RangeError('생산의 실제 현재 Player DWORD 없음');
    return new DataView(bytes.buffer, bytes.byteOffset + at, 4).getUint32(0, true);
}

// 실제 대여가 있으면 현재 Session 관문으로, 없으면 같은 장기 Deck·Pool로 원본 동기화를 실행한다.
export function synchronizeProductionControlContents(context) {
    if (!context?.storage || !context.gameplay || !context.players) throw new TypeError('생산 동기화의 실제 Client 문맥 미연결');
    const { storage, gameplay, players } = context;
    if (gameplay.borrowed() || storage.borrowed) { gameplay.requireSession().synchronizeProductionContents(); return; }
    const memory = players.productionDeckMemory();
    // 조회 이외의 Carrier 가상 효과는 현재 Runtime을 요구한다.
    const requireRuntime = () => gameplay.requireSession();
    const carrier = new RawCarrierPreDestroy(storage.pool, gameplay.finder, { validateDamageable: requireRuntime, damageablePre: requireRuntime, dependentFound: requireRuntime });
    const hooks = {
        deck: () => memory.bytes(memory.get(players.records(), gameplay.player.templateOwner, currentAllocationBytes)),
        technology: () => gameplay.player.playerKnown ? playerField(context, gameplay.player.battleOwner, 0x9c) : 0,
        temple: (owner) => playerField(context, owner, 0x7c) !== 0,
        cancelCursor: () => requireRuntime().cursor().cancel(), destroy: (sid, flags) => requireRuntime().world().destroy(sid, flags),
        findKind: (parent, mask, kind, index) => { const sid = carrier.findContained(parent, mask, kind, index); if (!sid) return 0; const raw = storage.pool.slot(sid); return raw[18] | (raw[19] << 8); },
        create: (...args) => requireRuntime().createContained(...args), assertion: (text) => console.error(`생산 동기화 원본 진단: ${text}`),
    };
    const state = { localOwner: () => gameplay.player.battleOwner, fullDeck: () => gameplay.fullDeck().value };
    new RawProductionSync(storage.pool, storage.types, gameplay.finder, state, hooks).synchronize();
}
