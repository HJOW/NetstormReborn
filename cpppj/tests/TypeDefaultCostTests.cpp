// 실제 두 판본 전체 TYPE 후처리의 비용 비트를 로더와 대조한다.
#include "DudeSpawnSupport.h"

namespace { // 원본 입력의 TYPE/속성 구성만 담당하며 기대 가격은 계산하지 않는다.
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
// 같은 로딩 순서의 한 TYPE에만 명시 원본 속성 입력을 공급한다.
RiftTypeTable CostTable(OriginalEdition edition,std::uint32_t number,const RiftTypeDefinition& definition) {
    const RiftTypeDefinition empty; // 나머지 TYPE는 원래 무속성/NO_GROUP 초기화다.
    std::vector<RiftTypeSource> sources; // 로더가 사용하는 실제 파일 이름 순서를 유지한다.
    for (const auto name:TypeLoadOrder(edition)) sources.push_back({name,sources.size()+70==number ? &definition : &empty}); // 목적 TYPE 밖의 속성을 바꾸지 않는다.
    return RiftTypeTable(edition,sources);
}
// 독립 기계어 관찰의 실제 입력으로 전체 로더를 실행한다.
void ReplayCost(std::string_view editionName,OriginalEdition edition) {
    std::size_t count{}; // 해당 판본에서 재생한 고유 입력 수다.
    for (const auto& row:LoadFixture(NETSTORM_TYPE_DEFAULT_COST_FIXTURE).rows) { // 원문 기대 비트는 새 C++ 계산으로 만들지 않는다.
        if (row[0]!=editionName) continue;CHECK(row.size()==7);
        const auto number=Number(row[1]),group=Number(row[2]),flags=Number(row[3]);
        const auto level=std::stoi(row[4]);const auto before=std::bit_cast<float>(Number(row[5])); // 원문 int level과 float32 cost 입력이다.
        RiftTypeDefinition definition; // 수치 AST의 float32 변환도 실제 로더를 통과한다.
        if (group!=10) definition.properties.push_back({"group",std::string(group==0 ? "archer" : group==3 ? "aviary" : group==7 ? "walker" : "balloon")});
        definition.properties.push_back({"level",static_cast<double>(level+1)});
        definition.properties.push_back({"cost",static_cast<double>(before)});
        if (flags&0x10000U) definition.flags.push_back("walker");
        if (flags&0x20000U) definition.flags.push_back("balloon");
        if (flags&0x400U) definition.flags.push_back("guy");
        if (flags&0x200000U) definition.flags.push_back("priest");
        const auto table=CostTable(edition,number,definition);
        CHECK(std::bit_cast<std::uint32_t>(table.Types()[number].cost)==Number(row[6]));++count;
    }
    CHECK(count==1500);
}
}
TEST_CASE(TypeDefaultCost_PatchNative1500) { ReplayCost("originals",OriginalEdition::Patch1078); } // 누락/양음0·명시비용·운반/일반·사제/사람 제외다.
TEST_CASE(TypeDefaultCost_CdNative1500) { ReplayCost("originalCD",OriginalEdition::Cd1072); } // 같은 가격인 CD의 별도 실제 본체도 검증한다.
TEST_CASE(TypeDefaultCost_ProtectedAssetsAndOmittedProperties) { // 합성 입력과 별개로 실제 두 판본의 생략 속성을 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 각 판본이 실제로 읽은 자산과 로더를 사용한다.
        const auto& assets=netstorm::test::dudespawn::Assets(edition).game;
        const auto types=assets.TypeTable().Types();
        CHECK(types[133].cost==400.0F && types[72].cost==400.0F);
        CHECK(types[158].cost==0.0F && types[70].cost==0.0F);
        CHECK(types[71].cost==(edition==OriginalEdition::Patch1078 ? 300.0F : 200.0F)); // CD 썬 발사기는 명시 가격 없이 일반 기본 단가를 쓴다.
        CHECK(types[128].cost==400.0F && types[135].cost==(edition==OriginalEdition::Patch1078 ? 500.0F : 600.0F)); // 벌프는 각 판본의 명시 가격을 보존한다.
        CHECK(!assets.Types()[133-70].definition.Number("cost"));
        CHECK(!assets.Types()[72-70].definition.Number("cost"));
    }
}
