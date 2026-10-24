// RawMenuControls::Impl 내부에서 같은 실제 노드/조각/글꼴/수명을 사용하는 Chat 공장 구현이다.
// 원문 전역의 PE 초기 사건과 현재 파일 팔레트를 읽고 동적 전역은 Client 문맥에서 매번 조회한다.
std::uint32_t ChatRead(std::uint32_t address) const {
    switch (address) {
    case 0x540424:return 9;case 0x548740:return chatContext->globals->window.current;case 0x548744:return chatContext->globals->window.registered;case 0x54da91:return chatContext->globals->window.completionReady;
    case 0x541348:return static_cast<std::uint32_t>(assets.TypeTable().Types().size());
    case 0x5acd24:return palette.named.at(9).index;case 0x5b5dc0:return palette.named.at(32).index;case 0x5b5e60:return palette.named.at(30).index;case 0x5b5dd4:return palette.named.at(31).index;case 0x5b5dd8:return palette.named.at(33).index;
    case 0x542b24:return 0x02000115;case 0x5427a8:return 0x0200002c;case 0x5429f0:return 0x020000bf;case 0x542914:return 0x02000088;case 0x5427d8:return 0x02000038;
    case 0x542794:return 0x02000027;case 0x542af8:return 0x02000101;case 0x54298c:return 0x020000a6;case 0x542b08:return 0x02000105;case 0x542b0c:return 0x02000106;case 0x5428ac:return 0x0200006d;
    default:return chatContext->read(address);
    }
}
// Native 직접 할당은 그 뒤의 포인터 대입/기본 생성/부착 전에 실패해도 같은 저장소에 소유한다.
std::uint32_t AllocateChat(std::size_t bytes) {
    std::uint32_t token{};
    if (bytes==0x128) token=textEntries->Allocate(true);
    else if (bytes==0xd8) token=Allocate(bytes,0);
    else if (bytes==0x170) token=AllocateHelp(bytes);
    else { token=common.token();if (bytes==0x13c) chatRoots.emplace(token,std::make_unique<HudChatNode>());else if (bytes==0xb8) backgrounds.emplace(token,std::make_unique<GumpBackgroundNode>());else throw std::logic_error("ChatWindow 실제 파생 할당 크기 오류"); }
    chatOwned.insert(token);return token;
}
// 같은 원문 생성/부착/입력 본체를 실제 공통 자식/버퍼/controlled 메뉴에 연결한다.
void BindChat(RawChatWindowContext context,RawMenuControls& owner) {
    if (!context.globals || !context.commands || !context.read || !context.translate || !context.text || !context.editor.now || assets.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("ChatWindow 실제 Client/문자열/편집 장치 필요");
    if (chatContext) { if (context.globals!=chatContext->globals || context.commands!=chatContext->commands) throw std::logic_error("ChatWindow 장기 소유자 교체 불가");return; }
    if (context.globals->uiOwner && context.globals->uiOwner!=this) throw std::logic_error("ChatWindow 공유 조각 저장소 이중 대여");
    owner.EnableTextEntry(context.editor);chatContext=std::move(context);auto& globals=*chatContext->globals;
    buffers->ConstructBorrowed(0x548758,globals.log,128);globals.uiOwner=this; // 원문 공유 객체 자체를 같은 StyleText/조각 표에 연결한다.
    HudChatGeometryHooks geometry;geometry.chat=[this](auto token) { return Find(chatRoots,token); };geometry.node=common.node;geometry.baseAttach=[this](auto token,float x,float y,auto parent,auto mode) { common.tree.Attach(token,x,y,parent,mode); };geometry.push=[this](auto token) { common.tree.Push(token); };geometry.attach=common.attach;
    geometry.bottomIndex=[this](auto token) { return std::uint32_t(title->AtBottom(token)); };geometry.requestBottom=[this](auto token,auto extra) { title->RequestBottom(token,std::bit_cast<std::int32_t>(extra)); };geometry.challenge=[this] { return ChatRead(0x594fc8); };chatGeometry=std::make_unique<HudChatGeometry>(std::move(geometry));
    ChatWindowHooks h;h.chat=[this](auto token) { return Find(chatRoots,token); };h.base=common.node;h.console=[this](auto token) { return textEntries->Console(token); };h.text=[this](auto token) { return Find(titles,token); };h.buffer=[this](auto token) { return textEntries->Storage().Find(token); };h.entry=&textEntries->Backend();h.geometry=chatGeometry.get();h.global=[this](auto address) { return ChatRead(address); };h.allocate=[this](auto bytes) { return AllocateChat(bytes); };
    h.constructBase=[this](auto token,float width,float height) { common.factory.Construct(token,0,width,height,1); };h.constructHelper=[this](auto token,std::string_view label,auto color) { const auto terminated=std::string(label);common.stones.Construct(token,0,0,terminated.c_str(),color,nullptr); };h.fitHelper=[this](auto token) { common.stones.FitToText(token); };
    h.title=[&owner](auto token,auto text) { owner.SetHelpText(token,text); };h.help=[&owner](auto token,auto text) { owner.SetHelpWord(token,text); };h.translate=chatContext->translate;
    h.measure=[this](auto token,std::string_view text) { std::string terminated(text);terminated.push_back(0);std::int32_t width{},height{};metrics.Measure({terminated.data(),terminated.size()},DecodeGumpTextContext(common.node(token)->textContext),width,height,-1);return std::array{width,height}; };
    h.constructBackground=[this](auto token,float width,float height,auto flags) { backgroundCreate->Construct(token,width,height,flags,1,0,0); };h.constructScrolling=[this](auto token,auto width,auto height,auto font,auto color) { titleWidgets->Construct(token,width,height,font,color); };h.replaceBuffer=[this](auto token,auto buffer,auto owns) { titleWidgets->ReplaceBuffer(token,buffer,owns); };h.sharedBuffer=[] { return 0x548758U; };
    h.setSlider=[this](auto token,auto value) { titleWidgets->SetSlider(token,value); };h.setScrollable=[this](auto token,auto value) { titleWidgets->SetScrollable(token,value); };h.attach=common.attach;h.push=[this](auto token) { common.tree.Push(token); };h.destructBase=[this](auto token) { common.lifetime.DestructBase(token); };
    h.registerCommand=[this](std::string_view name,auto callback,auto flag) { std::string terminated(name);terminated.push_back(0);DotCommandRegisterHooks copy;copy.copy=CopyDotCommandName;chatContext->commands->Register({reinterpret_cast<const std::uint8_t*>(terminated.data()),terminated.size()},callback,flag,copy); };
    h.bufferAppendCallback=[this](auto token,auto callback) { if (token!=0x548758U || callback!=0x42f590U) throw std::logic_error("ChatWindow 실제 공유 로그 callback 불일치");chatContext->globals->log.onAppend=[globals=chatContext->globals.get(),valid=chatContext->fileValid,write=chatContext->writeLog](auto text) { const auto file=globals->logFile;if (!file) return;if (!valid || !write) throw std::logic_error("Chat 로그 실제 파일 장치 미연결");if (!valid(file)) return;const auto end=std::find(text.begin(),text.end(),0);if (end==text.end()) throw std::logic_error("Chat 로그 원문 NUL 범위 없음");write(globals->logFile,text.first(static_cast<std::size_t>(end-text.begin()))); }; };
    h.frameCount=[this](auto type) { if (type<o::kFirstAssetTypeNumber || type-o::kFirstAssetTypeNumber>=assets.Types().size()) return 0;return static_cast<std::int32_t>(assets.Types()[type-o::kFirstAssetTypeNumber].definition.clusters.size()); }; // TYPE+114는 원문 프레임 코드/클러스터 수다.
    h.frameHeight=[this](auto type,auto frame) { const auto& asset=assets.Types()[type-o::kFirstAssetTypeNumber];const auto header=assets.Shapes().SquidHeader(asset.block,frame);const auto bits=std::uint16_t(header[0x1a])|(std::uint16_t(header[0x1b])<<8);return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(bits)); };
    h.requestScroll=[this](auto token,auto value) { title->RequestScroll(token,value); };h.relativeScroll=[this](auto token,auto value) { title->RequestRelativeScroll(token,value); };h.requestBottom=[this](auto token) { title->RequestBottom(token); };h.baseHandle=[this](auto token,auto event,auto first,auto second) { return BaseHelp(token,event,first,second); };h.bubble=[this](auto token,auto event,auto first,auto second) { common.events.Bubble(token,event,first,second); };
    h.recipient=[this](auto text) { if (!chatContext->recipient) throw std::logic_error("Chat 실제 수신 대상 검사 미연결");return chatContext->recipient(text); };h.sendChat=[this](auto manager,auto text) { if (!chatContext->sendChat) throw std::logic_error("Chat 실제 manager 전송 미연결");chatContext->sendChat(manager,text); };h.resolve=[this](auto& record) { if (!chatContext->resolve) throw std::logic_error("Chat 실제 identity 조회 미연결");return chatContext->resolve(record); };h.log=[this](auto level,auto format,auto parameter) { if (!chatContext->diagnostic) throw std::logic_error("Chat 실제 진단 미연결");chatContext->diagnostic(level,format,parameter); };h.config=[this](auto key) { if (!chatContext->config) throw std::logic_error("Chat 실제 설정 미연결");return chatContext->config(key); };
    h.menuBegin=[this] { widgets->BeginVertical(0); };h.menuFinalize=[this] { static_cast<void>(menu->Current()); }; //475c40은 현재 메뉴 토큰 조회이며 입력 반환값은 원문이 버린다.
    h.menuTitle=[this](auto text) { controls->AddTitle(text); };h.menuItem=[this](auto label,auto event,auto payload) { controls->AddText(label,event,payload ? chatContext->globals->Store(*payload) : 0,true,true,false); };h.menuChecked=[this](auto checked,auto label,auto event,auto payload) { controls->AddChecked(checked!=0,label,event,chatContext->globals->Store(payload),true,true); };h.menuTake=[this] { return menu->TakeCurrent(); };h.menuSubmenu=[this](auto label,auto child) { controls->AddSubmenu(label,child,true,false); };
    h.menuOpen=[this](auto x,auto y,auto reference) { popup=menu->Current();menu->AttachCurrent(std::bit_cast<std::int32_t>(x),std::bit_cast<std::int32_t>(y),reference); }; //475ca0의 세 번째 인자는 공통 부모 Gump다.
    h.assertion=[](auto line) { throw std::logic_error("ChatWindow 원본 진단: "+std::to_string(line)); };chatWindow=std::make_unique<ChatWindow>(globals.window,std::move(h));
}
