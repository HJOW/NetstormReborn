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
// CD10.72의 BYTE frame/float 비트 signed 비교로 구한 유한 유효점 판정이다.
TEST_CASE(ProjectileGeometry_CdNative) { CheckProjectileGeometry(OriginalEdition::Cd1072); }
// PATCH 썬 캐논 가상 포구 getter가 비정렬 raw x/y를 변형 없이 반환하는지 확인한다.
TEST_CASE(ProjectileGeometry_SunCannonRawMuzzle) {
    SidPool pool(OriginalEdition::Patch1078,24000,false);
    const auto object=pool.Allocate(2);
    std::vector<RiftTypeRecord> types(188);
    std::vector<ProjectileGeometryType> geometry(types.size());
    RawProjectileGeometry logic(pool,types,geometry);
    auto raw=pool.AllocatedBytes(object);
    constexpr std::uint32_t kRawXBits=0x422a0000U; // 좌표 보정이 없는 float32 x 입력의 원본 비트다.
    constexpr std::uint32_t kRawYBits=0x41a40000U; // 좌표 보정이 없는 float32 y 입력의 원본 비트다.
    raw[10]=128;
    Put(raw,14,kRawXBits);
    Put(raw,18,kRawYBits);
    const auto point=logic.Muzzle(object);
    CHECK(std::bit_cast<std::uint32_t>(point[0])==kRawXBits);
    CHECK(std::bit_cast<std::uint32_t>(point[1])==kRawYBits);
}
// PATCH 썬 캐논의 기본0..3과 발사24..27 프레임 모두에서 네 방향 종점의 축 정렬을 확인한다.
TEST_CASE(ProjectileGeometry_SunCannonAxisRayEnd) {
    SidPool pool(OriginalEdition::Patch1078,24000,false);
    const auto object=pool.Allocate(2);
    std::vector<RiftTypeRecord> types(188);
    std::vector<ProjectileGeometryType> geometry(types.size());
    RawProjectileGeometry logic(pool,types,geometry);
    auto raw=pool.AllocatedBytes(object);
    raw[10]=128;
    Put(raw,36,0);
    auto point=logic.CannonRayEnd({10,20},{11,30},object);
    CHECK(point[0]==10 && point[1]==30);
    Put(raw,36,1);
    point=logic.CannonRayEnd({10,20},{11,10},object);
    CHECK(point[0]==10 && point[1]==10);
    Put(raw,36,2);
    point=logic.CannonRayEnd({10,20},{1,19},object);
    CHECK(point[0]==1 && point[1]==20);
    Put(raw,36,3);
    point=logic.CannonRayEnd({10,20},{30,21},object);
    CHECK(point[0]==30 && point[1]==20);
    Put(raw,36,24);
    point=logic.CannonRayEnd({10,20},{11,30},object);
    CHECK(point[0]==10 && point[1]==30);
    Put(raw,36,25);
    point=logic.CannonRayEnd({10,20},{11,10},object);
    CHECK(point[0]==10 && point[1]==10);
    Put(raw,36,26);
    point=logic.CannonRayEnd({10,20},{1,19},object);
    CHECK(point[0]==1 && point[1]==20);
    Put(raw,36,27);
    point=logic.CannonRayEnd({10,20},{30,21},object);
    CHECK(point[0]==30 && point[1]==20);
}
