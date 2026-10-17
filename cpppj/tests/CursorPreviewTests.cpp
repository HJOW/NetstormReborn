// 원본 미리보기 전체 몸체와 실제 Query/Prepare 사슬의 독립 기대값을 비교한다.
#include "CursorPreviewSupport.h"

using namespace netstorm::test::cursorpreview;
using namespace netstorm::test::rawscene;
using namespace netstorm::client;

// 전체 원문과 실제 생성→Query/Prepare 사슬의 반환/전체 상태/바이트/효과를 대조한다.
TEST_CASE(CursorPreviews_NativeGenerationReuseAndPlacementChain) {
    PreviewScene scene;std::size_t count{},chains{};
    // 입력마다 상태를 새로 주며 repeat 안에서는 같은 목록을 유지한다.
    for (const auto& row:LoadFixture(NETSTORM_CURSOR_PREVIEWS_FIXTURE).rows) {
        CHECK(row.size()==4);if (row.size()!=4) continue;scene.Prepare(Numbers(row[0]));auto hooks=scene.Hooks();const auto placementHooks=scene.PlacementHooks(hooks);
        RawCursorPlacement placement(scene.pool,scene.types,{},scene.drop,scene.life,scene.placementState,placementHooks);
        if (scene.input[Chain]) { BindCursorPreviewPlacement(hooks,scene.pool,placement,placementHooks);++chains; }
        RawCursorPreviews previews(scene.pool,scene.types,scene.drop,scene.life,scene.surfaces,std::move(hooks));CursorLifecycleHooks lifecycle;BindCursorPreviews(lifecycle,previews);
        // 실제 원문처럼 두 번째 호출도 첫 호출 결과를 이어 사용한다.
        for (std::uint32_t call=0;call<scene.input[Repeat];++call) lifecycle.createPreviews();
        const bool same=(scene.trace.empty() ? "-" : scene.trace)==row[1] && scene.Snapshot()==row[2] && scene.Raws()==row[3];CHECK(same);
        if (!same) {
            std::printf("미리보기 입력=%s\n기대=%s / %s\n실제=%s / %s\n",row[0].c_str(),row[2].c_str(),row[1].c_str(),scene.Snapshot().c_str(),scene.trace.c_str());
            if (scene.Raws()!=row[3]) std::printf("기대 슬롯=%s\n실제 슬롯=%s\n",row[3].c_str(),scene.Raws().c_str());break;
        }++count;
    }
    CHECK(count==522);CHECK(chains==32);
}
// 누락 경계와 다른 판본/풀 연결을 성공으로 처리하지 않는다.
TEST_CASE(CursorPreviews_RequiresDependenciesAndOneWorld) {
    PreviewScene scene;scene.Prepare({1,1,0,4,0,2,1,0,0,0,0,1,0,0});RawCursorPreviews missing(scene.pool,scene.types,scene.drop,scene.life,scene.surfaces,{});
    CHECK(Throws([&] { missing.Create(); }));SidPool cd(OriginalEdition::Cd1072,24000,false);
    CHECK(Throws([&] { RawCursorPreviews invalid(cd,scene.types,scene.drop,scene.life,scene.surfaces,scene.Hooks()); }));
    auto hooks=scene.Hooks();const auto ph=scene.PlacementHooks(hooks);RawCursorPlacement placement(scene.pool,scene.types,{},scene.drop,scene.life,scene.placementState,ph);
    CHECK(Throws([&] { BindCursorPreviewPlacement(hooks,cd,placement,ph); }));CHECK(&missing.Pool()==&scene.pool);
}
