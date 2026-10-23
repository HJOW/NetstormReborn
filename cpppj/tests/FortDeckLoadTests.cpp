// Deck부모의독립PE관측1020행과별도cleanup/선택미연결장치를대조한다.
#include "TestSupport.h"
#include "o/RawFortDeckLoad.h"
#include <array>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace netstorm::o;
namespace {
// 빈필드도보존하는fixture의정확한구분자분리다.
std::vector<std::string> DeckFields(std::string_view text,char delimiter) {
    std::vector<std::string> result;std::size_t start{}; //다음필드의첫문자다.
    for (;;) { const auto at=text.find(delimiter,start);result.emplace_back(text.substr(start,at==text.npos ? text.npos : at-start));if (at==text.npos) return result;start=at+1; } //원문필드를재해석없이분리한다.
}
// 비정렬DWORD를원본little endian으로읽는다.
std::uint32_t DeckU(std::span<const std::uint8_t> raw,std::size_t offset) { std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(raw[offset+byte])<<(byte*8);return value; } //네BYTE만조합한다.
// 명시입력의한DWORD만쓴다.
void DeckPut(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) { for (std::size_t byte=0;byte<4;++byte) raw[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); } //이웃필드는보존한다.
// 지정하위callback에서만발생하는검사전용부분중단이다.
struct DeckStop {};
// 네이티브에공급한외부장치/실제버퍼입력만보관한다.부모분기나출력을미리계산하지않는다.
struct DeckDevices {
    std::array<std::uint32_t,11> values{}; //slot은별도signed이며나머지원시DWORD입력이다.
    std::int32_t slot{}; //고정this의검사입력슬롯이다.
    std::string mutation,trace; //명시callback효과와실제호출trace다.
    std::uint32_t manager{},selected{},depth{},full{},conversion{}; //본문이현재값으로읽을같은장치상태다.
    std::array<std::array<std::uint8_t,16>,2> streams{}; //두실제MemStream필드다.
    std::array<std::array<std::uint8_t,2048>,2> buffers{}; //논리end보다긴원시물리입력이다.
    std::array<std::array<std::uint8_t,255>,2> tables{}; //서로다른현재변환표입력이다.
    std::uint32_t assertions{},appends{}; //1기반진단/appendcallback의명시효과를구분한다.
    // fixture의공급입력을원래배치로구성하며제품기대를생성하지않는다.
    explicit DeckDevices(const std::vector<std::string>& fields) {
        slot=std::stoi(fields[0]);for (std::size_t index=1;index<values.size();++index) values[index]=std::stoul(fields[index]);mutation=fields[11]; //입력열만읽는다.
        manager=values[1];selected=values[3];depth=values[10];full=values[4];conversion=values[9];
        for (std::size_t index=0;index<streams.size();++index) { //두선택반환과서로다른버퍼를원본입력대로준비한다.
            DeckPut(streams[index],0,values[5] ? static_cast<std::uint32_t>(index+1) : 0);DeckPut(streams[index],4,values[7]);DeckPut(streams[index],8,values[6]);DeckPut(streams[index],12,2048);buffers[index].fill(index ? 0xdd : 0xcc);
            std::vector<std::uint8_t> types; //아래목록은제품출력이아닌네이티브에도공급한원시파일입력이다.
            if (values[8]==2) types={0,1,245,246,254,255};else if (values[8]==3) for (std::uint32_t type=0;type<255;++type) types.push_back(static_cast<std::uint8_t>(type));else if (values[8]!=1) types={75}; //255개입력은BYTE개수의상한을관측한다.
            for (const auto position:{std::size_t{0},std::size_t{512}}) { //callback의pos512입력도같은물리버퍼안에놓는다.
                buffers[index][position]=static_cast<std::uint8_t>(types.size());
                for (std::size_t item=0;item<types.size();++item) { const auto at=position+1+item*4;buffers[index][at]=types[item];buffers[index][at+1]=static_cast<std::uint8_t>(item*19+7);buffers[index][at+2]=static_cast<std::uint8_t>(item*37+128);buffers[index][at+3]=static_cast<std::uint8_t>(item*53+255); } //네저장BYTE를독립입력으로쓴다.
            }
        }
        for (std::size_t table=0;table<tables.size();++table) for (std::size_t type=0;type<255;++type) tables[table][type]=type==0 ? 9 : type==1 ? 0 : type==245 ? 5 : static_cast<std::uint8_t>(table ? type*7+9 : type); //0/245/246경계의명시변환입력이다.
    }
    // 네이티브와같은한호출의실제텍스트를순서대로기록한다.
    void Record(std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
    // 명시callback뒤의현재값만바꾼다.부모의후속조건을대신판정하지않는다.
    void Mutate(std::string_view boundary) {
        const auto separator=mutation.find('-');if (separator==std::string::npos) return;const auto trigger=mutation.substr(0,separator),action=mutation.substr(separator+1); //독립효과의경계와대상이다.
        const auto plain=boundary.substr(0,boundary.find_last_not_of("1234567890")+1);if (trigger!=boundary && trigger!=plain) return;
        if (action=="stop") throw DeckStop{};
        if (action.starts_with("manager")) manager=std::stoul(action.substr(7));else if (action.starts_with("conversion")) conversion=std::stoul(action.substr(10));
        else if (action.starts_with("depth")) depth=std::stoul(action.substr(5));else if (action.starts_with("full")) full=std::stoul(action.substr(4));else if (action=="stream1") selected=1;
        else if (action=="dataB") DeckPut(streams[selected],0,2);else if (action.starts_with("pos")) DeckPut(streams[selected],4,std::stoul(action.substr(3)));else if (action.starts_with("end")) DeckPut(streams[selected],8,std::stoul(action.substr(3)));else throw std::logic_error("Deck검사변경입력미해석");
    }
    // 실제부모에원시메모리와네이티브와같은외부응답만연결한다.
    FortDeckLoadHooks Hooks() {
        FortDeckLoadHooks hooks; //필수하위각각은도달한순서에서만사용한다.
        hooks.manager=[this](auto fixed) { CHECK(fixed==slot);return manager; };
        hooks.assertion=[this](auto kind) { Record("A:"+std::to_string(static_cast<std::uint32_t>(kind)));Mutate("assert"+std::to_string(++assertions)); };
        hooks.deck=[this](auto owner) { Record("G:"+std::to_string(owner)+':'+std::to_string(values[2]));Mutate("get");return values[2]; };
        hooks.select=[this](auto current,auto name,auto writable) { CHECK(name=="Deck" && writable==1);Record("S:"+std::to_string(current)+":Deck:1");Mutate("select");return selected+1; };
        hooks.access=[this](auto token,const auto& operation) { if (token<1 || token>2) throw std::out_of_range("Deck검사stream토큰");MemStreamHooks memory;memory.buffer=[this](auto data)->std::span<std::uint8_t> { if (data<1 || data>2) throw std::out_of_range("Deck검사data물리범위");return buffers[data-1]; };operation(streams[token-1],memory); };
        hooks.clear=[this](auto token) { Record("clear:"+std::to_string(token));Mutate("clear"); };
        hooks.open=[this](auto token,auto seed) { Record("open:"+std::to_string(token)+':'+std::to_string(seed));Mutate("open"); };
        hooks.full=[this](auto token) { Record("full:"+std::to_string(token));Mutate("full"); };
        hooks.prepareTypeNames=[this](auto fixed) { Record("P:"+std::to_string(fixed));Mutate("prepare"); };
        hooks.conversionBuffer=[this](auto token)->std::span<const std::uint8_t> { if (token!=11 && token!=22) throw std::out_of_range("Deck검사conversion물리범위");return tables[token==11 ? 0 : 1]; };
        hooks.append=[this](auto token,auto type,auto chance,auto power,auto remaining) { Record("E:"+std::to_string(token)+':'+std::to_string(type)+':'+std::to_string(chance)+':'+std::to_string(power)+':'+std::to_string(remaining));Mutate("append"+std::to_string(++appends)); };
        hooks.releaseConversion=[this](auto token) { Record("X:"+std::to_string(token));Mutate("free"); };
        hooks.refresh=[this](auto owner) { Record("R:"+std::to_string(owner));Mutate("refresh"); };
        hooks.loadingDepthChanged=[this](auto value) { Record("L:"+std::to_string(value)); };return hooks;
    }
};
// 선택되지않은장치누락을성공으로요구하지않고도달한호스트오류만관측한다.
template<class Function> bool DeckRejects(Function function) { try { function(); } catch (const std::exception&) { return true; }return false; }
}

// 고정된최초PE입력/순서/현재필드와명시부분중단·별도cleanup을모두비교한다.
TEST_CASE(FortDeckLoad_IndependentNativeParent) {
    std::ifstream input(NETSTORM_FORT_DECK_LOAD_FIXTURE);CHECK(input.good());std::string line;std::size_t compared{}; //독립fixture행수만최종확인한다.
    while (std::getline(input,line)) { //첫두열만입력이고뒤다섯열은제품전에관측한원문출력이다.
        if (line.empty() || line.starts_with('#')) continue;const auto row=DeckFields(line,'\t');CHECK(row.size()==7);if (row.size()!=7) continue;DeckDevices d(DeckFields(row[1],','));auto hooks=d.Hooks();FortDeckLoadGlobals globals{d.depth,d.conversion,d.full};bool completed=true;
        try { LoadFortDeck(d.slot,globals,hooks); } catch (const DeckStop&) { completed=false; }
        CHECK(completed==(row[2]=="1"));CHECK(d.trace==row[5]);const auto state=DeckFields(row[3],',');CHECK(state.size()==5);
        CHECK(d.manager==std::stoul(state[0]) && d.selected==std::stoul(state[1]) && d.depth==std::stoul(state[2]) && d.full==std::stoul(state[3]) && d.conversion==std::stoul(state[4]));
        const auto streams=DeckFields(row[4],',');CHECK(streams.size()==8);
        for (std::size_t stream=0;stream<2;++stream) for (std::size_t field=0;field<4;++field) CHECK(DeckU(d.streams[stream],field*4)==std::stoul(streams[stream*4+field])); //현재data/pos/end/capacity도비교한다.
        if (!completed) { d.trace.clear();UnwindFortDeckLoad(globals,hooks);CHECK(std::to_string(d.depth)+':'+d.trace==row[6]); }else CHECK(row[6]=="-");++compared; //부분중단깊이와cleanup뒤깊이를같은판정으로합치지않는다.
    }
    CHECK(compared==1020);
}

// FullDeck도Select/Clear/Open을생략하지않고비선택직접읽기/TypeNames는요구하지않는다.
TEST_CASE(FortDeckLoad_SelectedDevicesAndPhysicalBounds) {
    DeckDevices d({"1","11","101","0","1","1","1","0","0","11","0","none"});auto hooks=d.Hooks();hooks.access={};hooks.prepareTypeNames={};hooks.append={};hooks.conversionBuffer={};hooks.releaseConversion={};hooks.refresh={};
    LoadFortDeck(1,{d.depth,d.conversion,d.full},hooks);CHECK(d.depth==0 && d.conversion==11 && d.trace=="L:1;G:1:101;S:11:Deck:1;clear:101;open:101:0;full:101;L:0");
    d.trace.clear();hooks.open={};CHECK(DeckRejects([&] { LoadFortDeck(1,{d.depth,d.conversion,d.full},hooks); }));CHECK(d.depth==1 && d.trace=="L:1;G:1:101;S:11:Deck:1;clear:101");UnwindFortDeckLoad({d.depth,d.conversion,d.full},hooks);CHECK(d.depth==0);
    DeckDevices physical({"1","11","101","0","0","1","1","0","0","11","0","none"});auto bounded=physical.Hooks();bounded.prepareTypeNames=[&](auto) { DeckPut(physical.streams[0],4,2048); };CHECK(DeckRejects([&] { LoadFortDeck(1,{physical.depth,physical.conversion,physical.full},bounded); }));CHECK(physical.depth==1 && DeckU(physical.streams[0],4)==2048); //end가1이어도읽기를미리생략하지않고실제물리끝에서거부한다.
}
