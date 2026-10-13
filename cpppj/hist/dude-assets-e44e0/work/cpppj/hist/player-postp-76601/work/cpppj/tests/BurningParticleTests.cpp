// 실제 PE의 외부경계 없는24바이트 입자추가와 현재count/전체입자를 C++과 대조한다.
#include "ConstructionSupport.h"
#include "o/RawBurningVisual.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::construction;
namespace {
// 독립원본입력과출력은저장fixture에서만읽는다.
const auto& ParticleRows() { static const auto rows=LoadFixture(NETSTORM_BURNINGPARTICLE_FIXTURE).rows;return rows; }
// 현재입자6필드를호스트padding없이원본24바이트로직렬화한다.
std::string ParticleBytes(const BurningVisualFields& fields) {
    std::vector<std::uint8_t> bytes(fields.capacity*24);
    // 기존 source는원본this+74/null의허용표식만0/1로정규화한다.
    for (std::size_t index=0;index<fields.particles.size();++index) { const auto& particle=fields.particles[index];auto raw=std::span<std::uint8_t>(bytes).subspan(index*24,24);Put(raw,0,Bits(particle.x));Put(raw,4,Bits(particle.y));Put(raw,8,Bits(particle.velocityX));Put(raw,12,Bits(particle.velocityY));Put(raw,16,Bits(particle.life));Put(raw,20,particle.source ? 1U : 0U); }
    return bytes.empty() ? "-" : Hex(bytes);
}
// 실제추가몸체만실행하며사용하지않는생성/부착/팔레트호출은명시거부한다.
void ReplayParticle(std::string_view edition) {
    const auto original=edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;SidPool pool(original,kCapacity,true);
    RenderRegistryHooks registryHooks;registryHooks.reserve=[](std::uint32_t)->bool { throw std::logic_error("입자추가의미허용표시확보"); };registryHooks.releaseArray=[] { throw std::logic_error("입자추가의미허용표시반납"); };registryHooks.attach=[](RenderProcessFields&,std::uint32_t,Sid,std::uint32_t) { throw std::logic_error("입자추가의미허용부착"); };RawRenderRegistry registry(pool,std::move(registryHooks));BurningVisualState state;BurningVisualHooks hooks;hooks.reserve=[](std::uint32_t)->bool { throw std::logic_error("입자추가의미허용추가확보"); };RawBurningVisual visual(pool,registry,state,std::move(hooks));std::size_t count=0;int failures=0;
    // 입력숫자/old입자만공급하며 첫만료칸이나결과count를미리계산하지않는다.
    for (const auto& row:ParticleRows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==9);BurningVisualFields fields;fields.capacity=Number(row[1]);fields.total=Number(row[2]);fields.particles.resize(fields.capacity);const auto lifes=row[3]=="-" ? std::vector<std::string>{} : Split(row[3],',');CHECK(lifes.size()==fields.capacity);
        // 각입자의좌표/속도/수명과두허용source표식은native와같은입력이다.
        for (std::size_t index=0;index<fields.capacity;++index) fields.particles[index]={10.5f+static_cast<float>(index),20.0f+static_cast<float>(index),static_cast<float>(index)-1.0f,static_cast<float>(index)+1.0f,Float(Number(lifes[index])),(index&1)!=0};
        const bool made=visual.Append(fields,{100.5f,50.75f,-0.25f,1.5f,Float(Number(row[4])),Number(row[5])!=0});const std::vector<std::string> current{made ? "1" : "0",std::to_string(fields.total),ParticleBytes(fields)};const auto expected=std::vector<std::string>(row.begin()+6,row.end());const bool same=current==expected;CHECK(same);
        if (!same) {
            // 최초의원본/C++필드를양쪽원문값으로보여준다.
            for (std::size_t index=0;index<current.size();++index) if (current[index]!=expected[index]) std::printf("%s BurningParticle 행%zu 결과%zu 기대%s / 실제%s\n",row[0].c_str(),count,index,expected[index].c_str(),current[index].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==ParticleRows().size()/2);
}
}
// 10.78의ordered float<=0 첫만료칸과실제6DWORD쓰기/현재count를대조한다.
TEST_CASE(BurningParticle_PatchNativeAppend) { ReplayParticle("originals"); }
// CD10.72의같은DWORD signed비교와positive/negative NaN 차이를독립대조한다.
TEST_CASE(BurningParticle_CdNativeAppend) { ReplayParticle("originalCD"); }
