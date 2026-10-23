// 실제 PE writer의 전체 출력/현재 재조회와 고정 stream 쓰기 장치를 대조한다.
#include "TestSupport.h"
#include "client/CampaignTemplateStorage.h"
#include "o/RawFortSnapshotBasics.h"
#include <array>
#include <bit>
#include <fstream>
#include <stdexcept>
using namespace netstorm::o;
using namespace netstorm::client;
namespace {
// 독립 TSV의 마지막 빈 필드까지 그대로 분리한다.
std::vector<std::string> Fields(std::string_view source) {
    std::vector<std::string> result;std::size_t start{};
    for (;;) { const auto end=source.find('\t',start);result.emplace_back(source.substr(start,end==source.npos ? source.npos : end-start));if (end==source.npos) return result;start=end+1; } // 각 탭 경계의 원문 필드만 읽는다.
}
// 물리 필드의 원본 DWORD를 읽는다.
std::uint32_t U(std::span<const std::uint8_t> bytes,std::size_t at) { std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value; } // 낮은 네 바이트만 조합한다.
// 물리 필드의 지정 DWORD만 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); } // 인접 필드는 보존한다.
// 독립 관측과 같은 소문자 hex를 만든다.
std::string Hex(std::span<const std::uint8_t> bytes) { constexpr std::string_view digits="0123456789abcdef";std::string text;for (const auto byte:bytes) { text+=digits[byte>>4];text+=digits[byte&15U]; }return text; } // 각 BYTE를 두 자리로 기록한다.
// 선택한 진단 경계에서만 종료하는 검사 전용 예외다.
struct AssertionStop {};
// 독립 PE에 공급한 외부 장치와 같은 현재 저장소만 소유한다.
struct Devices {
    std::int32_t slot{},typeCount{};std::uint32_t manager{},mine{}; // 고정 this 슬롯과 현재 manager/owner/타입 수다.
    std::array<std::uint8_t,32> state{};std::array<std::uint8_t,352> deck{}; // State+17과 실제0x160BYTE Deck 입력이다.
    std::array<std::uint8_t,16> stream{};std::array<std::uint8_t,4096> output{}; // 원본 stream 필드와 초기cc 물리 버퍼다.
    std::string mutation,current{"old"},trace; // 명시 콜백 변경과 현재 선택/실제 호출 trace다.
    // 명시 외부 호출만 순서대로 기록한다.
    void Record(std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
    // 독립 입력의 callback 효과만 적용한다. 부모의 조건/결과를 대신 쓰지 않는다.
    void Mutate(std::string_view boundary) {
        const auto prefix=std::string(boundary)+'-';
        if (mutation==prefix+"manager22") manager=22;else if (mutation==prefix+"mine8") mine=8;
        else if (mutation==prefix+"version0") state[17]=0;else if (mutation==prefix+"version1") state[17]=1;
        else if (mutation==prefix+"typecount2") typeCount=2;else if (mutation==prefix+"typecount0") typeCount=0;
        else if (mutation==prefix+"deckcount2") Put(deck,0x14c,2);else if (mutation==prefix+"deckcount0") Put(deck,0x14c,0);
        else if (mutation==prefix+"capacity64") Put(stream,12,64);
    }
    // 제품 해시/폭별 쓰기가 직접 읽을 외부 이름/State/덱과 stream을 연결한다.
    FortSnapshotBasicsHooks Hooks() {
        FortSnapshotBasicsHooks hooks;hooks.manager=[this](auto selected) { CHECK(selected==slot);return manager; };
        hooks.assertion=[this](auto kind) { const auto line=kind==FortSnapshotAssertion::MissingNamesTemplate ? 586 : kind==FortSnapshotAssertion::NamesNotMine ? 587 : kind==FortSnapshotAssertion::MissingDeckTemplate ? 1220 : 36;
            Record("assert:"+std::to_string(line));Mutate("assert");if (line==36 && mutation=="assert-stop") throw AssertionStop{};
        };
        hooks.create=[this](auto selected,auto name) { Record("create:"+std::to_string(selected)+':'+std::string(name));current="Created";Mutate("create");return 1U; };
        hooks.state=[this](auto selected)->std::span<const std::uint8_t> { Record("state:"+std::to_string(selected));current="State";Mutate("state");return state; };
        hooks.typeName=[this](auto index)->std::string_view {
            Record("name:"+std::to_string(index));Mutate("name"+std::to_string(index));
            static const std::array<std::string,6> names{"","A","abcdE",std::string("\x80\xff" "Az",4),"Netstorm",std::string("Q\0ignored",9)}; // 고정 독립 PE 입력의 signed BYTE/NUL 이름이다.
            return names.at(static_cast<std::size_t>(index)%names.size());
        };
        hooks.refreshDeck=[this](auto owner) { Record("refresh:"+std::to_string(owner));Mutate("refresh"); };
        hooks.getDeck=[this](auto owner) { Record("get:"+std::to_string(owner));Mutate("get");return 2U; };
        hooks.deck=[this](auto token)->std::span<const std::uint8_t> { CHECK(token==2);return deck; };
        hooks.write=[this](auto token,auto bytes) { CHECK(token==1 && bytes.size()==1);Record("write:1:"+Hex(bytes));const auto position=U(stream,4);CHECK(position+bytes.size()<=U(stream,12));output[position]=bytes[0];Put(stream,4,position+1);if (U(stream,8)<position+1) Put(stream,8,position+1);Mutate("write"); };
        hooks.access=[this](auto token,const auto& operation) { CHECK(token==1);MemStreamHooks memory;memory.buffer=[this](auto pointer)->std::span<std::uint8_t> { CHECK(pointer==77);return output; };operation(stream,memory); };return hooks;
    }
};
// 실패한 장치를 정상 쓰기로 세지 않는다.
template<class Function> bool Rejects(Function function) { try { function(); } catch (const std::exception&) { return true; }return false; }
}

// signed 이름 hash/현재 count/인라인 쓰기는 원문이 만든 출력과 비교한다.
TEST_CASE(FortSnapshotBasics_IndependentNative) {
    std::ifstream input(NETSTORM_FORT_SNAPSHOT_BASICS_FIXTURE);CHECK(input.good());std::string line;std::size_t compared{};
    while (std::getline(input,line)) { // 앞12열만 장치 입력이며 뒤12열은 독립 PE 관측이다.
        if (line.empty() || line.starts_with('#')) continue;const auto row=Fields(line);CHECK(row.size()==24);if (row.size()!=24) continue;
        Devices d;d.slot=std::stoi(row[2]);d.manager=std::stoul(row[3]);d.mine=std::stoul(row[4]);d.state[17]=static_cast<std::uint8_t>(std::stoul(row[5]));d.typeCount=std::stoi(row[6]);Put(d.deck,0x14c,std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(std::stoi(row[7]))));
        for (std::size_t index=0;index<66;++index) for (std::size_t byte=0;byte<5;++byte) d.deck[index*5+byte]=static_cast<std::uint8_t>(index*19+byte*37+3); // 전용count 위치 밖의 고정5BYTE 항목 입력이다.
        Put(d.stream,0,77);Put(d.stream,4,std::stoul(row[8]));Put(d.stream,8,std::stoul(row[9]));Put(d.stream,12,std::stoul(row[10]));d.output.fill(0xcc);d.mutation=row[11];bool completed=true;
        try { if (row[1]=="names") SnapshotFortTypeNames(d.slot,d.mine,d.typeCount,d.Hooks());else { CHECK(row[1]=="deck");SnapshotFortDeck(d.slot,d.Hooks()); } } catch (const AssertionStop&) { completed=false; }
        CHECK(completed==(row[12]=="1"));CHECK(d.manager==std::stoul(row[13]));CHECK(d.mine==std::stoul(row[14]));CHECK(d.state[17]==std::stoul(row[15]));CHECK(std::bit_cast<std::uint32_t>(d.typeCount)==std::stoul(row[16]));CHECK(U(d.deck,0x14c)==std::stoul(row[17]));
        CHECK(d.current==row[18]);CHECK(U(d.stream,4)==std::stoul(row[19]));CHECK(U(d.stream,8)==std::stoul(row[20]));CHECK(Hex(std::span(d.output).first(U(d.stream,8)))==row[21]);CHECK(d.trace==row[22]);if (completed) CHECK(row[23]=="0");else CHECK(std::stoi(row[23])<0);++compared; // 중단 관측은 원본 예외 unwind 검증으로 확대하지 않는다.
    }CHECK(compared==2060);
}

// 같은 이름 재생성/manager 반납은 이전 참조를 무효화하며 current 변경은 무효화하지 않는다.
TEST_CASE(FortSnapshotBasics_ExplicitStorageStreamLifetime) {
    FortTemplate fort;fort.sections.resize(FortTemplate::kSectionCount);CampaignTemplateStorage storage;storage.Install(1,std::move(fort),"work.fort");
    std::array<std::uint8_t,28> state{};state[17]=1;storage.InitializeState(1,state);const auto names=storage.CreateSection(1,"TypeNames");storage.SelectState(1);
    const std::array<std::uint8_t,1> count{188};storage.WriteSection(names,count);storage.AccessSection(names,[](auto fields,const auto& memory) { const std::array<std::uint8_t,4> hash{1,2,3,4};WriteMemStream(fields,hash,4,memory); });
    CHECK(storage.CurrentSection(1)=="State");const auto order=storage.SectionOrder(1);CHECK(order.back()=="TypeNames");CHECK(storage.Template(1).sections[names.section-1]==std::vector<std::uint8_t>({188,1,2,3,4}));
    const auto replacement=storage.CreateSection(1,"TypeNames");CHECK(Rejects([&] { storage.WriteSection(names,count); }));storage.Borrow(8,1);storage.WriteSection(replacement,count);storage.Clear(8);storage.WriteSection(replacement,count);
    CHECK(storage.Template(1).sections[replacement.section-1]==std::vector<std::uint8_t>({188,188}));storage.Clear(1);CHECK(Rejects([&] { storage.WriteSection(replacement,count); }));
}

// 호스트 예외에서도 원문이 쓴 논리 끝/부분자료만 보존하며16384BYTE 물리 할당을 파일 자료로 공개하지 않는다.
TEST_CASE(FortSnapshotBasics_StoragePartialWriteAndMissingDevices) {
    FortTemplate fort;fort.sections.resize(FortTemplate::kSectionCount);CampaignTemplateStorage storage;storage.Install(1,std::move(fort),"partial.fort");const auto stream=storage.CreateSection(1,"Deck");
    const std::vector<std::uint8_t> tooLarge(16385,1);CHECK(Rejects([&] { storage.WriteSection(stream,tooLarge); }));CHECK(storage.Template(1).sections[stream.section-1].empty());storage.SectionOrder(1);
    storage.AccessSection(stream,[](auto fields,const auto&) { CHECK(U(fields,4)==0 && U(fields,8)==0); }); // 실패한 일반 쓰기의 물리0패딩을 논리 end로 공개하지 않는다.
    CHECK(Rejects([&] { storage.AccessSection(stream,[](auto fields,const auto& memory) { const std::array<std::uint8_t,2> bytes{7,9};WriteMemStream(fields,bytes,2,memory);throw std::logic_error("명시 쓰기 중단"); }); }));
    CHECK(storage.Template(1).sections[stream.section-1]==std::vector<std::uint8_t>({7,9}));Devices d;d.manager=11;d.mine=0;d.state[17]=0;auto hooks=d.Hooks();hooks.write={};hooks.access={};SnapshotFortTypeNames(0,d.mine,d.typeCount,hooks);CHECK(d.trace=="create:11:TypeNames;state:0");
    d.state[17]=1;CHECK(Rejects([&] { SnapshotFortTypeNames(0,d.mine,d.typeCount,hooks); }));CHECK(Rejects([&] { storage.CreateSection(1,"unknown-section"); }));
}
