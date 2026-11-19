// 미션 Guide의 지연 Fort 요청을 현재 State Poll 뒤에 한 번 소비한다.
// 사용: runCampaignRequestFrame(guide, poll, loadFort). 세 인자는 같은 Client 수명에 속해야 한다.

// Poll 전에 대기한 요청만 처리하고 Poll 중 새로 예약하거나 교체한 요청은 다음 프레임에 남긴다.
export function runCampaignRequestFrame(guide, poll, loadFort) {
    if (typeof poll !== 'function') throw new TypeError('미션 요청 프레임의 실제 State Poll 누락');
    const pending = guide?.pending();
    if (!pending) { poll(); return false; }
    const action = pending.takeAfterPoll(poll);
    if (!action) return false;
    if (action.command !== 'LoadMissionFort') throw new Error(`미션 요청의 다른 전환 미연결: ${action.command}`);
    const request = guide.fortRequest();
    if (!request || request.consumed || request.fort !== action.argument) throw new Error('미션 Fort 대기 요청과 현재 이미지 수명 불일치');
    if (typeof loadFort !== 'function') throw new Error('미션 Fort의 실제 State7 설치 장치 미연결');
    loadFort(request);
    return true;
}
