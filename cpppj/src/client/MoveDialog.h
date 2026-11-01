//004c8df0의 현재 모달 위치 명령을 공통 Gump의 실제 필드와 연결한다.
#pragma once
#include "client/GumpCreate.h"
#include <string_view>

namespace netstorm::client { // 같은 포인터를 Push 전후로 유지하는 원문 부모다.
inline constexpr std::uint32_t kMoveDialogCommand=0x020000edU; // 보호PE542aa8의 실제 일반 입력 명령이다.
// 현재 캡처한 노드의 분리와 같은 공통 트리의 재부착만 외부 효과로 제공한다.
struct MoveDialogHooks {
    std::function<void()> push; // 캡처한 현재 노드의 가상+18을 호출하며 다른 현재 모달로 대상을 바꾸지 않는다.
    std::function<void(float,float,std::uint32_t,std::uint32_t)> attach; // 같은 노드를00466750의x/y/0/0으로 다시 붙인다.
};
// lowercase b/t/r/l의 존재를 원문 순서로 검사한다. m/c는 좌표를 바꾸지 않고 NUL 뒤는 읽지 않는다.
// 화면 정수는 산술 뒤에만float으로 저장하며 Push 효과 뒤의 실제 화면/창 크기를 읽는다.
void MoveDialog(GumpCreateNode* current,std::string_view location,const int& screenWidth,const int& screenHeight,const MoveDialogHooks& hooks);
}
