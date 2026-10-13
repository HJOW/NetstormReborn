// 실제 보호Dude 생성 뒤 발자국0의 제거/재등록과 전체공간/장부/표시를 대조한다.
#include "DudeSpaceSupport.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
using namespace netstorm::test::dudespace;
namespace {
// 실제 PE의 독립 기대 출력은 fixture에만 저장한다.
const std::vector<std::vector<std::string>>& SpaceRows() { static const auto rows=LoadFixture(NETSTORM_DUDESPACE_FIXTURE).rows;return rows; }
// 현재 공간행에서 최초Birth 입력만 같은 실제 생성 장면에 넘긴다.
std::vector<std::string> BirthInput(const std::vector<std::string>& row) { return {row[0],"3","0","0",row[1],row[2],row[3],"0"}; }
// 기대 출력은 읽지 않고 실제 생성/Unpop/선택Pop을 실행한 뒤 전체 상태를 비교한다.
std::vector<std::string> RunSpace(const std::vector<std::string>& row) {
    const auto input=BirthInput(row);SpawnScene scene(input);const auto first=scene.Run(input);CHECK(first[0]=="6");const Sid sid{6};scene.unpop.Unpop(sid,scene.assets.types[70],0);const auto removed=Hex(scene.pool.Slot(sid));
    if (Number(row[6])) {
        const auto size=scene.display.FrameSize(scene.pool.Edition(),scene.pool.Slot(sid),64);
        CHECK(scene.pop.Pop(sid,scene.assets.types[70],size[0],size[1],std::bit_cast<float>(Number(row[4])),std::bit_cast<float>(Number(row[5])),0x50)==RawPopResult::Registered);
    }
    auto output=scene.Output(sid);output.insert(output.begin(),removed);return output;
}
// 같은24고유입력을 판본별로 모두 실제 처리기에 재생한다.
void ReplaySpace(std::string_view edition) {
    std::size_t count=0;int failures=0;
    // 제거시점raw/최종전체출력은 실제 실행 후에만 기대값과 비교한다.
    for (const auto& row:SpaceRows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==25);const auto output=RunSpace(row);const auto expected=std::vector<std::string>(row.begin()+7,row.end());const bool same=output==expected;CHECK(same);
        if (!same) {
            // 최초차이의 전체 native/C++ 값을 함께 기록한다.
            for (std::size_t index=0;index<output.size();++index) if (output[index]!=expected[index]) std::printf("%s DudeSpace 행%zu 결과%zu 기대%s / 실제%s\n",row[0].c_str(),count,index,expected[index].c_str(),output[index].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==24);
}
}
// 실제10.78 foot0 빈순회/네해시제거/실제Activate 재등록과 깊이를 대조한다.
TEST_CASE(DudeSpace_PatchNativeBirthUnpopAndRepop) { ReplaySpace("originals"); }
// CD10.72 같은공간/표시/최초I프레임의 실제 제거와 재등록을 대조한다.
TEST_CASE(DudeSpace_CdNativeBirthUnpopAndRepop) { ReplaySpace("originalCD"); }
