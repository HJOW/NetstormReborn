// 원본004b5ba0의 옵션·저장상태·벽시각·재접속 질문 순서만 소유한다.
#pragma once
#include <cstdint>
#include <functional>
#include <string_view>

namespace netstorm::o {
// 저장상태 reader와 실제 번역/서식/Tell은 현재 Client/Session 수명의 장치를 직접 빌린다.
struct StartupStateCheckHooks {
    std::function<std::int32_t(std::string_view)> configInt; //00441090 GetInt이며 값0에서만 다른 장치를 생략한다.
    std::function<std::int32_t()> readSavedState; //004b5890의 실제 Player/이름/CoreData/State 부분효과와 signed 저장시각이다.
    std::function<double()> wallSeconds; //00460d70의 매호출 벽시각이며 reader 반환0에서도 호출한다.
    std::function<std::string_view(std::string_view)> translate; //Yes/No/원문 질문 순서이며 반환주소는 뒤 서식 호출까지 유효해야 한다.
    std::function<std::string_view(std::string_view,std::string_view,std::string_view)> formatButtons; //원문format/Yes/No를 받아 같은 버튼 문자열을 반환한다.
    std::function<std::string_view(std::string_view,double,std::string_view)> formatWarning; //번역질문/float32분의double승격/현재버튼 문자열이다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t)> tell; //004cf550(text,0,0)의 실제 임시 Tell이며 MessageBox로 바꾸지 않는다.
};
// 옵션0 또는 저장시각/엄격경과분 관문실패는0,실제질문까지실행하면1이다.선택하위예외는부분효과와함께전파한다.
std::uint32_t CheckStartupState(const StartupStateCheckHooks& hooks);
}
