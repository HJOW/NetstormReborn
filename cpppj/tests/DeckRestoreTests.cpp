// Open/저장Append/FullDeck과실제하위의독립원문860행을전체Deck/공유난수와대조한다.
#include "TestSupport.h"
#include "o/RawDeckRestore.h"
#include <array>
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace netstorm::o;
namespace {
// 마지막빈필드까지그대로분리한다.
std::vector<std::string> RestoreFields(std::string_view text,char delimiter) { std::vector<std::string> fields;std::size_t start{};for (;;) { const auto next=text.find(delimiter,start);fields.emplace_back(text.substr(start,next==text.npos ? text.npos : next-start));if (next==text.npos) return fields;start=next+1; } } //fixture원시열만읽는다.
// 현재DWORD의네BYTE를정확히조합한다.
std::uint32_t RestoreRead(std::span<const std::uint8_t> bytes,std::size_t at) { std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value; } //이웃필드는섞지않는다.
// 명시입력/효과의한DWORD만쓴다.
void RestoreWrite(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); } //실제raw폭을보존한다.
// 네이티브관측과같은소문자hex를만든다.
std::string RestoreHex(std::span<const std::uint8_t> bytes) { constexpr std::string_view digits="0123456789abcdef";std::string text;for (const auto byte:bytes) { text+=digits[byte>>4];text+=digits[byte&15U]; }return text; } //모든352BYTE를기록한다.
// 명시진단/TYPE경계에서만중단하는검사전용표식이다.
struct RestoreStop {};
// 실제입력Deck과같은GameRandom/현재전역·TYPE만보관한다.
struct RestoreDevices {
    std::array<std::uint32_t,12> input{}; //kind/난수/seed/네인자/count/타입수/profile/너겟·간헐천예외타입의원시DWORD다.
    std::array<std::uint8_t,352> deck; //네이티브와같은현재원시할당이다.
    GameRandom random; //Open/FullDeck가함께소비할한실제게임난수다.
    std::int32_t typeCount{};std::uint32_t excludedA{},excludedB{}; //callback뒤재조회할현재외부전역이다.
    std::array<FullDeckType,260> types; //실제getter가원래순서로읽는원시TYPE입력이다.
    std::string mutation,trace; //명시callback효과와실제경계관측이다.
    std::uint32_t assertionCount{},typeCalls{},currentType{}; //1기반효과선택과그때조회한TYPE다.
    // fixture입력만구성하고제품의기대출력을계산하지않는다.
    explicit RestoreDevices(const std::vector<std::string>& fields) {
        for (std::size_t index=0;index<input.size();++index) input[index]=static_cast<std::uint32_t>(std::stoll(fields[index]));mutation=fields[12];random.SetState(input[1]);typeCount=std::bit_cast<std::int32_t>(input[8]);excludedA=input[10];excludedB=input[11];deck.fill(0xa7); //음수타입수도원시비트를유지한다.
        constexpr std::array<std::uint8_t,4> storedTypes{0,71,158,255};constexpr std::array<std::uint8_t,5> powers{255,0,1,128,127}; //독립네이티브에공급한같은초기항목입력이다.
        for (std::size_t index=0;index<66;++index) { deck[index*5]=storedTypes[index%storedTypes.size()];deck[index*5+1]=static_cast<std::uint8_t>(index%7+1);deck[index*5+2]=powers[index%powers.size()];deck[index*5+3]=37;deck[index*5+4]=static_cast<std::uint8_t>(index); } //tail까지입력그대로둔다.
        RestoreWrite(deck,332,input[7]);RestoreWrite(deck,336,0x12345678);RestoreWrite(deck,340,0x23456789);RestoreWrite(deck,344,0x3456789a);RestoreWrite(deck,348,0x456789ab);
        for (std::size_t index=0;index<types.size();++index) types[index]={input[9]==1 && !(index%2) ? 9 : 10,input[9]==2 ? 256U : 0U}; //FullDeck조건을재계산하지않고원시group/flags입력만만든다.
    }
    // 현재순서대로명시경계를기록한다.
    void Record(std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
    // 하위응답의현재메모리변경만수행하고제품분기를대신하지않는다.
    void Mutate(std::string_view boundary) {
        const auto separator=mutation.find('-');if (separator==std::string::npos || mutation.substr(0,separator)!=boundary) return;const auto action=mutation.substr(separator+1); //해당한번의callback효과다.
        if (action=="stop") throw RestoreStop{};
        if (action.starts_with("count")) { const auto value=static_cast<std::uint32_t>(std::stoul(action.substr(5)));if (boundary.starts_with("type")) typeCount=std::bit_cast<std::int32_t>(value);else RestoreWrite(deck,332,value); }
        else if (action.starts_with("sum")) RestoreWrite(deck,336,std::stoul(action.substr(3)));
        else if (action=="power0") deck[2]=0;else if (action=="chance0") deck[1]=0; //최초중복진단입력은아직Entry주소계산전의명시deck0을바꾼다.
        else if (action=="group9") types[currentType].group=9;else if (action=="flags256") types[currentType].flags2=256;else if (action=="excluded0") excludedA=0;else if (action=="random0") random.SetState(0);else throw std::logic_error("Deckprimitive검사효과미해석");
    }
    // TYPE/진단만외부장치이며RNG/Append/Entry쓰기는제품본문이직접계산한다.
    DeckRestoreHooks Hooks() {
        DeckRestoreHooks hooks;hooks.assertion=[this](auto line) { Record("A:"+std::to_string(static_cast<std::uint32_t>(line)));Mutate("assert"+std::to_string(++assertionCount)); };
        hooks.type=[this](auto index) { if (index>=types.size()) throw std::out_of_range("Deckprimitive검사TYPE물리범위");currentType=index;Record("T:"+std::to_string(index));Mutate("type"+std::to_string(++typeCalls));return types[index]; };return hooks;
    }
};
// selected미연결/물리거부를성공으로바꾸지않았는지관측한다.
template<class Function> bool RestoreRejects(Function function) { try { function(); } catch (const std::exception&) { return true; }return false; }
}

// 제품전에고정한모든정상·진단뒤계속·명시부분중단의메모리를전체비교한다.
TEST_CASE(DeckRestore_IndependentNativePrimitives) {
    std::ifstream fixture(NETSTORM_DECK_RESTORE_FIXTURE);CHECK(fixture.good());std::string line;std::size_t compared{}; //원문행수만최종집계한다.
    while (std::getline(fixture,line)) { //첫두열은입력이고뒤네열은독립PE기대다.
        if (line.empty() || line.starts_with('#')) continue;const auto row=RestoreFields(line,'\t');CHECK(row.size()==6);if (row.size()!=6) continue;RestoreDevices d(RestoreFields(row[1],','));const auto hooks=d.Hooks();bool completed=true;
        try { if (d.input[0]==0) OpenRestoredDeck(d.deck,d.random,d.input[2]);else if (d.input[0]==1) AppendRestoredDeckEntry(d.deck,d.input[3],d.input[4],d.input[5],d.input[6],hooks);else { CHECK(d.input[0]==2);LoadFullDeck(d.deck,d.random,{d.typeCount,d.excludedA,d.excludedB},hooks); } }catch (const RestoreStop&) { completed=false; }
        CHECK(completed==(row[2]=="1"));const auto state=RestoreFields(row[3],',');CHECK(state.size()==4);
        CHECK(d.random.State()==std::stoul(state[0]) && std::bit_cast<std::uint32_t>(d.typeCount)==std::stoul(state[1]) && d.excludedA==std::stoul(state[2]) && d.excludedB==std::stoul(state[3]));CHECK(RestoreHex(d.deck)==row[4]);CHECK((d.trace.empty() ? "-" : d.trace)==row[5]);++compared;
    }
    CHECK(compared==860);
}

// 실제게임난수공유와도달한진단누락/물리거부를원문본문순서에서확인한다.
TEST_CASE(DeckRestore_SelectedEffectsAndPhysicalStorage) {
    std::array<std::uint8_t,352> deck{};GameRandom random(1);const auto before=random.State();OpenRestoredDeck(deck,random,123);CHECK(random.State()==before && RestoreRead(deck,344)==123 && (RestoreRead(deck,348)&1)); //비영seed는난수를소비하지않는다.
    OpenRestoredDeck(deck,random,0);CHECK(random.State()!=before);DeckRestoreHooks hooks;
    AppendRestoredDeckEntry(deck,75,7,0xffffff80U,255,hooks);CHECK(RestoreRead(deck,332)==1 && deck[2]==128); //signedpower음수의정확한DWORD는절단진단없이통과한다.
    CHECK(RestoreRejects([&] { AppendRestoredDeckEntry(deck,256,7,1,255,hooks); }));CHECK(deck[5]==0 && RestoreRead(deck,332)==1); //Entry저장뒤type진단도달시누락을거부하며부분자료를보존한다.
    RestoreWrite(deck,332,70);hooks.assertion=[](auto) {};CHECK(RestoreRejects([&] { AppendRestoredDeckEntry(deck,256,7,1,255,hooks); })); //352BYTE밖진입은기존원본진단을통과한뒤물리범위에서거부한다.
    std::int32_t count=0;std::uint32_t a{},b{};RestoreWrite(deck,332,9);LoadFullDeck(deck,random,{count,a,b},{});CHECK(RestoreRead(deck,332)==0 && RestoreRead(deck,336)==0 && RestoreRead(deck,340)==0); //빈현재타입수는TYPE장치없이세초기화만수행한다.
}
