// 메타 NSInfo의 실제 참가자 정렬·루트 패널 배치를 같은 CS/Gump 저장소에 수행한다.
#pragma once
#include "client/HudNsInfoCreate.h"
#include "client/ClientChallengeGlobals.h"
#include "client/ClientNsInfoGlobals.h"

namespace netstorm::client { // 원문의 고정 this와 실제 장기 CS/정렬 표를 복사하지 않는다.
struct HudNsInfoMetaGlobals { // 두 원문 몸체가 현재 위치에서 읽는 실제 전역이다.
    ClientChallengeGlobals& challenge;ClientNsInfoGlobals& info; //같은 CS와005930b0 정렬 표/현재 NSInfo를 소유한다.
    const bool& metamap;const std::uint32_t& known;const std::uint32_t& owner; //같은594fc8/595344/540c70을 빌린다.
};
struct HudNsInfoMetaHooks { // 실제 UI 처리기·현재 색/기술 값만 선택된 위치에서 호출한다.
    std::function<HudNsInfoNode&(std::uint32_t)> root; //같은0xd8 NSInfo 노드다.
    std::function<HudPlayerInfoNode&(std::uint32_t)> player; //같은0xbc PlayerInfo 노드다.
    std::function<HudBattleInfoNode&(std::uint32_t)> battle; //같은0x16c BattleInfo 노드다.
    std::function<GumpCreateNode&(std::uint32_t)> node; //현재 실제 공통 노드/크기/위치를 빌린다.
    std::function<void(std::uint32_t)> push,fitHeight; //가상+18 Push/004b9f20의 실제 높이 맞춤이다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; //각 파생의 실제 가상+20이다.
    std::function<void(std::uint32_t,std::string_view)> replace,parse; //004bc250 모드0과004ba8f0의 실제 문자열 처리다.
    std::function<void(std::uint32_t,std::int32_t)> wrap; //004ba210은같은버퍼의noWrap만쓴다.
    std::function<void(std::uint32_t,std::string_view,const std::array<std::uint32_t,8>&,std::uint32_t)> append; //004ba0f0의길이-1/실제정렬행metadata다.
    std::function<std::uint32_t(std::uint32_t)> read; //현재팔레트색/metadataMode의원문DWORD를읽는다.
    std::function<std::uint32_t(std::uint32_t)> objectOwner; //같은Pool의실제SID+22 BYTE다.추가active필터는없다.
    std::function<std::uint32_t&(std::uint32_t)> textFlags; //같은ScrollingText+d0의현재플래그를빌린다.
    std::function<HudPlayerButtonNode&(std::uint32_t)> readyButton; //+c0 현재색/+c4 체크그림값을쓰는같은특수버튼이다.
    std::function<void(std::uint32_t,bool)> enabled; //실제Button가상+3c의활성화/Redraw다.
    std::function<void(std::uint32_t,std::uint32_t)> value; //실제Button가상+40의값/돌배경/Redraw다.
    std::function<void(std::uint32_t)> requestTop,fitContent; //004ba360(0)/004b9fb0(0,0)의같은실제스크롤/내용맞춤이다.
    std::function<void(std::uint32_t,std::string_view)> appendDefault; //004ba090은현재기본문맥으로문자열/metadata0을추가한다.
    std::function<std::string(std::int32_t)> rankPicture; //0042f480의같은현재icon B/C와전체256BYTE처리다.
    std::function<std::uint8_t(std::span<const std::uint8_t>)> weight; //0042b210의현재TYPE기술합계다.
    std::function<std::string(bool)> header; //번역→zone캡처→현재server조회→256BYTE서식을원문순서로수행한다.
    std::function<std::string(std::string_view)> translate; //현재표시바이트언어번역이며원문숫자행은번역하지않는다.
    std::function<void(std::uint32_t,std::string_view)> helpText,helpWord; //00464ed0/00464f80의실제소유문자열교체다.
};
void RefreshNsInfoRoster(std::uint32_t token,HudNsInfoMetaGlobals globals,const HudNsInfoMetaHooks& hooks); //0047e630전체의39명초기순서·원문쌍교환정렬·이름/문맥/행metadata다.
void LayoutMetaNsInfo(std::uint32_t token,HudNsInfoMetaGlobals globals,const HudNsInfoMetaHooks& hooks); //0047e9d0의일반앞단/관문뒤전체본문이다.실제자식Attach가Player/Battle후속을선택한다.
void LayoutMetaPlayerInfo(std::uint32_t token,HudNsInfoMetaGlobals globals,const HudNsInfoMetaHooks& hooks); //00479fb0의기존일반부착/관문뒤전체메타본문이다.실제이름/버튼/아이콘과최종높이를쓴다.
}
