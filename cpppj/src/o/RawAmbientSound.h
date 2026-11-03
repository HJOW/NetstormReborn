// 주변음004180f0/CD004eca20과 State의 같은 벽시각 기한을 직접 연결한다.
#pragma once
#include "o/SpStore.h"
#include <functional>
#include <string_view>
namespace netstorm::o { // 게임 정지/음향 장치와 별개인 Client 현재값을 읽는다.
struct AmbientSoundGlobals { // 원본00544d58/50/54 또는 CD 대응 전역의 비소유 뷰다.
    double& deadline; // State/Edit가 쓰는 같은 기한이며 프레임 사본으로 바꾸지 않는다.
    std::int32_t& kind; //0천둥/1독수리/2고래이며 범위 밖 값도 그대로 보존한다.
    std::int32_t& forced; //signed 양수일 때만 기존 kind를 유지한다. 원문 최초값은0이다.
};
struct AmbientSoundHooks { // 도달한 실제 장치만 요구하며 난수/시간 조건은 부모가 처리한다.
    std::function<double()> wallSeconds; // 비교와 재예약에 각각 현재 벽시각을 읽는다.
    std::function<void(std::string_view,std::uint32_t,std::int32_t,std::int32_t,std::uint32_t,std::int32_t)> sound; // 이름/반복/음량/좌우/우선순위/한도의 실제 PlayByName이다.
    std::function<std::uint32_t()> currentDialog; // 소리 요청이 복귀한 뒤 현재 모달 토큰을 다시 읽는다.
    std::function<void(std::int32_t)> flash; // 모달0 천둥일 때만00470fa0(0)을 요청한다.
};
void StepAmbientSound(AmbientSoundGlobals globals,GameRandom& random,const AmbientSoundHooks& hooks); // Movie 갱신 잠금 없는 가지에서 바람 앞에 호출한다.
void ResetAmbientSoundDeadline(double& deadline,const std::function<double()>& wallSeconds); //004180d0/CD004ec9e0의 현재 벽시각+10 재예약이다.
}
