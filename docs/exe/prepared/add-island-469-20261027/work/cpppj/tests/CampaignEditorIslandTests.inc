// 기존 Checkpoint 검사 TU 끝에 포함하며 같은 실제 Client/파일/Editor 설치 helper를 재사용한다.
#include "o/RawIslandPlacement.h"
#include "o/RawFortTerritorySave.h"

// 실제 설치 관문과 미리보기의 비월드 정체성,21행 seed,충돌 거부의 무변경을 검증한다.
TEST_CASE(CampaignEditorIsland_ActualPreviewChoicesAndRejectedDrop) {
    EditorInstallWireScene islandWire;std::shared_ptr<CampaignTemplateStorage> islandStorage;
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        ChunkMap preview(14,14);std::array<std::uint8_t,126> choices{};
        CHECK(CheckpointRejects<std::logic_error>([&] { static_cast<void>(session.BuildEditorIslandMap(preview)); }));
        CHECK(CheckpointRejects<std::logic_error>([&] { static_cast<void>(session.SeedEditorIslandChoices(choices)); }));
        RunEditorWireInstallState(session,client,checkpoint,work);session.CommitExistingEditorInstall(*client.LocalStateGlobals());
        const auto core=session.Core();const auto rawBefore=std::vector<std::uint8_t>(core.pool.Bytes().begin(),core.pool.Bytes().end());
        const auto countBefore=client.SurfaceStorage()->islands.records;const auto randomBefore=client.RandomGlobals()->Game().State();
        CHECK(session.BuildEditorIslandMap(preview));CHECK(core.pool.Bytes().size()==rawBefore.size() && std::equal(rawBefore.begin(),rawBefore.end(),core.pool.Bytes().begin()));
        CHECK(client.SurfaceStorage()->islands.records==countBefore && client.RandomGlobals()->Game().State()==randomBefore);
        auto expectedRandom=client.RandomGlobals()->Game();auto expectedChoices=choices;
        for (std::uint32_t index=0;index<21;++index) { //공통 seed helper를 같은 입력 난수 복사로 실행해 소비21회를 비교한다.
            auto record=std::span(expectedChoices).subspan(index*6U,6U);SeedFortTerritoryRecord(std::span<std::uint8_t,6>(record.data(),6),index,[&](auto limit) { return expectedRandom.Next(limit); });
        }
        CHECK(session.SeedEditorIslandChoices(choices) && choices==expectedChoices && client.RandomGlobals()->Game().State()==expectedRandom.State());
        const auto afterSeed=client.RandomGlobals()->Game().State();CHECK(session.SeedEditorIslandChoices(choices) && client.RandomGlobals()->Game().State()==afterSeed);
        std::array<std::uint8_t,6> record{};std::copy_n(choices.begin(),6,record.begin());record[2]=0x22;const auto unchanged=record;
        for (auto& chunk:preview.Entries()) chunk.island=0; //명시 충돌 지도이며 실제World 지도에는 쓰지 않는다.
        CHECK(!session.DropEditorIsland(preview,record) && record==unchanged && client.RandomGlobals()->Game().State()==afterSeed);
        CHECK(client.SurfaceStorage()->islands.records==countBefore && std::equal(rawBefore.begin(),rawBefore.end(),core.pool.Bytes().begin()));
        auto header=islandStorage->Headers().subspan(session.TemplateOwner()*20U,4);const auto manager=EditorWireDword(header,0);std::fill(header.begin(),header.end(),std::uint8_t{});
        choices.fill(0);const auto noManager=client.RandomGlobals()->Game().State();CHECK(!session.BuildEditorIslandMap(preview) && !session.SeedEditorIslandChoices(choices) && client.RandomGlobals()->Game().State()==noManager);
        for (std::size_t byte=0;byte<4;++byte) header[byte]=static_cast<std::uint8_t>(manager>>(byte*8U)); //원래 실제 Header를 복원해 정리 부모를 보존한다.
    },true,&islandWire,{},[&](Client&,RawCampaignSessionServices& services) { islandStorage=services.templateStorage; });
}

// 실제 기존 Editor의 합법 한 섬을 끝까지 배치하고 같은 Territory·Graph·Pool의 효과를 확인한다.
TEST_CASE(CampaignEditorIsland_ActualWorldPopulationGraphAndTerritory) {
    EditorInstallWireScene islandWire;std::shared_ptr<CampaignTemplateStorage> islandStorage;
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        RunEditorWireInstallState(session,client,checkpoint,work);session.CommitExistingEditorInstall(*client.LocalStateGlobals());
        ChunkMap preview(14,14);CHECK(session.BuildEditorIslandMap(preview));std::array<std::uint8_t,126> choices{};CHECK(session.SeedEditorIslandChoices(choices));
        auto core=session.Core();std::uint32_t piece=107;bool placed=false;std::uint32_t islandNumber=0;std::array<std::uint8_t,6> installed{};
        for (std::uint32_t row=0;row<21 && !placed;++row) { //원문 현재21후보를 순서대로 실제배치 가능한 위치만 찾는다.
            std::array<std::uint8_t,6> record{};std::copy_n(choices.begin()+row*6U,6,record.begin());
            for (std::uint32_t y=0;y<14 && !placed;++y) { //청크Y/X는 검사 입력 선택이며 원본UI 자동배치를 가장하지 않는다.
                for (std::uint32_t x=0;x<14 && !placed;++x) {
                    record[2]=static_cast<std::uint8_t>((y<<4U)|x);RawIslandBuilderState state{{0,0,14,14},session.TemplateOwner()};
                    if (!CanPlaceIsland(OriginalEdition::Patch1078,preview,core.frames,piece,state,record,[](auto) { throw std::logic_error("실제 섬 검사 입력의 원문 진단"); })) continue;
                    const auto& bytes=client.SurfaceStorage()->islands.records;islandNumber=EditorWireDword(bytes,0);CHECK(session.DropEditorIsland(preview,record));installed=record;placed=true;
                }
            }
        }
        CHECK(placed);const auto& list=client.SurfaceStorage()->islands.records;CHECK(EditorWireDword(list,0)==islandNumber+1U);
        const auto territory=islandStorage->Template(1).Section("Territory");CHECK(territory.size()==120);
        const auto actual=FortTerritoryIslandRecord(client.SurfaceStorage()->chunks,static_cast<std::int32_t>(islandNumber));CHECK(actual[1]&8U);
        if (islandNumber<20) CHECK(std::equal(actual.begin(),actual.end(),territory.begin()+islandNumber*6U)); //실제20행 저장 가능한 지역에서 같은 현재 레코드를 썼는지 확인한다.
        bool residence=false,priest=false;
        for (std::uint32_t number=2;number<core.pool.Capacity();++number) { //현재 새 섬의 실제거주구역/사제를 확인하며 단순 count 성공으로 대체하지 않는다.
            const auto raw=core.pool.Slot(Sid{static_cast<std::uint16_t>(number)});if (raw[11]&1U) continue;
            if (raw[10]!=89 && raw[10]!=158) continue;
            const auto center=core.neighbors.Center(Sid{static_cast<std::uint16_t>(number)});
            if (client.SurfaceStorage()->chunks.At(static_cast<int>(center[0])/16,static_cast<int>(center[1])/16).island!=islandNumber) continue;
            if (raw[10]==89) residence=true;if (raw[10]==158) priest=true;
        }
        CHECK(residence && priest && !(installed[1]&2U)); //동일원문세부slot/native확대 검증은 root의 실제빌드 뒤 추가한다.
    },true,&islandWire,{},[&](Client&,RawCampaignSessionServices& services) { islandStorage=services.templateStorage; });
}
