// 같은 namespace에서 GameWorld 생성자보다 앞에 포함한다. 실제 선택/강조/명령 수명을 고정한다.
struct GameWorld::Input {
    GameWorld& world; //선택 SID/카메라/마우스/Session의 유일한 소유자다.
    std::uint32_t enabled{1},consoleFocus{}; //보호 PE의5434e4 초기1/콘솔 포인터0 분기를 현재 상태로 유지한다.
    std::array<std::uint32_t,2> remembered{}; //5caf38/3c의 현재 기억한 두 float 비트다.
    std::vector<std::uint32_t> multiple; //다중 선택 SID 배열이며 단일 선택과 저장소를 합치지 않는다.
    std::int32_t multipleCount{}; //5caf34의 현재 signed 개수다.
    std::unique_ptr<RawCarrierInput> carrier; //같은 Session/로그인 전송을 사용하는 실제 커서/클릭 몸체다.
    std::shared_ptr<RawUserHighlight> highlight; //커서 수명과 원본 선택이 공유하는 같은 강조 몸체다.
    std::unique_ptr<RawUserSelection> selection; //현재 선택의 실제+7c/프로세스/마나/범위/음향을 분배한다.
    //구성 단계에는 선택이나 경로/전송 효과를 실행하지 않는다.
    Input(GameWorld& owner,UserSelectionHooks selectHooks,UserHighlightHooks highlightHooks,CarrierInputHooks carrierHooks):world(owner) {
        highlightHooks.project=[this](std::int32_t x,std::int32_t y) { const auto point=Project({x,y});return std::array<std::uint32_t,2>{std::bit_cast<std::uint32_t>(point.x),std::bit_cast<std::uint32_t>(point.y)}; };
        highlightHooks.move=[this](std::uint32_t sid,std::uint32_t x,std::uint32_t y,std::uint32_t force) { carrier->Highlight(sid,{std::bit_cast<float>(x),std::bit_cast<float>(y)},force); };
        carrierHooks.highlight=[this](std::uint32_t mode) { highlight->Highlight(mode); };carrierHooks.multiple=[this] { return multipleCount>0 ? 1U : 0U; };
        carrier=world.campaign_->MakeCarrierInput(std::move(carrierHooks));highlight=std::make_shared<RawUserHighlight>(world.HighlightSettings(),std::move(highlightHooks));
        selectHooks.enabled=[this] { return enabled; };selectHooks.blocked=[this] { return consoleFocus; };selectHooks.highlight=[this](std::uint32_t mode) { highlight->Highlight(mode); };
        selection=world.campaign_->MakeUserSelection({world.selected_,world.mouseType_,remembered,multiple,multipleCount},std::move(selectHooks));
    }
    //원본4971e0의 같은 현재 카메라/역투영을 사용한다.
    o::RawPathPoint Project(ScreenPoint point) const { const auto p=RawCursorPosition::ScreenToWorld(point.x,point.y,RawCursorPosition::CameraFromPixels(world.scrollX_,world.scrollY_));return {p[0],p[1]}; }
};
//실제Client와 동일 입력을 검사하는 호출자가 구체적인 장치를 공급한다.
void GameWorld::BindUserInput(UserSelectionHooks selection,UserHighlightHooks highlight,CarrierInputHooks carrier) {
    if (!campaign_ || input_) throw std::logic_error("실제 월드 입력의 새 수명 계약");input_=std::make_unique<Input>(*this,std::move(selection),std::move(highlight),std::move(carrier));
}
//선택과 별개인 현재 강조를 같은 커서와 공유한다.
void GameWorld::Highlight(std::uint32_t mode) { if (!input_) throw std::logic_error("실제 선택/강조 장치 미설치");input_->highlight->Highlight(mode); }
//취득한 건설 커서와 별개로 현재 이동 선택의 커서를 갱신한다.
void GameWorld::UpdateMovementCursor() { if (input_ && (highlightMode_==2 || highlightMode_==5)) input_->carrier->Highlight(selected_,input_->Project(mousePoint_),0); }
