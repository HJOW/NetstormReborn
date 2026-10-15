// 실제 원본 후처리 문자열과 보호188TYPE 공급을 구분해 검사한다. 게임/GUI는 실행하지 않는다.
#include "DudeSpawnSupport.h"
#include "client/AiTypeRequiredEnergy.h"
#include "o/RawTypeRequiredEnergy.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 관찰 문자열의 원본 바이트를 UTF-8 재해석 없이 복원한다.
std::string EnergyBytes(std::string_view hex) {
    if (hex.size()%2) throw std::runtime_error("mana 원본 문자열 hex 길이 오류");
    std::string result;
    // 두 자리 hex가 원본 C 문자열의 한 바이트다.
    for (std::size_t offset=0;offset<hex.size();offset+=2)
        result.push_back(static_cast<char>(std::stoul(std::string(hex.substr(offset,2)),nullptr,16)));
    return result;
}
// 원본 후처리에 공급한 입력과 독립 실행 결과다. 기대 문자열을 C++ 규칙으로 계산하지 않는다.
struct EnergyObservation {
    std::int32_t group{},level{},theme{}; // TYPE+9c/+94/+98의 실제 입력이다.
    std::uint32_t flags{}; // TYPE+ec의 실제 입력이며 제외 플래그를 포함한다.
    std::string mana; // 후처리 전 TYPE+a0의 명시 문자열 입력이다.
    std::string expected; // 실제 PE가 쓴 NUL 종료 문자열의 내용이다.
};
// 보존 fixture를 한 번 읽고 숫자/바이트를 변환하여 합성 입력과 보호 자산 검사에 함께 쓴다.
const std::vector<EnergyObservation>& EnergyObservations() {
    static const auto rows=[] {
        std::vector<EnergyObservation> result;
        // 머리글을 제외한 각 고유 입력의 실제 기대값을 보관한다.
        for (const auto& row:LoadFixture(NETSTORM_TYPE_ENERGY_FIXTURE).rows) {
            if (row[0]=="group") continue;
            if (row.size()!=6) throw std::runtime_error("mana 원본 fixture 열 수 오류");
            result.push_back({std::stoi(row[0]),std::stoi(row[2]),std::stoi(row[3]),Number(row[1]),row[4],EnergyBytes(row[5])});
        }
        return result;
    }();
    return rows;
}
// 보호 자산의 현재 입력을 원본 관찰의 동등 조건에 연결한다. 결과 자체는 계산하지 않는다.
const EnergyObservation& EnergyExpected(const RiftTypeRecord& type,std::string_view mana) {
    const auto& rows=EnergyObservations();
    const auto found=std::find_if(rows.begin(),rows.end(),[&](const EnergyObservation& row) {
        return row.group==(type.group==10 ? 10 : 0) && row.flags==(type.flags2&0x200000U) &&
            row.level==type.level && row.theme==type.theme && row.mana==mana;
    });
    if (found==rows.end()) throw std::runtime_error("보호 TYPE에 대응하는 원본 mana 관찰 누락");
    return *found;
}
}
// 음수/98 level·문자표 NUL·제외/group10·명시 문자열의2352개 실제 PE 결과를 대조한다.
TEST_CASE(TypeRequiredEnergy_Native2352) {
    const auto& rows=EnergyObservations();CHECK(rows.size()==2352);
    // AST 문자열 입력은 명시 대체 경계이며 후처리 기대값은 실제 기계어에서만 읽는다.
    for (const auto& row:rows) {
        RiftTypeRecord type;type.group=row.group;type.flags2=row.flags;type.level=row.level;type.theme=row.theme;
        RiftTypeDefinition definition;definition.properties.push_back({"mana",row.mana});
        CHECK(TypeRequiredEnergy(OriginalEdition::Patch1078,type,&definition)==row.expected);
    }
}
// 실제116개 보호 AST와 내부/빈 타입을 같은188TYPE getter로 읽고 각 입력의 원본 결과를 대조한다.
TEST_CASE(TypeRequiredEnergy_Protected188Bindings) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game;
    std::vector<RiftTypeRecord> types(assets.TypeTable().Types().begin(),assets.TypeTable().Types().end());
    const auto getter=netstorm::client::BindAiTypeRequiredEnergy(assets,types);
    std::map<std::size_t,const RiftTypeDefinition*> definitions;
    // 실제 SHP 블록 번호를 키로 쓰므로 별칭/이름 정렬에 의존하지 않는다.
    for (const auto& asset:assets.Types()) definitions.emplace(70+asset.block,&asset.definition);
    CHECK(types.size()==188 && definitions.size()==116);
    // 내부0..69와 빈186/187도 실제 초기 TYPE의 원본 후처리 결과를 비교한다.
    for (std::size_t number=0;number<types.size();++number) {
        const auto entry=definitions.find(number);
        const auto mana=entry==definitions.end() ? std::string_view{} : entry->second->String("mana").value_or("");
        CHECK(getter(static_cast<std::uint32_t>(number))==EnergyExpected(types[number],mana).expected);
    }
    // getter 생성 뒤 같은 TYPE의 변경을 읽으며 초기 복제나 캐시를 참조하지 않는다.
    types[69].group=0;types[69].flags2=0;types[69].level=2;types[69].theme=0;
    CHECK(getter(69)==EnergyExpected(types[69],{}).expected);
    CHECK(Throws([&] { getter(188); }));
    CHECK(Throws([&] { netstorm::client::BindAiTypeRequiredEnergy(assets,std::span<const RiftTypeRecord>(types).first(187)); }));
}
// 원본 assert/비ASCII CRT/표 밖 입력은 정상 관찰과 구분하여 명시적으로 거부한다.
TEST_CASE(TypeRequiredEnergy_BoundariesAndOrderedAssignments) {
    RiftTypeRecord type;type.group=0;type.level=1;type.theme=3;
    auto definition=RiftTypeDefinition::Parse("typename probe { mana = \"S\"; MANA = \"WRTS\"; }");
    CHECK(TypeRequiredEnergy(OriginalEdition::Patch1078,type,&definition)=="wrts");
    auto invalid=RiftTypeDefinition::Parse("typename probe { mana = 1; mana = \"s\"; }");
    CHECK(Throws([&] { TypeRequiredEnergy(OriginalEdition::Patch1078,type,&invalid); }));
    invalid.properties={{"mana",std::string("0123456789")}};
    CHECK(Throws([&] { TypeRequiredEnergy(OriginalEdition::Patch1078,type,&invalid); }));
    invalid.properties={{"mana",std::string("\xc3\xa9")}};
    CHECK(Throws([&] { TypeRequiredEnergy(OriginalEdition::Patch1078,type,&invalid); }));
    CHECK(Throws([&] { TypeRequiredEnergy(OriginalEdition::Cd1072,type,&definition); }));
    type.theme=12;CHECK(Throws([&] { TypeRequiredEnergy(OriginalEdition::Patch1078,type,nullptr); }));
    type.theme=-1;CHECK(Throws([&] { TypeRequiredEnergy(OriginalEdition::Patch1078,type,nullptr); }));
}
