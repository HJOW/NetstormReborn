// 신전 의식의 원본 전체 출력/조회와 기존 공통HP/실제프레임/공유예약 소비자를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawTempleRitual.h"
#include "o/RawSquidHitPoints.h"
#include "o/SquidFactory.h"
#include "o/SquidOwner.h"
#include "o/SquidFrame.h"
#include "o/SquidProcess.h"
#include "o/SquidUnpop.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 합성SHP와code/first-table 입력을 판본 공통 타입 인덱스로 준비한다. 출력 선택은 계산하지 않는다.
void TempleAssets(std::vector<RiftTypeRecord>& types,std::vector<RiftTypeFrames>& frames,std::vector<SquidDisplayShape>& shapes,char side) {
    // native와같은 세타입의 기본HP/핫스폿과 방향코드다.
    for (const auto type:{158U,165U,166U}) {
        const auto count=type==165 ? 180U : type==166 ? 6U : 32U;types[type].maxHitPoints=200;types[type].flags1=16;types[type].footX=types[type].footY=type==165 ? 3 : 1;shapes[type].loaded=true;shapes[type].frameCount=static_cast<int>(count);std::vector<FrameCode> codes;
        // SHP integer hotspot은입력이며목표점의계산은원본/복원몸체가각각한다.
        for (std::uint32_t index=0;index<count;++index) { shapes[type].frames.push_back({16,11,static_cast<std::int16_t>((index%5)*16),static_cast<std::int16_t>((index%5)*11),1,1,0,0,static_cast<std::int16_t>((index%5)*16),static_cast<std::int16_t>((index%5)*11)});codes.push_back({static_cast<std::uint8_t>(type==165 ? side : 'A'),'P',static_cast<std::uint8_t>(index+1),0}); }
        // 사제first-table은A둘/K..P각하나다. 이후P는해당첫프레임을바꾸지않는다.
        if (type==158) for (std::uint32_t index=2;index<count;++index) codes[index].side=static_cast<std::uint8_t>(index<8 ? 'K'+index-2 : 'P');
        frames[type]=RiftTypeFrames(std::move(codes));
    }
    types[158].flags2=0x210000;
}
// 효과 순서만 원본과 같은 구분자로 기록한다.
std::string TempleEffects(const std::vector<std::string>& effects) {
    std::string result;
    // 요청순서를그대로보존한다.
    for (const auto& effect:effects) { if (!result.empty()) result+=';';result+=effect; }
    return result.empty() ? "-" : result;
}
// 원본목표점출력의두단정도비트를묶는다.
std::string PointBits(TempleRitualPoint point) { return std::to_string(std::bit_cast<std::uint32_t>(point.x))+','+std::to_string(std::bit_cast<std::uint32_t>(point.y)); }
}

// 각고유입력의전체의식/현재단계/실제finder/현재raw와권한을원본PE출력만보고비교한다.
TEST_CASE(rawTempleRitual_native_stage_positions_queries_and_effects) {
    const auto fixture=LoadFixture(NETSTORM_TEMPLERITUAL_FIXTURE);std::size_t checked=0;
    // 입력마다같은상태저장소를새로만들어앞선행의예측결과/예약을남기지않는다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==7);const auto input=Split(row[1],',');CHECK(input.size()==9);const auto mode=Number(input[0]),currentFrame=Number(input[1]),profile=Number(input[3]),boss=Number(input[4]),reserve=Number(input[5]),spot=Number(input[6]),mutation=Number(input[8]);const auto xy=Split(row[2],',');CHECK(xy.size()==4);
        const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,24000,true);SquidHash hash;std::vector<std::uint8_t> spots(65536,static_cast<std::uint8_t>(spot));std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());TempleAssets(types,frames,shapes,input[2][0]);std::array<std::span<std::uint8_t>,9> slots;
        // 네raw슬롯의초기바이트는native와같고누락Rune도0이미지로유지한다.
        for (std::uint16_t sid=5;sid<=8;++sid) { CHECK(pool.Allocate(2)==Sid{sid});slots[sid]=pool.AllocatedBytes(Sid{sid});std::fill(slots[sid].begin(),slots[sid].end(),std::uint8_t{}); }
        const auto add=[&](Sid sid,std::uint32_t type,float x,float y,std::uint32_t extra,std::uint32_t rawState) { auto raw=slots[sid.value];Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(type);raw[11]=static_cast<std::uint8_t>(rawState);raw[patch ? 40 : 35]=static_cast<std::uint8_t>(extra);Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));auto& head=hash.Bucket(1,x,y);Put(raw,4,head,2);head=sid.value; };
        const float x=std::bit_cast<float>(Number(xy[0])),y=std::bit_cast<float>(Number(xy[1]));add(Sid{5},165,40.75F,41.25F,0,0);add(Sid{6},158,x,y,0,0);Put(slots[5],patch ? 36 : 34,currentFrame,patch ? 4 : 1);slots[5][patch ? 34 : 32]=1;slots[6][patch ? 34 : 32]=static_cast<std::uint8_t>(profile==2 ? 2 : 1);SetRawSquidHitPoints(pool,Sid{6},profile==1 ? 99 : 100);
        CHECK(xy.size()==4);
        if (profile>=3) add(Sid{7},166,std::bit_cast<float>(Number(xy[2])),std::bit_cast<float>(Number(xy[3])),profile==6 ? 8 : profile==5 ? 1 : 0,profile==4 ? 2 : 0);
        RawSquidNeighbors footprint(pool,hash,spots,types,frames);SquidPostPopState book;SquidRewardState rewardState;GameRandom random;ScrambledSpStore store(random,[] { return 1U; });SquidReward reward(pool,types,book,rewardState,patch ? &store : nullptr);TempleRitualState state;state.boss=boss!=0;std::vector<std::string> effects;
        PriestRitualActionHooks actionHooks;actionHooks.setFrame=[&](Sid sid,std::int32_t frame,std::uint32_t flags) { CHECK(sid==Sid{6} && flags==0);effects.push_back("F:6,"+std::to_string(frame)+",0");Put(slots[6],patch ? 36 : 34,static_cast<std::uint32_t>(frame),patch ? 4 : 1);if (mutation==1) state.boss=!state.boss;if (mutation==2) Put(slots[5],patch ? 36 : 34,14,patch ? 4 : 1); };
        const auto allocate=[&] { effects.push_back("N:40");if (mutation==3) state.boss=!state.boss;return reserve!=0; };
        const auto shared=[&](Sid sid,std::uint32_t event,float payload) { effects.push_back("S:"+std::to_string(event)+','+std::to_string(sid.value)+','+std::to_string(std::bit_cast<std::uint32_t>(payload))); };
        actionHooks.reserveShared=allocate;actionHooks.addShared=shared;RawPriestRitualActions actions(pool,frames,std::move(actionHooks));TempleRitualHooks hooks;hooks.reserveShared=allocate;hooks.addShared=shared;
        hooks.create=[&](std::uint32_t type,std::uint32_t flags) { CHECK(type==166 && flags==0);effects.push_back("C:166,0");auto raw=slots[8];std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,kVtable);raw[10]=166;raw[11]=4;return Sid{8}; };
        hooks.setHitPoints=[&](Sid sid,std::int32_t hp) { CHECK(sid==Sid{8} && hp==0);SetRawSquidHitPoints(pool,sid,hp); };
        hooks.setOwner=[&](Sid sid,std::uint32_t player) { CHECK(sid==Sid{8});effects.push_back("O:8,"+std::to_string(player));slots[8][patch ? 34 : 32]=static_cast<std::uint8_t>(player); };
        hooks.pop=[&](Sid sid,float px,float py,std::uint32_t flags) { CHECK(sid==Sid{8});effects.push_back("P:8,"+PointBits({px,py})+','+std::to_string(flags));Put(slots[8],14,std::bit_cast<std::uint32_t>(px));Put(slots[8],18,std::bit_cast<std::uint32_t>(py));slots[8][11]&=static_cast<std::uint8_t>(~4U); };
        hooks.playAt=[&](float px,float py,std::string_view sound,std::uint32_t flags) { effects.push_back("A:"+PointBits({px,py})+','+std::string(sound)+','+std::to_string(flags)); };
        RawTempleRitual ritual(pool,hash,types,frames,shapes,spots,footprint,reward,actions,state,std::move(hooks));const auto stage=ritual.Stage(Sid{5});const auto actual=std::to_string(ritual.Count(Sid{5}))+','+std::to_string(stage)+','+PointBits(ritual.StageAnchor(Sid{5},std::min(stage,4U)))+','+PointBits(ritual.StagePoint(Sid{5},std::min(stage,4U)))+','+std::to_string(ritual.RuneAt(Sid{5},{x,y}).value)+','+std::to_string(ritual.FindStrongOwnedPriest(Sid{5}).value)+','+std::to_string(ritual.HasAbstractRune(Sid{5}) ? 1 : 0);CHECK(actual==row[3]);
        ritual.Ritual(Sid{5},Sid{6},mode);CHECK(TempleEffects(effects)==row[4] && state.boss==(Number(row[6])!=0));const auto images=Split(row[5],';');CHECK(images.size()==4);
        // 가상표주입DWORD외raw전체를비교해HP폭/extra/프레임/옆stale바이트도확인한다.
        for (std::uint16_t sid=5;sid<=8;++sid) { const auto image=Hex(pool.Slot(Sid{sid}));CHECK(image.size()==images[sid-5].size() && image.substr(8)==images[sid-5].substr(8)); }
        ++checked;
    }
    CHECK(checked>1000);
}

// 실제Factory/Owner→같은Frame→실제0x307 Host예약의mode0사슬을현재신전/사제슬롯에서실행한다.
TEST_CASE(rawTempleRitual_real_frame_and_307_shared_consumer) {
    // 각판본의실제타입별생성자와공통HP폭을공유한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,24000,true);SquidHash hash;std::vector<std::uint8_t> spots(65536);std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());TempleAssets(types,frames,shapes,'A');
        // 165/158/166의실제생성자주소를입력으로공급하고base vtable로대체하지않는다.
        for (const auto type:{158U,165U,166U}) types[type].constructorAddress=TypeConstructorAddress(edition,type);
        SquidPostPopState book;SquidOwnerMode ownerMode;SquidOwner owner(pool,types,book,ownerMode);SquidFactory factory(pool,types);const auto temple=factory.Create(165,2),priest=factory.Create(158,2);owner.Set(temple,1);owner.Set(priest,1);SetRawSquidHitPoints(pool,priest,100);auto templeRaw=pool.AllocatedBytes(temple),priestRaw=pool.AllocatedBytes(priest);
        Put(templeRaw,14,std::bit_cast<std::uint32_t>(40.75F));Put(templeRaw,18,std::bit_cast<std::uint32_t>(41.25F));Put(templeRaw,patch ? 36 : 34,25,patch ? 4 : 1);templeRaw[11]=0;priestRaw[11]=0;priestRaw[patch ? 33 : 31]=1;
        RawSquidNeighbors footprint(pool,hash,spots,types,frames);SquidRewardState rewardState;GameRandom random;ScrambledSpStore store(random,[] { return 1U; });SquidReward reward(pool,types,book,rewardState,patch ? &store : nullptr);std::uint32_t updates=0,sounds=0,reservations=0;SquidFrameHooks frameHooks;
        frameHooks.frameSize=[&](Sid sid,std::int32_t frame) { CHECK(sid==priest && frame>=0 && static_cast<std::size_t>(frame)<frames[158].Codes().size());return std::array<float,2>{1,1}; };
        frameHooks.update=[&](Sid sid,std::uint32_t flags) { CHECK(sid==priest && flags==0);++updates; };
        frameHooks.unpop=[](Sid,std::uint32_t) { throw std::logic_error("같은단계의사제의식프레임은Unpop을요청하지않는다"); };
        frameHooks.pop=[](Sid,float,float,std::uint32_t) { throw std::logic_error("같은단계의사제의식프레임은Pop을요청하지않는다"); };
        SquidFrame frame(pool,types,std::move(frameHooks));SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel(32);SquidProcessState clock;clock.now=177;SquidDestroyHooks assets;assets.selected=[] { return Sid{}; };assets.emit=[](const SquidDestroyEvent&) { throw std::logic_error("신전의식mode0은자산삭제를요청하지않는다"); };SquidProcessHost host(pool,types,kernel,destroy,clock,{},assets);
        PriestRitualActionHooks actionHooks;actionHooks.reserveShared=[]()->bool { throw std::logic_error("신전의식mode0은사제604확보를요청하지않는다"); };RawPriestRitualActions actions(pool,frames,MakePriestRitualActionHooks(pool,frame,host,std::move(actionHooks)));TempleRitualState state;TempleRitualHooks hooks;
        hooks.reserveShared=[&] { ++reservations;return true; };
        hooks.pop=[](Sid,float,float,std::uint32_t) { throw std::logic_error("신전의식mode0은Rune생성/Pop을요청하지않는다"); };
        hooks.playAt=[&](float x,float y,std::string_view sound,std::uint32_t flags) { CHECK(x==42.75F && y==43.25F && sound=="forRain2.wav" && flags==0 && updates==2);++sounds; };
        RawTempleRitual ritual(pool,hash,types,frames,shapes,spots,footprint,reward,actions,state,MakeTempleRitualHooks(pool,factory,owner,host,std::move(hooks)));const auto point=ritual.StagePoint(temple,2);Put(priestRaw,14,std::bit_cast<std::uint32_t>(point.x));Put(priestRaw,18,std::bit_cast<std::uint32_t>(point.y));hash.Bucket(1,point.x,point.y)=priest.value;
        ritual.Ritual(temple,priest,0);CHECK(updates==2 && sounds==1 && reservations==1 && kernel.Size()==1);const auto* pending=host.FindEvent(temple,0x307);CHECK(pending && pending->Payload()==1.8799999952316284F && pending->Time()==177 && pending->Parent()==temple);CHECK(&ritual.Pool()==&host.Pool() && Get(priestRaw,patch ? 36 : 34,patch ? 4 : 1)==4);kernel.Clear();
    }
}
