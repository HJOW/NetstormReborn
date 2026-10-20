// 실제 미션 옵션의 범위·선택 TYPE 비용·파생값·타입 사용표 초기화를 복원한다.
#pragma once
#include "o/RiftType.h"

namespace netstorm::o {
// 0041cb90/0041cc10의 실제 PATCH 초기화다. BYTE23..62는 기존 값을 보존하고22개 기본 옵션/색0..7을 설치한다.
void ResetMissionBattleOptions(OriginalEdition edition,std::span<std::uint8_t> options);
// 옵션 수신/로그인/TYPE 소비자가 빌리는 같은 현재 저장소다.
struct MissionBattleOptionState {
    std::span<const std::uint8_t> options; // 0052f7e0의 실제87바이트이며 숨김9도 이 안에 있다.
    std::span<RiftTypeRecord> types; // 현재188TYPE의 실제cost(+c4)를 수정한다.
    std::span<std::uint8_t> typeUse; // 005451f8의 실제256바이트 사용표다.
    std::span<const float> rangeTable; // 005425f4의현재4float14/22/30/38이다. 기존0..2 독립 입력은3float도 허용한다.
    const std::uint32_t& selectedType; // 00541240의 현재 비용수정 대상이다.
    float& range; // 0052f488의 현재 범위다.
    float& rangeSquared; // 0052f48c의 현재 범위 제곱이다.
    std::uint32_t& value; // 005424bc의 현재 파생 옵션값이다.
};
// 원본의 작은 순수 몸체를 실제 현재 배열/필드에 연결한다.
class RawMissionBattleOptions {
public:
    // PATCH87옵션/188TYPE/256사용표/3~4float를 정확히 빌린다. 실제 로그인은 기본 인덱스3을 위해4개를 공급한다.
    RawMissionBattleOptions(OriginalEdition edition,MissionBattleOptionState state);
    //004b4860의선택→ordered최대/최소비교→float제곱쓰기다. NaN의원본분기도보존한다.
    void UpdateRange() const;
    //0041c8b0: 현재옵션+10의BYTE가고른1000/2000/3000/5000이다.
    std::int32_t TypeCost() const;
    //0041ca70: 현재옵션+0f의BYTE가고른0/25/50/75/100/150이다.
    std::uint32_t CurrentValue() const;
    //004b4900의현재TYPE+c4쓰기만실행한다. 같은현재TYPE의typed cost필드다.
    void SetSelectedTypeCost(float cost) const;
    //004b4900의작은순서인범위→비용조회/현재TYPE쓰기→현재파생값쓰기를실행한다.
    void ApplyDerived() const;
    //0041c320: 현재256BYTE만전부0으로만들며이웃메모리는건드리지않는다.
    void ClearTypes() const;
private:
    MissionBattleOptionState state_; // 실제 현재 배열과 필드의 참조다. 옵션 사본을 보관하지 않는다.
};
}
