// 디컴파일에서 생략된 fadd [00507548]와 비정렬 Core DWORD의 실제 쓰기를 보존한다.
#include "o/RawForeignRitualReward.h"
#include <bit>
#include <cmath>
#include <stdexcept>

// PATCH10.78의 비로컬 획득 계산과 장부 주소 산술을 복원한다.
namespace netstorm::o {
// 원문의 선택된 하위와 단일 물리 접근만 검사하는 내부 장치다.
namespace {
constexpr double kSpReward=5000.0,kSigned64Limit=9223372036854775808.0; // 실제 float 상수와 masked_ftol의 signed64 경계다.
// 실제로 도달한 callback만 요구하며 선택되지 않은 분기는 생략한다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("비로컬 의식 보상의 실제 하위 미연결");return function;
}
// 원문 _ftol의 signed64 절삭 뒤 하위 DWORD를 취하며 비유한/범위 밖의 불확정값도 하위0이다.
std::int32_t Integer(double value) {
    if (!std::isfinite(value) || value>=kSigned64Limit || value< -kSigned64Limit) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
}
// 가산용 AddSp나 AI 지갑을 호출하지 않고 현재 getter→fadd→ftol→정책 setter를 실행한다.
void GrantForeignRitualSp(std::uint32_t owner,const std::function<float(std::uint32_t)>& balance,
    const std::function<void(std::int32_t,std::int32_t)>& set) {
    const double value=static_cast<double>(Need(balance)(owner))+kSpReward;
    Need(set)(std::bit_cast<std::int32_t>(owner),Integer(value));
}
// TYPE getter와 진단 효과 뒤 같은 장부의 현재 DWORD를 읽으며 다른 Core 필드는 보존한다.
void GrantForeignRitualType(PlayerCoreRecordsView records,std::uint32_t owner,std::uint32_t type,
    const std::function<std::int32_t(std::uint32_t)>& technologyBit,const std::function<void(std::uint32_t)>& assertion) {
    const auto bit=Need(technologyBit)(type);if (bit==-1) return;
    if (bit<0 || bit>=32) Need(assertion)(0x15a);
    const auto offset=records.origin+owner*kPlayerCoreRecordStride+10U;
    if (offset>records.bytes.size() || 4U>records.bytes.size()-offset) throw std::out_of_range("비로컬 기술 Core DWORD의 실제 물리 범위");
    std::uint32_t value{}; // 진단 뒤 현재 장부에서 읽는 비정렬 기술 DWORD다.
    for (std::uint32_t byte=0;byte<4;++byte) value|=std::uint32_t(records.bytes[offset+byte])<<(byte*8); // 이웃 ID/랭크/padding을 읽지 않는다.
    value|=1U<<(static_cast<std::uint32_t>(bit)&31U);
    for (std::uint32_t byte=0;byte<4;++byte) records.bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 실제 선택 DWORD만 쓴다.
}
}
