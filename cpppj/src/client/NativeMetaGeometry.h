// 26h 기하 배치 통지를 현재 Renderer 한계와 CS 공통 배치 전역에 연결한다.
#pragma once
#include "o/RawChallengeState.h"
#include <functional>
#include <string_view>

namespace netstorm::client { //이 수신은 현재 CS 포인터/메타 모드를 읽지 않는 원문 전역 처리다.
void ObserveNativeMetaGeometry(std::span<std::uint8_t> packet,bool sending,const std::function<void(std::string_view,std::uint32_t)>& assertion); //0040a3e0: 송신일 때 두 미작성 길이 진단 뒤 현재 modLen/contactLen으로 WORD 길이를 쓴다.
void ReceiveNativeMetaGeometry(std::span<const std::uint8_t> packet,o::ChallengeGeometry& geometry,std::uint32_t& mode,const std::function<void(float,float)>& limits); //004b45a0/42c780 전체: 화면 한계→mode0→조건부 실제 contact DWORD의 원점/두 반경→mode1이다.
}
