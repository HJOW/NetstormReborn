// 원문cf570을 같은 Client의 전체 HTML 부모/메타/본문·serial 전역에 연결한다.
HtmlWindowGumpNode& LoadingNode(std::uint32_t token) const { auto* node=Find(htmlRoots,token);if (!node) throw std::logic_error("미션 안내의 실제 전체 HTML 부모 없음");return *node; } // 부분 장식 저장소를 전체 부모로 가장하지 않는다.
std::uint32_t LoadingRoot() const { return htmlContext ? Reference(htmlContext->globals->window.transientSerial) : 0U; } // 실제 현재 transient가 가리키는 비dead 부모만 읽는다.
std::uint32_t ShowLoading(RawHtmlWindowContext context,std::string_view message,std::function<std::string()> cancelText) { // 같은 두 전역을 값 복제 없이 원문 본체에 빌린다.
    BindHtml(std::move(context));auto& state=htmlContext->globals->window;MissionLoadingMessageHooks h;h.cancelText=std::move(cancelText);h.findSerial=[this](auto serial) { return Reference(serial); };h.serial=[this](auto token) { return LoadingNode(token).serial; };h.updateDepth=[this](auto token)->std::uint32_t& { return LoadingNode(token).updateDepth; };
    h.metadata=[this](auto token,std::string& source) { //cf570의 논리1024BYTE와 메타가 변경한 같은 첫NUL을 보존한다.
        std::array<char,1024> buffer{};if (source.size()>=buffer.size()) throw std::length_error("미션 안내 메타1024BYTE 범위 초과");std::copy(source.begin(),source.end(),buffer.begin());htmlMetadata->Process(token,buffer);source.assign(buffer.data());
    };
    h.body=[this](auto token,auto source) { htmlWindow->SetBody(token,source); };h.create=[this](auto source,auto flags,auto width,auto height) { return htmlFactory->Show(source,flags,std::bit_cast<std::uint32_t>(width),std::bit_cast<std::uint32_t>(height)); };
    return ShowMissionLoadingMessage(MissionLoadingMessageView{state.transientSerial,state.createdSerial},message,h);
}
