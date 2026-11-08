// 메타 섬의 원본4789b0 배율 관문·참가자/전투 메뉴·3×3 좌석 입력을 복원한다.
#pragma once
#include "client/ClientChallengeGlobals.h"
#include "client/InputEvent.h"
#include "client/Screen.h"
#include <array>
#include <optional>
#include <string>

namespace netstorm::client { // 실제 CS와 현재 로컬 참가자 주소를 입력 호출 동안만 빌린다.
struct NativeMetaObjectInputGlobals { //같은 CS와 현재 로컬 참가자 번호를 참조로 빌려 원문 재조회를 보존한다.
    ClientChallengeGlobals& challenge;const std::uint32_t& localOwner; //005467d8/00540c70의 현재 실제 소유자다.
};
struct NativeMetaObjectInputHooks { //도달한 실제 효과만 해당 원문 위치에서 요구한다.
    std::function<void(std::uint32_t,ScreenPoint)> playerMenu; //4785f0의 현재 QA 참가자/같은 실제 controlled 공장이다.
    std::function<void()> begin; //477ae0의 현재 fancy controlled 메뉴 생성이다.
    std::function<std::string()> typeName; //현재 객체 TYPE의49af80 이름/언어·좁은 바이트 결과다.
    std::function<void(std::string_view)> title; //77ee0의 실제 제목이며 입력에 원문 newline이 포함된다.
    std::function<std::optional<std::string>()> helpWord; //54db38의 NULL과 비영 빈 문자열을 구별한다.
    std::function<std::uint32_t(std::uint32_t)> command; //현재 원문 주소 식별자의 명령 DWORD다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t)> item; //77cb0의include/enabled1·force0 행이다.
    std::function<std::uint32_t(std::uint32_t)> descriptionParameter; //현재 CS+battle*124+101의 문자열 매개변수이며 번역 전에 계산한다.
    std::function<std::string(std::string_view)> translate; //Change Description만 현재 Xlat을 통과한다.
    std::function<void(std::uint32_t,ScreenPoint)> reference; //75cf0의 현재 메뉴를 실제 객체/원문좌표에 부착하고 스택을 반납한다.
    std::function<ScreenPoint(float,float)> project; //497220의 현재 snapshot 투영이며 frame0 검사보다 먼저 호출한다.
    std::function<std::array<std::int32_t,4>(std::uint32_t)> frameZero; //현재 TYPE/frame0의 원문 frameCheck 뒤 width/height/hotspotX/Y다.
    std::function<void(std::string_view)> query; //cf960의 실제 Tell QueryLeaveJoinChalBattle이다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> queue; //현재 InputFIFO에5428a4/0/포장값을 예약한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //Metadisplay.cpp1fe의 실제 복귀형 진단이다.
};
bool InputNativeMetaObject(std::uint32_t sid,std::span<const std::uint8_t> object,InputEvent event,NativeMetaObjectInputGlobals globals,const NativeMetaObjectInputHooks& hooks); //4789b0 전체이며 원문이 반환하는0/1을 유지한다. 실제 picking/caller는 별도다.
bool InputNativeMetaZoneObject(std::uint32_t sid,std::span<const std::uint8_t> object,InputEvent event,std::span<const std::uint8_t> zones,const NativeMetaObjectInputHooks& hooks); //479390의 같은 배율 관문·영역 제목/하강·도움말·객체 부착이다.
bool DispatchNativeMetaZoneRequest(InputEvent event,const std::function<void(std::string_view)>& tell); //4d7680의 e8은 signed 영역 번호의 원문 확인 Tell만 예약한다.
std::optional<std::uint32_t> MetaObjectInputCommandValue(std::uint32_t originalAddress); //10.78 PE의 네 명령 DWORD만 변환하며 미등록 주소는 소비자에게 돌려준다.
}
