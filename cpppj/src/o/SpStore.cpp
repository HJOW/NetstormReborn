// 패치판 SP 저장소와 전역 난수의 비트 단위 복원. 원본 순서대로 난수를 소비한다.
#include "o/SpStore.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 난수의 0 상태 대체값과 곱/덧셈 상수다.
constexpr std::uint32_t kSeedReplacement=0x0bad0bad,kMultiplier=0x10003,kIncrement=3;
// 초기화가 offset을 고를 때의 상한(push 0x64)과 순열 길이/잡음의 상한(push 0x20)이다.
constexpr std::uint32_t kOffsetLimit=100,kBitLimit=32;
// 한 칸마다 섞는 잡음 비트의 수(edi=5)다.
constexpr int kNoisePerBit=5;
}
GameRandom::GameRandom(std::uint32_t state):state_(state) {}
// 004558c0: 곱셈/덧셈은 32비트로 감기고 상위 16비트만 나머지 연산에 쓴다.
std::uint32_t GameRandom::Next(std::uint32_t limit) {
    if (limit==0) throw std::invalid_argument("게임 난수의 limit은 0일 수 없습니다");
    if (state_==0) state_=kSeedReplacement;
    state_=state_*kMultiplier+kIncrement;
    return (state_>>16)%limit;
}
std::uint32_t GameRandom::State() const { return state_; }
void GameRandom::SetState(std::uint32_t state) { state_=state; }

ScrambledSpStore::ScrambledSpStore(GameRandom& rng,std::function<std::uint32_t()> reseed,std::uint32_t fill)
    :rng_(rng),reseed_(std::move(reseed)),fill_(fill) {}
bool ScrambledSpStore::Initialized() const { return initialized_; }
// 소유자 번호가 9 이상이면 원본은 힙 밖을 읽거나 쓴다. 상태를 바꾸기 전에 거부한다.
void ScrambledSpStore::CheckOwner(std::uint32_t owner) {
    if (owner>=kOwners) throw std::out_of_range("SP 저장소 소유자 번호는 0~8이어야 합니다");
}
// 0040eba0: 재시드 → offset → 32번의 순열 교환 → 32개 배열 확보. 교환 결과는 곧 항등 순열로 덮여 쓰이지 않는다.
void ScrambledSpStore::Initialize() {
    if (!reseed_) throw std::logic_error("SP 저장소 재시드 콜백 누락");
    rng_.SetState(reseed_());
    offset_=rng_.Next(kOffsetLimit);
    // 원본은 순열의 각 위치를 난수 위치와 교환하지만 곧바로 항등으로 되돌린다. 난수 소비만 남는다.
    for (std::uint32_t i=0;i<kBitLimit;++i) static_cast<void>(rng_.Next(kBitLimit));
    pools_.fill(fill_);
    initialized_=true;
}
// 0040ec50: 배열 k의 (offset+k)&31번째 비트가 값의 k번째 비트다.
std::uint32_t ScrambledSpStore::Get(std::uint32_t owner) {
    CheckOwner(owner);
    if (!initialized_) Initialize();
    std::uint32_t bits=0;
    // 값의 32비트를 낮은 비트부터 하나씩 모은다.
    for (std::uint32_t k=0;k<kBits;++k) {
        const auto mask=std::uint32_t{1}<<((offset_+k)&31);
        if (pools_[k*kOwners+owner]&mask) bits|=std::uint32_t{1}<<k;
    }
    return bits;
}
// 0040edb0: 실제 비트를 쓰고 같은 DWORD의 다른 위치에 난수 잡음 5개를 OR한다. 실제 비트 위치는 건드리지 않는다.
void ScrambledSpStore::Set(std::uint32_t owner,std::uint32_t bits) {
    CheckOwner(owner);
    if (!initialized_) Initialize();
    // 값의 32비트를 낮은 비트부터 하나씩 기록한다.
    for (std::uint32_t k=0;k<kBits;++k) {
        auto& word=pools_[k*kOwners+owner];
        const auto mask=std::uint32_t{1}<<((offset_+k)&31);
        if ((bits>>k)&1) word|=mask; else word&=~mask;
        // 잡음은 실제 비트를 제외한 위치에만 들어간다.
        for (int n=0;n<kNoisePerBit;++n) word|=(std::uint32_t{1}<<rng_.Next(kBitLimit))&~mask;
    }
}
std::uint32_t ScrambledSpStore::Offset() const { return offset_; }
const std::array<std::uint32_t,ScrambledSpStore::kBits*ScrambledSpStore::kOwners>& ScrambledSpStore::Pools() const { return pools_; }
}
