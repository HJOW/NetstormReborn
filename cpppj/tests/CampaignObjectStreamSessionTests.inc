// 창/소켓/스테이지 대기 없이 실제 캠페인 Session이 현재 원시 섹션을 읽는지 확인한다.
#include "client/CampaignTemplateStorage.h"
#include "client/FortTypeConversionStorage.h"
#include "client/Mission.h"

namespace { // 저장 바이트의 독립 해석은 기대 좌표에만 사용하고 실제 생성 결과를 만들지 않는다.
std::size_t CampaignStreamStoredPositions(RawCampaignSession& session,const FortChunkSection& chaff) { // Chaff의 저장 y/x·nibble 좌표를 현재 월드에서 조회한다.
    std::size_t matched{}; // 실제 비교한 비섬 레코드 수이며 자동 지형 생성 수와 구별한다.
    for (std::size_t chunk=0;chunk<chaff.chunks.size();++chunk) { // 저장 Chaff는 전체16×16 청크 순서다.
        for (const auto& object:chaff.chunks[chunk].objects) { // 각 원본 레코드가 가리키는 종류/좌표를 확인한다.
            if (!object.type || static_cast<std::uint32_t>(object.type)==session.Core().bridgeState.islandType) continue;
            const auto x=static_cast<float>((chunk%16)*16+object.CellX()),y=static_cast<float>((chunk/16)*16+object.CellY());const auto found=session.World().Find(x,y,static_cast<std::uint32_t>(object.type));CHECK(found.value!=0);++matched;
        }
    }return matched;
}
std::size_t CampaignStreamSection(std::string_view name) { // 파일의 고정 섹션 위치만 찾으며 읽기 결과나 상태를 계산하지 않는다.
    const auto names=FortTemplate::SectionNames();const auto at=std::find(names.begin(),names.end(),name);if (at==names.end()) throw std::logic_error("캠페인 stream 검사 섹션 이름 없음");return static_cast<std::size_t>(at-names.begin());
}
}
TEST_CASE(CampaignObjectStream_CurrentBytesWinOverClearedObjectViews) { // Early/Training/Campaign/Demo의 실제 파일을 창 없이 읽고 사본 우회가 없음을 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);std::size_t index{};const std::array<std::size_t,4> storedCounts{145,0,390,24}; // 원본 Chaff의 별도 정적 해석 수다. Tutorial1의 BridgeTheGap에는 저장 객체가 없다.
    for (const auto mission:{"TheWarBegins","Tutorial1","bc8fury76","DemoStormRages"}) { // 네 경로의 첫 설치만 하며 전투 프레임/Start/클리어는 실행하지 않는다.
        ConfigInterface config([&](auto path) { return assets.files.TryRead(path); });config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}/{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}/{local.1}.fort\"\n[END]\n");MissionScript script(config,mission);CHECK(script.Loaded());
        auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());const auto expected=fort.chaff;LocalStateSessionDevices devices;auto state=std::make_shared<ClientLocalStateGlobals>();auto services=devices.Make(state,false);auto storage=std::make_shared<CampaignTemplateStorage>();auto conversion=std::make_shared<FortTypeConversionStorage>();services.templateStorage=storage;
        const bool shared=(index++%2)==0;if (shared) services.fortTypeConversion=conversion;std::size_t titles{};services.install.refreshWindowTitle=[&] {
            ++titles;auto& current=storage->Template(1);current.chaff={};for (auto& region:current.territories) region={}; // 실제 원문 제목 위치에서 편의 사본만 비우고 raw 섹션/노드는 유지한다.
            current.typeHashes.clear();current.hasTypeConversion=false;current.typeConversion.fill(0);
        };
        RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),std::move(services));session.Load();CHECK(titles==1 && session.IslandCount()>0 && session.FortObjectSettings().loadingDepth==0);
        CHECK(CampaignStreamStoredPositions(session,expected)==storedCounts[index-1]);CHECK(storage->Template(1).chaff.chunks.empty() && storage->CurrentSection(1)=="Chaff");if (shared) CHECK(conversion->Current()==0); // 정상 꼬리의 현재표 반납과 편의 사본 미보충을 함께 확인한다.
        const auto token=storage->CurrentWriteStream(1);std::uint32_t position{};storage->AccessSection(token,[&](auto fields,const auto&) { for (std::size_t byte=0;byte<4;++byte) position|=std::uint32_t(fields[4+byte])<<(8*byte); });CHECK(position>1); // 실제 같은 Chaff 노드의 읽은 cursor다.
    }
}
TEST_CASE(CampaignObjectStream_BadChaffKeepsTerritoryAndCurrentConversion) { // 편의 사본이 정상이어도 손상된 실제 바이트에서 중단하고 부분 월드/표를 보존한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto& chaff=fort.sections.at(CampaignStreamSection("Chaff"));CHECK(chaff.size()>1);chaff[1]=0; // 실제 저장 표지만 손상시키며 기존 해석 사본은 정상으로 둔다.
    LocalStateSessionDevices devices;auto state=std::make_shared<ClientLocalStateGlobals>();auto services=devices.Make(state,false);auto storage=std::make_shared<CampaignTemplateStorage>();auto conversion=std::make_shared<FortTypeConversionStorage>();services.templateStorage=storage;services.fortTypeConversion=conversion;
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));bool rejected{};try { session.Load(); }catch (const std::logic_error& error) { rejected=std::string_view(error.what())=="원본 Objects Template.cpp 진단 890"; }
    CHECK(rejected && conversion->Current()!=0 && session.FortObjectSettings().loadingDepth==1 && session.IslandCount()>0 && !session.World().Objects().empty());CHECK(storage->CurrentSection(1)=="Chaff");
    std::uint32_t position{};storage->AccessSection(storage->CurrentWriteStream(1),[&](auto fields,const auto&) { for (std::size_t byte=0;byte<4;++byte) position|=std::uint32_t(fields[4+byte])<<(8*byte); });CHECK(position==2); // 버전/표지를 읽은 바로 그 위치이며 정상 꼬리로 우회하지 않는다.
    bool blocked{};try { static_cast<void>(session.Cursor()); }catch (const std::logic_error&) { blocked=true; }CHECK(blocked); // 실패한 로딩을 사용자 입력 가능한 상태로 공개하지 않는다.
}
