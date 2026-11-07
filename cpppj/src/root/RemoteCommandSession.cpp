// 실제 원격 명령 세션의 IAC/행·에코와 부분 송신 버퍼를 기존 OS 장치에 연결한다.
#include "root/RemoteCommand.h"
#include <bit>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace netstorm::root { // 원문4*5000 ABI 대신 동일한 현재 논리 필드와 실제 소켓을 소유한다.
RemoteCommandSession::~RemoteCommandSession() noexcept(false) { if (Active()) End(); }
std::uintptr_t RemoteCommandSession::Handle() const { return socket_ ? socket_->Handle():~std::uintptr_t{}; }
SocketDevice& RemoteCommandSession::Socket() { if (!socket_) throw std::logic_error("R 원격 명령의 실제 NULL socket 접근");return *socket_; }
void RemoteCommandSession::Handler(std::shared_ptr<RemoteCommandHandler> handler) { handler_=std::move(handler); }
bool RemoteCommandSession::Notify(RemoteCommandEvent event) { const auto lease=handler_;if (!lease) return true;return (*lease)(event,*this,event==RemoteCommandEvent::Line ? std::span<std::uint8_t>(state_.line):std::span<std::uint8_t>{}); } // callback 교체/End 재진입 때도 현재 실행 함수/세션의 수명을 유지한다.
bool RemoteCommandSession::Start(std::unique_ptr<SocketDevice> socket,std::shared_ptr<RemoteCommandHandler> handler) {
    if (socket_ || !socket) throw std::logic_error("R 원격 명령의 빈세션/실제 socket 채택 조건");handler_=std::move(handler);socket_=std::move(socket);state_.lineLength=0;state_.outputLength=0;state_.masking=0;
    constexpr std::array<std::uint8_t,3> suppressGoAhead{0xff,0xfb,0x03},echo{0xff,0xfb,0x01}; // 보호PE의4c1140/113c 두 strlen3 문자열이다. 순서를 바꾸거나 표준 Telnet 상태로 보정하지 않는다.
    Write(suppressGoAhead);Write(echo);return true;
}
bool RemoteCommandSession::End() { socket_.reset();static_cast<void>(Notify(RemoteCommandEvent::Ended));return true; } // 이미NULL이어도 현재callback의 Ended 통지는 원문처럼 실행한다.
std::uint32_t RemoteCommandSession::Write(std::span<const std::uint8_t> bytes) {
    if (bytes.size()>std::numeric_limits<std::uint32_t>::max()) throw std::out_of_range("R 원격 명령 출력의 DWORD 길이 범위");const auto length=static_cast<std::uint32_t>(bytes.size()),total=state_.outputLength+length;if (std::bit_cast<std::int32_t>(total)>=4801) return 0;
    if (state_.outputLength>state_.output.size() || bytes.size()>state_.output.size()-state_.outputLength) throw std::out_of_range("R 원격 명령의 실제4800BYTE 출력 범위");if (!bytes.empty()) std::memmove(state_.output.data()+state_.outputLength,bytes.data(),bytes.size());state_.outputLength=total;return length;
}
void RemoteCommandSession::Print(std::string_view text) { text=text.substr(0,text.find('\0'));Write({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
void RemoteCommandSession::Printf(const char* format,...) {
    if (!format) throw std::invalid_argument("R 원격 명령 printf의 NULL 서식");std::array<char,1024> text;va_list args;va_start(args,format);const auto length=std::vsnprintf(text.data(),text.size(),format,args);va_end(args);text.back()=0;if (length<0 || static_cast<std::size_t>(length)>=text.size()) throw std::out_of_range("R 원격 명령 printf의 실제1024BYTE 지역 범위");Print(text.data());
}
void RemoteCommandSession::Byte(std::uint8_t value) { Write({&value,1}); }
void RemoteCommandSession::Consume(std::int32_t sent) {
    if (sent<0 || state_.outputLength>state_.output.size() || static_cast<std::uint32_t>(sent)>state_.outputLength) throw std::out_of_range("R 원격 명령 송신 뒤 실제 출력 이동 범위");const auto left=state_.outputLength-static_cast<std::uint32_t>(sent);std::memmove(state_.output.data(),state_.output.data()+sent,left);state_.outputLength=left;
}
bool RemoteCommandSession::FlushComplete() {
    while (std::bit_cast<std::int32_t>(state_.outputLength)>0) { //7e9b0은 현재 길이보다 짧은 첫송신에서 종료하며 부분 성공을 재시도하지 않는다.
        if (state_.outputLength>state_.output.size()) throw std::out_of_range("R 원격 명령 flush의 실제 출력 범위");const auto sent=Socket().Send(std::span(state_.output).first(state_.outputLength));if (sent<std::bit_cast<std::int32_t>(state_.outputLength)) { End();return false; }Consume(sent);
    }return true;
}
bool RemoteCommandSession::Poll() {
    std::array<std::uint8_t,20> input;const auto received=Socket().Receive(input);if (received==-1) { End();return false; }if (received<0 || static_cast<std::size_t>(received)>input.size()) throw std::out_of_range("R 원격 명령의 실제20BYTE 수신 범위");
    for (std::int32_t index=0;index<received && std::bit_cast<std::int32_t>(state_.lineLength)<159;++index) { // 행이159가 되면 남은 실제 수신 바이트를 버리며 자동잘라내기/행초기화를 추가하지 않는다.
        const auto value=input[static_cast<std::size_t>(index)];
        if (state_.negotiation==0) {
            if (value==0xff) state_.negotiation=1;
            else if (!state_.subnegotiation) {
                if (value==13) { Byte(13);Byte(10);if (state_.lineLength>=state_.line.size()) throw std::out_of_range("R 원격 명령의 실제160BYTE 행 범위");state_.line[state_.lineLength]=0;if (!Notify(RemoteCommandEvent::Line)) { End();return false; }state_.lineLength=0; }
                else if (value==8) { if (std::bit_cast<std::int32_t>(state_.lineLength)>0) { --state_.lineLength;Byte(8);Byte(32);Byte(8); } }
                else if (value!=10 && value!=0) { if (state_.lineLength>=state_.line.size()) throw std::out_of_range("R 원격 명령의 실제160BYTE 행 범위");state_.line[state_.lineLength++]=value;Byte(state_.masking ? state_.maskCharacter:value); }
            }
        } else if (state_.negotiation==1) {
            if (value==0xff) { if (!state_.subnegotiation) { if (state_.lineLength>=state_.line.size()) throw std::out_of_range("R 원격 명령 literal FF의 실제 행 범위");state_.line[state_.lineLength++]=0xff; }state_.negotiation=0; } // literal FF는 원문처럼 별도 에코를 하지 않는다.
            else { state_.command=value;state_.negotiation=2; }
        } else if (state_.negotiation==2) { if (value==0xf0) state_.subnegotiation=0;else if (value==0xfa) state_.subnegotiation=1;state_.negotiation=0; }
    }
    if (std::bit_cast<std::int32_t>(state_.outputLength)>0) { if (state_.outputLength>state_.output.size()) throw std::out_of_range("R 원격 명령 Poll의 실제 출력 범위");const auto sent=Socket().Send(std::span(state_.output).first(state_.outputLength));if (sent==-1) { End();return false; }Consume(sent); }return true; // 여기서는 실제 부분 송신/0을 보관하고 다음 순회에 같은 나머지를 사용한다.
}
}
