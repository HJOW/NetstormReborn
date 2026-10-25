// 실제 초기화는 원문 호출자가 수행하며 생성자/getter/Session 반납에서 숨은 재시드를 하지 않는다.
#include "client/ClientSpGlobals.h"
#include "client/CurrentAllocationBytes.h"
#include <ctime>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 같은 장기 저장소에만 Native SP 본체를 적용한다.
namespace { // 필수 소유자 누락을 멤버 참조 구성 전에 거부한다.
std::shared_ptr<ClientRandomGlobals> RequireRandom(std::shared_ptr<ClientRandomGlobals> random) { if (!random) throw std::invalid_argument("SP 실제 Client 난수 소유자 누락");return random; } // 별도 난수나 상수 시드를 생성하지 않는다.
}
std::int32_t ClientCurrentUnixSeconds() { const auto now=std::time(nullptr);if (now<0 || now>std::numeric_limits<std::int32_t>::max()) throw std::out_of_range("실제 time32 지원 날짜 범위 초과");return static_cast<std::int32_t>(now); } // 호출 당시 실제 초만 제공한다.
ClientSpGlobals::ClientSpGlobals(std::shared_ptr<ClientRandomGlobals> random,std::function<std::int32_t()> unixSeconds,std::function<void(std::span<std::uint8_t>)> allocation)
    :random_(RequireRandom(std::move(random))),seed_(random_->Game(),random_->CrtState(),random_->GameMode(),std::move(unixSeconds)),
    store_(random_->Game(),[this] { return seed_.Reseed(); },allocation ? std::move(allocation) : CurrentAllocationBytes) { store_.BindTrackingOffset(tracking_); } // 원문 초기화보다 먼저 같은 주소만 연결한다.
o::ScrambledSpStore& ClientSpGlobals::Store() { return store_; } // 실제 초기화/쓰기 소비자의 같은 저장소다.
const o::ScrambledSpStore& ClientSpGlobals::Store() const { return store_; } // 읽기 조회에서도 lazy 초기화/재시드를 하지 않는다.
const std::shared_ptr<ClientRandomGlobals>& ClientSpGlobals::RandomGlobals() const { return random_; } // 난수 상태 복사 대신 소유자 정체성을 반환한다.
}
