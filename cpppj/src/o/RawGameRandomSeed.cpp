// CRT와 게임 난수의 별도 상태 및 원본 DWORD 곱셈 감김/모드 복원을 보존한다.
#include "o/RawGameRandomSeed.h"
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 원본 고정 시드와 실제 MSVC CRT의 상태 대입 상수다.
constexpr std::uint32_t kFixedSeed=0x38d535,kCrtMultiplier=0x343fd,kCrtIncrement=0x269ec3;
// 원본 초기화는100회를 버린 다음 추가3회를 게임 상태 생성에 사용한다.
constexpr std::uint32_t kWarmup=100;
}
// 시각 입력 누락은 실제 세션 초기화 전에 거부한다.
RawGameRandomSeed::RawGameRandomSeed(GameRandom& game,std::uint32_t& crt,std::uint32_t& mode,std::function<std::int32_t()> unixSeconds)
    :game_(game),crt_(crt),mode_(mode),unixSeconds_(std::move(unixSeconds)) {
    if (!unixSeconds_) throw std::invalid_argument("게임 난수의 Unix 초 공급자 누락");
}
// unsigned 연산으로 실제32비트 상태 감김을 유지한 뒤 높은15비트를 반환한다.
std::uint32_t RawGameRandomSeed::NextCrt(std::uint32_t& state) { state=state*kCrtMultiplier+kCrtIncrement;return (state>>16)&0x7fffU; }
// 시각의−1 및 signed32 비트도 srand의 unsigned 상태로 그대로 전달한다.
std::uint32_t RawGameRandomSeed::Initialize() const {
    crt_=mode_==0 ? kFixedSeed : static_cast<std::uint32_t>(unixSeconds_());
    // 원본과 같은100회 소비를 수행하며 반환값은 사용하지 않는다.
    for (std::uint32_t i=0;i<kWarmup;++i) NextCrt(crt_);
    const auto first=NextCrt(crt_),second=NextCrt(crt_),third=NextCrt(crt_);
    const auto seed=first*second*third;game_.SetState(seed);return seed;
}
// 정상 원본 복원을 보존하며 공급자 예외 때도 임시mode를 외부 상태에 남기지 않는다.
std::uint32_t RawGameRandomSeed::Reseed() const {
    const auto previous=mode_;mode_=1;
    try { const auto seed=Initialize();mode_=previous;return seed; }
    catch (...) { mode_=previous;throw; }
}
}
