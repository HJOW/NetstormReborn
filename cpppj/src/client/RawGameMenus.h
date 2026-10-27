// 원본10.78 Game/Edit/View/Options/Players 부모를 같은 controlled 메뉴에 작성한다.
#pragma once
#include "client/RawGameMenuBar.h"
#include <array>
#include <span>
#include <variant>

namespace netstorm::client {
// 숫자DWORD와 NUL 종료 원문 문자열 인자를 구별한다. 실제 고리는 GUI 바인더가 공급한다.
using GameMenuParameter=std::variant<std::uint32_t,std::string>;
// 상위 메뉴가 실제 원문 시점에 조회하는 게임·설정·GUI 의존이다.
struct GameMenusHooks {
    std::function<std::uint32_t(std::uint32_t)> global; // 주소로 지정한 원본 전역의 현재DWORD/BYTE다. 별도 캐시 모드로 대체하지 않는다.
    std::function<std::int32_t(std::string_view)> config; //00441090의 현재 GetInt다.
    std::function<void(std::string_view,std::int32_t&)> readInt; //00441270의 누락시기존값을보존하는ReadInt다.
    std::function<std::string(std::string_view)> translate; // 원본 바이트 번역이다.
    std::function<std::string(std::string_view,std::int32_t)> format; // Players의 현재 번역된 고정%d 서식이다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> player; // 같은Player9×180BYTE의현재몸체다.
    std::function<std::uint32_t()> editingMission,demo,loggedIn,ritual,otherRitual; //004639e0/63a20/490970/448fd0/4490d0(0)의현재반환값이다.
    std::function<std::uint32_t(std::uint32_t)> priest,priestBusy,captured; //4921f0(owner)/492090(sid)/4922e0(owner)의실제조회다.
    std::function<float(std::uint32_t)> money; // 실제분산SP0040ec50이며PlayerKnown일때만호출한다.
    std::function<float()> templeCost; // 현재00541244 TYPE+c4의실제비용이다.
    std::function<void(std::uint32_t)> refreshPriest; //00495060(owner)의실제같은사제참여상태를갱신한다.
    std::function<std::int32_t(std::uint32_t)> rank,level; //41c5b0(owner-1)/42b210(Player+6c)의현재값이다.
    std::function<std::string(std::int32_t)> rankPicture; //42f480의현재랭크아이콘원문바이트다.
    std::function<std::uint32_t(std::uint32_t)> playerMenu; // 같은현재RawPlayerMenu::Open(subscriber)다.
    std::function<void()> vertical,fancy,separator; // 실제controlled생성/구분선이다.
    std::function<void(std::string_view)> title; // fancy Player List 제목이다.
    std::function<std::uint32_t()> take,current; // 실제controlled꺼내기/현재메뉴다.
    std::function<void(std::string_view,std::uint32_t,const GameMenuParameter&,std::uint32_t,std::uint32_t)> item; // label/code/parameter/include/enabled이며force0이다.
    std::function<void(std::uint32_t,std::string_view,std::uint32_t,const GameMenuParameter&,std::uint32_t)> checked; // check/label/code/parameter/enabled이며include1이다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t,std::uint32_t)> submenu; //label/token/enabled/checked다.
    std::function<const GumpCreateNode*(std::uint32_t)> node; // fancy배치의현재기본폭/높이를읽는다.
    std::function<void(std::int32_t,std::int32_t,std::uint32_t)> attachCurrent; // 실제가운데좌표/현재루트로controlled메뉴를부착한다.
    std::span<const std::array<std::uint32_t,2>> compatibilityResolutions; // Windows10/11 호환 Client만 원문 세 행 뒤에 추가한다. 기본 빈 목록은 원문 동작/조회 순서를 유지한다.
};
// 큰 게임helper는 명시경계이고 메뉴부모의조건·조회순서·문구·명령은원본몸체다.
class RawGameMenus {
public:
    explicit RawGameMenus(GameMenusHooks hooks); // 같은현재게임/GUI의실제의존을빌린다.
    std::uint32_t Game() const; //004cbb80 전체 Game 부모다.
    std::uint32_t Edit() const; //004cb9c0 전체 Edit 부모다.
    std::uint32_t View() const; //004ca4d0 전체 View 부모다.
    std::uint32_t Options() const; //004ca850 전체 Options 부모다.
    std::uint32_t Players(bool fancy=false) const; //004cbf10 전체 목록이며각항목은기존RawPlayerMenu다.
private:
    std::uint32_t Global(std::uint32_t address) const; // 원문조회위치의필수현재전역이다.
    std::uint32_t PlayerWord(std::uint32_t owner,std::size_t offset) const; // 실제180BYTE몸체의비정렬DWORD다.
    std::string Text(std::string_view key) const; // 현재원문번역을다음효과보다먼저완료한다.
    void Item(std::string_view key,std::uint32_t commandAddress,GameMenuParameter parameter=0U,std::uint32_t include=1,std::uint32_t enabled=1) const; //번역후현재명령을읽는다.
    void Check(std::uint32_t check,std::string_view text,std::uint32_t commandAddress,GameMenuParameter parameter=0U,std::uint32_t enabled=1) const; //번역완료한현재행을쓴다.
    void Toggle(std::string_view key,std::string_view label,std::uint32_t enabledAddress=0) const; //선택문자열을먼저정하고번역뒤현재체크값을재조회한다. 비영주소는번역뒤장치활성을읽는다.
    std::uint32_t Number(std::uint32_t owner) const; //48f550의현재메타맵진단과BYTE결과다.
    GameMenusHooks hooks_; // 생성후에도항상같은현재값/GUI를조회한다.
};
// 문자열인자만같은ScriptStringRing에보관하고작성은기존공통처리기를그대로사용한다.
void BindGameMenusControls(GameMenusHooks& hooks,const MenuGumpWidgets& widgets,
    const MenuGumpControls& controls,const MenuGump& menus,
    std::function<std::uint32_t(std::string_view)> storeText);
}
