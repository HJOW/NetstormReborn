// 같은 지갑의 원본 DWORD 감김·signed 비교·QWORD 기한·최종 float 저장을 유지한다.
#include "o/RawAiEconomy.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::uint32_t kBudget=2200,kFloor=1190; //004138c0의0x898 예산과0x4a6 데모 하한이다.
constexpr double kPeriod=1.0; //보호 PE00501580의 QWORD 지원 간격이다.
// 범위 안의 비정렬 little endian DWORD를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI 경제 DWORD 읽기 범위");std::uint32_t value{};
    // 네 BYTE를 낮은 자리부터 합친다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);return value;
}
// 지정 DWORD만 쓰고 나머지 본체를 유지한다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    if (offset>raw.size() || raw.size()-offset<4) throw std::out_of_range("AI 경제 DWORD 쓰기 범위");
    // 네 BYTE의 원본 little endian 표현을 쓴다.
    for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 원본 signed 비교를 구현 의존 변환 없이 적용한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 선택되지 않은 하위는 요구하지 않고 선택된 누락은 명시 오류로 남긴다.
template<class Function> const Function& Need(const Function& function,const char* message) { if (!function) throw std::logic_error(message);return function; }
}
// 생성 시 원본 지갑/SP/타이머에 아무 효과도 주지 않는다.
RawAiEconomy::RawAiEconomy(RawAiBrainMemory& memory,ScrambledSpStore& sp,const double& now,AiEconomyHooks hooks):memory_(memory),sp_(sp),now_(now),hooks_(std::move(hooks)) {}
// 가짜 핸들이나 작은 본체를 다른 저장소로 대신하지 않는다.
std::span<std::uint8_t> RawAiEconomy::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 경제 실제0xb08 본체 누락");return raw; }
// 현재 본체의 지정 필드를 다시 읽는다.
std::uint32_t RawAiEconomy::Field(std::uint32_t token,std::size_t offset) const { return Read(Body(token),offset); }
// 현재 본체의 지정 필드만 쓴다.
void RawAiEconomy::Field(std::uint32_t token,std::size_t offset,std::uint32_t value) const { Put(Body(token),offset,value); }
// getter의 효과가 바꾼 현재 지갑을 원본 저장 포인터에서 다시 읽는다.
void RawAiEconomy::DemoFloor(std::uint32_t token) const {
    if (Need(hooks_.demo,"AI 경제 현재 데모 조회 미연결")()) { const auto wallet=Field(token,0x2d8);Field(token,0x2d8,Signed(wallet)>Signed(kFloor) ? wallet : kFloor); }
}
// 포획 플래그만 먼저 판정하고 반환 뒤 비용/지갑을 현재 다시 읽는다.
void RawAiEconomy::CaptureBudget(std::uint32_t token,std::uint32_t owner) const {
    if (Need(hooks_.captured,"AI 경제 현재 사제 포획 조회 미연결")(owner)) { const auto target=Field(token,0x290)+kBudget;if (Signed(Field(token,0x2d8))<Signed(target)) { Field(token,0x2d8,target);DemoFloor(token); } }
}
// 원본은 기한이 안 되어도 양쪽 포획 판정과 최종 분산SP Set을 항상 수행한다.
void RawAiEconomy::Reconcile(std::uint32_t token) const {
    const auto due=std::bit_cast<double>((std::uint64_t(Field(token,0x5dc))<<32)|Field(token,0x5d8));
    if (now_>=due) {
        const auto next=std::bit_cast<std::uint64_t>(now_+kPeriod);Field(token,0x5d8,static_cast<std::uint32_t>(next));Field(token,0x5dc,static_cast<std::uint32_t>(next>>32));
        if (Signed(Field(token,0x2d8))<Signed(Field(token,0x290)+kBudget)) { Field(token,0x2d8,Field(token,0x24c)+Field(token,0x2d8));DemoFloor(token); }
    }
    CaptureBudget(token,Field(token,0x264));CaptureBudget(token,Field(token,0x260));
    const auto wallet=static_cast<float>(Signed(Field(token,0x2d8)));sp_.Set(Field(token,0x260),std::bit_cast<std::uint32_t>(wallet));
}
}
