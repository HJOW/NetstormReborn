// 실제 캠페인 미션 Start의 Client/World 외부 효과를 연결한다(10.78 단독 미션).
// 사용: makeCampaignMissionStartEffects(client, world, refreshCommandGump). 호출자와 실제 Client/World는 효과보다 오래 살아야 한다.
import { currentAllocationBytes } from './current-allocation-bytes.js';

// 같은 실제 Client 시계·화면 및 현재 World의 카메라·전체 간헐천 생성기를 빌린다.
export function makeCampaignMissionStartEffects(client, world, refreshCommandGump) {
    if (typeof refreshCommandGump !== 'function') throw new TypeError('미션 Start의 현재 명령 Gump 조회 미연결');
    return {
        settingsBytes: (_owner, bytes) => currentAllocationBytes(bytes),
        gameSeconds: () => client.time().game,
        refreshSp: () => { client.renderer().invalidateAll(); const menu = client.menu(); if (menu) menu.refresh(); },
        refreshCommandGump,
        randomGeysers: () => world.session().generateWorldGeysers(),
        resetCamera: () => world.resetCampaignCamera(),
        unknownAbilityAssertion: (name) => console.info(`미션 AI의 원본 능력 이름 진단: ${name}`),
        unknownAbilityLog: (name) => console.info(`미등록 미션 AI 능력: ${name}`),
    };
}
