// 같은 공통 UI와 실제 보호 설명을 사용하며 모달/일반 팝업 상태와 구별한다.

// 원본 PlacingIslands를 실제 파일/글꼴/링크/다중 줄 자식으로 생성·부착·출력·반납한다.
TEST_CASE(RawMenuEmbeddedTell_ActualPlacingIslandsAndBorrowedParentLifecycle) {
    MenuClient client;client.EnableHelp();client.helpConfig->Configuration().Assign("missionSpec=d/{local.1}.english\n");const auto previous=client.htmlOwner->tell.current;const auto instances=client.htmlOwner->window.instances;
    const auto serial=client.menus->ShowEmbeddedTell("PlacingIslands",*client.helpConfig);const auto token=client.buttons.SerialGump(serial);auto* body=static_cast<HtmlBodyGumpNode*>(client.menus->Node(token));
    CHECK(serial && token && body && body->vtable==0x50bd38 && body->baseWidth==320 && body->baseHeight==240 && body->parent==0 && body->text && body->body[0]);if (!body) return;
    const auto text=body->text;const auto* child=static_cast<ScrollingTextGumpNode*>(client.menus->Node(text));CHECK(child && child->parent==token && child->buffer && child->slider);if (!child) return;const auto slider=child->slider;
    body->baseWidth=body->width=200;body->baseHeight=body->height=300;client.menus->Attach(token,20,30,client.buttons.RootToken(),0);CHECK(body->parent==client.buttons.RootToken() && client.menus->Node(text)->parent==token && client.menus->Node(text)->baseWidth==200);
    std::array<std::uint32_t,256> logical{};
    for (std::size_t index=0;index<logical.size();++index) { const auto c=client.assets.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=c.red|(std::uint32_t(c.green)<<8)|(std::uint32_t(c.blue)<<16); } // 기존 실제 색표와 글리프 출력을 사용한다.
    constexpr int pitch=647,height=480; // 화면 뒤 padding도 같은 클립 경계를 지켜야 한다.
    const ScreenRect clip{20,30,220,330};std::vector<std::uint8_t> pixels(pitch*height,0x5a);client.menus->Draw(pixels,pitch,height,client.Palette(),BuildGumpShadeMaps(logical),clip);CHECK(std::any_of(pixels.begin(),pixels.end(),[](auto value) { return value!=0x5a; }));
    for (int y=0;y<height;++y) { for (int x=0;x<pitch;++x) if (x<clip.left || x>=clip.right || y<clip.top || y>=clip.bottom) CHECK(pixels[static_cast<std::size_t>(y)*pitch+x]==0x5a); } // 클립 밖과 행 padding은 기존 바이트를 보존한다.
    CHECK(client.buttons.DialogCount()==0 && client.htmlOwner->tell.current==previous && client.htmlOwner->window.instances==instances && !client.menus->HtmlWindowsOpen());
    client.menus->CloseEmbeddedTells();CHECK(!client.menus->Node(token) && !client.menus->Node(text) && !client.menus->Node(slider));
}

// 기본 파서의 링크 저장소와 실패 전 접두부를 같은 소유 노드에서 보존한다.
TEST_CASE(RawMenuEmbeddedTell_ActualLinksEmptyBodyAndParserFailureCleanup) {
    MenuClient client;client.EnableHelp();const auto mission=[](auto name)->std::optional<std::string> { if (name=="empty") return "";if (name=="fail") return "prefix <$Missing>suffix";return "<h3>Title</h3><p>Body <a href=\"#TEST\">Link</a>"; };
    const auto first=client.menus->ShowEmbeddedTell("links",*client.helpConfig,mission),token=client.buttons.SerialGump(first);auto* body=static_cast<HtmlBodyGumpNode*>(client.menus->Node(token));CHECK(body && body->text && std::string(body->body.data()).find("Title")!=std::string::npos && std::string(body->links.data())=="TEST");
    const auto empty=client.menus->ShowEmbeddedTell("empty",*client.helpConfig,mission);const auto* blank=static_cast<HtmlBodyGumpNode*>(client.menus->Node(client.buttons.SerialGump(empty)));CHECK(blank && blank->text && std::string(blank->body.data())=="\n");
    bool rejected{};try { static_cast<void>(client.menus->ShowEmbeddedTell("fail",*client.helpConfig,mission)); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    bool prefix{};
    for (std::uint32_t id=1;id<1000;++id) { const auto current=client.buttons.RegisteredGump(id);const auto* node=client.menus->Node(current);if (node && node->vtable==0x50bd38 && !static_cast<const HtmlBodyGumpNode*>(node)->text) prefix=std::string(static_cast<const HtmlBodyGumpNode*>(node)->body.data())=="prefix "; } // 실패 부모는 반납 전까지 현재 접두부와 자식0을 유지한다.
    CHECK(prefix);client.menus->CloseEmbeddedTells();CHECK(!client.menus->Node(token) && !client.buttons.SerialGump(empty) && client.buttons.DialogCount()==0);
}
