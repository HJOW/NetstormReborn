// 최초 원문55행의 전체 표·호출 순서를 실제 미션 목록 처리기/TotalMade와 비교한다.
#include "DudeSpawnSupport.h"
#include "client/MissionTechnology.h"

using namespace netstorm::client;
namespace {
// TSV의 NUL 포함 바이트를 대체 문자 없이 복원한다.
std::string TechnologyBytes(std::string_view encoded) {
    if (encoded=="-") return {};std::string text;
    // 두16진수 글자를 원본 한 바이트로 돌린다.
    for (std::size_t index=0;index<encoded.size();index+=2) text+=static_cast<char>(std::stoul(std::string(encoded.substr(index,2)),nullptr,16));return text;
}
// 실제255칸 쓰기와 경계 두 칸을 관측하는 독립 초기 표다. 기대값은 fixture에서만 읽는다.
struct TechnologyScene {
    const RiftTypeTable& types; // 보호 타입의 실제 이름/번호 표다.
    SquidPostPopState counts; // TotalMade가 요구하는 같은 장부이며 허용 쓰기로 변하지 않아야 한다.
    std::array<std::uint32_t,257> table{}; // 앞 관측 칸+물리256칸이다. 원본 미등록-1 쓰기도 격리한다.
    RawTotalMade total; // 유효 쓰기는 실제255칸/개별 쓰기 몸체를 사용한다.
    std::string text,changed,trace; // 현재 Config 문자열 입력/변경 입력과 실제 효과 순서다.
    std::size_t lookups{}; // 각 항목마다 발생한 현재 Config 재조회 횟수다.
    // 실제 번호별 다른 초기DWORD를 공급하고 본문 결과를 미리 만들지 않는다.
    TechnologyScene(const RiftTypeTable& source,const std::vector<std::string>& row)
        :types(source),total(OriginalEdition::Patch1078,source.Types(),counts,std::span(table).subspan(1)),text(TechnologyBytes(row[0])),changed(TechnologyBytes(row[1])) {
        const auto seed=Number(row[2]);
        // 앞/끝 경계도 서로 다른 입력 값으로 유지한다.
        for (std::size_t index=0;index<table.size();++index) table[index]=(static_cast<std::uint32_t>(index)*7U)^seed;
    }
    // 호출 효과만 기록하며 결과 표는 실제 TotalMade 또는 격리된 잘못된 주소 한 칸이 쓴다.
    void Add(std::string value) { if (!trace.empty()) trace+='|';trace+=value; }
    // 원본 목록 처리와 실제 표를 같은 현재 Config 입력으로 실행한다.
    void Run() {
        MissionTechnologyHooks hooks;hooks.item=[&](std::int32_t index) { Add("G,"+std::to_string(index));const auto& current=!changed.empty() && lookups ? changed : text;++lookups;const auto visible=std::string_view(current).substr(0,current.find('\0'));return ConfigListItem(visible,index); };
        hooks.findType=[&](std::string_view name) { return types.Find(name); };
        hooks.allowAll=[&](std::int32_t value) { Add("A,"+std::to_string(value));total.AllowAll(value); };
        hooks.allow=[&](std::int32_t type,std::int32_t value) {
            Add("T,"+std::to_string(std::bit_cast<std::uint32_t>(type))+','+std::to_string(value));
            if (type==-1) table[0]=value==0 ? 1U : 0U; // 원본의 표 앞 쓰기를 이 검사에서만 격리해 관측한다. 실제 backend는 아래 검사처럼 거부한다.
            else total.Allow(type,value);
        };
        ApplyMissionTechnology(hooks);
    }
};
}

// 실제 항목 분할/이름 검색/전체257DWORD와 호출 순서를 제품 출력으로 다시 계산하지 않는다.
TEST_CASE(MissionTechnology_Native55) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto rows=LoadFixture(NETSTORM_MISSION_TECHNOLOGY_FIXTURE).rows;CHECK(rows.size()==55);
    // 각 독립 입력은 새 장부/표/현재 목록을 갖는다.
    for (const auto& row:rows) {
        CHECK(row.size()==5);TechnologyScene scene(assets.game.TypeTable(),row);const auto before=scene.counts;scene.Run();std::vector<std::uint8_t> bytes;
        // 호스트 정렬에 의존하지 않고 전체DWORD를 원본 little endian으로 직렬화한다.
        for (const auto value:scene.table) for (std::size_t index=0;index<4;++index) bytes.push_back(static_cast<std::uint8_t>(value>>(index*8)));
        CHECK(Hex(bytes)==row[3] && scene.trace==row[4]);CHECK(scene.counts.localCounts==before.localCounts && scene.counts.localSecondaryCounts==before.localSecondaryCounts && scene.counts.globalCounts==before.globalCounts);
    }
}

// 보호 초기 목록을 같은 Script/타입/표에 적용하고 미등록 이름의 실제 물리 저장소 거부도 확인한다.
TEST_CASE(MissionTechnology_ProtectedElevenAndInvalidStorage) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });
    config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}\\{local.1}.{currentLanguage}\"\n[END]\n");
    const std::array<std::string_view,11> names{"Tutorial1","Tutorial2","Tutorial3","Tutorial4","Tutorial5","Tutorial6","TheWarBegins","MasterOfWhirligigs","DemoStormRages","GuardMyBack","DissolvedAlliance"}; // 지정11개 원본 미션이며 일반/데모5개는 목록이 없다.
    // 매 미션은 소멸 뒤의 실제 AllowAll1과 같은 초기 허용 표로 시작한다.
    for (std::size_t index=0;index<names.size();++index) {
        MissionScript script(config,names[index]);CHECK(script.Loaded());SquidPostPopState counts;std::array<std::uint32_t,256> table{};table.back()=0x7391U;RawTotalMade total(OriginalEdition::Patch1078,assets.game.TypeTable().Types(),counts,table);
        ApplyMissionTechnology(script,assets.game.TypeTable(),total);CHECK(table.back()==0x7391U);const auto& types=assets.game.TypeTable();
        CHECK(total.Allowed(types.Find("windVortex")));CHECK(total.Allowed(types.Find("rainCannon"))==(index>=6));CHECK(total.Allowed(types.Find("dais"))==(index==4 || index>=6));
        if (index==0) CHECK(!total.Allowed(types.Find("sunArcher")));if (index==1) CHECK(total.Allowed(types.Find("sunArcher")));
        if (index==0) {
            const auto before=table;script.Set("techAllowed","missingType");CHECK(Throws([&] { ApplyMissionTechnology(script,types,total); }));CHECK(table==before);
        }
    }
}
