// 원문 메타 참가자 메뉴의 자기/다른 사용자 분기와 현재 신뢰도 조회를 복원한다.
#pragma once
#include "client/Screen.h"
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::client { //같은 실제 CS/controlled 메뉴를 빌리고 독립 참가자 상태를 만들지 않는다.
struct NativeMetaPlayerMenuHooks { //원문 위치에서 선택된 장치만 호출한다.
    std::function<std::uint32_t()> subscriber,root; //현재540c74/563e38을각조회위치에서읽는다.
    std::function<std::uint32_t(std::uint32_t)> command; //현재542714/542960의실제명령DWORD다.
    std::function<std::string(std::string_view)> translate; //현재표시바이트번역이다.
    std::function<void()> beginFancy; //00477ae0의같은controlled스택생성이다.
    std::function<void(std::string_view)> title; //00477ee0의같은현재메뉴제목이다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t)> item; //00477cb0의include/enabled1·forceColor0항목이다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t)> attachCurrentObject; //00475cf0의현재메뉴객체부착과스택반납이다.
    std::function<std::uint32_t(std::uint32_t)> playerMenu; //기존004c9700의전체RawPlayerMenu.Open이다.
    std::function<void(std::uint32_t,std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attachObject; //이미꺼낸다른메뉴의실제가상+3c다.
};
void OpenMetaParticipantMenu(std::span<const std::uint8_t> participant,std::int32_t x,std::int32_t y,const NativeMetaPlayerMenuHooks& hooks); //004785f0 전체.활성여부는원문caller가판정한다.
std::uint32_t MetaPlayerReliability(const bool& metamap,std::uint32_t participant,const std::function<std::span<const std::uint8_t>(std::uint32_t)>& record); //00478310:meta0은CS조회없이100,그외현재두WORD의비율0..100또는100이다.
}
