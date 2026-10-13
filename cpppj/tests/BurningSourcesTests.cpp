// 실제 원본 source 선택과 현재HP/난수/입자/행표식을 같은 입력으로 대조한다.
#include "ConstructionSupport.h"
#include "o/RawBurningSources.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::construction;
namespace {
// 독립 원본 출력은 fixture의 마지막 여섯 칸에서만 읽는다.
const auto& SourceRows() { static const auto rows=LoadFixture(NETSTORM_BURNINGSOURCES_FIXTURE).rows;return rows; }
// 같은24바이트 입자의 source this+74/null만0/1로 정규화한다.
std::string SourceParticleBytes(const BurningVisualFields& fields) {
    std::vector<std::uint8_t> bytes(fields.capacity*24);
    // 호스트 bool/padding을 읽지 않고 여섯DWORD를 직렬화한다.
    for (std::size_t index=0;index<fields.particles.size();++index) { const auto& value=fields.particles[index];auto raw=std::span<std::uint8_t>(bytes).subspan(index*24,24);Put(raw,0,Bits(value.x));Put(raw,4,Bits(value.y));Put(raw,8,Bits(value.velocityX));Put(raw,12,Bits(value.velocityY));Put(raw,16,Bits(value.life));Put(raw,20,value.source ? 1U : 0U); }
    return bytes.empty() ? "-" : Hex(bytes);
}
// 실제source/Append만 소비하며 생성/부착/첫팔레트 호출은 명시 거부한다.
void ReplaySources(std::string_view edition) {
    const auto original=edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;const bool patch=original==OriginalEdition::Patch1078;SidPool pool(original,kCapacity,true);std::vector<RiftTypeRecord> types(patch ? 188 : 171);GameRandom random;BurningSourcesState sourceState;
    // 현재피해50까지 실제 client SID로 입력번호를 확보한다.
    for (std::uint16_t id=5;id<=50;++id) CHECK(pool.Allocate(2)==Sid{id});
    RenderRegistryHooks registryHooks;registryHooks.reserve=[](std::uint32_t)->bool { throw std::logic_error("source선택의미허용표시확보"); };registryHooks.releaseArray=[] { throw std::logic_error("source선택의미허용표시반납"); };registryHooks.attach=[](RenderProcessFields&,std::uint32_t,Sid,std::uint32_t) { throw std::logic_error("source선택의미허용부착"); };RawRenderRegistry registry(pool,std::move(registryHooks));BurningVisualState state;BurningVisualHooks hooks;hooks.reserve=[](std::uint32_t)->bool { throw std::logic_error("source선택의미허용추가확보"); };RawBurningVisual visual(pool,registry,state,std::move(hooks));RawBurningSources sources(pool,types,visual,random,sourceState);std::size_t count=0;int failures=0;
    // 현재 raw HP/타입·캡처·기존입자만 입력에서 만들며 목표 행을 계산하지 않는다.
    for (const auto& row:SourceRows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==19);const auto kind=Number(row[5]);const auto type=kind==2 ? 158U : kind==1 ? 70U : 89U;types[type].flags1=16;types[type].flags2=kind==1 ? 0x400U : 0U;types[type].maxHitPoints=static_cast<std::int32_t>(Number(row[4]));auto raw=pool.AllocatedBytes(Sid{50});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(type);Put(raw,26,Number(row[3]),patch ? 4U : 2U);random.SetState(Number(row[8]));sourceState.weakenedMana=Number(row[11])!=0;
        BurningVisualFields fields;fields.victim=Sid{static_cast<std::uint16_t>(Number(row[12]))};fields.capacity=Number(row[9]);fields.particles.resize(fields.capacity);fields.image.width=static_cast<std::int32_t>(Number(row[1]));fields.image.height=static_cast<std::int32_t>(Number(row[2]));fields.image.pixels.assign(static_cast<std::size_t>(fields.image.width)*fields.image.height,Number(row[7]) ? std::uint8_t{255} : std::uint8_t{});fields.rows.resize(static_cast<std::size_t>(fields.image.width+fields.image.height)/5);fields.selected=static_cast<std::int32_t>(Number(row[6]));const auto pattern=Number(row[10]);fields.total=pattern==2 ? fields.capacity : pattern==1 ? std::min(3U,fields.capacity) : 0U;
        const std::array<std::array<float,2>,3> points=fields.image.width>=8 && fields.image.height>=8 ? std::array<std::array<float,2>,3>{{{2,3},{7,3},{0,0}}} : std::array<std::array<float,2>,3>{};
        // 기존 source 좌표와 rowTaken 표식은 같은 명시 입력이다.
        for (std::size_t index=0;index<fields.particles.size();++index) { const bool source=pattern==1 && index<3;const auto x=source ? points[index][0] : 10.0f+static_cast<float>(index),y=source ? points[index][1] : 20.0f+static_cast<float>(index);fields.particles[index]={x,y,0,0,source ? 9999.0f : pattern==2 ? 1.25f : 0.0f,source};if (source) { const auto group=(static_cast<std::int32_t>(x)+static_cast<std::int32_t>(y))/5;if (static_cast<std::size_t>(group)<fields.rows.size()) fields.rows[group]=1; } }
        sources.Select(fields);const std::vector<std::string> output{"return",std::to_string(fields.selected),std::to_string(fields.total),fields.rows.empty() ? "-" : Hex(fields.rows),SourceParticleBytes(fields),std::to_string(random.State())};const auto expected=std::vector<std::string>(row.begin()+13,row.end());const bool same=output==expected;CHECK(same);
        if (!same) {
            // 첫 불일치의 원본/C++ 필드를 각각 보여준다.
            for (std::size_t index=0;index<output.size();++index) if (output[index]!=expected[index]) std::printf("%s BurningSources 행%zu 결과%zu 기대%s / 실제%s\n",row[0].c_str(),count,index,expected[index].c_str(),output[index].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==SourceRows().size()/2);
}
}
// 패치10.78의 현재HP/고정source/300시도/폴백/제거와 정수난수를 대조한다.
TEST_CASE(BurningSources_PatchNativeSelectionAndRemoval) { ReplaySources("originals"); }
// CD10.72의 signed HP WORD와 별도HasHP/위크마나/Append body를 대조한다.
TEST_CASE(BurningSources_CdNativeSelectionAndRemoval) { ReplaySources("originalCD"); }
