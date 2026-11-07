// PlayerInfo 특수 버튼의 원문 전체 Draw를 현재 공통 표면/자산에 적용한다.
#pragma once
#include "client/HudNsInfoCreate.h"

namespace netstorm::client { //노드/그림 수명은 같은 동기 Draw 동안 유지한다.
struct HudPlayerButtonDrawHooks { //현재 표면을 빌리며 별도 픽셀 버퍼를 만들지 않는다.
    std::function<const HudPlayerButtonNode&(std::uint32_t)> node; //실제 기본 value/pressed와+c0/+c4를 읽는다.
    std::function<ScreenRect(std::uint32_t)> bounds; //004648e0의 같은 실제 Gump 경계다.
    std::function<std::uint32_t(std::uint32_t)> read; //현재 배경 frame/TYPE와fortGump TYPE 전역이다.
    std::function<void(ScreenRect,std::uint32_t,std::uint32_t,std::uint32_t)> background; //00465880의 실제 원문 플래그/배경 그림이다.
    std::function<void(ScreenRect,std::uint32_t)> fill; //004a2060의 현재 클립/원색 단색 채우기다.
    std::function<std::uint32_t(std::uint32_t)> type; //0049a840의 실제 현재 TYPE 조회와 같은 논리 레코드 번호다.
    std::function<std::int32_t(std::uint32_t)> firstL; //조회한 현재 TYPE+15c의 L 첫 프레임이다.
    std::function<void(std::uint32_t,std::uint32_t,ScreenPoint)> shape; //004daa70의 같은 원색 SHP 기준점 출력이다.
};
void DrawPlayerInfoButton(std::uint32_t token,const HudPlayerButtonDrawHooks& hooks); //004252b0 전체: 경계/활성 캡처→배경→안쪽 단색→현재TYPE/L→선택적 그림이다.
}
