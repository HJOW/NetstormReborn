// 원본 Territory 부모/지역 getter/seed의 독립 실행 결과를 같은 실제 저장 본체와 대조한다.
#include "TestSupport.h"
#include "client/CampaignTemplateStorage.h"
#include "o/RawFortTerritorySave.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
using namespace netstorm::o;
using namespace netstorm::client;
namespace {
// 마지막 빈 필드까지 지정 구분자로 분리한다.
std::vector<std::string> Split(std::string_view source,char separator) {
    std::vector<std::string> result;std::size_t start{};
    for (;;) { const auto end=source.find(separator,start);result.emplace_back(source.substr(start,end==source.npos ? source.npos : end-start));if (end==source.npos) return result;start=end+1; } // 원문의 각 경계를 순서대로 읽는다.
}
// 명시 raw 필드의 little endian 값을 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value; } // 지정 폭만 조합한다.
// 지정 폭의 원시 필드만 변경한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) { for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); } // 주변 필드는 그대로 둔다.
// 실제 BYTE 자료를 독립 관측과 같은 소문자 hex로 변환한다.
std::string Hex(std::span<const std::uint8_t> bytes) { constexpr std::string_view digits="0123456789abcdef";std::string result;for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15U]; }return result; } // BYTE당 두 자리를 기록한다.
// 관측 입력의 hex를 그대로 BYTE로 복원한다.
std::vector<std::uint8_t> Bytes(std::string_view hex) { std::vector<std::uint8_t> result;for (std::size_t at=0;at<hex.size();at+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(hex.substr(at,2)),nullptr,16)));return result; } // 각 입력 두 자리만 해석한다.
// 실제 진단 도달에서 중단하는 검사 경계다. 원본 예외 unwind를 뜻하지 않는다.
struct Stop {};
// 부모 입력에 지정한 외부 장치와 callback 효과만 소유한다.
struct Devices {
    std::int32_t slot{};std::uint32_t manager{};std::array<std::uint32_t,2> currents{}; // 고정 Template 슬롯과 현재 manager/current 두 개다.
    std::array<std::uint32_t,4> globals{}; // special/fort/mine/noRegion의 현재 raw DWORD다.
    std::array<std::array<std::uint8_t,16>,2> streams{};std::array<std::array<std::uint8_t,16384>,2> buffers{}; // 두 실제 stream과 data 변경을 구별하는 물리 자료다.
    std::array<std::uint8_t,120> input{},saved{};std::span<std::uint8_t> source; // getter 입력과 살아 있는 부모 로컬 기록/마지막 관측 사본이다.
    std::map<std::int32_t,std::array<std::int32_t,2>> regions; // 일반/Editor의 명시 지역 반환표다.
    std::map<std::string,std::size_t> calls;std::vector<std::vector<std::string>> effects;std::string trace; // 경계 횟수/입력 효과/실제 외부 trace다.
    // 부모 로컬 배열이 살아 있을 때만 마지막 관측 사본을 보관한다.
    void Capture() { if (!source.empty()) std::copy(source.begin(),source.end(),saved.begin()); }
    // 외부 경계를 기록하고 입력에서 지정한 현재 값만 바꾼다.
    void Boundary(std::string_view kind,std::string text) {
        if (!trace.empty()) trace+=';';trace+=text;const auto event=std::string(kind)+std::to_string(++calls[std::string(kind)]);
        for (const auto& effect:effects) { // 같은 경계의 입력 변경을 작성 순서대로 적용한다.
            if (effect[0]!=event) continue;const auto& target=effect[1];const auto& field=effect[2];const auto value=static_cast<std::uint32_t>(std::stoul(effect[3]));
            if (target=="global") globals[field=="special" ? 0 : field=="fort" ? 1 : field=="mine" ? 2 : 3]=value;
            else if (target=="header") { CHECK(field=="manager");manager=value; }
            else if (target.starts_with("manager")) { CHECK(field=="current");currents.at(std::stoul(target.substr(7))==11 ? 0 : 1)=value; }
            else if (target.starts_with("stream")) Put(streams.at(std::stoul(target.substr(6))),field=="data" ? 0 : field=="position" ? 4 : field=="end" ? 8 : 12,value);
            else if (target=="source") source[std::stoul(field.substr(4))]=static_cast<std::uint8_t>(value);
            else { CHECK(target=="control" && field=="stop" && value==1);Capture();throw Stop{}; }
        }Capture();
    }
    // 명시 getter/지도/seed/생성 장치만 연결하고 부모 판단/쓰기는 제품 본체가 수행한다.
    FortTerritorySaveHooks Hooks() {
        FortTerritorySaveHooks hooks;hooks.load=[this](auto selected,auto output) { source=output;std::copy(input.begin(),input.end(),source.begin());Boundary("get","get:"+std::to_string(selected)); };
        hooks.special=[this] { return globals[0]; };hooks.noRegion=[this] { return std::bit_cast<std::int32_t>(globals[3]); };
        hooks.island=[this](auto owner,auto region) { const auto answer=regions.contains(region) ? regions.at(region)[0] : std::bit_cast<std::int32_t>(globals[3]);Boundary("region","region:"+std::to_string(globals[1])+':'+std::to_string(owner)+':'+std::to_string(region));return answer; };
        hooks.anyIsland=[this](auto region) { const auto answer=regions.contains(region) ? regions.at(region)[1] : std::bit_cast<std::int32_t>(globals[3]);Boundary("any","any:"+std::to_string(region));return answer; };
        hooks.record=[this](auto island) { std::array<std::uint8_t,6> result{};for (std::size_t byte=0;byte<6;++byte) result[byte]=static_cast<std::uint8_t>(island*17+byte*29+7);Boundary("record","record:"+std::to_string(island));return result; }; // 독립 PE에 공급한 외부 지도 반환값이다.
        hooks.seed=[this](auto record,auto index) { record[0]=static_cast<std::uint8_t>((record[0]&0xc0U)|(index&0x3fU));record[1]=static_cast<std::uint8_t>((record[1]&0xf7U)|3U);record[3]=static_cast<std::uint8_t>((index*37+11)%254+1);Boundary("seed","seed:"+std::to_string(index)); }; // 독립 부모 관측의 명시 seed 효과다. 하위 seed는 별도로 검증한다.
        hooks.manager=[this](auto selected) { CHECK(selected==slot);return manager; };hooks.create=[this](auto selected) { Boundary("create","create:"+std::to_string(selected)+":Territory"); };
        hooks.current=[this](auto selected) { CHECK(selected==11 || selected==22);return currents.at(selected==11 ? 0 : 1); };
        hooks.assertion=[this](auto kind) { Boundary("assert","assert:"+std::to_string(kind==FortTerritorySaveAssertion::MissingManager ? 521 : 36)); };
        hooks.access=[this](auto selected,const auto& operation) { MemStreamHooks memory;memory.buffer=[this](auto pointer)->std::span<std::uint8_t> { return buffers.at(pointer); };operation(streams.at(selected),memory);Capture(); };return hooks;
    }
    // 부모 TSV의 앞15열만 장치 입력으로 배치한다.
    void Initialize(const std::vector<std::string>& row) {
        slot=std::stoi(row[1]);manager=std::stoul(row[2]);currents={static_cast<std::uint32_t>(std::stoul(row[3])),static_cast<std::uint32_t>(std::stoul(row[4]))};
        for (std::size_t index=0;index<4;++index) globals[index]=std::stoul(row[5+index]); // 현재 네 전역을 원시 입력대로 놓는다.
        for (std::size_t index=0;index<2;++index) { Put(streams[index],0,static_cast<std::uint32_t>(index));Put(streams[index],4,std::stoul(row[9]));Put(streams[index],8,std::stoul(row[10]));Put(streams[index],12,std::stoul(row[11]));buffers[index].fill(0xcc); } // 두 stream은 동일 cursor와 서로 다른 초기 data를 갖는다.
        if (row[12]!="-") for (const auto& record:Split(row[12],';')) { const auto fields=Split(record,':');regions[std::stoi(fields[0])]={std::stoi(fields[1]),std::stoi(fields[2])}; } // 명시 후보표만 파싱한다.
        const auto bytes=Bytes(row[13]);CHECK(bytes.size()==120);std::copy(bytes.begin(),bytes.end(),input.begin());if (row[14]!="-") for (const auto& record:Split(row[14],';')) effects.push_back(Split(record,'/')); // 기존 입력/순서 효과를 그대로 보관한다.
    }
    // 현재 두 stream의 data/pos/end/capacity를 입력과 같은 순서로 출력한다.
    std::string StreamOutput() const { std::string result;for (const auto& stream:streams) { if (!result.empty()) result+=';';result+=std::to_string(Read(stream,0))+':'+std::to_string(Read(stream,4))+':'+std::to_string(Read(stream,8))+':'+std::to_string(Read(stream,12)); }return result; } // callback 뒤 현재 값을 전부 대조한다.
};
// 물리 손상이나 누락된 실제 장치가 성공으로 바뀌지 않는지 확인한다.
template<class Function> bool Rejects(Function function) { try { function(); } catch (const std::exception&) { return true; }return false; }
}

// 일반/Editor/현재 manager/current와 이미 고정한 stream의 전체 출력/부분 실패를 원문과 대조한다.
TEST_CASE(FortTerritorySave_IndependentNativeParent) {
    std::ifstream input(NETSTORM_FORT_TERRITORY_SAVE_FIXTURE);CHECK(input.good());std::string line;std::size_t compared{};
    while (std::getline(input,line)) { // 앞15열은 입력이고 뒤11열은 실제 PE 관측이다.
        if (line.empty() || line.starts_with('#')) continue;const auto row=Split(line,'\t');CHECK(row.size()==26);if (row.size()!=26) continue;Devices d;d.Initialize(row);bool completed=true;const auto failures=netstorm::test::FailureCount();
        try { SnapshotFortTerritory(d.slot,d.Hooks()); } catch (const Stop&) { completed=false; }
        CHECK(completed==(row[15]=="1"));CHECK(d.manager==std::stoul(row[16]));CHECK(std::to_string(d.currents[0])+':'+std::to_string(d.currents[1])==row[17]);CHECK(std::to_string(d.globals[0])+':'+std::to_string(d.globals[1])+':'+std::to_string(d.globals[2])+':'+std::to_string(d.globals[3])==row[18]);CHECK(d.StreamOutput()==row[19]);
        const auto end=std::max(Read(d.streams[0],8),Read(d.streams[1],8));CHECK(Hex(std::span(d.buffers[0]).first(end))==row[20]);CHECK(Hex(std::span(d.buffers[1]).first(end))==row[21]);CHECK(Hex(d.saved)==row[22]);CHECK(d.trace==row[23]);CHECK(completed ? std::stoi(row[24])==0 : std::stoi(row[24])<0);CHECK(std::stoul(row[25])>0);++compared;
        if (netstorm::test::FailureCount()!=failures) std::fprintf(stderr,"Territory 부모 입력: %s\n",row[0].c_str());
    }CHECK(compared==638);
}
// 실제 getter 본체가 수행한 signed BYTE/owner/mode/첫 존재 일치 결과와 비교한다.
TEST_CASE(FortTerritorySave_IndependentNativeQueries) {
    std::ifstream input(NETSTORM_FORT_TERRITORY_QUERY_FIXTURE);CHECK(input.good());std::string line;std::size_t compared{};
    while (std::getline(input,line)) { // 앞7열은 원문32BYTE 섬 입력이고 뒤4열은 실행 결과다.
        if (line.empty() || line.starts_with('#')) continue;const auto row=Split(line,'\t');CHECK(row.size()==11);if (row.size()!=11) continue;std::array<std::uint8_t,3204> records{};Put(records,0,std::stoul(row[5]));std::size_t index{};
        if (row[6]!="-") for (const auto& record:Split(row[6],';')) { const auto fields=Split(record,':');const auto at=4+index++*32;Put(records,at+4,static_cast<std::uint32_t>(std::stoll(fields[0])));Put(records,at+8,static_cast<std::uint32_t>(std::stoll(fields[1])));Put(records,at+13,static_cast<std::uint32_t>(std::stoll(fields[2])),1);Put(records,at+12,static_cast<std::uint32_t>(std::stoll(fields[3])),1); } // signed 입력을 원시 비트로 바꾸고 필드 사이의 padding/Temple은0으로 둔다.
        const auto answer=row[1]=="normal" ? FindFortTerritoryIsland(records,row[2]!="0",std::stoi(row[3]),std::stoi(row[4])) : FindAnyFortTerritoryIsland(records,std::stoi(row[4]));CHECK(static_cast<std::uint32_t>(answer)==std::stoul(row[7]));CHECK(row[8].empty() && row[9]=="0" && std::stoul(row[10])>0);++compared;
    }CHECK(compared==20162);
}
// 실제 seed의 난수 호출 전/후 보존 필드와 진단 중단 자료를 대조한다.
TEST_CASE(FortTerritorySave_IndependentNativeSeed) {
    std::ifstream input(NETSTORM_FORT_TERRITORY_SEED_FIXTURE);CHECK(input.good());std::string line;std::size_t compared{};
    while (std::getline(input,line)) { // 앞5열은 독립 장치 입력, 뒤5열은 실제 PE 관측이다.
        if (line.empty() || line.starts_with('#')) continue;const auto row=Split(line,'\t');CHECK(row.size()==10);if (row.size()!=10) continue;auto bytes=Bytes(row[2]);CHECK(bytes.size()==6);bool completed=true;std::string trace;
        const auto random=[&](auto limit) { trace="random:"+std::to_string(limit);if (row[4]!="-") for (const auto& effect:Split(row[4],';')) { const auto fields=Split(effect,'/');CHECK(fields[0]=="random1");if (fields[1]=="control") throw Stop{};bytes.at(std::stoul(fields[2].substr(4)))=static_cast<std::uint8_t>(std::stoul(fields[3])); }return static_cast<std::uint32_t>(std::stoul(row[3])); }; // 명시 난수 반환과 raw 효과만 공급한다.
        try { SeedFortTerritoryRecord(std::span<std::uint8_t,6>(bytes.data(),6),std::stoul(row[1]),random); } catch (const Stop&) { completed=false; }
        CHECK(completed==(row[5]=="1"));CHECK(Hex(bytes)==row[6]);CHECK(trace==row[7]);CHECK(completed ? std::stoi(row[8])==0 : std::stoi(row[8])<0);CHECK(std::stoul(row[9])>0);++compared;
    }CHECK(compared==188);
}
// 같은 ChunkMap의 y/x 첫 일치와 signed 섬 BYTE, current 쓰기 참조의 수명을 확인한다.
TEST_CASE(FortTerritorySave_CurrentMapAndStreamLifetime) {
    ChunkMap map(3,3);map.At(1,0).island=2;map.At(1,0).record={1,2,3,4,5,6};map.At(0,1).island=2;map.At(0,1).record={7,8,9,10,11,12};CHECK(FortTerritoryIslandRecord(map,2)==map.At(1,0).record);map.At(1,0).record[2]=77;CHECK(FortTerritoryIslandRecord(map,2)[2]==77);CHECK((FortTerritoryIslandRecord(map,99)==std::array<std::uint8_t,6>{}));map.At(0,0).island=255;map.At(0,0).record={9,8,7,6,5,4};CHECK(FortTerritoryIslandRecord(map,-1)==map.At(0,0).record);
    FortTemplate fort;fort.sections.resize(FortTemplate::kSectionCount);CampaignTemplateStorage storage;storage.Install(1,std::move(fort),"territory.fort");CHECK(Rejects([&] { storage.CurrentWriteStream(1); }));const auto created=storage.CreateSection(1,"Territory");const auto selected=storage.CurrentWriteStream(1);CHECK(selected.manager==created.manager && selected.section==created.section && selected.generation==created.generation);
    std::array<std::uint8_t,28> state{};storage.InitializeState(1,state);const auto stateStream=storage.CurrentWriteStream(1);CHECK(stateStream.section!=created.section);const std::array<std::uint8_t,2> record{7,9};storage.AccessSection(selected,[](auto fields,const auto&) { CHECK(Read(fields,4)==0 && Read(fields,8)==0 && Read(fields,12)==0); });CHECK(Rejects([&] { storage.WriteSection(selected,record); }));CHECK(storage.CurrentSection(1)=="State");storage.WriteSection(stateStream,record);storage.CreateSection(1,"Territory");CHECK(Rejects([&] { storage.WriteSection(selected,record); }));storage.Clear(1);CHECK(Rejects([&] { storage.WriteSection(stateStream,record); })); // 새 마지막 State를 만들 때 빈 Territory의 할당이0으로 압축되므로 참조 유효성과 쓰기 용량을 구별한다.
    std::array<std::uint8_t,6> partial{0xff,0xff,1,2,3,4};CHECK(Rejects([&] { SeedFortTerritoryRecord(partial,5,{}); }));CHECK((partial==std::array<std::uint8_t,6>({0xc5,0xf7,1,2,3,4}))); // 장치 누락 때에도 이미 수행한 첫 두 BYTE 변경을 보존한다.
    FortTemplate loaded;loaded.sections.resize(FortTemplate::kSectionCount);const auto names=FortTemplate::SectionNames();const auto at=static_cast<std::size_t>(std::find(names.begin(),names.end(),"Territory")-names.begin());loaded.sections[at].resize(120);loaded.sections[at].reserve(128);CampaignTemplateStorage fromFile;fromFile.Install(1,std::move(loaded),"loaded.fort");
    fromFile.SelectTerritory(1).position=120;const auto loadedStream=fromFile.CurrentWriteStream(1);fromFile.WriteSection(loadedStream,record);CHECK(fromFile.Template(1).sections[at].size()==122 && fromFile.Template(1).sections[at][120]==7 && fromFile.Territory(1).position==122);
    fromFile.SelectTerritory(1).position=6;fromFile.WriteSection(loadedStream,record);CHECK(fromFile.Template(1).sections[at][6]==7 && fromFile.Template(1).sections[at][7]==9 && fromFile.Territory(1).position==8); // 보관한 참조도 이후 실제 읽기 부모가 진행한 현재 위치에 쓴다.
    FortTemplate nested; nested.sections.resize(FortTemplate::kSectionCount);CampaignTemplateStorage borrowing;borrowing.Install(1,std::move(nested),"borrowed.fort");borrowing.InitializeState(1,state);const auto outer=borrowing.CurrentWriteStream(1);
    borrowing.AccessSection(outer,[&](auto fields,const auto& memory) { WriteMemStream(fields,record,2,memory);CHECK(Read(fields,4)==30);const auto same=borrowing.CurrentWriteStream(1);CHECK(same.section==outer.section && same.generation==outer.generation && Read(fields,4)==30);CHECK(Rejects([&] { borrowing.AccessSection(same,[](auto,const auto&) {}); }));CHECK(Read(fields,4)==30);WriteMemStream(fields,record,2,memory); });CHECK(borrowing.State(1).position==32 && borrowing.State(1).end==32); // 조회는 바깥 위치를 보존하며 중첩 쓰기 거부 뒤에도 같은 빌림에서 계속 쓸 수 있다.
}
