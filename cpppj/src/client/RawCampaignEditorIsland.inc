// 기존 Editor의 실제 지도·섬·Pool을 빌리는 Add Island 접합이다. 전체 창 생성은 별도 경계다.
// 실제 설치 완료와 기존 Editor 모드만 허용하며 State1 동기 UI 예외 범위를 넓히지 않는다.
bool RawCampaignSession::BuildEditorIslandMap(o::ChunkMap& target) {
    auto& s=*impl_;
    if (!s.loaded || s.services.installMode!=CampaignInstallMode::ExistingEditor || !s.globals.editor || !s.core.ownerMode.fort || s.core.ownerMode.battle)
        throw std::logic_error("Add Island의 실제 기존 Editor 설치 관문");
    if (&target==&s.chunks || target.XLength()!=14 || target.YLength()!=14) throw std::invalid_argument("Add Island의 독립14×14 미리보기 지도 계약");
    const auto owner=s.playerState.templateOwner;
    if (owner>=9) throw std::out_of_range("Add Island의 실제 Template9 범위");
    if (!Read(s.templateStorage->Headers(),owner*20U)) return false;
    std::array<std::uint8_t,120> records{};s.runtime->ReadTerritory(owner,records);
    target.Reset();o::RawIslandBuilderState state{{0,0,14,14},owner>=1 && owner<=8 ? owner : 0};
    o::RawIslandBuilder builder(target,s.chunks,*s.islands,s.core.frames,s.piece,state,SessionMissing<void,o::RawIslandBuilderAssertion>());
    builder.PlaceAll(records,0,20);return true;
}
// 원본 지도/로컬 지형/Bitmap을 같은 Client 최초 분류와 실제 자산으로 생성한다.
std::optional<o::TerrainBitmapImage> RawCampaignSession::BuildEditorIslandMapImage(o::ChunkMap& target) {
    if (!BuildEditorIslandMap(target)) return {};
    auto& s=*impl_;const auto owner=s.playerState.templateOwner;
    return client::BuildEditorIslandBitmap(s.assets.game,target,owner>=1 && owner<=8 ? owner : 0,{0,0,14,14},s.terrainGlobals,s.gameplayGlobals->TerrainBitmaps(),s.services.editorIslandGreen);
}
// 목록 이미지도 실제 임시 지도에 Place(ignorePosition)를 실행한 뒤 같은 renderer를 사용한다.
o::TerrainBitmapImage RawCampaignSession::BuildEditorIslandChoiceImage(std::span<const std::uint8_t,6> record) {
    o::ChunkMap map(14,14);BuildEditorIslandChoiceMap(record,map);auto& s=*impl_;
    return client::BuildEditorIslandBitmap(s.assets.game,map,0,{0,0,14,14},s.terrainGlobals,s.gameplayGlobals->TerrainBitmaps(),s.services.editorIslandGreen);
}
// 창 전용21행은 저장 Territory20행과 별개이며 비활성 행만 같은 공용 난수로 보충한다.
bool RawCampaignSession::SeedEditorIslandChoices(std::span<std::uint8_t,126> records) {
    auto& s=*impl_;
    if (!s.loaded || s.services.installMode!=CampaignInstallMode::ExistingEditor || !s.globals.editor || !s.core.ownerMode.fort || s.core.ownerMode.battle)
        throw std::logic_error("Add Island 선택의 실제 기존 Editor 설치 관문");
    const auto owner=s.playerState.templateOwner;
    if (owner>=9) throw std::out_of_range("Add Island 선택의 실제 Template9 범위");
    if (!Read(s.templateStorage->Headers(),owner*20U)) return false;
    for (std::uint32_t index=0;index<21;++index) { //0047f9b0의 모든 실제 행을 현재 flag로 재조회한다.
        auto record=records.subspan(index*6U,6U);
        if (!(record[1]&1U)) o::SeedFortTerritoryRecord(std::span<std::uint8_t,6>(record.data(),6),index,[&s](auto limit) { return s.random.Next(limit); });
    }
    return true;
}
// 목록 이미지의 지도 부분만 만든다. 원문004c1cc0 bitmap을 성공한 것으로 대신하지 않는다.
void RawCampaignSession::BuildEditorIslandChoiceMap(std::span<const std::uint8_t,6> record,o::ChunkMap& target) {
    auto& s=*impl_;
    if (!s.loaded || s.services.installMode!=CampaignInstallMode::ExistingEditor || !s.globals.editor || !s.core.ownerMode.fort || s.core.ownerMode.battle)
        throw std::logic_error("Add Island 단일 미리보기의 실제 기존 Editor 관문");
    if (&target==&s.chunks || target.XLength()!=14 || target.YLength()!=14) throw std::invalid_argument("Add Island 단일 독립14×14 지도 계약");
    target.Reset();o::RawIslandBuilderState state{{0,0,14,14},0};
    o::RawIslandBuilder builder(target,s.chunks,*s.islands,s.core.frames,s.piece,state,SessionMissing<void,o::RawIslandBuilderAssertion>());
    static_cast<void>(builder.Place(record,0,true));
}
// 같은 실제 배치 부모를 연결한다. 거부는 false이며 중간 하위 실패의 앞선 월드 효과는 보존한다.
bool RawCampaignSession::DropEditorIsland(const o::ChunkMap& preview,std::span<std::uint8_t,6> record) {
    auto& s=*impl_;
    if (!s.loaded || s.services.installMode!=CampaignInstallMode::ExistingEditor || !s.globals.editor || !s.core.ownerMode.fort || s.core.ownerMode.battle)
        throw std::logic_error("Add Island 드롭의 실제 기존 Editor 설치 관문");
    if (&preview==&s.chunks || preview.XLength()!=14 || preview.YLength()!=14) throw std::invalid_argument("Add Island 드롭의 독립14×14 지도 계약");
    auto singleHooks=MakeNormalTerrainFarmWorldHooks(s.world,s.chunks,*s.islands,*s.pipeline,s.hp);
    singleHooks.mayPlace=[&s](const o::CanonPlacementQuery& query) { return s.MayPlaceGameplay(query); }; // 현재 Editor/Player/동맹/섬·Graph를 읽는 실제 전체 MayPlace다. 생성 시 고정된 초기 관문을 재사용하지 않는다.
    o::RawFortRandomPlacement single(s.core.pool,s.core.hash,s.core.spots,s.core.types,s.core.frames,s.random,s.randomPlacement,std::move(singleHooks));
    o::FortFinishOwnerHooks finishHooks;
    finishHooks.regionBounds=[&s](auto region) { const auto b=s.islands->Bounds(region);return o::FortRandomRectangle{b.left,b.top,b.right,b.bottom}; };
    finishHooks.wallSeconds=[&s] { if (!s.services.sound.wallSeconds) throw std::logic_error("Add Island의 실제 벽시각 미연결");return s.services.sound.wallSeconds(); };
    finishHooks.invalidType=SessionMissing<void>();finishHooks=o::MakeFortSinglePlacementHook(s.core.pool,single,std::move(finishHooks));
    o::RawFortFinishOwner finish(s.core.pool,s.core.types,s.assetState.Residences(),s.finishState,std::move(finishHooks));finish.BindPlayerRecords(s.players);
    o::IslandPopulationHooks population;
    population.assertion=[](auto reason) { throw std::logic_error("Add Island 원문 population 진단:"+std::to_string(static_cast<std::uint32_t>(reason))); }; // 원문 선택된 진단을 숨기지 않고 지역/권한/배치 실패의 종류를 보존한다.
    population.wallSeconds=[&s] { if (!s.services.sound.wallSeconds) throw std::logic_error("Add Island 교체의 실제 벽시각 미연결");return s.services.sound.wallSeconds(); };
    population.position=[&s](auto sid) { return s.core.neighbors.Center(sid); };
    population.destroy=[&s](auto sid,auto flags) { s.world.Destroy(sid,flags); };
    population.create=[&s](auto type,auto flags) { return s.world.Create(type,flags); };
    population.defaultFrame=[&s](auto type) { if (type<70 || type-70>=s.assets.game.Types().size()) throw std::out_of_range("Add Island 현재 TYPE 기본 프레임 범위");return s.assets.game.Types()[type-70].definition.specialFrames.defaultFrame; };
    population.typeHitPoints=[&s](auto type) { return s.hp.TypeHitPoints(type); };population.hasHitPoints=[&s](auto sid) { return s.hp.HasHitPoints(sid); };
    population.setOwner=[&s](auto sid,auto owner) { s.ownership->SetOwner(sid,owner); };
    population.pop=[&s](auto sid,float x,float y,auto flags) { s.world.Pop(sid,x,y,flags); };
    population.refreshProduction=[&s] { if (!s.services.ownership.refreshProduction) throw std::logic_error("Add Island 실제 생산 통지 미연결");s.services.ownership.refreshProduction(); };
    o::RawIslandPopulation fill(s.core.pool,s.chunks,s.chunks,*s.islands,finish,
        {s.gameplayGlobals->Authority()->boss,s.core.ownerMode.fort,s.finishState.residenceType,s.tutorialArcherType,s.templeProduct,s.rewardState.types.priest},std::move(population));
    const auto owner=s.playerState.templateOwner;
    o::RawIslandBuilderState state{{0,0,preview.XLength(),preview.YLength()},owner>=1 && owner<=8 ? owner : 0};
    o::IslandAddHooks add;
    add.fortAssertion=SessionMissing<void>();add.generate=[&s](const auto& current,auto region) { s.normal->Generate({current.owner,current.bounds,region,NormalTerrainBranch::Normal}); };
    add.graph=[&s](bool reset) { s.core.graph.Rebuild(reset); };add.territory=[this](auto selected) { WriteFortTerritory(selected); };
    o::RawIslandAdd body(s.chunks,*s.islands,s.core.frames,s.piece,state,s.core.ownerMode.fort,fill,SessionMissing<void,o::RawIslandBuilderAssertion>(),std::move(add));
    return body.TryDrop(preview,record);
}
