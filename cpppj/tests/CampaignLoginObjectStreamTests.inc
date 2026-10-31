// 실제 단독 로그인 큐에서 객체의 선행 파싱 제거를 확인하며 게임/창/소켓은 실행하지 않는다.
#include "client/FortTypeConversionStorage.h"
#include "o/RawFortImage.h"
namespace { // 현재 전역 헤더의 포인터 토큰만 관측하며 manager/섹션을 선택하지 않는다.
std::uint32_t LoginManager667(CampaignTemplateStorage& storage) { // 로컬 owner1의 실제20BYTE 헤더 첫DWORD를 읽는다.
    const auto bytes=storage.Headers();return bytes[20]|(std::uint32_t(bytes[21])<<8)|(std::uint32_t(bytes[22])<<16)|(std::uint32_t(bytes[23])<<24);
}
}

TEST_CASE(CampaignLoginObjectStream_NormalReloadLeavesObjectViewsEmpty) { // 실제 첫 파일 설치는 State9 지역 읽기 뒤 서버 수신에서 수행한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto bytes=assets.files.Read("d/thewarbegins.fort");
    for (const bool physical:{false,true}) { // 논리 이름과 경로 모양의 manager 이름을 각각 명시 메모리 source로 확인한다.
        const auto chaff=FortTemplate::Parse(bytes,assets.game.TypeTable()).chaff;LocalStateSessionDevices devices;auto state=std::make_shared<ClientLocalStateGlobals>();auto services=devices.Make(state,true);auto storage=std::make_shared<CampaignTemplateStorage>();auto conversion=std::make_shared<FortTypeConversionStorage>();services.templateStorage=storage;services.fortTypeConversion=conversion;std::size_t reads{},titles{},rights{};
        services.missionFortFileSelect=[&](auto name) { CHECK(name=="thewarbegins");return MissionFortFileSelection{physical ? "d/current-667.fort" : "thewarbegins",[&] { ++reads;return bytes; }}; };
        services.missionFortLoad.readOnly=[&](auto) { ++rights;CHECK(reads==1 && storage->OwnedImages()==1);return false; }; // 지역 readonly Template는 권한을 조회하지 않으며 서버의 공개된 전역 manager에서만 호출한다.
        services.install.refreshWindowTitle=[&] { ++titles;CHECK(reads==2 && storage->Template(1).chaff.chunks.empty() && storage->Template(1).typeHashes.empty()); };
        RawCampaignSession session(assets.game,assets.files,FortTemplate{},"thewarbegins",std::move(services));CHECK(reads==0 && !storage->OwnedImages());session.LoadLocalConnection();
        CHECK(reads==2 && rights==1 && titles>0 && session.LocalConnectionPhase()==14 && session.LocalConnectionReady() && session.HasCursorRuntimeForState());
        CHECK(CampaignStreamStoredPositions(session,chaff)==145 && storage->Template(1).chaff.chunks.empty() && conversion->Current()==0 && session.FortObjectSettings().loadingDepth==0);
        CHECK(storage->Name(1)==(physical ? "d/current-667.fort" : "thewarbegins")); // 논리 선택과 서버가 실제 읽은 파일 이름은 별개다.
    }
}

TEST_CASE(CampaignLoginObjectStream_BadChaffFailsAfterActualTerritory) { // 재읽기는 성공하고 실제 Objects 단계가 부분 지형/표/cursor를 보존한 채 실패해야 한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto initial=assets.files.Read("d/thewarbegins.fort");
    for (const bool physical:{false,true}) { // 같은 손상 바이트를 두 manager 이름의 독립 Session에서 소비하며 실제 디스크 읽기로 주장하지 않는다.
        auto fort=FortTemplate::Parse(initial,assets.game.TypeTable());auto image=fort;image.sections.at(CampaignStreamSection("Chaff"))[1]=0;const auto encoded=RawFortImage(OriginalEdition::Patch1078,image,{[&] { return static_cast<std::uint32_t>(assets.types.size()); },{}}).Save();const std::vector<std::uint8_t> bad(encoded.Bytes().begin(),encoded.Bytes().end());
        LocalStateSessionDevices devices;auto state=std::make_shared<ClientLocalStateGlobals>();auto services=devices.Make(state,true);auto storage=std::make_shared<CampaignTemplateStorage>();auto conversion=std::make_shared<FortTypeConversionStorage>();services.templateStorage=storage;services.fortTypeConversion=conversion;std::size_t reads{},titles{};
        services.missionFortFileSelect=[&](auto name) { CHECK(name=="thewarbegins");return MissionFortFileSelection{physical ? "d/bad-667.fort" : "thewarbegins",[&] { ++reads;return bad; }}; };
        services.missionFortLoad.readOnly=[](auto) { return false; };services.install.refreshWindowTitle=[&] { ++titles;CHECK(reads==2 && storage->Template(1).chaff.chunks.empty()); };
        RawCampaignSession session(assets.game,assets.files,FortTemplate{},"thewarbegins",std::move(services));CHECK(!storage->OwnedImages() && !reads);bool rejected{};try { session.LoadLocalConnection(); }catch (const std::logic_error& error) { rejected=std::string_view(error.what())=="원본 Objects Template.cpp 진단 890"; }
        CHECK(rejected && reads==2 && titles>0 && session.IslandCount()>0 && !session.World().Objects().empty());CHECK(!session.LocalConnectionReady() && session.LocalConnectionPhase()!=14 && !session.HasCursorRuntimeForState());
        CHECK(conversion->Current()!=0 && session.FortObjectSettings().loadingDepth==1 && storage->CurrentSection(1)=="Chaff");std::uint32_t position{};
        storage->AccessSection(storage->CurrentWriteStream(1),[&](auto fields,const auto&) { for (std::size_t byte=0;byte<4;++byte) position|=std::uint32_t(fields[4+byte])<<(8*byte); });CHECK(position==2); // 버전과 잘못된 표지를 읽은 같은 현재 cursor다.
    }
}

TEST_CASE(CampaignLoginObjectStream_ExistingManagerSkipsServerReadAndKeepsCurrentImage) { // 지역 로그인 이미지는 따로 읽되 전역 manager의 같은 이름/다른 이름 모두 원문 보존 분기를 사용한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto bytes=assets.files.Read("d/thewarbegins.fort");
    for (const bool different:{false,true}) { // CRT 대소문자 일치와 진단 뒤 계속의 두 원문 분기를 실제 Session으로 확인한다.
        LocalStateSessionDevices devices;auto state=std::make_shared<ClientLocalStateGlobals>();auto services=devices.Make(state,true);auto storage=std::make_shared<CampaignTemplateStorage>();services.templateStorage=storage;storage->InstallFile(1,{},bytes,"Existing.fort");const auto manager=LoginManager667(*storage);std::size_t reads{},rights{},diagnostics{};
        services.missionFortFileSelect=[&](auto name) { CHECK(name=="thewarbegins");return MissionFortFileSelection{different ? "Other.fort" : "eXISTING.FORT",[&] { ++reads;return bytes; }}; };
        services.missionFortLoad.readOnly=[&](auto) { ++rights;return false; };services.missionFortLoad.wrongName=[&] { ++diagnostics;CHECK(LoginManager667(*storage)==manager && reads==1); };
        RawCampaignSession session(assets.game,assets.files,{},"thewarbegins",std::move(services));CHECK(LoginManager667(*storage)==manager && !reads);session.LoadLocalConnection();
        CHECK(session.LocalConnectionReady() && reads==1 && !rights && diagnostics==(different ? 1U : 0U));CHECK(storage->OwnedImages()==1 && LoginManager667(*storage)==manager && storage->Name(1)=="Existing.fort");
    }
}

TEST_CASE(CampaignLoginObjectStream_StrippedTerritoryReadsSharedDwordWithoutByteTruncation) { // CoreData의BYTE 대여와 별개로 strip은005c85d0의 상위24비트도 읽어야 한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto bytes=assets.files.Read("d/thewarbegins.fort");LocalStateSessionDevices devices;auto state=std::make_shared<ClientLocalStateGlobals>();state->BattleTerritory()=0x12340000U;state->BattleByte()=7;
    CHECK(state->BattleTerritory()==0x12340007U);auto storage=std::make_shared<CampaignTemplateStorage>();auto services=devices.Make(state,true);services.templateStorage=storage;std::size_t reads{};
    services.missionFortFileSelect=[&](auto) { return MissionFortFileSelection{"source.fort",[&] { ++reads;return bytes; }}; };RawCampaignSession session(assets.game,assets.files,{},"thewarbegins",std::move(services));bool failed{};
    try { session.LoadLocalConnection(); }catch (const std::logic_error& error) { failed=std::string_view(error.what())=="로그인이미지복제의원문섹션누락진단"; }
    CHECK(failed && reads==1 && !storage->OwnedImages() && !session.LocalConnectionReady() && state->BattleTerritory()==0x12340007U);
}
