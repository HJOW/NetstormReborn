// 원문 R의 진단 수신/응답과 실제 역방향 TCP 연결을 같은 전송·게임 시계에 연결한다.
#include "root/RootControl.h"
#include "root/AddressCodec.h"
#include "root/ChallengeDetail.h"
#include "root/Clock.h"
#include <cstdio>

namespace netstorm::root { // 테스트를 실행하지 않으며 실제 등록 부모에서 선택하는 R 수신 몸체만 복원한다.
using namespace challenge_detail; // 같은 실제 x86 필드 폭과 미작성 지역 바이트 정책을 공유한다.
std::uint32_t RootControl::Connect(const std::string& text,std::uint32_t capacity) {
    if (!(state_.modeFlags&0x13U)) Assert("smModeFlags & (smDIAG|smROOT)","c:\\ns\\r\\r.cpp",213);const auto started=clock_.CaptureGame();const auto id=transport_.ConnectStream(text,addresses_,capacity);const auto finished=clock_.CaptureGame();
    std::array<char,128> message{};const auto length=std::snprintf(message.data(),message.size(),"Connection time (start to finish) = %f secs\n",finished-started);if (length<0 || static_cast<std::size_t>(length)>=message.size()) throw std::out_of_range("R 연결 시각 로그의 호스트 버퍼 범위");Need(log_)(39,std::string_view(message.data(),static_cast<std::size_t>(length)));return id; // 원문 시작→종료 두 별도 캡처이며 현재 frame wall 값으로 대체하지 않는다.
}
void RootControl::ReceiveDiagnostic(std::int32_t sender,std::span<const std::uint8_t> packet) {
    CheckExit();if (!(state_.modeFlags&0x11U)) return;const bool isReply=Read(packet,4)==2;std::array<std::uint8_t,12> reply;AllocationInput(reply);Header(reply,0x11,12);Put(reply,4,Read(packet,4));auto address=addresses_.Parse(std::optional<std::string>{"0.0.0.0"}); //443ff0은 헤더만 쓰고 body+8은 미작성이다. 요청의 port를 응답에 복사하지 않는다.
    if (!isReply) {
        const auto current=PendingTransceiver(state_,sender);const auto close=[&] { if (current) ClosePending(state_,std::bit_cast<std::int32_t>(transport_.Connection(current).sender),transport_.Lifecycle()); }; //4754e0은 하위pending을 빌리며475520은 같은 노드의 현재sender로 닫는다.
        try {
            address=transport_.Connection(current).address;
            if (Read(packet,8) && (state_.modeFlags&kDiagReceive)) {
                const auto text="tcp,"+std::to_string(address[4])+'.'+std::to_string(address[5])+'.'+std::to_string(address[6])+'.'+std::to_string(address[7])+':'+std::to_string(std::bit_cast<std::int32_t>(Read(packet,8)))+",cb"; //59560은 주소+4 getter다. 기존에 push한 port DWORD는 sprintf의 signed %d로 남으며 Parse에서 낮은 WORD가 된다.
                Need(log_)(29,"Diag: Calling back to "+text+"\n");if (!Connect(text,512)) Need(log_)(9,"Diag: Callback failed.\n");
            }
            transport_.Send(current,reply);transport_.Flush(current);
        } catch (...) { close();throw; }close(); // 원문 guard의 정상/예외 닫힘은 응답/flush 뒤이며 상위 ReadTransceiver로 대체하지 않는다.
    }
    const auto at=Read(address,0,2)==6 ? 12U:2U;const auto port=(std::uint32_t(address[at])<<8U)|address[at+1];
    Need(log_)(29,"Test conn. from "+std::string(addresses_.Format(address,true))+':'+std::to_string(port)+" ("+(isReply ? "udp":"tcp")+" code="+std::to_string(std::bit_cast<std::int32_t>(Read(packet,4)))+" port="+std::to_string(std::bit_cast<std::int32_t>(Read(packet,8)))+")\n"); // 원문 Format(1,0,0) 뒤 별도 :port를 다시 붙이는 서식과 닫힘 뒤 현재 요청 필드 읽기도 유지한다.
}
void RootControl::ReceiveDiagnosticRequest(std::int32_t sender,o::PacketConnection* connection) {
    if (!(state_.modeFlags&kDiagReceive)) return;const auto current=ReadTransceiver(state_,sender,transport_.Lifecycle());if (current ? &transport_.Connection(current)!=connection:connection!=nullptr) Assert("t == _t","c:\\ns\\r\\r.cpp",830);if (!current) return;
    std::array<std::uint8_t,12> reply;AllocationInput(reply);Header(reply,0x11,12);Put(reply,4,1);transport_.Send(current,reply);transport_.Flush(current);Need(log_)(29,"Diag: Test connection 2 handled from "+std::string(addresses_.Format(transport_.Connection(current).address,true))+"\n"); // body+8 미작성·같은 노드 flush 뒤 현재 주소를 읽으며 exitRequested는 원문처럼 조회하지 않는다.
}
}
