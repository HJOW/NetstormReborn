// Draw는 Open에 저장한 seed를 소비하지 않고 별도 시각 난수 한 번을 만든다.
#include "o/RawDeckDraw.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 실제 Deck의 다섯 DWORD 위치다.
constexpr std::size_t kCount=0x14c,kChance=0x150,kRemaining=0x154,kSeed=0x158,kOpen=0x15c;
// 원본 DWORD를 정렬과 무관하게 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value{}; // 현재 DWORD 조립값이다.
    // 정확한 네 바이트만 읽는다.
    for (std::size_t i=0;i<4;++i) value|=std::uint32_t(raw[offset+i])<<(8*i);return value;
}
// 원본 DWORD만 쓰고 인접 항목과 미초기화 꼬리는 보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    // 낮은 바이트부터 쓴다.
    for (std::size_t i=0;i<4;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 실제352바이트 이외 저장소는 받아들이지 않는다.
void Check(std::span<const std::uint8_t> raw) { if (raw.size()!=kProductionDeckBytes) throw std::invalid_argument("Deck의 실제352바이트 누락"); }
// signed 음수 개수는 빈 순회이며 양수 물리 초과만 거부한다.
std::int32_t Count(std::span<const std::uint8_t> raw) { const auto count=std::bit_cast<std::int32_t>(Read(raw,kCount));if (count>66) throw std::out_of_range("Deck의66항목 물리 범위");return count; }
// 선택된 외부 효과를 빈 성공으로 바꾸지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("Deck 추첨의 실제 효과 미연결");return hook; }
// 현재 signed power가 양수인 항목의 현재 BYTE chance만 합산한다.
std::uint32_t Sum(std::span<const std::uint8_t> raw) {
    std::uint32_t sum{};const auto count=Count(raw); // 각 원본 순회의 현재 개수다.
    // power의0/음수는 제외한다.
    for (std::int32_t i=0;i<count;++i) { const auto offset=static_cast<std::size_t>(i)*5;if (std::bit_cast<std::int8_t>(raw[offset+2])>0) sum+=raw[offset+1]; }return sum;
}
}
// 같은 실제 물리 Deck을 빌리며 어떤 필드도 초기화하지 않는다.
RawDeckDraw::RawDeckDraw(std::span<std::uint8_t> raw,GameRandom& global,DeckDrawHooks hooks):raw_(raw),global_(global),hooks_(std::move(hooks)) { Check(raw); }
// 원본 생성자의 쓰기 순서와 폭을 유지한다.
void RawDeckDraw::Initialize(std::span<std::uint8_t> raw) { Check(raw);Put(raw,kCount,0);Put(raw,kSeed,0);Put(raw,kChance,0);Put(raw,kRemaining,0);Put(raw,kOpen,0); }
// 현재 Player용 추가와 스택용 추가가 같은 본문을 쓴다.
void RawDeckDraw::Append(std::span<std::uint8_t> raw,std::uint32_t type,std::int32_t chance,const std::function<void(ProductionDeckAssertion)>& assertion) {
    Check(raw);const auto count=Count(raw); // 최초 개수는 중복 진단 전에 보관한다.
    // 원본은 중복 진단이 돌아와도 계속 순회하고 추가한다.
    for (std::int32_t i=0;i<count;++i) if (std::uint32_t(raw[static_cast<std::size_t>(i)*5])==type) Need(assertion)(ProductionDeckAssertion::DuplicateType);
    if (count<0 || count>=66) { Need(assertion)(ProductionDeckAssertion::Capacity);throw std::out_of_range("Deck 추가의 물리 범위"); }
    const auto offset=static_cast<std::size_t>(count)*5;raw[offset+2]=1;raw[offset]=static_cast<std::uint8_t>(type);raw[offset+1]=static_cast<std::uint8_t>(chance);raw[offset+3]=255;raw[offset+4]=0;
    if (type>255) Need(assertion)(ProductionDeckAssertion::TypeTruncated);if (std::uint32_t(raw[offset+1])!=std::bit_cast<std::uint32_t>(chance)) Need(assertion)(ProductionDeckAssertion::ChanceTruncated);
    Put(raw,kChance,Read(raw,kChance)+std::bit_cast<std::uint32_t>(chance));Put(raw,kCount,std::uint32_t(count)+1);
}
// Open의0입력은 두번째 전역 난수 소비를 유발할 수 있다.
void RawDeckDraw::Open(std::uint32_t seed) const { if (!seed) seed=global_.Next(10000);Put(raw_,kSeed,seed);Put(raw_,kOpen,Read(raw_,kOpen)|1U); }
// 다른 open 플래그와 저장 seed는 지우지 않는다.
void RawDeckDraw::Close() const { Put(raw_,kOpen,Read(raw_,kOpen)&~1U); }
// x87 ftol의무효64비트 결과는8000000000000000이라 낮은DWORD가0이다.
std::uint32_t RawDeckDraw::WallSeed(double seconds) {
    const double scaled=seconds*1013.0;std::uint32_t low{}; // 기본53비트 제어어의 곱과 ftol 낮은32비트다.
    if (std::isfinite(scaled) && scaled>=-9223372036854775808.0 && scaled<9223372036854775808.0) low=static_cast<std::uint32_t>(static_cast<std::int64_t>(scaled));
    const auto shifted=(low>>4)|((low&0x80000000U) ? 0xf0000000U : 0U);return low*33U+shifted; // unsigned 감김으로 실제 sar/imul/add를 보존한다.
}
// 합계 진단 뒤 현재 표를 다시 합산하고 시각 효과 뒤 현재 항목을 선택한다.
std::uint32_t RawDeckDraw::Draw(std::int32_t* power) const {
    if (!(raw_[kOpen]&1U)) Need(hooks_.assertion)(0xe5);
    if (Sum(raw_)!=Read(raw_,kChance)) {
        const auto stored=Read(raw_,kChance),sum=Sum(raw_);Need(hooks_.mismatch)(stored,sum);Put(raw_,kChance,Sum(raw_));
    }
    const auto chance=Read(raw_,kChance);GameRandom local(WallSeed(Need(hooks_.wallSeconds)())); // 원본은 시각 공급 전에 chance를 보관한다.
    std::uint32_t remaining=std::bit_cast<std::int32_t>(chance)>0 ? local.Next(chance) : 0;std::int32_t index{};const auto limit=Count(raw_);
    // 현재 개수/항목은 시각 효과가 끝난 뒤 읽으며 선택 성공 뒤 남은 필드는 읽지 않는다.
    while (index<limit) { const auto offset=static_cast<std::size_t>(index)*5;if (std::bit_cast<std::int8_t>(raw_[offset+2])>0) remaining-=raw_[offset+1];if (std::bit_cast<std::int32_t>(remaining)<0) break;++index; }
    if (index<limit) {
        const auto offset=static_cast<std::size_t>(index)*5;const auto type=std::uint32_t(raw_[offset]);++raw_[offset+4];Put(raw_,kRemaining,Read(raw_,kRemaining)+1U);
        if (power) *power=Need(hooks_.isProduct)(type) ? 0 : -1;return type;
    }
    if (power) *power=0;return 0;
}
}
