// 원본 건설 선택의 단락 순서·signed 절삭/곱 감김과 중복 선호 수량을 보존한다.
#include "o/RawAiBuildChoice.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::int32_t kLowFunds=1000; //00411ad0의signed지갑/SP절삭값경계다.
constexpr float kUnknownSp=0.0F; //PE00500fc4의로컬소유자미확정SP대체값이다.
// 비정렬 원문DWORD를 물리 범위 안에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) { if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 건설 선택 DWORD 범위");std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[at+index])<<(index*8);return value; } //반복문은지정네BYTE만합친다.
// 원본DWORD의signed비교해석이다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 원본ftol의signed64절삭하위DWORD이며무효FISTP는하위0이다.
std::uint32_t Ftol(double value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;return static_cast<std::uint32_t>(static_cast<std::int64_t>(std::trunc(value))); }
// 선택된하위누락을가짜구매허용/기본TYPE로대신하지않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("AI 건설 선택 실제 하위 미연결");return function; }
}
// 생성은 비용/사제 상태/자금을 읽지 않는다.
RawAiBuildChoice::RawAiBuildChoice(RawAiBrainMemory& memory,std::span<const RiftTypeRecord> types,AiBuildChoiceGlobals globals,AiBuildChoiceHooks hooks):memory_(memory),types_(types),globals_(globals),hooks_(std::move(hooks)) { if (types.size()!=188 || globals.workshops.size()!=4) throw std::invalid_argument("AI 건설 선택 실제188TYPE/워크샾4 누락"); }
// 해제/가짜본체를거부한다.
std::span<std::uint8_t> RawAiBuildChoice::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 건설 선택 실제0xb08본체 누락");return raw; }
// 하위효과뒤현재필드를다시읽는다.
std::uint32_t RawAiBuildChoice::Field(std::uint32_t token,std::size_t at) const { return Read(Body(token),at); }
// 비용조회에종류/소유자필터를추가하지않는다.
const RiftTypeRecord& RawAiBuildChoice::Type(std::uint32_t type) const { if (type>=types_.size()) throw std::out_of_range("AI 건설 선택 현재TYPE 범위");return types_[type]; }
// 원본imul의하위DWORD곱과signed지갑비교다.곱셈overflow를signed UB로만들지않는다.
bool RawAiBuildChoice::ReserveForCollectors(std::uint32_t token) const { const auto cost=Ftol(Type(globals_.collectorType).cost);const auto reserve=cost*Field(token,0x234);return Signed(reserve)<=Signed(Field(token,0x2d8)); }
// 원격은AI지갑,로컬확정은분산SP,미확정은0을사용하며리소스를변경하지않는다.
bool RawAiBuildChoice::EnemyLowFunds(std::uint32_t token) const { const auto enemy=Field(token,0x264),local=globals_.localOwner;if (enemy!=local) return Need(hooks_.wallet)(enemy)<kLowFunds;const auto value=globals_.playerKnown ? Need(hooks_.storedSp)(local) : kUnknownSp;return Signed(Ftol(value))<kLowFunds; }
// 범위진단이반환하면원본switch의default0으로계속한다.
std::uint32_t RawAiBuildChoice::WorkshopType(std::int32_t theme) const { if (theme<0 || theme>5) Need(hooks_.assertion)("theIndex>=0 && theIndex<=(MAX_THEMES)",0x1a);return theme>=0 && theme<4 ? globals_.workshops[static_cast<std::size_t>(theme)] : 0U; }
// IDIV5의signed remainder다.원본점프표의5번칸은정상나머지에서결코선택되지않는다.
std::uint32_t RawAiBuildChoice::TempleType(std::int32_t theme) const { const auto index=theme%5;if (index>=0 && index<3) return Need(globals_.temple)(static_cast<std::uint32_t>(index));Need(hooks_.assertion)("0",0x47);return 0; }
// 모든비용은현재TYPE의원본cost→ftol이며각관문뒤현재본체를재조회한다.
std::uint32_t RawAiBuildChoice::Select(std::uint32_t token) const {
    if ((Field(token,0x248)&0x200U) && !Field(token,0x2a4) && Need(hooks_.captured)(Field(token,0x264)) && !Need(hooks_.dead)(Field(token,0x264)) && Need(hooks_.canAfford)(token,globals_.daisType) && (Field(token,0x294) || EnemyLowFunds(token))) return globals_.daisType;
    if (!Field(token,0x294) && Field(token,0x390)!=3) { const auto temple=TempleType(Signed(Field(token,0x390)));const auto cost=Ftol(Type(temple).cost);return Signed(Field(token,0x2d8))>=Signed(cost) ? temple : 0U; }
    const auto candidate=Field(token,0x394);if (!candidate) return 0;if (Signed(Field(token,0x2d8))<Signed(Ftol(Type(candidate).cost)) || !ReserveForCollectors(token)) return 0;
    std::array<std::uint32_t,5> counts{};
    // 원본은선호count가0이어도다섯theme수량을먼저복사하며theme4는TYPE0의수량이다.
    for (std::uint32_t index=0;index<counts.size();++index) { const auto type=WorkshopType(static_cast<std::int32_t>(index));counts[index]=Field(token,0x6d4+static_cast<std::size_t>(type)*4); }
    const auto count=Signed(Field(token,0x3b0));
    // 선호중복마다같은지역수량을감소시켜첫음수의현재선호TYPE를반환한다.실제수량표는쓰지않는다.
    for (std::int32_t index=0;index<count;++index) { const auto theme=Field(token,0x398+static_cast<std::size_t>(index)*4);if (theme>=counts.size()) throw std::out_of_range("AI 건설 선택 원본임시수량 인덱스 범위");--counts[theme];if (Signed(counts[theme])<0) return WorkshopType(Signed(Field(token,0x398+static_cast<std::size_t>(index)*4))); }
    return 0;
}
}
