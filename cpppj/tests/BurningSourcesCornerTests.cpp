// 작은 투명 캡처에서 원문 두 판본의 조회 순서와 실제 난수/입자/행 결과를 대조한다.
#include "ConstructionSupport.h"
#include "o/RawBurningSources.h"
#include <stdexcept>

using namespace netstorm::o; // 실제 Source/Append/공유 게임 난수를 사용한다.
using namespace netstorm::test::construction; // 명시 입력의 바이트 직렬화만 빌린다.
namespace { // corner fixture 전용 입력 장치이며 기존 gold/검사 본문은 유지한다.
// 원문 입자의 다섯 float와 source 포인터 의미를 호스트 padding 없이 기록한다.
std::string CornerParticleBytes(const BurningVisualFields& fields) {
    std::vector<std::uint8_t> bytes(fields.capacity*24);
    for (std::size_t index=0;index<fields.particles.size();++index) { // 실제150입자의 모든 의미 필드를 비교한다.
        const auto& p=fields.particles[index];auto raw=std::span<std::uint8_t>(bytes).subspan(index*24,24);
        Put(raw,0,Bits(p.x));Put(raw,4,Bits(p.y));Put(raw,8,Bits(p.velocityX));Put(raw,12,Bits(p.velocityY));Put(raw,16,Bits(p.life));Put(raw,20,p.source ? 1U : 0U);
    }
    return Hex(bytes);
}
// 현재 픽셀/HP/행 메모리는 입력이고 최종 selected/입자/난수는 원문 출력만 기대한다.
void ReplayCornerSources(std::string_view edition,bool selectedGuard=false) {
    const bool patch=edition=="originals";SidPool pool(patch ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,true);
    std::vector<RiftTypeRecord> types(patch ? 188 : 171);GameRandom random;BurningSourcesState sourceState;
    for (std::uint16_t id=5;id<=50;++id) CHECK(pool.Allocate(2)==Sid{id}); // 원문 입력의 피해50을 같은 실제 풀에 확보한다.
    RenderRegistryHooks registryHooks;registryHooks.reserve=[](std::uint32_t)->bool { throw std::logic_error("corner의 미허용 표시 확보"); };
    registryHooks.releaseArray=[] { throw std::logic_error("corner의 미허용 표시 반납"); };registryHooks.attach=[](RenderProcessFields&,std::uint32_t,Sid,std::uint32_t) { throw std::logic_error("corner의 미허용 부착"); };
    RawRenderRegistry registry(pool,std::move(registryHooks));BurningVisualState visualState;BurningVisualHooks visualHooks;
    visualHooks.reserve=[](std::uint32_t)->bool { throw std::logic_error("corner의 미허용 입자 확보"); };
    RawBurningVisual visual(pool,registry,visualState,std::move(visualHooks));RawBurningSources sources(pool,types,visual,random,sourceState);std::size_t count{};
    const auto rows=LoadFixture(NETSTORM_BURNING_CORNER_FIXTURE).rows;
    for (const auto& row:rows) { // 각 실제 원문 입력을 새로운 필드/같은 실제 장치에 공급한다.
        if (row[0]!=edition) continue;CHECK(row.size()==20 && Number(row[5])==0 && Number(row[10])==0 && Number(row[11])==0);
        types[89].flags1=16;types[89].flags2=0;types[89].maxHitPoints=static_cast<std::int32_t>(Number(row[4]));
        auto raw=pool.AllocatedBytes(Sid{50});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=89;Put(raw,26,Number(row[3]),patch ? 4U : 2U);random.SetState(Number(row[8]));
        BurningVisualFields fields;fields.victim=Sid{static_cast<std::uint16_t>(Number(row[12]))};fields.capacity=Number(row[9]);fields.particles.resize(fields.capacity);fields.total=0;fields.selected=static_cast<std::int32_t>(Number(row[6]));
        fields.image.width=static_cast<std::int32_t>(Number(row[1]));fields.image.height=static_cast<std::int32_t>(Number(row[2]));fields.image.pixels.assign(static_cast<std::size_t>(fields.image.width)*fields.image.height,255);
        fields.rows.assign(Number(row[13]),0); // CD 추가 읽기 가능0BYTE도 fixture가 명시한 현재 입력이며 caller 할당을 바꾸지 않는다.
        for (std::size_t index=0;index<fields.particles.size();++index) fields.particles[index]={10.0f+static_cast<float>(index),20.0f+static_cast<float>(index),0,0,0,false}; // 원문 실행기의 초기 입자 입력이다.
        if (selectedGuard) {
            std::fill(fields.image.pixels.begin(),fields.image.pixels.end(),std::uint8_t{});bool rejected{};
            try { sources.Select(fields); }catch (const std::out_of_range& error) { rejected=std::string_view(error.what())=="Burning sources 새 행 인덱스 오류"; }
            CHECK(rejected);return; // 불투명 픽셀 뒤 실제 선택된 잘못된 행은 계속 보호해야 한다.
        }
        sources.Select(fields);
        const std::vector<std::string> actual{"return",std::to_string(fields.selected),std::to_string(fields.total),Hex(fields.rows),CornerParticleBytes(fields),std::to_string(random.State())};
        CHECK(actual==std::vector<std::string>(row.begin()+14,row.end()));++count;
    }
    CHECK(count==18);
}
}
// 10.78은 투명 픽셀에서 논리 행 끝 후보를 읽지 않아야 한다.
TEST_CASE(BurningSourcesCorner_PatchNativeTransparentShortCircuit) { ReplayCornerSources("originals"); }
// CD는 행 우선이며 추가 읽기 가능0BYTE를 명시한 입력을 같은 결과로 재생한다.
TEST_CASE(BurningSourcesCorner_CdNativeRowsBeforePixels) { ReplayCornerSources("originalCD"); }
// guard 삭제/배열 임의 확장으로 투명 단락만 통과시키는 수정을 막는다.
TEST_CASE(BurningSourcesCorner_SelectedOpaqueRowStillRejected) { ReplayCornerSources("originals",true); }
