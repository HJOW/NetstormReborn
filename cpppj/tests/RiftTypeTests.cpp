// TYPE+44의 현재 사거리를 실제 자산 로딩과 속성 순서로 확인한다.
#include "DudeSpawnSupport.h"
#include "RawSceneSupport.h" // 기대 방향/클러스터 수는 기존 실제 PE metadata fixture에서 읽는다.
#include <filesystem> // 원문 metadata 파일을 현재 검사 소스의 실제 위치에서 읽는다.

namespace { //보호 자산의 실제 정의와 TYPE 기록을 독립 경계에서 대조한다.
using namespace netstorm::o; //제품의 실제 타입/판본을 사용한다.
}

TEST_CASE(RiftTypeDamageRate_AssetsInitialZeroAndDuplicateFloatStore) { // 원문 hpPerSec와 양판본 실제 양수 피해율·대소문자·중복 float32 저장을 대조한다.
    std::size_t checked{}; // 두 판본 실제 자산을 빠짐없이 방문한 수다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 보호 자산의 원래 선언이 기대값이며 제품 키를 재사용하지 않는다.
        const auto& game=netstorm::test::dudespawn::Assets(edition).game;const auto types=game.TypeTable().Types();std::size_t positive{};
        for (const auto& asset:game.Types()) { // 실제 양수 선언도 확인하여 잘못된 키의 전부0 비교를 막는다.
            const auto expected=static_cast<float>(asset.definition.Number("hpPerSec").value_or(0));CHECK(types[70+asset.block].damagePerSecond==expected);if (expected>0) ++positive;++checked;
        }
        CHECK(positive>0 && types[71].damagePerSecond==(edition==OriginalEdition::Patch1078 ? 15.0F:10.0F));std::printf("hpPerSec 판본%d: 실제 자산%zu, 양수%zu, TYPE71 %.9g\n",static_cast<int>(edition),game.Types().size(),positive,types[71].damagePerSecond);
        for (std::size_t type=0;type<70;++type) CHECK(types[type].damagePerSecond==0); // 내장 타입의 원문 미지정 초기0이다.
        const auto empty=RiftTypeDefinition::Parse("typename empty {} A00:default:\"a.gif\"#0;");const auto changed=RiftTypeDefinition::Parse("typename damage { hpPerSec=3.25; HpPeRsEc=-7.5; } A00:default:\"a.gif\"#0;");const auto order=TypeLoadOrder(edition);std::vector<RiftTypeSource> sources;
        for (std::size_t index=0;index<order.size();++index) sources.push_back({order[index],index ? &empty:&changed}); // 첫 자산에만 순서가 있는 중복 속성을 공급한다.
        const RiftTypeTable table(edition,sources);CHECK(std::bit_cast<std::uint32_t>(table.Types()[70].damagePerSecond)==0xc0f00000U && table.Types()[71].damagePerSecond==0);
    }CHECK(checked==217);
}

// 양판본 모든 자산의 사거리와 내장 타입의 초기0을 실제 같은 Table 로더로 확인한다.
TEST_CASE(RiftTypeRange_OriginalAssetsAndBuiltinZero) {
    std::size_t checked{}; //양판본 실제 자산을 빠짐없이 방문한 개수다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { //보호된 두 판본을 읽기 전용으로 조회한다.
        const auto& game=netstorm::test::dudespawn::Assets(edition).game;const auto types=game.TypeTable().Types();
        for (const auto& asset:game.Types()) { //TYPE 순서와 실제 정의의 수치 사거리만 대조한다.
            CHECK(types[70+asset.block].range==static_cast<float>(asset.definition.Number("range").value_or(0)));++checked;
        }
        for (std::size_t type=0;type<70;++type) CHECK(types[type].range==0); //원문 전체 TYPE 초기화의 미지정 사거리다.
    }
    CHECK(checked==217);
}

// 원문의0초기화/마지막 속성 대입과 음수 float 비트를 유지한다.
TEST_CASE(RiftTypeRange_DefaultDuplicateOrderAndFloatBits) {
    const auto empty=RiftTypeDefinition::Parse("typename empty {} A00:default:\"a.gif\"#0;");
    const auto changed=RiftTypeDefinition::Parse("typename rangeType { range=3.25; range=-7.5; } A00:default:\"a.gif\"#0;");
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { //양판본+44의 같은 수치 대입을 확인한다.
        const auto order=TypeLoadOrder(edition);std::vector<RiftTypeSource> sources; //실제 로더가 요구하는 전체 이름 순서를 보존한다.
        for (std::size_t index=0;index<order.size();++index) sources.push_back({order[index],index ? &empty:&changed}); //첫 자산만 명시 중복 사거리 입력이다.
        const RiftTypeTable table(edition,sources);CHECK(std::bit_cast<std::uint32_t>(table.Types()[70].range)==0xc0f00000U && table.Types()[71].range==0.0F);
    }
}


namespace { // 방향/클러스터 기대는 기존 실제 TYPE callback 관측에서 읽는다.
// 원문 callback 입력 텍스트만 복원하고 기대 출력은 별도 metadata 칸에서 읽는다.
std::string TypeAnimationInput(std::string_view hex) {
    std::string text;for (std::size_t offset=0;offset<hex.size();offset+=2) text+=static_cast<char>(std::stoul(std::string(hex.substr(offset,2)),nullptr,16)); // 입력의 각 BYTE를 순서대로 복원한다.
    return text;
}
// 양판본 Table의 실제 초기화/속성 적용을 거쳐 첫 자산의 두 새 DWORD를 원문 metadata와 대조한다.
void CheckTypeAnimationRecord(OriginalEdition edition,std::string_view name) {
    const auto fixture=std::filesystem::path(__FILE__).parent_path()/"fixtures/archertype-x86.tsv";const RiftTypeDefinition empty;std::size_t count{};
    for (const auto& row:netstorm::test::rawscene::LoadFixture(fixture.string().c_str()).rows) { // 각판본 접두1/전체callback21의 기존 관측을 모두 사용한다.
        if (row[0]!=name) continue;CHECK(row.size()==8);RiftTypeDefinition definition;if (row[1]!="prefix") definition=RiftTypeDefinition::Parse(TypeAnimationInput(row[4]));
        const auto order=TypeLoadOrder(edition);std::vector<RiftTypeSource> sources;for (std::size_t index=0;index<order.size();++index) sources.push_back({order[index],index ? &empty:&definition}); // 원본 전체 이름/TYPE 수를 유지하고 첫 자산만 명시 입력으로 바꾼다.
        const RiftTypeTable table(edition,sources);const auto comma=row[5].find(',');CHECK(comma!=std::string::npos);
        CHECK(table.Types()[70].directions==std::stoi(row[5]) && table.Types()[70].frameCount==std::stoi(row[5].substr(comma+1)));
        for (std::size_t type=0;type<70;++type) CHECK(table.Types()[type].directions==0 && table.Types()[type].frameCount==0); // 내장 TYPE의 원래 미작성 초기0도 유지한다.
        ++count;
    }
    CHECK(count==22);
}
}
// PATCH의 현재 공통 TYPE 기록을 기존 기계어 callback22입력과 비교한다.
TEST_CASE(RiftTypeAnimation_PatchGoldenMetadata) { CheckTypeAnimationRecord(OriginalEdition::Patch1078,"originals"); }
// CD의 별도 초기화/구조 크기에서도 같은 두 의미 DWORD를 기존22입력과 비교한다.
TEST_CASE(RiftTypeAnimation_CdGoldenMetadata) { CheckTypeAnimationRecord(OriginalEdition::Cd1072,"originalCD"); }
