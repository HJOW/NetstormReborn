// 시간 표본의 비교/최초 일치/부분 효과와 같은 현재 시계 흐름을 유지한다.
#include "o/RawClockSynchronization.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 플랫폼 포인터는 토큰으로만 전달하고 wire의 비정렬 double은 비트 단위로 다룬다.
namespace { // 원문 little endian 필드와 x86 _ftol의 낮은 DWORD를 읽고 쓴다.
template<class F> F ClockNeed(const F& hook) { if (!hook) throw std::logic_error("시간 동기화의 실제 장치 미연결");return hook; } // callback이 자기 등록을 교체해도 현재 callable을 보유한다.
double ClockRead(std::span<const std::uint8_t> bytes,std::size_t at) { std::uint64_t bits{};for (std::size_t index=0;index<8;++index) bits|=std::uint64_t(bytes[at+index])<<(index*8);return std::bit_cast<double>(bits); } // 같은8BYTE만 낮은 바이트부터 조합한다.
void ClockWrite(std::span<std::uint8_t> bytes,std::size_t at,double value) { const auto bits=std::bit_cast<std::uint64_t>(value);for (std::size_t index=0;index<8;++index) bytes[at+index]=static_cast<std::uint8_t>(bits>>(index*8)); } // 이웃 헤더/필드 바이트를 보존한다.
std::uint32_t ClockTruncate(double value) { if (!std::isfinite(value) || value>=9223372036854775808.0 || value< -9223372036854775808.0) return 0;return static_cast<std::uint32_t>(static_cast<std::int64_t>(value)); } // 유효 signed64 밖의 x86 indefinite 낮은 DWORD는0이다.
int ClockCompare(const void* first,const void* second) { const auto* left=static_cast<const std::array<double,2>*>(first);const auto* right=static_cast<const std::array<double,2>*>(second);return (*left)[1]<(*right)[1] ? -1 : (*right)[1]<(*left)[1] ? 1 : 0; } //004c1e30의 두 ordered 비교이며 NaN/동률은0이다.
}
RawClockSynchronization::RawClockSynchronization(ClockSyncResetGlobals globals,ClockSynchronizationData& data,ClockSynchronizationHooks hooks,std::shared_ptr<void> dataOwner):dataOwner_(std::move(dataOwner)),globals_(globals),data_(data),hooks_(std::move(hooks)) {} // 생성 자체는 전역/등록/기한을 변경하지 않는다.
void RawClockSynchronization::Register(RawPacketDispatch& dispatch) {
    dispatch.Observe(5,[](std::uint32_t) {}); //40b380의 비NULL 빈 observer를 등록하며 version/length 정책은 기존 값을 유지한다.
    if (dataOwner_) {
        dispatch.RegisterCanonicalDirect(5,0x004c1f60U,[owner=dataOwner_,globals=globals_,data=&data_,hooks=hooks_](auto,PacketConnection* connection,auto packet) { //저장 F는 같은 Client 전역/실제 시계·Registry만 보유하고 짧은 this를 캡처하지 않는다.
            static_cast<void>(owner);if (!connection) throw std::logic_error("시간 수신의 실제 connection 누락");if (packet.size()<32) throw std::out_of_range("시간 동기화 실제32BYTE 부족");
            std::array<std::uint8_t,32> current;std::copy_n(packet.begin(),32,current.begin());RawClockSynchronization selected(globals,*data,hooks);selected.Receive(*connection,current); //이 wrapper 생성은 전역을 reset/재등록하지 않고 같은 현재 데이터만 읽는다.
        });dispatch.PersistCanonicalBaseline(5);return; //실제4c1e70/4c1eb0 등록 시점에만 장기 F/observer를 반납 baseline으로 보존한다. owner는 실제 LocalState 수명이며 enqueue Runtime pin이 아니다.
    }
    dispatch.Register(5,[this](auto,auto& connection,auto packet) { if (packet.size()<32) throw std::out_of_range("시간 동기화 실제32BYTE 부족");std::array<std::uint8_t,32> current;std::copy_n(packet.begin(),32,current.begin());Receive(connection,current); }); // const 전송 어댑터의 현재 수신32BYTE만 지역 writable로 빌리며 실제 응답/표본 바이트를 유지한다.
}
void RawClockSynchronization::InitializeServer(RawPacketDispatch& dispatch) { Register(dispatch);data_.sent=0;data_.received=0;globals_.replyMode=1; } // 표본/기한/이전 연결은 원문에서 쓰지 않는다.
void RawClockSynchronization::Attach(RawPacketDispatch& dispatch,std::uint32_t connection) {
    Register(dispatch);for (auto& sample:data_.samples) sample.fill(0); // 원문560DWORD0은70개 표본의 모든 헤더/시간/padding을 포함한다.
    const auto now=ClockNeed(hooks_.wallSeconds)();data_.sent=0;data_.received=0;globals_.replyMode=0;data_.next=now;globals_.connection=connection;
}
void RawClockSynchronization::ClearTimes(std::span<std::uint8_t> packet) { ClockWrite(packet,8,0.0);ClockWrite(packet,16,0.0);ClockWrite(packet,24,0.0); } // 세 원문 double 이외의 바이트를 초기화하지 않는다.
void RawClockSynchronization::Receive(PacketConnection& connection,std::span<std::uint8_t> packet) const {
    if (packet.size()<32) throw std::out_of_range("시간 수신의 실제32BYTE 범위");std::array<std::uint8_t,20> address{};address[0]=2;if (connection.flags&0x220U) address=connection.address;
    if (globals_.replyMode) { const auto now=ClockNeed(hooks_.gameSeconds)();ClockWrite(packet,16,now);ClockNeed(hooks_.reply)(connection,packet.first(32),(connection.flags&0x220U) ? &address : nullptr);return; }
    const auto now=ClockNeed(hooks_.wallSeconds)();ClockWrite(packet,24,now);
    for (auto& sample:data_.samples) { // 원문은 중복/빈 요청시간도 첫 double 동등 표본을 선택하고 응답 수를 누적한다.
        if (ClockRead(sample,8)==ClockRead(packet,8)) { ++data_.received;std::copy_n(packet.begin(),32,sample.begin());return; }
    }
}
void RawClockSynchronization::Poll() const {
    if (globals_.replyMode || !globals_.connection) return;const auto now=ClockNeed(hooks_.wallSeconds)();std::array<std::array<double,2>,70> accepted{};std::size_t count{};
    for (auto& sample:data_.samples) { //70개 현재 표본의 요청/서버/수신 시각을 원문 순서로 읽고 만료 시간만 지운다.
        const auto request=ClockRead(sample,8),server=ClockRead(sample,16),received=ClockRead(sample,24);
        if (!(now-request<=2.0) && received==0.0) { ClearTimes(sample);continue; }
        if (request!=0.0 && server!=0.0 && received!=0.0) { const auto roundTrip=received-request;const auto offset=received-server;if (roundTrip>=2.0 || std::isnan(roundTrip)) { ClearTimes(sample);continue; }accepted[count++]={offset,roundTrip}; }
    }
    if (count) {
        std::qsort(accepted.data(),count,sizeof(accepted[0]),ClockCompare);const auto used=std::max<std::size_t>(count/2,1);double roundTrips{},offsets{};
        for (std::size_t index=0;index<used;++index) { // RTT가 빠른 절반(최소1)의 각 합계를 원문 인덱스 순서로 누적한다.
            roundTrips+=accepted[index][1];offsets+=accepted[index][0];
        }
        const auto reciprocal=1.0/static_cast<double>(used);const auto latency=(reciprocal*roundTrips)*0.5;data_.latencyMilliseconds=ClockTruncate(1000.0*latency);globals_.gameTicks=ClockTruncate((reciprocal*offsets-latency)*1000.0);
    }
    if (!((std::bit_cast<std::int32_t>(data_.sent)<=30 || data_.received!=0) && data_.next!=0.0 && data_.next<=now)) return;
    for (auto& sample:data_.samples) { // 현재 첫 빈 표본 하나만 송신하며 공간이 없더라도 원문 기한/송신수 관문은 후반에서 갱신한다.
        if (now-ClockRead(sample,8)>30.0) ClearTimes(sample);if (ClockRead(sample,8)!=0.0) continue;
        ClearTimes(sample);sample[1]=5;sample[2]=32;sample[3]=0;ClockWrite(sample,8,now);ClockNeed(hooks_.send)(globals_.connection,sample);++data_.sent;break;
    }
    data_.next=0.0;if (std::bit_cast<std::int32_t>(data_.sent)<60) data_.next=now+1.0;
}
}
