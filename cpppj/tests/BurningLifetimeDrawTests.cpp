// 독립10.78 기계어의 첫 색표/출력/삭제655입력과 공통 프레임128입력을 대조한다.
#include "ConstructionSupport.h"
#include "o/RawBurningLifetimeDraw.h"
#include <iostream>
#include <numeric>
using namespace netstorm::o;
using namespace netstorm::test::construction;
namespace {
// 원본 출력 열은 결과 비교에만 사용하며 제품 계산에 공급하지 않는다.
const auto& LifetimeRows() { static const auto rows=LoadFixture(NETSTORM_BURNING_LIFETIME_DRAW_FIXTURE).rows;return rows; }
// 원본 DWORD 순서의 현재값을 직렬화한다.
template<class T> std::string Join(const T& values,char separator) {
    std::string result;
    for (const auto& value:values) { if (!result.empty()) result+=separator;result+=std::to_string(value); } // 모든 현재 원소의 순서를 유지한다.
    return result;
}
// 실제5픽셀 출력의 순서와 전체64x64표면을 독립 원문과 대조한다.
std::string Draw(const std::vector<std::uint32_t>& v) {
    BurningVisualFields fields;fields.capacity=v[0];fields.total=v[1];fields.particles.resize(v[0]);
    if (fields.capacity) fields.particles[0]={Float(v[2]),Float(v[3]),0,0,Float(v[4]),v[5]!=0};
    if (v[9]) for (std::size_t index=1;index<4;++index) fields.particles[index]={20.75f+static_cast<float>(index),22.5f-static_cast<float>(index),0,0,index==3 ? -1.0f : 4.0f,index==1}; // 별도 source/이동/만료 입자의 명시 입력이다.
    BurningFrameState state;state.cameraX=Float(v[6]);state.cameraY=Float(v[7]);state.viewport=v[8] ? SquidDisplayRect{10,10,30,30} : SquidDisplayRect{0,0,64,64};
    const std::array<std::uint32_t,9> colors{31,32,33,34,35,36,37,38,39}; // 실제 격리 색표의 인접8번 DWORD도 명시 입력이다.
    std::vector<std::uint8_t> pixels(4096,0xa5);std::string writes;
    DrawBurningVisual(fields,state,{8,colors},[&](std::int32_t x,std::int32_t y,std::uint8_t color) { const auto at=static_cast<std::size_t>(y)*64+x;CHECK(at<pixels.size());pixels.at(at)=color;if (!writes.empty()) writes+=';';writes+=std::to_string(at)+':'+std::to_string(color); });
    return std::to_string(Adler(pixels))+':'+std::to_string(std::accumulate(pixels.begin(),pixels.end(),0U))+'|'+writes;
}
// 선택 버퍼와 scalar flags에 따른 실제 반납·소리·등록/base 순서다.
std::string Destroy(const std::vector<std::uint32_t>& v) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true);RenderRegistryHooks hooks;hooks.reserve=[](auto) { return true; };hooks.releaseArray=[] {};hooks.attach=[](auto&,auto,auto,auto) {};
    RawRenderRegistry registry(pool,std::move(hooks),{111,777,222});BurningVisualFields fields;auto& r=fields.registration;r.token=777;r.form=83;r.parent=50;r.slot=1;r.zOrder=-12;fields.sound=v[4] ? 55 : 0;
    if (v[1]) fields.image.pixels.resize(1);if (v[2]) fields.rows.resize(1);if (v[3]) fields.particles.resize(1);std::string actions;
    const auto append=[&](std::string event) { if (!actions.empty()) actions+=';';actions+=event; };
    DestroyBurningVisual(fields,registry,v[0],[&](BurningRelease kind) { const std::array<std::string,4> names{"image","rows","particles","object"};append("Free:"+names.at(static_cast<std::size_t>(kind))); },[&](auto sound) { append("Stop:"+std::to_string(sound)); });
    const std::array<std::uint32_t,4> values{static_cast<std::uint32_t>(r.form),static_cast<std::uint32_t>(r.parent),static_cast<std::uint32_t>(r.slot),static_cast<std::uint32_t>(r.zOrder)};
    return Join(values,',')+'|'+Join(registry.Slots(),',')+'|'+actions;
}
// 생성자의 의미27DWORD/입자zero/부착·확보/첫 gradient 인수를 원문과 대조한다.
std::string Construct(const std::vector<std::uint32_t>& v) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true);InputRaw(pool,{})[35]=static_cast<std::uint8_t>(-8);InputRaw(pool,Sid{50})[35]=static_cast<std::uint8_t>(-11);std::string actions,extra;
    const auto append=[&](std::string event) { if (!actions.empty()) actions+=';';actions+=event; };
    RenderRegistryHooks registryHooks;registryHooks.reserve=[&](auto amount) { append("N:"+std::to_string(amount));return true; };registryHooks.releaseArray=[] {};
    registryHooks.attach=[&](RenderProcessFields& fields,auto type,Sid parent,auto flags) { append("B:"+std::to_string(type)+':'+std::to_string(parent.value)+":0:"+std::to_string(flags));fields.form=83;fields.parent=parent.value; };
    RawRenderRegistry registry(pool,std::move(registryHooks),{0});BurningVisualState state;state.palette=v[0] ? 0 : 1;BurningVisualHooks hooks;
    hooks.reserve=[&](auto amount) { append("N:"+std::to_string(amount));return true; };hooks.createPalette=[] { return 1U; };
    hooks.gradient=[&](auto from,auto to,auto count,double width) { std::vector<std::uint64_t> bits;for (auto value:from) bits.push_back(std::bit_cast<std::uint64_t>(value));for (auto value:to) bits.push_back(std::bit_cast<std::uint64_t>(value));extra="G:"+Join(bits,',')+':'+std::to_string(count)+':'+std::to_string(std::bit_cast<std::uint64_t>(width)); }; // 원본6double을 인수 비트로 보존한다.
    hooks.buildPalette=[&](auto) { extra+=";Palette"; };RawBurningVisual visual(pool,registry,state,std::move(hooks));BurningVisualFields fields;visual.Initialize(fields,777,Sid{static_cast<std::uint16_t>(v[1])},v[2]);const auto& r=fields.registration;
    const auto last=std::bit_cast<std::uint64_t>(fields.lastTime),elapsed=std::bit_cast<std::uint64_t>(fields.elapsed);
    const std::vector<std::uint32_t> values{static_cast<std::uint32_t>(r.form),static_cast<std::uint32_t>(r.parent),Bits(r.left),Bits(r.top),Bits(r.right),Bits(r.bottom),static_cast<std::uint32_t>(r.slot),static_cast<std::uint32_t>(r.zOrder),fields.capacity,fields.total,static_cast<std::uint32_t>(last),static_cast<std::uint32_t>(last>>32),static_cast<std::uint32_t>(elapsed),static_cast<std::uint32_t>(elapsed>>32),fields.palette,fields.sound,fields.stage,fields.image.pixels.empty() ? 0U : 1U,static_cast<std::uint32_t>(fields.image.width),static_cast<std::uint32_t>(fields.image.height),static_cast<std::uint32_t>(fields.image.hotspotX),static_cast<std::uint32_t>(fields.image.hotspotY),fields.victim.value,fields.mode,fields.rows.empty() ? 0U : 1U,static_cast<std::uint32_t>(fields.selected),fields.off ? 1U : 0U};
    std::vector<std::uint8_t> bytes(fields.particles.size()*24);
    for (std::size_t index=0;index<fields.particles.size();++index) { const auto& particle=fields.particles[index];auto raw=std::span<std::uint8_t>(bytes).subspan(index*24,24);Put(raw,0,Bits(particle.x));Put(raw,4,Bits(particle.y));Put(raw,8,Bits(particle.velocityX));Put(raw,12,Bits(particle.velocityY));Put(raw,16,Bits(particle.life));Put(raw,20,particle.source ? 1U : 0U); } // 호스트 padding을 배제한 실제24바이트 필드다.
    return Join(values,',')+'|'+Join(registry.Slots(),',')+'|'+actions+'|'+std::to_string(Adler(bytes))+'|'+std::to_string(std::accumulate(bytes.begin(),bytes.end(),0U))+'|'+(state.palette ? "True" : "False")+'|'+extra;
}
// 입력 종류별 전체 독립 관찰을 빠짐없이 소비한다.
void Replay(std::string_view kind,std::size_t expectedCount) {
    std::size_t count{};
    for (const auto& row:LifetimeRows()) { // 기대 열을 현재값과만 비교한다.
        if (row[0]!=kind) continue;CHECK(row.size()==3);const auto v=Inputs(row[1]);const auto actual=kind=="D" ? Draw(v) : kind=="X" ? Destroy(v) : Construct(v);if (actual!=row[2]) std::cerr<<"Burning "<<kind<<" "<<count<<" ?? "<<row[2]<<" ?? "<<actual<<'\n';CHECK(actual==row[2]);++count;
    }
    CHECK(count==expectedCount);
}
}
TEST_CASE(BurningLifetimeDraw_Native579PixelCases) { Replay("D",579); } // 수명/좌표/clip과 실제 쓰기순서다.
TEST_CASE(BurningLifetimeDraw_Native64DestructorCases) { Replay("X",64); } // 선택 버퍼/flags의 해제 순서다.
TEST_CASE(BurningLifetimeDraw_Native12ColdConstructorCases) { Replay("C",12); } // 최초 색표와 실제 시작/끝 RGB다.
TEST_CASE(BurningLifetimeDraw_Native128FrameGates) {
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_RENDER_FRAME_GATE_FIXTURE).rows) { CHECK(row.size()==3);CHECK(RenderProcessFrameDue(std::stoi(row[0]),Number(row[1]))==(row[2]=="1"));++count; } // signed slot과 현재DWORD frame의 원본 반환이다.
    CHECK(count==128);
}

// 새 원본88개의 현재 프레임/출력WORD/extra를 실제 캡처용+a4 몸체에 공급한다.
TEST_CASE(BurningLifetimeDraw_Native88ArcherCaptureCases) {
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_BURNING_ARCHER_CAPTURE_FIXTURE).rows) { // 원본 그림 요청을 현재 C++ 요청과 직접 비교한다.
        const auto fields=Split(row[0],',');std::array<std::uint8_t,50> raw{};const auto type=Number(fields[0]);Put(raw,0,type==71 ? 0x50b888 : 0x50b9b8);raw[10]=static_cast<std::uint8_t>(type);Put(raw,36,Number(fields[2]));Put(raw,12,Number(fields[3]),2);raw[40]=static_cast<std::uint8_t>(Number(fields[4]));std::string actions;
        DrawBurningArcherCapture(raw,std::stoi(fields[1]),-7,11,[&](auto shape,auto frame,auto x,auto y) { if (!actions.empty()) actions+=';';actions+=std::to_string(shape)+','+std::to_string(frame)+','+std::to_string(x)+','+std::to_string(y); },311,233);
        const auto actual=std::to_string(static_cast<std::uint32_t>(std::stoi(fields[1])))+'|'+actions;CHECK(actual==row[1]);++count;
    }
    CHECK(count==88);
}
