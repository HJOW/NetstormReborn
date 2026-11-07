// 원본 메타 섬의 동적 Image·전투 TYPE·설명/버전·8자리 그림 표시를 복원한다.
#pragma once
#include "client/ClientChallengeGlobals.h"
#include "client/GumpText.h"

namespace netstorm::client { // 같은 CS와 실제 표시 장치를 동기 호출 동안만 빌린다.
inline constexpr std::array<std::int32_t,8> kMetaBattleSeatOrder{4,0,6,2,7,3,5,1}; //00540b68의 읽기 전용8자리 순서이며 원문 writer는 없다.
struct NativeMetaSpriteDrawGlobals { // 출력 시점의 같은 Client 전역 주소다.
    ClientChallengeGlobals& challenge; // 현재 CS 기록과 Image 할당 토큰 소유자다.
    const std::uint32_t& normalColor;const std::uint32_t& ownColor;const std::uint32_t& textColor; //005bec2c/59afd8/5a4058의 현재 문맥 값이다.
};
struct NativeMetaSpriteDrawHooks { // 도달한 실제 하위만 해당 호출 지점에서 요구한다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> rigImage; //QA ProcessForm의 실제 Rig+78 Image이며 복사하지 않는다.
    std::function<std::uint32_t()> type; //004ac550의 현재 객체 TYPE 선택이다.
    std::function<void(std::uint32_t,std::uint32_t,ScreenPoint,std::int32_t,std::int32_t)> shape; //같은 TYPE/SHP의 plain VFX 배율 출력이다.
    std::function<void(std::span<const std::uint8_t>,ScreenPoint,std::int32_t,std::int32_t)> image; //같은 CS가 소유한 동적 shape의 plain VFX 출력이다.
    std::function<std::int32_t(std::uint32_t,std::uint8_t)> firstFrame; //현재 TYPE의 B/C 첫 프레임 DWORD를 반환한다.
    std::function<std::array<float,2>(std::uint32_t,std::int32_t)> muzzle; //00419850의 frameCheck 뒤 현재 signed 포구 WORD 두 개다.
    std::function<std::string(std::string_view)> translate; //현재 Xlat의 버전 문자열을 글꼴 바이트로 변환한다.
    std::function<std::array<std::int32_t,2>(std::string_view,const std::array<std::uint32_t,8>&)> measure; //004a3df0의 실제 같은 글꼴 문맥/길이-1 측정이다.
    std::function<void(std::string_view,ScreenPoint,const std::array<std::uint32_t,8>&)> text; //004a3f70의 현재 통합 화면 글자 출력이다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //Metadisplay.cpp의 복귀형 진단이다.
};
void DrawNativeMetaSprite(std::span<const std::uint8_t> object,ScreenPoint point,NativeMetaSpriteDrawGlobals globals,const NativeMetaSpriteDrawHooks& hooks); //00478d10 전체를 실제+a4 위치에서 호출한다. 별도 목록/그림자/소유자 색표를 추가하지 않는다.
}
