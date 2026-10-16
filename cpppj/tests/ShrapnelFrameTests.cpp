// 파편의 원본76BYTE/용량 전체/두 난수/요청/실제 clip 픽셀을 비트 단위로 비교한다.
#include "RawSceneSupport.h"
#include "o/RawShrapnelFrame.h"

using namespace netstorm::test::rawscene;
namespace {
// 원본 장치 입력의 폭/높이/padding 포함 행 폭이다.
constexpr std::int32_t kWidth=64,kHeight=48,kPitch=68;
// 빈 토큰과 원본 hex를 입력 바이트로만 복원한다.
std::vector<std::uint8_t> Decode(std::string_view text) {
    if (text=="-") return {};if (text.size()%2) throw std::invalid_argument("파편 hex 폭 오류");std::vector<std::uint8_t> bytes;
    // 두 글자마다 바이트 하나를 읽는다.
    for (std::size_t index=0;index<text.size();index+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16)));
    return bytes;
}
// 원본 little endian DWORD/QWORD를 정확한 폭으로 저장한다.
void Append(std::vector<std::uint8_t>& bytes,std::uint64_t value,std::size_t width=4) {
    // 낮은 바이트부터 지정 폭만 추가한다.
    for (std::size_t index=0;index<width;++index) bytes.push_back(static_cast<std::uint8_t>(value>>(index*8)));
}
// 입력 float 비트는 수치 재계산 없이 읽는다.
float Float(std::span<const std::uint8_t> bytes,std::size_t offset) { return std::bit_cast<float>(Get(bytes,offset)); }
// 입력 double 비트는 정렬에 의존하지 않고 읽는다.
double Double(std::span<const std::uint8_t> bytes,std::size_t offset) { return std::bit_cast<double>(static_cast<std::uint64_t>(Get(bytes,offset))|(static_cast<std::uint64_t>(Get(bytes,offset+4))<<32)); }
// 실제76BYTE 의미 상태를 원문 순서로 직렬화한다.
std::vector<std::uint8_t> Fields(const ShrapnelFrameFields& fields) {
    std::vector<std::uint8_t> bytes;const auto& rect=fields.registration;
    // 네 표시 영역 float를 먼저 보존한다.
    for (const auto value:{rect.left,rect.top,rect.right,rect.bottom}) Append(bytes,std::bit_cast<std::uint32_t>(value));
    Append(bytes,static_cast<std::uint32_t>(rect.zOrder));Append(bytes,fields.particles.size());Append(bytes,static_cast<std::uint32_t>(fields.total));Append(bytes,std::bit_cast<std::uint64_t>(fields.lastTime),8);Append(bytes,std::bit_cast<std::uint64_t>(fields.elapsed),8);
    Append(bytes,std::bit_cast<std::uint32_t>(fields.delta));Append(bytes,std::bit_cast<std::uint32_t>(fields.scaledDelta));Append(bytes,fields.first ? 1 : 0);
    // 캡처 치수/기준점은 signed DWORD 전체를 보존한다.
    for (const auto value:{fields.image.width,fields.image.height,fields.image.hotspotX,fields.image.hotspotY}) Append(bytes,static_cast<std::uint32_t>(value));
    Append(bytes,std::bit_cast<std::uint32_t>(fields.gravity));return bytes;
}
// 용량 전체의24BYTE를 원문 순서로 직렬화한다.
std::vector<std::uint8_t> Particles(const ShrapnelFrameFields& fields) {
    std::vector<std::uint8_t> bytes;
    // 미사용 입자의0도 모두 포함한다.
    for (const auto& particle:fields.particles) {
        Append(bytes,std::bit_cast<std::uint32_t>(particle.x));Append(bytes,std::bit_cast<std::uint32_t>(particle.y));Append(bytes,std::bit_cast<std::uint32_t>(particle.velocityX));Append(bytes,std::bit_cast<std::uint32_t>(particle.velocityY));Append(bytes,std::bit_cast<std::uint32_t>(particle.life));Append(bytes,particle.pixel);
    }
    return bytes;
}
// 첫 차이를 행/바이트와 함께 보고한다.
void Compare(std::span<const std::uint8_t> actual,std::span<const std::uint8_t> expected,std::size_t row,const char* field) {
    if (actual.size()!=expected.size()) { const auto text="파편 행"+std::to_string(row)+" "+field+" 폭 실제"+std::to_string(actual.size())+" 원본"+std::to_string(expected.size());netstorm::test::ReportFailure(__FILE__,__LINE__,text.c_str());return; }
    const auto mismatch=std::mismatch(actual.begin(),actual.end(),expected.begin());
    if (mismatch.first!=actual.end()) {
        const auto text="파편 행"+std::to_string(row)+" "+field+" 바이트"+std::to_string(mismatch.first-actual.begin())+" 실제"+std::to_string(*mismatch.first)+" 원본"+std::to_string(*mismatch.second);netstorm::test::ReportFailure(__FILE__,__LINE__,text.c_str());
    }
}
// 실제 원문 입력의 Prepare 또는 Frame/Draw를 재현한다.
void Replay(std::uint32_t kind) {
    SidPool pool(OriginalEdition::Patch1078,24000,false);
    // 원문 부모50까지 실제 슬롯을 확보한다.
    for (std::uint16_t sid=5;sid<=50;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    std::size_t count=0,index=0;
    // 기대 상태/픽셀은 다시 계산하지 않고 원문을 읽는다.
    for (const auto& row:LoadFixture(NETSTORM_SHRAPNEL_FRAME_FIXTURE).rows) {
        const auto current=index++;if (row.size()!=15) throw std::runtime_error("파편 fixture 열 수 오류");if (Number(row[0])!=kind) continue;
        const auto before=Decode(row[2]),image=Decode(row[3]),particles=Decode(row[4]),parent=Decode(row[5]),globals=Decode(row[6]);
        if (before.size()!=76 || parent.size()!=50 || globals.size()!=28 || particles.size()!=Get(before,20)*24) throw std::runtime_error("파편 입력 폭 오류");
        auto raw=pool.AllocatedBytes(Sid{50});std::copy(parent.begin(),parent.end(),raw.begin());GameRandom game(Number(row[7]));CrtRandom crt(Number(row[8]));ShrapnelFrameState state;
        state.now=Double(globals,0);state.pixelScaleX=Float(globals,8);state.pixelScaleY=Float(globals,12);state.rate=Float(globals,16);state.cameraX=Float(globals,20);state.cameraY=Float(globals,24);
        ShrapnelFrameFields fields;auto& rect=fields.registration;rect.parent=50;rect.left=Float(before,0);rect.top=Float(before,4);rect.right=Float(before,8);rect.bottom=Float(before,12);rect.zOrder=static_cast<std::int32_t>(Get(before,16));
        fields.particles.resize(Get(before,20));fields.total=static_cast<std::int32_t>(Get(before,24));fields.lastTime=Double(before,28);fields.elapsed=Double(before,36);fields.delta=Float(before,44);fields.scaledDelta=Float(before,48);fields.first=Get(before,52)!=0;
        fields.image={static_cast<std::int32_t>(Get(before,56)),static_cast<std::int32_t>(Get(before,60)),static_cast<std::int32_t>(Get(before,64)),static_cast<std::int32_t>(Get(before,68)),image};fields.gravity=Float(before,72);
        // 색 주소만 같은 bitmap의 offset+1 토큰으로 입력한다.
        for (std::size_t particle=0;particle<fields.particles.size();++particle) {
            const auto offset=particle*24;fields.particles[particle]={Float(particles,offset),Float(particles,offset+4),Float(particles,offset+8),Float(particles,offset+12),Float(particles,offset+16),Get(particles,offset+20)};
        }
        std::vector<std::uint8_t> events,canvas(static_cast<std::size_t>(kPitch*kHeight),199);bool killed=false;ShrapnelFrameHooks hooks;
        hooks.reserve=[&](std::uint32_t amount) { events.push_back('N');Append(events,amount);return true; };hooks.release=[&](bool scratch) { events.push_back('F');Append(events,scratch ? 1 : 2); };
        hooks.invalidate=[&](FieryTailDirtyRect value) { events.push_back('D');Append(events,static_cast<std::uint32_t>(value.left));Append(events,static_cast<std::uint32_t>(value.top));Append(events,static_cast<std::uint32_t>(value.right));Append(events,static_cast<std::uint32_t>(value.bottom));Append(events,0); };
        hooks.removeForm=[&] { events.push_back('K');Append(events,0);killed=true; };
        hooks.pixel=[&](ShrapnelPixel value) {
            events.push_back('P');Append(events,static_cast<std::uint32_t>(value.x));Append(events,static_cast<std::uint32_t>(value.y));Append(events,value.color);
            if (value.x>=1 && value.x<kWidth-1 && value.y>=2 && value.y<kHeight-2) canvas[static_cast<std::size_t>(value.y*kPitch+value.x)]=static_cast<std::uint8_t>(value.color);
        };
        RawShrapnelFrame logic(pool,game,crt,state,std::move(hooks));
        if (kind==0) logic.Prepare(fields,static_cast<ShrapnelKind>(Number(row[1])));else { logic.Step(fields);if (!killed) logic.Draw(fields); }
        Compare(Fields(fields),Decode(row[9]),current,"의미 상태");Compare(Particles(fields),Decode(row[10]),current,"용량 전체");CHECK(game.State()==Number(row[11]) && crt.State()==Number(row[12]));
        Compare(events,Decode(row[13]),current,"요청 순서");Compare(canvas,Decode(row[14]),current,"픽셀/padding");CHECK(fields.image.pixels==image);++count;
    }
    CHECK(count==(kind==0 ? 198 : 782));
}
}
// 세 가상 생성·매 반복 검사 rand·게임 난수·가장자리/투명 픽셀을 전체 원문과 비교한다.
TEST_CASE(ShrapnelFrame_NativePreparation) { Replay(0); }
// 저장된 가속도/넓은 y·진입 수명/소진·정수화/clip·행 padding을 전체 원문과 비교한다.
TEST_CASE(ShrapnelFrame_NativeFrameDrawPixels) { Replay(1); }
