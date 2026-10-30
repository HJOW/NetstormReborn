// 실제 Pool·자산·Native DrawEntry가 마나103 본체/조건부 그림자를 같은 픽셀로 출력하는지 확인한다.
TEST_CASE(CampaignManaAdditional_NativeEntryUsesRealManaAssetAndConditionalShadow) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();UseWorldTestLogin(services);
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});auto& session=world.Session();const auto core=session.Core();
    const auto position=WorkshopDisplaySupport(session);const auto mana=session.World().Create(103,2);core.owner.Set(mana,1);session.World().Pop(mana,position[0],position[1],0);auto raw=core.pool.AllocatedBytes(mana);
    CHECK(Get(raw,0)==0x50c3b0U);core.ownerMode.battle=false;raw[40]=2; // 본체의+a4 출력과 원문 생성자의 가상 그림 비트만 검증한다.
    const auto palette=WorkshopDisplayPalette(assets.game);std::array<std::uint32_t,256> logical{};
    for (std::size_t index=0;index<logical.size();++index) logical[index]=std::uint32_t(palette[index].red)|(std::uint32_t(palette[index].green)<<8)|(std::uint32_t(palette[index].blue)<<16); // 같은 실제 파일 팔레트의 그림자 치환 입력이다.
    const auto maps=BuildSpriteColorMaps(logical);RawCampaignFactoryDisplay display(core,assets.game);display.SetPalette(palette);
    const auto block=assets.game.Types()[103-netstorm::o::kFirstAssetTypeNumber].block;const auto frameCount=static_cast<std::uint32_t>(assets.game.Shapes().Blocks()[block].frames.size());CHECK(frameCount>0);if (!frameCount) return;
    const int px=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Get(raw,22))),py=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Get(raw,24)));const std::array<std::int32_t,2> camera{px-512,py-384}; // 원문 Pop이 저장한 실제 투영값으로 카메라를 잡는다.
    constexpr int width=1024,height=768,pitch=1031;const ScreenRect clip{0,0,width,height}; // 실제 출력과 행 padding을 함께 검사한다.
    netstorm::o::RenderCollectionEntry entry;entry.token=mana.value*core.pool.Layout().stride;std::size_t cases{}; // 실제 공통 목록의 SID/stride 토큰이다.
    for (const auto counter:{0U,1U,0xffffU}) for (const auto frame:{0U,frameCount/2,frameCount-1}) { // 조건 WORD 세 값과 실제 첫/중간/끝 프레임을 교차한다.
        raw[12]=static_cast<std::uint8_t>(counter);raw[13]=static_cast<std::uint8_t>(counter>>8);Put(raw,36,frame);std::vector<std::uint8_t> actual(pitch*height,199),expected(actual);
        display.DrawEntry(entry,actual,pitch,height,clip,camera,1,0,{});
        const auto image=assets.game.Shapes().Decode(block,frame);const auto rect=assets.game.Shapes().Blocks()[block].frames[frame].rect;const int left=512-core.types[103].hotspotX+rect.left,top=384-core.types[103].hotspotY+rect.top;
        if (counter) DrawIndexedImage(expected,pitch,height,image,left+10,top+10,clip,&maps.shadow);DrawIndexedImage(expected,pitch,height,image,left,top,clip);
        CHECK(actual==expected && std::any_of(actual.begin(),actual.end(),[](auto value) { return value!=199; }));++cases;
    }
    CHECK(cases==9);
}
