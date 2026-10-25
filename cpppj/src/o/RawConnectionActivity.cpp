// 실제 wall→ftol→frame 캡처→wall store→server 조회→frame store→상태 삭제의 순서를 보존한다.
#include "o/RawConnectionActivity.h"
#include "o/RawPacketDispatch.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 상위 Client 수명과 분리된 실제 패킷/Timer 공통 처리기다.
namespace { // 이 몸체에 필요한 정확한 signed64 변환만 내부에서 공유한다.
//004e49c0의 signed64 0방향 절삭 뒤 EAX다. masked x87의 비유한/범위밖 integer-indefinite 하위DWORD는0이다.
std::uint32_t WallLowDword(double value) {
    if (!std::isfinite(value) || value<-0x1p63 || value>=0x1p63) return 0;
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(value));
}
}

// 두 Native 주소를 실수로 같은 표에 연결하거나 실제 필수 효과를 생략한 구성을 거부한다.
RawConnectionActivity::RawConnectionActivity(ConnectionActivityTables tables,ConnectionActivityHooks hooks)
    :tables_(tables),hooks_(std::move(hooks)) {
    if (tables_.frames.data()==tables_.wallSeconds.data() || !hooks_.wallSeconds || !hooks_.frameNumber || !hooks_.server || !hooks_.eraseStatus)
        throw std::invalid_argument("연결 활동의 별개 두 표/실제 필수 장치 누락");
}

// server는 앞선 Wall 효과 뒤 다시 읽으며 frame은 wall store 전에 캡처한 같은 DWORD를 사용한다.
void RawConnectionActivity::Receive(std::int32_t sender) const {
    if (sender<0 || static_cast<std::uint32_t>(sender)>=kConnectionActivityEntries) throw std::out_of_range("연결 활동의 정상 Native 인덱스0..21 범위밖");
    const auto slot=static_cast<std::size_t>(sender);const auto wall=WallLowDword(hooks_.wallSeconds());const auto frame=hooks_.frameNumber(); //004b6188/6196/619b의 선택된 현재 입력이다.
    tables_.wallSeconds[slot]=wall;const bool server=hooks_.server();tables_.frames[slot]=frame; //004b61a6/61ad/61b6이며 두 store를 한꺼번에 앞당기지 않는다.
    hooks_.eraseStatus(server ? 2U : 1U,server ? static_cast<std::uint32_t>(sender) : 0U); // 원문 호출 뒤 표를 다시 동기화하거나 값을 복구하지 않는다.
}

// 실제0040be79..0040be97의 세 등록만 옮기며 길이5는 기존 PE 길이 표의 현재값이다.
void RegisterConnectionActivity(RawPacketDispatch& dispatch,RawConnectionActivity& activity) {
    dispatch.Observe(0x64,[](std::uint32_t) {}); //0040b380은 송신/수신 선택과 관계없이 아무 효과 없이 반환한다.
    dispatch.Version(0x64,1,0);
    dispatch.Register(0x64,[&activity](std::uint32_t sender,PacketConnection&,std::span<const std::uint8_t>) { activity.Receive(std::bit_cast<std::int32_t>(sender)); }); // 인접 connection/payload는 원문처럼 소비하지 않는다.
}
}
