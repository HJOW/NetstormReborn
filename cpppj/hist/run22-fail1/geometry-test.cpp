// 원본 PE의 독립 포구/치수/충돌 상자240입력을 float 전체 비트로 비교한다.
#include "RawSceneSupport.h"
#include "o/RawProjectileGeometry.h"
using namespace netstorm::test::rawscene;

namespace {
// 판본별 같은 합성 입력을 구성하며 원본 결과를 C++로 재계산하지 않는다.
void CheckProjectileGeometry(OriginalEdition edition) {
    SidPool pool(edition,24000,false);const auto object=pool.Allocate(2);
    std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
    std::vector<ProjectileGeometryType> geometry(types.size());
    geometry[71].defaultFrame=1;geometry[71].frameCount=4;
    geometry[71].shotOffsets={{{-35,91}},{{20,-7}},{{32767,-32768}},{{-127,43}}};
    std::size_t count=0;
    // 현재 행의 판본 입력만 읽어 같은 조회기의 모든 결과 DWORD를 원본과 비교한다.
    for (const auto& row:LoadFixture(NETSTORM_PROJECTILE_GEOM_FIXTURE).rows) {
        if (row[0]!=(edition==OriginalEdition::Patch1078 ? "originals" : "originalCD")) continue;
        auto raw=pool.AllocatedBytes(object);raw[10]=71;
        Put(raw,14,Number(row[6]));Put(raw,18,Number(row[7]));
        Put(raw,edition==OriginalEdition::Patch1078 ? 36 : 34,Number(row[2]),edition==OriginalEdition::Patch1078 ? 4 : 1);
        raw[edition==OriginalEdition::Patch1078 ? 40 : 35]=static_cast<std::uint8_t>(Number(row[3]));
        types[71].footX=std::stoi(row[8]);types[71].footY=std::stoi(row[9]);geometry[71].heightExtension=std::stoi(row[10]);
        RawProjectileGeometry logic(pool,types,geometry,std::stof(row[4]),std::stof(row[5]));
        std::vector<std::uint32_t> actual;
        if (row[1]=="M") { const auto point=logic.ArcherMuzzle(object);actual={std::bit_cast<std::uint32_t>(point[0]),std::bit_cast<std::uint32_t>(point[1])}; }
        else if (row[1]=="D") { const auto dims=logic.Dimensions(object);actual={static_cast<std::uint32_t>(dims[0]),static_cast<std::uint32_t>(dims[1])}; }
        else {
            const auto box=logic.TargetBounds(object,{std::bit_cast<float>(Number(row[6])),std::bit_cast<float>(Number(row[7]))});
            // 네 모서리 좌표를 순서대로 binary32 전체 비트로 읽는다.
            for (const auto value:box) actual.push_back(std::bit_cast<std::uint32_t>(value));
        }
        const auto expected=Split(row[11],',');CHECK(actual.size()==expected.size());
        // 허용오차 없이 원본 DWORD 전체를 비교한다.
        for (std::size_t i=0;i<actual.size();++i) CHECK(actual[i]==static_cast<std::uint32_t>(std::stoul(expected[i],nullptr,16)));
        ++count;
    }
    CHECK(count==120);
}
}
// 10.78의 현재 DWORD frame/SHP/float 유효점 판정이다.
TEST_CASE(ProjectileGeometry_PatchNative) { CheckProjectileGeometry(OriginalEdition::Patch1078); }
// CD10.72의 BYTE frame/int 유효점 판정이다.
TEST_CASE(ProjectileGeometry_CdNative) { CheckProjectileGeometry(OriginalEdition::Cd1072); }
