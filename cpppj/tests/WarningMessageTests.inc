// 기존MessageGumpTests의같은192BYTE직렬화로Warning전체생성자와명시외부효과를대조한다.
namespace { //원문기대값은fixture에서만읽고입력문자열/시간만공급한다.
struct WarningMessageScene759:MessageScene {
    std::uint32_t source{},language{};double wall{}; //원문포인터/서식입력번호와벽시각의독립입력이다.
    std::array<std::optional<std::string>,5> inputs{std::string("Warning"),std::string(),std::string("A\0tail",6),std::string("~B~E~yWarning")+std::bit_cast<char>(std::uint8_t{0xff}),std::nullopt}; //NULL과빈문자열을서로다른ABI입력으로유지한다.
    std::optional<std::string_view> Source() const { if (inputs.at(source)) return std::string_view(*inputs.at(source));return std::nullopt; } //같은버퍼의명시길이뷰이며첫NUL절삭은제품서식위치에서수행한다.
    // 원문 문자열/벽시각과 실제 외부 효과의 호출 순서만 명시 장치로 공급한다.
    MessageGumpHooks WarningHooks() {
        auto hooks=Hooks(); //기존명시Text/이름/Style/측정을그대로사용한다.
        hooks.warningSound=[this] { Event("Sound"); };
        hooks.translate=[this](auto value) { Event("Translate,"+TextHex(value));const std::array<std::string_view,5> patterns{"%s","~E~w[%s]","","%.3s","%% %s"};return std::string(patterns.at(language)); };
        hooks.wallSeconds=[this] { Event("Wall,"+Hex(std::bit_cast<std::array<std::uint8_t,8>>(wall)));return wall; };
        hooks.idle=[this](auto token,auto enabled) { CHECK(token==kNode && enabled);Event("Idle,1");node.flags|=0x80U; };
        return hooks;
    }
};
}

// 실제문자열CRT까지실행한50원문입력의192BYTE/순서/NULL/첫NUL/서식결과를대조한다.
TEST_CASE(MessageGump_WarningNative50) {
    std::size_t count{}; //5본문×5서식×2벽시각의실제독립행이다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture(NETSTORM_WARNING_MESSAGE_FIXTURE).rows) {
        CHECK(row.size()==5);if (row.size()!=5) return;WarningMessageScene759 scene;scene.source=Number(row[0]);scene.language=Number(row[1]);
        const auto bytes=Bytes(row[2]);CHECK(bytes.size()==8);if (bytes.size()!=8) return;std::array<std::uint8_t,8> wall;std::copy(bytes.begin(),bytes.end(),wall.begin());scene.wall=std::bit_cast<double>(wall);
        MessageGump message(scene.create,scene.WarningHooks());CHECK(message.Construct(kNode,17,0,0,scene.Source())==kNode);
        CHECK(scene.Snapshot()==row[3]);CHECK(scene.trace==row[4]);++count;
    }
    CHECK(count==50);
}

// 입력문자열은이름/음향/번역뒤에현재첫NUL을읽고전역조회효과가쓴다른메시지필드는유지한다.
TEST_CASE(MessageGump_WarningReadsCurrentTextAfterSelectedEffects) {
    WarningMessageScene759 scene;std::array<char,32> input{};std::copy_n("before",7,input.begin());auto hooks=scene.WarningHooks();std::string shown;
    hooks.warningSound=[&] { scene.Event("Sound");std::copy_n("sound",6,input.begin());scene.node.player=8;scene.node.parameter=41;scene.node.deadline=700.0; };
    hooks.translate=[&](auto value) { CHECK(value=="%s");scene.Event("Translate");std::copy_n("translated",11,input.begin());return std::string("[%s]"); };
    hooks.style=[&](auto token,auto text) { CHECK(token==kNode);shown=text;scene.Event("Style");scene.node.head=0x12408000; };
    MessageGump message(scene.create,std::move(hooks));message.Construct(kNode,17,0,0,std::string_view(input.data(),input.size()));
    CHECK(shown=="[translated]" && scene.node.player==8 && scene.node.parameter==41 && scene.node.deadline==10.0 && (scene.node.flags&0x80U));
    CHECK(scene.trace=="Text;PlayerName,0;Sound;Translate;Style;Wall,0000000000000000;Idle,1;Fit,0,0");
}

// 선택장치실패는앞선공통필드/체인/기한만유지하고뒤Idle/Fit을앞당기지않는다.
TEST_CASE(MessageGump_WarningSelectedFailuresKeepNativePrefix) {
    for (std::uint32_t phase=1;phase<=5;++phase) { //음향/번역/Style/벽시각/Idle의독립실패지점이다.
        WarningMessageScene759 scene;scene.wall=2.5;auto hooks=scene.WarningHooks();
        if (phase==1) hooks.warningSound=[] { throw std::logic_error("sound stop"); };
        if (phase==2) hooks.translate=[](auto)->std::string { throw std::logic_error("translate stop"); };
        if (phase==3) hooks.style=[](auto,auto) { throw std::logic_error("style stop"); };
        if (phase==4) hooks.wallSeconds=[]()->double { throw std::logic_error("wall stop"); };
        if (phase==5) hooks.idle=[](auto,auto) { throw std::logic_error("idle stop"); };
        MessageGump message(scene.create,std::move(hooks));bool failed{};try { message.Construct(kNode,17,0,0,"Warning"); }catch (const std::logic_error&) { failed=true; }
        CHECK(failed && scene.node.kind==17 && scene.node.player==0 && scene.node.parameter==0 && scene.node.vtable==0x517b60U && !(scene.node.flags&0x80U));
        CHECK(scene.node.deadline==(phase==5 ? 12.5 : 0.0));CHECK(scene.node.head==(phase>=4 ? 0x12408000U : 0U));CHECK(scene.trace.find("Fit")==std::string::npos);
    }
}

// 제품호스트경계는NULL/빈/첫NUL을구별하고원문미공급서식인자와물리초과를합성하지않는다.
TEST_CASE(MessageGump_WarningTextAbiAndFormatLimits) {
    CHECK(FormatWarningMessage("%s",std::nullopt)=="(null)");CHECK(FormatWarningMessage("%s",std::string_view{})=="");CHECK(FormatWarningMessage("%s",std::string_view("A\0tail",6))=="A");
    CHECK(FormatWarningMessage(std::string_view("%s\0%q",5),"value")=="value");CHECK(FormatWarningMessage("%%",std::nullopt)=="%");CHECK(FormatWarningMessage("%-5.3s","abcdef")=="abc  ");
    CHECK(FormatWarningMessage("%s",std::string(255,'x')).size()==255);bool large{},missing{};
    try { FormatWarningMessage("%s",std::string(256,'x')); }catch (const std::length_error&) { large=true; }
    try { FormatWarningMessage("%s %s","one"); }catch (const std::logic_error&) { missing=true; }CHECK(large && missing);
}
