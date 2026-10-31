// 실제 단독 로그인 큐에서 객체의 선행 파싱 제거를 확인하며 게임/창/소켓은 실행하지 않는다.
#include "client/FortTypeConversionStorage.h"
#include "o/RawFortImage.h"

TEST_CASE(CampaignLoginObjectStream_NormalReloadLeavesObjectViewsEmpty) { // 파일 이름을 보유한 reader와 기존 BYTE reader가 같은 원시 객체 부모를 사용한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto bytes=assets.files.Read("d/thewarbegins.fort");
    for (const bool physical:{false,true}) { // 현행 두 재읽기 장치만 바꾸며 실제 로그인/월드 결과를 대신 만들지 않는다.
        auto fort=FortTemplate::Parse(bytes,assets.game.TypeTable());const auto chaff=fort.chaff;LocalStateSessionDevices devices;auto state=std::make_shared<ClientLocalStateGlobals>();auto services=devices.Make(state,true);auto storage=std::make_shared<CampaignTemplateStorage>();auto conversion=std::make_shared<FortTypeConversionStorage>();services.templateStorage=storage;services.fortTypeConversion=conversion;std::size_t reads{},titles{};
        services.missionFortRead=[&](auto name) { CHECK(!physical && name=="thewarbegins");++reads;return bytes; };
        if (physical) services.missionFortFileRead=[&](auto name) { CHECK(name=="thewarbegins");++reads;return MissionFortSourceImage{bytes,"d/current-666.fort"}; };
        services.install.refreshWindowTitle=[&] { ++titles;CHECK(reads==1 && storage->Template(1).chaff.chunks.empty() && storage->Template(1).typeHashes.empty()); }; // 원문 World 제목 위치에서 미리 해석한 객체/TypeNames 사본이 없는지 확인한다.
        RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.LoadLocalConnection();
        CHECK(reads==1 && titles>0 && session.LocalConnectionPhase()==14 && session.LocalConnectionReady() && session.HasCursorRuntimeForState());
        CHECK(CampaignStreamStoredPositions(session,chaff)==145 && storage->Template(1).chaff.chunks.empty() && conversion->Current()==0 && session.FortObjectSettings().loadingDepth==0);
        CHECK(storage->Name(1)==(physical ? "d/current-666.fort" : "thewarbegins")); // 논리 선택과 이번에 읽은 실제 이름을 기존 계약대로 구별한다.
    }
}

TEST_CASE(CampaignLoginObjectStream_BadChaffFailsAfterActualTerritory) { // 재읽기는 성공하고 실제 Objects 단계가 부분 지형/표/cursor를 보존한 채 실패해야 한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto initial=assets.files.Read("d/thewarbegins.fort");
    for (const bool physical:{false,true}) { // 같은 손상 입력을 두 기존 reader에서 독립 Session으로 소비한다.
        auto fort=FortTemplate::Parse(initial,assets.game.TypeTable());auto image=fort;image.sections.at(CampaignStreamSection("Chaff"))[1]=0;const auto encoded=RawFortImage(OriginalEdition::Patch1078,image,{[&] { return static_cast<std::uint32_t>(assets.types.size()); },{}}).Save();const std::vector<std::uint8_t> bad(encoded.Bytes().begin(),encoded.Bytes().end());
        LocalStateSessionDevices devices;auto state=std::make_shared<ClientLocalStateGlobals>();auto services=devices.Make(state,true);auto storage=std::make_shared<CampaignTemplateStorage>();auto conversion=std::make_shared<FortTypeConversionStorage>();services.templateStorage=storage;services.fortTypeConversion=conversion;std::size_t reads{},titles{};
        services.missionFortRead=[&](auto name) { CHECK(!physical && name=="thewarbegins");++reads;return bad; };
        if (physical) services.missionFortFileRead=[&](auto name) { CHECK(name=="thewarbegins");++reads;return MissionFortSourceImage{bad,"d/bad-666.fort"}; };
        services.install.refreshWindowTitle=[&] { ++titles;CHECK(reads==1 && storage->Template(1).chaff.chunks.empty()); };
        RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));bool rejected{};try { session.LoadLocalConnection(); }catch (const std::logic_error& error) { rejected=std::string_view(error.what())=="원본 Objects Template.cpp 진단 890"; }
        CHECK(rejected && reads==1 && titles>0 && session.IslandCount()>0 && !session.World().Objects().empty());CHECK(!session.LocalConnectionReady() && session.LocalConnectionPhase()!=14 && !session.HasCursorRuntimeForState());
        CHECK(conversion->Current()!=0 && session.FortObjectSettings().loadingDepth==1 && storage->CurrentSection(1)=="Chaff");std::uint32_t position{};
        storage->AccessSection(storage->CurrentWriteStream(1),[&](auto fields,const auto&) { for (std::size_t byte=0;byte<4;++byte) position|=std::uint32_t(fields[4+byte])<<(8*byte); });CHECK(position==2); // 버전과 잘못된 표지를 읽은 같은 현재 cursor다.
    }
}
