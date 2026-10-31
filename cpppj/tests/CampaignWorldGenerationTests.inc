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
