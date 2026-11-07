// R의 전역 관찰기/버전·실제 수신 초기화를 같은 전송 등록표에 연결한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::root { //00482b60/442950 및 _p_packets.cpp 관찰기의 공통 부모다.
class TransceiverTransport;class RootControl;class RootDirectory;class LoginRequest;class Routing;class ChallengeRuntime; // 실제 수신 부모들은 초기화와 모든 등록 콜백보다 오래 살아야 한다.
using PacketAssertion=std::function<void(std::string_view,std::string_view,std::uint32_t)>; // 선택된 원문 표현식/파일·줄 진단이며 복귀 뒤 현재 필드를 다시 읽는다.
void ObservePacketFields(std::uint8_t type,std::span<std::uint8_t> packet,std::uint32_t sending,const PacketAssertion& assertion); // 송신만 동적 길이를 작성하고 빈 원문 관찰기는 물리를 읽지 않는다. 폐기7f/80은 처리하지 않는다.
void InitializePackets(TransceiverTransport& transport,RootControl& control,RootDirectory& directory,LoginRequest& login,Routing& routing,ChallengeRuntime& challenges); // 실행/Session 대여 밖에서 함수·버전만0으로 만들고 현재17수신/102관찰 등록·2번 종료 수신을 연결한다. 길이표와 같은 표 owner는 보존한다.
}
