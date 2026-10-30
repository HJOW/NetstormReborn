// 자연 적재한 Tutorial5 덱 항목의 점멸만 합성 입력해 실제 UI/Renderer 경계를 검사한다.
#include "client/InventoryGump.h"
TEST_CASE(InventoryBlink_ActualProductionDirtyRegionsPreserveFullPixels) { // 게임 창/진행 없이 동일 아이콘 자식/표시/부분 clip을 비교한다.
    ProductionLifetimeScene scene(true);scene.controls->IdleGumps(); // 실제 UI 첫 Idle이 Deck 재계산/내용물 생성/격자 재부착을 끝내게 하며 게임 Kernel은 진행하지 않는다.
    auto& production=*scene.controls->Production();const auto token=scene.Token(0x5068f4U);CHECK(token!=0);if (!token) { std::fprintf(stderr,"Blink UI contents: %s\n",production.Report().c_str());return; }
    auto& node=*static_cast<InventoryItemNode*>(production.Node(token));auto content=scene.session->Core().pool.AllocatedBytes({static_cast<std::uint16_t>(node.content)});const auto originalContent=content[21];
    std::array<ScreenColor,256> palette{};std::array<std::uint32_t,256> logical{};
    for (std::size_t index=0;index<palette.size();++index) { const auto color=scene.assets.game.Palette().Color(static_cast<std::uint8_t>(index));palette[index]={color.red,color.green,color.blue,0};logical[index]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); } // 보호 파일의 같은 색/팔레트 인덱스다.
    const auto colors=BuildPaletteColorTable(logical);const auto maps=BuildGumpShadeMaps(logical);scene.menus->SetPalette(colors);production.SetPalette(palette);
    constexpr int width=1024,height=768,pitch=1031;constexpr std::uint8_t background=0x5a; // 논리 화면과 행 padding을 구별해 전체 바이트를 대조한다.
    std::vector<std::uint8_t> partial(pitch*height,background),whole=partial;scene.menus->Draw(partial,pitch,height,colors,maps);
    const auto compare=[&] { // 실제 전달 영역만 비우고 다시 그려 독립 전체 출력의 동일성을 확인한다.
        CHECK(scene.fullRequests==0 && !scene.dirtyRectangles.empty());
        for (const auto& [rect,flags]:scene.dirtyRectangles) { // 화면과 교차하는 같은 half-open dirty 영역만 갱신한다.
            CHECK(flags==0);const auto left=std::max(0,rect.left),right=std::min(width,rect.right);
            if (left<right) for (int y=std::max(0,rect.top);y<std::min(height,rect.bottom);++y) std::fill(partial.begin()+y*pitch+left,partial.begin()+y*pitch+right,background); // 행 끝 padding과 영역 밖은 보존한다.
            scene.menus->Draw(partial,pitch,height,colors,maps,rect);
        }
        std::fill(whole.begin(),whole.end(),background);scene.menus->Draw(whole,pitch,height,colors,maps);CHECK(partial==whole);
    };
    for (int transition=0;transition<3;++transition) { // 보임→숨김·숨김→보임·종료의 실제 세 경계를 독립 초기화한다.
        scene.dirtyRectangles.clear();scene.fullRequests=0;node.blink=transition==2 ? 0 : 1;node.blinkUntil=scene.wall;content[21]|=2U;production.Idle(token);
        CHECK(bool(node.flags&1U)==(transition!=0));CHECK(node.blink==0);CHECK(bool(content[21]&2U)==(transition!=2));compare();
        CHECK(scene.dirtyRectangles.size()==1); // 전체 UI 변경 통지가 아닌 해당 항목의 기본 영역 한 번이다.
    }
    content[21]=originalContent; // 실제 Session 반납 전에 합성 표시 입력의 내용물 bit만 원래대로 돌린다.
}
