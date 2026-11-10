// UberGump.cpp의 같은 namespace에서 포함하며 별도 가짜 UI/전송 상태를 만들지 않는다.
namespace { // snapshot 이후 새 sidecar 준비 실패만 구별한다.
struct EditorScenarioPrepareError:std::runtime_error { using std::runtime_error::runtime_error; }; // Guide를 닫기 전의 실패만 기존 Editor 안내로 처리한다.
}
// 패널/자식 모달의 실제 토큰만 Kill한다. 저장하지 않은 변경은 사용자의 취소/World 반납에서 버린다.
void UberGump::CloseScenarioUi(bool discard) {
    if (scenarioToken_ && rawButtons_) { if (rawButtons_->Menus() && rawButtons_->Menus()->Node(scenarioToken_)) rawButtons_->KillGump(scenarioToken_);scenarioToken_=0; } // 메뉴 행 선택이 이미 닫은 토큰을 다시 Kill해 종료/전환이 중단되던 결함을 교정한다(S1 실제 확인).
    if (discard) { scenario_.reset();scenarioPage_=0; }
}
// 최초 열기에서만 현재 맵의 실제 문서를 소유하며 페이지 이동은 보류 편집을 유지한다.
void UberGump::OpenScenarioPage(std::uint32_t page) {
    if (!world_ || !client_.GameplayGlobals()->world.editor || state_.phase!=ClientPhase::Mission) throw std::logic_error("Scenario는 설치 완료 Editor에서만 열 수 있습니다");
    const auto current=std::string(client_.FortGlobals()->Current());
    if (!scenario_ || scenario_->Map()!=current) {
        EditorScenarioContext context;context.localOwner=client_.GameplayGlobals()->player.battleOwner;context.startingSp=world_->Session().MissionAiSettings().startingSp;context.stuffRefreshRate=world_->Session().MissionAiSettings().stuffRefreshRate; // 존재를 추정한 Config 키 대신 실제 원문 기본 전역을 읽는다.
        context.knownTechnology=[this](auto name) { return client_.Assets().TypeTable().Find(name)>=0; };context.resolve=[this](const auto& document,auto key) { return ResolveEditorScenarioField(client_.Configuration(),document,key); };
        scenario_=std::make_unique<EditorScenarioController>(scenarioFiles_,current,std::move(context));
    }
    CloseScenarioUi(false);scenarioPage_=page;const auto rows=scenario_->Page(page,[this](auto owner) { return world_->Session().FindPriestParticipation(owner)!=0; });auto& menus=*rawButtons_->Menus();
    menus.OpenAttached([&](const auto& widgets,const auto& controls,const auto& menu) {
        widgets.BeginFancy();controls.AddTitle(FontBytes("Scenario / 시나리오: "+current));
        for (const auto& row:rows) controls.AddText(FontBytes(row.label),row.command,row.parameter,true,row.enabled,false); // 원래 메뉴40행 제한 안의 유한 페이지를 작성한다.
        const auto token=menu.TakeCurrent();const auto* node=menus.Node(token);if (!node) throw std::logic_error("Scenario 실제 메뉴 노드 없음");widgets.Attach(token,(client_.GetScreen().Width()-node->baseWidth)*0.5F,(client_.GetScreen().Height()-node->baseHeight)*0.5F,rawButtons_->RootToken(),0);scenarioToken_=token;return token;
    },true);menus.SetGumpModal(scenarioToken_,true);rebuild_=true;
}
// 화면 입력은 기존 실제 TextEntry/clipboard/IME/MCP의 일반 사건 경로를 그대로 사용한다.
void UberGump::OpenScenarioField(std::uint32_t field) {
    if (!scenario_ || !world_ || !client_.GameplayGlobals()->world.editor) throw std::logic_error("Scenario 편집 문서/현재 Editor 없음");
    const auto request=scenario_->Edit(field);const auto map=scenario_->Map();CloseScenarioUi(false);SaveFortNameDialogContext context;context.editor.now=[this] { return client_.WallSeconds(); };BindTextClipboard(context.editor,textClipboard_);
    context.translate=[this](auto text) { return FontBytes(client_.Translate(o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}))); };
    context.editorAccept=[this,field,map](auto bytes) {
        try { if (!scenario_ || scenario_->Map()!=map || client_.FortGlobals()->Current()!=map || !client_.GameplayGlobals()->world.editor) throw std::logic_error("맵 전환 뒤의 Scenario 입력은 적용할 수 없습니다");const auto value=o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size()});scenario_->Apply(field,value);client_.Input().PushCommand(kEditorScenarioPage,0,static_cast<std::int32_t>(scenarioPage_));return true; }
        catch (const std::exception& error) { warnings_->Add(FontBytes(error.what()));return false; } // 잘못된 새 값은 문서/기존 입력창을 보존한다.
    };
    context.editorCancel=[this] { client_.Input().PushCommand(kEditorScenarioPage,0,static_cast<std::int32_t>(scenarioPage_)); }; // 실제 Cancel은 보류 문서 전체를 버리지 않고 현재 필드만 취소한다.
    scenarioToken_=rawButtons_->Menus()->ShowScenarioInputDialog(std::move(context),FontBytes(request.title),FontBytes(request.initial),request.maximum,request.multiline);rebuild_=true;
}
// 원문 새 Guide 공장이 기존 Editor를 반납하기 전에 sidecar 오류를 확정한다.
void UberGump::PrepareScenarioTest() {
    try {
        const auto name=std::string(client_.FortGlobals()->Current());if (scenario_ && scenario_->Map()==name && scenario_->Dirty()) scenario_->Save(false);
        if (!scenarioFiles_->EnsureEnglish(name)) throw std::runtime_error("삭제한 English 시나리오를 Scenario의 Save로 다시 작성하세요");
    } catch (const std::exception& error) { throw EditorScenarioPrepareError(error.what()); }
}
// 원문 입력과 겹치지 않는 명시 Editor 확장 명령만 분배한다.
bool UberGump::ScenarioEvent(const InputEvent& event) {
    const auto code=event.code&0x03ffffffU;
    if (scenario_ && code==0x1bU) { if (!(event.code&InputCode::kRelease)) CloseScenarioUi(true);rebuild_=true;return true; }
    if (code<kEditorScenarioOpen || code>kEditorScenarioInherit) return false;
    if (event.code&InputCode::kRelease) return true;
    if (!world_ || !client_.GameplayGlobals()->world.editor || state_.phase!=ClientPhase::Mission) return true;
    try {
        if (code==kEditorScenarioOpen) OpenScenarioPage(0);
        else if (code==kEditorScenarioPage) OpenScenarioPage(static_cast<std::uint32_t>(event.y));
        else if (code==kEditorScenarioEdit) OpenScenarioField(static_cast<std::uint32_t>(event.y));
        else if (code==kEditorScenarioSave) { if (!scenario_) throw std::logic_error("저장할 Scenario 문서 없음");scenario_->Save(true);CloseScenarioUi(true); }
        else if (code==kEditorScenarioDiscard) CloseScenarioUi(true);
        else if (code==kEditorScenarioInherit) { if (!scenario_) throw std::logic_error("상속할 Scenario 문서 없음");scenario_->Inherit(static_cast<std::uint32_t>(event.y));OpenScenarioPage(scenarioPage_); }
    } catch (const std::exception& error) { warnings_->Add(FontBytes(error.what())); }rebuild_=true;return true;
}
