// 같은 실제 Session/지도/후보 Image를 미리보기 기하/배치 가능 색상에 연결한다.
#include "client/EditorIslandLayout.h"
#include <cmath>

// 진입 관문·합법/충돌·지도 밖 미리보기와 월드/난수의 읽기 전용 계약을 확인한다.
TEST_CASE(CampaignEditorIslandMotion_ActualSessionPreviewColorAndReadOnlyWorld) {
    EditorInstallWireScene wire;std::shared_ptr<RawSurfaceStorage> surface;
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        ChunkMap preview(14,14);std::array<std::uint8_t,6> record{0,1,0,37,0,0};
        CHECK(CheckpointRejects<std::logic_error>([&] { static_cast<void>(session.CanPlaceEditorIsland(preview,record)); }));
        RunEditorWireInstallState(session,client,checkpoint,work);session.CommitExistingEditorInstall(*client.LocalStateGlobals());CHECK(session.BuildEditorIslandMap(preview));
        CHECK(CheckpointRejects<std::invalid_argument>([&] { static_cast<void>(session.CanPlaceEditorIsland(surface->chunks,record)); }));ChunkMap wrong(13,14);CHECK(CheckpointRejects<std::invalid_argument>([&] { static_cast<void>(session.CanPlaceEditorIsland(wrong,record)); }));
        const auto core=session.Core();const std::vector<std::uint8_t> pool(core.pool.Bytes().begin(),core.pool.Bytes().end());const auto seed=client.RandomGlobals()->Game().State();const auto islands=surface->islands.records;
        std::optional<std::uint8_t> valid,blocked;
        for (std::uint32_t y=0;y<14;++y) { // 동일한 실제 지도에서 관측용 합법/충돌 입력을 찾으며 배치 자체는 하지 않는다.
            for (std::uint32_t x=0;x<14;++x) { record[2]=static_cast<std::uint8_t>((y<<4U)|x);if (session.CanPlaceEditorIsland(preview,record)) { if (!valid) valid=record[2]; }else if (!blocked) blocked=record[2]; } // 모든 청크의 현재 프레임/충돌 검사를 호출한다.
        }
        CHECK(valid && blocked);if (!valid || !blocked) return;
        auto image=session.BuildEditorIslandChoiceShape(record);EditorIslandDragState interaction;interaction.drag=2;interaction.record=record;MetaImageGumpNode drag;drag.image=&image;drag.baseWidth=static_cast<float>(image.Width()+1);drag.baseHeight=static_cast<float>(image.Height()+1);
        EditorIslandLayoutFields fields;fields.map=3;fields.interaction=&interaction;std::uint32_t calls{},pushes{},attaches{};
        EditorIslandLayoutHooks hooks;hooks.fields=[&](auto)->EditorIslandLayoutFields& { return fields; };hooks.node=[&](auto token)->GumpCreateNode* { return token==2 ? &drag : nullptr; };hooks.image=[&](auto token) { return token==2 ? &drag : nullptr; };
        hooks.push=[&](auto token) { CHECK(token==2);++pushes; };hooks.bounds=[](auto token) { CHECK(token==3);return ScreenRect{100,100,324,324}; };hooks.position=[](auto token) { CHECK(token==3);return ScreenPoint{100,100}; };hooks.root=[] { return 1U; };
        hooks.canPlace=[&](auto value) { ++calls;return session.CanPlaceEditorIsland(preview,value); };hooks.attach=[&](auto token,float x,float y,auto parent,auto mode) { CHECK(token==2 && parent==1 && mode==0);drag.x=x;drag.y=y;++attaches; };
        EditorIslandLayout body(std::move(hooks));
        for (const auto coordinate:std::array{*valid,*blocked}) { // 합법/충돌 모두 같은 실제 CanPlace를 통해 색상만 바뀐다.
            const auto x=coordinate&15U,y=static_cast<std::uint32_t>(coordinate>>4);body.Motion(1,100+static_cast<int>(x*16U)+static_cast<int>(std::ceil(drag.baseWidth*.5F)),100+static_cast<int>(y*16U)+static_cast<int>(std::ceil(drag.baseHeight*.5F)));
            CHECK(interaction.record[2]==coordinate && drag.colorMode==(coordinate==*valid ? 0U : 1U) && drag.depth==0xffffff9cU);CHECK(drag.x==100+static_cast<float>(x*16U) && drag.y==100+static_cast<float>(y*16U));
        }
        const auto before=interaction.record;body.Motion(1,-100,-100);CHECK(interaction.record==before && drag.colorMode==1 && calls==2 && pushes==3 && attaches==3);
        CHECK(surface->islands.records==islands && client.RandomGlobals()->Game().State()==seed && std::equal(pool.begin(),pool.end(),core.pool.Bytes().begin()));CHECK(session.CanPlaceEditorIsland(preview,std::array<std::uint8_t,6>{0,1,*valid,37,0,0}));
    },true,&wire,{},[&](Client&,RawCampaignSessionServices& services) { surface=services.surfaceStorage;services.editorIslandGreen=[] { return 43U; }; });
}
