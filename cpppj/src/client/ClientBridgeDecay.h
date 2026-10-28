//00422bc0의 실제 공통 프레임 위치와 World 밖에도 남는 스캔 저장소를 연결한다.
#pragma once
#include "o/GameClock.h"
#include "o/SidPool.h"
#include <functional>
#include <string>

namespace netstorm::client { //Client와 독립 검사가 같은 전역 스캔 입구를 사용한다.
class Client;class RawSurfaceStorage;class ClientGameplayGlobals; //장기 소유자의 주소만 빌린다.
void RunSurfaceBridgeDecay(RawSurfaceStorage& storage,const ClientGameplayGlobals& globals,const o::FrameTime& time,bool paused,const std::function<void(o::Sid)>& cell); //현재 전역 관문과 같은 실제 커서/주기를 읽으며 초기화하지 않는다.
void RunClientBridgeDecay(Client& client); //메뉴/World 양쪽에서469f60 앞에 호출하고, 대상 칸만 현재 Session에 분배한다.
std::string ReportClientBridgeDecay(const Client& client); //CLI 보고에서만 같은 현재 스캔/대여/대상 개수를 읽으며 프레임을 실행하지 않는다.
}
