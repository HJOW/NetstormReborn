// NSInfo 루트의 원문 입력을 같은 정렬표/CS와 현재 메뉴·카메라·송신 장치에 연결한다.
#pragma once
#include "client/Renderer.h"
#include <functional>
#include <span>

namespace netstorm::client { // 원문 DWORD 입력은 실제 효과를 선택할 때만 좁힌다.
struct NativeNsInfoInputHooks {
    std::function<std::uint32_t(std::uint32_t)> command,roster; // 현재 명령 DWORD와 5930b0의 같은 정렬표를 읽는다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> participant; // 현재 실제 CS의 참가자 물리를 빌린다.
    std::function<ScreenPoint()> mouse; // 5cae94/98의 원시 화면 좌표를 소비 시점에 읽는다.
    std::function<void(std::uint32_t,ScreenPoint)> menu; // 4785f0 전체 현재 참가자 메뉴 공장이다.
    std::function<void(std::uint32_t)> center; // 현재 참가자 SID를 실제 433ba0/즉시 카메라에 전달한다.
    std::function<void(const std::function<void(std::span<std::uint8_t>)>&)> status; // 실제 1fh/12BYTE 헤더 뒤 부분효과를 수행하고 11로 보낸다.
    std::function<void()> kickoffSound; // chalKickedoff.wav의 원문 우선순위1 재생이다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t)> base; // 처리하지 않은 원래 세 DWORD를 같은 BaseHelp에 전달한다.
};
std::uint32_t HandleNativeNsInfo(std::uint32_t event,std::uint32_t first,std::uint32_t second,const NativeNsInfoInputHooks& hooks); //0047c8c0 전체: 정렬 메뉴/중심·직접 참가자 추방·원래 입력의 기본 처리다.
}
