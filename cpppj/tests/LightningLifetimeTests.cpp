// 빔의 실제 생성/공유 색/경로 소유/반납 전체 원문을 같은 입력으로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawLightningLifetime.h"
#include "o/ParticleGradient.h"

using namespace netstorm::test::rawscene;
namespace {
// fixture의 실제 입력/출력 비트만 읽는다.
std::vector<std::uint8_t> Decode(std::string_view text) {
    if (text.size()%2) throw std::invalid_argument("빔 수명 hex 폭 오류");std::vector<std::uint8_t> result;
    // 두 글자마다 원문 바이트 하나만 추가한다.
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16)));
    return result;
}
// 지정 DWORD/QWORD를 원문 little endian으로 끝에 붙인다.
void Append(std::vector<std::uint8_t>& bytes,std::uint64_t value,std::size_t width=4) {
    // 지정 폭만 저장하여 signed 상위 확장을 막는다.
    for (std::size_t index=0;index<width;++index) bytes.push_back(static_cast<std::uint8_t>(value>>(index*8)));
}
// 실제88BYTE 의미 상태와 같은 순서로 현재 필드를 직렬화한다.
std::vector<std::uint8_t> Fields(const LightningFrameFields& fields,std::size_t allocated) {
    std::vector<std::uint8_t> bytes;const auto& reg=fields.registration;
    Append(bytes,static_cast<std::uint32_t>(reg.form));Append(bytes,static_cast<std::uint32_t>(reg.parent));
    // 공통 영역은 네 float 전체 비트로 비교한다.
    for (const auto value:{reg.left,reg.top,reg.right,reg.bottom}) Append(bytes,std::bit_cast<std::uint32_t>(value));
    Append(bytes,static_cast<std::uint32_t>(reg.slot));Append(bytes,static_cast<std::uint32_t>(reg.zOrder));Append(bytes,allocated);Append(bytes,static_cast<std::uint32_t>(fields.particles.total));
    Append(bytes,std::bit_cast<std::uint64_t>(fields.lastTime),8);Append(bytes,std::bit_cast<std::uint64_t>(fields.elapsed),8);Append(bytes,std::bit_cast<std::uint32_t>(fields.delta));Append(bytes,std::bit_cast<std::uint32_t>(fields.scaledDelta));
    Append(bytes,fields.first ? 1 : 0);Append(bytes,fields.palette);Append(bytes,static_cast<std::uint32_t>(fields.firstLive));Append(bytes,fields.pathAllocated ? 2 : 0);Append(bytes,std::bit_cast<std::uint32_t>(fields.jitter));Append(bytes,fields.sound ? 3 : 0);return bytes;
}
// 공통 슬롯의 빈 칸과 기존 토큰을 모두 원문 순서로 기록한다.
std::string Slots(const RawRenderRegistry& registry) {
    std::string result;
    // 빈 칸0도 건너뛰지 않는다.
    for (const auto token:registry.Slots()) { if (!result.empty()) result+=',';result+=std::to_string(token); }
    return result.empty() ? "-" : result;
}
// 최초 불일치 바이트 위치를 입력 번호와 함께 보고한다.
void Compare(std::span<const std::uint8_t> actual,std::span<const std::uint8_t> expected,std::size_t row,const char* field) {
    if (actual.size()!=expected.size()) { const auto text="빔 수명 행"+std::to_string(row)+" "+field+" 폭 불일치";netstorm::test::ReportFailure(__FILE__,__LINE__,text.c_str());return; }
    const auto mismatch=std::mismatch(actual.begin(),actual.end(),expected.begin());
    if (mismatch.first!=actual.end()) {
        const auto text="빔 수명 행"+std::to_string(row)+" "+field+" 바이트"+std::to_string(mismatch.first-actual.begin())+" 실제"+std::to_string(*mismatch.first)+" 원본"+std::to_string(*mismatch.second);
        netstorm::test::ReportFailure(__FILE__,__LINE__,text.c_str());
    }
}
}
// 기본53비트55입력의 생성·공유 색·경로/입자·등록/반납 순서를 실제 원문과 대조한다.
TEST_CASE(LightningLifetime_Native027f) {
    std::size_t count=0,extended=0,index=0;
    // 비교64비트 행은 보존하지만 기본 고유 입력 수에 중복 가산하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_LIGHTNING_LIFETIME_FIXTURE).rows) {
        const auto current=index++;if (row.size()!=13) throw std::runtime_error("빔 수명 fixture 열 수 오류");if (Number(row[1])==0x037f) { ++extended;continue; }CHECK(Number(row[1])==0x027f);
        const auto args=Split(row[2],',');if (args.size()!=9) throw std::runtime_error("빔 수명 인수 폭 오류");
        const bool warm=Number(args[1])!=0,sound=Number(args[7])!=0;const auto capacity=std::stoi(args[2]),hole=std::stoi(args[3]),parent=std::stoi(args[4]),depth=std::stoi(args[5]),mutate=std::stoi(args[6]);
        SidPool pool(OriginalEdition::Patch1078,32768,false);
        // 부착 전/후 부모와 표적을 원문 실험과 같은 raw 입력으로 구성한다.
        for (const auto values:{std::array<int,3>{0,149,255},{50,148,depth},{51,144,127},{52,149,0}}) {
            auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(pool.Slot(Sid{static_cast<std::uint16_t>(values[0])}).data()),50);std::fill(raw.begin(),raw.end(),std::uint8_t{});
            raw[6]=1;raw[10]=static_cast<std::uint8_t>(values[1]);raw[35]=static_cast<std::uint8_t>(values[2]);
            const auto x=values[0]==0 ? 1.5F : values[0]==50 ? 20.5F : values[0]==51 ? -7.75F : 40.125F;
            const auto y=values[0]==0 ? 1.5F : values[0]==50 ? 21.25F : values[0]==51 ? 8.25F : 35.375F;Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));
        }
        std::vector<std::uint32_t> initial;
        // 목록 검색/확장 결과는 제품 몸체가 결정한다.
        for (int slot=0;slot<capacity;++slot) initial.push_back(slot==hole ? 0U : static_cast<std::uint32_t>(100+slot));
        std::string events;const auto event=[&](std::string value) { if (!events.empty()) events+=';';events+=value; }; // 실제 요청 순서다.
        RenderRegistryHooks registryHooks;registryHooks.reserve=[&](std::uint32_t bytes) { event("N:"+std::to_string(bytes));return true; };registryHooks.releaseArray=[&] { event("F:R"); };
        registryHooks.attach=[&](RenderProcessFields& fields,std::uint32_t type,Sid target,std::uint32_t flags) {
            event("B:"+std::to_string(type)+":"+std::to_string(target.value)+":0:"+std::to_string(flags)+":"+std::to_string(fields.slot)+":"+std::to_string(fields.zOrder));fields.form=7;fields.parent=mutate==2 ? 51 : target.value;
            if (mutate==1) const_cast<std::uint8_t*>(pool.Slot(target).data())[35]=128;
        };
        RawRenderRegistry registry(pool,std::move(registryHooks),std::move(initial));GameRandom random(1);LightningPathState pathState;
        RawLightningPath path(pool,random,pathState,[&](Sid sid) {
            const auto raw=pool.Slot(sid);const auto foot=raw[10]==144 ? 2 : 3;
            return std::array<float,2>{static_cast<float>(static_cast<double>(std::bit_cast<float>(Get(raw,14)))-foot*.5),static_cast<float>(static_cast<double>(std::bit_cast<float>(Get(raw,18)))-foot*.5)};
        });
        LightningLifetimeState state{warm ? 1U : 0U};std::vector<std::uint32_t> colors;
        // warm 표는 원문과 같은11개 외부 입력이다.
        if (warm) for (std::uint32_t color=31;color<42;++color) colors.push_back(color);
        const auto rgbx=Decode(row[3]);std::array<std::uint32_t,256> palette{};CHECK(rgbx.size()==1024);
        // 입력 RGBX는 재계산 없이 DWORD 전체를 읽는다.
        for (std::size_t color=0;color<palette.size();++color) palette[color]=Get(rgbx,color*4);
        LightningLifetimeHooks hooks;hooks.reserve=[&](std::uint32_t bytes) { event("N:"+std::to_string(bytes));return true; };hooks.createPalette=[] { return 1U; };
        hooks.buildPalette=[&](std::uint32_t token) { CHECK(token==1);event("N:44");colors=BuildLightningPalette(palette); };
        RawLightningLifetime lifetime(pool,registry,path,state,std::move(hooks));LightningFrameFields fields;fields.delta=fields.scaledDelta=std::bit_cast<float>(0xa5a5a5a5U);fields.path.bytes.fill(0xa5);
        lifetime.Initialize(fields,1,Sid{static_cast<std::uint16_t>(parent)},Sid{52},sound ? std::optional<std::string>{"shortZap.wav"} : std::nullopt);const auto allocated=fields.particles.values.size();
        Compare(Fields(fields,allocated),Decode(row[4]),current,"생성 상태");Compare(fields.path.bytes,Decode(row[5]),current,"경로");
        // 생성 직후 모든24BYTE 필드는0이어야 하며 용량120 전체가 대상이다.
        for (const auto& particle:fields.particles.values) CHECK(particle.x==0 && particle.y==0 && particle.strength==0 && particle.jitterWeight==0 && particle.life==0 && particle.previous==0);
        CHECK(Decode(row[6])==std::vector<std::uint8_t>(allocated*24));CHECK(Slots(registry)==row[8]);CHECK(events==row[9]);CHECK(state.palette==1);
        std::vector<std::uint8_t> colorBytes;
        // 공유 색 표11개 전체를 비교한다. 인접 할당 DWORD는 이 빔 Draw에서 읽지 않는다.
        for (const auto color:colors) Append(colorBytes,color);
        Compare(colorBytes,Decode(row[7]),current,"팔레트");events.clear();
        lifetime.Destroy(fields,Number(args[8]),[&](LightningRelease released) {
            const auto token=registry.Slots()[static_cast<std::size_t>(fields.registration.slot)];CHECK(token==(released==LightningRelease::Object ? 0U : 1U));event(released==LightningRelease::Path ? "F:L" : released==LightningRelease::Particles ? "F:P" : "F:O");
        });
        CHECK(fields.particles.values.empty());Compare(Fields(fields,allocated),Decode(row[10]),current,"반납 상태");CHECK(Slots(registry)==row[11]);CHECK(events==row[12]);CHECK(state.palette==1);++count;
    }
    CHECK(count==55);CHECK(extended==55);
}
