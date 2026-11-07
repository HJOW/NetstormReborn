// 신전 가상6c의 확인WORD와 일반 Descend의 서버/지역WORD를 실제 입력에서 보존한다.
#pragma once
#include "client/InputEvent.h"
#include "o/SidPool.h"
#include <functional>
#include <string_view>

namespace netstorm::client { // 같은 일반 입력 큐와 실제 현재 Session의 경계다.
std::uint32_t VortexCommandScriptCode(std::string_view name); // 보호 PE 이름표의 WipeOut/Ascend/Descend/ViewMeta만 조회하며 별칭을 추가하지 않는다.
bool DispatchVortexCommandInput(const InputEvent& event,const std::function<void(o::Sid,std::uint32_t,std::uint16_t)>& command); //6f/7c/a7의 y낮은WORD SID/높은WORD 인자를 전달하고 뗌은 소비한다.
struct ViewMetaInputHooks { // 선택된 원문 분기에서만 실제 장치를 요구한다.
    std::function<std::int32_t()> serverCount; //00458e60의 현재 시작 서버 행 개수다.
    std::function<void()> invalidServer; //19번 로그 뒤 같은 Config curServer를"0"으로 쓴다.
    std::function<void(std::int32_t)> descend; // 낮은WORD 지역으로004b3760을 요청한다. 서버 선택을 여기서 바꾸지 않는다.
};
bool DispatchViewMetaInput(const InputEvent& event,std::uint32_t command,const ViewMetaInputHooks& hooks); // 현재ViewMeta 명령을 비교하고 signed서버WORD의-1/목록 상한을 검사한다.
}
