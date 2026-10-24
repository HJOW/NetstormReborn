// 같은 Impl의 실제 HTML 부모/본문/돌 버튼 저장소를 원문 본체에 연결한다.
void BindHtml(RawHtmlWindowContext context) { // 같은 Client의 장기 상태에 한 번만 연결한다.
    if (assets.Edition()!=o::OriginalEdition::Patch1078 || !context.globals || !context.wallSeconds) throw std::invalid_argument("HTML 실제10.78 Client 전역/벽 시각 필요");
    if (htmlContext) { if (htmlContext->globals!=context.globals) throw std::logic_error("HTML 다른 Client 소유자 연결 불가");return; }htmlContext=std::move(context);
    HtmlWindowGumpHooks h;h.node=[this](auto token) { return Find(htmlRoots,token); };h.base=common.node;h.button=[this](auto token)->ButtonGumpNode* { return Find(helpButtons,token); };
    h.constructDecoration=[this](auto token) { decoration->Construct(token,0,0); };h.destructBase=[this](auto token) { common.lifetime.DestructBase(token); };h.push=[this](auto token) { common.tree.Push(token); };h.kill=[this](auto token) { common.lifetime.Kill(token); };
    h.allocate=[this](auto bytes) { const auto token=AllocateHelp(bytes);htmlOwned.insert(token);return token; };
    h.constructButton=[this](auto token,auto label) { const auto text=std::string(label);common.stones.Construct(token,0,0,text.c_str(),palette.named.at(9).index); };
    h.constructBody=[this](auto token) { titleWidgets->Construct(token,0,0,0,palette.basic[7]); };h.parseBody=[this](auto token,auto source) { titleMarkup->Parse(token,source); };h.scrollable=[this](auto token,auto value) { titleWidgets->SetScrollable(token,value); };
    h.attach=[this](auto token,auto x,auto y,auto parent,auto mode) { Attach(token,x,y,parent,mode); };h.reference=[this](auto serial) { return Reference(serial); };
    h.wallSeconds=[this] { if (!htmlContext->wallSeconds) throw std::logic_error("HTML 실제 벽 시각 미연결");return htmlContext->wallSeconds(); };
    h.queue=[this](auto event,auto first,auto second) { if (!htmlContext->queue) throw std::logic_error("HTML 실제 원시 입력 큐 미연결");htmlContext->queue(event,first,second); };h.link=[this](auto token) { if (!htmlContext->link) throw std::logic_error("HTML 실제 링크 미연결");htmlContext->link(token); };
    h.baseHandle=[this](auto token,auto event,auto first,auto second) { return BaseHelp(token,event,first,second); };
    HtmlWindowFitHooks fit;fit.node=h.node;fit.base=common.node;fit.fitButton=[this](auto token) { common.stones.FitToText(token); };fit.push=[this](auto token) { common.tree.Push(token); };fit.fitHeight=[this](auto token) { title->FitHeight(token); };fit.fitContent=[this](auto token,auto x,auto y) { title->FitContent(token,x,y); };
    fit.slider=[this](auto token,auto value) { titleWidgets->SetSlider(token,value); };fit.scrollable=[this](auto token,auto value) { titleWidgets->SetScrollable(token,value); };fit.requestScroll=[this](auto token,auto value) { title->RequestScroll(token,value); };
    fit.attach=[this](auto token,auto x,auto y,auto parent,auto mode) { Attach(token,x,y,parent,mode); };fit.attachBase=[this](auto token,auto x,auto y,auto parent,auto mode) { common.tree.Attach(token,x,y,parent,mode); };fit.currentMenu=[this] { return menu->Current(); };fit.attachMenu=[this](auto x,auto y,auto parent) { menu->AttachCurrent(x,y,parent); };
    fit.reference=[this](auto serial) { return Reference(serial); };fit.read=[this](auto address) { if (!htmlContext->read) throw std::logic_error("HTML 실제 현재 뷰/화면 전역 미연결");return htmlContext->read(address); };fit.addDepth=[this](auto token,auto delta) { common.tree.AddDepth(token,delta); };htmlFit=std::make_unique<HtmlWindowFit>(std::move(fit));
    htmlWindow=std::make_unique<HtmlWindowGump>(htmlContext->globals->window,std::move(h));
}
std::uint32_t CreateHtml(RawHtmlWindowContext context,std::uint32_t width,std::uint32_t height) { // 원문 new0x21e8에 실제 고정 파생 저장소를 제공한다.
    BindHtml(std::move(context));const auto token=common.token();htmlRoots.emplace(token,std::make_unique_for_overwrite<HtmlWindowGumpNode>());htmlOwned.insert(token);const auto result=htmlWindow->Construct(token,width,height);htmlConstructed.insert(token);return result;
}
void CloseHtml() { // 부분 생성/미부착/교체 중인 실제 소유 노드도 같은 공통 수명으로 정리한다.
    for (std::size_t id=1;id<common.create.entries.size();++id) { const auto token=common.create.entries[id];if (htmlOwned.contains(token) && !(common.node(token)->flags&0x10000)) common.lifetime.Kill(token); } // 자식 우선 정리는 같은 공통 Cleanup이 수행한다.
    common.lifetime.Cleanup();
    for (const auto token:htmlOwned) { if (std::find(common.create.entries.begin(),common.create.entries.end(),token)!=common.create.entries.end()) throw std::logic_error("HTML 정리 뒤 실제 등록 노드 잔류");htmlRoots.erase(token); } // 기본 생성 전 실패의 실제 저장소만 반납한다.
    htmlOwned.clear();htmlConstructed.clear();common.changed();
}
