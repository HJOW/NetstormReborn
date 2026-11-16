// 현재 Template의 Fort/Battle snapshot 부모와 두 현재칸 wrapper다(225553df, WB04-E627).
// 사용: snapshotFortTemplate(slot, globals, hooks) 또는 아래 세 진입에 같은 현재 getter/실제 child를 준다.
// RawCampaignSession의 기술 획득·신전 상승·의식 완료도 공유한다. 파일 저장 UI/편집기는 만들지 않는다.
// 고정 this, Create 반환 stream, 각 child 뒤 전역 재조회와 부분 효과를 보존한다. DOM 의존성은 없다.

// Template.cpp 진단의 실제 행 번호다. 진단이 정상 복귀하면 다음 원문 위치에서 계속한다.
export const FORT_SNAPSHOT_PARENT_ASSERTION = Object.freeze({
    missingFortTemplate: 0x530, notFortMode: 0x531, fortNotMine: 0x532,
    missingBattleTemplate: 0x50e, notBattleMode: 0x50f, battleNotMine: 0x510,
    missingTechnologyTemplate: 0x4a6, missingMissionTemplate: 0x1e8,
});
// 원문 Template 배열의 주소 비교 기준과20BYTE 간격이며 호스트 주소로 역참조하지 않는다.
const TEMPLATE_BASE = 0x005c89f0, TEMPLATE_STRIDE = 20;

// 실제 선택한 장치만 그 위치에서 요구한다. 앞선 child의 효과는 되돌리지 않는다.
function need(device) { if (typeof device !== 'function') throw new Error('전체 Fort snapshot 부모의 실제 경계 미연결'); return device; }
// 원문 LEA의 DWORD 감김 주소다. owner 상위 두 비트가 같은 this를 만드는 경우도 보존한다.
function templateAddress(slot) { return (TEMPLATE_BASE + Math.imul(slot, TEMPLATE_STRIDE)) >>> 0; }
// 같은 원문 주소를 실제20BYTE 기록 경계로 해석한다. 비정렬 물리 입력은 성공 슬롯으로 반올림하지 않는다.
function currentSlot(owner) {
    const delta = templateAddress(owner) - TEMPLATE_BASE;
    if (delta % TEMPLATE_STRIDE) throw new RangeError('현재 Template 주소가 실제20BYTE 기록 경계 밖입니다');
    return (delta / TEMPLATE_STRIDE) | 0;
}
// 기술 부모를 먼저 보관한 뒤 현재 manager와 Create 반환 stream을 별도로 캡처한다.
function technology(slot, hooks) {
    if (!need(hooks.manager)(slot)) need(hooks.assertion)(FORT_SNAPSHOT_PARENT_ASSERTION.missingTechnologyTemplate);
    const parent = need(hooks.technologyParent)(); const manager = need(hooks.manager)(slot);
    const stream = need(hooks.create)(manager, 'Technology'); need(hooks.technology)(parent, stream);
}
// Money와 Mission의 각 manager는 그 위치에서 다시 읽고 Create가 반환한 같은 stream을 넘긴다.
function moneyMission(slot, hooks) {
    const moneyManager = need(hooks.manager)(slot); const moneyStream = need(hooks.create)(moneyManager, 'Money'); need(hooks.money)(moneyStream);
    if (!need(hooks.manager)(slot)) need(hooks.assertion)(FORT_SNAPSHOT_PARENT_ASSERTION.missingMissionTemplate);
    const missionManager = need(hooks.manager)(slot); const missionStream = need(hooks.create)(missionManager, 'Mission'); need(hooks.mission)(missionStream);
}

// 004bf9a0: 같은 고정 this의 진단→TypeNames/기술/Objects/Territory/Deck/Money/Mission/Badges→Notify다.
export function snapshotFortTemplate(slot, globals, hooks) {
    slot |= 0;
    if (!need(hooks.manager)(slot)) need(hooks.assertion)(FORT_SNAPSHOT_PARENT_ASSERTION.missingFortTemplate);
    if (!need(globals.fort)()) need(hooks.assertion)(FORT_SNAPSHOT_PARENT_ASSERTION.notFortMode);
    if (templateAddress(slot) !== templateAddress(need(globals.templateOwner)())) need(hooks.assertion)(FORT_SNAPSHOT_PARENT_ASSERTION.fortNotMine);
    need(hooks.typeNames)(slot); technology(slot, hooks); need(hooks.objects)(slot); need(hooks.territory)(slot); need(hooks.deck)(slot);
    moneyMission(slot, hooks); need(hooks.badges)(slot); need(hooks.notify)(slot);
}
// 004bf880: TypeNames/Badges를 추가하지 않고 Deck을 Objects 앞, CoreData를 마지막 Notify 앞에 둔다.
export function snapshotBattleTemplate(slot, globals, hooks) {
    slot |= 0;
    if (!need(hooks.manager)(slot)) need(hooks.assertion)(FORT_SNAPSHOT_PARENT_ASSERTION.missingBattleTemplate);
    if (!need(globals.battle)()) need(hooks.assertion)(FORT_SNAPSHOT_PARENT_ASSERTION.notBattleMode);
    if (templateAddress(slot) !== templateAddress(need(globals.templateOwner)())) need(hooks.assertion)(FORT_SNAPSHOT_PARENT_ASSERTION.battleNotMine);
    technology(slot, hooks); need(hooks.deck)(slot); need(hooks.objects)(slot); need(hooks.territory)(slot);
    moneyMission(slot, hooks); need(hooks.coreData)(slot); need(hooks.notify)(slot);
}
// 004bfac0: 두 독립 if마다 당시 모드/owner를 읽는다. Battle child가 바꾼 두 번째 입력을 보존한다.
export function snapshotCurrentTemplate(globals, hooks) {
    if (need(globals.battle)()) { const slot = currentSlot(need(globals.templateOwner)()); snapshotBattleTemplate(slot, globals, hooks); }
    if (need(globals.fort)()) { const slot = currentSlot(need(globals.templateOwner)()); snapshotFortTemplate(slot, globals, hooks); }
}
// 004bfb50: 최초 manager0이면 바로 반환한다. Battle이 실행된 경우에만 owner를 다시 읽는다.
export function snapshotCurrentTemplateIfPresent(globals, hooks) {
    let owner = need(globals.templateOwner)(); const slot = currentSlot(owner); if (!need(hooks.manager)(slot)) return;
    if (need(globals.battle)()) { snapshotBattleTemplate(slot, globals, hooks); owner = need(globals.templateOwner)(); }
    if (need(globals.fort)()) snapshotFortTemplate(currentSlot(owner), globals, hooks);
}
