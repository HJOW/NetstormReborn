// 실제 반더 발사와 두 빔 wrapper의 전체 원문 상태/난수/요청 순서를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawCannonFire.h"
#include "o/RawLightningLifetime.h"

using namespace netstorm::test::rawscene;
namespace {
// 원문 경계가 반환한 객체 주소는 이 검사에서만 역참조하지 않는 토큰으로 표현한다.
constexpr std::uintptr_t kBoundaryObject=0x12300000;
// fixture의 원문 hex만 복원하며 기대 결과를 계산하지 않는다.
std::vector<std::uint8_t> Decode(std::string_view text) {
    if (text.size()%2) throw std::invalid_argument("반더 발사 hex 폭 오류");std::vector<std::uint8_t> bytes;
    // 두 글자마다 바이트 하나만 읽는다.
    for (std::size_t index=0;index<text.size();index+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16)));
    return bytes;
}
// 실제 double 필드를 원본 두 DWORD 순서로 저장한다.
void Time(std::span<std::uint8_t> raw,std::size_t offset,double value) {
    const auto bits=std::bit_cast<std::uint64_t>(value);Put(raw,offset,static_cast<std::uint32_t>(bits));Put(raw,offset+4,static_cast<std::uint32_t>(bits>>32));
}
// 첫 불일치 바이트와 입력 번호를 보고한다.
void Compare(std::span<const std::uint8_t> actual,std::span<const std::uint8_t> expected,std::size_t row,const char* field) {
    if (actual.size()!=expected.size()) { const auto text="반더 행"+std::to_string(row)+" "+field+" 폭 불일치";netstorm::test::ReportFailure(__FILE__,__LINE__,text.c_str());return; }
    const auto mismatch=std::mismatch(actual.begin(),actual.end(),expected.begin());
    if (mismatch.first!=actual.end()) {
        const auto text="반더 행"+std::to_string(row)+" "+field+" 바이트"+std::to_string(mismatch.first-actual.begin())+" 실제"+std::to_string(*mismatch.first)+" 원본"+std::to_string(*mismatch.second);netstorm::test::ReportFailure(__FILE__,__LINE__,text.c_str());
    }
}
// 원본과 같은 raw 입력과 명시 월드/Host callback을 사용한다.
struct VanderScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 같은 네 슬롯/현재 부모/표적의 실제 저장소다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 실제 HP getter의 입력 TYPE 표다.
    std::vector<PriestPlainCanonType> frames=std::vector<PriestPlainCanonType>(188); // 반더 Step은 문자 코드를 읽지 않는다.
    CannonFireState state; // 같은 현재 시각과 실제 반더 가상 getter 전역이다.
    GameRandom random; // 원본과 같은 구간 난수의 단일 상태다.
    GunAnimationFields fields; // 미초기화 padding까지 비교할 실제64BYTE다.
    const std::vector<std::string>* row{}; // 현재 독립 입력 행이다.
    std::string events; // 실제 외부 요청을 순서대로 기록한다.
    // 원본 번호70까지 실제 슬롯을 확보한다.
    VanderScene() { for (std::uint16_t sid=5;sid<=70;++sid) CHECK(pool.Allocate(2)==Sid{sid}); }
    // 현재 입력의32비트 숫자를 읽는다.
    std::uint32_t N(std::size_t index) const { return Number((*row)[index]); }
    // 다음 독립 입력은 이전 행의 dead/void도 재설치한다. 제품 쓰기 경로와 구별한 검사 전용 접근이다.
    std::span<std::uint8_t> Raw(std::uint16_t sid) { const auto raw=pool.Slot(Sid{sid});return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 효과 하나를 순서대로 추가한다.
    void Event(std::string value) { if (!events.empty()) events+=';';events+=value; }
    // 현재 원문 입력만 구성하며 출력 기대값은 fixture에서 읽는다.
    void Prepare(const std::vector<std::string>& input) {
        row=&input;events.clear();state=CannonFireState{};state.kind=CannonFireKind::Vander;state.type=148;state.displayType=80;state.effectType=19;state.delay=.3F;state.now=10;random=GameRandom(N(6));
        types[146].flags1=types[158].flags1=N(4) ? 0x10U : 0U;
        // 원본64BYTE의 동일 미초기화 패턴을 먼저 보존한다.
        for (std::size_t index=0;index<fields.bytes.size();++index) fields.bytes[index]=static_cast<std::uint8_t>(index*13+7);
        Put(fields.bytes,0,0x50b170);Put(fields.bytes,4,81);Put(fields.bytes,8,50);Put(fields.bytes,12,std::bit_cast<std::uint32_t>(60.F));Put(fields.bytes,16,std::bit_cast<std::uint32_t>(60.F));Put(fields.bytes,20,std::bit_cast<std::uint32_t>(59.5F));Put(fields.bytes,24,std::bit_cast<std::uint32_t>(59.5F));Put(fields.bytes,32,0x50b16c);fields.bytes[40]=0x20;Put(fields.bytes,42,70,2);Put(fields.bytes,44,60,2);Time(fields.bytes,48,N(2) ? 10 : 10.25);Put(fields.bytes,56,0);
        // 네 관찰 슬롯과 실제 GunForm을 원문과 같은 바이트로 준비한다.
        for (const auto pair:{std::array<std::uint16_t,2>{50,148},{51,148},{60,static_cast<std::uint16_t>(N(5) ? 146 : 158)},{61,158},{70,34}}) {
            auto raw=Raw(pair[0]);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x12400000);Put(raw,6,81,2);raw[10]=static_cast<std::uint8_t>(pair[1]);raw[11]=static_cast<std::uint8_t>(pair[0]==60 ? N(3) : 0);raw[12]=0xa5;raw[34]=1;
            Put(raw,14,std::bit_cast<std::uint32_t>(pair[0]<60 ? 50.5F : 60.F));Put(raw,18,std::bit_cast<std::uint32_t>(pair[0]<60 ? 50.5F : 60.F));Put(raw,36,N(1));
        }
    }
    // 원문 관찰과 같은 callback 뒤에서만 현재 입력을 바꾼다.
    void Mutate(std::string_view where) {
        const auto mutation=N(7);
        if (where=="Center" && mutation==1) Put(fields.bytes,44,61,2);
        if (where=="Point" && mutation==2) { Put(fields.bytes,8,51);Put(fields.bytes,44,61,2); }
        if (where=="Beam" && mutation==3) Raw(60)[11]=2;
        if (where=="Beam" && mutation==4) Put(fields.bytes,44,61,2);
        if (where=="Hit" && mutation==5) Raw(60)[11]=2;
        if (where=="Refresh" && mutation==6) { Put(fields.bytes,8,51);Put(fields.bytes,44,61,2);state.now=100; }
        if (where=="Unpop" && mutation==7) Put(fields.bytes,8,51);
        if (where=="Notify" && mutation==8) Put(fields.bytes,4,82);
    }
    // 원본104BYTE 확보와 별도 대조한 빔 생성자만 외부 경계다.
    LightningCreateResult Beam(Sid parent,Sid target,std::optional<std::string> sound) {
        return CreateLightning(pool,parent,target,std::move(sound),[this](Sid source,Sid victim,std::optional<std::string> name) {
            Event("A:104");if (!N(8)) return static_cast<LightningVisualProcess*>(nullptr);
            Event("L:"+std::to_string(source.value)+":"+std::to_string(victim.value)+":"+(name ? *name : "null"));Mutate("Beam");return reinterpret_cast<LightningVisualProcess*>(kBoundaryObject);
        });
    }
    // 실제 공간/중심/Host 요청만 관찰하며 발사/회피 계산은 제품 모듈이 수행한다.
    GunAnimationHooks Hooks() {
        GunAnimationHooks hooks;hooks.center=[this](Sid sid) { Event("C:"+std::to_string(sid.value));Mutate("Center");return std::array<float,2>{59.5F,59.5F}; };
        hooks.targetPoint=[this](Sid source,Sid target) { Event("T:"+std::to_string(source.value)+":"+std::to_string(target.value));Mutate("Point");return std::array<float,2>{52.25F,52.75F}; };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { Event("U:"+std::to_string(sid.value)+":"+std::to_string(flags));Mutate("Unpop"); };
        hooks.repop=[this](Sid sid,std::uint32_t flags) { Event("P:"+std::to_string(sid.value)+":"+std::to_string(flags)); };
        hooks.notify=[this](Sid,std::uint32_t event,std::uint32_t info,std::uint32_t value) { Event("M:"+std::to_string(event)+":"+std::to_string(info)+":"+std::to_string(value));Mutate("Notify"); };
        hooks.kill=[this](Sid sid,std::uint32_t flags) { Event("K:"+std::to_string(sid.value)+":"+std::to_string(flags)); };return hooks;
    }
    // 빔 생성 뒤 초기 source의 refresh 또는 현재 target Hit를 실제 모듈이 선택한다.
    CannonBeamHooks BeamHooks() {
        return {[this](Sid source,Sid target) { static_cast<void>(Beam(source,target,std::string{"shortZap.wav"})); },
            [this](Sid source) { Event("R:"+std::to_string(source.value));Mutate("Refresh"); },
            [this](Sid source,Sid target) { Event("H:"+std::to_string(source.value)+":"+std::to_string(target.value));Mutate("Hit"); },&random};
    }
};
// 현재 종류의 모든 원문 행을 실행한다. wrapper는 raw 주소/NULL/생성의 결과 종류까지 비교한다.
void Replay(bool wrappers) {
    VanderScene scene;std::size_t count=0,index=0;
    // 기본53비트 원문 기대값을 변경하지 않고 읽는다.
    for (const auto& row:LoadFixture(NETSTORM_VANDER_FIRE_FIXTURE).rows) {
        const auto current=index++;if (row.size()!=16) throw std::runtime_error("반더 발사 fixture 열 수 오류");const auto kind=Number(row[0]);if ((kind>=4)!=wrappers) continue;
        scene.Prepare(row);RawCannonFire logic(scene.pool,scene.types,scene.frames,scene.state,scene.Hooks(),scene.BeamHooks());std::string result="return";
        if (kind==0) {
            logic.Initialize(scene.fields,Sid{70},Sid{60});Put(scene.fields.bytes,4,81);Put(scene.fields.bytes,8,50);scene.Event("G:70:0:16");scene.Event("B:35:50:0:16"); // native의 명시 Host 경계와 같은 부착 입력이다.
        } else if (kind==1) result=std::to_string(logic.Step(scene.fields,static_cast<std::int32_t>(scene.N(1))));
        else if (kind==2) result=logic.Launch(scene.fields) ? "1" : "0";
        else if (kind==3) logic.Frame(scene.fields);
        else {
            const auto created=scene.Beam(Sid{50},Sid{60},kind==4 || scene.N(9) ? std::optional<std::string>{"shortZap.wav"} : std::nullopt);
            result=created.status==LightningCreateStatus::TargetUnavailable ? "raw"+std::to_string(created.skippedTarget.value) : created.status==LightningCreateStatus::Created ? "beam" : "0";
        }
        CHECK(result==row[10]);CHECK((scene.events.empty() ? "-" : scene.events)==row[11]);Compare(scene.fields.bytes,Decode(row[12]),current,"64BYTE");std::vector<std::uint8_t> slots;
        // 네 raw 슬롯의200BYTE를 생존/프레임/재진입 변경까지 보존한다.
        for (const auto sid:{50,51,60,61}) { const auto raw=scene.pool.Slot(Sid{static_cast<std::uint16_t>(sid)});slots.insert(slots.end(),raw.begin(),raw.end()); }
        Compare(slots,Decode(row[13]),current,"네 슬롯");CHECK(scene.random.State()==Number(row[14]));std::array<std::uint8_t,8> now{};Time(now,0,scene.state.now);Compare(now,Decode(row[15]),current,"현재 시각");++count;
    }
    CHECK(count==(wrappers ? 24 : 393));
}
}
// 초기 source/target과 현재 필드의 분리·회피 난수·동시 완료·notify 재진입을 원본과 비교한다.
TEST_CASE(VanderFire_NativeConstructorStepLaunchFrame) { Replay(false); }
// 명명/NULL 파일명 wrapper의 dead/void raw 주소·할당 실패0·실제 ctor 호출을 원본과 비교한다.
TEST_CASE(LightningWrapper_NativeGateAndAllocation) { Replay(true); }
