// 간헐천 부분 표시의 원본 출력과 실제 Pop/Unpop/프레임 연결만 검사한다. 게임/공격/의식/MP는 실행하지 않는다.
#include "RawSceneSupport.h"
#include "o/RawAssetPostPop.h"
#include "o/SquidDisplay.h"
#include "o/SquidFactory.h"
#include "o/SquidFrameBinding.h"
#include "o/SquidPostPop.h"
#include "o/SquidUnpop.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 같은 제품 표시기가 만든 사각형을 변형 없이 보관한다.
struct GeyserPartialSink final:SquidDisplaySink { // 단일 부분 표시와 old/new 갱신의 순서를 관찰하는 출력 경계다.
    bool suppressed{}; // 원본 표시 억제 입력이다.
    std::vector<std::pair<SquidDisplayRect,std::uint32_t>> entries; // 실제 요청 순서의 사각형/flags다.
    bool Suppressed() const override { return suppressed; } // 같은 현재 억제값을 호출 위치에서 조회한다.
    void Invalidate(SquidDisplayRect rect,std::uint32_t flags) override { entries.emplace_back(rect,flags); } // 원본 변경표와 대조할 실제 출력을 보존한다.
    std::string Packed() const { // 단언 기대값을 계산하지 않고 실제 출력만 Native 형식으로 직렬화한다.
        std::string text;
        for (const auto& [rect,flags]:entries) { // 원문 반환 순서대로 모든 모서리와 flags를 기록한다.
            if (!text.empty()) text+=';';
            text+=std::to_string(rect.left)+','+std::to_string(rect.top)+','+std::to_string(rect.right)+','+std::to_string(rect.bottom)+','+std::to_string(flags);
        }
        return text.empty() ? "-" : text;
    }
};
// 원본 SHP 입력의 signed 픽셀/핫스폿과 별도 해시 칸 크기를 그대로 만든다.
SquidDisplayShape GeyserInputShape() { return {4,true,{{100,180,-3,5,1,1},{130,60,7,-9,1,1},{50,260,2,3,1,1},{80,90,4,6,1,1}}}; }
}

TEST_CASE(GeyserPartialDisplay_Native432CurrentBaseFrameAndPixelInsets) { // 두 정밀도·현재 TYPE·무효 raw frame·WORD/extra·확대율/클리핑의 실제 원문 출력이다.
    std::vector<RiftTypeRecord> types(188);std::vector<SquidDisplayShape> shapes(188);
    shapes[74]=GeyserInputShape();shapes[75]=shapes[74];types[74].maxHitPoints=types[75].maxHitPoints=91;
    GeyserPartialSink sink;SquidDisplay display(OriginalEdition::Patch1078,types,shapes,sink,{7,9,65536,{12,9,620,450}});
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_GEYSER_PARTIAL_DISPLAY_FIXTURE).rows) { // fixture 입력과 독립 원본 기대값을 한 행씩 직접 대조한다.
        CHECK(row.size()==12);types[74].defaultFrame=std::stoi(row[1]);types[75].defaultFrame=(types[74].defaultFrame+1)%3;
        types[74].flags1=types[75].flags1=Number(row[6]);sink.suppressed=Number(row[8])!=0;sink.entries.clear();
        display.SetView({7,9,static_cast<int>(Number(row[4])),{12,9,620,450}});
        std::vector<std::uint8_t> raw(50);
        for (std::size_t index=0;index<raw.size();++index) raw[index]=static_cast<std::uint8_t>(std::stoul(row[9].substr(index*2,2),nullptr,16)); // 원본 실행 직전50바이트만 복사한다.
        const auto before=raw;display.Validate(OriginalEdition::Patch1078,raw,0x2000);CHECK(sink.entries.empty());
        display.Update(raw,0x2000);CHECK(raw==before && Number(row[10])==Number(row[8]) && sink.Packed()==row[11]);++count;
    }
    CHECK(count==432);
}

TEST_CASE(GeyserPartialDisplay_ActualPopUnpopAndFrameUseSameFixedOutline) { // 실제 공간/파생 후처리/프레임 몸체를 연결하고 이미 최초등록된 간헐천의 비멀티 재등록만 실행한다.
    std::vector<RiftTypeRecord> types(188);auto& type=types[122];type.constructorAddress=0x4857a0;type.flags2=0x10000;type.defaultFrame=0;
    std::vector<SquidDisplayShape> shapes(188);shapes[122]=GeyserInputShape();GeyserPartialSink sink;
    shapes[122].frames[2].cellWidth=shapes[122].frames[2].cellHeight=8; // 공간 재등록 검사의 별도 큰 칸 입력이며 Native 부분표시 입력과 분리한다.
    SidPool pool(OriginalEdition::Patch1078,32768,false);SquidHash hash;std::vector<std::uint8_t> spots(65536);
    SquidDisplay display(OriginalEdition::Patch1078,types,shapes,sink,{0,0,65536,{0,0,640,480}});
    SquidPostPopState state;SquidPostPop base(pool,types,state);CarrierPostPopState carrierState;AssetPostPopState assetState;
    RawCarrierPostPop damageable(pool,types,carrierState,MakeCarrierPostPopHooks(pool,base,{},{}));
    RawAssetPostPop asset(pool,types,base,damageable,assetState,{});base.SetAssetPostPop(&asset);
    SquidUnpop unpop(pool,hash,spots,&display);SquidPop pop(pool,hash,spots,&display,&base);SquidFactory factory(pool,types,false,&unpop);
    SquidFrame frame(pool,types,MakeSquidFrameHooks(pool,types,display,unpop,pop));const auto sid=factory.Create(122,2);
    auto raw=pool.AllocatedBytes(sid);raw[40]|=0x80;Put(raw,36,1); // 실제 기존 객체 입력이다. 이번 검사는 최초 생성/예약/AI 등록을 요청하지 않는다.
    CHECK(SquidUnpop::SupportsDisplay(OriginalEdition::Patch1078,0x50da88,0x2000));
    CHECK(pop.Pop(sid,type,1,1,20,20,0x2000)==RawPopResult::Registered);
    const std::string partial="323,265,424,346,0";
    CHECK(sink.Packed()==partial && state.depth==0 && hash.Bucket(1,20,20)==sid.value);
    sink.entries.clear();frame.Set(sid,2,kFrameUpdateAlternate);
    CHECK(sink.Packed()==partial+';'+partial && Get(raw,36)==2 && state.depth==0);
    sink.entries.clear();frame.Set(sid,1,kFrameUpdateAlternate); // 이전 큰 칸 크기 때문에 실제 Unpop→쓰기→Pop으로 이동한다.
    CHECK(sink.Packed()==partial+';'+partial && Get(raw,36)==1 && hash.Bucket(1,20,20)==sid.value && state.depth==0);
    sink.entries.clear();unpop.Unpop(sid,type,0x2000);
    CHECK(sink.Packed()==partial && (raw[11]&4U) && hash.Bucket(1,20,20)==0);
}

TEST_CASE(GeyserPartialDisplay_FullUpdateStillUsesCurrentFrameAndSelection) { // 일반88을 부분8c로 바꾸지 않으며 현재 표/표시 억제를 호출마다 읽는다.
    std::vector<RiftTypeRecord> types(188);std::vector<SquidDisplayShape> shapes(188);types[74].defaultFrame=0;types[74].maxHitPoints=91;shapes[74]=GeyserInputShape();
    GeyserPartialSink sink;SquidDisplay display(OriginalEdition::Patch1078,types,shapes,sink,{0,0,65536,{0,0,640,480}});
    std::array<std::uint8_t,50> raw{};Put(raw,0,0x50da88);raw[10]=74;raw[40]=4;Put(raw,14,std::bit_cast<std::uint32_t>(20.F));Put(raw,18,std::bit_cast<std::uint32_t>(20.F));Put(raw,36,1);
    display.Update(raw,0);CHECK(sink.Packed()=="310,214,447,290,16");
    sink.entries.clear();display.Update(raw,0x2000);CHECK(sink.Packed()=="323,265,424,346,0");
    sink.entries.clear();types[74].defaultFrame=1;display.Update(raw,0x2000);CHECK(sink.entries.empty());
    types[74].defaultFrame=0x7fffffff;sink.suppressed=true;display.Validate(OriginalEdition::Patch1078,raw,0x2000);display.Update(raw,0x2000);CHECK(sink.entries.empty());
}

TEST_CASE(GeyserPartialDisplay_TypeLoaderKeepsDefaultDistinctFromBaseFrameTag) { // 양판본 로더의 실제+128 연결과 빈 TYPE의 최초0을 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 서로 다른 판본 번호표를 같은 자산 로딩 경로로 구성한다.
        const auto order=TypeLoadOrder(edition);std::vector<RiftTypeDefinition> definitions;definitions.reserve(order.size());
        for (std::size_t index=0;index<order.size();++index) { // 특별 태그가 있는74와 기본0인 다른 타입을 별도로 입력한다.
            const auto clusters=index==4 ? "A00::\"a.gif\"#0; A01:default:\"b.gif\"#0; A02:baseframe:\"c.gif\"#0;" : "A00:default:\"a.gif\"#0;";
            definitions.push_back(RiftTypeDefinition::Parse("typename "+std::string(order[index])+" {} "+clusters));
        }
        std::vector<RiftTypeSource> sources;
        for (std::size_t index=0;index<order.size();++index) sources.push_back({order[index],&definitions[index]}); // 정의 배열을 완성한 뒤 수명이 안정된 주소를 전달한다.
        const RiftTypeTable table(edition,sources);
        CHECK(definitions[4].specialFrames.defaultFrame==1 && definitions[4].specialFrames.baseFrame==2);
        CHECK(table.Types()[74].defaultFrame==1 && table.Types()[75].defaultFrame==0 && table.Types()[0].defaultFrame==0);
    }
}
