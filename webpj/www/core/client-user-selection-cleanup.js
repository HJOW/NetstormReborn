// Client 장기 선택 전역을 실제 표면과 현재 Session에 연결해 State 전환에서 정리한다.
// 사용: clearClientUserSelection(gameplay, storage, selectCursor). 객체 효과는 도달한 경우에만 현재 Session을 요구한다.
import { RawUserHighlight } from './raw-user-highlight.js';
import { RawUserSelection } from './raw-user-selection.js';
import { RawSquidSelection } from './raw-squid-selection.js';

// 원본 004d5c60의 활성 관문과 다중 선택 통지·기본 선택 해제·강조 해제 순서를 유지한다.
export function clearClientUserSelection(gameplay, storage, selectCursor) {
    const owner = gameplay.selection, highlight = new RawUserHighlight(owner.highlightState(), { selectCursor });
    // 실제 객체 통지 때만 현재 Session과 표면의 같은 풀을 확인한다.
    const currentCore = () => {
        const core = gameplay.requireSession().core();
        if (core.pool !== storage.pool) throw new Error('선택 정리의 현재 Session/Pool 불일치');
        return core;
    };
    // 개별 갱신은 도달할 때마다 현재 Session을 다시 확인한다.
    const refresh = (sid) => { const core = currentCore(); core.display.update(core.pool.slot(sid)); };
    const selection = new RawUserSelection(storage.pool, storage.types, owner.selectionState(), {
        enabled: () => owner.enabled, highlight: (mode) => highlight.highlight(mode),
        selectNotice: (sid, on) => { const core = currentCore(); new RawSquidSelection(core.pool, (number) => core.display.update(core.pool.slot(number))).mark(sid, on); }, refresh,
    });
    selection.clear();
}
