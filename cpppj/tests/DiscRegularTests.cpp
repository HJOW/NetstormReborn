// 실제 썬 디스크 Handle의 독립192입력을 현재 프레임 지정/반환에 대조한다.
#include "RawSceneSupport.h"
#include "o/RawDiscRegular.h"
using namespace netstorm::test::rawscene;
namespace {
// 명시 SHP 크기/표시 관찰 경계에서 원본 Handle의 요청과 같은 raw frame/반환을 확인한다.
void CheckDiscRegular(OriginalEdition edition) {
    const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,24000,false);const auto object=pool.Allocate(2);
    std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<ArcherTypeData> directions(types.size());
    std::size_t updates=0;
    SquidFrame setter(pool,types,{
        [](Sid,std::int32_t) { return std::array<float,2>{1,1}; }, // 명시 외부 SHP 입력이다. 실제 그림을 재현한 입력으로 세지 않는다.
        [&](Sid,std::uint32_t flags) { CHECK(flags==0);++updates; }, // 원본 공통 지정의 표시 요청만 관찰한다.
        [](Sid,std::uint32_t) { throw std::logic_error("동일 해시 단계 입력에서 예상 밖 Unpop"); },
        [](Sid,float,float,std::uint32_t) { throw std::logic_error("동일 해시 단계 입력에서 예상 밖 Pop"); }});
    RawDiscRegular logic(pool,directions,setter);std::size_t count=0;
    // 새 독립 원본 행을 판본별로 같은 풀/지정기에 재생한다.
    for (const auto& row:LoadFixture(NETSTORM_DISC_REGULAR_FIXTURE).rows) {
        if (row[0]!=(patch ? "originals" : "originalCD")) continue;
        auto raw=pool.AllocatedBytes(object);Put(raw,0,patch ? 0x50b570U : 0x501440U);raw[10]=83;raw[11]=0;raw[patch ? 33 : 31]=1;
        Put(raw,patch ? 36 : 34,Number(row[1]),patch ? 4 : 1);updates=0;
        directions[83]={std::stoi(row[3]),std::stoi(row[2])};
        CHECK(RawDiscRegular::NextFrame(std::stoi(row[1]),std::stoi(row[2]),std::stoi(row[3]))==std::stoi(row[7]));
        const auto returned=logic.Handle(object,Number(row[4]),Number(row[5]),std::bit_cast<float>(Number(row[6])));
        CHECK(std::bit_cast<std::uint32_t>(returned)==Number(row[9]));
        CHECK(Get(pool.Slot(object),patch ? 36 : 34,patch ? 4 : 1)==(Number(row[7])&(patch ? 0xffffffffU : 0xffU)));
        CHECK(Number(row[8])==0 && updates==2);++count;
    }
    CHECK(count==96);
}
}
// 패치의 DWORD frame/TYPE modulo와 float payload 전체 비트다.
TEST_CASE(DiscRegular_PatchNative) { CheckDiscRegular(OriginalEdition::Patch1078); }
// CD의 BYTE frame과 동일 실제 modulo 요청/float 반환이다.
TEST_CASE(DiscRegular_CdNative) { CheckDiscRegular(OriginalEdition::Cd1072); }
