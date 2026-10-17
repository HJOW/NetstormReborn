// 원본 명령의 반환·공급 문구·잔여 문자·전체 공유 선택 목록을 두 정밀도 결과와 대조한다.
#include "RawSceneSupport.h"
#include "o/RawEnergySupply.h"
#include <fstream>
using namespace netstorm::test::rawscene;
namespace {
// 입력 DWORD를 signed로 재해석하기 전에 원래 비트 그대로 읽는다.
std::vector<std::uint32_t> EnergyNumbers(std::string_view text) { std::vector<std::uint32_t> result;for (const auto& value:Split(text,',')) result.push_back(Number(value));return result; }
// '-'는 빈 문자열이고 나머지는 원본 ASCII 바이트의16진수다.
std::string EnergyText(std::string_view text) { if (text=="-") return {};std::string result;for (std::size_t index=0;index<text.size();index+=2) result+=static_cast<char>(std::stoul(std::string(text.substr(index,2)),nullptr,16));return result; }
// 실제 raw 풀/TYPE와 명시 입력의 같은 참조를 실행하는 검사 장면이다.
struct EnergyScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 다른 월드나 단순 객체 모형을 쓰지 않는다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 독립 원문과 같은 현재 TYPE 필드다.
    SquidPostPopList providers,selected;std::array<std::uint32_t,81> alliances{}; // 실제 공유 공급/선택/비대칭 동맹 표다.
    std::int32_t mapLimit{256};EnergySupplyState state; // 수집/공급 몸체가 빌리는 현재 전역이다.
    std::unique_ptr<RawEnergySupply> logic; // 매 입력에서 연결한 실제 제품 계산기다.
    std::array<std::uint32_t,30> selectedStorage{}; // 용량 밖은 원문이 쓰지 않는 인접 바이트를 그대로 보존한다.
    std::vector<std::uint32_t> input;std::string available,required,remainder; // 원문과 같은 명시 문자열 입력이다.
    // 모든 관찰 슬롯을 실제 풀에서 확보한다.
    EnergyScene() { for (std::uint16_t sid=5;sid<=300;++sid) CHECK(pool.Allocate(2)==Sid{sid}); }
    // free/dead 입력은 검사 준비에만 쓰고 제품 할당 계약을 바꾸지 않는다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 기대값을 계산하지 않고 원문 실행기와 동일한 입력을 준비한다.
    void Prepare(std::vector<std::uint32_t> values,std::string initial,std::string demand) {
        CHECK(values.size()==30);input=std::move(values);available=std::move(initial);required=std::move(demand);remainder="sentinel";
        state.special=input[1];state.mission=input[2];state.userCheat=input[3];state.ignore=input[4];state.editor=input[5];state.allied=input[6];state.radiusSquared=std::bit_cast<float>(input[16]);mapLimit=std::bit_cast<std::int32_t>(input[17]);
        providers.entries.resize(input[8]);providers.count=input[8];selected.entries.resize(input[7]);selected.count=input[23];
        // 선택 물리30칸 중 실제 용량만 제품 컨테이너에 공급한다.
        for (std::size_t index=0;index<30;++index) { selectedStorage[index]=index%2==0 ? 0U : static_cast<std::uint32_t>(10+index);if (index<selected.entries.size()) selected.entries[index]=selectedStorage[index]; }
        alliances.fill(0);
        // sourceOwner 행/요청owner 열만 설정하여 반대 방향의 동맹을 합성하지 않는다.
        for (std::uint32_t owner=0;owner<9;++owner) alliances[owner*9+input[9]]=(input[10]>>owner)&1U;
        types[100].footX=std::bit_cast<std::int32_t>(input[12]);types[100].footY=std::bit_cast<std::int32_t>(input[13]);types[100].flags2=input[22];
        // 원문이 읽는 raw 상태·좌표·현재 TYPE 원소/발자국을 독립 입력으로 채운다.
        for (std::uint32_t index=0;index<std::max(1U,input[8]);++index) {
            const Sid sid{static_cast<std::uint16_t>(10+index)};const auto kind=101+index%8;if (index<input[8]) providers.entries[index]=sid.value;auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});
            Put(raw,0,(input[20]>>(index%32))&1 ? 0x501ea0 : 0x501dd0);raw[10]=static_cast<std::uint8_t>(kind);raw[11]=static_cast<std::uint8_t>(input[24]);raw[34]=static_cast<std::uint8_t>(input[21] ? 1+index%3 : input[9]);Put(raw,14,input[25]);Put(raw,18,input[26]);
            types[kind].footX=std::bit_cast<std::int32_t>(input[27]);types[kind].footY=std::bit_cast<std::int32_t>(input[28]);types[kind].theme=input[29]==0 ? static_cast<int>(index%4) : input[29]==1 ? 1 : 6+static_cast<int>(index%2);
        }
        EnergySupplyHooks hooks;hooks.requirement=[this](std::uint32_t type) { CHECK(type==100);return required; };hooks.providerEntry=[](std::uint32_t table) { if (table==0x501ea0) return 0x41c170U;CHECK(table==0x501dd0);return 0x41c0f0U; };
        logic=std::make_unique<RawEnergySupply>(pool,types,providers,selected,alliances,mapLimit,state,std::move(hooks));
    }
    // 실제 제품 API의 반환과 목록을 원문 형식으로 기록한다.
    std::array<std::string,6> Run() {
        std::uint32_t result{};std::string take="-";const auto operation=input[0];const auto x=std::bit_cast<float>(input[14]),y=std::bit_cast<float>(input[15]);
        if (operation==0) result=logic->Match(available,required,&remainder,std::bit_cast<std::int32_t>(input[19]));
        else if (operation==1) logic->Collect(available,x,y,100,input[9],std::bit_cast<std::int32_t>(input[19]));
        else if (operation==2) result=logic->Evaluate(available,x,y,100,input[9]);
        else if (operation==3) result=logic->Covers(Sid{10},{1,2,3,4},x,y,std::bit_cast<float>(input[16]),input[22]);
        else { const auto source=logic->TakeSource();result=source ? 1U : 0U;take=std::to_string(source); }
        // 용량 밖 인접 메모리는 초기 원본 그대로이며 실제 제품의 소유 칸만 갱신한다.
        for (std::size_t index=0;index<selected.entries.size();++index) selectedStorage[index]=selected.entries[index];
        std::array<std::uint8_t,120> bytes{};
        // 전체30DWORD의 little endian 바이트를 대조한다.
        for (std::size_t index=0;index<selectedStorage.size();++index) Put(bytes,index*4,selectedStorage[index]);
        const auto encode=[](const std::string& text) { return text.empty() ? std::string("-") : Hex(std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size())); }; // 원문 ASCII 바이트 직렬화다.
        return {std::to_string(result),encode(available),encode(remainder),std::to_string(selected.count),Hex(bytes),take};
    }
};
}
// 모든 고유 입력과 두 x87 정밀도의 명시 결과를 한 칸도 생략하지 않고 비교한다.
TEST_CASE(EnergySupply_NativeFullResults) {
    EnergyScene scene;std::ifstream stream(NETSTORM_ENERGY_SUPPLY_FIXTURE);CHECK(stream.good());std::string line;std::size_t rows{};
    // 두 번째 정밀도가 '-'일 때만 원문이 확인한 첫 결과와 같다는 뜻이다.
    while (std::getline(stream,line)) {
        if (line.empty() || line[0]=='#') continue;const auto cells=Split(line,'\t');CHECK(cells.size()==15);scene.Prepare(EnergyNumbers(cells[0]),EnergyText(cells[1]),EnergyText(cells[2]));const auto actual=scene.Run();
        // 원본 기본53비트와64비트의 개별 결과를 모두 확인한다.
        for (std::size_t index=0;index<6;++index) { if (actual[index]!=cells[3+index]) std::printf("에너지 행%zu 열%zu 실제%s 원문%s\n",rows,index,actual[index].c_str(),cells[3+index].c_str());CHECK(actual[index]==cells[3+index]);CHECK(cells[9+index]=="-" || cells[9+index]==actual[index]); }
        ++rows;
    }
    CHECK(rows==1058);
}
// 기본 공급0·알 수 없는 가상 표/다른 판본·손상된 요구를 성공으로 바꾸지 않는 계약을 검사한다.
TEST_CASE(EnergySupply_GuardsAndCurrentInputs) {
    EnergyScene scene;EnergySupplyHooks hooks;hooks.providerEntry=[](std::uint32_t) { return 0xdeadbeefU; };hooks.requirement=[](std::uint32_t) { return std::string("w"); };
    std::uint32_t special{};EnergySupplyReads reads;reads.special=[&] { return special; };RawEnergySupply logic(scene.pool,scene.types,scene.providers,scene.selected,scene.alliances,scene.mapLimit,scene.state,hooks,reads);
    std::string output="r",remaining;CHECK(!logic.Match(output,"w",&remaining) && output=="w" && remaining=="r");special=1;scene.state.ignore=1;output="r";CHECK(logic.Match(output,"w",&remaining)==1 && output=="w");
    CHECK(&logic.Pool()==&scene.pool);CHECK(Throws([&] { logic.Match(output,"x"); }));CHECK(Throws([&] { logic.Covers(Sid{0},{},0,0,0,0); }));CHECK(Throws([&] { logic.Covers(Sid{10},{},0,0,0,0); }));
}
// 원본83개 가상 표의+a0 DWORD를 제품 분배 표와 한 항목씩 대조한다.
TEST_CASE(EnergySupply_ActualProviderVtables) {
    const auto fixture=LoadFixture(NETSTORM_ENERGY_PROVIDER_VTABLES_FIXTURE);CHECK(fixture.rows.size()==83);std::size_t supplying{};
    // 기본0과 다섯 실제 공급형 이외의 함수를 임의로 허용하지 않는다.
    for (const auto& row:fixture.rows) { CHECK(row.size()==2);const auto entry=EnergyProviderEntry(Number(row[0]));CHECK(entry==Number(row[1]));if (entry==0x41c170) ++supplying; }
    CHECK(supplying==5);CHECK(Throws([] { static_cast<void>(EnergyProviderEntry(0)); }));
}
