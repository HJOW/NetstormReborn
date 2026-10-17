// 원본 정렬 직후와 연속 소비 뒤의 전체 목록·진단·DWORD 반환을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawEnergySupply.h"
using namespace netstorm::test::rawscene;
namespace {
// 독립 입력의 DWORD 비트를 원래 순서대로 읽는다.
std::vector<std::uint32_t> SortNumbers(std::string_view text) { std::vector<std::uint32_t> result;for (const auto& value:Split(text,',')) result.push_back(Number(value));return result; }
// 요구 문자열은 NUL도 보존한 원본 바이트다.
std::string SortText(std::string_view text) { std::string result;if (text=="-") return result;for (std::size_t index=0;index<text.size();index+=2) result+=static_cast<char>(std::stoul(std::string(text.substr(index,2)),nullptr,16));return result; }
// 실제 Pool과 같은 선택 저장소에 원문 입력만 공급한다.
struct SortScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false};std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 독립 raw/TYPE 계약이다.
    SquidPostPopList providers,selected;std::array<std::uint32_t,81> alliances{}; // 정렬은 공급 목록이나 동맹을 다시 조회하지 않는다.
    std::int32_t limit{256};EnergySupplyState state; // 같은 실제 모듈 생성 계약의 전역이다.
    std::array<std::uint32_t,30> storage{};std::vector<std::uint32_t> input;std::string demand,diagnostics; // 물리 입력/반복 진단이다.
    std::unique_ptr<RawEnergySupply> logic; // 실제 제품 정렬과 소비 몸체다.
    // free 입력까지 독립적으로 준비할 실제 슬롯을 확보한다.
    SortScene() { for (std::uint16_t sid=5;sid<=50;++sid) CHECK(pool.Allocate(2)==Sid{sid}); }
    // 원문 준비용 raw 접근이며 제품의 할당 검사를 바꾸지 않는다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 전체 입력을 새로 공급하며 정렬 순서는 이 함수에서 계산하지 않는다.
    void Prepare(std::vector<std::uint32_t> row,std::string text) {
        CHECK(row.size()==68);input=std::move(row);demand=std::move(text);diagnostics.clear();selected.entries.resize(input[1]);selected.count=input[2];
        // 원문은 용량 밖 인접30칸도 입력으로 보존한다. 제품에는 실제 소유 용량만 제공한다.
        for (std::size_t index=0;index<30;++index) {
            const auto sid=static_cast<std::uint32_t>(10+index);storage[index]=input[3]>>index&1 ? 0U : input[5]>>index&1 ? 24001U : sid;
            if (index<selected.entries.size()) selected.entries[index]=storage[index];auto raw=Raw(Sid{static_cast<std::uint16_t>(sid)});std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});raw[11]=input[4]>>index&1 ? 1 : 0;Put(raw,14,input[8+index*2]);Put(raw,18,input[9+index*2]);
        }
        EnergySupplyHooks hooks;hooks.providerEntry=[](std::uint32_t)->std::uint32_t { throw std::logic_error("정렬 중 공급 가상 재조회"); };hooks.requirement=[](std::uint32_t)->std::string { throw std::logic_error("정렬 중 TYPE 요구 재조회"); };
        hooks.sortDiagnostic=[this](EnergySortDiagnostic why) { if (!diagnostics.empty()) diagnostics+=',';diagnostics+=why==EnergySortDiagnostic::InvalidSource ? "321" : "336"; };
        logic=std::make_unique<RawEnergySupply>(pool,types,providers,selected,alliances,limit,state,std::move(hooks));
    }
    // 제품이 소유한 칸만 반영하고 실제 전체30DWORD를 직렬화한다.
    std::string Bytes() {
        std::array<std::uint8_t,120> bytes{};
        // 미소유 꼬리에 쓰기가 생기지 않도록 현재 소유 칸만 읽는다.
        for (std::size_t index=0;index<selected.entries.size();++index) storage[index]=selected.entries[index];
        // 모든 원본 DWORD를 little endian으로 저장한다.
        for (std::size_t index=0;index<storage.size();++index) Put(bytes,index*4,storage[index]);return Hex(bytes);
    }
    // 실제 정렬/소비를 실행하고 단계별 저장소와 반환을 기록한다.
    std::array<std::string,5> Run() {
        std::string snapshot="-",taken="-";
        if (input[0]!=1) { logic->SortSources(demand,std::bit_cast<float>(input[6]),std::bit_cast<float>(input[7]));snapshot=std::to_string(selected.count)+':'+Bytes(); }
        if (input[0]==1 || input[0]==2) {
            taken.clear();
            // 첫0 반환 뒤에도 원문과 같은 횟수의 빈 조회를 계속 확인한다.
            for (std::size_t index=0;index<32;++index) { const auto value=logic->TakeSource();if (index) taken+=',';taken+=std::to_string(value ? 1 : 0)+':'+std::to_string(value); }
        }
        return {std::to_string(selected.count),Bytes(),diagnostics.empty() ? "-" : diagnostics,taken,snapshot};
    }
};
}
// 선택 결과만이 아니라 원본이 지운 꼬리·같은 메모리의 연속 소비·반복 진단까지 대조한다.
TEST_CASE(EnergySort_NativeFullStatesAndConsumption) {
    const auto fixture=LoadFixture(NETSTORM_ENERGY_SORT_FIXTURE);SortScene scene;std::size_t count{};
    // 두 번째 정밀도의 생략은 원문이 둘의 전체 출력을 같다고 확인한 행에만 사용한다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==12);scene.Prepare(SortNumbers(row[0]),SortText(row[1]));const auto actual=scene.Run();
        // 기본53비트와64비트의 전체 결과를 각각 확인한다.
        for (std::size_t column=0;column<5;++column) { if (actual[column]!=row[2+column]) std::printf("공급 정렬 행%zu 열%zu 실제%s 원문%s\n",count,column,actual[column].c_str(),row[2+column].c_str());CHECK(actual[column]==row[2+column]);CHECK(row[7+column]=="-" || row[7+column]==actual[column]); }
        ++count;
    }
    CHECK(count==567);
}
// 진단을 공급하지 않은 손상 목록을 성공 정렬로 바꾸지 않는다.
TEST_CASE(EnergySort_MissingDiagnosticRejectsInvalidSource) {
    SortScene scene;const auto fixture=LoadFixture(NETSTORM_ENERGY_SORT_FIXTURE);scene.Prepare(SortNumbers(fixture.rows.front()[0]),"x");scene.selected.entries={24001};scene.selected.count=1;
    EnergySupplyHooks hooks;hooks.providerEntry=EnergyProviderEntry;hooks.requirement=[](std::uint32_t) { return std::string{}; };RawEnergySupply logic(scene.pool,scene.types,scene.providers,scene.selected,scene.alliances,scene.limit,scene.state,std::move(hooks));
    CHECK(Throws([&] { logic.SortSources("x",100,110); }));
}
