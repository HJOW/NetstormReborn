// 빔의 실제 생성/공유 색/경로 소유/반납 전체 원문을 같은 입력으로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawLightningLifetime.h"
#include "o/RawBombPreDestroy.h"
#include "o/ParticleGradient.h"
#include "client/SoundProcess.h"
#include "o/RawSquidDestroy.h"

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

// 독립원문의 직접좌표/화면생성·지연타이머147입력을 실제 몸체와 대조한다.
TEST_CASE(LightningScreen_NativeBodies) {
    std::size_t count{},timers{},failures{}; // 전체 고유 입력과 두 작은 경계의 개수다.
    // 64비트 정밀도 관측도 모든 열이 같음을 확인한다.
    for (const auto& row:LoadFixture(NETSTORM_LIGHTNING_SCREEN_FIXTURE).rows) {
        CHECK(row.size()==23);if (row.size()!=23) continue;std::vector<std::uint32_t> args; // 원문19DWORD 입력이다.
        // 기대값은 계산하지 않고 입력만 그대로 변환한다.
        for (const auto& part:Split(row[0],',')) args.push_back(Number(part));
        CHECK(args.size()==19);if (args.size()!=19) continue;
        // 두정밀도 전체원문을 빠뜨리지 않고 비교한다.
        for (std::size_t i=1;i<=11;++i) CHECK(row[i]==row[i+11]);
        if (args[0]==2) {
            SidPool pool(OriginalEdition::Patch1078,24000,false);SquidHash hash;std::vector<std::uint8_t> spots(65536);std::vector<RiftTypeRecord> types(188);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;SquidProcessState clock;SquidProcessHost host(pool,types,kernel,destroy,clock); // 실제타이머필드를소유할SoundProcess의같은Host다.
            netstorm::client::SoundProcessState state;netstorm::client::SoundProcessHooks hooks;std::size_t reads{};bool wrong{};const auto now=std::bit_cast<double>((std::uint64_t(args[18])<<32)|args[17]); // 장치시각만명시입력이다.
            hooks.wallSeconds=[&] { ++reads;return now; };hooks.playLoopAt=[&](auto,auto,auto,auto) { wrong=true;return 0U; };hooks.playOnceAt=[&](auto,auto,auto,auto,auto) { wrong=true;return 0U; };hooks.play=[&](auto,auto,auto,auto,auto,auto) { wrong=true;return 0U; };hooks.stop=[&](auto) { wrong=true; };hooks.isPlaying=[&](auto) { wrong=true;return 0U; };
            netstorm::client::SoundProcessSystem system(host,state,std::move(hooks));netstorm::client::SoundProcess sound(system,123,0,12);sound.Delay(std::bit_cast<float>(args[14]));std::vector<std::uint8_t> bytes;Append(bytes,std::bit_cast<std::uint64_t>(sound.StartTime()),8); // 원본418070의실제QWORD와비교한다.
            CHECK(Hex(bytes)==row[11] && reads==1 && !wrong && row[7]=="W");++timers;++count;continue;
        }
        SidPool pool(OriginalEdition::Patch1078,32768,false); // 원문의현재부모깊이만입력한다.
        // 부착에따른현재부모/깊이변경은큰효과입력이다.
        for (const auto value:{std::array<std::uint32_t,3>{0,149,255},{50,148,args[5]},{51,144,127},{52,149,0}}) { auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(pool.Slot(Sid{static_cast<std::uint16_t>(value[0])}).data()),50);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[6]=1;raw[10]=static_cast<std::uint8_t>(value[1]);raw[35]=static_cast<std::uint8_t>(value[2]); }
        std::vector<std::uint32_t> initial; // 원문의초기공통목록만입력한다.
        for (std::uint32_t i=0;i<args[2];++i) initial.push_back(i==args[3] ? 0U : 100U+i);
        std::string events;const auto event=[&](std::string value) { if (!events.empty()) events+=';';events+=value; }; // 실제효과순서다.
        RenderRegistryHooks registryHooks;registryHooks.reserve=[&](std::uint32_t bytes) { event("N:"+std::to_string(bytes));return true; };registryHooks.releaseArray=[&] { event("F:R"); };
        registryHooks.attach=[&](RenderProcessFields& fields,std::uint32_t type,Sid parent,std::uint32_t flags) { event("B:"+std::to_string(type)+":"+std::to_string(parent.value)+":0:"+std::to_string(flags)+":"+std::to_string(fields.slot)+":"+std::to_string(fields.zOrder));fields.form=7;fields.parent=args[6]==2 ? 51 : parent.value;if (args[6]==1) const_cast<std::uint8_t*>(pool.Slot(parent).data())[35]=128; };
        RawRenderRegistry registry(pool,std::move(registryHooks),std::move(initial));GameRandom random(1);LightningPathState pathState;RawLightningPath path(pool,random,pathState,[](Sid)->std::array<float,2> { throw std::logic_error("직접좌표빔이중심/SID를조회하면오류"); });LightningLifetimeState state{args[1] ? 1U : 0U};std::vector<std::uint32_t> colors; // 현재단일팔레트다.
        if (args[1]) for (std::uint32_t color=31;color<42;++color) colors.push_back(color);
        const auto rgbx=Decode(row[1]);std::array<std::uint32_t,256> palette{};CHECK(rgbx.size()==1024);
        // 원문RGBX 입력만읽는다.
        for (std::size_t i=0;i<palette.size();++i) palette[i]=Get(rgbx,i*4);
        LightningLifetimeHooks hooks;hooks.reserve=[&](std::uint32_t amount) { event("N:"+std::to_string(amount));return true; };hooks.createPalette=[] { return 1U; };hooks.buildPalette=[&](std::uint32_t token) { CHECK(token==1);event("N:44");colors=BuildLightningPalette(palette); };
        RawLightningLifetime lifetime(pool,registry,path,state,std::move(hooks));LightningFrameFields fields;fields.delta=fields.scaledDelta=std::bit_cast<float>(0xa5a5a5a5U);fields.path.bytes.fill(0xa5); // 실제미정바이트의명시입력이다.
        const auto create=[&](std::array<float,2> from,std::array<float,2> to,float life,std::optional<std::string> name) { lifetime.InitializePoints(fields,1,Sid{static_cast<std::uint16_t>(args[4])},from,to,life,std::move(name)); }; // 원본생성몸체를실행한다.
        if (args[0]==0) create({std::bit_cast<float>(args[8]),std::bit_cast<float>(args[9])},{std::bit_cast<float>(args[10]),std::bit_cast<float>(args[11])},std::bit_cast<float>(args[14]),args[15] ? std::optional<std::string>{"shortZap.wav"} : std::nullopt);
        else {
            const auto created=CreateLightningAtScreen(Sid{static_cast<std::uint16_t>(args[4])},{std::bit_cast<std::int32_t>(args[8]),std::bit_cast<std::int32_t>(args[9])},{std::bit_cast<std::int32_t>(args[10]),std::bit_cast<std::int32_t>(args[11])},std::bit_cast<float>(args[14]),std::bit_cast<std::int32_t>(args[12]),std::bit_cast<std::int32_t>(args[13]),[&](Sid parent,std::array<std::int32_t,2> from,std::array<std::int32_t,2> to,float life,std::string name)->LightningVisualProcess* {
                CHECK(parent.value==args[4]);event("N:104");if (!args[16]) return nullptr;create({static_cast<float>(from[0]),static_cast<float>(from[1])},{static_cast<float>(to[0]),static_cast<float>(to[1])},life,std::move(name));return reinterpret_cast<LightningVisualProcess*>(std::uintptr_t{1}); // 명시생성경계의포인터는비영의미토큰이며역참조하지않는다.
            });CHECK((created!=nullptr)==(args[16]!=0));
        }
        if (!args[16]) { CHECK(events==row[7] && Slots(registry)==row[6] && row[2]=="-" && row[3]=="-");++failures;++count;continue; }
        const auto allocated=fields.particles.values.size();Compare(Fields(fields,allocated),Decode(row[2]),count,"직접생성");Compare(fields.path.bytes,Decode(row[3]),count,"직접경로");CHECK(Slots(registry)==row[6] && events==row[7]);CHECK(Decode(row[4])==std::vector<std::uint8_t>(120*24));
        // 실제120입자의모든필드가0으로초기화됐는지확인한다.
        for (const auto& particle:fields.particles.values) CHECK(particle.x==0 && particle.y==0 && particle.strength==0 && particle.jitterWeight==0 && particle.life==0 && particle.previous==0);
        std::vector<std::uint8_t> colorBytes;
        // 실제공유색11개전체를비교한다.
        for (const auto color:colors) Append(colorBytes,color);Compare(colorBytes,Decode(row[5]),count,"직접팔레트");events.clear();
        lifetime.Destroy(fields,args[7],[&](LightningRelease released) { event(released==LightningRelease::Path ? "F:L" : released==LightningRelease::Particles ? "F:P" : "F:O"); });
        Compare(Fields(fields,allocated),Decode(row[8]),count,"직접반납");CHECK(fields.particles.values.empty() && Slots(registry)==row[9] && events==row[10]);++count;
    }
    CHECK(count==147 && timers==25 && failures==2);
}

// 실제41e960의관문·효과뒤현재좌표·비가상공통전처리를독립원문96개와비교한다.
TEST_CASE(LightningScreen_BombPreDestroyNative) {
    SidPool pool(OriginalEdition::Patch1078,24000,true);std::size_t count{}; // 원문물리50바이트와대조수다.
    // 기대효과/바이트는원문fixture에서만읽는다.
    for (const auto& row:LoadFixture(NETSTORM_BOMB_PREDESTROY_FIXTURE).rows) {
        CHECK(row.size()==5);if (row.size()!=5) continue;const auto input=Split(row[0],',');const auto extra=Number(input[0]),flags=Number(input[1]),mutation=Number(input[2]);auto raw=std::span<std::uint8_t>(const_cast<std::uint8_t*>(pool.Slot(Sid{50}).data()),50);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,0x502370);raw[10]=113;raw[11]=2;raw[40]=static_cast<std::uint8_t>(extra);Put(raw,14,std::bit_cast<std::uint32_t>(98.0F));Put(raw,18,std::bit_cast<std::uint32_t>(122.0F));std::string events; // 실제호출순서다.
        const auto event=[&](std::string text) { if (!events.empty()) events+=';';events+=text; }; // 효과는원문과동일한입력변경만한다.
        BombPreDestroyHooks hooks;hooks.explosion=[&](Sid sid) { event("E:"+std::to_string(sid.value));if (mutation==1) { Put(raw,14,0x42cb0000);Put(raw,18,0xc0880000); }if (mutation==2) { raw[40]=9;Put(raw,0,0x501dd0); } };
        hooks.sound=[&](float x,float y) { event("S:"+std::to_string(std::bit_cast<std::uint32_t>(x))+":"+std::to_string(std::bit_cast<std::uint32_t>(y))+":0");if (mutation==3) { raw[40]=0;Put(raw,12,0xbeef,2); } };
        hooks.base=[&](Sid sid,std::uint32_t value) { event("B:"+std::to_string(sid.value)+":"+std::to_string(value)); };RawBombPreDestroy bomb(pool,std::move(hooks));CHECK(bomb.Handles(Sid{50}));bomb.PreDestroy(Sid{50},flags);
        CHECK(events==row[1] && Hex(raw)==row[2] && row[1]==row[3] && row[2]==row[4]);++count;
    }
    CHECK(count==96);
}
