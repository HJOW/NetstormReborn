// 생산 세션과 GameWorld의 표시/선택/명령이 같은 실제 raw 월드를 사용하는지 검사한다.
#include "DudeSpawnSupport.h"
#include "client/GameWorld.h"
#include "client/AiTypeRequiredEnergy.h"
#include "o/RawMissionAiSettings.h"
#include "o/RawGeyserAiRegistration.h"
#include <unordered_set>
using namespace netstorm::client;
namespace {
// 장치/UI/송신은 명시 검사 경계이며 생산 세션의 게임 계산을 대체하지 않는다.
struct SessionServicesFixture {
    SoundList names{OriginalEdition::Patch1078}; // 실제 이름표의 opaque sound handle이다.
    std::unordered_set<SoundHandle> playing; // 장치 외부 입력의 현재 재생 상태다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // 원본 부모의 명시 영어 번역 입력이다.
    std::vector<std::vector<std::uint8_t>> packets; // 실제 직렬화 송신 요청 전체바이트다.
    std::size_t notices{}; // 외부 UI/음향 요청의 관찰 개수다.
    std::size_t constructionConfigReads{},constructionRefreshes{}; // 명시 건설 요청만 선택해야 하는 실제 설정/표시 외부 경계다.
    // 같은 fixture가 세션보다 오래 살아야 하며 모든 실제 선택 경계를 명시한다.
    RawCampaignSessionServices Services() {
        names.Initialize();RawCampaignSessionServices s;s.unixSeconds=[] { return 946684800; };s.soundNames=&names;
        s.sound.wallSeconds=[] { return 100.0; };
        s.sound.playLoopAt=[this](float,float,SoundHandle current,SoundHandle sound) { const auto token=current ? current : sound;playing.insert(token);++notices;return token; };
        s.sound.playOnceAt=[this](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { ++notices;return sound; };
        s.sound.play=[this](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { playing.insert(sound);++notices;return sound; };
        s.sound.stop=[this](SoundHandle sound) { playing.erase(sound);++notices; };
        s.sound.isPlaying=[this](SoundHandle sound) { return playing.contains(sound) ? 1U : 0U; };
        s.priest.ownerNotice=[this](Sid) { ++notices; };s.priest.shield.noticeSound=[this](const PriestShieldNotice&) { ++notices; };
        s.priest.shield.translate=[](std::string_view key) { return std::string(key); };s.priest.shield.tell=[this](std::string_view) { ++notices; };
        s.ownership.refreshProduction=[this] { ++notices; };s.ownership.themeMode=[] { return 0U; };
        s.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };s.install.setConfig=[this](std::string_view,std::span<const std::uint8_t>) { ++notices; };
        s.install.refreshWindowTitle=[this] { ++notices; };s.install.resetCamera=[this] { ++notices; };
        s.assets.postPop.playImpactSound=[this](std::uint32_t) { ++notices; };
        s.assets.queueCommand=[this](std::uint32_t,std::int32_t,std::int32_t) { ++notices; };
        s.assets.sendGunPacket=s.pathPacket=[this](std::span<const std::uint8_t> bytes) { packets.emplace_back(bytes.begin(),bytes.end()); };
        s.archer.animation.common.sound=[this](Sid,std::string_view) { ++notices; };
        s.archer.missile.firingSound=s.archer.missile.impactSound=[this](float,float,std::uint32_t) { ++notices; };
        s.motion.path.stepSound=[this](Sid,float,float,std::uint32_t) { ++notices; };
        s.player.log=[this](std::uint32_t,std::string_view) { ++notices; };
        s.deletion.emit=[this](const SquidDestroyEvent& event) { if (event.effect!=SquidDestroyEffect::Transmit) throw std::logic_error("다른파생삭제미연결");++notices; };
        s.deletion.selected=[] { return Sid{}; };s.dependencyTransmit=[this](Sid,std::uint32_t) { ++notices; };
        s.construction.configInt=[this](std::string_view key,std::int32_t fallback) { CHECK(key=="predictableGeysers" && fallback==1);++constructionConfigReads;return 1; };
        s.construction.wallSeconds=[] { return 100.0; };s.construction.refresh=[this] { ++constructionRefreshes; };
        s.construction.negativeQuantity=[] { throw std::logic_error("실제 Session 검사에서 음수 생성수량 진단"); };
        s.construction.boundsAssertion=[](PlayerGeyserBoundsAssertion) { throw std::logic_error("실제 Session 검사에서 PlayerBounds 진단"); };
        return s; // 실제 부모 VT 통지와 Player AI 조회 래퍼를 세션이 공급한다.
    }
};
// 실제 원본 기본값 몸체로600바이트를 준비하고 명시 프로필만 덮어쓴다. 전략 프레임을 실행하는 입력은 아니다.
std::array<std::uint8_t,RawMissionAiSettings::kSize> SessionAiSettings(std::string_view name,std::string_view technology,std::uint32_t allies=0) {
    std::array<std::uint8_t,RawMissionAiSettings::kSize> profile;profile.fill(0xa7); // 생성자가 보존해야 할 미초기화 입력 바이트다.
    MissionAiSettingsState defaults;MissionAiSettingsHooks hooks; // Initialize에서는 설정 조회를 선택하지 않는다.
    hooks.get=[](std::string_view)->std::string { throw std::logic_error("AI 기본값 생성 중 선택하지 않은 설정 조회"); };
    RawMissionAiSettings settings(OriginalEdition::Patch1078,defaults,std::move(hooks));settings.Initialize(profile);
    std::copy(name.begin(),name.end(),profile.begin());profile[name.size()]=0;
    std::copy(technology.begin(),technology.end(),profile.begin()+30);profile[30+technology.size()]=0;
    Put(profile,0x22c,777);Put(profile,0x23c,allies);return profile;
}
// 실제 같은 지도의 빈3×3칸을 명시 배치 입력으로 선택한다. 확정 결과나 서버SID를 계산하지 않는다.
CellPoint EmptySessionSupport(const RawWorldCore& core) {
    // 가장자리 밖 원본 발자국을 피하고 실제 spot 하위6비트로만 입력 위치를 고른다.
    for (int y=4;y<248;++y) {
        // 같은 행의 왼쪽부터 자유로운 첫 정사각형을 찾는다.
        for (int x=4;x<248;++x) {
            bool empty=true;
            // 후보발자국의세행을검사하며막힌칸을찾은뒤에는다음후보로넘어간다.
            for (int dy=0;dy<3 && empty;++dy) {
                // 현재spot의실제점유비트를읽으며좌표/지형을합성하지않는다.
                for (int dx=0;dx<3;++dx) if (core.spots[(y+dy)*256+x+dx]&0x3fU) { empty=false;break; }
            }
            if (empty) return {x,y};
        }
    }
    throw std::logic_error("보호 Session의 빈3×3 받침 입력 위치 부재");
}
// 실제Stage3가읽을전역과직접SP지정이동일Session의원문32×9분산배열에만영향을주어야한다.
TEST_CASE(CampaignSession_MissionInputsUseSameScrambledSpSlots) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());
    CHECK(session.LocalOwner()==1 && session.PlayerKnown()==1 && session.Editor()==0);session.Load();
    CHECK(session.LocalOwner()==1 && session.PlayerKnown()==1 && session.Editor()==0 && session.AiMemory().Allocations().empty());
    const auto& store=session.SpStore();const auto* address=store.Pools().data();const auto* const* firstAddress=&store.FirstPoolCurrent(); // 실제장기분산배열과첫주소전역의주소다.
    const auto offset=store.Offset();const auto tracking=store.TrackingOffsetCurrent();const auto permutation=store.Permutation();const auto original=store.Pools();
    const auto notices=external.notices;constexpr std::uint32_t owner=1; // 실제미션로컬SP슬롯에직접float비트를쓴다.
    session.SetSp(owner,123.25F);CHECK(std::bit_cast<std::uint32_t>(session.Sp(owner))==std::bit_cast<std::uint32_t>(123.25F));
    CHECK(&session.SpStore()==&store && store.Pools().data()==address && &store.FirstPoolCurrent()==firstAddress && store.FirstPoolCurrent()==address);
    CHECK(store.Offset()==offset && store.TrackingOffsetCurrent()==tracking && store.Permutation()==permutation && store.Pools()!=original);
    std::uint32_t decoded{}; // API구현을호출하지않고실제원문slot에서독립적으로조립할32비트다.
    // 실제순열이선택한각배열의owner1보존비트와다른모든owner의물리DWORD를확인한다.
    for (std::uint32_t bit=0;bit<32;++bit) {
        const auto pool=static_cast<std::size_t>(permutation[bit]);if (store.Pools()[pool*9+owner]&(1U<<((offset+bit)&31U))) decoded|=1U<<bit;
        // Set이선택한owner외에는난수잡음조차쓰지않아야한다.
        for (std::uint32_t other=0;other<9;++other) if (other!=owner) CHECK(store.Pools()[pool*9+other]==original[pool*9+other]);
    }
    CHECK(decoded==std::bit_cast<std::uint32_t>(123.25F));const auto written=store.Pools();static_cast<void>(session.Sp(owner));CHECK(store.Pools()==written);
    session.SetSp(owner,std::bit_cast<float>(0x80000000U));CHECK(std::bit_cast<std::uint32_t>(session.Sp(owner))==0x80000000U);
    CHECK(external.notices==notices && session.AiMemory().Allocations().empty()); // 직접Set은AddSp의UI/지갑/AI생성을실행하지않는다.
    bool rejected=false;const auto valid=store.Pools();try { session.SetSp(9,1.0F); } catch (const std::out_of_range&) { rejected=true; }
    CHECK(rejected && store.Pools()==valid && store.Pools().data()==address);
}
// 첫객체전연결한참여조회가보호월드의실제사제목록/ownerState와동일한지확인한다.
TEST_CASE(CampaignSession_PriestParticipationUsesCurrentSharedState) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());
    auto& shared=session.Priests().Priests();CHECK(&session.PriestParticipation().Pool()==&session.Core().pool && &session.PriestParticipation().Priests()==&shared);
    CHECK(session.FindPriestParticipation(1)==0);shared.ownerState[0]=80;shared.ownerState[8]=88;session.Load();const auto core=session.Core();
    CHECK(RawPriestLookup(core.pool,shared).Find(8)==0 && shared.ownerState[0]==80 && shared.ownerState[8]==88); // Load에서없는소유자를자동2로고치지않는다.
    const auto live=RawPriestLookup(core.pool,shared).Find(1);CHECK(live!=0 && core.pool.Slot(Sid{static_cast<std::uint16_t>(live)})[10]==158);
    shared.ownerState.fill(77);const auto previous=shared.ownerState;CHECK(session.FindPriestParticipation(1)==live && shared.ownerState==previous);
    session.RunFrame(FrameTime{0,0,0,0},true);CHECK(shared.ownerState==previous); // 프레임도로그인타이머를대신하지않는다.
    session.RefreshPriestParticipation(1);CHECK(shared.ownerState[1]==0);session.RefreshPriestParticipation(8);CHECK(shared.ownerState[8]==2);
    const auto beforeInvalid=shared.ownerState;session.RefreshPriestParticipation(0);session.RefreshPriestParticipation(9);session.RefreshPriestParticipation(0xffffffff);
    CHECK(shared.ownerState==beforeInvalid && shared.ownerState[0]==77);
    CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 보호188TYPE/실제Factory로만든Carrier/Content/Altar/Dais의현재raw와실제해시를읽는다.
TEST_CASE(CampaignSession_PriestParticipationFindsRealCapturedFallbacks) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());session.Load();const auto core=session.Core();
    CHECK(RawPriestLookup(core.pool,session.Priests().Priests()).Find(8)==0 && session.FindPriestParticipation(8)==0);
    const auto carrier=session.World().Create(133,2),first=core.factory.CreateContained(2),second=core.factory.CreateContained(2); // 실제sunWalker/실제타입6생성자다.
    CHECK((core.types[133].flags2&0x30000U)!=0 && core.pool.Slot(first)[10]==6 && core.pool.Slot(second)[10]==6);
    auto firstRaw=core.pool.AllocatedBytes(first),secondRaw=core.pool.AllocatedBytes(second);Put(core.pool.AllocatedBytes(carrier),6,first.value,2);Put(firstRaw,4,second.value,2);Put(secondRaw,4,0,2);
    // 완성된포획체인의명시raw입력이다.실제포획트랜잭션이나부모통지를실행한것으로계산하지않는다.
    for (const auto form:{first,second}) { auto raw=core.pool.AllocatedBytes(form);Put(raw,16,carrier.value,2);Put(raw,18,158,2);Put(raw,20,0x20,2);raw[11]=8; }
    Put(firstRaw,12,1,2);Put(secondRaw,12,136,2);CHECK(session.Priests().Carrier().FindContained(carrier,0x20,158,0)==first);
    CHECK(session.FindPriestParticipation(8)==0); // 첫일치종속의원소유자가다르면같은Carrier의두번째종속을고르지않는다.
    Put(firstRaw,12,136,2);CHECK(session.FindPriestParticipation(8)==carrier.value);session.RefreshPriestParticipation(8);CHECK(session.Priests().Priests().ownerState[8]==1);
    const auto firstAltar=session.World().Create(164,2),firstDais=session.World().Create(165,2),lastAltar=session.World().Create(164,2),lastDais=session.World().Create(165,2);
    // 실제typed탐색에필요한현재좌표/버킷등록만입력한다.결과SID나포획bool을주입하지않는다.
    for (const auto pair:{std::array<Sid,2>{firstAltar,firstDais},std::array<Sid,2>{lastAltar,lastDais}}) {
        const float coordinate=pair[0]==firstAltar ? 100.75F : 110.75F;
        // 기준점과Dais는같은현재raw좌표를사용하며새등록의기존버킷꼬리를보존한다.
        for (const auto sid:pair) { auto raw=core.pool.AllocatedBytes(sid);Put(raw,14,std::bit_cast<std::uint32_t>(coordinate));Put(raw,18,std::bit_cast<std::uint32_t>(coordinate)); }
        Put(core.pool.AllocatedBytes(pair[0]),12,8,2);auto& head=core.hash.Bucket(1,coordinate,coordinate);auto daisRaw=core.pool.AllocatedBytes(pair[1]);Put(daisRaw,4,head,2);head=pair[1].value;daisRaw[33]=1;
    }
    CHECK(core.types[165].footX==7 && core.types[165].footY==7); // 보호dais.type의실제발자국은7×7이며독립합성fixture의3×3과구분한다.
    CHECK(session.FindPriestParticipation(8)==carrier.value);
    Put(firstRaw,12,1,2);CHECK(session.FindPriestParticipation(8)==lastAltar.value);session.RefreshPriestParticipation(8);CHECK(session.Priests().Priests().ownerState[8]==1);
    core.pool.AllocatedBytes(lastDais)[11]|=2;CHECK(session.FindPriestParticipation(8)==firstAltar.value);
    Put(core.pool.AllocatedBytes(firstAltar),12,264,2);CHECK(session.FindPriestParticipation(8)==0);session.RefreshPriestParticipation(8);CHECK(session.Priests().Priests().ownerState[8]==2);
    CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 실제Session의첫객체전바인딩이같은서버확정/예측삭제/패킷/공간으로들어가야한다.
TEST_CASE(CampaignSession_ConstructionSupportUsesSameWorld) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());
    const auto initial=session.Core().pool.FreeCount();bool rejected=false; // 설치전요청은실제공간쓰기이전에거부한다.
    try { session.ConfirmSupport(10,10); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && session.Core().pool.FreeCount()==initial && session.Construction().LastPacket().empty());
    session.Load();const auto core=session.Core();CHECK(&session.Construction().Pool()==&core.pool);
    CHECK(session.Construction().LastPacket().empty() && external.constructionConfigReads==0 && external.constructionRefreshes==0);
    const auto origin=EmptySessionSupport(core);const auto preview=session.World().Create(157,2); // 명시예측SID입력이다.
    const std::array<std::uint32_t,8> patterns{107,82,94,157,131,129,140,142}; // 실제기본Canon타입전역이며미리보기입력프레임선택에만쓴다.
    const auto decoder=DecodeCanonType(core.frames[157],0,patterns,{157,0,0,static_cast<float>(origin.x),static_cast<float>(origin.y),false},OriginalEdition::Patch1078);
    auto raw=core.pool.AllocatedBytes(preview);raw[40]|=1;Put(raw,36,std::bit_cast<std::uint32_t>(decoder.Frame()));
    CHECK(Get(raw,0)==kPatchNoIslandVtable); // 실제생성자VT로분배하며타입157만보고일반삭제를허용하지않는다.
    session.World().Pop(preview,static_cast<float>(origin.x),static_cast<float>(origin.y),2);
    CHECK(session.ConfirmSupport(static_cast<float>(origin.x),static_cast<float>(origin.y)));
    const auto bytes=session.Construction().LastPacket();CHECK(bytes.size()==45 && bytes[1]==0x3f && bytes[26]==9 && external.constructionRefreshes==1);
    CHECK((core.pool.Slot(preview)[11]&1U)!=0);
    // 동일wire에서읽은서버9SID가같은raw표면과spot에등록되어야한다.
    for (std::size_t index=0;index<bytes[26];++index) {
        const Sid sid{static_cast<std::uint16_t>(Get(bytes,27+2*index,2))};const auto placed=core.pool.Slot(sid);
        CHECK(sid.value>=core.pool.Layout().serverFirst && sid!=preview && placed[10]==157 && !(placed[11]&15U) && !(placed[40]&1U));
        const auto x=static_cast<int>(std::bit_cast<float>(Get(placed,14))),y=static_cast<int>(std::bit_cast<float>(Get(placed,18)));
        CHECK(core.hash.Entries(0)[y*256+x]==sid.value && (core.spots[y*256+x]&2U));
    }
    CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 실제Session의현재Player/실제Bounds/전용최근목록/같은보상pool/실제Geyser후처리를연결한다.
TEST_CASE(CampaignSession_LocalGeysersUseCurrentInputs) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());session.Load();const auto core=session.Core();
    const auto& local=session.Construction().LocalGeysers();CHECK(&local.Pool()==&core.pool && local.Recent().count==0 && local.ResourcePool()==0);
    auto records=session.PlayerRecords();
    // 이전가짜참여표없이현재원문flags만명시입력으로설정하며실제로드한지역은유지한다.
    for (std::size_t owner=1;owner<9;++owner) records[owner*0xb4+0x50]=0;
    local.ResourcePool()=9000;session.GenerateLocalGeysers();CHECK(local.Recent().count==0 && local.ResourcePool()==9000);
    records[0xb4+0x50]=2;CHECK(Get(records,0xb4+0x78)!=127);
    session.GenerateLocalGeysers();CHECK(local.Recent().count==3 && local.ResourcePool()==3000 && external.constructionConfigReads==2);
    CHECK(session.Construction().LastPacket().size()==45 && external.constructionRefreshes!=0);
    // 최근목록의실제SID에최초수량2000과실제Regular가부착되어야한다.
    for (const auto value:local.Recent().Items()) {
        const Sid sid{static_cast<std::uint16_t>(value)};const auto raw=core.pool.Slot(sid);
        CHECK(raw[10]==122 && Get(raw,12,2)==2000 && !(raw[11]&15U));
        CHECK(core.processes.FindEvent(sid,0)!=nullptr);
    }
    records[0xb4+0x50]=0;session.GenerateLocalGeysers();CHECK(local.Recent().count==3 && local.ResourcePool()==3000);
    // 원본권한/요새guard는호출시점현재core모드를읽어생성을생략한다.
    records[0xb4+0x50]=2;core.ownerMode.fort=true;const auto free=core.pool.FreeCount();session.GenerateLocalGeysers();
    CHECK(core.pool.FreeCount()==free && local.Recent().count==3 && local.ResourcePool()==3000);core.ownerMode.fort=false;
    CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 실제CreateAi뒤간헐천등록은미복원AI하위를명시거부하며임시성공이나빈AI로대체하지않는다.
TEST_CASE(CampaignSession_LocalGeysersKeepNonnullAiBoundary) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto services=external.Services();
    services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };
    services.ai.participation.clearChat=[&external](std::uint32_t) { ++external.notices; };
    services.ai.participation.addChat=[&external](std::uint32_t,std::string_view,std::uint32_t) { ++external.notices; };
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();
    const auto token=session.CreateAi(2,SessionAiSettings("Explicit","sunCannon"));auto records=session.PlayerRecords();records[0xb4+0x50]=2;
    CHECK(token!=0 && session.Participation().Brain(2)==token);bool rejected=false;
    try { session.GenerateLocalGeysers(); } catch (const GeyserAiRegistrationBoundary& boundary) {
        rejected=true;CHECK(boundary.owner==2 && boundary.aiPointer==token && boundary.state==1);
        CHECK(boundary.sid>=session.Core().pool.Layout().serverFirst && session.Core().pool.Slot(Sid{static_cast<std::uint16_t>(boundary.sid)})[10]==122);
    }
    CHECK(rejected && session.Participation().Brain(2)==token && session.AiMemory().Contains(token));
    CHECK(session.Construction().LocalGeysers().Recent().count==0); // 원본순서에서실패한Pop뒤최근목록추가는실행하지않는다.
    session.RemoveAi(2);CHECK(session.Participation().Brain(2)==0);
}
// 같은 Session의 실제 본체/기술/동맹/현재 그래프 getter와 정상 목록 소멸을 함께 검사한다.
TEST_CASE(CampaignSession_AiUsesCurrentWorldAndLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto services=external.Services();
    std::uint32_t chatClears=0,chatAdds=0,alliancePackets=0,nameReads=0; // 선택된 실제 외부 경계의 순서/인자를 관찰한다.
    services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xb6}); };
    services.ai.defaultName=[&] { ++nameReads;return std::string("Current AI"); };
    services.ai.participation.clearChat=[&](std::uint32_t channel) { CHECK(channel==0x40 || channel==0x80);++chatClears; };
    services.ai.participation.addChat=[&](std::uint32_t,std::string_view,std::uint32_t channel) { CHECK(channel==0x40 || channel==0x80);++chatAdds; };
    services.ai.participation.sendAlliance=[&](std::uint32_t server,std::uint32_t owner,std::uint32_t mask) { CHECK(server==9 && owner==1 && mask==6);++alliancePackets; };
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto records=session.PlayerRecords();
    CHECK(session.Participation().Brain(2)==0 && session.AiMemory().Allocations().empty());
    const auto core=session.Core();CHECK(core.types[74].flags2==assets.game.TypeTable().Types()[74].flags2);
    core.bookkeeping.invalidGraph=252;const auto profile=SessionAiSettings("","sunWalker;sunCannon",2);const auto token=session.CreateAi(2,profile);
    CHECK(token!=0 && session.Participation().Brain(2)==token && session.AiMemory().Contains(token));
    const auto raw=session.AiMemory().Bytes(token);CHECK(raw.size()==RawAiBrainConstructor::kBytes && Get(raw,0x260)==2 && Get(raw,0x2d8)==777);
    CHECK(Get(raw,0x298)==252 && Get(raw,0x2ac)==252 && nameReads==1);
    CHECK(std::string(reinterpret_cast<const char*>(records.data()+2*0xb4+12))=="Current AI");
    CHECK(session.FindTechnology(2,assets.game.TypeTable().Find("sunCannon")).value!=0);
    CHECK((Get(records,2*0xb4+0x50)&0x20)!=0 && Get(records,2*0xb4+0xb0)==token && Get(records,2*0xb4+0x2c)==10002);
    CHECK(session.Alliances()[2*9+1]==1 && session.Alliances()[1*9+2]==1 && session.Alliances()[2*9+3]==0 && session.Alliances()[3*9+2]==0);
    CHECK(chatClears==2 && chatAdds>=1 && alliancePackets==1 && session.AiMemory().Allocations().size()==18);
    const auto unchanged=session.AiMemory().Bytes(token);const std::vector<std::uint8_t> before(unchanged.begin(),unchanged.end());
    const auto changed=SessionAiSettings("Ignored","windArcher");CHECK(session.CreateAi(2,changed)==token);
    CHECK(std::equal(before.begin(),before.end(),session.AiMemory().Bytes(token).begin()) && nameReads==1 && session.AiMemory().Allocations().size()==18);
    session.RemoveAi(2);CHECK(session.Participation().Brain(2)==0 && !session.AiMemory().Contains(token));
    CHECK(session.AiMemory().Frees().size()==18 && session.AiMemory().Frees().back()==token && (Get(records,2*0xb4+0x50)&0x20)!=0);
    CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 언어/Chat/할당 누락을 생존 표식 성공으로 바꾸지 않고 선택 위치의 부분 효과와 실제 해제를 보존한다.
TEST_CASE(CampaignSession_AiMissingServicesKeepRealFailure) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    RawCampaignSession noAllocation(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());auto profile=SessionAiSettings("Explicit","sunCannon");
    bool rejected=false;try { noAllocation.CreateAi(2,profile); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected && noAllocation.AiMemory().Allocations().empty());
    noAllocation.Load();rejected=false;try { noAllocation.CreateAi(2,profile); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && noAllocation.Participation().Brain(2)==0 && noAllocation.AiMemory().Allocations().empty());
    auto second=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto services=external.Services();
    services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x91}); };
    RawCampaignSession noChat(assets.game,assets.files,std::move(second),"thewarbegins",std::move(services));noChat.Load();rejected=false;
    try { noChat.CreateAi(2,profile); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && noChat.Participation().Brain(2)==0 && noChat.AiMemory().Allocations().size()==18 && noChat.AiMemory().Frees().size()==18);
    auto third=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());services=external.Services();
    services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x83}); };
    services.ai.participation.clearChat=[](std::uint32_t) {};services.ai.participation.addChat=[](std::uint32_t,std::string_view,std::uint32_t) {};
    RawCampaignSession noLanguage(assets.game,assets.files,std::move(third),"thewarbegins",std::move(services));noLanguage.Load();profile=SessionAiSettings("","sunCannon");rejected=false;
    try { noLanguage.CreateAi(2,profile); } catch (const std::logic_error&) { rejected=true; }
    const auto attached=noLanguage.Participation().Brain(2);CHECK(rejected && attached!=0 && noLanguage.AiMemory().Contains(attached));
    noLanguage.RemoveAi(2);CHECK(noLanguage.Participation().Brain(2)==0 && !noLanguage.AiMemory().Contains(attached) && noLanguage.AiMemory().Frees().size()==18);
}
// Color는 기존 brain보다 먼저 같은 실제 색 표/섬 프레임을 바꾼 뒤 선택형 화면 소비자에 통지한다.
TEST_CASE(CampaignSession_AiColorPrecedesExistingBrain) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto services=external.Services();std::uint32_t colors=0;
    services.ai.allocationBytes=[](std::span<std::uint8_t> bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x62}); };
    services.ai.participation.clearChat=[](std::uint32_t) {};services.ai.participation.addChat=[](std::uint32_t,std::string_view,std::uint32_t) {};
    RawCampaignSession* current{};services.ai.changeColor=[&](std::uint32_t owner,std::uint32_t color) { CHECK(owner==2 && color==3 && current->Participation().Brain(owner)!=0 && current->Core().bridgeState.ownerColors[2]==3);++colors; };
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));current=&session;session.Load();
    auto profile=SessionAiSettings("Explicit","sunCannon");const auto token=session.CreateAi(2,profile);Put(profile,0x250,3);
    CHECK(session.CreateAi(2,profile)==token && colors==1 && session.AiMemory().Allocations().size()==18);
    // 명시 RemoveAi 없이도 실제 세션 destructor가 살아 있는 현재 brain을 먼저 소멸/반납한다.
}
// 값형 기본값과 다른 현재 getter 및 Chat callback 이후 변경이 다음 원본 읽기에 반영되는지 확인한다.
TEST_CASE(CampaignSession_ParticipationUsesCurrentReads) {
    std::array<std::uint8_t,9*0xb4> players{};std::array<std::uint32_t,81> allies{};std::array<std::uint8_t,9> hidden{}; // 한 실제 기록/방향별표/숨김표다.
    PlayerParticipationState stale;stale.localOwner=8;stale.playerKnown=0; // 실제 getter가 이 값형 사본을 읽으면 검사가 실패한다.
    std::uint32_t owner=1,known=1,assigns=0,sends=0;bool battle=true,metamap=false,localTransport=true;std::vector<std::string> names; // 실제 외부 전역과 관찰 결과다.
    // 각 현재 Player에 이름/subscriber와 자기 동맹을 준비한다.
    for (std::uint32_t index=0;index<9;++index) {
        const auto name="P"+std::to_string(index);std::copy(name.begin(),name.end(),players.begin()+index*0xb4+12);Put(players,index*0xb4+0x2c,index+100);allies[index*9+index]=1;
    }
    allies[2*9+3]=1;PlayerParticipationHooks hooks;
    hooks.assignConnection=[&](std::int32_t,std::int32_t) { ++assigns; };
    hooks.clearChat=[&](std::uint32_t channel) { if (channel==0x40) owner=2; };
    hooks.addChat=[&](std::uint32_t,std::string_view name,std::uint32_t channel) { CHECK(channel==0x40);names.emplace_back(name);if (name=="P2") owner=3; };
    hooks.sendAlliance=[&](std::uint32_t server,std::uint32_t player,std::uint32_t mask) { CHECK(server==9 && player==1 && mask==4);++sends; };
    RawPlayerParticipation current(OriginalEdition::Patch1078,players,allies,hidden,stale,std::move(hooks));
    current.BindCurrentInputs({[&] { return owner; },[&] { return known; },[&] { return battle; },[&] { return metamap; },[&] { return localTransport; }});
    CHECK(current.CheckLogin(1,1) && assigns==0);current.SetAlliance(3,1U<<2);
    CHECK(names==std::vector<std::string>({"P2","P3"}) && owner==3);
    current.AddAlly(1,2);CHECK(sends==1 && allies[1*9+2]==1 && allies[2*9+1]==0);
    bool rejected=false;try { current.BindCurrentInputs({}); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
    known=0;names.clear();current.SetAlliance(3,0);CHECK(names.empty());
}
// 실제 Session의 원문/장기 latch와 선택형 UI 경계를 같은 수명에서 검사한다. 원본 초기화 입력을 대신하지 않는다.
TEST_CASE(CampaignSession_MissionOutcomeSharesCurrentWorld) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());auto services=external.Services();
    RawCampaignSession* active{};std::uint32_t demo=0,blocked=0,close=0,notified=0;std::vector<std::string> sections;
    services.missionOutcome.demo=[&] { return demo; };services.missionOutcome.tellBlocked=[&] { return blocked; };
    // 첫 실제 UI callback이 같은 Player9/사제 상태를 바꾸면 다음 원본 조건은 즉시 현재 값을 읽어야 한다.
    services.missionOutcome.section=[&](std::string_view key) {
        sections.emplace_back(key);
        if (key!="ai1TempleHalfDead") return false;
        auto records=active->PlayerRecords();Put(records,0xb4+0x7c,0);records[2*0xb4+0x50]=0;
        active->Priests().Priests().ownerState[1]=1;demo=1;return true;
    };
    services.missionOutcome.closeTransient=[&] { ++close; };services.missionOutcome.onSection=[&] { ++notified; };
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));active=&session;session.Load();
    CHECK(sections.empty() && close==0 && notified==0);auto records=session.PlayerRecords();auto alliances=session.Alliances();
    CHECK(records.size()==9*0xb4 && alliances.size()==81);
    // 이 검사는 명시 입력으로 현재 참여 조건을 설정한다. 실제 로그인/AI 초기화 구현의 증거로 합산하지 않는다.
    records[0xb4+0x50]=2;records[2*0xb4+0x50]=0x20;alliances[10]=7;alliances[20]=9;
    const Sid temple{static_cast<std::uint16_t>(Get(records,0xb4+0x7c))};CHECK(temple.value!=0);
    Put(session.Core().pool.AllocatedBytes(temple),26,0);session.CheckMissionOutcome();
    CHECK(sections.front()=="ai1TempleHalfDead");CHECK(close==1 && notified==1);
    CHECK(std::find(sections.begin(),sections.end(),"Demoai1TempleDead")!=sections.end());
    CHECK(std::find(sections.begin(),sections.end(),"Demoai1PriestCaptured")!=sections.end());
    CHECK(std::find(sections.begin(),sections.end(),"DemoBadTeamDead")!=sections.end());
    CHECK(std::none_of(sections.begin(),sections.end(),[](const auto& key) { return key.find("ai2")!=std::string::npos; }));
    const auto count=sections.size();session.CheckMissionOutcome();CHECK(sections.size()==count);
}
// 생산 세션의 실제 기술 생성/삭제가 같은 Player 체인과 Form 수명을 사용해야 한다.
TEST_CASE(CampaignSession_TechnologyUsesCurrentLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    auto services=external.Services();std::uint32_t uiLookups=0;
    services.playerContent.gump=[&](std::uint32_t) { ++uiLookups;return 0U; };
    services.playerContent.handle=[](std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { throw std::logic_error("없는 UI 대상 처리"); };
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();
    const auto type=assets.game.TypeTable().Find("sunCannon");CHECK(type>=70);
    session.AddTechnology(1,"sunCannon");const auto first=session.FindTechnology(1,type);CHECK(first.value!=0);
    session.AddTechnology(1,"sunCannon;sunCannon");CHECK(session.FindTechnology(1,type)==first);
    const auto core=session.Core();CHECK(core.pool.Slot(first)[10]==6);
    session.ClearTechnology(1);CHECK(session.FindTechnology(1,type).value==0);
    CHECK((core.pool.Slot(first)[11]&1U)!=0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
    CHECK(core.bookkeeping.depth==0 && uiLookups==0);
}
// 실제1-1의신전/Player/금지표/기술내용물이한생산저장소를사용하는지확인한다.
TEST_CASE(CampaignSession_ProductionUsesCurrentPlayerFactoriesAndRestrictions) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    auto services=external.Services();std::size_t allocations{},configReads{}; // 실제선택된할당과설정조회만관찰한다.
    services.production.allocationBytes=[&](std::span<std::uint8_t> bytes) { CHECK(bytes.size()==kProductionDeckBytes);++allocations;std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa7}); };
    services.production.effects.configInt=[&](std::string_view key) { CHECK(key=="getTeleportTechFree");++configReads;return 0; };
    services.production.effects.modeAssertion=[] { throw std::logic_error("설치된실제미션의Fort/Battle모드진단"); };
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();
    const auto core=session.Core();const auto& production=session.Production();CHECK(&production.Pool()==&core.pool && allocations==0 && configReads==0);
    CHECK(core.bookkeeping.factories.entries.size()==500 && core.bookkeeping.factories.count==2);const auto factoryCount=core.bookkeeping.factories.count;
    const auto token=production.RefreshCurrent();CHECK(token!=0 && allocations==1 && configReads==1);
    CHECK(Get(session.PlayerRecords(),session.LocalOwner()*0xb4+0x68)==token && production.Deck().Player().Records().data()==session.PlayerRecords().data());
    CHECK(production.Contains(133) && production.Allowed(133));CHECK(production.RefreshCurrent()==token && allocations==1 && configReads==2);
    session.TotalMade().Allow(133,0);CHECK(!production.Allowed(133));CHECK(production.RefreshCurrent()==token && !production.Contains(133));
    session.TotalMade().Allow(133,1);production.RefreshCurrent();CHECK(production.Contains(133));
    core.bookkeeping.localSecondaryCounts[133]=17;core.bookkeeping.globalCounts[133]=23;
    CHECK(session.TotalMade().Built(133)==17 && session.TotalMade().Global(133)==23);
    const auto technology=assets.game.TypeTable().Find("sunCannon");production.IndicateTechnology(static_cast<std::uint32_t>(technology));
    const auto first=session.FindTechnology(static_cast<std::int32_t>(session.LocalOwner()),technology);CHECK(first.value!=0);
    production.IndicateTechnology(static_cast<std::uint32_t>(technology));CHECK(session.FindTechnology(static_cast<std::int32_t>(session.LocalOwner()),technology)==first);
    session.ClearTechnology(static_cast<std::int32_t>(session.LocalOwner()));CHECK(session.FindTechnology(static_cast<std::int32_t>(session.LocalOwner()),technology).value==0);
    CHECK(core.bookkeeping.factories.count==factoryCount && core.bookkeeping.depth==0);
}
// 실제 할당 입력이 없으면 원본 Player 포인터를 붙이거나 0채움 성공으로 진행하지 않아야 한다.
TEST_CASE(CampaignSession_ProductionMissingAllocationLeavesPlayerUnchanged) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());
    const std::vector<std::uint8_t> before(session.PlayerRecords().begin(),session.PlayerRecords().end());
    CHECK(Throws([&] { session.Production().RefreshCurrent(); }));CHECK(std::equal(before.begin(),before.end(),session.PlayerRecords().begin()));
}
// 같은 생산 월드의 empty84는 관찰 카운터 대신 실제 공통 삭제/반납으로 들어가야 한다.
TEST_CASE(CampaignSession_EmptyGeyserUsesCurrentDeletion) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Services());session.Load();
    const auto core=session.Core();const auto free=core.pool.FreeCount();
    const auto empty=session.World().Create(84);session.World().Pop(empty,10,10,0);
    CHECK(session.World().Destroy(empty,0x400000));
    CHECK(core.pool.FreeCount()==free && (core.pool.Slot(empty)[11]&1U)!=0);
    CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 실제1-1/1-2를 production GameWorld에 설치하고 같은 SID/프레임 표시를 확인한다.
void CheckSessionWorld(std::string_view mission) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/"+std::string(mission)+".fort"),assets.game.TypeTable());
    auto services=external.Services();services.install.resetCamera={}; // 실제 GameWorld 카메라가 로더의 소비자를 공급해야 한다.
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},std::string(mission),std::move(services),{1024,768});world.Resize(1024,768);
    const auto core=world.Surfaces().Core();Sid priest{};
    // 실제 live 슬롯에서 내 사제를 선택한다. 별도 순번 객체를 만들지 않는다.
    for (const auto& object:world.Surfaces().Objects()) if (object.object.type==158 && object.object.owner==1) { priest=object.sid;break; }
    CHECK(priest.value>=15000 && world.Object(priest.value)!=nullptr);
    CHECK(world.Object(priest.value)->x==std::bit_cast<float>(Get(core.pool.Slot(priest),14)));
    const auto& type=core.types[core.pool.Slot(priest)[10]];const auto view=world.Object(priest.value); // 실제 초기 카메라는 내 사제의 전체 발자국 차감 지점을 중심으로 둔다.
    CHECK(world.ToScreen(view->x-type.footX,view->y-type.footY).x==512 && world.ToScreen(view->x-type.footX,view->y-type.footY).y==384);
    CHECK(!world.Sprites().empty());
    // 실제 캠페인의 레인 신전은 현재 물결만 제출하지 않고 기본0과 같은 항목으로 그린다.
    if (mission=="thewarbegins") {
        const auto& temple=assets.game.Find("rainVortex");bool found=false;
        // 화면에 제출된 실제 신전의 원본 Draw 순서와 전체 몸체 클릭 범위를 확인한다.
        for (const auto& sprite:world.Sprites()) if (sprite.block==temple.block) {
            CHECK(sprite.frame==0 && sprite.followingFrames==std::vector<std::size_t>{1});found=true;
            const auto point=world.ToScreen(89,116);CHECK(world.Pick({point.x-112,point.y-80})==15002);
        }
        CHECK(found);
    }
    // paused 프레임도 실제 절대 시각은 공유하고 Kernel은 진행하지 않는다.
    const auto kernel=core.kernel.Size();world.frameTime=[] { return FrameTime{0,1,0,1}; };world.paused=[] { return true; };world.RunFrame();
    CHECK(core.kernel.Size()==kernel && world.Object(priest.value)!=nullptr);
    world.frameTime={};world.paused={};world.Step(1.0/60.0);
    CHECK(core.bookkeeping.depth==0 && !world.Report().empty());
}
}
// 보호 첫 캠페인도 실제 세션이 같은 raw 지도/객체를 표시한다.
TEST_CASE(CampaignSession_ActualWarBeginsGameWorld) { CheckSessionWorld("thewarbegins"); }
// 보호 둘째 캠페인도 같은 production 소유자를 사용한다.
TEST_CASE(CampaignSession_ActualMasterGameWorld) { CheckSessionWorld("masterofwhirligigs"); }

// 같은 production GameWorld의 선택/좌클릭은 원본 BFS/Host54를 만들고 현재 슬롯을 이동한다.
void CheckSessionMovement(bool actualBroadcast) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    auto services=external.Services();
    if (actualBroadcast) { services.assets.sendGunPacket={};services.pathPacket={}; }
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});world.Resize(1024,768);
    auto core=world.Surfaces().Core();Sid priest{};
    // 현재 raw 내 사제만 선택한다.
    for (const auto& object:world.Surfaces().Objects()) if (object.object.type==158 && object.object.owner==1) { priest=object.sid;break; }
    CHECK(priest.value!=0);const auto initial=std::array<std::uint32_t,2>{Get(core.pool.Slot(priest),14),Get(core.pool.Slot(priest),18)};
    const int x=static_cast<int>(std::bit_cast<float>(initial[0])),y=static_cast<int>(std::bit_cast<float>(initial[1]));
    SquidPostPopList additional;ContainedFinderState finder;PlayerPlacementAnchorState anchorState;anchorState.graphReady=1;
    RawPlayerPlacementAnchor anchor(core.pool,core.types,core.hash.Entries(0),core.bookkeeping,additional,finder,anchorState);
    const auto graph=anchor.GraphFor(priest.value);CellPoint goal{};bool found=false;
    // 같은 실제 Graph의 자유 칸을 명시 클릭 입력으로 선택하며 경로 결과를 합성하지 않는다.
    for (int radius=4;radius<10 && !found;++radius) for (int dy=-radius;dy<=radius && !found;++dy) for (int dx=-radius;dx<=radius;++dx) {
        const int px=x+dx,py=y+dy;if (px<2 || py<2 || px>=254 || py>=254) continue;
        if ((core.spots[py*256+px]&7)==2 && anchor.GraphAt(static_cast<float>(px),static_cast<float>(py))==graph) { goal={px,py};found=true;break; }
    }
    CHECK(found);world.Select(priest.value);CHECK(world.MoveSelected(world.ToScreen(goal.x,goal.y)));
    bool path=false,moved=false;
    // 실제 현재 Kernel을 실행하고 화면 뷰도 같은 슬롯의 현재 위치를 따라가는지 확인한다.
    for (std::uint32_t frame=0;frame<480;++frame) {
        world.Step(1.0/60.0);
        // 현재 실제 내 사제의 부모 Form 체인에서 살아 있는54를 조회한다.
        for (auto form=static_cast<std::uint16_t>(Get(core.pool.Slot(priest),6,2));form;form=static_cast<std::uint16_t>(Get(core.pool.Slot(Sid{form}),4,2)))
            if (core.pool.Slot(Sid{form})[10]==54) path=true;
        const auto raw=core.pool.Slot(priest);if (Get(raw,14)!=initial[0] || Get(raw,18)!=initial[1]) moved=true;
        CHECK(world.Object(priest.value)->x==std::bit_cast<float>(Get(raw,14)) && world.Object(priest.value)->y==std::bit_cast<float>(Get(raw,18)));
    }
    CHECK(path && moved && core.bookkeeping.depth==0);
}

// 실제 GameWorld 선택/이동은 원본 경로를 사용한다.
TEST_CASE(CampaignSession_ActualWarBeginsClickMovesSamePriest) { CheckSessionMovement(false); }
// 단독 캠페인의 NULL 연결 표는 실제 Gun/Path 직렬화 뒤 원본 송신 분배에서 자연 반환한다.
TEST_CASE(CampaignSession_ActualSingleplayerBroadcastAndPath) { CheckSessionMovement(true); }

// 실제 생산 세션의 보호 TYPE 원문과 현재 재시드 상태에서 교전/HP1/감시 종료를 검사한다.
TEST_CASE(CampaignSession_ActualWarBeginsCombatToOne) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SessionServicesFixture external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    auto services=external.Services(); // UI/장치는 명시 외부 입력이며 Form/삭제는 실제 writer를 사용한다.
    services.dependencyTransmit={};services.deletion={};
    services.formPacket=[&external](std::span<const std::uint8_t> bytes) { external.packets.emplace_back(bytes.begin(),bytes.end()); };
    GameWorld world(assets.game,assets.files,std::move(fort),MissionPlayers{},"thewarbegins",std::move(services),{1024,768});
    auto core=world.Surfaces().Core();Sid priest{};
    // 실제 설치된 내 사제를 선택하며 피해용 가짜 객체를 만들지 않는다.
    for (const auto& object:world.Surfaces().Objects()) if (object.object.type==158 && object.object.owner==1) { priest=object.sid;break; }
    CHECK(priest.value!=0);const auto before=world.Object(priest.value)->hitPoints;
    world.Surfaces().Unpop(priest,0);world.Surfaces().Pop(priest,130,114,0);
    bool impact=false;std::uint32_t frame=0,firstDamage=0;
    // 명시 재배치 입력 뒤 같은 Kernel만 실행하여 실제 피해/충돌/화면 HP를 관찰한다.
    for (;frame<3600 && world.Object(priest.value)->hitPoints>1;++frame) {
        world.Step(1.0/60.0);
        if (!firstDamage && world.Object(priest.value)->hitPoints<before) firstDamage=frame+1;
        // 현재 살아 있는 충돌 자산을 직접 관찰한다.
        for (const auto& object:world.Surfaces().Objects()) if (object.object.type==160) impact=true;
    }
    CHECK(firstDamage>0 && impact && world.Object(priest.value)->hitPoints==1);
    const auto low=frame;
    // HP1 이후에도 보호막/감시/실제 소리 프로세스와 투사체 종료를 계속 실행한다.
    for (;frame<low+720;++frame) world.Step(1.0/60.0);
    CHECK(world.Object(priest.value)->hitPoints==1 && !external.playing.empty());
    // 현재 모든 총은 HP1 사제 감시를 해제해야 한다.
    for (std::uint32_t id=5;id<core.pool.Capacity();++id) {
        const Sid sid{static_cast<std::uint16_t>(id)};const auto raw=core.pool.Slot(sid);if ((raw[11]&7) || raw[10]!=34) continue;
        auto* gun=dynamic_cast<GunProcess*>(core.processes.FromForm(sid));CHECK(gun!=nullptr);
        CHECK(gun->Fields().Target()!=priest && !core.dependencies.Has(priest,sid));
    }
    // 투사체와 충돌 효과는 실제 공통 삭제를 완료한다.
    for (const auto& object:world.Surfaces().Objects()) CHECK(object.object.type!=83 && object.object.type!=160);
    CHECK(core.bookkeeping.depth==0 && core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
    bool dependencyPacket=false,deletionPacket=false; // 같은 세션의 실제 감시/삭제 writer 송신 결과다.
    // 실제 교전 동안 생성/해제한 Form과 투사체의 패킷을 관찰한다. 별도 합성 writer로 대신하지 않는다.
    for (const auto& packet:external.packets) {
        if (packet.size()==13 && packet[1]==0x65) { dependencyPacket=true;CHECK(packet[0]==1 && packet[2]==13 && packet[3]==0 && (packet[8]&4)==0); }
        if (packet.size()==8 && packet[1]==0x2e) { deletionPacket=true;CHECK(packet[0]==1 && packet[2]==8 && packet[3]==0); }
    }
    CHECK(dependencyPacket && deletionPacket);
    std::printf("생산 세션 실제 교전: 최초피해 frame%u, HP1 frame%u, 종료 frame%u\n",firstDamage,low,frame);
}
