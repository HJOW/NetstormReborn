// 원본 반복 조건/소유자 재조회/기술 부모 캐시·회전 예산과 unordered 쿨다운을 보존한다.
#include "o/RawAiDropSelection.h"
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kBlocker=0x4000000U; //블로커우선선택과현재생산조각을판단하는원본flags2다.
constexpr std::size_t kFirstChoice=0x408,kChoiceStride=8; //같은30후보XY의본체시작/행크기다.
// 비정렬DWORD를실제블록범위안에서읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) { if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 배치 TYPE 선택 DWORD 범위");std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[at+index])<<(index*8);return value; } //반복문은지정네BYTE만조합한다.
// DWORD감김뒤signed해석이다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 원본signed나눗셈은0쪽절삭이며나머지도부호를보존한다.
std::int32_t Remainder(std::int32_t value,std::int32_t count) { if (!count || (count==-1 && value==std::numeric_limits<std::int32_t>::min())) throw std::overflow_error("AI 배치 기술 순환 signed 나눗셈 범위");return value%count; }
// 선택된실제하위만그자리에서요구한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("AI 배치 TYPE 선택 실제 하위 미연결");return function; }
}
// 생성중게임판단/난수/로그를실행하지않는다.
RawAiDropSelection::RawAiDropSelection(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const std::uint32_t> directionalTypes,GameRandom& random,const double& now,AiDropSelectionHooks hooks):memory_(memory),pool_(pool),types_(types),directionalTypes_(directionalTypes),random_(random),now_(now),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || directionalTypes.size()!=4) throw std::invalid_argument("AI 배치 TYPE 선택 같은PATCH/현재TYPE누락"); }
// 해제/작은가짜본체를거부한다.
std::span<std::uint8_t> RawAiDropSelection::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 배치 TYPE 선택 실제0xb08본체누락");return raw; }
// 현재같은본체를매원문위치에서읽는다.
std::uint32_t RawAiDropSelection::Field(std::uint32_t token,std::size_t at) const { return Read(Body(token),at); }
// 지정DWORD만쓴다.
void RawAiDropSelection::Field(std::uint32_t token,std::size_t at,std::uint32_t value) const { auto raw=Body(token);if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 배치 TYPE 선택 쓰기 범위");for (std::size_t index=0;index<4;++index) raw[at+index]=static_cast<std::uint8_t>(value>>(index*8)); } //반복문은지정네BYTE만쓴다.
// TYPE0도원본처럼조회하며범위밖만명시거부한다.
const RiftTypeRecord& RawAiDropSelection::Type(std::uint32_t type) const { if (type>=types_.size()) throw std::out_of_range("AI 배치 TYPE 선택 현재TYPE 범위");return types_[type]; }
// 현재네전역과DWORD그대로비교하며TYPE배열은아직조회하지않는다.
bool RawAiDropSelection::Directional(std::uint32_t type) const { for (auto current:directionalTypes_) if (type==current) return true;return false; } //반복문은원본순서의첫일치를반환한다.
// 진입시개수와목록포인터를값으로보존한다.
bool RawAiDropSelection::Contains(std::uint32_t token,std::uint32_t target) const { const auto count=Signed(Field(token,0x374));if (count<=0) return false;const auto list=memory_.Bytes(Field(token,0x36c));std::int32_t index{};do { if (Read(list,static_cast<std::size_t>(index)*4)==target) return true;++index; }while (index<count);return false; } //반복문은최초count안의첫일치까지만진행한다.
// b00은로그위치마다현재다시읽으며읽지않을TYPE/방향은인자로요구하지않는다.
void RawAiDropSelection::Log(std::uint32_t token,AiDropSelectionLog action,std::uint32_t type,std::int32_t direction) const { if (Field(token,0xb00)) Need(hooks_.log)(token,action,type,direction); }
// 원본 QWORD 시각을 재구성한다.
double RawAiDropSelection::Time(std::uint32_t token,std::size_t at) const { return std::bit_cast<double>((std::uint64_t(Field(token,at+4))<<32)|Field(token,at)); }
// 실제기술개수/부모는최초값이고다른본체필드는각하위효과뒤현재값이다.
void RawAiDropSelection::Select(std::uint32_t token) const {
    Field(token,0xaf8,10);Field(token,0xafc,10);Field(token,0x3ec,0);const auto technology=Need(hooks_.technology)(Field(token,0x260));const auto count=Need(hooks_.count)(technology);if (!count) { Field(token,0x3e0,0);Field(token,0x3f0,1);return; }
    bool repeat{};if (Field(token,0x248)&0x80U) { const auto draw=random_.Next(100);if (Field(token,0x3f4) && Field(token,0x3e0)) { if (Contains(token,Field(token,0x3fc))) { if (draw<80) { Log(token,AiDropSelectionLog::Hated);repeat=true; } }else if (!Field(token,0x374) && draw<30) repeat=true; } }
    bool forceBlocker{};if (Field(token,0x404) && !(Type(Field(token,0x3e0)).flags2&kBlocker)) { if (Need(hooks_.blocker)(technology).value) forceBlocker=true;else Field(token,0x404,0); }
    if (repeat && !forceBlocker) { std::int32_t index{};
        // 제거시같은index/좌표위치에서압축된현재항목을재검사한다.
        while (index<Signed(Field(token,0x570))) { if (index<0 || index>=30) throw std::out_of_range("AI 배치 재사용 후보 인덱스");const auto at=kFirstChoice+static_cast<std::size_t>(index)*kChoiceStride;const CanonPlacementQuery query{Field(token,0x3e0),0,std::bit_cast<float>(Field(token,at)),std::bit_cast<float>(Field(token,at+4)),0,Field(token,0x260)&255U,1};static_cast<void>(Type(query.type));if (!Need(hooks_.mayPlace)(query)) Need(hooks_.remove)(token,index);else ++index; }
        Log(token,AiDropSelectionLog::Retried);
    }else { auto remaining=static_cast<std::uint32_t>(count)+2U;
        // 원본은기술개수+2번까지회전/다음TYPE를시도하고블로커우선도이예산안에서만수행한다.
        do { --remaining;const auto current=Field(token,0x3e0);if (current && Directional(current) && Signed(Field(token,0x3e4))<6) Field(token,0x3e4,Field(token,0x3e4)+2U);else { const auto next=Remainder(Signed(Field(token,0x3dc)+1U),count);Field(token,0x3dc,std::bit_cast<std::uint32_t>(next));const auto selected=Need(hooks_.nth)(technology,std::bit_cast<std::uint32_t>(next));Field(token,0x3e0,selected);Field(token,0x3e4,0); }if (!remaining || !Field(token,0x404)) break;if (Type(Field(token,0x3e0)).flags2&kBlocker) break; }while (true);
        Field(token,0x404,0);Field(token,0x570,0);
    }
    const auto choices=Field(token,0x570);if (!choices) Field(token,0x3e8,0);Field(token,0x3f0,0);Field(token,0x3f4,0);Field(token,0x3fc,0);Field(token,0x400,Signed(choices)<=0 ? 1U : 0U);const auto flags=Type(Field(token,0x3e0)).flags2;
    if (flags&0x30000U) { Field(token,0x3e0,0);Field(token,0x3f0,1); }if ((flags&0x800U) && !(now_>=Time(token,0x5b8))) { Field(token,0x3e0,0);Field(token,0x3f0,1); }
    if (Field(token,0x3e0)) { const auto type=Field(token,0x3e0);const auto direction=Signed(Field(token,0x3e4));static_cast<void>(Type(type));Log(token,AiDropSelectionLog::Scanning,type,direction); }
}
// 같은실제풀연결확인용이다.
const SidPool& RawAiDropSelection::Pool() const { return pool_; }
}
