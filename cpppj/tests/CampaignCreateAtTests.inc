// 실제 보호 미션의 같은 Runtime/Factory/Owner/HP/Pop을 사용한다. Kernel/AI 프레임이나 게임은 실행하지 않는다.
//문자열 명령이 실제 생성한 일반 제네레이터의 SID/HP/좌표/공급장부를 확인한다.
TEST_CASE(CampaignCreateAt_ActualFactoryOwnerHitPointsPopAndProviderList) {
    SessionServicesFixture external;auto session=SharedKernelSession(external.Services());session->Load();auto core=session->Core();const auto point=EmptySessionSupport(core);const auto sid=core.pool.FirstFree(false);const auto free=core.pool.FreeCount(),providers=core.bookkeeping.Providers().count;const auto money=session->Sp(1);const auto x=point.x+2,y=point.y+2;
    session->CreateAt("("+std::to_string(x)+'x'+std::to_string(y)+')'+core.types[76].name+"/9#1"); //비전투 일반TYPE만 만들며 미노출 명령에 새 화면을 붙이지 않는다.
    const auto raw=core.pool.Slot(sid);CHECK(raw[10]==76 && raw[34]==1 && !(raw[11]&4U));CHECK(core.pool.FreeCount()<free && core.bookkeeping.Providers().count==providers+1 && core.bookkeeping.Providers().Items().back()==sid.value);
    CHECK(Get(raw,26)==std::bit_cast<std::uint32_t>(session->HitPoints().MaxHitPoints(sid)) && std::bit_cast<float>(Get(raw,14))==static_cast<float>(x) && std::bit_cast<float>(Get(raw,18))==static_cast<float>(y));CHECK(session->World().Find(static_cast<float>(x),static_cast<float>(y),76)==sid && session->Sp(1)==money);
}
//실제 받침 생성/소유권 부모가 만든 지면owner를 새 명령의#0이 현재 위치에서 상속한다.
TEST_CASE(CampaignCreateAt_ActualCurrentSurfaceOwnershipInference) {
    SessionServicesFixture external;auto session=SharedKernelSession(external.Services());session->Load();auto core=session->Core();const auto point=EmptySessionSupport(core);CHECK(session->ConfirmSupport(static_cast<float>(point.x),static_cast<float>(point.y)));const auto packet=session->Construction().LastPacket();CHECK(packet.size()==45 && packet[26]==9);if (packet.size()!=45) return;
    float x=-10000,y=-10000;
    for (std::size_t index=0;index<9;++index) { const auto raw=core.pool.Slot(Sid{static_cast<std::uint16_t>(Get(packet,27+index*2,2))});x=std::max(x,std::bit_cast<float>(Get(raw,14)));y=std::max(y,std::bit_cast<float>(Get(raw,18))); } //실제로 생성한 받침 좌표에서 오른쪽 아래를 읽는다.
    session->World().SetSupportOwner(x,y,1);const auto surface=core.building.Surface(static_cast<std::int32_t>(x),static_cast<std::int32_t>(y));CHECK(surface.value && core.pool.Slot(surface)[34]==1);const auto sid=core.pool.FirstFree(false);const auto money=session->Sp(1);
    session->CreateAt("("+std::to_string(static_cast<int>(x))+'x'+std::to_string(static_cast<int>(y))+')'+core.types[76].name+"/0#0");CHECK(core.pool.Slot(sid)[34]==1 && session->World().Find(x,y,76)==sid && session->Sp(1)==money);
}
//없는 Runtime·현재Boss 실패와 signed좌표/미등록TYPE의 무효 입력을 실제 Pool에서 구별한다.
TEST_CASE(CampaignCreateAt_ActualLoadedAndBossBoundariesNeverInventAnObject) {
    SessionServicesFixture external;auto session=SharedKernelSession(external.Services());CHECK(Throws([&] { session->CreateAt("(20x21)rainGenerator#1"); }));session->Load();const auto core=session->Core();const auto free=core.pool.FreeCount();auto authority=session->World().Storage()->Authority();authority->boss=false;CHECK(Throws([&] { session->CreateAt("(20x21)rainGenerator#1"); }));CHECK(core.pool.FreeCount()==free);authority->boss=true;
    session->CreateAt("(0x21)rainGenerator#1");session->CreateAt("(20x0)rainGenerator#1");session->CreateAt("(20x21)no-such-type#1");CHECK(core.pool.FreeCount()==free);
}
