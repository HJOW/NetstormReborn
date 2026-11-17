// 같은 물리 슬롯으로 펜스 Rebuild·Refresh 부모를 Session에 연결한다(225553df).
import { runFenceTypeParent, FENCE_TYPE_OPERATION } from './raw-fence-type-parent.js';

// 원문 진단 뒤 바뀐 TYPE도 같은 슬롯에서 재조회하며 선택된 실제 하위만 실행한다.
export function dispatchCampaignFence(owner, sid, operation) {
    const c = owner.core;
    runFenceTypeParent(c.pool.physical(sid), sid, () => owner.gameplayGlobals.gameAux().sunFenceType, () => owner.state.gunState.checking, operation, {
        assertion: (expression, _file, line) => {
            if (typeof owner.services.ai?.assertion !== 'function') throw new Error(`펜스 타입 원문 진단 장치 미연결 ${line}`);
            owner.services.ai.assertion(expression);
        },
        body: (target) => {
            if (operation === FENCE_TYPE_OPERATION.rebuild) {
                if (!owner.fenceRebuild) throw new Error('실제 펜스 Rebuild 설치 전 호출'); owner.fenceRebuild.rebuild(target); return;
            }
            if (!owner.fenceLifetime) throw new Error('실제 펜스 Refresh 설치 전 호출'); owner.fenceLifetime.refresh(target);
        },
    });
}
