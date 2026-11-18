// 일반 전투 복귀 State6의 결과 배경·음악·정리 전 지역값을 원본 순서로 처리한다.
// 사용: installNativeBattleReturnState(globals, hooks), 이후 같은 snapshot으로 completeNativeBattleReturn(snapshot, outcome, hooks).
// Fort 설치와 멀티플레이 옵션은 외부 장치 경계이며 웹의 실제 구현을 의미하지 않는다.
import { notifyNativeFortEntryLeave, readNativeFortEntrySource } from './native-main-menu-state.js';

// 선택된 외부 장치가 없으면 그 호출 지점에서만 실패한다.
function need(hook) {
    if (typeof hook !== 'function') throw new Error('일반 전투 복귀의 실제 장치 미연결');
    return hook;
}

// 원본 strcpy처럼 이름과 NUL만 쓰며 그 뒤의 배경 물리는 유지한다.
function writeBackground(bytes, text) {
    if (text.length >= bytes.length) throw new RangeError('일반 복귀 배경 이름의 실제 물리 범위');
    // 현재 좁은 문자열의 각 BYTE를 원본 순서로 복사한다.
    for (let index = 0; index < text.length; index += 1) bytes[index] = text.charCodeAt(index) & 255;
    bytes[text.length] = 0;
}

// State6 진입의 지역 결과·출처와 실제 효과 순서를 유지해 외부 Fort 부모의 결과를 돌려준다.
export function installNativeBattleReturnState(g, h) {
    if (g.state.current[0] !== 6) throw new Error('일반 복귀의 현재 State6 계약');
    const saved = { outcome: g.outcome.value >>> 0, source: readNativeFortEntrySource(g.state.phase[0], g.sourceType) };
    if (g.background.length === 0) throw new RangeError('일반 복귀 배경 첫BYTE 범위');
    g.background[0] = 0;
    if (g.outcome.value === 1) { need(h.deleteSavedState)(); writeBackground(g.background, 'victory'); need(h.requestMusic)('fanfare.mus'); }
    else if (g.outcome.value === 2) { need(h.deleteSavedState)(); writeBackground(g.background, 'defeat'); need(h.requestMusic)('defeat.mus'); }
    else if (g.outcome.value !== 0) writeBackground(g.background, 'titleMenu');
    g.outcome.value = 0;
    if (saved.source) {
        need(h.deleteSavedState)(); need(h.requestMusic)('fanfare.mus');
        const name = need(h.sourceBackground)(g.sourceType.value >>> 0);
        if (name !== null && name !== undefined) writeBackground(g.background, name); else saved.source = 0;
    }
    notifyNativeFortEntryLeave(g.state, g.battle, h.leaveBattle);
    need(h.prefix)(); return need(h.installFort)(saved);
}

// 공통 Fort 뒤 원본 결과 안내 또는 멀티플레이 옵션 장치를 선택한다.
export function completeNativeBattleReturn(saved, outcome, hooks) {
    if (!saved.outcome) { need(hooks.multiplayerOptions)(); return; }
    if (!saved.source) need(hooks.tell)(`BattleOver${saved.outcome | 0}`);
    outcome.value = 0;
}
