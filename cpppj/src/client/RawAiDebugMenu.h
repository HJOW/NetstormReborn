// 원문413dd0의 선택적 AI 디버그 메뉴를 현재 Player/brain과 controlled 작성기에 연결한다.
#pragma once
#include "client/RawPriestMenu.h"
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::client { // 파일/창/AI행동을새로소유하지않는현재메뉴부모다.
struct AiDebugMenuReads {
    std::function<std::uint32_t()> checking; // showSoftAsserts 현재DWORD는첫단락에서만읽는다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> player,body; // player(owner)/body(token)의실제현재물리다.
    std::function<std::uint32_t(std::uint32_t)> debug,command; // 원문로그전역/명령주소의현재값을각효과앞에읽는다.
};
struct AiDebugMenuHooks {
    std::function<std::string(std::uint32_t,std::string_view,std::span<const PriestMenuArgument>)> format; //4b9390의요청80/문자열서식장치이며실제UI는기존서식기를공유한다.
    std::function<void(std::string_view)> header; // 번역없는원문머리문자열을그대로작성한다.
    std::function<void(std::uint32_t,std::string_view,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> checked; // checked/label/command/param/include/enabled의전체DWORD다.
    std::function<void(std::uint32_t)> begin; //477bb0(0)의같은수직controlled메뉴스택이다.
    std::function<std::uint32_t()> take; //475c00의현재메뉴를한번꺼내는부모다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t,std::uint32_t)> submenu; //label/child/enabled/checked의실제Submenu작성이다.
};
void OpenAiDebugMenu(std::uint32_t owner,const AiDebugMenuReads& reads,const AiDebugMenuHooks& hooks); //checking0/owner비양수/brain0은효과없이다시돌아간다.
}
