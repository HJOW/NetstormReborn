// NSInfo 전체 생성에 같은 실제 Client 전역/물리와 공유 문자열 고리를 빌린다.
#pragma once
#include "client/ClientNsInfoGlobals.h"
#include <functional>
#include <memory>
namespace netstorm::client {
class ClientGameplayGlobals;class RawSurfaceStorage; // 초기 HUD와 Session의 실제 장기 저장소다.
class ClientChallengeGlobals; //메타목록/패널이같은실제CS할당을빌리며별도참가자사본을만들지않는다.
struct RawNsInfoContext {
    std::shared_ptr<ClientNsInfoGlobals> globals; // 같은 current/zone/서버 목록과 이름80BYTE다.
    std::shared_ptr<ClientChallengeGlobals> challenge; //실제메타분기에서만필수인같은Client CS owner다.일반생성은없어도기존관문으로반환한다.
    std::shared_ptr<ClientGameplayGlobals> gameplay;std::shared_ptr<RawSurfaceStorage> storage; // 현재 known/메타모드를 값 사본 없이 조회한다.
    std::function<std::uint32_t(std::uint32_t)> read; //같은현재배경/fortGump·팔레트색/metadataMode를실제사용시조회한다.
    std::function<std::string(std::string_view)> translate; // 실제 현재 번역을 원본 글꼴 바이트로 제공한다.
    std::function<std::uint32_t(std::string_view)> storeText;std::function<std::string_view(std::uint32_t)> text; // 실제80칸 형식 문자열 고리의 같은 수명이다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t,std::uint32_t)> constructButton; // 공통 ButtonGump::ConstructBase를 RawButtonControls가 연결한다.
    std::function<void(std::uint32_t,bool)> enabled; //같은Button가상+3c의현재활성화/Redraw다.
    std::function<void(std::uint32_t,std::uint32_t)> value; //같은Button가상+40의기본/돌버튼현재값쓰기다.
};
}
