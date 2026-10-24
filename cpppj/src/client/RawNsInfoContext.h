// NSInfo 전체 생성에 같은 실제 Client 전역/물리와 공유 문자열 고리를 빌린다.
#pragma once
#include "client/ClientNsInfoGlobals.h"
#include <functional>
#include <memory>
namespace netstorm::client {
class ClientGameplayGlobals;class RawSurfaceStorage; // 초기 HUD와 Session의 실제 장기 저장소다.
struct RawNsInfoContext {
    std::shared_ptr<ClientNsInfoGlobals> globals; // 같은 current/zone/서버 목록과 이름80BYTE다.
    std::shared_ptr<ClientGameplayGlobals> gameplay;std::shared_ptr<RawSurfaceStorage> storage; // 현재 known/메타모드를 값 사본 없이 조회한다.
    std::function<std::uint32_t(std::uint32_t)> read; // 같은 현재 배경3전역/fortGump/흰색을 실제 사용 시 조회한다.
    std::function<std::string(std::string_view)> translate; // 실제 현재 번역을 원본 글꼴 바이트로 제공한다.
    std::function<std::uint32_t(std::string_view)> storeText;std::function<std::string_view(std::uint32_t)> text; // 실제80칸 형식 문자열 고리의 같은 수명이다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t,std::uint32_t)> constructButton; // 공통 ButtonGump::ConstructBase를 RawButtonControls가 연결한다.
};
}
