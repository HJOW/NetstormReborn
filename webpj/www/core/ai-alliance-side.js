// 생성 전 AI의 현재 동맹 판정이다. Player 동맹표에 아직 쓰지 않은 설정 목록도 읽는다(225553df AiAllianceSide.h, WB04-E623).
// 600BYTE 설정의 동맹 마스크 위치다. 함수는 원문처럼 해당 DWORD까지의 물리 길이만 요구한다.
export const AI_SETTINGS_ALLY_MASK_OFFSET = 0x23c;
// 사용: 난이도 조회 직전 owner/localOwner와 현재 Player+54 및 설정 원문을 전달한다. 자체 동맹표는 만들지 않는다.
export function isAiEnemyOfLocal(owner, localOwner, localAllies, settings) {
    owner >>>= 0; localOwner >>>= 0; localAllies >>>= 0;
    if (settings.length < AI_SETTINGS_ALLY_MASK_OFFSET + 4) throw new RangeError('AI 아군/적 판정: 설정 동맹 목록 범위 누락');
    if (owner > 8 || localOwner > 8) throw new RangeError('AI 아군/적 판정: owner 범위 오류');
    if (owner === localOwner || ((localAllies >>> owner) & 1)) return false;
    let declared = 0;
    // 비정렬 네 BYTE를 낮은 비트부터 합치고 로컬 비트를 판정한다.
    for (let index = 0; index < 4; index += 1) declared |= settings[AI_SETTINGS_ALLY_MASK_OFFSET + index] << (8 * index);
    return ((declared >>> localOwner) & 1) === 0;
}
