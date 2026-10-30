// 원문 VT/필드를 명시한 합성 객체로 실제 Native 분배·같은 자산·VFX 픽셀 경계를 대조한다.
#include "client/VFXTransform.h"
TEST_CASE(CampaignAuxiliaryDraw_NativeEntryScaledAndAbstractRealAssetPixels) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();UseWorldTestLogin(services);
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});auto& session=world.Session();const auto core=session.Core();
    const auto position=WorkshopDisplaySupport(session);const auto actor=session.World().Create(103,2);core.owner.Set(actor,1);session.World().Pop(actor,position[0],position[1],0);auto raw=core.pool.AllocatedBytes(actor);core.ownerMode.battle=false;
    struct RestoreAuxiliaryRaw { // 합성 VT 검사가 예외로 끝나도 실제 마나 소멸 전에 원문 상태를 되돌린다.
        std::span<std::uint8_t> target; // 실제 Pool에서 빌린 같은50BYTE 주소다.
        std::vector<std::uint8_t> original; // 합성 필드 변경 전의 실제 생성/Pop 결과다.
        ~RestoreAuxiliaryRaw() { std::copy(original.begin(),original.end(),target.begin()); } // World/Pool보다 먼저 반납한다.
    } restore{raw,{raw.begin(),raw.end()}};
    const auto palette=WorkshopDisplayPalette(assets.game);std::array<std::uint32_t,256> logical{};
    for (std::size_t index=0;index<logical.size();++index) logical[index]=std::uint32_t(palette[index].red)|(std::uint32_t(palette[index].green)<<8)|(std::uint32_t(palette[index].blue)<<16); // 실제 파일 팔레트의 고정 추상색 입력이다.
    const auto maps=BuildSpriteColorMaps(logical);RawCampaignFactoryDisplay display(core,assets.game);display.SetPalette(palette);
    const auto block=assets.game.Types()[103-netstorm::o::kFirstAssetTypeNumber].block;const auto frameCount=static_cast<std::uint32_t>(assets.game.Shapes().Blocks()[block].frames.size());CHECK(frameCount>0);if (!frameCount) return;
    const int px=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Get(raw,22))),py=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(Get(raw,24)));const std::array<std::int32_t,2> camera{px-512,py-384}; // 실제 Pop의 투영값으로 기준점을 중앙에 둔다.
    constexpr int height=768,pitch=1031;const ScreenRect clip{450,330,570,440};netstorm::o::RenderCollectionEntry entry;entry.token=actor.value*core.pool.Layout().stride;std::size_t cases{}; // padding/부분 clip을 포함한 같은 Native 토큰이다.
    const std::array<std::array<std::uint32_t,3>,6> scales{{{{0,0,65536}},{{0,32768,32768}},{{1,32768,98304}},{{2,0,131072}},{{0x4000,0,0}},{{0xabcd0000,65535,65535}}}}; // 원문에서 읽는 flags/분수와 별도 고정 기대 배율이다.
    for (const auto& setting:scales) for (const auto frame:{0U,frameCount/2,frameCount-1}) { // 실제 첫/중간/끝 이미지와 여섯 배율·숨김 조건이다.
        Put(raw,0,0x50cc08);Put(raw,26,setting[0]);raw[8]=static_cast<std::uint8_t>(setting[1]);raw[9]=static_cast<std::uint8_t>(setting[1]>>8);Put(raw,36,frame);raw[40]=2;
        std::vector<std::uint8_t> actual(pitch*height,199),expected(actual);display.DrawEntry(entry,actual,pitch,height,clip,camera,1,0,{});
        if (!(setting[0]&0x4000U)) { const auto image=assets.game.Shapes().Decode(block,frame);const auto& header=assets.game.Shapes().Blocks()[block].frames[frame];DrawScaledVfxImage(expected,pitch,height,image,header,{512-core.types[103].hotspotX,384-core.types[103].hotspotY},static_cast<std::int32_t>(setting[2]),static_cast<std::int32_t>(setting[2]),clip); }
        CHECK(actual==expected);++cases;
    }
    for (const auto extra:{2U,0x12U}) for (const auto owner:{1U,2U}) for (const auto frame:{0U,frameCount/2,frameCount-1}) { // 고정 추상색은 outer 추상/소유자 값과 별도인 추가 그림이다.
        Put(raw,0,0x50db78);Put(raw,36,frame);raw[40]=static_cast<std::uint8_t>(extra);raw[34]=static_cast<std::uint8_t>(owner);std::vector<std::uint8_t> actual(pitch*height,199),expected(actual); // 합성 소유자 BYTE만 바꾸며 게임 소유 이전 사건은 실행하지 않는다.
        display.DrawEntry(entry,actual,pitch,height,clip,camera,1,0,{});const auto image=assets.game.Shapes().Decode(block,frame);const auto rect=assets.game.Shapes().Blocks()[block].frames[frame].rect;
        DrawIndexedImage(expected,pitch,height,image,512-core.types[103].hotspotX+rect.left,384-core.types[103].hotspotY+rect.top,clip,&maps.bright);CHECK(actual==expected);++cases;
    }
    CHECK(cases==30);
}
