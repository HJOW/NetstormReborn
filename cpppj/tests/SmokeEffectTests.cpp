// 실제 냉각 연기의112BYTE 의미 필드·전체 입자/색·난수·장치/수명 요청을 독립 원문에 대조한다.
#include "RawSceneSupport.h"
#include "o/RawSmokeEffect.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 원본 입력을 준비하고 실제 제품 결과만 직렬화하는 대조 장면이다.
// 도구의 입력11열 뒤 원문 출력8열이 이어진다.
enum SmokeColumn : std::size_t { Kind,FieldsInput,ParticlesInput,ParentInput,StateInput,PaletteInput,Seed,Warm,NullStorage,HotX,HotY,Output };
// 빈 배열 표시까지 보존하는 원본 hex decoder다.
std::vector<std::uint8_t> SmokeBytes(const std::string& text) {
    if (text=="-") return {};if (text.size()%2) throw std::runtime_error("연기 원문 hex 길이 오류");std::vector<std::uint8_t> bytes;
    // 각 입력 BYTE를 결과 계산 없이 옮긴다.
    for (std::size_t i=0;i<text.size();i+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(i,2),nullptr,16)));
    return bytes;
}
// 현재 DWORD/QWORD float 비트를 그대로 읽는다.
float SmokeFloat(std::span<const std::uint8_t> raw,std::size_t at) { return std::bit_cast<float>(Get(raw,at)); }
std::int32_t SmokeInt(std::span<const std::uint8_t> raw,std::size_t at) { return std::bit_cast<std::int32_t>(Get(raw,at)); }
double SmokeDouble(std::span<const std::uint8_t> raw,std::size_t at) { return std::bit_cast<double>(static_cast<std::uint64_t>(Get(raw,at))|(static_cast<std::uint64_t>(Get(raw,at+4))<<32)); }
// 원문의 실제112BYTE 순서로 의미 필드만 직렬화한다.
std::string SmokeFieldsHex(const SmokeFields& fields) {
    std::array<std::uint8_t,112> bytes{};std::size_t at{};
    const auto integer=[&](std::uint32_t value) { Put(bytes,at,value);at+=4; };
    const auto floating=[&](float value) { integer(std::bit_cast<std::uint32_t>(value)); };
    const auto real=[&](double value) { const auto bits=std::bit_cast<std::uint64_t>(value);integer(static_cast<std::uint32_t>(bits));integer(static_cast<std::uint32_t>(bits>>32)); };
    const auto& r=fields.registration;integer(r.form);integer(r.parent);floating(r.left);floating(r.top);floating(r.right);floating(r.bottom);integer(r.slot);integer(r.zOrder);integer(fields.capacity);integer(fields.total);real(fields.lastTime);real(fields.elapsed);floating(fields.delta);floating(fields.scaledDelta);integer(fields.first);floating(fields.due);floating(fields.windX);floating(fields.windY);integer(fields.hotspotX);integer(fields.hotspotY);integer(fields.remaining);floating(fields.maximumLife);floating(fields.velocity);floating(fields.spread);floating(fields.period);floating(fields.noise);CHECK(at==bytes.size());return Hex(bytes);
}
// 실제 생성/Frame/Draw 경계를 같은 raw 부모와 공유 난수로 묶는다.
struct SmokeScene {
    SidPool pool{OriginalEdition::Patch1078,kCapacity,false}; // 현재129 부모50의 같은 저장소다.
    SmokeFields fields; // 원본 생성자가 쓰지 않은 입력도 보존한다.
    FieryTailFrameState state; // 현재 시각/픽셀 배율/카메라 입력이다.
    GameRandom random; // 제품의 실제 공유 난수 몸체다.
    FieryTailFramePalette palette; // 원문이 읽을 수 있는 색수/인접 메모리다.
    std::vector<std::uint32_t> colors; // 명시64DWORD 입력을 소유한다.
    std::vector<std::string> events; // new/부착/kill/반납의 실제 순서다.
    std::vector<std::uint8_t> dirty,lines; // 원본 장치 인자5DWORD를 순서대로 보존한다.
    // 입력 원문을 현재 의미 필드/배열에 설치한다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==Output+8);events.clear();dirty.clear();lines.clear();const auto raw=SmokeBytes(row[FieldsInput]);CHECK(raw.size()==128);auto& r=fields.registration;r.token=1;r.form=SmokeInt(raw,4);r.parent=SmokeInt(raw,8);r.left=SmokeFloat(raw,12);r.top=SmokeFloat(raw,16);r.right=SmokeFloat(raw,20);r.bottom=SmokeFloat(raw,24);r.slot=SmokeInt(raw,28);r.zOrder=SmokeInt(raw,32);
        fields.capacity=SmokeInt(raw,36);fields.total=SmokeInt(raw,40);fields.hasStorage=Get(raw,44)!=0;fields.lastTime=SmokeDouble(raw,48);fields.elapsed=SmokeDouble(raw,56);fields.delta=SmokeFloat(raw,64);fields.scaledDelta=SmokeFloat(raw,68);fields.first=Get(raw,72)!=0;fields.due=SmokeFloat(raw,80);fields.windX=SmokeFloat(raw,84);fields.windY=SmokeFloat(raw,88);fields.hotspotX=SmokeInt(raw,92);fields.hotspotY=SmokeInt(raw,96);fields.remaining=SmokeInt(raw,100);fields.maximumLife=SmokeFloat(raw,104);fields.velocity=SmokeFloat(raw,108);fields.spread=SmokeFloat(raw,112);fields.period=SmokeFloat(raw,116);fields.noise=SmokeFloat(raw,120);
        const auto particles=SmokeBytes(row[ParticlesInput]);fields.particles.resize(static_cast<std::size_t>(fields.capacity));CHECK(particles.size()==fields.particles.size()*24);
        // 사용 꼬리 밖의 속도/spare도 같은 비트로 보존한다.
        for (std::size_t i=0;i<fields.particles.size();++i) { auto& p=fields.particles[i];const auto at=i*24;p.x=SmokeFloat(particles,at);p.y=SmokeFloat(particles,at+4);p.velocityX=SmokeFloat(particles,at+8);p.velocityY=SmokeFloat(particles,at+12);p.life=SmokeFloat(particles,at+16);p.spare=Get(particles,at+20); }
        const auto parent=SmokeBytes(row[ParentInput]);const auto slot=pool.Slot(Sid{50});std::copy(parent.begin(),parent.end(),const_cast<std::uint8_t*>(slot.data()));const auto globals=SmokeBytes(row[StateInput]);state.now=SmokeDouble(globals,0);state.pixelScaleX=SmokeFloat(globals,8);state.pixelScaleY=SmokeFloat(globals,12);state.rate=SmokeFloat(globals,16);state.cameraX=SmokeFloat(globals,20);state.cameraY=SmokeFloat(globals,24);random.SetState(Number(row[Seed]));
        const auto color=SmokeBytes(row[PaletteInput]);palette.count=SmokeInt(color,0);colors.resize(64);
        // 실제 공급한 인접 DWORD까지 옮기며 새 색으로 해석하지 않는다.
        for (std::size_t i=0;i<colors.size();++i) colors[i]=Get(color,4+i*4);
        palette.readable=colors;
    }
    // 원본 장치가 받는 다섯 DWORD의 실제 비트를 저장한다.
    void Device(std::vector<std::uint8_t>& target,std::array<std::uint32_t,5> args) {
        // 호출마다 같은20BYTE 인자를 마지막에 추가한다.
        for (const auto value:args) { const auto old=target.size();target.resize(old+4);Put(target,old,value); }
    }
    // 실제 원문 input/output 형식의 전체 입자 배열이다.
    std::string ParticleHex() const {
        std::vector<std::uint8_t> bytes(fields.particles.size()*24);
        // 실제 남은 배열 전체를 비교한다.
        for (std::size_t i=0;i<fields.particles.size();++i) { const auto& p=fields.particles[i];const auto at=i*24;Put(bytes,at,std::bit_cast<std::uint32_t>(p.x));Put(bytes,at+4,std::bit_cast<std::uint32_t>(p.y));Put(bytes,at+8,std::bit_cast<std::uint32_t>(p.velocityX));Put(bytes,at+12,std::bit_cast<std::uint32_t>(p.velocityY));Put(bytes,at+16,std::bit_cast<std::uint32_t>(p.life));Put(bytes,at+20,p.spare); }
        return bytes.empty() ? "-" : Hex(bytes);
    }
    // 실제 제품의 수명 또는 Frame/Draw를 실행한다.
    std::vector<std::string> Run(const std::vector<std::string>& row) {
        RenderRegistryHooks registryHooks;registryHooks.reserve=[this](std::uint32_t bytes) { events.push_back("N:"+std::to_string(bytes));return true; };registryHooks.releaseArray=[this] { events.push_back("F:R"); };registryHooks.attach=[this](RenderProcessFields& r,std::uint32_t type,Sid parent,std::uint32_t flags) { events.push_back("B:"+std::to_string(type)+':'+std::to_string(parent.value)+":0:"+std::to_string(flags));r.form=81;r.parent=parent.value; };RawRenderRegistry registry(pool,std::move(registryHooks));SmokeLifetimeState lifetimeState{Number(row[Warm]) ? 1U : 0U};
        SmokeLifetimeHooks lifetimeHooks;lifetimeHooks.reserve=[this](std::uint32_t bytes) { events.push_back("N:"+std::to_string(bytes));return true; };lifetimeHooks.createPalette=[] { return 1U; };lifetimeHooks.buildPalette=[this](std::uint32_t token) { CHECK(token==1);events.push_back("N:48");const auto native=BuildSmokePalette();std::copy(native.begin(),native.end(),colors.begin());palette.count=12; };RawSmokeLifetime lifetime(pool,registry,lifetimeState,std::move(lifetimeHooks));
        SmokeFrameHooks hooks;hooks.invalidate=[this](FieryTailDirtyRect r) { Device(dirty,{static_cast<std::uint32_t>(r.left),static_cast<std::uint32_t>(r.top),static_cast<std::uint32_t>(r.right),static_cast<std::uint32_t>(r.bottom),0}); };hooks.line=[this](FieryTailLine r) { Device(lines,{static_cast<std::uint32_t>(r.x1),static_cast<std::uint32_t>(r.y1),static_cast<std::uint32_t>(r.x2),static_cast<std::uint32_t>(r.y2),r.color}); };hooks.kill=[this](std::uint32_t flags) { events.push_back("K:"+std::to_string(flags)); };RawSmokeFrame frame(pool,random,state,palette,std::move(hooks));const auto& kind=row[Kind];const bool constructed=kind=="ctor" || kind=="cold" || kind=="wrapper";
        if (constructed) {
            if (kind=="ctor") { SmokeRequest request;request.parent=Sid{50};request.hotspotX=std::stoi(row[HotX]);request.hotspotY=std::stoi(row[HotY]);request.windX=fields.windX;request.windY=fields.windY;request.maximumLife=fields.maximumLife;request.velocity=fields.velocity;request.spread=fields.spread;request.period=fields.period;request.noise=fields.noise;lifetime.Initialize(fields,1,request); }
            else { if (kind=="wrapper") { const auto parent=pool.Slot(Sid{50});events.push_back("H:"+std::to_string(parent[40]&1)+':'+std::to_string(Get(parent,36))); }lifetime.InitializeCooling(fields,1,Sid{50},{std::stoi(row[HotX]),std::stoi(row[HotY])}); }
        } else { if (kind=="spawn") frame.Emit(fields,-7.75F,31.125F,100);else frame.Step(fields);frame.Draw(fields); }
        std::vector<std::uint8_t> colorBytes(4+colors.size()*4);Put(colorBytes,0,palette.count);
        // 원문 공유 표의 읽을 수 있는 전체 메모리를 비교한다.
        for (std::size_t i=0;i<colors.size();++i) Put(colorBytes,4+i*4,colors[i]);
        std::vector<std::string> result{SmokeFieldsHex(fields),ParticleHex(),Hex(colorBytes),std::to_string(random.State()),dirty.empty() ? "-" : Hex(dirty),lines.empty() ? "-" : Hex(lines)};
        if (constructed) { lifetime.Destroy(fields,1,[this](SmokeRelease what) { events.push_back(what==SmokeRelease::Particles ? "F:P" : "F:O"); });result.push_back(SmokeFieldsHex(fields));CHECK(registry.Slots().size()==40 && registry.Slots()[0]==0);CHECK(lifetimeState.palette==1); }else result.emplace_back("-");
        std::string trace;
        // 실제 외부 경계의 호출 순서를 그대로 기록한다.
        for (const auto& event:events) { if (!trace.empty()) trace+=';';trace+=event; }
        result.push_back(std::move(trace));return result;
    }
};
// 최초 차이의 입력/의미 열을 진단하고 원문 기대값은 변경하지 않는다.
void ReplaySmoke(const std::string& kind,std::size_t expectedCount) {
    const auto rows=LoadFixture(NETSTORM_SMOKE_EFFECT_FIXTURE).rows;SmokeScene scene;std::size_t count{};
    // 같은 종류의 모든 독립 입력을 대조한다.
    for (std::size_t i=0;i<rows.size();++i) {
        const auto& row=rows[i];if (row[Kind]!=kind) continue;scene.Prepare(row);const auto actual=scene.Run(row);++count;
        // 전체8개 출력 열을 허용오차 없이 비교한다.
        for (std::size_t j=0;j<actual.size();++j) if (actual[j]!=row[Output+j]) { std::fprintf(stderr,"연기 원문 차이 kind=%s row=%zu column=%zu actual=%s expected=%s\n",kind.c_str(),i,j,actual[j].c_str(),row[Output+j].c_str());CHECK(false);return; }
    }
    CHECK(count==expectedCount);
}
}
TEST_CASE(SmokeEffect_NativeFrameDrawAndSharedRandom) { ReplaySmoke("frame",841); } // 전체 delta/동적 분열/가득 찬 꼬리/dirty·선/색을 대조한다.
TEST_CASE(SmokeEffect_NativeEmitSlotFailureAndReuse) { ReplaySmoke("spawn",28); } // 슬롯 확보 실패의 난수 보존과 재사용을 대조한다.
TEST_CASE(SmokeEffect_NativeCoolingConstructorAndRelease) { ReplaySmoke("cold",8); } //200입자/공유 색/10회·3간격과 실제 소멸을 대조한다.
TEST_CASE(SmokeEffect_NativeCurrentShapeCoolingWrapper) { ReplaySmoke("wrapper",8); } // 현재 SHP 조회 인자/signed 포인트를 원문 wrapper와 대조한다.
TEST_CASE(SmokeEffect_NativeBaseConstructorPreservesTimerPadding) { ReplaySmoke("ctor",8); } // 기본 생성의8회와 쓰지 않는 delta 입력을 대조한다.
