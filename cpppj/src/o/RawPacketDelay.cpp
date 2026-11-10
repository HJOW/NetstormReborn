// 원문 지연 함수의 signed 분포·난수 소비·ring 확장·현재 주소 복원을 같은 연결에서 수행한다.
#include "o/RawPacketDelay.h"
#include <array>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 호스트 할당 주소 자체를 원본32비트 포인터로 가장하지 않는다.
namespace { // 보호 PE의 분포와 밀리초 상수만 이 파일에 한정한다.
constexpr std::array<std::int8_t,2> kDefault{1,-1}; //00543600의 기본01ff다.
constexpr std::array<std::int8_t,10> kLag1{0,4,12,6,2,1,1,1,1,-1}; //005435e0의 실제 signed BYTE다.
constexpr std::array<std::int8_t,6> kLag2{0,0,0,0,1,-1}; //005435ec의 실제 signed BYTE다.
constexpr std::array<std::int8_t,9> kLag3{0,0,0,0,0,0,0,1,-1}; //005435f4의 실제 signed BYTE다.
constexpr double kSecondsPerMillisecond=0.001; //PE0050a668의 실제double이다.
constexpr std::size_t kInitialCapacity=100,kMaximumWeights=50; //4def40의 최초 ring 크기와 signed 분포 상한이다.
}
//새 Client의 최초 기본 선택만 구성하며 난수나 시계는 읽지 않는다.
PacketDelayGlobals::PacketDelayGlobals() { ResetDefault(); }
//원문의 최초 지도 모드가 요청한 기본01ff 주소로 돌아간다.
void PacketDelayGlobals::ResetDefault() { distribution=kDefault; }
//누락/다른 lag 값은 기존 선택을 그대로 유지한다.
void PacketDelayGlobals::SelectLag(std::int32_t lag) { if (lag==1) distribution=kLag1;else if (lag==2) distribution=kLag2;else if (lag==3) distribution=kLag3; }
struct RawPacketDelayQueue::Entry { //원문 node의 현재 sender/F/packet/deadline/address/connection을 보유한다.
    std::uint32_t sender{}; //enqueue 때 저장한 원문 첫DWORD 인자다.
    std::shared_ptr<const CanonicalPacketReceiver> receiver; //현재 표가 교체돼도 이미 선택한 동일F를 사용한다.
    std::vector<std::uint8_t> packet; //observer1 뒤 선언WORD 길이만 복사한 실제 버퍼다.
    double deadline{}; //원문 wall+delay*0.001이며 FIFO 안에서 재정렬하지 않는다.
    std::array<std::uint8_t,20> address{}; //enqueue 때 같은 연결의 실제 주소20BYTE다.
    PacketConnection* connection{}; //ring을 소유한 같은 실제 노드이며 별도 연결 사본이 아니다.
};
// 분포는 ff 또는50BYTE까지만 읽고 현재 Client 장치를 그대로 보유한다.
RawPacketDelayQueue::RawPacketDelayQueue(std::shared_ptr<PacketDelayGlobals> globals):globals_(std::move(globals)),distribution_(globals_ ? globals_->distribution:std::span<const std::int8_t>{}),ring_(kInitialCapacity) {
    if (!globals_ || !globals_->wall || !globals_->random || distribution_.empty()) throw std::invalid_argument("지연큐의 실제 현재 분포/벽시각/CRT 미연결");
    for (std::size_t index=0;index<distribution_.size() && index<kMaximumWeights;++index) { //원문의 signed BYTE 합계를 ff까지 보존한다.
        if (distribution_[index]==-1) break;weight_+=distribution_[index];
    }
}
//미배송 packet/receiver만 실제 해제하며 종료 callback을 새로 실행하지 않는다.
RawPacketDelayQueue::~RawPacketDelayQueue()=default;
// 감긴 접두만 기존 capacity 뒤로 옮기며 read 위치를 바꾸지 않는다.
void RawPacketDelayQueue::Grow() {
    const auto capacity=ring_.size();if (capacity>std::numeric_limits<std::size_t>::max()/2) throw std::length_error("지연 ring 두 배 크기 초과");ring_.resize(capacity*2);
    if (write_<read_) { for (std::size_t index=0;index<write_;++index) ring_[capacity+index]=std::move(ring_[index]);write_+=capacity; } //원문 realloc 뒤 감긴0..write 접두를 새 뒤쪽으로 이동한다.
}
// 같은 집합의 고정 지연도 bucket 난수는 먼저 소비하며 receiver NULL은 앞에서 반환한다.
std::int32_t RawPacketDelayQueue::Enqueue(PacketConnection& connection,std::shared_ptr<const CanonicalPacketReceiver> receiver,const RawPacketDispatch& dispatch,std::span<const std::uint8_t> packet,std::int32_t delay) {
    if (!receiver) return 0;const auto sender=connection.sender;if (!weight_) throw std::domain_error("원문 지연 분포 합계0 나눗셈");
    const auto draw=static_cast<std::int32_t>(globals_->random())%weight_;std::size_t bucket{};std::int32_t cumulative=distribution_[0];
    while (cumulative<draw) { //원문은 동일 경계에서 다음 bucket으로 넘어가지 않는다.
        ++bucket;if (bucket>=distribution_.size() || bucket>=kMaximumWeights || distribution_[bucket]==-1) throw std::out_of_range("원문 지연 분포 bucket 물리 범위");cumulative+=distribution_[bucket];
    }
    const auto available=write_<read_ ? read_-write_:ring_.size()-write_+read_;if (available<2) Grow();
    if (delay==-1) delay=static_cast<std::int32_t>(globals_->random()%100U)+static_cast<std::int32_t>(bucket)*100;
    auto entry=std::make_unique<Entry>();entry->sender=sender;entry->receiver=std::move(receiver);entry->address=connection.address;entry->connection=&connection; //원문 생성자 값 인수로 넘긴 주소는 observer1보다 먼저 저장한다.
    if (packet.size()<4) throw std::out_of_range("지연 packet 헤더 물리 부족");const auto observer=dispatch.Registration(packet[1]);observer.NotifyObserver({const_cast<std::uint8_t*>(packet.data()),packet.size()},1); //현재 실제 mutable 프레임을 observer1에 전달한 뒤 선언 길이를 읽는다.
    const auto length=std::size_t(packet[2])|(std::size_t(packet[3])<<8);if (length>packet.size()) throw std::out_of_range("지연 packet 선언 길이 물리 초과");
    entry->packet.assign(packet.begin(),packet.begin()+static_cast<std::ptrdiff_t>(length));entry->deadline=static_cast<double>(delay)*kSecondsPerMillisecond+globals_->wall();
    ring_[write_]=std::move(entry);write_=(write_+1)%ring_.size();return delay;
}
// 예외에서도 실제 주소/깊이는 복구하고 실패한 선두는 보존하여 정상 배송으로 숨기지 않는다.
std::int32_t RawPacketDelayQueue::Pump(PacketConnection& connection) {
    std::int32_t delivered{};
    while (read_!=write_) { //선두가 준비되지 않으면 뒤 deadline이 더 작아도 앞지르지 않는다.
        const auto slot=read_;const auto entry=ring_[slot].get();if (!entry || entry->connection!=&connection) throw std::logic_error("지연 ring의 실제 현재 연결/선두 불일치");if (!(entry->deadline<globals_->wall())) return delivered;
        const auto previous=connection.address;++connection.depth;connection.address=entry->address;
        try { entry->receiver->Invoke(entry->sender,entry->connection,entry->packet); }catch (...) { connection.address=previous;--connection.depth;throw; }
        std::vector<std::uint8_t>{}.swap(entry->packet);connection.address=previous;--connection.depth;ring_[slot].reset();read_=(read_+1)%ring_.size();++delivered;if (connection.flags&0x40U) return -1; //packet 메모리는 주소/깊이 복구보다 먼저 반납한다. callback의 Grow에도 실제 Entry/slot은 유지한다.
    }
    return delivered;
}
// 이미 설치된 큐의 분포는 새 선택/Session 교체 때 다시 초기화하지 않는다.
void PreparePacketDelay(PacketConnection& connection,const std::shared_ptr<PacketDelayGlobals>& globals) {
    if (!connection.delayQueue && globals && !globals->distribution.empty()) connection.delayQueue=std::make_shared<RawPacketDelayQueue>(globals);if (connection.delayQueue) connection.delayed=true;
}
}
