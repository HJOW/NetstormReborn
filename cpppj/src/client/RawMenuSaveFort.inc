// 같은 Impl의 장식/문자/돌 버튼과 실제 편집 namespace로 Save as를 조립한다.
std::uint32_t AllocateSaveFort(std::size_t bytes) {
    std::uint32_t token{};
    if (bytes==0x110) token=textEntries->Allocate(false);
    else if (bytes==0x170) token=AllocateHelp(bytes);
    else {
        token=common.token();
        if (bytes==0xdc || bytes==0xd8) { saveFortRoots.emplace(token,std::make_unique<SaveFortNameDialogNode>());if (!saveFortBuilding) throw std::logic_error("입력 창의 실제 생성 문맥 없음");saveFortContexts.emplace(token,*saveFortBuilding); } // 숫자 창은 같은 공통 자식과+d4의 송금 대상을 사용한다.
        else if (bytes==0xa4) plainTexts.emplace(token,std::make_unique<TextGumpNode>());
        else if (bytes==0xb8) backgrounds.emplace(token,std::make_unique<GumpBackgroundNode>());
        else throw std::logic_error("Save as 실제 파생 할당 크기 오류");
    }
    saveFortOwned.insert(token);return token;
}
// 현재 창의 실제 편집 버퍼와 동일 작업본 경로/헤더/전체 이름 부모를 묶는다.
std::uint32_t AcceptSaveFort(std::uint32_t token) {
    const auto& context=saveFortContexts.at(token);auto& root=*saveFortRoots.at(token);SaveFortNameConfirmHooks effects;
    if (context.sendMoney) { std::array<char,256> text{};static_cast<void>(textEntries->Backend().CopyText(root.entry,text));text.back()=0;return context.sendMoney(root.fortMode,std::string_view(text.data())); } //004c9350은 현재 편집 문자열을 읽은 뒤 송금하고1을 반환한다.
    effects.copyText=[this,&root](auto output) { static_cast<void>(textEntries->Backend().CopyText(root.entry,output)); };
    effects.alnum=[](auto value) { return std::isalnum(value); };effects.path=context.fortPath;
    effects.exists=[&context](auto path) { const auto relative=std::filesystem::path(path).is_absolute() ? std::string(path) : o::NormalizeGamePath(path);std::error_code error;return std::filesystem::exists(context.work->ResolvePath(relative),error) ? 1 : 0; }; // 원문 GetFileAttributesA처럼 디렉터리도 비영이며 작업본 경계는 동일하게 검사한다.
    effects.header=context.header;effects.setName=context.setName;effects.tell=context.tell;effects.readFort=context.readFort;
    return ConfirmSaveFortName(root.fortMode,effects);
}
// 이미 복원된 공통 처리기에 하위 계산을 맡기고 새 파생 부모의 조회/수명만 추가한다.
void BindSaveFortDialog() {
    if (saveFortDialog) return;SaveFortNameDialogHooks h;
    h.node=[this](auto token) { return Find(saveFortRoots,token); };h.base=common.node;h.entry=[this](auto token) { return textEntries->Node(token); };
    h.allocate=[this](auto bytes) { return AllocateSaveFort(bytes); };h.decoration=[this](auto token,float width,float height) { decoration->Construct(token,width,height); };
    h.textEntry=[this](auto token,auto width,auto height,auto text) { textEntries->Backend().Construct(token,width,height,text,0,0,palette.basic[5]); };
    h.excluded=[this](auto token,auto text) { textEntries->Backend().SetExcluded(token,text); };
    h.background=[this](auto token,float width,float height) { backgroundCreate->Construct(token,width,height,0x03e40000U,1,decorationState.backgroundType,decorationState.backgroundFrame); };
    h.title=[this](auto token,auto text) {
        std::uint32_t current{}; // 원문 대입이 끝난 현재 제목의 실제 부모만 조회한다.
        for (const auto& [root,node]:saveFortRoots) if (node->title==token) { current=root;break; } // 원문 대입이 끝난 현재 부모만 선택한다.
        if (!current) throw std::logic_error("Save as 제목의 실제 생성 부모 없음");costText->Construct(token,static_cast<std::int32_t>(saveFortRoots.at(current)->baseWidth*0.8F),20,0,0);
        const std::array<std::uint32_t,8> context{2,3,palette.basic[7],0,0,0,1,1};costText->AppendText(token,{text.data(),text.size()},context,static_cast<std::int32_t>(text.size()));
    };
    h.stone=[this](auto token,auto text) { const std::string terminated(text);common.stones.Construct(token,70,18,terminated.c_str(),palette.named.at(8).index,nullptr); };
    h.fitTitle=[this](auto token) { costText->Fit(token); };h.fitStone=[this](auto token) { common.stones.FitToText(token); };h.push=[this](auto token) { common.tree.Push(token); };h.attach=common.attach;
    h.depth=[this](auto token,auto depth) { common.tree.AddDepth(token,std::bit_cast<std::uint32_t>(depth)); };h.modal=[this](auto token) { SetModal(token,true); };h.kill=[this](auto token) { common.lifetime.Kill(token); };
    h.accept=[this](auto token) { return AcceptSaveFort(token); };h.cancel=[this](auto token) { const auto& root=*saveFortRoots.at(token);return CancelSaveFortName(root.canCancel,saveFortContexts.at(token).tell); };
    h.baseHandle=[this](auto token,auto event,auto first,auto second) { return BaseHelp(token,event,first,second); };
    h.translate=[this](auto text) { if (!saveFortBuilding || !saveFortBuilding->translate) throw std::logic_error("Save as 현재 번역 장치 미연결");return saveFortBuilding->translate(text); };
    saveFortDialog=std::make_unique<SaveFortNameDialog>(common.create,std::move(h));
}
