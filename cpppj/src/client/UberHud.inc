// 같은 UberGump의 실제 장기 화면/지도 전역과 전체 HUD 설치를 연결한다.
struct UberGump::HudRuntime {
    std::int32_t width{},height{},left{},top{},mapWidth{},mapHeight{}; // 실제 화면/지도 원점·폭높이를 같은 도킹 몸체가 쓴다.
    std::uint32_t shown{1},fullRefresh{};float chatY{}; // 현재 미션 HUD 표시와 전체 갱신/이전 채팅 위치다.
    std::array<float,4> floatRect{};ScreenRect leftBottom{}; // 정수 지도LTRB는 공통 GumpTree 소유자를 직접 빌리고 float/제외 영역만 여기에 둔다.
    std::array<std::uint8_t,256> rankOutput,rankTemporary; // 원문의 미작성 임시 BYTE를 임의0으로 채우지 않는다.
    std::string paletteProbe;bool paletteFound{}; // 실제 고정9인자 경로의 현재 결과와 파일 존재 관측이다.
};
// 현재 실제 네 공장과 같은 등록표의 전체 루트를 설치한다. State1의 원문 정리/prefix를 대신하지 않는다.
void UberGump::InstallHud() {
    if (!world_ || client_.Assets().Edition()!=o::OriginalEdition::Patch1078) return;
    if (!hud_) hud_=std::make_unique<HudRuntime>();auto& value=*hud_;
    value.width=client_.GetScreen().Width();value.height=client_.GetScreen().Height();
    auto& menus=*rawButtons_->Menus();
    if (!rawButtons_->UberRoot()) {
        RawBackgroundGifContext gif;gif.files=&client_.Files();gif.config=&client_.Configuration();
        gif.dispatch=[this](auto event,auto first,auto second) { client_.Input().Push(event,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)); };
        menus.EnableBackgroundGifs(std::move(gif));
        RawUberRootContext context{{value.width,value.height,value.shown,value.left,value.top,value.mapWidth,value.mapHeight,value.floatRect,rawButtons_->MapViewRect(),value.leftBottom,value.fullRefresh,value.chatY},{},{},{},{},{}};
        context.poll=[this](auto code) { return client_.Poll(code); };context.cursor=[this](auto cursor) { client_.GetCursor().Set(static_cast<int>(cursor)); };
        context.root.read=[this](auto address) -> std::uint32_t { switch (address) {
            case 0x5411c8:return static_cast<std::uint32_t>(client_.Assets().Find("fortGump").block+o::kFirstAssetTypeNumber);
            case 0x565e70:return static_cast<std::uint8_t>(htmlGlobals_.background[0]);
            case 0x531860:return static_cast<std::uint32_t>(client_.GetScreen().Width());case 0x531864:return static_cast<std::uint32_t>(client_.GetScreen().Height());
            case 0x540cec:return 0x503c8b;case 0x540cf8:return 0x508ab4;case 0x542a04:return 0x020000c4U;
            default:throw std::logic_error("전체 Uber 실제 Client 전역 미연결:"+std::to_string(address));
        } };
        context.root.type=[this](auto type) { if (type>=client_.Assets().TypeTable().Types().size()) throw std::out_of_range("전체 Uber 실제 TYPE 범위");return 0x100000U+type; };
        context.root.createMinimap=[this] { auto& mini=rawButtons_->ConstructMinimap(client_.SurfaceStorage(),RawMinimapHooks{
            [this] { return world_->MinimapView(); },[this](auto x,auto y) { world_->CenterCamera(x,y); },[this](auto x,auto y) { world_->MoveFromMinimap(x,y); },
            [this] { return world_->InputMode(); },[this] { return client_.Poll(0xffff); }});mini.BindWorld(world_->Session().Core());return mini.RootToken(); };
        context.root.createProduction=[this] { auto& panel=rawButtons_->ConstructProduction(world_->Session().ProductionContext(),client_.Assets(),RawProductionHooks{
            [this] { return client_.WallSeconds(); },[this](auto name) { client_.WorldAudio().Sounds().PlayByName(name,0,0,0,0,0); },[this](auto name) { state_.Post({"Tell",std::string(name)}); },
            [this](auto key,std::int32_t fallback) { client_.Configuration().ReadInt(key,fallback);return fallback; },[this](auto key) { return FontBytes(client_.Translate(o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(key.data()),key.size()}))); },
            [this](const InventoryItemNode& item,ScreenPoint point,const std::function<bool(o::Sid)>& affordable) { return objectMenus_->OpenProduction(*world_,item,point,affordable); },
            [this](auto code) { return client_.Poll(code); },[this](auto cursor) { client_.GetCursor().Set(static_cast<int>(cursor)); },[this] { return client_.Time().game; }});return panel.RootToken(); };
        context.root.createKnowledge=[this] {
            RawNsInfoContext data;data.globals=client_.NsInfoGlobals();data.gameplay=client_.GameplayGlobals();data.storage=client_.SurfaceStorage();
            data.read=[this](auto address) { if (address==0x5411c8U) return static_cast<std::uint32_t>(client_.Assets().Find("fortGump").block+o::kFirstAssetTypeNumber);if (address==0x5bec3cU) return static_cast<std::uint32_t>(client_.GetScreen().Colors().basic[7]);throw std::out_of_range("전체 Uber NSInfo 현재 전역"); };
            data.translate=[this](auto text) { return FontBytes(client_.Translate(text)); };data.storeText=[this](auto text) { return client_.ChatWindowGlobals()->Store(text); };
            data.text=[this](auto token) -> std::string_view { return client_.ChatWindowGlobals()->strings.at(token); };return rawButtons_->ConstructNsInfo(std::move(data));
        };
        context.root.createChat=[this] {
            RawChatWindowContext data;data.globals=client_.ChatWindowGlobals();data.commands=client_.DotCommandGlobals();data.editor.now=[this] { return client_.WallSeconds(); };
            data.translate=[this](auto text) { return FontBytes(client_.Translate(text)); };data.storeText=[this](auto text) { return client_.ChatWindowGlobals()->Store(text); };
            data.text=[this](auto token) -> std::string_view { return client_.ChatWindowGlobals()->strings.at(token); };
            data.read=[this](auto address) -> std::uint32_t { switch (address) {
                case 0x594fc8:return client_.SurfaceStorage()->ownerMode.challenge;case 0x594fbc:return world_ ? world_->Session().BattleActive() : 0U;
                case 0x548714:return client_.ChatManagerGlobals()->Current();case 0x563e38:return rawButtons_->RootToken();
                case 0x5cae94:return std::bit_cast<std::uint32_t>(client_.GameplayGlobals()->cursor.screenX);case 0x5cae98:return std::bit_cast<std::uint32_t>(client_.GameplayGlobals()->cursor.screenY);
                case 0x542944:return 0x02000094U;default:throw std::logic_error("Chat 실제 Client 전역 미연결:"+std::to_string(address));
            } };
            data.config=[this](auto key) { return client_.Configuration().GetInt(key); };data.quit=[this](auto event,auto first,auto second) { client_.Input().Push(event,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)); };
            return rawButtons_->Menus()->CreateChatWindow(std::move(data)); // 선택한 recipient/전송/clipboard 미연결은 기존 본체의 도달 위치에서 계속 명시 거부한다.
        };
        context.root.refreshTechnology=[this] {
            auto hooks=MakeClientChatManagerHooks(client_.ChatManagerGlobals());
            hooks.core=[this] { const std::uint32_t metamap=client_.SurfaceStorage()->ownerMode.challenge;return client_.CoreDataGlobals()->Read(client_.GameplayGlobals()->player,metamap,{[](auto line) { throw std::logic_error("Chat 랭크 Core 원문 진단:"+std::to_string(line)); }}); };
            hooks.maximumRank=[]() -> std::int32_t { throw std::logic_error("Chat 실제 icon 최대랭크 장치 미연결"); };hooks.minorCount=[]() -> std::int32_t { throw std::logic_error("Chat 실제 icon 소랭크 장치 미연결"); };
            o::RefreshChatManagerRankPrefix({client_.ChatManagerGlobals()->Current(),client_.CurrentSubscriber(),client_.CoreDataGlobals()->Active()},hooks,hud_->rankOutput,hud_->rankTemporary);
        };
        context.root.translate=[this](auto text) { return client_.ChatWindowGlobals()->Store(FontBytes(client_.Translate(text))); };
        context.root.help=[this](auto token,auto text) { rawButtons_->Menus()->SetHelpText(token,client_.ChatWindowGlobals()->strings.at(text)); };
        context.root.helpWord=[this](auto token,auto text) { rawButtons_->Menus()->SetHelpWord(token,text); };
        context.root.savedMessage=[] { return 0x565e70U; };context.root.equalText=[this](auto token,auto text) { if (token!=0x565e70U) throw std::logic_error("Uber 실제 메시지 주소 불일치");return o::AsciiLower(HtmlBackgroundName(htmlGlobals_))==o::AsciiLower(text); };
        context.root.constructMessage=[this](auto token,auto name) { if (name!=0x565e70U) throw std::logic_error("Uber 실제 배경 이름 주소 불일치");rawButtons_->Menus()->BackgroundGifs()->Backend().Construct(token,HtmlBackgroundName(htmlGlobals_)); };
        context.root.formatMessage=[this](const std::array<std::uint32_t,9>& expression) {
            const std::array<std::uint32_t,9> expected{0xfffffffeU,0x503c8bU,0xfffffffeU,0x508ab4U,0xfffffffdU,0x565e70U,0xfffffffcU,0x51501cU,0}; // 보호PE의 빈 root/d/현재 이름/.col이다.
            if (expression!=expected) throw std::logic_error("Uber 고정 팔레트 경로의 실제 원문 인자 변경");
            auto name=std::string(HtmlBackgroundName(htmlGlobals_));std::replace(name.begin(),name.end(),'/','\\');
            while (!name.empty() && name.front()=='\\') name.erase(name.begin()); // 원문 filename(-3)은 선행 구분자만 제거한다.
            if (name.ends_with('\\')) throw std::logic_error("Uber 팔레트 이름의 후행 디렉터리 구분자 원문 물리 확인 필요");
            std::string path=".\\d\\"+name;if (!path.ends_with('.')) path+='.';path+="col"; // 원문의 -2 빈root→-2 d→-3 이름→-4 .col 선택 경로다.
            if (path.size()>=256) throw std::length_error("Uber 팔레트 경로의 원문256BYTE 물리 범위");hud_->paletteProbe=path;return client_.ChatWindowGlobals()->Store(path);
        };
        context.root.testMessage=[this](auto token) -> std::uint32_t { const auto& path=client_.ChatWindowGlobals()->strings.at(token);hud_->paletteFound=client_.Files().TryRead(path).has_value();std::clog<<"Uber palette probe "<<path<<" exists "<<hud_->paletteFound<<'\n';return hud_->paletteFound ? 1U : 0U; }; // 실제 VFS 조회 결과만 조건으로 사용하며 없는 파일을 만들지 않는다.
        context.root.visitMessage=[](auto) { throw std::logic_error("Uber 실제 전체 팔레트 전환 미연결"); };
        context.root.messageEvent=[this](auto token,auto event,auto first,auto second) { rawButtons_->Menus()->BackgroundGifs()->Backend().SetEvent(token,event,first,second); };
        context.dock.invalidate=[this](auto left,auto top,auto width,auto height,auto flags) { client_.GetRenderer().Invalidate({left,top,left+width,top+height},flags); };
        context.dock.finishResize=[this] { worldRedraw_=1;if (world_) world_->Resize(hud_->width,hud_->height);rebuild_=true; };
        context.dock.chatVisible=[this](bool visible) { const auto token=client_.ChatWindowGlobals()->window.current;rawButtons_->Menus()->ChatGeometryBackend()->SetHelperVisible(token,visible); };
        context.dock.chatInput=[this](bool visible) { const auto token=client_.ChatWindowGlobals()->window.current;rawButtons_->Menus()->ChatGeometryBackend()->SetInputVisible(token,visible); };
        rawButtons_->EnableUberRoot(std::move(context));
    }
    if (!rawButtons_->UberRoot()->Current()) {
        rawButtons_->UberRoot()->Replace();rawButtons_->HandleGump(rawButtons_->RootToken(),0x0200003dU,3,0); // 기존 미션 설치 위치에서 실제 일반 HUD mode3을 적용한다.
    }
    rawButtons_->UberRoot()->Reflow(); // 같은 기존 구분선/등록/드래그 위치를 화면 장치 크기 전환에서도 유지한다.
    rawButtons_->Production()->SetPalette(client_.GetScreen().Palette(),client_.ColorSortInput()->Logical());rawButtons_->Minimap()->SetPalette(client_.GetScreen().Colors());
    world_->BindMapView([this] { return rawButtons_->MapViewRect(); }); // 현재 지도 영역의 같은 저장소를 표시/미니맵/선택/음향에서 읽는다.
}
