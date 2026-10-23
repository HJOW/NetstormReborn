// 직접읽기는end로자르지않고호스트실제버퍼범위만거부하며반환stream과Deck을처음값으로유지한다.
#include "o/RawFortDeckLoad.h"
#include <array>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 도달한미연결장치만거부하여앞선Select/초기화/직접읽기의부분효과를보존한다.
template<class Function> const Function& NeedDeckLoad(const Function& function) { if (!function) throw std::logic_error("Fort Deck 로드의 실제 경계 미연결");return function; }
// 같은16BYTEMemStream의DWORD를호스트정렬과무관하게읽는다.
std::uint32_t ReadDeckLoad(std::span<const std::uint8_t> bytes,std::size_t at) {
    if (at>bytes.size() || bytes.size()-at<4) throw std::out_of_range("Fort Deck 현재stream DWORD 물리범위");std::uint32_t value{}; //현재네BYTE조립값이다.
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value; //필요한DWORD만조합한다.
}
// 원본이쓴현재pos한DWORD만대입한다.
void PutDeckLoad(std::span<std::uint8_t> bytes,std::uint32_t value) {
    if (bytes.size()<8) throw std::out_of_range("Fort Deck 현재stream pos 물리범위");
    for (std::size_t byte=0;byte<4;++byte) bytes[4+byte]=static_cast<std::uint8_t>(value>>(byte*8)); //data/end/capacity를변경하지않는다.
}
// depth자체를먼저쓰고같은값만관측소비자에전달한다.
void DeckLoadDepth(const FortDeckLoadGlobals& globals,const FortDeckLoadHooks& hooks,std::uint32_t value) {
    globals.loadingDepth=value;if (hooks.loadingDepthChanged) hooks.loadingDepthChanged(value);
}
// 직접BYTE읽기는논리end를검사하지않는다.실제물리입력이없을때만명시거부한다.
std::uint8_t DeckLoadByte(std::span<const std::uint8_t> bytes,std::uint32_t position) {
    if (position>=bytes.size()) throw std::out_of_range("Fort Deck 직접BYTE의 실제물리범위");return bytes[position];
}
}
// slot1..8이아니면두owner하위에0을전달하며고정this의Select/TypeNames를다른slot으로바꾸지않는다.
void LoadFortDeck(std::int32_t slot,const FortDeckLoadGlobals& globals,const FortDeckLoadHooks& hooks) {
    DeckLoadDepth(globals,hooks,globals.loadingDepth+1U);
    if (!NeedDeckLoad(hooks.manager)(slot)) NeedDeckLoad(hooks.assertion)(FortDeckLoadAssertion::MissingTemplate);
    const auto owner=slot>=1 && slot<=8 ? static_cast<std::uint32_t>(slot) : 0U; //원본this의슬롯을계산한ownerclamp다.
    const auto deck=NeedDeckLoad(hooks.deck)(owner); //이반환값은뒤callback이현재Player포인터를바꾸어도유지한다.
    if (!deck) NeedDeckLoad(hooks.assertion)(FortDeckLoadAssertion::MissingDeck);
    const auto manager=NeedDeckLoad(hooks.manager)(slot); //진단/Deckgetter뒤현재manager를다시읽고Select장치조회보다먼저보관한다.
    const auto stream=NeedDeckLoad(hooks.select)(manager,"Deck",1); //같은원문CALL의반환참조를이후전체루프가유지한다.
    NeedDeckLoad(hooks.clear)(deck);NeedDeckLoad(hooks.open)(deck,0);
    if (globals.fullDeck) NeedDeckLoad(hooks.full)(deck);
    else {
        bool present{}; //Select뒤Clear/Open이바꾼실제현재data/end를같이확인한다.
        NeedDeckLoad(hooks.access)(stream,[&](auto fields,const auto&) { present=ReadDeckLoad(fields,0)!=0 && ReadDeckLoad(fields,8)!=0; });
        if (present) {
            NeedDeckLoad(hooks.prepareTypeNames)(slot);std::uint32_t count{}; //변환표부모복귀뒤같은stream의현재count를읽는다.
            NeedDeckLoad(hooks.access)(stream,[&](auto fields,const auto& memory) {
                const auto position=ReadDeckLoad(fields,4);const auto bytes=NeedDeckLoad(memory.buffer)(ReadDeckLoad(fields,0)); //현재data/pos쌍이다.
                count=DeckLoadByte(bytes,position);PutDeckLoad(fields,position+1U);
            });
            for (std::uint32_t index=0;index<count;++index) { //처음countBYTE를보관하며appendcallback이뒤의현재stream/표를바꿀수있다.
                std::array<std::uint32_t,4> entry{}; //type/chance/power/remaining의현재네BYTE를영확장한인자다.
                NeedDeckLoad(hooks.access)(stream,[&](auto fields,const auto& memory) {
                    const auto position=ReadDeckLoad(fields,4);const auto bytes=NeedDeckLoad(memory.buffer)(ReadDeckLoad(fields,0)); //원본EAX/ECX가캡처한한항목의pos/data다.
                    entry[0]=DeckLoadByte(bytes,position);PutDeckLoad(fields,position+1U);
                    if (entry[0]<0xf6U && globals.conversion) { const auto table=NeedDeckLoad(hooks.conversionBuffer)(globals.conversion);entry[0]=DeckLoadByte(table,entry[0]); } //246이상은변환표를읽지않는다.
                    for (std::size_t field=1;field<entry.size();++field) { entry[field]=DeckLoadByte(bytes,position+static_cast<std::uint32_t>(field));PutDeckLoad(fields,position+static_cast<std::uint32_t>(field)+1U); } //같은처음data/pos에서나머지세BYTE를읽는다.
                });
                if (entry[0]) NeedDeckLoad(hooks.append)(deck,entry[0],entry[1],entry[2],entry[3]);
            }
            if (globals.conversion) NeedDeckLoad(hooks.releaseConversion)(globals.conversion);
            globals.conversion=0;NeedDeckLoad(hooks.refresh)(owner);
        }
    }
    DeckLoadDepth(globals,hooks,globals.loadingDepth-1U);
}
// 명시부분중단과원본예외cleanup호출을분리해같은현재DWORD감소만관측가능하게한다.
void UnwindFortDeckLoad(const FortDeckLoadGlobals& globals,const FortDeckLoadHooks& hooks) { DeckLoadDepth(globals,hooks,globals.loadingDepth-1U); }
}
