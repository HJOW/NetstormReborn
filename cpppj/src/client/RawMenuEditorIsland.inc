// 같은 Impl의 실제 장식/글자/이미지/목록과 현재 Session으로 섬 추가 창을 조립한다.
EditorIslandWindowNode& Island(std::uint32_t token) const { auto* node=Find(islandRoots,token);if (!node) throw std::out_of_range("섬 추가 실제 창 없음");return *node; } // 공통 저장소의 같은 부모를 반환한다.
// 현재 창의 장치는 같은 Session의 살아 있는 기간에만 빌린다.
const EditorIslandWindowContext& IslandContext(std::uint32_t token) const { return islandContexts.at(token); }
// 새 MetaImage의 비소유 Image 주소를 실제 소유 토큰으로 조회한다.
std::uint32_t IslandImageToken(const RawImage* image) const {
    if (!image) return 0;for (const auto& [token,value]:islandImages) if (value.get()==image) return token; // 같은 Image 객체의 주소만 비교한다.
    throw std::logic_error("섬 추가 후보 Image의 실제 소유자 없음");
}
// 원본 Refresh는 두 번, 성공 drop은 한 번 Destroy 후 같은 Image 할당을 반납한다.
void ReleaseIslandImage(std::uint32_t token,std::uint32_t destroys) {
    auto& image=*islandImages.at(token);for (std::uint32_t index=0;index<destroys;++index) image.Destroy(); // 원문 중복 Destroy도 호출 순서대로 수행한다.
    islandImages.erase(token);
}
// 원문 창/후보에서 만든 실제 MetaImage는 상태 이탈 정리에 참여한다.
std::uint32_t CreateIslandMeta(RawImage& image) { const auto token=CreateMeta(image);islandOwned.insert(token);return token; }
// 현재 창의 실제 가상 배치/미리보기를 같은 공통 저장소에 짧게 연결한다.
EditorIslandLayout IslandLayout(std::uint32_t token) const {
    EditorIslandLayoutHooks h;h.fields=[this](auto current)->EditorIslandLayoutFields& { return Island(current).layout; };h.node=common.node;h.image=[this](auto current) { return Find(metaImages,current); };
    h.resizeImage=[this](auto current) { metaImage->Resize(current); };h.push=[this](auto current) { common.tree.Push(current); };h.baseAttach=[this](auto current,float x,float y,auto parent,auto mode) { common.tree.Attach(current,x,y,parent,mode); };h.attach=common.attach;
    h.bounds=[this](auto current) { return common.collection.Bounds(current); };h.position=[this](auto current) { return common.collection.Position(current); };h.root=[this] { return common.create.collection.root; };
    h.canPlace=[this,token](auto record) { const auto& node=Island(token);return IslandContext(token).session->CanPlaceEditorIsland(*node.preview,record); };h.assertion=[](auto text) { throw std::logic_error(std::string("섬 추가 원문 진단:")+text); };return EditorIslandLayout(std::move(h));
}
//0047f450의 실제 자식 생성/문맥/초기 부착과 원본 참조 수다.
void ConstructIsland(std::uint32_t token) {
    auto& node=Island(token);const auto& context=IslandContext(token);decoration->Construct(token,432,412);node.vtable=0x50d308;node.currentImage.Destroy();node.preview.emplace(14,14);node.interaction.choices.fill(0);node.interaction.record.fill(0);
    if (islandReferences) throw std::logic_error("섬 추가 원문 refCount == 0 진단");++islandReferences;node.counted=true;node.command=4;node.layout.image=&node.currentImage;node.layout.interaction=&node.interaction;node.interaction.sharedFlags=&node.flags;
    const auto background=common.token();backgrounds.emplace(background,std::make_unique<GumpBackgroundNode>());islandOwned.insert(background);backgroundCreate->Construct(background,1,1,0x03e40000U,1,127,0);node.layout.background=background;
    node.layout.map=CreateIslandMeta(node.currentImage);common.node(node.layout.map)->alignFlags|=0xc;
    const auto hidden=Allocate(0xd8,0);islandOwned.insert(hidden);titleWidgets->Construct(hidden,0,0,0,0);node.layout.hidden=hidden;
    const auto heading=Allocate(0xd8,0);islandOwned.insert(heading);titleWidgets->Construct(heading,0,0,0,0);node.layout.title=heading;
    const auto list=common.token();imageLists.emplace(list,std::make_unique<MetaImageListNode>());islandOwned.insert(list);imageList->Construct(list,4);node.layout.list=list;
    const auto done=AllocateHelp(0x170);islandOwned.insert(done);const auto doneText=context.translate("Done");common.stones.Construct(done,0,0,doneText.c_str(),palette.named.at(9).index);node.layout.done=done;common.stones.FitToText(done);
    node.layout.tell=Reference(context.tell("PlacingIslands"));if (!node.layout.tell) throw std::logic_error("섬 추가 원문 PlacingIslands HtmlBody serial 조회 실패");islandOwned.insert(node.layout.tell);
    common.node(hidden)->textContext={2,0,palette.basic[7],0,0,palette.basic[3],1,1};title->ReplaceText(hidden,std::span("",1),0);title->RequestScroll(hidden,0);common.node(hidden)->flags&=~1U;
    title->ReplaceText(heading,std::span("",1),0);common.node(heading)->textContext={2,3,palette.basic[7],0,0,0,1,1};titleMarkup->Parse(heading,context.translate("Place New Islands\n"));title->RequestScroll(heading,0);title->FitContent(heading,0,0);common.node(heading)->alignFlags|=4;
    Attach(node.layout.title,0,0,token,0);Attach(node.layout.background,0,0,token,0);Attach(node.layout.map,0,0,node.layout.background,0);Attach(node.layout.hidden,0,0,token,0);Attach(node.layout.list,0,0,token,0);Attach(node.layout.done,0,0,token,0);common.node(node.layout.done)->alignFlags|=4;Attach(node.layout.tell,0,0,token,0);
    node.flags=(node.flags&~0x200U)|0x800U;common.lifetime.SetIdle(token,true);node.interaction.drag=0;node.interaction.dirty=1;
}
//00480090은 실제 생성 뒤 가운데 정렬과 전체 상대 깊이만 설정한다.
std::uint32_t CreateIsland(EditorIslandWindowContext context) {
    if (assets.Edition()!=o::OriginalEdition::Patch1078 || !context.session || !context.translate || !context.tell || !context.random || !context.camera || !context.revision || !context.pulseRegistered) throw std::invalid_argument("섬 추가 창의 실제 Client/Session 장치 미연결");
    const auto token=common.token();islandRoots.emplace(token,std::make_unique<EditorIslandWindowNode>());islandContexts.emplace(token,std::move(context));islandOwned.insert(token);ConstructIsland(token);auto& node=Island(token);node.alignFlags|=0xc;common.tree.AddDepth(token,0U-80U-node.depth);return token;
}
//0047f9b0은 manager를 먼저 조회하고 현재 Image/목록/각 후보를 원문 순서대로 교체한다.
void RefreshIsland(std::uint32_t token) {
    auto& node=Island(token);const auto& context=IslandContext(token);node.ready=0;if (!context.session->EditorIslandManagerAvailable()) return;
    node.currentImage.Destroy();auto map=context.session->BuildEditorIslandMapShape(*node.preview);if (!map) throw std::logic_error("섬 추가 지도 생성 중 실제 manager 소실");node.currentImage=std::move(*map);if (node.currentImage.Shape().empty()) throw std::logic_error("섬 추가 원문 currentImage.shape != NULL 진단");
    while (Find(imageLists,node.layout.list)->count>0) { // 현재 첫 Image를 반납하고 실제 목록의 Kill/압축을 수행한다.
        if (auto* image=imageList->Image(node.layout.list,0)) ReleaseIslandImage(IslandImageToken(image),2);imageList->Remove(node.layout.list,0);
    }
    for (std::uint32_t index=0;index<21;++index) { // 원문 각 행의 seed→새 Image→crop1→실제 Add 순서를 유지한다.
        auto record=std::span<std::uint8_t,6>(node.interaction.choices.data()+index*6U,6);if (!(record[1]&1U)) o::SeedFortTerritoryRecord(record,index,context.random);
        const auto imageToken=common.token();auto [entry,added]=islandImages.emplace(imageToken,std::make_unique<RawImage>());static_cast<void>(added);*entry->second=context.session->BuildEditorIslandChoiceShape(record);
        if (entry->second->Shape().empty()) throw std::logic_error("섬 추가 원문 im->shape != NULL 진단");
        imageList->Add(node.layout.list,*entry->second,index);islandOwned.insert(Find(imageLists,node.layout.list)->images[Find(imageLists,node.layout.list)->count-1]);node.ready=1;
    }
    common.tree.Push(token);Attach(token,node.x,node.y,0,0);
}
//00480130은 shape가 생긴 뒤에만 dirty를 지우고 ready일 때 원문 버튼 점멸 관문을 호출한다.
void IdleIsland(std::uint32_t token) {
    auto& node=Island(token);if (!node.interaction.dirty) return;RefreshIsland(token);if (!node.currentImage.Shape().empty()) { Redraw(token,false);node.interaction.dirty=0; }if (node.ready) IslandContext(token).pulseRegistered(islandPulseId);
}
// 기존0047fb90을 실제 창/목록/Image/World와 같은 저장소에 연결한다.
std::uint32_t HandleIsland(std::uint32_t token,InputEvent event) {
    auto& node=Island(token);const auto& context=IslandContext(token);EditorIslandDragHooks h;
    h.closeSerial=[&] { return common.node(node.layout.done)->serial; };h.killParent=[&] { common.lifetime.Kill(token); };h.pushParent=[&] { if (!common.life.focus) common.life.focus=token; };h.assertion=[](auto text) { throw std::logic_error(std::string("섬 추가 원문 진단:")+text); };
    h.image=[&](auto index) { return IslandImageToken(imageList->Image(node.layout.list,index)); };h.selection=[&] { return Find(imageLists,node.layout.list)->selection; };h.recordIndex=[&](auto index) { return std::bit_cast<std::int32_t>(imageList->Id(node.layout.list,index)); };
    h.createDrag=[&](auto image) { return CreateIslandMeta(*islandImages.at(image)); };h.adjustDrag=[&](auto drag) { metaImage->Resize(drag); };h.killDrag=[&](auto drag) { common.lifetime.Kill(drag); };h.size=[&](auto drag) { const auto* current=common.node(drag);return std::array{current->baseWidth,current->baseHeight}; };
    h.attachDrag=[&](auto drag,float x,float y) { Attach(drag,x,y,common.create.collection.root,0);common.node(drag)->depth=0xffffff9cU; };
    const auto bounds=[&](auto current) { const auto value=common.collection.Bounds(current);return std::array{value.left,value.top,value.right,value.bottom}; };h.parentBounds=[&] { return bounds(token); };h.listBounds=[&] { return bounds(node.layout.list); };h.mapBounds=[&] { return bounds(node.layout.map); };h.mapSize=[&] { return std::array{node.preview->XLength(),node.preview->YLength()}; };
    h.drop=[&](auto record) { return context.session->DropEditorIsland(*node.preview,record); };h.camera=context.camera;h.releaseImage=[&](auto image) { ReleaseIslandImage(image,1); };h.remove=[&](auto index) { imageList->Remove(node.layout.list,index); };h.clearSelectionAndUpdate=[&] { Find(imageLists,node.layout.list)->selection=-1;Redraw(node.layout.list,false); };h.incrementRevision=context.revision;
    return HandleEditorIslandDrag(node.interaction,event,h) ? 1U : 0U;
}
//0047f940/00480110은 후보 Image를 소유하지 않으며 현재 Image와 실제 참조 수/기본 부모만 반납한다.
void DestructIsland(std::uint32_t token) {
    auto& node=Island(token);node.vtable=0x50d308;node.currentImage.Destroy();if (node.counted) { --islandReferences;node.counted=false; }node.currentImage.Destroy();common.lifetime.DestructBase(token);
}
