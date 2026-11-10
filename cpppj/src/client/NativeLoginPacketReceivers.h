// Login/연결 활동의 고정 F를 짧은 Login 객체와 분리한다.
#pragma once
#include "o/RawPacketDispatch.h"
namespace netstorm::client { // 장기 Client의 현재 저장소와 실제 호출 시점의 Runtime을 사용한다.
class Client; // 같은 앱 전역과 현재 장치를 소유한다.
void InstallNativeLoginPacketReceivers(Client& client,o::RawPacketDispatch& dispatch); //현재 Session을 조회하지 않고 검토한 기본 F/metadata를 설치하며 기존67h는 보존한다.
void BindNativeLoginPacketReceiver(o::RawPacketDispatch& dispatch,std::uint32_t type); //기존 caller 위치에서 같은 F/metadata를 검증하고 이번 lease만 붙인다.
}
