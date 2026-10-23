// 원본004bf190의Deck선택/초기화/fullDeck·저장항목복원/변환표반납순서를보존한다.
#pragma once
#include "o/RawMemStream.h"
#include <string_view>

namespace netstorm::o {
// TypeNames및같은Client가직접공유하는현재전역이다.부모가읽을때값사본을고정하지않는다.
struct FortDeckLoadGlobals {
    std::uint32_t& loadingDepth; //005c89b8이며진입/정상종료/별도cleanup이현재값을직접쓴다.
    std::uint32_t& conversion; //005c89bc의현재255BYTE변환표토큰이다.TypeNames와같은소유자를빌린다.
    const std::uint32_t& fullDeck; //0054db14이며Clear/Open복귀뒤원문위치에서읽는다.
};
// 실제Template.cpp진단행이며복귀하면원래위치에서계속한다.
enum class FortDeckLoadAssertion : std::uint32_t {
    MissingTemplate=0x4d6, //진입깊이증가뒤고정this의현재manager0이다.
    MissingDeck=0x4d8 //Get이반환한Deck0이다.진단으로현재owner/반환값을다시조회하지않는다.
};
// 큰하위몸체만연결하고slotclamp/직접BYTE읽기/변환조건과부모순서는본문이소유한다.
struct FortDeckLoadHooks {
    std::function<std::uint32_t(std::int32_t)> manager; //고정this의현재manager를진단시와Select직전에각각읽는다.
    std::function<void(FortDeckLoadAssertion)> assertion; //실제두진단의선택경계이며없으면도달시거부한다.
    std::function<std::uint32_t(std::uint32_t)> deck; //0044f700(owner)의실제현재Deck반환이다.owner는slot1..8또는0이다.
    std::function<std::uint32_t(std::uint32_t,std::string_view,std::int32_t)> select; //0044ceb0(manager,"Deck",1)의같은반환stream이다.
    std::function<void(std::uint32_t,const std::function<void(std::span<std::uint8_t>,const MemStreamHooks&)>&)> access; //선택한같은16BYTE/버퍼를동기적으로빌리며호출자지역사본을쓰지않는다.
    std::function<void(std::uint32_t)> clear; //0044f230은선택뒤항목/합계/remaining을초기화한다.
    std::function<void(std::uint32_t,std::uint32_t)> open; //0044f250(deck,0)은같은게임난수를소비하고seed/open을쓴다.
    std::function<void(std::uint32_t)> full; //0044fbf0→0044f680의실제FullDeck이다.빈성공으로대체하지않는다.
    std::function<void(std::int32_t)> prepareTypeNames; //004bd800은같은고정this를받는다.별도복원된실제변환표부모를연결한다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> conversionBuffer; //현재토큰의255BYTE물리범위를순수조회한다.이getter에추가게임효과를넣지않는다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> append; //0044f330(deck,type,chance,power,remaining)의실제복원추가다.세저장필드는BYTE를DWORD로영확장한다.
    std::function<void(std::uint32_t)> releaseConversion; //_free에그시점현재pointer를넘긴다.복귀하면부모가현재전역을무조건0으로쓴다.
    std::function<void(std::uint32_t)> refresh; //0044f7c0(owner)은비full·비어있지않은stream경로끝에만실행한다.
    std::function<void(std::uint32_t)> loadingDepthChanged; //실제깊이쓰기를같은월드소비자에동기화하는관측경계다.추가게임효과를넣지않는다.
};
// 본문만실행한다.선택callback/물리중단시이미쓴자료와증가깊이를보존하며자동cleanup을추가하지않는다.
void LoadFortDeck(std::int32_t slot,const FortDeckLoadGlobals& globals,const FortDeckLoadHooks& hooks);
// 004fb860→004bcc50의현재깊이한번감소만복원한다.정상감소한본문에중복적용하지않는다.
void UnwindFortDeckLoad(const FortDeckLoadGlobals& globals,const FortDeckLoadHooks& hooks);
}
