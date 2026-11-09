// 메타 섬의 실제 큰 배율 영역과 연결된 Gump의 원문 갱신 순서를 복원한다.
#pragma once
#include "client/ClientChallengeGlobals.h"
#include "client/Screen.h"
#include "o/RawCheckingFlag.h"

namespace netstorm::client { // 현재 CS·권한·finder 전역은 실제 Client 주소를 빌린다.
struct NativeMetaSpriteRefreshGlobals {
    ClientChallengeGlobals& challenge; //현재 전투/참가자 TYPE의 같은 장기 DWORD 소유자다.
    const std::uint32_t& excludedType;const std::uint32_t& processTypeEnd; //00541074/0054116c의 현재 전체DWORD다.
    o::RawCheckingFlag checking;const bool& metamap;const bool& boss; //dead 부모 진단에서 현재 상태를 다시 읽는다.
};
struct NativeMetaSpriteRefreshHooks { // 선택된 실제 장치만 해당 원문 위치에서 요구한다.
    std::function<ScreenRect(std::span<const std::uint8_t>)> bounds; //00498ff0의 현재 TYPE/frame/위치·Renderer 배율 사각형이다.
    std::function<void(ScreenRect)> dirty; //00499310의 좌표별 viewport clamp와 유효 면적 등록이다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> raw; //같은 실제 Pool의50BYTE Form 슬롯이다.
    std::function<void(std::uint32_t)> redrawGump; //현재 등록ID의 FindGump→비영일때만 가상Redraw(false)다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //Squidfinder.cpp158의 복귀형 진단이다.
};
void RefreshNativeMetaSprite(std::span<const std::uint8_t> object,NativeMetaSpriteRefreshGlobals globals,const NativeMetaSpriteRefreshHooks& hooks); //004787c0 전체: 기본 영역→큰 배율 보정→dirty→기본 Form finder/이름표 갱신이다.
void RefreshNativeSprite(std::span<std::uint8_t> object,NativeMetaSpriteRefreshGlobals globals,const NativeMetaSpriteRefreshHooks& hooks); //004791b0 전체: 일반 Sprite의 중심 확대→dirty→BYTE35=20→동일 기본 Form finder/Gump 갱신이다.
}
