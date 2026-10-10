// 건설 배치 실행·비용 부족 처리·환불/취소 통지·SP 조회의 세 PE 관찰 재생과 실제 수신/소유자/SP 모듈 연결을 검사한다.
#include "ConstructionSupport.h"
#include "o/RawConstructionPlace.h"

namespace {
using namespace netstorm::test::construction;
// 판본별 독립 관찰 행 수다(10.78은 비용 처리 직접 호출을 포함한다).
constexpr std::size_t kPatchRows=1778,kCdRows=952;
// 문구 조립 경계가 돌려주는 고정 토큰이다.
constexpr std::string_view kToken="cheat-token";
// 배치 전 void 슬롯 입력이다. 0=0으로 채움, 1=0xff로 채움, 2=위치마다 다른 값. 상태는 void를 켜고 free를 끈다.
void FillSlot(std::span<std::uint8_t> raw,std::uint32_t profile,std::uint32_t type) {
    // 실행기의 slot_bytes와 같은 바이트를 만든다.
    for (std::size_t i=0;i<raw.size();++i) raw[i]=profile==0 ? std::uint8_t{0} : profile==1 ? std::uint8_t{0xff} : static_cast<std::uint8_t>(i*37+11);
    Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(type);raw[11]=static_cast<std::uint8_t>((raw[11]|4)&0xfe);
}
// 조각 순서의 SID 입력이다. 0=클라이언트 영역, 1=서버 영역, 2=번갈아 섞음.
std::uint16_t SidOf(const SidPool& pool,std::uint32_t profile,std::size_t index) {
    const bool local=profile==0 || (profile==2 && index%2==0);
    return static_cast<std::uint16_t>((local ? 50U : pool.Layout().serverFirst)+index);
}
// Player 표시 필드 입력이다. 홀수 번호는 0x400 밖의 비트를 모두 켠다.
std::uint32_t PlayerFlags(std::uint32_t player,bool flagged) { return (player%2 ? 0xfffffbffU : 0U)|(flagged ? 0x400U : 0U); }
// Python/원본 파일 없이 기대 출력만 한 번 읽는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_CONSTRUCTIONPLACE_FIXTURE).rows;return rows; }
// 타입·프레임·전역·경계를 행마다 다시 준비하는 재생 장면이다.
struct PlaceScene {
    SidPool pool;
    std::vector<RiftTypeRecord> types;
    std::vector<PriestPlainCanonType> frames;
    ConstructionPlaceState state;
    SquidPostPopState books;
    SquidRewardState rewardState;
    GameRandom rng;
    ScrambledSpStore store;
    bool patch;
    std::string events;
    std::uint32_t money{}; // SP 저장소 읽기 경계가 돌려줄 잔액 비트 입력이다.
    // 풀 객체의 주소는 행 사이에 바뀌지 않는다. SP 저장소는 보상 모듈 생성 조건만 채우며 재생에서는 읽지 않는다.
    explicit PlaceScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        frames(types.size()),store(rng,[] { return 1U; }),patch(edition==OriginalEdition::Patch1078) {}
    // 한 타입의 레코드와 프레임 표, altar의 HP, 전역 입력을 실행기의 setup과 같게 준비한다.
    void Prepare(std::uint32_t type,std::uint32_t cost,std::uint32_t flags1,std::uint32_t flags2,std::uint32_t maxHp,std::uint32_t altarHp,
        std::uint32_t defaultFrame,std::uint32_t frameProfile,std::uint32_t local,bool blocked,bool server,bool authority,bool shown,bool flagged,std::uint32_t life) {
        types.assign(types.size(),RiftTypeRecord{});frames.assign(frames.size(),PriestPlainCanonType{});events.clear();
        auto& record=types.at(type);record.cost=Float(cost);record.flags1=flags1;record.flags2=flags2;record.maxHitPoints=std::bit_cast<std::int32_t>(maxHp);
        types.at(kTypeAltar).maxHitPoints=std::bit_cast<std::int32_t>(altarHp);
        frames.at(type)={InputFrames(frameProfile),std::bit_cast<int>(defaultFrame)};
        state=ConstructionPlaceState{};state.localPlayer=local;state.chargeBlocked=blocked;state.server=server;state.authority=authority;
        state.battleShown=shown;state.bridgeLife=std::bit_cast<std::int32_t>(life);
        // Player 이름과 표시 비트를 번호마다 넣는다.
        for (std::uint32_t player=0;player<=kPlayerCount;++player) {
            state.playerNames[player]="P"+std::to_string(player);state.playerFlags[player]=PlayerFlags(player,flagged);
        }
    }
    // 사건 문자열에 한 항목을 잇는다.
    void Append(const std::string& value) { if (!events.empty()) events+=';';events+=value; }
    // 실행기의 경계와 같은 일을 한다: 인자를 기록하고, 소유자 지정은 소유자 바이트만 쓰며, 수신은 같은 번호를 돌려준다.
    ConstructionPlaceHooks Hooks() {
        ConstructionPlaceHooks hooks;
        hooks.take=[this](std::uint32_t type,Sid sid) { Append("T:"+std::to_string(type)+':'+std::to_string(sid.value));return sid; };
        hooks.notifySurface=[this](Sid sid) { Append("N:"+std::to_string(sid.value)); };
        hooks.setOwner=[this](Sid sid,std::uint32_t player) {
            InputRaw(pool,sid)[patch ? 34 : 32]=static_cast<std::uint8_t>(player);Append("O:"+std::to_string(sid.value)+':'+std::to_string(player));
        };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) {
            Append("P:"+std::to_string(sid.value)+':'+std::to_string(Bits(x))+':'+std::to_string(Bits(y))+':'+std::to_string(flags));
        };
        hooks.sendMoney=[this](std::uint32_t target,const ConstructionMoneyNotice& notice) {
            std::array<std::uint8_t,7> body{notice.player,notice.source,notice.kind};Put(body,3,std::bit_cast<std::uint32_t>(notice.amount));
            Append("M:"+std::to_string(target)+":money:"+Hex(body));
        };
        if (!patch) return hooks;
        hooks.storedSp=[this](std::uint32_t owner) { Append("G:"+std::to_string(owner));return Float(money); };
        hooks.addLocalSp=[this](float delta) { Append("L:"+std::to_string(Bits(delta))); };
        hooks.addPlayerSp=[this](std::uint32_t owner,float delta) { Append("A:"+std::to_string(owner)+':'+std::to_string(Bits(delta))); };
        hooks.format=[this](std::string_view key,std::string_view name) { Append("F:"+std::string(key)+':'+std::string(name));return std::string(kToken); };
        hooks.tellOthers=[this](std::uint32_t player,std::string_view text) { Append("X:"+std::to_string(player)+':'+std::string(text)); };
        hooks.tellPlayer=[this](std::uint32_t player,std::string_view text) { Append("Y:"+std::to_string(player)+':'+std::string(text)); };
        hooks.tell=[this](std::string_view text) { Append("D:"+std::string(text)); };
        hooks.sendReject=[this](std::uint32_t target,const ConstructionRejectNotice& notice) {
            std::array<std::uint8_t,11> body{};Put(body,0,Bits(notice.x));Put(body,4,Bits(notice.y));
            body[8]=notice.type;body[9]=notice.argument;body[10]=notice.direction;
            Append("M:"+std::to_string(target)+":reject:"+Hex(body));
        };
        return hooks;
    }
};
// 한 판본의 모든 관찰 행을 재생한다. 사건 순서·조각 슬롯 전체·부가 결과를 원본 출력과 그대로 비교한다.
void Replay(std::string_view edition) {
    PlaceScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    CHECK(Fixture().size()==kPatchRows+2*kCdRows);std::size_t count=0;
    // 연산 종류에 따라 입력 칸을 풀어 같은 호출을 한다.
    for (const auto& row:Fixture()) {
        if (row[1]!=edition) continue;CHECK(row.size()==6);const auto& op=row[0];const auto v=Inputs(row[2]);
        std::string slots="-",extra="-";
        if (op=="Place") {
            CHECK(v.size()==26);
            scene.Prepare(v[0],v[8],v[19],v[20],v[21],v[22],v[23],v[18],v[6],v[7]!=0,v[10]!=0,v[11]!=0,v[12]!=0,v[13]!=0,v[24]);scene.money=v[9];
            std::vector<std::uint16_t> sids;
            // 실제 decoder가 센 조각 수만큼 void 슬롯 입력을 준비한다.
            for (std::size_t i=0;i<v[25];++i) {
                sids.push_back(SidOf(scene.pool,v[16],i));FillSlot(InputRaw(scene.pool,Sid{sids.back()}),v[17],v[0]);
            }
            SquidReward reward(scene.pool,scene.types,scene.books,scene.rewardState,scene.patch ? &scene.store : nullptr);
            RawConstructionPlace place(scene.pool,scene.types,scene.frames,scene.state,reward,scene.Hooks());
            place.Place({v[0],Float(v[3]),Float(v[4]),v[1],v[2],v[5],sids,v[14],v[15]});
            // 조각 순서대로 슬롯 전체를 16진수로 잇는다.
            for (std::size_t i=0;i<sids.size();++i) slots=(i ? slots+',' : std::string{})+Hex(scene.pool.Slot(Sid{sids[i]}));
            const auto player=std::bit_cast<std::int32_t>(v[5]);
            if (scene.patch && player>0 && player<9) extra=std::to_string(scene.state.playerFlags[v[5]]);
        } else {
            // 직접 호출은 한 칸 타입의 비용과 전역만 쓴다. 나머지 입력은 실행기의 기본값과 같다.
            const bool typed=op=="Charge" || op=="Refund" || op=="Reject";
            const std::uint32_t type=typed ? v[0] : kTypePlain,local=op=="Money" || op=="Add" ? v[1] : v[2];
            const std::uint32_t cost=op=="Charge" ? v[6] : typed ? v[4] : 0;
            const bool authority=op=="Charge" && v[3]!=0,flagged=op=="Charge" && v[4]!=0;
            const bool shown=op=="Charge" ? v[5]!=0 : op=="Money" ? v[2]!=0 : typed && v[3]!=0;
            scene.Prepare(type,cost,0,0,0,0,0,0,local,false,true,authority,shown,flagged,5);
            SquidReward reward(scene.pool,scene.types,scene.books,scene.rewardState,scene.patch ? &scene.store : nullptr);
            RawConstructionPlace place(scene.pool,scene.types,scene.frames,scene.state,reward,scene.Hooks());
            if (op=="Charge") { place.ChargeShortfall(v[0],v[1],Float(v[7]),Float(v[8]),v[9],v[10]);extra=std::to_string(scene.state.playerFlags[v[1]]); }
            else if (op=="Refund") place.Refund(v[0],v[1]);
            else if (op=="Reject") place.Reject(v[0],v[1],Float(v[5]),Float(v[6]),v[7],v[8]);
            else if (op=="Money") { scene.money=v[3];extra=std::to_string(Bits(place.Money(v[0]))); }
            else { CHECK(op=="Add");place.AddMoney(v[0],Float(v[2])); }
        }
        const auto events=scene.events.empty() ? std::string("-") : scene.events;
        const bool same=events==row[3] && slots==row[4] && extra==row[5];
        CHECK(same);if (!same) { std::printf("건설 배치 %s %s 행 %zu 불일치\n  기대 %s | %s\n  실제 %s | %s\n",std::string(edition).c_str(),op.c_str(),count,row[3].c_str(),row[5].c_str(),events.c_str(),extra.c_str());break; }
        ++count;
    }
    CHECK(count==(edition=="originals" ? kPatchRows : kCdRows));
}
// 실제 factory/소유자/보상/SP 저장소를 연결한 통합 장면이다. Pop·표면 알림·문구·메시지는 기록 경계다.
struct LiveScene {
    SidPool pool;
    std::vector<RiftTypeRecord> types;
    std::vector<PriestPlainCanonType> frames;
    ConstructionPlaceState state;
    SquidPostPopState books;
    SquidOwnerMode mode;
    SquidRewardState rewardState;
    GameRandom rng;
    ScrambledSpStore store;
    bool patch;
    std::vector<std::string> log;
    // 한 칸 타입 하나(비용 250, 최대 HP 120, 기본 프레임 3)를 등록한다. 서버 풀이므로 SID를 직접 쓴다.
    explicit LiveScene(OriginalEdition edition):pool(edition,kCapacity,true),types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        frames(types.size()),store(rng,[] { return 1U; }),patch(edition==OriginalEdition::Patch1078) {
        types[kTypePlain].cost=250.0F;types[kTypePlain].maxHitPoints=120;types[kTypePlain].flags1=kHasHp;frames[kTypePlain]={InputFrames(0),3};
        state.localPlayer=1;
        // 안내 문구에 쓰일 이름이다.
        for (std::uint32_t player=0;player<=kPlayerCount;++player) state.playerNames[player]="P"+std::to_string(player);
    }
    // Pop과 통지는 호출 내용을 글자로 남긴다. 수신·소유자·SP는 어댑터가 실제 모듈로 바꾼다.
    ConstructionPlaceHooks Recording() {
        ConstructionPlaceHooks hooks;
        hooks.take=[](std::uint32_t,Sid sid) { return sid; };
        hooks.notifySurface=[this](Sid) { log.push_back("notify"); };
        hooks.setOwner=[](Sid,std::uint32_t) {};
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) {
            log.push_back("pop:"+std::to_string(sid.value)+':'+std::to_string(x)+':'+std::to_string(y)+':'+std::to_string(flags));
        };
        hooks.sendMoney=[this](std::uint32_t target,const ConstructionMoneyNotice& notice) {
            log.push_back("money:"+std::to_string(target)+':'+std::to_string(notice.amount));
        };
        hooks.storedSp=[](std::uint32_t) { return 0.0F; };
        hooks.addLocalSp=[](float) {};
        hooks.addPlayerSp=[](std::uint32_t,float) {};
        hooks.format=[](std::string_view key,std::string_view name) { return std::string(key)+'/'+std::string(name); };
        hooks.tellOthers=[this](std::uint32_t player,std::string_view text) { log.push_back("others:"+std::to_string(player)+':'+std::string(text)); };
        hooks.tellPlayer=[this](std::uint32_t player,std::string_view text) { log.push_back("player:"+std::to_string(player)+':'+std::string(text)); };
        hooks.tell=[this](std::string_view text) { log.push_back("tell:"+std::string(text)); };
        hooks.sendReject=[this](std::uint32_t target,const ConstructionRejectNotice& notice) {
            log.push_back("reject:"+std::to_string(target)+':'+std::to_string(notice.type));
        };
        return hooks;
    }
};
}

// 10.78의 배치 전체·비용 부족 처리·환불/취소 통지·SP 조회/가산을 원본 관찰과 대조한다.
TEST_CASE(ConstructionPlace_ReplaysOriginals) { Replay("originals"); }
// CD판의 배치(비용 판정 없음)와 환불 통지를 대조한다.
TEST_CASE(ConstructionPlace_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37 PE도 독립 원본 관찰에 대조한다.
TEST_CASE(ConstructionPlace_ReplaysExtra1037) { Replay("original1037"); }

// 실제 SID 생성·소유자 지정·SP 저장소와 연결해 확정 배치, abstract 배치, 잔액 부족을 차례로 검사한다.
TEST_CASE(ConstructionPlace_ChargesStoredSpAndPlacesRealObjects) {
    // 두 판본에서 같은 흐름을 돌린다. CD판에는 비용 판정이 없다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        LiveScene scene(edition);const bool patch=scene.patch;
        SquidFactory factory(scene.pool,scene.types);SquidOwner owner(scene.pool,scene.types,scene.books,scene.mode);
        SquidReward reward(scene.pool,scene.types,scene.books,scene.rewardState,patch ? &scene.store : nullptr);
        RawConstructionPlace place(scene.pool,scene.types,scene.frames,scene.state,reward,
            MakeConstructionPlaceHooks(scene.pool,factory,owner,reward,patch ? &scene.store : nullptr,scene.state,scene.Recording()));
        const std::size_t ownerOffset=patch ? 34 : 32,extraOffset=patch ? 40 : 35,frameOffset=patch ? 36 : 34;
        if (patch) { scene.store.Set(2,Bits(1000.0F));scene.store.Set(3,Bits(10.0F)); }
        // 확정 배치: 다른 플레이어의 잔액에서 비용을 빼고 최대 HP·기본 프레임·단어·소유자를 쓴 뒤 Pop 0x41을 요청한다.
        const Sid placed=factory.Create(kTypePlain,0);const std::array<std::uint16_t,1> first{placed.value};
        place.Place({kTypePlain,20.5F,21.25F,0x1234,0,2,first,0,0});
        auto raw=scene.pool.Slot(placed);
        CHECK(raw[ownerOffset]==2 && (raw[extraOffset]&1)==0 && Get(raw,26,patch ? 4 : 2)==120 && Get(raw,frameOffset,patch ? 4 : 1)==3 && Get(raw,12,2)==0x1234);
        CHECK(scene.log.size()==1 && scene.log[0]=="pop:"+std::to_string(placed.value)+':'+std::to_string(20.5F)+':'+std::to_string(21.25F)+":65");
        if (patch) CHECK(Float(scene.store.Get(2))==750.0F);
        // abstract 배치: 로컬 플레이어는 비용을 판정하지 않고 HP 0·abstract 비트·Pop 2로 놓는다.
        const Sid ghost=factory.Create(kTypePlain,2);const std::array<std::uint16_t,1> second{ghost.value};scene.log.clear();
        place.Place({kTypePlain,30.0F,31.0F,7,0,1,second,1,0});
        raw=scene.pool.Slot(ghost);
        CHECK(raw[ownerOffset]==1 && (raw[extraOffset]&1)==1 && Get(raw,26,patch ? 4 : 2)==0 && scene.log.size()==1 && scene.log[0].ends_with(":2"));
        // 잔액 부족: 권한 쪽은 안내·환불·취소 통지를 보내고 표시 비트를 켠다. 원본처럼 조각은 그대로 놓인다.
        const Sid third=factory.Create(kTypePlain,0);const std::array<std::uint16_t,1> short3{third.value};scene.log.clear();
        place.Place({kTypePlain,40.0F,41.0F,0,0,3,short3,0,0});
        CHECK(scene.pool.Slot(third)[ownerOffset]==3 && scene.log.back().starts_with("pop:"));
        if (patch) {
            CHECK(scene.log.size()==5 && scene.log[0]=="others:3:ClientCheated(%s)/P3" && scene.log[1]=="player:3:ServerThinksYouCheated");
            CHECK(scene.log[2]=="money:3:250" && scene.log[3]=="reject:3:100" && (scene.state.playerFlags[3]&0x400)!=0);
            CHECK(Float(scene.store.Get(3))==260.0F && Float(scene.store.Get(2))==750.0F);
            // 같은 플레이어의 두 번째 부족은 다른 플레이어에게 다시 알리지 않는다. 비권한 쪽은 강제 차감한다.
            scene.log.clear();place.ChargeShortfall(kTypePlain,3,40.0F,41.0F,0,0);
            CHECK(scene.log.size()==3 && scene.log[0]=="player:3:ServerThinksYouCheated");
            scene.state.authority=false;scene.state.playerFlags[4]=0;scene.store.Set(4,Bits(5.0F));scene.log.clear();
            place.ChargeShortfall(kTypePlain,4,40.0F,41.0F,0,0);
            CHECK(Float(scene.store.Get(4))==-245.0F && scene.log.size()==1 && scene.log[0]=="tell:ServerCheated(%s)/P4" && place.Money(4)==-245.0F);
        } else {
            CHECK(scene.log.size()==1 && Throws([&] { place.ChargeShortfall(kTypePlain,3,40.0F,41.0F,0,0); }) && Throws([&] { static_cast<void>(place.Money(3)); }));
        }
    }
}

// 다리 조각은 품질에 맞는 프레임과 수명 비트를 받고 표면 알림을 두 번 부른다. 서버가 아니면 서버 영역 SID를 수신한다.
TEST_CASE(ConstructionPlace_SetsBridgeQualityAndTakesServerSids) {
    PlaceScene scene(OriginalEdition::Patch1078);
    scene.Prepare(kTypeBridge,0,0,0,0,0,0,0,1,false,false,true,false,false,5);
    SquidReward reward(scene.pool,scene.types,scene.books,scene.rewardState,&scene.store);
    RawConstructionPlace place(scene.pool,scene.types,scene.frames,scene.state,reward,scene.Hooks());
    const auto first=static_cast<std::uint16_t>(scene.pool.Layout().serverFirst+3);const std::array<std::uint16_t,1> sids{first};
    FillSlot(InputRaw(scene.pool,Sid{first}),0,kTypeBridge);
    place.Place({kTypeBridge,20.5F,21.25F,0,0,1,sids,0,1});
    const auto raw=scene.pool.Slot(Sid{first});const auto code=scene.frames[kTypeBridge].frames.Codes()[Get(raw,36)];
    // 금 간 품질: 플래그 0x20 프레임, 수명 비트는 상수 5에서 1을 뺀 4다.
    CHECK(code.flags==0x20 && (Get(raw,12,2)&0x78)==(4U<<3) && scene.events.starts_with("T:82:"+std::to_string(first)+";N:"));
    CHECK(scene.events.find(";N:"+std::to_string(first)+";O:")!=std::string::npos && scene.events.ends_with(":65"));
}

// 누락 경계·다른 풀·개수 불일치·서버 번호·void 아님·없는 품질 프레임을 거부하고 개수 불일치에서는 아무 효과도 내지 않는다.
TEST_CASE(ConstructionPlace_RejectsBadInputsBeforeEffects) {
    PlaceScene scene(OriginalEdition::Patch1078),cd(OriginalEdition::Cd1072);
    scene.Prepare(kTypePlain,Bits(250.0F),kHasHp,0,120,0,3,0,1,false,true,true,false,false,5);
    SquidPostPopState books;SquidRewardState rewardState;SidPool other(OriginalEdition::Patch1078,kCapacity,false);
    SquidReward reward(scene.pool,scene.types,scene.books,scene.rewardState,&scene.store),foreign(other,scene.types,books,rewardState,&scene.store);
    auto missing=scene.Hooks();missing.pop={};
    CHECK(Throws([&] { RawConstructionPlace bad(scene.pool,scene.types,scene.frames,scene.state,reward,missing); }));
    missing=scene.Hooks();missing.sendReject={};
    CHECK(Throws([&] { RawConstructionPlace bad(scene.pool,scene.types,scene.frames,scene.state,reward,missing); }));
    CHECK(Throws([&] { RawConstructionPlace bad(scene.pool,scene.types,scene.frames,scene.state,foreign,scene.Hooks()); }));
    // CD판은 패치 전용 경계 없이도 만들 수 있다.
    cd.Prepare(kTypePlain,0,kHasHp,0,120,0,3,0,1,false,true,true,false,false,5);
    SquidReward cdReward(cd.pool,cd.types,cd.books,cd.rewardState,nullptr);
    RawConstructionPlace cdPlace(cd.pool,cd.types,cd.frames,cd.state,cdReward,cd.Hooks());
    RawConstructionPlace place(scene.pool,scene.types,scene.frames,scene.state,reward,scene.Hooks());
    const std::array<std::uint16_t,2> two{50,51};const std::array<std::uint16_t,1> one{50};
    FillSlot(InputRaw(scene.pool,Sid{50}),0,kTypePlain);FillSlot(InputRaw(scene.pool,Sid{51}),0,kTypePlain);
    // 한 칸 타입에 SID 둘, 또는 0개를 주면 비용 판정 전에 거부한다.
    CHECK(Throws([&] { place.Place({kTypePlain,20.5F,21.25F,0,0,2,two,0,0}); }) && scene.events.empty());
    CHECK(Throws([&] { place.Place({kTypePlain,20.5F,21.25F,0,0,2,{},0,0}); }) && scene.events.empty());
    CHECK(Throws([&] { place.Place({kTypePlain,20.5F,21.25F,0,0,9,one,0,0}); }) && scene.events.empty());
    CHECK(Throws([&] { place.Place({300,20.5F,21.25F,0,0,1,one,0,0}); }) && scene.events.empty());
    InputRaw(scene.pool,Sid{50})[11]=0;
    CHECK(Throws([&] { place.Place({kTypePlain,20.5F,21.25F,0,0,1,one,0,0}); }) && scene.events.empty());
    // 품질 프레임이 없는 표에서 금 간 품질을 요구하면 원본 assert 대신 예외다.
    scene.Prepare(kTypeBridge,0,0,0,0,0,0,3,1,false,true,true,false,false,5);FillSlot(InputRaw(scene.pool,Sid{50}),0,kTypeBridge);
    SquidReward bridgeReward(scene.pool,scene.types,scene.books,scene.rewardState,&scene.store);
    RawConstructionPlace bridge(scene.pool,scene.types,scene.frames,scene.state,bridgeReward,scene.Hooks());
    CHECK(Throws([&] { bridge.Place({kTypeBridge,20.5F,21.25F,0,0,1,one,0,1}); }));
    // 어댑터는 다른 풀의 모듈과 패치판의 빈 SP 저장소를 거부한다.
    SquidOwnerMode mode;SquidFactory factory(scene.pool,scene.types),foreignFactory(other,scene.types);
    SquidOwner owner(scene.pool,scene.types,scene.books,mode),foreignOwner(other,scene.types,books,mode);
    CHECK(Throws([&] { static_cast<void>(MakeConstructionPlaceHooks(scene.pool,foreignFactory,owner,reward,&scene.store,scene.state,scene.Hooks())); }));
    CHECK(Throws([&] { static_cast<void>(MakeConstructionPlaceHooks(scene.pool,factory,foreignOwner,reward,&scene.store,scene.state,scene.Hooks())); }));
    CHECK(Throws([&] { static_cast<void>(MakeConstructionPlaceHooks(scene.pool,factory,owner,foreign,&scene.store,scene.state,scene.Hooks())); }));
    CHECK(Throws([&] { static_cast<void>(MakeConstructionPlaceHooks(scene.pool,factory,owner,reward,nullptr,scene.state,scene.Hooks())); }));
}
