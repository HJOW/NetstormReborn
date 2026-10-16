// 실제 빔 Frame/Draw의 전체 의미 상태·경로·용량 입자·요청 순서를 원본과 대조한다.
#include "RawSceneSupport.h"
#include "o/RawLightningFrame.h"

using namespace netstorm::test::rawscene;
namespace {
// fixture의 빈 토큰/hex를 원문 바이트로 복원한다.
std::vector<std::uint8_t> Decode(std::string_view text) {
    if (text=="-") return {};if (text.size()%2) throw std::runtime_error("빔 프레임 hex 폭 오류");std::vector<std::uint8_t> result;
    // 두 글자마다 원문 바이트 하나만 읽는다.
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16)));
    return result;
}
// 실제 DWORD를 원본 little endian으로 끝에 붙인다.
void Append(std::vector<std::uint8_t>& bytes,std::uint32_t value) {
    // 낮은 바이트부터 저장한다.
    for (unsigned index=0;index<4;++index) bytes.push_back(static_cast<std::uint8_t>(value>>(index*8)));
}
// float 입력 비트는 수치 재계산 없이 읽는다.
float Float(std::span<const std::uint8_t> bytes,std::size_t offset) { return std::bit_cast<float>(Get(bytes,offset)); }
// 실제 double 입력 비트는 호스트 정렬에 의존하지 않고 읽는다.
double Double(std::span<const std::uint8_t> bytes,std::size_t offset) {
    const auto value=static_cast<std::uint64_t>(Get(bytes,offset))|(static_cast<std::uint64_t>(Get(bytes,offset+4))<<32);return std::bit_cast<double>(value);
}
// double 필드는 낮은/높은 DWORD를 원문 순서로 저장한다.
void AppendDouble(std::vector<std::uint8_t>& bytes,double value) {
    const auto bits=std::bit_cast<std::uint64_t>(value);Append(bytes,static_cast<std::uint32_t>(bits));Append(bytes,static_cast<std::uint32_t>(bits>>32));
}
// 입자 전체를 실제24BYTE 순서로 직렬화한다.
std::vector<std::uint8_t> Particles(const LightningParticles& particles) {
    std::vector<std::uint8_t> bytes;
    // 미사용 입자와 음수 수명도 비교한다.
    for (const auto& particle:particles.values) {
        Append(bytes,std::bit_cast<std::uint32_t>(particle.x));Append(bytes,std::bit_cast<std::uint32_t>(particle.y));Append(bytes,std::bit_cast<std::uint32_t>(particle.strength));
        Append(bytes,std::bit_cast<std::uint32_t>(particle.jitterWeight));Append(bytes,std::bit_cast<std::uint32_t>(particle.life));Append(bytes,particle.previous);
    }
    return bytes;
}
// 원본 객체의 의미64BYTE와 같은 순서로 상태를 직렬화한다.
std::vector<std::uint8_t> Fields(const LightningFrameFields& fields) {
    std::vector<std::uint8_t> bytes;const auto& rect=fields.registration;
    // 현재 사각형 네 float를 보존한다.
    for (const auto value:{rect.left,rect.top,rect.right,rect.bottom}) Append(bytes,std::bit_cast<std::uint32_t>(value));
    Append(bytes,static_cast<std::uint32_t>(fields.particles.values.size()));Append(bytes,static_cast<std::uint32_t>(fields.particles.total));AppendDouble(bytes,fields.lastTime);AppendDouble(bytes,fields.elapsed);
    Append(bytes,std::bit_cast<std::uint32_t>(fields.delta));Append(bytes,std::bit_cast<std::uint32_t>(fields.scaledDelta));Append(bytes,fields.first ? 1U : 0U);
    Append(bytes,static_cast<std::uint32_t>(fields.firstLive));Append(bytes,std::bit_cast<std::uint32_t>(fields.jitter));Append(bytes,fields.sound ? 1U : 0U);return bytes;
}
// 첫 불일치 위치를 행/필드와 함께 보고한다.
void Compare(std::span<const std::uint8_t> actual,std::span<const std::uint8_t> expected,std::size_t row,const char* field) {
    if (actual.size()!=expected.size()) {
        const auto text="빔 프레임 행"+std::to_string(row)+" "+field+" 폭 실제"+std::to_string(actual.size())+" 원본"+std::to_string(expected.size());netstorm::test::ReportFailure(__FILE__,__LINE__,text.c_str());return;
    }
    const auto mismatch=std::mismatch(actual.begin(),actual.end(),expected.begin());
    if (mismatch.first!=actual.end()) {
        const auto text="빔 프레임 행"+std::to_string(row)+" "+field+" 바이트"+std::to_string(mismatch.first-actual.begin())+" 실제"+std::to_string(*mismatch.first)+" 원본"+std::to_string(*mismatch.second);
        netstorm::test::ReportFailure(__FILE__,__LINE__,text.c_str());
    }
}
// 기본53비트 원문 입력을 같은 raw 부모·난수·출력 경계로 재현한다.
void Replay(std::uint32_t kind) {
    SidPool pool(OriginalEdition::Patch1078,24000,false);
    // 원본 부모50을 포함하는 실제 저장소를 준비한다.
    for (std::uint16_t sid=5;sid<=50;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    std::size_t count=0,index=0;
    // 기대값은 다시 만들지 않고 전체 원문 바이트를 읽는다.
    for (const auto& row:LoadFixture(NETSTORM_LIGHTNING_FRAME_FIXTURE).rows) {
        const auto current=index++;if (row.size()!=12) throw std::runtime_error("빔 프레임 열 수 오류");if (Number(row[0])!=kind) continue;
        const auto before=Decode(row[1]),pathBytes=Decode(row[2]),particles=Decode(row[3]),parent=Decode(row[4]),stateBytes=Decode(row[5]);
        if (before.size()!=64 || pathBytes.size()!=64 || parent.size()!=50 || stateBytes.size()!=28 || particles.size()!=Get(before,16)*24) throw std::runtime_error("빔 프레임 입력 폭 오류");
        auto raw=pool.AllocatedBytes(Sid{50});std::copy(parent.begin(),parent.end(),raw.begin());GameRandom random(Number(row[6]));LightningPathState pathState;
        RawLightningPath path(pool,random,pathState,[](Sid)->std::array<float,2> { throw std::logic_error("프레임/Draw에서 중심 조회가 호출됨"); });
        LightningFrameState state;state.now=Double(stateBytes,0);state.pixelScaleX=Float(stateBytes,8);state.pixelScaleY=Float(stateBytes,12);state.rate=Float(stateBytes,16);state.cameraX=Float(stateBytes,20);state.cameraY=Float(stateBytes,24);
        LightningFrameFields fields;auto& rect=fields.registration;rect.token=1;rect.form=1234;rect.parent=50;rect.left=Float(before,0);rect.top=Float(before,4);rect.right=Float(before,8);rect.bottom=Float(before,12);
        fields.particles.values.resize(Get(before,16));fields.particles.total=static_cast<std::int32_t>(Get(before,20));fields.particles.owner=1;
        fields.lastTime=Double(before,24);fields.elapsed=Double(before,32);fields.delta=Float(before,40);fields.scaledDelta=Float(before,44);fields.first=Get(before,48)!=0;
        fields.firstLive=static_cast<std::int32_t>(Get(before,52));fields.jitter=Float(before,56);if (Get(before,60)) fields.sound="shortZap.wav";
        std::copy(pathBytes.begin(),pathBytes.end(),fields.path.bytes.begin());
        // fixture 토큰은 그대로 같은 입자 저장소의 index+1이다.
        for (std::size_t particle=0;particle<fields.particles.values.size();++particle) {
            const auto offset=particle*24;fields.particles.values[particle]={Float(particles,offset),Float(particles,offset+4),Float(particles,offset+8),Float(particles,offset+12),Float(particles,offset+16),Get(particles,offset+20)};
        }
        std::vector<std::uint8_t> events;bool killed=false;
        RawLightningFrame frame(pool,random,path,state,{
            [&](LightningDirtyRect value) { events.push_back('D');Append(events,static_cast<std::uint32_t>(value.left));Append(events,static_cast<std::uint32_t>(value.top));Append(events,static_cast<std::uint32_t>(value.right));Append(events,static_cast<std::uint32_t>(value.bottom));Append(events,0); },
            [&](LightningLine value) { events.push_back('L');Append(events,static_cast<std::uint32_t>(value.x1));Append(events,static_cast<std::uint32_t>(value.y1));Append(events,static_cast<std::uint32_t>(value.x2));Append(events,static_cast<std::uint32_t>(value.y2));Append(events,value.color); },
            [&](float x,float y,const std::string& filename) { CHECK(filename=="shortZap.wav");events.push_back('S');Append(events,std::bit_cast<std::uint32_t>(x));Append(events,std::bit_cast<std::uint32_t>(y));Append(events,1);Append(events,0); },
            [&] { events.push_back('K');Append(events,0);killed=true; }
        });
        if (kind==0) frame.Step(fields);if (!killed) frame.Draw(fields);
        Compare(Fields(fields),Decode(row[7]),current,"의미 상태");Compare(fields.path.bytes,Decode(row[8]),current,"경로");Compare(Particles(fields.particles),Decode(row[9]),current,"용량 전체 입자");
        CHECK(random.State()==Number(row[10]));Compare(events,Decode(row[11]),current,"출력 순서");++count;
    }
    CHECK(count==(kind==0 ? 220 : 30));
}
}
// 최초/연속 소리·시간 후퇴/긴 간격·전체 용량 수명·흔들림·소진을 원본과 비교한다.
TEST_CASE(LightningFrame_NativeLifetime) { Replay(0); }
// 굵기 경계·포인터 방향·카메라 float 저장/절삭·보조선 위치를 원본과 비교한다.
TEST_CASE(LightningFrame_NativeDraw) { Replay(1); }
