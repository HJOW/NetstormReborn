// 실제WarningMessages공장이kind17도공통이름조회/번역/음향/기한/Idle/측정을사용하는지확인한다.
TEST_CASE(Warnings_ActualWarningUsesSharedConstructorAndCurrentText) {
    std::size_t names{},translations{},sounds{},wallReads{};std::string order;std::array<char,32> input{};std::copy_n("before",7,input.begin());auto context=PauseContext(); //독립장치입력만공급하며실제Gump/StyleText는기존공장을쓴다.
    context.playerName=[&](auto owner) { CHECK(owner==0);++names;order+='N'; };
    context.translate=[&](auto value) { CHECK(value=="%s");++translations;order+='T';std::copy_n("translated",11,input.begin());return std::string("~E~y[%s]"); };
    WarningClient client([&] { ++wallReads;order+='W';return 3.25; },[&] { ++sounds;order+='S';std::copy_n("sound",6,input.begin()); },std::move(context));
    client.messages->Add(std::string_view(input.data(),input.size()));const auto rows=client.Rows();CHECK(rows.size()==1 && rows[0].source=="~E~y[translated]" && rows[0].deadline==13.25);
    CHECK(names==1 && translations==1 && sounds==1 && wallReads==1 && order=="NSTW" && client.Fields()[4]==1);const auto token=client.buttons.SerialGump(rows[0].serial);CHECK(client.messages->Node(token)->kind==17 && client.messages->Node(token)->player==0 && client.messages->Node(token)->parameter==0);
    CHECK(client.Pixels()!=std::vector<std::uint8_t>(1024*768,7));client.buttons.KillGump(token);client.buttons.CleanupGumps();CHECK(client.Rows().empty() && client.Fields()[1]==1);
}
// 실제20칸 공장과 StyleText에서 NULL/빈문자열/첫NUL 입력이 서로 다른 결과를 보존한다.
TEST_CASE(Warnings_ActualNullAndEmptyUseSameBoard) {
    std::size_t sounds{};WarningClient client([] { return 2.0; },[&] { ++sounds; });
    client.messages->AddNull();client.messages->Add("");client.messages->Add(std::string_view("A\0tail",6));
    const auto rows=client.Rows();CHECK(rows.size()==3 && sounds==3);
    if (rows.size()==3) { CHECK(rows[0].source=="(null)" && rows[1].source.empty() && rows[2].source=="A");CHECK(rows[0].y==0 && rows[1].y==12 && rows[2].y==24); }
    client.Reset();CHECK(client.Rows().empty() && client.Fields()[4]==0);
}
// 생성 중 이름/음향/번역/서식/벽시각/Style 부분 실패가 실제 등록표와 조각을 남기지 않고 같은 슬롯을 재사용한다.
TEST_CASE(Warnings_ActualConstructorFailureReleasesRegisteredText) {
    int phase{};auto context=PauseContext();
    context.playerName=[&](auto) { if (phase==0) throw std::logic_error("name stop"); };
    context.translate=[&](auto) { if (phase==2) throw std::logic_error("translation stop");if (phase==3) return std::string(256,'x');if (phase==5) return std::string("prefix~[I1.")+std::string(80,'1')+"]";return std::string("~E~y[%s]"); };
    WarningClient client([&] { if (phase==4) throw std::logic_error("wall stop after Style");return 1.25; },[&] { if (phase==1) throw std::logic_error("sound stop"); },std::move(context));
    const auto fields=client.Fields();const auto menus=client.menus->Report();
    // 벽시각 실패는 Style 완성 뒤, 마지막 입력은 prefix 조각 뒤 그림 프레임 표현식80BYTE에서 실패한다.
    for (phase=0;phase<6;++phase) {
        bool failed{};try { client.messages->Add("current"); }catch (const std::logic_error&) { failed=true; }
        const auto current=client.Fields();CHECK(failed && client.Rows().empty() && client.menus->Report()==menus);
        // 원문 serial 누계는 되돌리지 않고 살아 있는 등록·포커스·Idle·저장소만 복원한다.
        for (std::size_t index=0;index<fields.size();++index) if (index!=2) CHECK(current[index]==fields[index]);
        CHECK(current[2]==fields[2]+static_cast<std::uint32_t>(phase)+1);
    }
    phase=-1;client.messages->Add("current");const auto rows=client.Rows();CHECK(rows.size()==1 && rows[0].source=="~E~y[current]" && rows[0].y==0);
    client.Reset();const auto after=client.Fields();CHECK(client.Rows().empty() && client.menus->Report()==menus);
    // 루트 재생성도 serial만 증가하며 반납된 자원의 수를 바꾸지 않는다.
    for (std::size_t index=0;index<fields.size();++index) if (index!=2) CHECK(after[index]==fields[index]);
}
