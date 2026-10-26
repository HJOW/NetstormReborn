// 실제 Editor Session/공통 UI를 같은 동기 수명에 연결해 전체 섬 추가 창을 검증한다.
#include "client/EditorIslandWindow.h"
#include "client/RawCampaignSession.h"
#include "client/ClientMain.h"
#include "client/ClientRandomGlobals.h"
#include <cmath>

void WithEditorIslandWindowSession480(bool fresh,const std::function<void(RawCampaignSession&,Client&)>& body); // 다른 검사 TU의 실제 설치 helper를 빌린다.

// 기존/빈 새 맵에서 생성→갱신→목록 선택/취소→격자/거부→실제 배치→재갱신→Done을 통과한다.
TEST_CASE(RawMenuEditorIslandWindow_ActualExistingAndFreshWorldFullInputLifecycle) {
    for (const bool fresh:std::array{false,true}) { // 두 실제 설치 분기는 같은 UI/배치 몸체를 사용해야 한다.
        WithEditorIslandWindowSession480(fresh,[&](RawCampaignSession& session,Client& owner) {
            MenuClient ui;ui.EnableHelp();ui.helpConfig->Configuration().Assign("missionSpec=d/{local.1}.english\n");std::uint32_t pulses{},revisions{};std::array<float,2> camera{};
            EditorIslandWindowContext context;context.session=&session;context.translate=[](auto text) { return std::string(text); };context.tell=[&](auto name) { return ui.menus->ShowEmbeddedTell(name,*ui.helpConfig); };context.random=[&](auto limit) { return owner.RandomGlobals()->Game().Next(limit); };context.camera=[&](float x,float y) { camera={x,y}; };context.revision=[&] { ++owner.GameplayGlobals()->MapRevision();++revisions; };context.pulseRegistered=[&](auto id) { CHECK(id==0);++pulses; };
            const auto root=ui.menus->ShowEditorIslandWindow(context);const auto* window=ui.menus->EditorIslandWindow(root);CHECK(window && ui.menus->EditorIslandWindowCount()==1 && ui.buttons.DialogCount()==1);if (!window) return;
            CHECK(window->baseWidth==432 && window->baseHeight==412 && window->x==104 && window->y==34 && window->depth==0xffffffb0U && window->command==4 && window->currentImage.Shape().empty());
            CHECK(window->interaction.sharedFlags==&window->flags && window->layout.image==&window->currentImage && window->layout.interaction==&window->interaction);ui.menus->Idle(root);
            const auto* tell=static_cast<const HtmlBodyGumpNode*>(ui.menus->Node(window->layout.tell));CHECK(tell && tell->text && ui.menus->Node(tell->text)->textContext[2]==ui.Palette().named[23].index); // 원본005b5dc8의 실제219/213/203 기본색이다.
            auto* list=ui.menus->ImageListNode(window->layout.list);CHECK(list && list->count==21 && list->columns==4 && !window->interaction.dirty && window->ready==1 && pulses==1 && window->currentImage.Width()==224 && window->currentImage.Height()==224);if (!list) return;
            const auto afterSeed=owner.RandomGlobals()->Game().State();ui.menus->Idle(root);CHECK(pulses==1 && owner.RandomGlobals()->Game().State()==afterSeed);CHECK(ui.menus->Node(window->layout.map)->baseWidth==225 && ui.menus->Node(window->layout.background)->baseWidth==257);
            std::array<std::uint32_t,256> logical{};
            for (std::size_t index=0;index<logical.size();++index) { const auto c=ui.assets.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=c.red|(std::uint32_t(c.green)<<8)|(std::uint32_t(c.blue)<<16); } // 실제 원본 글자/질감/동적 Image를 같은 현재 표면에 그린다.
            constexpr int pitch=647,height=480;std::vector<std::uint8_t> pixels(pitch*height,0x5a);ui.menus->Draw(pixels,pitch,height,ui.Palette(),BuildGumpShadeMaps(logical));CHECK(std::any_of(pixels.begin(),pixels.end(),[](auto value) { return value!=0x5a; }));
            for (int y=0;y<height;++y) for (int x=640;x<pitch;++x) CHECK(pixels[static_cast<std::size_t>(y)*pitch+x]==0x5a); // 행 padding은 실제 공통 클립 밖이다.
            const auto listNode=ui.menus->Node(window->layout.list);const ScreenPoint listPoint{static_cast<int>(window->x+listNode->x+list->cellWidth*.5F),static_cast<int>(window->y+listNode->y+list->cellHeight*.5F)};
            ui.buttons.RouteInput({InputCode::kLeftButton,listPoint.x,listPoint.y});CHECK(window->interaction.drag && (window->flags&0x300U)==0x300U && !(window->flags&8U));const auto cancelled=window->interaction.drag;
            ui.buttons.RouteInput({InputCode::kLeftButton,listPoint.x,listPoint.y});CHECK(!window->interaction.drag && list->selection==-1 && !(window->flags&0x300U));ui.buttons.CleanupGumps();CHECK(!ui.menus->Node(cancelled));
            bool dropped{};const auto countBefore=session.IslandCount();
            for (std::int32_t index=0;index<21 && !dropped;++index) { // 원문 후보 중 실제 현재 지도에 놓을 수 있는 입력만 선택한다.
                ui.menus->ImageListBackend().Handle(window->layout.list,InputCode::kLeftButton,static_cast<std::uint32_t>(listPoint.x+static_cast<int>((index%4)*list->cellWidth)),static_cast<std::uint32_t>(listPoint.y+static_cast<int>((index/4)*list->cellHeight)));
                if (!window->interaction.drag) continue;const auto* drag=ui.menus->MetaImageNode(window->interaction.drag);const auto* map=ui.menus->Node(window->layout.map);const auto* background=ui.menus->Node(window->layout.background);const auto mapX=static_cast<int>(window->x+background->x+map->x),mapY=static_cast<int>(window->y+background->y+map->y);
                for (std::uint32_t y=0;y<14 && !dropped;++y) { // UI 중앙점과 실제 지도 경계 안의 합법 좌표를 찾는다.
                    for (std::uint32_t x=0;x<14 && !dropped;++x) { // 원문 위치 nibble과 half-size 좌표를 실제 충돌 검사로 판단한다.
                        auto record=window->interaction.record;record[2]=static_cast<std::uint8_t>((y<<4U)|x);const auto px=mapX+static_cast<int>(x*16)+static_cast<int>(std::ceil(drag->baseWidth*.5F)),py=mapY+static_cast<int>(y*16)+static_cast<int>(std::ceil(drag->baseHeight*.5F));if (px>=mapX+224 || py>=mapY+224) continue;
                        if (!session.CanPlaceEditorIsland(*window->preview,record)) continue;
                        ui.menus->Broadcast(root,0,static_cast<std::uint32_t>(px),static_cast<std::uint32_t>(py));CHECK(drag->colorMode==0 && drag->depth==0xffffff9cU && window->interaction.record[2]==record[2]);
                        ui.buttons.RouteInput({InputCode::kLeftButton,px,py});dropped=window->interaction.drag==0;
                    }
                }
                if (!dropped) ui.buttons.RouteInput({InputCode::kLeftButton,listPoint.x,listPoint.y});
            }
            CHECK((dropped && revisions==1 && camera==std::array<float,2>{112,112} && session.IslandCount()==countBefore+1 && window->interaction.dirty==1 && list->count==20 && list->selection==-1));
            const auto beforeRefresh=owner.RandomGlobals()->Game().State();ui.buttons.CleanupGumps();ui.menus->Idle(root);CHECK(list->count==21 && pulses==2 && owner.RandomGlobals()->Game().State()==beforeRefresh && !window->interaction.dirty);
            const auto done=window->layout.done;const auto* button=ui.menus->Button(done);CHECK(button && ui.buttons.PulseRegisteredIfIdle(button->id));CHECK(!ui.buttons.PulseRegisteredIfIdle(button->id));
            CHECK(ui.menus->Handle(root,0x0200002cU,ui.menus->Node(done)->serial,0)==1 && ui.menus->EditorIslandWindowCount()==1 && ui.buttons.DialogCount()==0);ui.buttons.CleanupGumps();CHECK(!ui.menus->Node(root) && ui.menus->EditorIslandWindowCount()==0);ui.menus->CloseEditorIslandWindows();
        });
    }
}

// 부분 중복 생성과 창 밖 드래그의 비소유 Image 수명도 상태 이탈까지 보존한다.
TEST_CASE(RawMenuEditorIslandWindow_ReferencesPartialConstructionAndRootDragRetirement) {
    WithEditorIslandWindowSession480(false,[&](RawCampaignSession& session,Client& owner) {
        MenuClient ui;ui.EnableHelp();ui.helpConfig->Configuration().Assign("missionSpec=d/{local.1}.english\n");EditorIslandWindowContext context;context.session=&session;context.translate=[](auto text) { return std::string(text); };context.tell=[&](auto name) { return ui.menus->ShowEmbeddedTell(name,*ui.helpConfig); };context.random=[&](auto limit) { return owner.RandomGlobals()->Game().Next(limit); };context.camera=[](float,float) {};context.revision=[] {};context.pulseRegistered=[](auto id) { CHECK(id==0); };
        const auto token=ui.menus->ShowEditorIslandWindow(context);ui.menus->Idle(token);const auto* window=ui.menus->EditorIslandWindow(token);const auto list=window->layout.list;auto* node=ui.menus->ImageListNode(list);node->selection=0;CHECK(ui.menus->Handle(token,0x02000004,0,1)==1);const auto drag=window->interaction.drag;const auto* image=ui.menus->MetaImageNode(drag)->image;
        CHECK(ui.menus->Handle(token,0x0200002cU,ui.menus->Node(window->layout.done)->serial,0)==1);ui.buttons.CleanupGumps();CHECK(!ui.menus->Node(token) && ui.menus->MetaImageNode(drag) && !image->Shape().empty() && ui.menus->EditorIslandWindowCount()==0);ui.menus->CloseEditorIslandWindows();CHECK(!ui.menus->Node(drag));
        const auto next=ui.menus->ShowEditorIslandWindow(context);bool rejected{};try { static_cast<void>(ui.menus->ShowEditorIslandWindow(context)); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && ui.menus->EditorIslandWindowCount()==1);ui.menus->CloseEditorIslandWindows();CHECK(!ui.menus->Node(next) && ui.menus->EditorIslandWindowCount()==0 && ui.buttons.DialogCount()==0);
    });
}
