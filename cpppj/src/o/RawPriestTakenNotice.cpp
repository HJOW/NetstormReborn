// 원문 안내의 이름 인자 평가 순서와20초 억제 중의 음향/기한 재설정도 유지한다.
#include "o/RawPriestTakenNotice.h"
#include <cstdio>
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 원문 안내와 호스트의 지역256BYTE 범위를 구별한다.
namespace { // 원문004cef50의 고정 색 문자열이며 사용자 색 표와 다르다.
constexpr std::array<std::string_view,9> kOwnerColors{"~q","~l","~r","~w","~u","~s","~y","~d","~a"}; // PE의각3BYTE 문자열을 owner0..8순으로 읽는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("사제 포획 안내의 실제 하위 미연결");return hook; } // 도달한 하위 누락만 진단하며 앞선 효과는 되돌리지 않는다.
}
// 이 복원 단계의 실제 판본과 참조 수명만 확인한다.
RawPriestTakenNotice::RawPriestTakenNotice(const SidPool& pool,const std::uint32_t& localOwner,std::array<double,9>& deadlines,PriestTakenNoticeHooks hooks)
    :pool_(pool),localOwner_(localOwner),deadlines_(deadlines),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("사제 포획 안내는10.78 실제 연결 전용");
}
// 기한 억제는 음향을 막지 않으며 최종 시각은 안내 뒤 다시 읽는다.
void RawPriestTakenNotice::Notify(Sid parent,std::uint32_t previousOwner) const {
    Need(hooks_.refresh)(previousOwner);
    const auto owner=pool_.Slot(parent)[34];if (owner==localOwner_) return;
    if (previousOwner>=deadlines_.size()) throw std::out_of_range("사제 포획 안내의 이전 owner 기한 범위");
    const auto color=owner<kOwnerColors.size() ? kOwnerColors[owner] : kOwnerColors[0];
    const auto previousColor=kOwnerColors[previousOwner];std::string_view key;
    if (previousOwner==localOwner_) { key="WarnYourPriestTaken";Need(hooks_.sound)("ourPriestCaptured.wav"); }
    else if (!previousOwner) key="WarnNeutralPriestTaken";
    else { key="WarnPriestTaken";Need(hooks_.sound)("enemyPriestCaptured.wav"); }
    const auto now=Need(hooks_.wallSeconds)(); // 원문은 clock 호출이 끝난 뒤 현재 기한을 읽어 비교한다.
    if (deadlines_[previousOwner]<=now) {
        const auto currentOwner=pool_.Slot(parent)[34]; // 원문은 이름 getter 둘보다 먼저 현재 포획자를 보관한다.
        const auto previousName=Need(hooks_.name)(previousOwner);const auto currentName=Need(hooks_.name)(currentOwner);
        std::array<char,256> output{}; // 원문 지역 버퍼의 크기를 유지하고 범위 밖 쓰기만 거부한다.
        const auto count=std::snprintf(output.data(),output.size(),"%s(%d,%s,%s,%s,%s)",key.data(),static_cast<int>(previousOwner),
            currentName.c_str(),previousName.c_str(),color.data(),previousColor.data());
        if (count<0 || static_cast<std::size_t>(count)>=output.size()) throw std::length_error("사제 포획 안내의 원문256BYTE 서식 범위");
        Need(hooks_.tell)(output.data());
    }
    deadlines_[previousOwner]=Need(hooks_.wallSeconds)()+20.0;
}
}
