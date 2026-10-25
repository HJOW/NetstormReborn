//제품전에고정한부모230·입력384관측과같은실제Deck·난수·현재전역을대조한다.
#include "TestSupport.h"
#include "o/RawDeckRestore.h"
#include "client/GameMenuActions.h"
#include "client/GameMenuCommands.h"
#include <algorithm>
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { //원시입력과실제관측만보관하며기대출력을계산하지않는다.
using namespace netstorm::o;
using namespace netstorm::client;
//빈마지막열까지구분하여Native열의의미를유지한다.
std::vector<std::string> FullCommandSplit(std::string_view text,char delimiter) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { const auto end=text.find(delimiter,begin);result.emplace_back(text.substr(begin,end==text.npos?text.npos:end-begin));if (end==text.npos) return result;begin=end+1; } //현재열끝뒤에서다음열을읽는다.
}
//부호있는초기입력도하위DWORD비트를보존한다.
std::uint32_t FullCommandNumber(std::string_view text) { return static_cast<std::uint32_t>(std::stoll(std::string(text))); }
//원본과같은전체352BYTE의명시초기입력이다.
std::array<std::uint8_t,352> FullCommandInitial() {
    std::array<std::uint8_t,352> bytes;bytes.fill(0xa7);
    for (std::size_t index=0;index<66;++index) { //비활성tail과5BYTE항목도서로다르게준비한다.
        constexpr std::array<std::uint8_t,4> types{0,71,158,255};constexpr std::array<std::uint8_t,5> powers{255,0,1,128,127}; //Native독립입력의원시항목패턴이다.
        bytes[index*5]=types[index%4];bytes[index*5+1]=static_cast<std::uint8_t>(index%7+1);bytes[index*5+2]=powers[index%5];bytes[index*5+3]=37;bytes[index*5+4]=static_cast<std::uint8_t>(index);
    }
    constexpr std::array<std::uint32_t,5> fields{4,0x12345678,0x23456789,0x3456789a,0x456789ab}; //count·합계·남은수·seed·open의초기입력이다.
    for (std::size_t field=0;field<5;++field) for (std::size_t byte=0;byte<4;++byte) bytes[332+field*4+byte]=static_cast<std::uint8_t>(fields[field]>>(byte*8)); //정확한DWORD폭만쓴다.
    return bytes;
}
//Native의전체메모리hex와같은원시표현을만든다.
std::string FullCommandHex(std::span<const std::uint8_t> bytes) { constexpr char digits[]="0123456789abcdef";std::string result;for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; }return result; } //바이트를생략하지않는다.
struct FullCommandStop {}; //명시하위중단을일반실패나정상반환과구별한다.
}

//실제부모·FullDeck·Append·Entry·공유난수의230관측전체를검사한다.
TEST_CASE(FullDeckCommand_NativeParent230AndPartialStores) {
    std::ifstream file(std::filesystem::path(__FILE__).parent_path()/"fixtures/full-deck-command-x86.tsv");CHECK(bool(file));std::string line;std::size_t rows{},stops{};
    while (std::getline(file,line)) { //두정밀도와현재값변경·중단입력을모두읽는다.
        if (line.empty() || line.front()=='#') continue;if (line.back()=='\r') line.pop_back();const auto row=FullCommandSplit(line,'\t');CHECK(row.size()==8);if (row.size()!=8) continue;const auto input=FullCommandSplit(row[1],',');const auto expected=FullCommandSplit(row[3],',');CHECK(input.size()==7 && expected.size()==5);if (input.size()!=7 || expected.size()!=5) continue;
        auto deck=FullCommandInitial();auto full=FullCommandNumber(input[2]),owner=FullCommandNumber(input[1]),dirty=FullCommandNumber(input[3]);auto count=std::bit_cast<std::int32_t>(FullCommandNumber(input[4]));const auto profile=FullCommandNumber(input[5]);const auto mutation=input[6];const std::uint32_t nugget=2,geyser=4;GameRandom random(FullCommandNumber(input[0]));std::size_t typeCalls{},getCalls{};std::string trace;bool completed=true;
        DeckRestoreHooks hooks;hooks.assertion=[](auto) { throw std::logic_error("정상부모입력의예상밖진단"); };
        hooks.type=[&](auto index) { ++typeCalls;if (!trace.empty()) trace+=';';trace+="T:"+std::to_string(index);if (typeCalls==1) { if (mutation=="type1-stop") throw FullCommandStop{};if (mutation=="type1-full0") full=0;if (mutation=="type1-count0") count=0; }return FullDeckType{profile!=1 || index%2 ? 10 : 9,profile==2 ? 256U : 0U}; };
        try { GetFullDeckCommand(full,owner,dirty,[&](auto selected)->std::span<std::uint8_t> { ++getCalls;CHECK(selected==FullCommandNumber(input[1]) && full==1 && dirty==FullCommandNumber(input[3]));if (mutation=="deck1-stop") throw FullCommandStop{};if (mutation=="deck1-full0") full=0;if (mutation=="deck1-owner8") owner=8;if (mutation=="deck1-dirty1234") dirty=1234;return deck; },random,{count,nugget,geyser},hooks); }catch (const FullCommandStop&) { completed=false;++stops; }
        CHECK(completed==(row[2]=="1") && getCalls==1);CHECK(full==FullCommandNumber(expected[0]) && owner==FullCommandNumber(expected[1]) && dirty==FullCommandNumber(expected[2]) && random.State()==FullCommandNumber(expected[3]) && std::bit_cast<std::uint32_t>(count)==FullCommandNumber(expected[4]));CHECK(FullCommandHex(deck)==row[4] && (trace.empty()?"-":trace)==row[5]);CHECK(row[6].starts_with("F:1;G:") && FullCommandNumber(row[7])>0);++rows; //Native store순서는CPU감사이고C++물리store계측으로설명하지않는다.
    }
    CHECK(rows==230 && stops==4);
}

//실제Guide/Gump/release뒤의Editor·mission관문384관측을그대로비교한다.
TEST_CASE(FullDeckCommand_NativeInput384ModeGateAndRelease) {
    std::ifstream file(std::filesystem::path(__FILE__).parent_path()/"fixtures/full-deck-input-x86.tsv");CHECK(bool(file));std::string line;std::size_t rows{},calls{};
    while (std::getline(file,line)) { //원문입력열과실제호출관측을읽고기대분기를다시계산하지않는다.
        if (line.empty() || line.front()=='#') continue;if (line.back()=='\r') line.pop_back();const auto row=FullCommandSplit(line,'\t');CHECK(row.size()==22);if (row.size()!=22) continue;const auto globals=FullCommandSplit(row[5],':'),event=FullCommandSplit(row[2],':');CHECK(globals.size()==16 && event.size()==3);if (globals.size()!=16 || event.size()!=3) continue;std::uint32_t hide{},redraw{};std::size_t currentCalls{};GameMenuActionHooks hooks;
        hooks.command=[](auto command) { return GameMenuCommandValue(static_cast<std::uint32_t>(command)).value(); };hooks.getFullDeck=[&] { ++currentCalls; };
        GameMenuActionGlobals state{[&](auto address) { if (address==0x5c85a4) return FullCommandNumber(globals[0]);if (address==0x594fa4) return FullCommandNumber(globals[3]);throw std::logic_error("전체Deck입력의다른전역조회"); },hide,redraw};
        if (FullCommandNumber(row[3])==0 && FullCommandNumber(row[4])==0) CHECK(GameMenuActions(state,hooks).Handle(FullCommandNumber(event[0]),FullCommandNumber(event[1]),FullCommandNumber(event[2]))==0U); //앞Guide/Gump소비는기존전체부모가소유하며여기서중복실행하지않는다.
        const auto nativeEnvelope=FullCommandSplit(row[19],';');const auto selected=std::find(nativeEnvelope.begin(),nativeEnvelope.end(),"fullDeck")!=nativeEnvelope.end();CHECK(currentCalls==(selected?1U:0U) && hide==0 && redraw==0 && row[18]=="-");calls+=currentCalls;++rows; //기존9명령분류기의새부모호출관측은envelope열에보존되어있다.
    }
    CHECK(rows==384 && calls==60);
}

//선택하지않은하위장치는요구하지않고선택된실패의이미쓴모드와현재소유자를보존한다.
TEST_CASE(FullDeckCommand_MissingDevicePrefixAndInputShortCircuit) {
    auto deck=FullCommandInitial();auto before=deck;std::uint32_t full=0xffffffff,owner=8,dirty=77,nugget=2,geyser=4;std::int32_t count=0;GameRandom random(123);bool failed{};
    try { GetFullDeckCommand(full,owner,dirty,{},random,{count,nugget,geyser},{}); }catch (const std::logic_error&) { failed=true; }CHECK(failed && full==1 && dirty==77 && owner==8 && deck==before && random.State()==123);
    std::uint32_t hide{},redraw{};std::vector<std::uint32_t> reads;GameMenuActionHooks hooks;hooks.command=[](auto command) { return GameMenuCommandValue(static_cast<std::uint32_t>(command)).value(); };hooks.getFullDeck=[] { throw FullCommandStop{}; };
    GameMenuActions action({[&](auto address) { reads.push_back(address);return 0x80000000U; },hide,redraw},hooks);try { static_cast<void>(action.Handle(0x020000cd,0,0));CHECK(false); }catch (const FullCommandStop&) {}CHECK(reads==std::vector<std::uint32_t>{0x5c85a4});reads.clear();CHECK(action.Handle(0x820000cd,0,0)==0U && reads.empty());
}

//실제명령표와Tell/script경로가같은원문ID를사용하고대소문자만무시한다.
TEST_CASE(FullDeckCommand_ActualGetFullDeckCommandMapping) { CHECK(GameMenuCommandValue(0x542a28)==0x020000cdU && IsGameMenuCommand(0x020000cdU) && GameMenuScriptCode("GetFullDeck")==0x020000cdU && GameMenuScriptCode("GETFULLDECK")==0x020000cdU); }
