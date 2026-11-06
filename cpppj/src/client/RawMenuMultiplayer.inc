// 같은 Impl의 실제 기본/장식/문자/돌 버튼을 원문 Multiplayer Options 창으로 조립한다.
std::uint32_t AllocateMultiplayer(std::size_t bytes) {
    std::uint32_t token{};
    if (bytes==0x110) token=textEntries->Allocate(false);
    else if (bytes==0x170) token=AllocateHelp(bytes);
    else {
        token=common.token();
        if (bytes==0xe0) multiplayerRoots.emplace(token,std::make_unique<MultiplayerOptionsWindowNode>());
        else if (bytes==0xc4) decorations.emplace(token,std::make_unique<GumpDecorationNode>());
        else if (bytes==0xa4) plainTexts.emplace(token,std::make_unique<TextGumpNode>());
        else throw std::logic_error("Multiplayer Options 실제 할당 크기 범위");
    }
    multiplayerOwned.insert(token);return token;
}
//004bc800의 현재 체인 교체 뒤 같은 노드 문맥으로 스타일 문자를 추가한다.
void SetMultiplayerText(std::uint32_t token,std::string_view text) {
    costText->Clear(token);costText->AppendText(token,{text.data(),text.size()},plainTexts.at(token)->textContext,static_cast<std::int32_t>(text.size()));
}
// 번역을 먼저 수행한 뒤 현재 문자열 값을 넣어 원문 %s 한 곳만 치환한다.
std::string MultiplayerLabel(const MultiplayerOptionsWindowContext& context,std::string_view pattern,const std::function<std::string()>& value) const {
    if (!context.translate || !value) throw std::logic_error("Multiplayer Options 현재 번역/값 미연결");auto text=context.translate(pattern);const auto current=value();const auto marker=text.find("%s");if (marker==text.npos) throw std::logic_error("Multiplayer Options 번역의 실제 %s 누락");text.replace(marker,2,current);return text;
}
//004cc970의250×145와 장식/제목·네 행·상대 화살표·하단 세 버튼을 현재 자식 크기로 배치한다.
void AttachMultiplayer(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) {
    auto& root=*multiplayerRoots.at(token);const auto& context=multiplayerContexts.at(token);root.width=root.scale*250.0F;root.height=root.scale*145.0F;root.baseWidth=250;root.baseHeight=145;
    common.attach(root.decoration,0,0,token,0);common.attach(root.title,0,5,token,0);
    if (!context.fortName || !context.playerName || !context.serverName || !context.rememberFort) throw std::logic_error("Multiplayer Options 현재 이름/설정 장치 미연결");
    SetMultiplayerText(root.labels[0],MultiplayerLabel(context,"~ySave Game: ~w%s",context.fortName));context.rememberFort(context.fortName());
    SetMultiplayerText(root.labels[1],MultiplayerLabel(context,"~yPlayer Name: ~w%s",context.playerName));
    SetMultiplayerText(root.labels[2],MultiplayerLabel(context,"~yServer: ~w%s",context.serverName));
    SetMultiplayerText(root.labels[3],MultiplayerLabel(context,"~yProtocol: ~w%s",[] { return std::string("TCP/IP"); }));
    common.node(root.arrows[3])->flags&=~1U; // 요구된 로컬 TCP/IP 제품은 구 SPX/IPX 선택을 비활성화하며 검증 성공값을 합성하지 않는다.
    common.attach(root.labels[0],10,30,token,0);
    for (std::size_t index=1;index<root.labels.size();++index) common.tree.MoveRelative(root.labels[index],root.labels[index-1],0,3,4,token); // 원문 앞 행의 실제 높이 뒤3픽셀 간격이다.
    for (std::size_t index=0;index<root.arrows.size();++index) common.tree.MoveRelative(root.arrows[index],root.labels[index],10,-4,8,token); // 원문 현재 행의 오른쪽 끝+10/위쪽−4다.
    const auto bottom=[&](std::uint32_t child) { return static_cast<float>(static_cast<std::int32_t>(root.baseHeight)-static_cast<std::int32_t>(common.node(child)->baseHeight)-10); }; // 원문 _ftol의 각 높이 정수부로 하단 간격을 계산한다.
    common.attach(root.connect,10,bottom(root.connect),token,0);common.attach(root.cancel,10+common.node(root.connect)->baseWidth+10,bottom(root.cancel),token,0);
    if (context.special && context.special()) common.node(root.subscriber)->flags|=1U;else common.node(root.subscriber)->flags&=~1U;
    common.attach(root.subscriber,10+common.node(root.connect)->baseWidth+common.node(root.cancel)->baseWidth+20,bottom(root.subscriber),token,0);
    common.tree.Attach(token,x,y,parent,mode);
    const auto unnamed=context.translate("Unnamed");if (_stricmp(context.playerName().c_str(),unnamed.c_str())==0) context.action(MultiplayerOptionsAction::ChooseName); // 원문은 각 Attach의 끝에서 현재 Unnamed를 다시 읽고 같은 일반 입력 큐에 이름 입력을 예약한다.
}
//004cc4d0의 생성 순서/미표시 편집 자식과004ccf30의 하단 정렬·깊이·모달 등록이다.
std::uint32_t CreateMultiplayer(MultiplayerOptionsWindowContext context) {
    if (!context.translate || !context.action) throw std::logic_error("Multiplayer Options 실제 생성 문맥 누락");
    const auto token=AllocateMultiplayer(0xe0);multiplayerContexts.emplace(token,std::move(context));auto& root=*multiplayerRoots.at(token);const auto& current=multiplayerContexts.at(token);
    common.factory.Construct(token,0,0,0,1);root.vtable=0x00515c88U;root.flags|=0x800U;root.command=61; // PE00540458의 실제 signed GT 기본값61이다.
    root.decoration=AllocateMultiplayer(0xc4);decoration->Construct(root.decoration,0,0);common.node(root.decoration)->alignFlags|=3;common.node(root.decoration)->secondaryDepth=1;
    root.title=AllocateMultiplayer(0xa4);costText->Construct(root.title,0,0,0,0);SetMultiplayerText(root.title,current.translate("~w~3~EMultiplayer Options"));costText->Fit(root.title);common.node(root.title)->alignFlags|=4;
    root.entry=AllocateMultiplayer(0x110);const std::array<char,3> nativeThis{static_cast<char>(0x88),'\\','Q'}; // 원문은 미부착 entry에 this를 문자 주소로 전달하므로 첫 Native VT00515c88의 NUL 전3BYTE만 복원한다.
    textEntries->Backend().Construct(root.entry,static_cast<std::int32_t>(root.baseWidth*0.8F),24,std::string_view(nativeThis.data(),nativeThis.size()),0,0,palette.basic[5]);auto* entry=textEntries->Node(root.entry);entry->maxLength=0x29;entry->multiline=0;
    for (auto& label:root.labels) { label=AllocateMultiplayer(0xa4);costText->Construct(label,200,17,0,0); } // 원문 네 한 줄 자식의 폭/높이를 실제 TextGump에 설정한다.
    for (auto& arrow:root.arrows) { arrow=AllocateMultiplayer(0x170);common.stones.Construct(arrow,0,0,">",palette.named.at(8).index,nullptr);common.stones.FitToText(arrow); } // 각 선택 돌 버튼은 원문 단일 > 그림 문자를 사용한다.
    root.connect=AllocateMultiplayer(0x170);const auto connect=current.translate("Connect!");common.stones.Construct(root.connect,0,0,connect.c_str(),palette.named.at(8).index,nullptr);common.stones.FitToText(root.connect);
    root.cancel=AllocateMultiplayer(0x170);const auto cancel=current.translate("Cancel");common.stones.Construct(root.cancel,0,0,cancel.c_str(),palette.named.at(8).index,nullptr);common.stones.FitToText(root.cancel);
    const auto buttonWidth=static_cast<float>(static_cast<std::int32_t>(std::max(common.node(root.connect)->baseWidth,common.node(root.cancel)->baseWidth))); // 원문 두 버튼의 현재 최대 폭을 정수부로 맞춘다.
    for (const auto button:{root.connect,root.cancel}) { auto& node=*common.node(button);node.baseWidth=buttonWidth;node.width=buttonWidth*node.scale; } // 원문 크기 쓰기는 자식 그림/높이를 교체하지 않는다.
    root.subscriber=AllocateMultiplayer(0x170);common.stones.Construct(root.subscriber,0,0,"Sub ID",palette.named.at(8).index,nullptr);common.stones.FitToText(root.subscriber);
    root.alignFlags|=0x20;AttachMultiplayer(token,0,0,common.create.entries[0],2);common.tree.AddDepth(token,std::bit_cast<std::uint32_t>(std::bit_cast<std::int32_t>(root.depth)-80));SetModal(token,true);return token; // PE004ccf7e의 실제 mode2로 하단 정렬을 적용한다.
}
//004cfc80의 현재 serial 비교와 명령 위임이다. 선택 효과 뒤 같은 원문 Kill 위치를 유지한다.
std::uint32_t HandleMultiplayer(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) {
    auto& root=*multiplayerRoots.at(token);const auto context=multiplayerContexts.at(token);const auto code=event&0x03ffffffU;
    if (code==0x0200002cU) {
        for (std::size_t index=0;index<root.arrows.size();++index) { // 원문 네 버튼의 실제 생성 serial만 비교한다.
            if (first!=common.node(root.arrows[index])->serial) continue;if (index==3) return 1;context.action(static_cast<MultiplayerOptionsAction>(index));return BaseHelp(token,event,first,second);
        }
        if (first==common.node(root.connect)->serial) { context.action(MultiplayerOptionsAction::Connect);common.lifetime.Kill(token);return 1; }
        if (first==common.node(root.cancel)->serial) { context.action(MultiplayerOptionsAction::Cancel);common.lifetime.Kill(token);return 1; }
        if (first==common.node(root.subscriber)->serial) return 1;
    } else if (code==0x020000c8U || code==0x0200006dU) { common.RedrawBase(token,true);AttachMultiplayer(token,root.x,root.y,0,0); }
    return BaseHelp(token,event,first,second);
}
