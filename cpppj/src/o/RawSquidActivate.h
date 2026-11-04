// 공통 Activate의 void·현재 권한·송신·가상 후처리·깊이 진단 순서를 보존한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string_view>

namespace netstorm::o { // 실제004ac560의 현재 물리 슬롯과 같은 장부를 사용한다.
struct SquidActivateHooks { // 도달한 원문 경계만 선택하며 callable 실행 중 연결 교체를 허용한다.
    std::function<bool()> server; //00540bc0을 void/전송상태 진단 뒤 현재값으로 읽는다.
    std::function<void(Sid,std::uint32_t)> transmit; // 현재 가상 writer와 실제 방송 분배를 실행한다.
    std::function<void(Sid,std::uint32_t)> postPop; // 깊이를 증가시킨 뒤 현재 가상+20을 호출한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // checking과 무관한 진단이며 정상 복귀 시 계속한다.
};
// 앞선 void/송신/깊이 변경은 진단 또는 하위 실패가 발생해도 되돌리지 않는다.
void RunSquidActivate(SidPool& pool,Sid sid,std::uint32_t flags,std::uint32_t& depth,const SquidActivateHooks& hooks);
}
