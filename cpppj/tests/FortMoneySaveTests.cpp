// Money 부모·공유난수·float32·현재stream을 실제 원본 x86의 독립 결과와 비교한다.
#include "TestSupport.h"
#include "o/RawFortMoneySave.h"
#include "o/RawMemStream.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::o;
namespace {
constexpr std::array<std::uint32_t,2> kMoneyStreams{0x14000100U,0x14000120U}; // 독립PE의두16BYTE stream토큰이다.
constexpr std::array<std::uint32_t,2> kMoneyBuffers{0x14001000U,0x14001100U}; // 독립PE의두물리64BYTE 버퍼토큰이다.
// 끝의빈필드도남겨 독립입력과실제관측열을정확히나눈다.
std::vector<std::string> MoneyFields(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { // 다음구분자위치를찾아원문그대로복사한다.
        const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1;
    }
}
// 현재raw DWORD를little endian으로읽는다.
std::uint32_t MoneyRead(std::span<const std::uint8_t> bytes,std::size_t at=0) {
    std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value; // 지정한네바이트만합친다.
}
// 명시효과가선택한현재stream필드만바꾼다.
void MoneyPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 인접필드는보존한다.
}
// 실제바이트열을독립fixture와같은소문자16진수로관측한다.
std::string MoneyHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; // 출력형식만정의하며기대값은만들지않는다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15U]; }return result; // 모든가드/필드바이트도함께남긴다.
}
// 원본명시중단과검사실패를구분한다.
struct MoneyStop {};
// 명시Create/진단과같은실제일반stream writer만공급하며Money본체는제품함수를쓴다.
struct MoneyDevices {
    GameRandom random{0}; // 부모/난수소비/진단이공유하는동일생성기다.
    std::int32_t slot{};std::uint32_t manager{},selected{}; // 고정this슬롯과현재manager·명시Create반환번호다.
    std::array<std::array<std::uint8_t,16>,2> fields{}; // 실제writer에빌려주는두현재stream기록이다.
    std::array<std::array<std::uint8_t,64>,2> buffers{}; // 두물리출력과나머지가드바이트다.
    std::map<std::string,std::size_t> calls;std::vector<std::vector<std::string>> effects;std::string trace; // 독립입력효과와실제호출순서다.
    // 실제인자를먼저기록하고선택된외부효과를입력순서대로적용한다.
    void Boundary(std::string event,std::string text) {
        if (!trace.empty()) trace+=';';trace+=text;event+=std::to_string(++calls[event]);
        for (const auto& effect:effects) { // 같은경계의여러명시변경을순서대로적용한다.
            if (effect[0]!=event) continue;const auto value=static_cast<std::uint32_t>(std::stoul(effect[2]));
            if (effect[1]=="stop") { CHECK(value==1);throw MoneyStop{}; }
            if (effect[1]=="random") random.SetState(value);
            else if (effect[1]=="manager") manager=value;
            else {
                const auto target=MoneyFields(effect[1],'.');const auto index=target[0]=="a" ? 0U : 1U;
                const auto offset=target[1]=="data" ? 0U : target[1]=="pos" ? 4U : target[1]=="end" ? 8U : 12U; // 현재명시필드주소다.
                MoneyPut(fields[index],offset,value);
            }
        }
    }
    // 앞11열만명시입력으로읽고뒤의원본결과를장치분기에사용하지않는다.
    void Initialize(const std::vector<std::string>& row) {
        random.SetState(static_cast<std::uint32_t>(std::stoul(row[2])));slot=std::stoi(row[3]);manager=static_cast<std::uint32_t>(std::stoul(row[5]));selected=static_cast<std::uint32_t>(std::stoul(row[6]));
        for (std::size_t index=0;index<fields.size();++index) { // 현재stream과물리가드버퍼를독립입력과같게초기화한다.
            MoneyPut(fields[index],0,kMoneyBuffers[index]);MoneyPut(fields[index],4,static_cast<std::uint32_t>(std::stoul(row[7])));
            MoneyPut(fields[index],8,static_cast<std::uint32_t>(std::stoul(row[8])));MoneyPut(fields[index],12,static_cast<std::uint32_t>(std::stoul(row[9])));
            for (std::size_t byte=0;byte<buffers[index].size();++byte) buffers[index][byte]=static_cast<std::uint8_t>(byte); // 출력밖의바이트보존도검사한다.
        }
        if (row[10]!="-") for (const auto& effect:MoneyFields(row[10],';')) effects.push_back(MoneyFields(effect,'/')); // 명시외부효과만분리한다.
    }
    // 부모manager/Create와일반stream writer를각각실제도달시점에제공한다.
    FortMoneySaveHooks Hooks() {
        FortMoneySaveHooks hooks;
        hooks.manager=[this](auto selectedSlot) { CHECK(selectedSlot==slot);const auto captured=manager;Boundary("manager","manager:"+std::to_string(selectedSlot)+':'+std::to_string(captured));return captured; };
        hooks.create=[this](auto captured,std::string_view name) { CHECK(name=="Money");const auto stream=kMoneyStreams.at(selected);Boundary("create","create:"+std::to_string(captured)+":Money:"+std::to_string(stream)+':'+std::to_string(random.State()));return stream; };
        hooks.write=[this](auto stream,std::span<const std::uint8_t> source) {
            CHECK(source.size()==4);Boundary("write","write:"+std::to_string(stream)+':'+std::to_string(MoneyRead(source))+':'+std::to_string(random.State()));
            const auto index=stream==kMoneyStreams[0] ? 0U : 1U;CHECK(stream==kMoneyStreams[index]);MemStreamHooks memory; // 반환된고정stream의현재기록을빌린다.
            memory.buffer=[this](auto token)->std::span<std::uint8_t> { const auto buffer=token==kMoneyBuffers[0] ? 0U : 1U;CHECK(token==kMoneyBuffers[buffer]);return buffers[buffer]; };
            memory.assertion=[this](auto assertion) { CHECK(assertion==MemStreamAssertion::WriteCapacity);Boundary("assert","assert:capacity:"+std::to_string(random.State())); };
            WriteMemStream(fields[index],source,4,memory); // 기존실제일반writer를직접실행해진단뒤재조회를검사한다.
        };
        return hooks;
    }
};
// 도달한경계누락이원문앞선효과를보존한채명시실패하는지확인한다.
template<class Function> bool MoneyRejects(Function function) { try { function(); } catch (const std::logic_error&) { return true; }return false; }
}

// 실제1010개float비트·공유난수·부모현재manager/Create·일반쓰기와부분중단1105개를대조한다.
TEST_CASE(FortMoneySave_IndependentNativeFloatAndStream) {
    std::ifstream input(NETSTORM_FORT_MONEY_SAVE_FIXTURE);CHECK(input.good());std::string line;std::size_t compared{};
    while (std::getline(input,line)) { // 독립입력과실제PE관측열을구분해한행씩실행한다.
        if (line.empty() || line.starts_with('#')) continue;const auto row=MoneyFields(line,'\t');CHECK(row.size()==21);if (row.size()!=21) continue;
        MoneyDevices devices;devices.Initialize(row);const auto failures=netstorm::test::FailureCount();bool completed=true;const auto hooks=devices.Hooks();
        try {
            if (row[1]=="money") SaveFortMoney(kMoneyStreams.at(devices.selected),devices.random,hooks.write);
            else { CHECK(row[1]=="parent");SnapshotFortMoney(devices.slot,devices.random,hooks); }
        } catch (const MoneyStop&) { completed=false; }
        CHECK(completed==(row[11]=="1"));CHECK(devices.random.State()==std::stoul(row[12]));CHECK(devices.manager==std::stoul(row[13]));
        CHECK(MoneyHex(devices.fields[0])==row[14]);CHECK(MoneyHex(devices.fields[1])==row[15]);CHECK(MoneyHex(devices.buffers[0])==row[16]);CHECK(MoneyHex(devices.buffers[1])==row[17]);
        CHECK((devices.trace.empty() ? "-" : devices.trace)==row[18]);CHECK(completed ? std::stoi(row[19])==0 : std::stoi(row[19])<0);CHECK(std::stoul(row[20])>0);++compared;
        if (netstorm::test::FailureCount()!=failures) std::fprintf(stderr,"Fort Money 입력: %s\n",row[0].c_str());
    }
    CHECK(compared==1105);
}
// 하위누락은기존Create/난수효과를되돌리지않으며manager0과배열밖slot도임의진단하지않는다.
TEST_CASE(FortMoneySave_LazyEffectsAndUnclampedSlot) {
    GameRandom random{1};CHECK(MoneyRejects([&] { SaveFortMoney(77,random,{}); }));CHECK(random.State()==0x10006U); // 독립첫seed1의소비뒤상태다.
    random.SetState(1);FortMoneySaveHooks hooks;CHECK(MoneyRejects([&] { SnapshotFortMoney(-2,random,hooks); }));CHECK(random.State()==1);
    std::size_t managers{},creates{},writes{};hooks.manager=[&](auto slot) { CHECK(slot==-2);++managers;return 0U; };
    CHECK(MoneyRejects([&] { SnapshotFortMoney(-2,random,hooks); }));CHECK(random.State()==1 && managers==1);
    hooks.create=[&](auto manager,std::string_view name) { CHECK(manager==0 && name=="Money");++creates;random.SetState(1);return 77U; };
    CHECK(MoneyRejects([&] { SnapshotFortMoney(-2,random,hooks); }));CHECK(random.State()==0x10006U && creates==1);
    hooks.write=[&](auto stream,std::span<const std::uint8_t> bytes) { CHECK(stream==77 && bytes.size()==4);CHECK(MoneyRead(bytes)==0x43a70000U);++writes; };
    SnapshotFortMoney(-2,random,hooks);CHECK(random.State()==0x10006U && managers==3 && creates==2 && writes==1);
}
// SP분산저장소가공유하는동일난수만전진하고SP초기화/재시드/배열쓰기는수행하지않는다.
TEST_CASE(FortMoneySave_SharedRandomWithoutSpStoreWrites) {
    GameRandom random{1};std::size_t reseeds{};ScrambledSpStore store(random,[&] { ++reseeds;return 0U; },0x87654321U);const auto pools=store.Pools();
    std::array<std::uint8_t,4> saved{};SaveFortMoney(77,random,[&](auto stream,std::span<const std::uint8_t> bytes) { CHECK(stream==77 && bytes.size()==saved.size());std::copy(bytes.begin(),bytes.end(),saved.begin()); });
    CHECK(random.State()==0x10006U);CHECK(MoneyRead(saved)==0x43a70000U);CHECK(reseeds==0 && !store.Initialized() && store.Pools()==pools);
}
