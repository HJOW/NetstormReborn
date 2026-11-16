// RawFortTerritoryRecover의 공유004bd600 writer만 옮긴다(225553df, WB04-E588).
// 사용: writeFortTerritoryRecords(고정 슬롯, 120BYTE 또는 null, manager/assertion/create/current/write 장치).
// RecoverFortTerritory(004b5ec0)는 ExistingEditor 전용이므로 웹 범위에서 제외한다.

// 선택된 실제 장치만 요구한다. 누락을 성공으로 채우거나 앞선 효과를 되돌리지 않는다.
function need(fn, label) { if (typeof fn !== 'function') throw new Error(`Territory writer ${label}미연결`); return fn; }
// 현재 manager→진단→Create→현재 manager.current→일반 Write120 순서를 보존한다. Create 반환값은 사용하지 않는다.
export function writeFortTerritoryRecords(slot, records, hooks) {
    slot >>>= 0;
    if (!need(hooks.manager, '현재manager')(slot)) need(hooks.assertion, '실제진단')(521);
    if (records == null) need(hooks.assertion, '실제진단')(522);
    need(hooks.create, '실제Create')(hooks.manager(slot), 'Territory');
    const stream = need(hooks.current, '현재stream')(hooks.manager(slot));
    need(hooks.write, '실제일반쓰기')(stream, records, 120);
}
