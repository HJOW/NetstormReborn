// 메타 참가자/전투섬 객체 갱신과 실제 Gump 이름표의 원문 처리 순서를 복원한다.
#pragma once
#include "client/ClientChallengeGlobals.h"
#include "client/GumpCreate.h"
#include "o/RawMetaZoneCleanup.h"
#include <string>
#include <string_view>

namespace netstorm::client { // 표시 부모는 실제 CS/Pool/카메라/Gump 장치를 같은 수명에서 빌려야 한다.
struct NativeMetaDisplayGlobals { // 원문 필드의 현재 주소를 빌리며 표시 상태의 독립 사본을 만들지 않는다.
    ClientChallengeGlobals& challenge; //현재 CS 기록과1096의 같은 Client 투영 owner다.
    const std::uint32_t& playerKnown;const std::uint32_t& localOwner; //00595344/00540c70의 현재 자기 상태다.
    const std::uint32_t& metadataMode;const std::uint32_t& screenFlags; //005c85c8/005c78d4의 현재 표시 관문이다.
    const std::uint32_t& capacity;const std::uint32_t& attachParent; //005c847c의 Pool 상한과00563e38의 현재 Gump 부모다.
    const std::uint32_t& ownFont;const std::uint32_t& normalFont; //0059afd8/005bec2c의 두 표시 글꼴을 구별한다.
    const std::uint32_t& noticeFont;const std::uint32_t& textColor; //005b5e58/005c78d0의 추가 안내 글꼴/현재 색이다.
    MetaProjectionView view; //실제 현재 Renderer/viewport/화면 크기를1096 계산에 전달한다.
};
struct NativeMetaDisplayHooks { // 선택된 실제 하위는 필수이며 빈 callback/가짜 노드를 허용하지 않는다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> raw; //같은 Pool의 실제50BYTE를 반환한다. callback 뒤 새 free/활성 필터를 추가하지 않는다.
    std::function<void(std::uint32_t,std::uint32_t)> unpop,setOwner,paintOwner,destroy; //가상+48/004ac220/가상+74/가상+10의 실제 몸체다.
    std::function<void(std::uint32_t,float,float,std::uint32_t)> move; //가상+90의 실제 위치 쓰기/공간 재등록이다.
    std::function<GumpCreateNode&(std::uint32_t)> gump; //CS+d4 토큰의 같은 실제 노드/serial/문맥을 빌린다.
    std::function<void(std::uint32_t)> redraw; //Gump 가상+18의 실제 재그리기다.
    std::function<void(std::uint32_t,std::string_view,std::uint32_t)> replaceText; //004bc250의 텍스트 교체/모드0이다.
    std::function<void(std::uint32_t,std::string_view,const std::array<std::uint32_t,8>&,std::int32_t,std::uint32_t)> appendText; //004ba0f0의 명시 문맥/길이-1/메타값0이다.
    std::function<void(std::uint32_t,std::string_view)> parseName; //004ba8f0이 현재 노드 문맥으로 이름을 처리한다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t)> fit; //004b9fb0의 여백0/0 내용 맞춤이다.
    std::function<void(std::uint32_t,std::int32_t)> scroll; //004ba360의 실제0 스크롤 위치 쓰기다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; //Gump 가상+20의 실제 현재 부모/모드1이다.
    std::function<std::string(std::string_view)> translate; //BATTLEMASTER 안내만 원문 위치에서 현재 언어로 번역한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //Squid/Metadisplay의 실제 복귀형 진단이다.
};
void UpdateMetaParticipant(std::uint32_t participant,std::uint8_t mask,NativeMetaDisplayGlobals globals,const NativeMetaDisplayHooks& hooks); //0047d8c0 전체: 객체 갱신/이름표 내용/8자리 배치와 가시성을 원문 순서로 처리한다.
void UpdateMetaBattle(std::uint32_t battle,NativeMetaDisplayGlobals globals,const NativeMetaDisplayHooks& hooks); //00479c40의 현재 전투섬 owner/HP/특수BYTE10/투영·재등록이다.
void ClearMetaDisplayZones(std::span<std::uint8_t> records,const NativeMetaDisplayHooks& hooks); //00479560의51기록 순회/현재 양수존·비영SID 삭제→SID0이다.
}
