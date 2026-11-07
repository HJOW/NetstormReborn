// PlayerInfo의 전체 입력을 같은 파생 노드/CS·87BYTE 옵션과 실제 메뉴·송신에 연결한다.
#pragma once
#include "client/HudNsInfoCreate.h"
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::client { // 원문 PlayerInfo는 별도 모드/권한 검사를 추가하지 않는다.
struct NativePlayerInfoInputHooks {
    std::function<std::uint32_t(std::uint32_t)> command,serial,buttonValue; // 현재 명령/자식 serial과 실제 버튼 DWORD 값을 읽는다.
    std::function<std::uint32_t()> owner,subscriber; // 현재 540c70/74를 같은 Client 장기 저장소에서 읽는다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> participant,battle; // 현재 실제 CS 참가자/전투 기록의 같은 물리를 빌린다.
    std::function<void(std::uint32_t)> auxiliary; // 자기 특수 슬롯 변경 때만 실제594fc4 DWORD를 쓴다.
    std::function<void(const std::function<void(std::span<std::uint8_t>)>&)> status; // 실제 1fh 헤더 뒤 payload/부분효과를 실행하고 TCP11로 보낸다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>,std::uint32_t)> data; // 실제 현재 옵션87BYTE를 같은67h/TCP11 장치로 보낸다.
    std::function<std::string(std::string_view)> translate; // 현재 언어의 원본 글꼴 BYTE 번역이다.
    std::function<void()> beginMenu,attachMenu; // 같은 Fancy controlled 메뉴 생성과 Current→원시좌표/루트 부착이다.
    std::function<void(std::string_view)> title; // 같은 실제 메뉴 제목을 추가한다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t)> item; // 원문의 include/enabled1·force0 항목을 작성한다.
};
std::uint32_t HandleNativePlayerInfo(HudPlayerInfoNode& node,std::uint32_t event,std::uint32_t first,std::uint32_t second,const NativePlayerInfoInputHooks& hooks); //0047a900 전체 준비/시작·좌우 슬롯 변경·추방 확인이며 처리하지 않은 입력은0이다.
}
