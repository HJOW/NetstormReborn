// Quick Help 요청의 실제 주제 선택·이력·창 갱신·현재 위치 재부착 순서를 복원한다.
#pragma once
#include "client/GumpCreate.h"
#include <functional>
#include <string>
#include <string_view>
namespace netstorm::client {
// 명시 조회/서식·실제 이력/전용 창 및 같은 Gump의 가상 동작을 공급한다.
struct QuickHelpRequestHooks {
    std::function<std::int32_t(std::string_view)> lookup; // 원본49a860의 타입 이름 조회다. 음수는 미발견이고0은 유효한 반환이다.
    std::function<std::string(std::string_view,std::string_view)> print; // 원본4a89c0의 %s/sun%s/wind%s 순수 서식이다.
    std::function<void(std::string_view)> history; // 원본4c52c0에 정규화 전 원래 주제를 보낸다.
    std::function<void(std::uint32_t,std::string_view)> show; // 원본4c5970 전용 본문/그림 창을 갱신한다. 일반 Tell로 대체하지 않는다.
    std::function<void()> push; // 같은 현재 창의 가상+18이다. 현재 위치/플래그를 바꿀 수 있다.
    std::function<void(float,float,std::uint32_t,std::uint32_t)> attach; // 같은 현재 창의 가상+20이다. Push 후의 실제 x/y를 전달한다.
};
// PATCH004c5c00의 요청 계층이다. 실제 창 생성/파생 본문/이력 저장소는 공급자가 이어서 연결해야 한다.
class QuickHelpRequest {
public:
    QuickHelpRequest(GumpCreateNode& window,QuickHelpRequestHooks hooks); // 같은 창의 현재 기본 노드와 수명을 빌린다.
    void Open(std::uint32_t type,std::string_view topic) const; // type0만 접미 제거/3단계 조회를 한다. 직접 TYPE는 원문 주제를 그대로 전달한다.
private:
    GumpCreateNode& window_; // 창 갱신과 Push 효과 이후 재조회하는 실제 flags/x/y 저장소다.
    QuickHelpRequestHooks hooks_; // 실제 선택된 외부 경계만 요구한다. 미연결은 명시 거부한다.
};
}
