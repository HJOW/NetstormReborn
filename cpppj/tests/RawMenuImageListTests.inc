// 같은 실제 공통 트리의 목록·자식·입력·체크무늬·지연 삭제를 검사한다.
#include "client/MetaImageList.h"

//21개 실제 자식을 만들어 라우팅/ID/토글/삭제 압축과 비소유 Image를 확인한다.
TEST_CASE(RawMenuImageList_ActualTreeTwentyOneSlotsSelectionPixelsAndRemoval) {
    MenuClient client;std::array<RawImage,22> images;
    for (auto& image:images) image.Convert({6,4,std::vector<std::uint8_t>(24)},false); // 빈 shape는 선택 체크무늬의 픽셀을 독립적으로 확인하게 한다.
    const auto token=client.menus->CreateImageList(4);auto* node=client.menus->ImageListNode(token);CHECK(node && node->count==0 && node->selection==-1 && node->command==22 && node->vtable==0x50d0d4 && (node->flags&0x200));if (!node) return;
    node->baseWidth=node->width=200;node->baseHeight=node->height=120;client.menus->Attach(token,20,30,client.buttons.RootToken(),0);const auto& body=client.menus->ImageListBackend();
    for (std::uint32_t index=0;index<21;++index) CHECK(body.Add(token,images[index],1000+index)); // 원문4열/21개를 같은 실제 MetaImage 공장으로 만든다.
    const auto originalChildren=node->images;CHECK(node->count==21 && node->cellWidth==50 && node->cellHeight==20 && !body.Add(token,images[21],2000));
    const auto clicked=client.buttons.RouteInput({InputCode::kLeftButton,75,55});CHECK(clicked.consumed!=0 && node->selection==5 && body.Id(token,5)==1005 && body.Image(token,5)==&images[5]);
    constexpr int pitch=647,height=480; // 화면 너비 뒤의 padding까지 검사한다.
    std::array<std::uint32_t,256> logical{};
    for (std::size_t index=0;index<logical.size();++index) { const auto color=client.assets.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); } // 실제 파일 팔레트의 RGB 순서다.
    std::vector<std::uint8_t> pixels(pitch*height,0x5a);client.menus->Draw(pixels,pitch,height,client.Palette(),BuildGumpShadeMaps(logical));
    for (int y=0;y<height;++y) { // 원본 선택 사각형은 half-open 체크무늬다.
        for (int x=0;x<pitch;++x) { const auto expected=x>=70 && x<120 && y>=50 && y<70 && ((x^y)&1) ? static_cast<std::uint8_t>(client.Palette().basic[2]) : 0x5a;CHECK(pixels[static_cast<std::size_t>(y)*pitch+x]==expected); } // 선택 밖과 행 padding은 보존한다.
    }
    client.buttons.RouteInput({InputCode::kLeftButton|0x80000000U,75,55});client.buttons.RouteInput({InputCode::kLeftButton,75,55});CHECK(node->selection==-1);
    CHECK(body.Handle(token,InputCode::kLeftButton,75,55)==1 && node->selection==5);CHECK(body.Remove(token,5) && node->count==20 && node->selection==5 && body.Id(token,5)==1006);
    CHECK(node->images[19]==originalChildren[20] && node->images[20]==originalChildren[20] && node->ids[19]==1020 && node->ids[20]==1020);
    client.buttons.KillGump(token);client.buttons.CleanupGumps();CHECK(!client.menus->ImageListNode(token));
    for (const auto child:originalChildren) CHECK(!client.menus->MetaImageNode(child)); // 삭제 큐는 압축 후에도 실제 트리의 모든 자식을 반납한다.
    for (const auto& image:images) CHECK(!image.Shape().empty()); // 목록/MetaImage 소멸은 빌린 Image를 해제하지 않는다.
}
