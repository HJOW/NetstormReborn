// 실제 미션 옵션의 범위·선택 TYPE 비용·파생값·타입 사용표 초기화를 복원한다.
#pragma once
#include "o/RiftType.h"

namespace netstorm::o {
class GameRandom; // 일반 State7의 밀도도 SP/지형과 같은 게임 난수 상태를 직접 소비한다.
// 0041cb90/0041cc10의 실제 PATCH 초기화다. BYTE23..62는 기존 값을 보존하고22개 기본 옵션/색0..7을 설치한다.
void ResetMissionBattleOptions(OriginalEdition edition,std::span<std::uint8_t> options);
void UpdateMissionOptionSupplyRange(std::span<const std::uint8_t> options,std::span<const float> choices,float& range,float& squared); //004b4860의 현재 선택/ordered 비교/F32·제곱 쓰기다. Session/가짜256BYTE 사용표를 요구하지 않는다.
std::int32_t MissionOptionTypeCost(std::span<const std::uint8_t> options); //0041c8b0의 현재BYTE10h/1000·2000·3000·5000 조회다.
std::uint32_t MissionOptionRefundPercent(std::span<const std::uint8_t> options); //0041ca70의 현재BYTE0fh/0·25·50·75·100·150 조회다.
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
    std::int32_t MinimumDensity() const; //0041c9b0: 현재 옵션+7의 BYTE가 고른10/15/20/25다.
    std::int32_t MaximumDensity() const; //0041c9e0: 같은 현재 옵션이 고른30/35/40/45다.
    void InitializeDensity(GameRandom& random,std::int32_t& minimum,std::int32_t& maximum,std::uint32_t& percent) const; // State7 서버의 현재 두 한계 쓰기→반값 범위 난수 두 번→감김 합계다.
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
