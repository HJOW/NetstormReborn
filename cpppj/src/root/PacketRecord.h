// R Zacket의 실제 방향/현재 FullTime·프레임/주소·sender/패킷 기록 장치다.
#pragma once
#include "root/Clock.h"
#include "root/Configuration.h"
#include "platform/UserDataRoot.h"
#include <cstdio>

namespace netstorm::root { // 기존 서버 기록 문법을 사용자 홈 파일로 보관한다.
class PacketRecord final { // 시계와 이 owner는 전송/설정 callback보다 오래 살아야 한다.
public:
    PacketRecord(ClockState& clock,platform::UserDataRoot user); // 생성은 파일/폴더를 만들지 않는다.
    ~PacketRecord(); // 자기 실제 FILE 하나만 닫는다.
    PacketRecord(const PacketRecord&)=delete;PacketRecord& operator=(const PacketRecord&)=delete; // 현재 FILE/쓰기 권한을 복제하지 않는다.
    void Open(std::string_view name);void Close(); //00482720/826d0의 먼저닫기→enabled1→w+b이며 자동 쓰기는 사용자 Diagnostics 아래다.
    void Write(bool outgoing,const o::PacketConnection& connection,std::span<const std::uint8_t> packet,std::uint16_t length); //00482770의21BYTE 접두/현재 헤더 길이 본문·6번 fwrite/flush다. 호출자의5번째 length는 원문처럼 사용하지 않는다.
    void Bind(ConfigurationHooks& config,TransceiverHooks& transport); // 실제 openRecord/방향 기록을 같은 FILE/시계에 연결한다.
private:
    ClockState& clock_;platform::UserDataRoot user_; // 현재1b28/1b00과 기존 사용자 홈 정책이다.
    std::FILE* file_{};std::uint32_t enabled_{}; //004d8a80/84의 BSS0과 열기 실패 뒤enabled1을 구별한다.
};
}
