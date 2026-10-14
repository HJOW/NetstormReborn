// 새 독립 PE의 전체 입력/출력으로 현재 Gun 방향과 Factory 예약/상태 몸체를 비교한다.
#include "RawSceneSupport.h"
#include "o/RawAssetRegular.h"
#include "o/RawFactoryPostPop.h"
#include "o/RawGunMode.h"
#include "o/RawSquidLevel.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 원본 입력과 같은 A..F의8프레임 코드 구간을 공급한다.
RiftTypeFrames CampaignNativeCodes() {
    std::vector<FrameCode> codes;
    // 결과 프레임은 복원 모듈이 결정하며 입력 표만 만든다.
    for (unsigned i=0;i<48;++i) codes.push_back({static_cast<std::uint8_t>('A'+i/8),'A',static_cast<std::uint8_t>(i),0});
    return RiftTypeFrames(std::move(codes));
}
// 표시 외부 경계의 요청 순서를 원본과 같은 문자열로 기록한다.
std::string CampaignNativeEvents(const std::vector<std::string>& events) {
    std::string result;
    // 요청을 정렬하지 않고 호출 순서 그대로 잇는다.
    for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; }
    return result.empty() ? "-" : result;
}
// 한 판본의 실제 입력 raw와 FrameTable을 같은 풀의 생산 모듈에 전달한다.
void CheckCampaignNativeFrame(OriginalEdition edition) {
    const bool patch=edition==OriginalEdition::Patch1078;
    SidPool pool(edition,kCapacity,false);SquidHash hash;
    // native client 번호50까지 동일 실제 풀을 확보한다.
    for (std::uint16_t i=5;i<=50;++i) CHECK(pool.Allocate(2)==Sid{i});
    std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<RiftTypeFrames> frames(types.size(),CampaignNativeCodes());
    // 실제 setter의 공간 분류가 native 입력과 같은1칸 발자국을 읽는다.
    for (auto& type:types) { type.footX=1;type.footY=1; }
    std::vector<std::int32_t> elements(types.size());std::vector<std::string> events;
    const auto update=[&](Sid sid,std::uint32_t flags) {
        const auto raw=pool.Slot(sid);events.push_back("U:"+std::to_string(sid.value)+':'+std::to_string(flags)+':'+std::to_string(Get(raw,patch ? 36U : 34U,patch ? 4U : 1U))+':'+std::to_string(raw[11]));
    }; // 실제 표시 요청의 frame/state를 당시 raw에서 관찰한다.
    SquidFrame setter(pool,types,{[](Sid,std::int32_t) { return std::array<float,2>{1,1}; },update,
        [](Sid,std::uint32_t) { throw std::logic_error("native 입력의 공간단계 변경 예상 밖"); },
        [](Sid,float,float,std::uint32_t) { throw std::logic_error("native 입력의 재Pop 예상 밖"); }});
    RawFrameAdvance advance(pool,frames,setter);GameRandom random;AssetPostPopState assets;AssetRegularState state;
    RawAssetRegular regular(pool,hash,types,frames,elements,setter,advance,random,assets,state,{});
    std::uint32_t windArcher=140;RawGunMode mode(pool,frames,windArcher);
    SquidPostPopState baseState;SquidPostPop base(pool,types,baseState);CarrierPostPopState carrierState;
    RawCarrierPostPop damageable(pool,types,carrierState,{});
    FactoryPostPopHooks factoryHooks;factoryHooks.updateState=[&](Sid sid) { update(sid,0); };
    RawFactoryPostPop factory(pool,hash,types,frames,base,damageable,assets,std::move(factoryHooks));
    RawSquidLevel level(pool,types,[](SquidLevelAssertion) { throw std::logic_error("새 native 정상Level의 예상 밖 진단"); });
    const auto rows=LoadFixture(NETSTORM_CAMPAIGN_FRAME_FIXTURE).rows;std::size_t count=0;
    // 독립 원본의 각 기대값을 재계산하지 않고 반환/raw/난수/요청 전체를 비교한다.
    for (const auto& row:rows) {
        if (row[0]!=(patch ? "originals" : "originalCD")) continue;CHECK(row.size()==8);events.clear();hash.Reset();
        std::vector<std::uint32_t> input;
        // fixture에 저장된 입력 인자만 파싱한다.
        for (const auto& text:Split(row[2],',')) input.push_back(Number(text));
        auto raw=pool.AllocatedBytes(kSource);
        // 원본 호출 직전 슬롯 전체를 같은 폭으로 공급한다.
        for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(std::stoul(row[3].substr(i*2,2),nullptr,16));
        random.SetState(0x12345678U);std::uint32_t result=0;
        if (row[1]=="M") {
            frames[input[0]]=RiftTypeFrames(std::vector<FrameCode>(48,FrameCode{static_cast<std::uint8_t>(input[3]),'A',0,0}));
            result=mode.Get(kSource);
        } else if (row[1]=="S") factory.SetState(level,kSource,input[2]);
        else result=std::bit_cast<std::uint32_t>(regular.Handle(kSource,input[2],input[3],0.5f));
        const bool same=result==Number(row[4]) && random.State()==Number(row[5]) && Hex(pool.Slot(kSource))==row[6] && CampaignNativeEvents(events)==row[7];
        if (!same) std::printf("CampaignNative 불일치 %s %s: 요청 %s / %s\n",row[1].c_str(),row[2].c_str(),CampaignNativeEvents(events).c_str(),row[7].c_str());
        CHECK(same);++count;
    }
    CHECK(count==378);
}
}
// 10.78 원본378고유입력을 실제 생산 모듈로 비교한다.
TEST_CASE(CampaignNativeFrame_PatchActualPE) { CheckCampaignNativeFrame(OriginalEdition::Patch1078); }
// 10.72의 BYTE frame/extra 배치와 같은378고유입력을 비교한다.
TEST_CASE(CampaignNativeFrame_CdActualPE) { CheckCampaignNativeFrame(OriginalEdition::Cd1072); }
