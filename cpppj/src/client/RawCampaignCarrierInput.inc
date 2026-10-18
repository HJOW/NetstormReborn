// Impl 정의 뒤 같은 namespace에서 포함하여 실제 사용자 명령을 동일 Session에 연결한다.
std::unique_ptr<RawCarrierInput> RawCampaignSession::MakeCarrierInput(CarrierInputHooks hooks) {
    if (!impl_->loaded) throw std::logic_error("설치 전 실제 운반체 사용자입력 요청");auto* s=impl_.get(); //동일 월드/로그인/소리 장부의 고정주소다.
    // 각 TYPE의 기존 자산 정의에 등록된 실제 음향 핸들만 읽는다.
    for (const auto& asset:s->assets.game.Types()) {
        auto& audio=s->inputAudio.at(70+asset.block); //원본+78은moveSound,+80은dropSound다.
        const auto move=asset.definition.String("moveSound"),drop=asset.definition.String("dropSound");audio.moveSound=move && !move->empty() ? s->soundNames.Lookup(*move) : 0;audio.dropSound=drop && !drop->empty() ? s->soundNames.Lookup(*drop) : 0;
    }
    s->inputState.priestType=s->priestState.movement.priest;const auto golemType=s->assets.game.TypeTable().Find("sunWalker"); //현재 원본 타입 조회값이며 -1을 유효번호로 바꾸지 않는다.
    if (golemType<0) throw std::logic_error("운반체 입력 골렘 타입 누락");s->inputState.golemType=static_cast<std::uint32_t>(golemType);
    s->inputDropState.mapLimit=static_cast<std::int32_t>(s->cursorManaGlobals.mapLimit);
    CarrierDropPointHooks drop;drop.mayPlace=[s](const CanonPlacementQuery& query) { s->PrepareConstruction();return s->MayPlaceGameplay(query); };drop.center=[s](Sid sid) { return s->gunTarget->Center(sid); };
    const auto point=std::make_shared<RawCarrierDropPoint>(s->core.pool,s->core.types,s->inputDropState,std::move(drop)); //평가와 실제 메뉴가 같은 기존 위치 몸체를 사용한다.
    hooks.immobile=[s](Sid sid) { return s->priests->PriestState().GroundImmobile(sid); };
    hooks.content=[s](Sid sid,std::uint32_t mask) { return s->priests->Carrier().FindContained(sid,mask,0,0); };
    hooks.candidate=[s](Sid sid,RawPathPoint p) { return s->motion->CarrierArrival().PickupCandidate(sid,p.x,p.y); };
    hooks.dropPoint=[point](Sid sid,std::uint32_t type) { return point->Find(sid,type); };
    hooks.graph=[s](Sid sid) { return s->anchor->GraphFor(sid.value); };
    hooks.connected=[s](RawPathPoint a,RawPathPoint b) { const auto first=s->anchor->GraphAt(a.x,a.y);return first==s->anchor->GraphAt(b.x,b.y); };
    hooks.surface=[s](RawPathPoint p) { const auto cell=RawCursorPosition::TruncatePoint({p.x,p.y});const auto x=static_cast<std::int32_t>(cell[0]),y=static_cast<std::int32_t>(cell[1]);if (x<0 || x>=256 || y<0 || y>=256) throw std::out_of_range("운반체 입력 spot 물리 범위");return std::uint32_t(s->core.spots[static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x)]); };
    hooks.geyser=[s](RawPathPoint p) { return s->world.Find(p.x,p.y,s->motionState.carrier.geyserType); };
    hooks.validGeyser=[s](Sid sid,Sid target) { return s->motion->CarrierArrival().ValidGeyser(sid,target); };
    hooks.returnPlace=[s](RawPathPoint p) { return s->motion->Cleanup().ReturnPlaceAt(p.x,p.y,0); };
    hooks.daisAt=[s](RawPathPoint p) { return s->damageablePost->GenusAt(p.x,p.y,0x400000); };
    hooks.altarAllowed=[s](Sid dais,Sid priest) { return s->sacrificeRequest->Allowed(dais,priest); }; // 실제 서버40h와 같은 현재 Dais 자격 판정이다.
    hooks.localOwner=[s] { return s->playerState.battleOwner; };hooks.checking=[s] { return s->gunState.checking; };
    hooks.earlySacrifice=[s] { if (s->globals.missionPost) return false;throw std::logic_error("일반전투 희생 제한시간 부모 미연결"); };
    // 원본4cfc10은 같은00594fa4가 비영인 캠페인/튜토리얼에서 보상창을 열지 않는다. 의식 이동 요청은 앞선Commit에서 그대로 보낸다.
    hooks.showSacrifice=[s,show=std::move(hooks.showSacrifice)](std::uint32_t dais) { if (s->globals.missionPost) return;if (!show) throw std::logic_error("일반전투 희생 메뉴 소비자 미연결");show(dais); };
    hooks.select=[s](std::uint32_t sid,std::uint32_t force,std::uint32_t activate) { auto link=s->activeSelection.lock();if (!link || !link->selection) throw std::logic_error("운반체 입력의 현재 선택 수명 미연결");static_cast<void>(link->selection->Select(sid,force,activate)); };
    hooks.request=[s](Sid sid,RawPathPoint p,std::uint32_t sacrifice) { s->carrierMove->Request(sid,p,sacrifice); };hooks.pickup=[s](Sid sid) { s->carrierPickup->Request(sid); };
    hooks.random=[s](std::int32_t low,std::int32_t high) { if (high<=low) throw std::invalid_argument("이동 음향 정수 난수 구간");return low+static_cast<std::int32_t>(s->random.Next(static_cast<std::uint32_t>(high-low))); };
    hooks.namedSound=[s](bool priest,std::uint32_t index) {
        const std::array<std::string_view,2> priests{"priestMove1.wav","priestMove3.wav"};const std::array<std::string_view,4> golems{"golemMove1.wav","golemMove2.wav","golemMove4.wav","golemMove5.wav"}; //실제531690/53169c 표이며 파일 번호를 연속이라고 가정하지 않는다.
        if (index>=(priest ? priests.size() : golems.size())) throw std::logic_error("원본 음향 이름 표 밖 인접 DWORD 직접 인덱스");const auto name=priest ? priests[index] : golems[index]; //자연 -1 선택은 원본처럼0..1/0..3이다.
        if (!s->services.sound.play) throw std::logic_error("이동 확인 음향 실제 장치 미연결");s->services.sound.play(s->soundNames.Lookup(name),0,0,0,1,0);
    };
    hooks.sound=[s](std::uint32_t sound) { if (!s->services.sound.play) throw std::logic_error("운반체 TYPE 음향 실제 장치 미연결");s->services.sound.play(sound,0,0,0,1,0); };
    return std::make_unique<RawCarrierInput>(s->core.pool,s->core.types,s->inputAudio,std::span(s->soundState.typeFrames).first(s->core.types.size()),s->soundState.frame,s->inputState,std::move(hooks));
}
