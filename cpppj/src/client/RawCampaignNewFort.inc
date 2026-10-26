// 실제 Session 내부에서 원본 신규 자료 부모와 같은 Client/Template/SP/Core/난수 소유자를 연결한다.
// 이 파일은 RawCampaignSession::Impl 안에서만 포함하며 별도 번역 단위로 컴파일하지 않는다.

// Subscriber 읽기와 새 자료 작성이 같은 실제 등록 설정/CRT/ID 초기화 장치를 사용한다.
FortSubscriberLoadHooks SubscriberRegistrationHooks() {
    FortSubscriberLoadHooks hooks;hooks.assertion=services.subscriber.assertion;
    hooks.readRegistered=services.subscriber.readRegistered;hooks.configure=services.subscriber.configure;
    hooks.generate=[this] {
        const auto owner=services.randomGlobals;if (!owner) throw std::logic_error("Subscriber 실제Client CRT 소유자 미연결");
        SubscriberIdGenerateHooks devices;devices.time32=services.unixSeconds;devices.clock=services.subscriber.clock;
        devices.expired=[this](auto level,auto text) { if (!services.player.log) throw std::logic_error("Subscriber ID 실제소진 로그 미연결");services.player.log(static_cast<std::uint32_t>(level),text); };
        return GenerateSubscriberId({owner->SubscriberIdInitialized()},BindSubscriberIdCrtRandom(owner->Crt(),std::move(devices)));
    };
    return hooks;
}

// 004bd400의 고정 ID/물리 handle과 등록 뒤 현재 manager를 같은 실제 typed stream에 저장한다.
void WriteNewFortSubscriber(std::uint32_t slot,std::uint32_t id,std::span<const std::uint8_t> name) {
    FortSubscriberWriteHooks hooks;std::optional<TemplateWriteStream> stream; // 원문 Create 반환의 수명을 이 호출 끝까지 고정한다.
    hooks.manager=[this](auto owner) { if (owner>=9) throw std::out_of_range("신규 Subscriber Template9 범위");return Read(templateStorage->Headers(),owner*20); };
    hooks.assertion=[](auto) { throw std::logic_error("신규 Subscriber 원문 Template 진단"); };
    hooks.registerSubscriber=[this] { FinishFortSubscriberRegistration({globals.mySubscriber},SubscriberRegistrationHooks()); };
    hooks.create=[&](auto manager,auto section) { stream=templateStorage->CreateManagerSection(manager,section);return 1U; };
    hooks.write=[&](auto token,auto bytes) { if (token!=1 || !stream) throw std::logic_error("신규 Subscriber 고정 stream 누락");templateStorage->WriteSection(*stream,bytes); };
    SaveFortSubscriber(slot,id,name.data() ? std::optional(name) : std::nullopt,hooks);
}

// 새 맵 초기 자료004b3920을 실제 현재 Client와 같은 저장 본체에 연결한다. 외곽의 State bit0/alternate/Subscriber 읽기는 여기서 앞당기지 않는다.
void CreateEditorFortData(const std::function<void(std::uint32_t)>& coreWrite) {
    if (!services.randomGlobals) throw std::logic_error("신규 Fort의 실제 Client 난수 소유자 누락");
    NewFortDataGlobals current{globals.mySubscriber,[this] { return std::span(handle); },[this] { return globals.editor; },
        [this] { return playerState.playerKnown; },[this] { return playerState.battleOwner; },[this] { return playerState.templateOwner; },
        coreDataGlobals->Active(),services.randomGlobals->FortCoreSeed()};
    NewFortDataHooks hooks;
    hooks.translate=[this](auto key)->std::string_view {
        if (!services.editorPlayerUi.translate) throw std::logic_error("신규 Fort Unnamed 실제 번역 장치 누락");const auto text=services.editorPlayerUi.translate(key);
        return {reinterpret_cast<const char*>(text.data()),text.size()}; // 실제 반환 버퍼를 빌리며 복사/NUL 검색은 원문 부모가 그 뒤 수행한다.
    };
    hooks.configure=[this](auto key,auto value) { if (!services.install.setConfig) throw std::logic_error("신규 Fort 실제 설정 장치 누락");services.install.setConfig(key,{reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); };
    hooks.title=services.install.refreshWindowTitle;hooks.registerSubscriber=[this] { FinishFortSubscriberRegistration({globals.mySubscriber},SubscriberRegistrationHooks()); };
    hooks.sp=[this](auto owner) { return std::bit_cast<float>(sp.Get(owner)); };hooks.addSp=[this](auto value) { hp.AddSp(playerState.battleOwner,value); };
    hooks.coreData=[this] { const std::uint32_t metamap=core.ownerMode.challenge ? 1U : 0U;return coreDataGlobals->Read(playerState,metamap,{SessionMissing<void,std::uint32_t>()}); };
    hooks.coreWrite=coreWrite;hooks.prepareRandom=[this] { static_cast<void>(seed.Reseed()); };hooks.nextRandom=[this](auto bound) { return random.Next(bound); };
    hooks.subscriber=[this](auto owner,auto id,auto bytes) { WriteNewFortSubscriber(owner,id,bytes); };
    hooks.territory=[this](auto owner,auto records) { WriteEditorTerritory(owner,records); };
    hooks.money=[this](auto owner) {
        FortMoneySaveHooks money;std::optional<TemplateWriteStream> stream; // Money도 Create가 선택한 노드만 같은 일반 writer로 쓴다.
        money.manager=[this](auto slot) { if (slot<0 || slot>=9) throw std::out_of_range("신규 Money Template9 범위");return Read(templateStorage->Headers(),static_cast<std::size_t>(slot)*20); };
        money.create=[&](auto manager,auto section) { stream=templateStorage->CreateManagerSection(manager,section);return 1U; };
        money.write=[&](auto token,auto bytes) { if (token!=1 || !stream) throw std::logic_error("신규 Money 고정 stream 누락");templateStorage->WriteSection(*stream,bytes); };
        SnapshotFortMoney(std::bit_cast<std::int32_t>(owner),random,money);
    };
    hooks.state=[this](auto owner) { return templateState->Get(owner); };hooks.notify=[this](auto owner) { templateState->Changed(owner); };
    CreateNewFortData(current,hooks);
}
