// 분해의 원본84BYTE/픽셀 입자/게임 난수/시계와 실제 clip 표면을 비트로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawDissolveFrame.h"

using namespace netstorm::test::rawscene;
namespace {
constexpr std::int32_t kWidth=64,kHeight=48,kPitch=68; //원문 장치의 폭/높이/padding 포함 행 폭이다.
// 빈 토큰 또는 원본 hex의 바이트만 읽으며 기대값을 재계산하지 않는다.
std::vector<std::uint8_t> Decode(std::string_view text) {
    if (text=="-") return {};if (text.size()%2) throw std::invalid_argument("분해 hex 폭 오류");std::vector<std::uint8_t> bytes;
    for (std::size_t index=0;index<text.size();index+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16))); //두 글자가 한 바이트다.
    return bytes;
}
// little endian DWORD/QWORD의 원래 비트를 지정 폭만 저장한다.
void Append(std::vector<std::uint8_t>& bytes,std::uint64_t value,std::size_t width=4) {
    for (std::size_t index=0;index<width;++index) bytes.push_back(static_cast<std::uint8_t>(value>>(index*8))); //낮은 바이트가 먼저다.
}
// 원문 binary32를 정렬이나 산술 변환 없이 읽는다.
float Float(std::span<const std::uint8_t> bytes,std::size_t offset) { return std::bit_cast<float>(Get(bytes,offset)); }
// 원문 binary64의 두 DWORD를 그대로 결합한다.
double Double(std::span<const std::uint8_t> bytes,std::size_t offset) { return std::bit_cast<double>(static_cast<std::uint64_t>(Get(bytes,offset))|(static_cast<std::uint64_t>(Get(bytes,offset+4))<<32)); }
// 미사용64/68과 실제 주소를 제외한 원본84바이트 의미 순서다.
std::vector<std::uint8_t> Fields(const DissolveFrameFields& fields) {
    const auto& value=fields.common;const auto& rect=value.registration;std::vector<std::uint8_t> bytes;
    for (const auto part:{rect.left,rect.top,rect.right,rect.bottom}) Append(bytes,std::bit_cast<std::uint32_t>(part)); //등록 영역의 네 저장 float다.
    Append(bytes,static_cast<std::uint32_t>(rect.zOrder));Append(bytes,value.particles.size());Append(bytes,static_cast<std::uint32_t>(value.total));Append(bytes,std::bit_cast<std::uint64_t>(value.lastTime),8);Append(bytes,std::bit_cast<std::uint64_t>(value.elapsed),8);
    Append(bytes,std::bit_cast<std::uint32_t>(value.delta));Append(bytes,std::bit_cast<std::uint32_t>(value.scaledDelta));Append(bytes,value.first ? 1 : 0);
    for (const auto part:{value.image.width,value.image.height,value.image.hotspotX,value.image.hotspotY}) Append(bytes,static_cast<std::uint32_t>(part)); //캡처의 signed DWORD다.
    Append(bytes,0);Append(bytes,static_cast<std::uint32_t>(fields.opaqueCount));Append(bytes,fields.clockStarted ? 1 : 0);return bytes;
}
// 색 주소만 offset+1로 정규화한 용량 전체의24바이트 입자를 남긴다.
std::vector<std::uint8_t> Particles(const DissolveFrameFields& fields) {
    std::vector<std::uint8_t> bytes;
    for (const auto& part:fields.common.particles) { //수명이 끝난 입자와 미사용 꼬리도 보존한다.
        Append(bytes,std::bit_cast<std::uint32_t>(part.x));Append(bytes,std::bit_cast<std::uint32_t>(part.y));Append(bytes,std::bit_cast<std::uint32_t>(part.velocityX));Append(bytes,std::bit_cast<std::uint32_t>(part.velocityY));Append(bytes,std::bit_cast<std::uint32_t>(part.life));Append(bytes,part.pixel);
    }
    return bytes;
}
// 처음 다른 바이트를 원본 행 번호와 함께 표시한다.
void Compare(std::span<const std::uint8_t> actual,std::span<const std::uint8_t> expected,std::size_t row,const char* field) {
    if (actual.size()!=expected.size()) { const auto message="분해 행"+std::to_string(row)+" "+field+" 폭 실제"+std::to_string(actual.size())+" 원본"+std::to_string(expected.size());netstorm::test::ReportFailure(__FILE__,__LINE__,message.c_str());return; }
    const auto mismatch=std::mismatch(actual.begin(),actual.end(),expected.begin());
    if (mismatch.first!=actual.end()) { const auto message="분해 행"+std::to_string(row)+" "+field+" 바이트"+std::to_string(mismatch.first-actual.begin())+" 실제"+std::to_string(*mismatch.first)+" 원본"+std::to_string(*mismatch.second);netstorm::test::ReportFailure(__FILE__,__LINE__,message.c_str()); }
}
// 직접 준비·캡처 뒤 초기화·실제 Frame/Draw를 각각 같은 고정 입력으로 실행한다.
void Replay(std::uint32_t kind) {
    SidPool pool(OriginalEdition::Patch1078,24000,false);
    for (std::uint16_t sid=5;sid<=62;++sid) CHECK(pool.Allocate(2)==Sid{sid}); //부모와 두 폴의 실제 raw 슬롯을 확보한다.
    std::size_t count{},index{};
    for (const auto& row:LoadFixture(NETSTORM_DISSOLVE_FRAME_FIXTURE).rows) { //기대 상태/픽셀은 최초 원문에서만 읽는다.
        const auto current=index++;if (row.size()!=13) throw std::runtime_error("분해 fixture 열 수 오류");if (Number(row[0])!=kind) continue;
        const auto before=Decode(row[2]),image=Decode(row[3]),particles=Decode(row[4]),parents=Decode(row[5]),globals=Decode(row[6]);
        if (before.size()!=84 || parents.size()!=150 || globals.size()!=28 || particles.size()!=Get(before,20)*24) throw std::runtime_error("분해 입력 폭 오류");
        for (std::size_t part=0;part<3;++part) { const Sid sid{static_cast<std::uint16_t>(part==0 ? 50 : 60+part)};auto raw=pool.AllocatedBytes(sid);std::copy_n(parents.begin()+part*50,50,raw.begin()); } //현재 더미/두 폴을 별도로 공급한다.
        GameRandom game(Number(row[7]));ShrapnelFrameState state;
        state.now=Double(globals,0);state.pixelScaleX=Float(globals,8);state.pixelScaleY=Float(globals,12);state.rate=Float(globals,16);state.cameraX=Float(globals,20);state.cameraY=Float(globals,24);
        DissolveFrameFields fields;auto& value=fields.common;auto& rect=value.registration;rect.parent=50;rect.left=Float(before,0);rect.top=Float(before,4);rect.right=Float(before,8);rect.bottom=Float(before,12);rect.zOrder=static_cast<std::int32_t>(Get(before,16));
        value.particles.resize(Get(before,20));value.total=static_cast<std::int32_t>(Get(before,24));value.lastTime=Double(before,28);value.elapsed=Double(before,36);value.delta=Float(before,44);value.scaledDelta=Float(before,48);value.first=Get(before,52)!=0;
        value.image={static_cast<std::int32_t>(Get(before,56)),static_cast<std::int32_t>(Get(before,60)),static_cast<std::int32_t>(Get(before,64)),static_cast<std::int32_t>(Get(before,68)),image};fields.opaqueCount=static_cast<std::int32_t>(Get(before,76));fields.clockStarted=Get(before,80)!=0;fields.particlesAllocated=kind==2;
        for (std::size_t part=0;part<value.particles.size();++part) { const auto offset=part*24;value.particles[part]={Float(particles,offset),Float(particles,offset+4),Float(particles,offset+8),Float(particles,offset+12),Float(particles,offset+16),Get(particles,offset+20)}; } //원문 색 주소 토큰까지 복원한다.
        std::vector<std::uint8_t> events,canvas(static_cast<std::size_t>(kPitch*kHeight),199);bool killed{};ShrapnelFrameHooks hooks;
        hooks.reserve=[&](std::uint32_t amount) { events.push_back('N');Append(events,amount);return true; };hooks.release=[&](bool scratch) { events.push_back('F');Append(events,scratch ? 1 : 2); };
        hooks.invalidate=[&](FieryTailDirtyRect area) { events.push_back('D');Append(events,static_cast<std::uint32_t>(area.left));Append(events,static_cast<std::uint32_t>(area.top));Append(events,static_cast<std::uint32_t>(area.right));Append(events,static_cast<std::uint32_t>(area.bottom));Append(events,0); };
        hooks.removeForm=[&] { events.push_back('K');Append(events,0);killed=true; };
        hooks.pixel=[&](ShrapnelPixel pixel) {
            events.push_back('P');Append(events,static_cast<std::uint32_t>(pixel.x));Append(events,static_cast<std::uint32_t>(pixel.y));Append(events,pixel.color);
            if (pixel.x>=1 && pixel.x<kWidth-1 && pixel.y>=2 && pixel.y<kHeight-2) canvas[static_cast<std::size_t>(pixel.y*kPitch+pixel.x)]=static_cast<std::uint8_t>(pixel.color);
        };
        RawDissolveFrame logic(pool,game,state,std::move(hooks));
        if (kind==0) logic.Prepare(fields,29,31,Sid{61},Sid{62},static_cast<std::int32_t>(Number(row[1])));
        else if (kind==1) logic.Initialize(fields,Sid{61},Sid{62},static_cast<std::int32_t>(Number(row[1])));
        else { logic.Step(fields);if (!killed) logic.Draw(fields); }
        Compare(Fields(fields),Decode(row[8]),current,"의미 상태");Compare(Particles(fields),Decode(row[9]),current,"전체 입자");CHECK(game.State()==Number(row[10]));Compare(events,Decode(row[11]),current,"요청 순서");Compare(canvas,Decode(row[12]),current,"픽셀/padding");CHECK(value.image.pixels==image);++count;
    }
    CHECK(count==(kind==0 ? 44 : kind==1 ? 44 : 164));
}
}
// 투명/1픽셀/가로·세로와 게임 난수 두 번의 순서를 보존한다.
TEST_CASE(DissolveFrame_NativePreparation) { Replay(0); }
// 현재 더미 좌표/깊이와 캡처 기준점으로 준비하는 원문을 대조한다.
TEST_CASE(DissolveFrame_NativeInitialization) { Replay(1); }
// 최초 delta0·퇴행 시계·소진·dirty·실제 clip과 padding을 대조한다.
TEST_CASE(DissolveFrame_NativeFrameDrawPixels) { Replay(2); }
