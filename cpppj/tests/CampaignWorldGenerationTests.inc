// 실제 로그인/원문 MissionStart 뒤 같은87BYTE/Pool로 세계 생성의 장식·전체 분기를 접합한다.
TEST_CASE(CampaignSession_WorldGenerationDecorationAndWholeActualPool) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const bool neutral:{false,true}) { //645에서 두 지역/중립 지면을 확인한Tutorial3과 소유된1-1을 따로 읽는다.
        ConfigInterface config([&](std::string_view path) { return assets.files.TryRead(path); });
        config.Configuration().Assign("DataDir=\"d\"\ncurrentLanguage=\"english\"\nmissionSpec=\"{DataDir}/{local.1}.{currentLanguage}\"\nfortSpec=\"{DataDir}/{local.1}.fort\"\n[END]\n");
        MissionScript script(config,neutral ? "Tutorial3":"TheWarBegins");CHECK(script.Loaded());
        for (const std::uint8_t layout:{std::uint8_t{1},std::uint8_t{2}}) { //단일 전체와9회 요청은 서로 독립된 실제 Session이다.
            for (const std::uint8_t decorations:{std::uint8_t{0},std::uint8_t{1}}) { //현재 옵션9의 비선택/50회 장식 요청을 명시 입력한다.
                SessionServicesFixture external;auto services=external.Services();UseWorldTestLogin(services);
                services.ai.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };services.ai.defaultName=[] { return std::string("AI"); }; //기존 검사와 같은 caller 미정 바이트/영어 외부 입력이다.
                services.deletion.emit=[&external](const SquidDestroyEvent&) { ++external.notices; }; //원문 NoTemple 초기화의 화면/음향 통지만 관측하고 실제 내부 삭제/소유권은 대체하지 않는다.
                auto options=std::make_shared<std::array<std::uint8_t,87>>();ResetMissionBattleOptions(OriginalEdition::Patch1078,*options);services.missionOptions=options;
                auto fort=FortTemplate::Parse(assets.files.Read(script.FortPath()),assets.game.TypeTable());
                RawCampaignSession session(assets.game,assets.files,std::move(fort),script.FortName(),std::move(services));session.LoadLocalConnection();auto core=session.Core();
                CHECK(core.ownerMode.battle && !core.ownerMode.fort && session.LocalConnectionPhase()==14 && session.PlayerKnown());
                MissionFrameGateState gate;MissionStartState state;CampaignMissionStartEffects effects;std::uint32_t startGeysers{};
                effects.settingsBytes=[](auto,auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };effects.gameSeconds=[] { return 0.0; };effects.refreshSp=[] {};effects.refreshCommandGump=[] {};effects.resetCamera=[] {}; //창/시계 장치만 명시 경계다.
                effects.randomGeysers=[&] { ++startGeysers;session.GenerateWorldGeysers(); }; //원문이 선택하면 같은 실제 생성 본체를 실행한다.
                RawCampaignMissionStartBindings start(session,script,gate,state,session.MissionStartSettings(),session.MissionAiSettings(),std::move(effects));start.Start();CHECK(gate.started==1);
                (*options)[9]=decorations;(*options)[17]=1;(*options)[18]=layout; //Start 뒤 현재 입력이며 설정/UI writer 검증은 아니다.
                auto players=session.PlayerRecords();
                for (std::size_t owner=1;owner<9;++owner) { Put(players,owner*180+0x2c,owner==1 ? 100U:0U);(*options)[62+owner]=0; } //예산은 실제 Player/숨김 BYTE에서 원문 getter가 계산한다.
                const auto capacity=core.pool.Capacity();std::vector<bool> before(capacity);std::uint32_t neutralGround{};
                for (std::uint32_t value=1;value<capacity;++value) { //생성 전 실제 live 슬롯과 중립 지면을 기록한다.
                    const auto raw=core.pool.Slot(Sid{static_cast<std::uint16_t>(value)});before[value]=(raw[11]&1U)==0;if (before[value] && raw[10]==97 && raw[34]==0) ++neutralGround;
                }
                const auto recent=session.Construction().LocalGeysers().Recent().count;
                session.GenerateWorldGeysers();CHECK(session.BattleOption(9)==decorations && session.BattleOption(18)==layout);
                std::array<std::uint32_t,4> added{};std::uint32_t geysers{};
                for (std::uint32_t value=1;value<capacity;++value) { //실제 새 live 객체와 공간/Regular 연결을 읽는다.
                    if (before[value]) continue;const Sid sid{static_cast<std::uint16_t>(value)};const auto raw=core.pool.Slot(sid);if (raw[11]&1U) continue;
                    const auto type=raw[10];const std::array<std::uint32_t,4> types{100,184,101,185};
                    const auto found=std::find(types.begin(),types.end(),type);if (found!=types.end()) { ++added[static_cast<std::size_t>(found-types.begin())];CHECK(!(raw[11]&8U)); }
                    if (type==122) { ++geysers;CHECK(!(raw[11]&15U) && Get(raw,12,2)>0 && core.processes.FindEvent(sid,0)!=nullptr); }
                }
                const auto total=added[0]+added[1]+added[2]+added[3];
                CHECK(decorations && neutral ? total>0 && total<=50 && added[3]<=10 : total==0);CHECK(geysers>0);if (neutral) CHECK(neutralGround>0);
                CHECK(session.Construction().LocalGeysers().Recent().count>=recent && external.constructionConfigReads>0);
                CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
                std::printf("WORLD_GENERATION source=%s neutral=%u layout=%u decorations=%u added=%u,%u,%u,%u geysers=%u startCalls=%u\n",std::string(script.FortName()).c_str(),neutralGround,unsigned(layout),unsigned(decorations),added[0],added[1],added[2],added[3],geysers,startGeysers);
            }
        }
    }
}

// 원문대본에쓰인건설마나134를현재1-1의실제섬/공간/Factory에배치하고잘못된TYPE로그도확인한다.
TEST_CASE(CampaignSession_AddRandomPlacesScriptManaAndLogsInvalidType) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();UseWorldTestLogin(services);
    std::vector<std::pair<std::uint32_t,std::string>> logs;
    services.player.log=[&](auto group,auto text) { logs.emplace_back(group,std::string(text)); }; // 실제 배치 부모의 원문29번 진단 출력만 기록한다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/TheWarBegins.fort"),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"TheWarBegins",std::move(services));session.LoadLocalConnection();auto core=session.Core();
    std::vector<bool> active(core.pool.Capacity());
    for (std::uint32_t value=1;value<core.pool.Capacity();++value) active[value]=(core.pool.Slot(Sid{static_cast<std::uint16_t>(value)})[11]&1U)==0; // 기존 live SID를 보관해 새 객체와 구별한다.
    CHECK((core.types[134].flags2&0x02000000U)!=0); // 원문타입의Anywhere비트가정의된대본대상을사용하며배치정책을강제허용으로바꾸지않는다.
    session.AddRandomObjects(134,1);std::size_t added{};
    for (std::uint32_t value=1;value<core.pool.Capacity();++value) { // 실제 성공한 건설 마나의 타입/owner/좌표를 읽는다.
        const auto raw=core.pool.Slot(Sid{static_cast<std::uint16_t>(value)});if (active[value] || (raw[11]&1U) || raw[10]!=134) continue;
        ++added;const auto x=std::bit_cast<float>(Get(raw,14)),y=std::bit_cast<float>(Get(raw,18));CHECK(raw[34]==session.LocalOwner() && x>=16 && x<240 && y>=16 && y<240 && !(raw[11]&15U));
    }
    CHECK(added==1);
    const auto snapshot=[&] { std::vector<std::uint8_t> bytes;for (std::uint32_t value=0;value<core.pool.Capacity();++value) { const auto raw=core.pool.Slot(Sid{static_cast<std::uint16_t>(value)});bytes.insert(bytes.end(),raw.begin(),raw.end()); }return bytes; }; // 잘못된 TYPE에는 전체 물리 슬롯의 변경이 없어야 한다.
    const auto before=snapshot();logs.clear();session.AddRandomObjects(0,1);session.AddRandomObjects(0xffffffffU,1);
    CHECK(snapshot()==before && logs.size()==2);for (const auto& [group,text]:logs) CHECK(group==0x1d && text=="AddRandom: Invalid Type\n"); // 원문과 같은 로그 뒤0 반환이며 가짜 객체/성공을 만들지 않는다.
    CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}

// 실제Session의비활성자기소유주문을점검색/타입·owner필터뒤가상삭제하며공격/프레임은실행하지않는다.
TEST_CASE(CampaignSession_DestroyAtUsesActualFinderAndVirtualDeletion) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;auto services=external.Services();UseWorldTestLogin(services);
    services.deletion.emit=[&](const SquidDestroyEvent&) { ++external.notices; }; // 외부표시/통지만기록하고실제내부삭제/공간/장부를대체하지않는다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/TheWarBegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"TheWarBegins",std::move(services));session.LoadLocalConnection();auto core=session.Core();
    const auto target=session.World().Create(114,2);core.owner.Set(target,session.LocalOwner());CHECK(!(core.pool.Slot(target)[40]&8U));session.World().Pop(target,10,10,0); // 기존Detonate연결검사에서확인한비활성114를실제Factory/owner/Pop으로준비하며Cast/폭발은요청하지않는다.
    CHECK((core.types[114].flags2&0x7c77ef00U)!=0 && !(core.pool.Slot(target)[11]&1U));const auto initial=core.pool.Slot(target);const std::vector<std::uint8_t> before(initial.begin(),initial.end());
    const auto type=core.types[114].name;CHECK(!type.empty() && assets.game.TypeTable().Find(type)==114);
    session.DestroyAt("(10x10)missing/25#1");CHECK(std::equal(before.begin(),before.end(),core.pool.Slot(target).begin()));
    session.DestroyAt("(10x10)"+type+"/25#2");CHECK(std::equal(before.begin(),before.end(),core.pool.Slot(target).begin()));
    session.DestroyAt("(10x10)"+type+"/25#1");CHECK((core.pool.Slot(target)[11]&1U)!=0);
    CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
