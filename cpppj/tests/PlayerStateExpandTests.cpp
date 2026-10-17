// 실제Datamanager 원문에서 관측한 State 확장과 같은 getter의 현재stream을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawPlayerTemplateState.h"
using namespace netstorm::test::rawscene;
// 양쪽정밀도99개독립입력에서원문보존/0확장/현재pos·end를검사한다.
TEST_CASE(PlayerStateExpand_Native99AndSameCurrentStream) {
    std::size_t count{}; //대조한원본입력전수다.
    //C++로기대값을계산하지않고원본행전체와현재메모리를대조한다.
    for (const auto& row:LoadFixture(NETSTORM_PLAYER_STATE_EXPAND_FIXTURE).rows) {
        CHECK(row.size()==10);if (row.size()!=10) continue;const auto length=Number(row[0]),pattern=Number(row[1]);std::array<std::uint8_t,64> old{},fresh{};fresh.fill(0xa5); //명시원문과새할당의미초기화입력이다.
        //원본관측기와같은독립물리바이트만입력한다.
        for (std::size_t i=0;i<old.size();++i) old[i]=static_cast<std::uint8_t>(i*17+pattern);
        const auto before=old;std::array<std::uint8_t,180> headers{};headers[20]=1;std::array<PlayerTemplateStateStream,9> streams{};streams[1]={old,13,length,true};std::array<std::string,9> names{};names[1]="tutorial1";std::uint32_t depth{},allocations{};bool mission=true; //원본lookup이pos13을0으로바꾸는현재입력이다.
        RawPlayerTemplateState body(OriginalEdition::Patch1078,headers,streams,names,depth,mission,[](PlayerTemplateStateAssertion) { throw std::logic_error("State확장 정상입력의예상밖진단"); },[](std::uint32_t) { throw std::logic_error("PATCH State확장에서CD depth효과금지"); },[&](std::uint32_t owner,std::uint32_t size) { CHECK(owner==1 && size==28);++allocations;return std::span(fresh).first(size); });
        const auto bytes=body.Get(1);CHECK(allocations==Number(row[2]) && streams[1].position==Number(row[3]) && streams[1].end==Number(row[4]));CHECK(Hex(bytes.first(28))==row[5] && old==before && depth==0);
        //두정밀도와다음Get의같은현재버퍼/pos0도확인한다.
        for (std::size_t i=2;i<6;++i) CHECK(row[i]==row[i+4]);const auto* pointer=bytes.data();CHECK(body.Get(1).data()==pointer && streams[1].position==0 && allocations==Number(row[2]));++count;
    }
    CHECK(count==99);
}
