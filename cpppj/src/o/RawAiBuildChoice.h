// 사제AI의 다음 건설TYPE와 수집자 여유 비용/적 자금 조건을 같은 본체에서 판단한다.
#pragma once
#include "o/RawAiBrain.h"
#include <array>
#include <string_view>

namespace netstorm::o {
// 원본 전역의 현재 저장소/조회기를 빌린다.
struct AiBuildChoiceGlobals {
    const std::uint32_t& localOwner; //00540c70의 현재 로컬 소유자다.
    const std::uint32_t& playerKnown; //00595344가0이면 분산SP를 읽지 않고 원본0.0f를 쓴다.
    const std::uint32_t& collectorType; //0054126c의 현재 신전 생산TYPE이며 초기값133이다.
    const std::uint32_t& daisType; //005412ec의 현재 다이스TYPE다.
    std::span<const std::uint32_t> workshops; //004709d0의현재0..3 워크샾TYPE다.4/5는0이다.
    std::function<std::uint32_t(std::uint32_t)> temple; //00470a30의remainder0..2에대응하는현재541244/248/24c다.
};
// 기존 실제 사제 상태/비용/지갑/SP와 원본 진단만 요청한다.
struct AiBuildChoiceHooks {
    std::function<bool(std::uint32_t)> captured; //00492340의현재참여/ownerState1이다.
    std::function<bool(std::uint32_t)> dead; //004922e0의현재참여/ownerState2다.
    std::function<bool(std::uint32_t,std::uint32_t)> canAfford; //다이스관문의같은00410280이다.
    std::function<std::int32_t(std::uint32_t)> wallet; //00413c20의현재Player+b0→실제AI2d8이다.
    std::function<float(std::uint32_t)> storedSp; //0040ec50의현재분산SP이며지갑배열로대체하지않는다.
    std::function<void(std::string_view,std::uint32_t)> assertion; //Isleinfo.cpp의복귀가능한원본진단/줄번호다.
};
// 독립 구매 추천/수량표 없이 원본 지갑/선호 목록/현재 TYPE 비용을 읽는다.
class RawAiBuildChoice {
public:
    RawAiBuildChoice(RawAiBrainMemory& memory,std::span<const RiftTypeRecord> types,AiBuildChoiceGlobals globals,AiBuildChoiceHooks hooks); //같은실제188TYPE/워크샾4/본체를빌린다.
    bool ReserveForCollectors(std::uint32_t token) const; //00411a40:ftol(현재133전역cost)*234의감김signed값<=현재지갑이다.
    bool EnemyLowFunds(std::uint32_t token) const; //00411ad0:원격지갑또는현재로컬SP절삭값이1000미만인지판단한다.
    std::uint32_t Select(std::uint32_t token) const; //00411b40전체다이스→신전→선호워크샾TYPE선택이다.
    std::uint32_t WorkshopType(std::int32_t theme) const; //004709d0:0..3현재값,4/5는0,범위밖진단뒤0이다.
    std::uint32_t TempleType(std::int32_t theme) const; //00470a30:signed%5의0..2만현재신전,그외진단뒤0이다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //현재실제0xb08본체만허용한다.
    std::uint32_t Field(std::uint32_t token,std::size_t at) const; //현재본체DWORD만읽는다.
    const RiftTypeRecord& Type(std::uint32_t type) const; //현재TYPE를물리범위검사뒤조회한다.
    RawAiBrainMemory& memory_; //다른AI와같은본체/목록소유자다.
    std::span<const RiftTypeRecord> types_; //현재cost를읽는같은188TYPE다.
    AiBuildChoiceGlobals globals_; //복사상수가아닌현재전역참조/조회기다.
    AiBuildChoiceHooks hooks_; //선택된실제하위와진단효과다.
};
}
