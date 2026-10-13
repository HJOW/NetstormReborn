// 의식 사제 wrapper의 원본 현재타입/프레임/예약 순서와 실제 같은풀 소비자를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawPriestRitualActions.h"
#include "o/SquidFrame.h"
#include "o/SquidFactory.h"
#include "o/SquidProcess.h"
#include "o/SquidUnpop.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 앞 A 둘과 K..P 각1/2/3개의 실제 LetterRun 입력을 만든다.
std::vector<FrameCode> RitualCodes(std::uint32_t profile) {
    std::vector<FrameCode> result{{'A','P',1,0},{'A','P',2,0}};
    // stage마다 현재 자산의 같은 글자 구간을 구성한다. 기대 선택값을 계산하지 않는다.
    for (std::uint8_t stage=0;stage<6;++stage) {
        // 같은 글자의 실제 frame 수를 profile+1로 만든다.
        for (std::uint8_t index=0;index<=profile;++index) result.push_back({static_cast<std::uint8_t>('K'+stage),'P',static_cast<std::uint8_t>(index+1),0});
    }
    return result;
}
// 원본 효과 요청의 발생 순서만 합친다.
std::string RitualEffects(const std::vector<std::string>& effects) {
    std::string result;
    // native와 같은 구분자 순서를 유지한다.
    for (const auto& effect:effects) { if (!result.empty()) result+=';';result+=effect; }
    return result.empty() ? "-" : result;
}
}

// 유효/범위밖 SID, 현재 first-table, frame callback 뒤 현재값, 확보 실패/604 요청을 실제 PE와 대조한다.
TEST_CASE(rawPriestRitualActions_native_current_type_frame_and_604) {
    const auto fixture=LoadFixture(NETSTORM_PRIESTRITUALACTIONS_FIXTURE);std::size_t checked=0;
    // 모든 고유 입력마다 새 원본 폭의 풀/프레임 표를 구성한다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==4);const auto values=Split(row[1],',');CHECK(values.size()==8);const auto mode=Number(values[0]),sid=Number(values[1]),stage=Number(values[2]),profile=Number(values[3]),type=Number(values[4]),reserve=Number(values[5]),mutation=Number(values[6]),rawState=Number(values[7]);
        const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;SidPool pool(edition,24000,true);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);SquidFactory factory(pool,types);
        const Sid dataSid{static_cast<std::uint16_t>(sid>0 && sid<24000 ? sid : 5)};
        // SID5는 client 목록에서 생성하고, 서버 영역 SID23999만 실제 수신 Take로 확보한다.
        CHECK((dataSid==Sid{5} ? factory.Create(type,2) : factory.Take(type,dataSid))==dataSid);auto raw=pool.AllocatedBytes(dataSid);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(type);raw[11]=static_cast<std::uint8_t>(rawState);
        std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));frames[133]=RiftTypeFrames(RitualCodes(profile));frames[158]=RiftTypeFrames(RitualCodes(profile));std::vector<std::string> effects;PriestRitualActionHooks hooks;
        hooks.setFrame=[&](Sid current,std::int32_t index,std::uint32_t flags) {
            CHECK(current.value==sid && flags==0);effects.push_back("F:"+std::to_string(current.value)+','+std::to_string(static_cast<std::uint32_t>(index))+','+std::to_string(flags));Put(raw,edition==OriginalEdition::Patch1078 ? 36 : 34,static_cast<std::uint32_t>(index),edition==OriginalEdition::Patch1078 ? 4 : 1);
            if (mutation==1) raw[10]=type==158 ? 133 : 158;
        };
        hooks.reserveShared=[&] { effects.push_back("N:40");if (mutation==2) raw[11]=2;return reserve!=0; };
        hooks.addShared=[&](Sid current,std::uint32_t event,float payload) { CHECK(current.value==sid && event==604 && std::bit_cast<std::uint32_t>(payload)==0x3c23d70a);effects.push_back("S:"+std::to_string(event)+','+std::to_string(current.value)+','+std::to_string(std::bit_cast<std::uint32_t>(payload))); };
        RawPriestRitualActions actions(pool,frames,std::move(hooks));if (mode) actions.Continue(sid,stage);else actions.Begin(sid,stage);
        CHECK(RitualEffects(effects)==row[2]);const auto actual=Hex(pool.Slot(dataSid));CHECK(actual.size()==row[3].size() && actual.substr(8)==row[3].substr(8));++checked;
    }
    CHECK(checked>1000);
}

// 실제 LetterRun→SquidFrame의 raw 쓰기/표시요청→같은 Host/Kernel의604 예약을 연결한다.
TEST_CASE(rawPriestRitualActions_real_frame_and_shared_consumer) {
    // 두 판본에서 같은 의식 요청/중복 예약을 별도 새 월드로 수행한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,24000,true);SquidHash hash;std::vector<std::uint8_t> spots(65536);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);types[158].flags2=0x210000;types[158].constructorAddress=TypeConstructorAddress(edition,158);SquidFactory factory(pool,types);const auto parent=factory.Create(158,2);auto raw=pool.AllocatedBytes(parent);raw[11]=0;raw[edition==OriginalEdition::Patch1078 ? 33 : 31]=1;
        std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));frames[158]=RiftTypeFrames(RitualCodes(1));std::uint32_t updates=0,reserved=0;SquidFrameHooks frameHooks;
        frameHooks.frameSize=[&](Sid sid,std::int32_t index) { CHECK(sid==parent && index>=0 && static_cast<std::size_t>(index)<frames[158].Codes().size());return std::array<float,2>{1,1}; };
        frameHooks.update=[&](Sid sid,std::uint32_t flags) { CHECK(sid==parent && flags==0);++updates; };
        frameHooks.unpop=[](Sid,std::uint32_t) { throw std::logic_error("같은 hash단계 의식 프레임은 Unpop을 실행하지 않는다"); };
        frameHooks.pop=[](Sid,float,float,std::uint32_t) { throw std::logic_error("같은 hash단계 의식 프레임은 Pop을 실행하지 않는다"); };
        SquidFrame frame(pool,types,std::move(frameHooks));SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel(32);SquidProcessState clock;clock.now=99;
        SquidDestroyHooks asset;asset.selected=[] { return Sid{}; };asset.emit=[](const SquidDestroyEvent&) { throw std::logic_error("의식 프레임/예약 검사는 자산 삭제를 실행하지 않는다"); };
        SquidProcessHost host(pool,types,kernel,destroy,clock,{},asset);PriestRitualActionHooks hooks;hooks.reserveShared=[&] { ++reserved;return true; };
        RawPriestRitualActions actions(pool,frames,MakePriestRitualActionHooks(pool,frame,host,std::move(hooks)));actions.Begin(parent.value,2);CHECK(updates==2 && reserved==0 && kernel.Size()==0);
        actions.Continue(parent.value,4);CHECK(updates==4 && reserved==1 && kernel.Size()==1);auto* pending=host.FindEvent(parent,604);CHECK(pending && pending->Payload()==0.01F && pending->Count()==0 && pending->Time()==99);
        actions.Continue(parent.value,4);CHECK(updates==6 && reserved==2 && kernel.Size()==2 && host.FindEvent(parent,604)!=pending);CHECK(&actions.Pool()==&host.Pool());
        actions.Begin(0,0xffffffff);CHECK(updates==6 && reserved==2);CHECK(Throws([&] { actions.Begin(parent.value,6); }));CHECK(Throws([&] { RawPriestRitualActions missing(pool,frames,{}); }));kernel.Clear();
    }
}
