// Client의 선택적 원문 패킷 기록 FILE/요청 DWORD를 Session보다 오래 소유한다.
#pragma once
#include "o/GameClock.h"
#include "o/RawPacketDispatch.h"
#include "platform/UserDataRoot.h"
#include <cstdio>
#include <optional>
#include <string_view>

namespace netstorm::client { //메타/일반/내부/편집 전송이 같은 현재 기록 장치를 빌린다.
class ClientPacketRecord final { //4debe0/4dec30의 실제 파일·전역 수명이며 생성은 파일을 열지 않는다.
public:
    ClientPacketRecord(std::shared_ptr<o::FrameTime> frame,std::optional<platform::UserDataRoot> user); //현재 프레임의 실제 wall/number를 pin한다.
    ~ClientPacketRecord(); //자기가 연 FILE만 닫고 빌린 시계는 뒤에 반납한다.
    ClientPacketRecord(const ClientPacketRecord&)=delete;ClientPacketRecord& operator=(const ClientPacketRecord&)=delete; //FILE/기록 권한을 복제하지 않는다.
    std::uint32_t& Requested() { return requested_; } //0054dbac의 최초0 요청 DWORD이며 현재 enabled와 별개다.
    const std::uint32_t& Requested() const { return requested_; } //원문 수신 부모는 이 현재 요청 값만 검사한다.
    void Open(std::string_view name); //기존 파일 닫기→enabled1→같은 이름의 사용자 Diagnostics/w+b 순서다.
    void Close(); //enabled0→실제 FILE 닫기→FILE0이며 요청 DWORD는 바꾸지 않는다.
    void Write(bool outgoing,const o::PacketConnection& connection,std::span<const std::uint8_t> packet,std::uint16_t length); //방향/현재 프레임 wall/low32번호/진입 주소·sender/현재 선언 본문을 여섯 번 fwrite 후 flush한다.
private:
    std::shared_ptr<o::FrameTime> frame_;std::optional<platform::UserDataRoot> user_; //같은 실제 프레임과 자동 쓰기의 사용자 홈 정책이다.
    std::FILE* file_{};std::uint32_t enabled_{},requested_{}; //005e4770/74·0054dbac의 독립 최초0이며 열기 실패 뒤enabled1을 보존한다.
};
}
