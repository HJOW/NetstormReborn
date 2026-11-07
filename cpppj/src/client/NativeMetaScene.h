// 메타 화면의 원격 존 배치·객체/세 이름표 생성을 현재 표시 장치에 연결한다.
#pragma once
#include "client/NativeMetaDisplay.h"
#include "client/ClientCameraGlobals.h"

namespace netstorm::client { // 기존 CS/Pool/카메라/Gump를 빌리며 별도 메타 장면 사본을 만들지 않는다.
class RawSurfaceStorage; //현재 TYPE/SHP와 원문 표시 진단의 같은 장기 소유자다.
struct NativeMetaSceneGlobals { //0047dd60이 현재 위치에서 읽는 전역들의 비소유 뷰다.
    NativeMetaPollGlobals poll; //같은39참가자/9전투/51존과 현재 투영·Gump 장치를 빌린다.
    const RawCameraTargetState& target; //현재Renderer+48/+4c의 두 한계이며 현재픽셀 원점과 구별한다.
    const std::uint32_t& geometryMode;const std::uint32_t& glyphType; //005467d4와005412dc의 현재DWORD다.
    const std::uint32_t& black;const std::uint32_t& white;const std::uint32_t& green; //005a4058/005bec2c/005acd20의 실제팔레트색이다.
    std::uint32_t& pending; //005c85e8의 카메라 초기화 예약이며 정상호출뒤0을쓴다.
};
struct NativeMetaSceneHooks { //원문에서 선택된 실제 생성/글자/카메라만 요구한다.
    NativeMetaDisplayHooks display; //1099의 실제Gump/객체 장치와1096 투영을 공유한다.
    std::function<std::int32_t(std::uint32_t)> glyphExtent; //현재TYPE/SHP frame0의 signed폭/16·높이/11 중최댓값이다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> create; //004af530의현재타입/flags2를같은Pool에생성하고실제SID를반환한다.
    std::function<std::uint32_t(std::uint32_t)> defaultFrame,remapFrame; //가상+78/004558c0의기본코드·현재난수선택을순서대로수행한다.
    std::function<std::uint32_t()> createText; //new(a4)→TextGump(0,0,0,0)의실제노드생성이다.
    std::function<std::uint32_t(std::uint32_t)> findSerial; //00464020의같은등록표/serial 조회이며없으면0이다.
    std::function<void(std::uint32_t)> clearText; //004bb240의실제소유조각반납이다.
    std::function<void(std::uint32_t,std::string_view,const std::array<std::uint32_t,8>&,std::int32_t)> appendText; //004bb250의문자열/명시문맥/길이-1이다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t)> fitText; //004bb310의실제여백0/0맞춤이다.
    std::function<void(std::uint32_t,std::uint32_t)> reference; //00464540의같은GumpForm/객체부착과world2000h 설정이다.
    std::function<void()> resetCamera; //004340d0의실제현재카메라/NSInfo 초기화이며임의좌표0으로대체하지않는다.
};
std::int32_t MetaSceneGlyphExtent(RawSurfaceStorage& storage,std::uint32_t type); //원문frameCheck/진단복귀뒤현재SHP의signed픽셀폭/높이를읽는다.
void BuildMetaScene(NativeMetaSceneGlobals globals,const NativeMetaSceneHooks& hooks); //0047dd60 전체의선행갱신·두링계산·50존/세이름표·후행카메라 예약처리다.
}
