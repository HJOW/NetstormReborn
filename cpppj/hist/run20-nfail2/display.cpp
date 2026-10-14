// 실제 신전 파생8c와 공통88의 독립 PE 변경표 전체를 비교한다.
#include "RawSceneSupport.h"
#include "o/SquidDisplay.h"
#include <cstdio>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// Renderer와 같은 호출순서의 변경표를 기록하는 검사 출력 경계다.
struct VortexDisplaySink final : SquidDisplaySink {
    std::vector<std::string> entries; // 원본 변경표에 대응하는 사각형/flags다.
    // 이 fixture의 활성 표시 입력은 전체 억제를 켜지 않는다.
    bool Suppressed() const override { return false; }
    // 원본 변경표처럼 좌/상/우/하/flags를 순서대로 보존한다.
    void Invalidate(SquidDisplayRect r,std::uint32_t flags) override {
        entries.push_back(std::to_string(r.left)+','+std::to_string(r.top)+','+std::to_string(r.right)+','+std::to_string(r.bottom)+','+std::to_string(flags));
    }
};
// 같은 입력 TYPE/SHP/raw와 viewport로 production 표시 모듈을 실행한다.
void CheckVortexDisplayNative(OriginalEdition edition) {
    const bool patch=edition==OriginalEdition::Patch1078;
    std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<SquidDisplayShape> shapes(types.size());
    auto& shape=shapes[74];shape.frameCount=2;shape.loaded=true;
    shape.frames={{100,120,-3,5,1,1},{130,140,7,-9,1,1},{50,70,2,3,1,1},{80,90,4,6,1,1}};
    types[74].flags2=0x10000;types[74].footX=types[74].footY=1;
    VortexDisplaySink sink;SquidDisplay display(edition,types,shapes,sink,{7,9,65536,{12,9,620,450}});
    const auto rows=LoadFixture(NETSTORM_VORTEX_DISPLAY_FIXTURE).rows;std::size_t count=0;
    // 원본이 기록한 모든 입력을 재생하며 기대 사각형은 재계산하지 않는다.
    for (const auto& row:rows) {
        if (row[0]!=(patch ? "originals" : "originalCD")) continue;CHECK(row.size()==5);sink.entries.clear();
        std::vector<std::uint32_t> input;
        // 저장된 인자만 파싱한다.
        for (const auto& text:Split(row[1],',')) input.push_back(Number(text));
        types[74].flags1=input[3];display.SetView({7,9,static_cast<int>(input[4]),{12,9,620,450}});
        std::vector<std::uint8_t> raw(patch ? 50 : 36);
        // 실제 호출 직전 슬롯 전체 바이트를 그대로 제공한다.
        for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(std::stoul(row[2].substr(i*2,2),nullptr,16));
        const auto before=raw;display.Update(raw,0x2000);CHECK(raw==before);
        std::string output;
        // 변경표의 호출순서를 그대로 이어 독립 native와 비교한다.
        for (const auto& entry:sink.entries) { if (!output.empty()) output+=';';output+=entry; }
        if (output.empty()) output="-";
        if (output!=row[4]) std::printf("VortexDisplay 불일치 %s: %s / %s\n",row[1].c_str(),output.c_str(),row[4].c_str());
        CHECK(row[3]=="0" && output==row[4]);++count;
    }
    CHECK(count==324);
}
}
// 패치판의 lazy inset/WORD 비영점 공통88·선택/그림자·카메라324입력을 비교한다.
TEST_CASE(VortexDisplay_PatchActualPE) { CheckVortexDisplayNative(OriginalEdition::Patch1078); }
// CD판의 같은324입력과 BYTE frame/extra 차이를 비교한다.
TEST_CASE(VortexDisplay_CdActualPE) { CheckVortexDisplayNative(OriginalEdition::Cd1072); }
