// 부호 있는 합산 넘침 대신 원본 DWORD의 감김을 보존한다.
#include "o/RawTotalMade.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kInitializedEntries=255; // 원본 rep stosd가 초기화하는 DWORD 수다.
constexpr std::size_t kFirstType=70; // genus 집계가 시작하는 첫 실제 자산 타입 번호다.
}
// 실제 판본의 TYPE 크기와 초기화할 물리 저장소만 확인하고 내용을 바꾸지 않는다.
RawTotalMade::RawTotalMade(OriginalEdition edition,std::span<const RiftTypeRecord> types,SquidPostPopState& counts,
    std::span<std::uint32_t> restrictions,std::function<void(TotalMadeAssertion)> assertion)
    :edition_(edition),types_(types),counts_(counts),restrictions_(restrictions),assertion_(std::move(assertion)) {
    if (types.size()!=(edition==OriginalEdition::Patch1078 ? 188U : 171U) || restrictions.size()<kInitializedEntries)
        throw std::invalid_argument("Totalmade 판본/물리 표 크기 오류");
}
// 원본은 허용값을 bool로 저장하지 않고 금지DWORD의0/1로 뒤집어 쓴다.
void RawTotalMade::AllowAll(std::int32_t allowed) const { std::fill_n(restrictions_.begin(),kInitializedEntries,allowed==0 ? 1U : 0U); }
// 사용 가능한 호스트 저장소 밖 쓰기는 원본의 무검사 메모리 접근 대신 명시적으로 거부한다.
void RawTotalMade::Allow(std::int32_t type,std::int32_t allowed) const {
    if (type<0 || static_cast<std::size_t>(type)>=restrictions_.size()) throw std::out_of_range("Totalmade 허용 표 물리 범위");
    restrictions_[static_cast<std::size_t>(type)]=allowed==0 ? 1U : 0U;
}
// 패치 debug 진단을 실행한 뒤에도 원본처럼 getter의 범위 판정을 유지한다.
bool RawTotalMade::Valid(std::int32_t type) const {
    if (type>0 && static_cast<std::size_t>(type)<types_.size()) return true;
    if (edition_==OriginalEdition::Patch1078) {
        if (!assertion_) throw std::logic_error("Totalmade 잘못된 타입의 원본 진단 미연결");
        assertion_(TotalMadeAssertion::InvalidType);
    }
    return false;
}
// 허용 상태는 매 조회마다 현재 금지 표에서 읽는다.
bool RawTotalMade::Allowed(std::int32_t type) const { return Valid(type) && restrictions_[static_cast<std::size_t>(type)]==0; }
// 원본 세 배열의 물리 마지막 칸은 초기화 범위 밖이다.
void RawTotalMade::ResetCounts() const {
    std::fill_n(counts_.localCounts.begin(),kInitializedEntries,0U);
    std::fill_n(counts_.localSecondaryCounts.begin(),kInitializedEntries,0U);
    std::fill_n(counts_.globalCounts.begin(),kInitializedEntries,0U);
}
// 첫 로컬 표의 현재 비트를 그대로 노출한다.
std::uint32_t RawTotalMade::Local(std::int32_t type) const { return Valid(type) ? counts_.localCounts[static_cast<std::size_t>(type)] : 0U; }
// 두 번째 로컬 표의 현재 비트를 그대로 노출한다.
std::uint32_t RawTotalMade::Built(std::int32_t type) const { return Valid(type) ? counts_.localSecondaryCounts[static_cast<std::size_t>(type)] : 0U; }
// 전역 표의 현재 비트를 그대로 노출한다.
std::uint32_t RawTotalMade::Global(std::int32_t type) const { return Valid(type) ? counts_.globalCounts[static_cast<std::size_t>(type)] : 0U; }
// 전역 getter 호출로 생기는 중간 복사 없이 같은 TYPE/장부의 현재 값을 읽는다.
std::int32_t RawTotalMade::BuiltForGenus(std::uint32_t genus) const {
    if (!genus) {
        if (edition_==OriginalEdition::Patch1078) {
            if (!assertion_) throw std::logic_error("Totalmade 빈 genus의 원본 진단 미연결");
            assertion_(TotalMadeAssertion::ZeroGenus);
        }
        return 0;
    }
    std::uint32_t total=0; // 원본 signed 결과의 DWORD 감김을 계산할 누산기다.
    // 원본은 genus의 모든 비트 일치가 아니라 한 비트라도 겹치는 타입을 더한다.
    for (std::size_t type=kFirstType;type<types_.size();++type)
        if (types_[type].flags2&genus) total+=counts_.localSecondaryCounts[type];
    return std::bit_cast<std::int32_t>(total);
}
}
