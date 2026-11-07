// Impl의 같은 현재 World/Template/Client 장치를 신전 상승 부모에 연결한다. 별도 상태 사본을 만들지 않는다.
    std::string VortexPlayerName(std::uint32_t owner) const { //00484480의 현재 이름을 기존 실제 Player/Chat 조회로 빌린다.
        if (!services.priestTakenName) throw std::logic_error("신전 상승의 실제 Player 이름 장치 미연결");const auto end=std::find(handle.begin(),handle.end(),std::uint8_t{});
        if (end==handle.end()) throw std::out_of_range("신전 상승 현재 이름의 NUL 누락");return services.priestTakenName(owner,{reinterpret_cast<const char*>(handle.data()),static_cast<std::size_t>(end-handle.begin())});
    }
    void SetVortexState(Sid sid,std::uint16_t state) { //004ac220의 WORD 저장 뒤 현재 특정TYPE 표면 통지를 실행한다.
        auto raw=core.pool.AllocatedBytes(sid);raw[12]=static_cast<std::uint8_t>(state);raw[13]=static_cast<std::uint8_t>(state>>8);
        if (core.pool.Slot(sid)[10]==gameplayGlobals->GameAux()->StateNotifyType()) core.building.NotifySurface(sid);
    }
    void InstallVortexDeparture() { // 같은 실제 자산 모듈을 만들기 전에 상승 Tick/수신/완료의 공통 하위를 구성한다.
        vortexAscendState.sharedNoIslandType=&core.bridgeState.noIslandType;
        VortexAscendHooks ascend;ascend.islandNumber=[this](float x,float y) { return ownership->IslandNumber(x,y); };
        ascend.reserveCandidateBuffer=[this] { auto memory=std::unique_ptr<std::uint8_t[]>(new(std::nothrow) std::uint8_t[0x50]);const bool allocated=memory!=nullptr;vortexCandidateBuffers.push_back(std::move(memory));return allocated; }; // 각 중첩 Tick의 실제80BYTE 할당 수명을 별도로 보관한다.
        ascend.releaseCandidateBuffer=[this] { if (vortexCandidateBuffers.empty()) throw std::logic_error("신전 상승 후보 버퍼의 실제 수명 불일치");vortexCandidateBuffers.pop_back(); };
        ascend.destroy=[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        vortexAscend=std::make_unique<RawVortexAscend>(core.pool,core.hash,core.spots,core.types,*islands,random,vortexAscendState,std::move(ascend));
        VortexDepartureHooks departure;departure.assertion=[storage=world.Storage()](auto expression,auto file,auto line) { storage->AssertDisplay(expression,file,line); };
        departure.state=[this](Sid sid,std::uint16_t state) { SetVortexState(sid,state); };
        departure.readTerritory=[this](auto owner,auto records) { if (!runtime) throw std::logic_error("신전 상승의 실제 Territory reader 미설치");runtime->ReadTerritory(owner,records); };
        departure.writeTerritory=[this](auto owner,auto records) { WriteEditorTerritory(owner,records); };
        departure.refreshProduction=[this] { if (!services.ownership.refreshProduction) throw std::logic_error("신전 상승의 실제 생산 갱신 미연결");services.ownership.refreshProduction(); };
        departure.snapshot=[this] { if (!vortexSnapshot) throw std::logic_error("신전 상승의 실제 현재 snapshot 부모 미설치");vortexSnapshot(); };
        departure.owner=[this](Sid sid,std::uint32_t owner) { ownership->SetOwner(sid,owner); };departure.destroy=[this](Sid sid,std::uint32_t flags) { world.Destroy(sid,flags); };
        departure.islandNumber=[this](float x,float y) { return ownership->IslandNumber(x,y); };
        departure.ownedIslands=[this](auto owner) { return RawIslandQueries(*islands,finishState).CountOwned(owner); };
        departure.requestEdit=[this] { if (!services.requestFortEdit) throw std::logic_error("신전 상승의 실제 Fort 편집 상태 요청 미연결");services.requestFortEdit(); };
        departure.log=[this](auto text) { if (!services.player.log) throw std::logic_error("신전 상승의 실제 로그 장치 미연결");services.player.log(29,text); };
        departure.sendExcept=[this](auto excluded,auto packet) { broadcast->SendExcept(excluded,packet); };
        departure.regular=[this](Sid sid) { static_cast<void>(core.processes.AddRegular(sid,3,0)); }; // 원문 new 실패는 새 예약 없이 후단 경고로 복귀한다.
        departure.warnAscend=[this](auto owner) { // 원문 이름 주소를 같은 설정/표시 부호화 경계로 전달한다.
            const auto name=VortexPlayerName(owner);if (!services.install.setConfig || !services.factoryTell) throw std::logic_error("신전 상승의 실제 설정/Tell 장치 미연결");
            services.install.setConfig("global.subject",{reinterpret_cast<const std::uint8_t*>(name.c_str()),name.size()+1});services.factoryTell("WarnAscend");
        };
        departure.sound=[this](auto name) { if (!services.sound.play) throw std::logic_error("신전 상승의 실제 전역 음향 미연결");services.sound.play(soundNames.Lookup(name),0,0,0,0,0); };
        vortexDeparture=std::make_unique<RawVortexDeparture>(core.pool,core.hash,core.types,chunks,*islands,*vortexAscend,playerState.battleOwner,core.processState.Server(),revision,std::move(departure));
    }
    void RequestVortexDeparture(Sid temple,std::uint32_t island) { //004dc220 event3의 방송 뒤 같은 직접 receiver를 호출한다.
        std::array<std::uint8_t,8> packet{1,0x51,8,0,static_cast<std::uint8_t>(island),static_cast<std::uint8_t>(temple.value),static_cast<std::uint8_t>(temple.value>>8),1}; // 보호 PE0052ee12의1/51h/8과 실제 완료BYTE다.
        broadcast->Send(packet);vortexDeparture->Receive(9,packet);
    }
