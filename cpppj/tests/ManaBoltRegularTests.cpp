// 원본 Regular의 프레임/좌표/음향/가상 공간 요청과 반환을 독립 fixture로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawManaBoltRegular.h"
#include "client/RawCursorPosition.h"
#include "client/Sound.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 현재 fixture의 소수 입력은 원본과 같은 float 비트로 공급한다.
std::uint32_t Bits(float v) { return std::bit_cast<std::uint32_t>(v); }
// 비정렬 DWORD/WORD/BYTE의 지정 폭만 덮어쓴다.
void Store(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 대상 필드 바깥의 상태를 바꾸지 않는다.
    for (std::size_t i=0;i<width;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(i*8));
}
// 원본 독립 입력을 해시/TYPE/SHP/현재 카메라에 배치한다.
struct BoltRegularScene {
    SidPool pool{OriginalEdition::Patch1078,32768,true}; // 실제 raw 폭과 client50/60을 사용한다.
    SquidHash hash; // 실제 일반 finder의 네 단계 공간이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 TYPE/발자국/SHP 유효성이다.
    std::vector<RiftTypeFrames> frames=std::vector<RiftTypeFrames>(188,RiftTypeFrames({})); // 실제 글자 구간 표다.
    std::vector<SquidDisplayShape> shapes=std::vector<SquidDisplayShape>(188); // 자기 TYPE frame0의 signed 헤더다.
    ManaBoltArrivalState arrivalState; // 검색 모듈은 이 검사에서 도착 효과를 호출하지 않는다.
    RawManaBoltArrival arrival{pool,hash,types,arrivalState,{}}; // 주기의 실제 Target을 공유한다.
    ManaBoltRegularState state; // 원본 간격0.05/spacing16이다.
    std::array<std::uint32_t,2> camera{}; // 원본497490이 만든 float 원점 비트를 fixture에서 받는다.
    std::array<std::int32_t,2> origin{}; // 원본 투영의 현재 정수 scroll pixel이다.
    std::string events; // 실제 외부 경계의 전체 순서다.
    // 실제 client 번호를 한 번 할당하고 각 행의 raw만 초기화한다.
    BoltRegularScene() {
        // 마지막 후보60까지 같은 실제 client free list에서 확보한다.
        for (std::uint32_t i=5;i<=60;++i) CHECK(pool.Allocate(2).value==i);
        frames[105]=RiftTypeFrames({{'A','P',1,0},{'A','P',2,0},{'B','P',1,0},{'B','P',2,0}});
        types[105].maxHitPoints=100;types[100].footX=types[100].footY=1;shapes[105].loaded=true;shapes[105].frameCount=4;shapes[105].frames.resize(4);
    }
    // 원본 순서를 구분자 하나로 연결한다.
    void Event(std::string v) { if (!events.empty()) events+=';';events+=v; }
    // 카메라 결과는 이 구현에서 재계산하지 않고 실제497490 관측을 사용한다.
    void Setup(const std::vector<std::string>& input,const std::string& observedCamera) {
        hash.Reset();events.clear();auto bolt=pool.AllocatedBytes(Sid{50}),target=pool.AllocatedBytes(Sid{60});std::fill(bolt.begin(),bolt.end(),std::uint8_t{});std::fill(target.begin(),target.end(),std::uint8_t{});
        Store(bolt,0,0x509810);Store(bolt,10,105,1);Store(bolt,12,Number(input[3]),2);Store(bolt,14,Bits(20.75f));Store(bolt,18,Bits(21.9f));Store(bolt,34,1,1);Store(bolt,36,Number(input[2]));
        if (Number(input[4])) { Store(target,10,100,1);Store(target,40,Number(input[5]),1);Store(target,34,Number(input[6]),1);Store(target,14,Bits(20.5f));Store(target,18,Bits(21.5f));hash.Bucket(1,20.5f,21.5f)=60; }
        const auto profile=Number(input[7]);const std::array<std::array<std::int32_t,2>,4> origins{{{0,0},{319,127},{1920,1080},{2147483600,-2147483600}}};origin=origins.at(profile);
        const std::array<std::array<std::int16_t,2>,4> headers{{{32,5},{1,-7},{32767,32767},{-16,-8}}};shapes[105].frames[0].width=headers.at(profile)[0];shapes[105].frames[0].hotspotX=headers.at(profile)[1];
        const auto values=Split(observedCamera,',');camera={Number(values[0]),Number(values[1])};CHECK(netstorm::client::RawCursorPosition::CameraFromPixels(origin[0],origin[1])==camera);
    }
    // 실제 Frame/공간은 독립 원본과 같은 외부 경계다. 표 계산과 현재 검색은 모듈 자체가 실행한다.
    ManaBoltRegularHooks Hooks() {
        ManaBoltRegularHooks h;
        h.setFrame=[this](Sid sid,std::int32_t f,std::uint32_t flags) { Event("F:"+std::to_string(static_cast<std::uint32_t>(f))+':'+std::to_string(flags));Store(pool.AllocatedBytes(sid),36,static_cast<std::uint32_t>(f)); };
        h.sound=[this](float x,float y,std::string_view name) { CHECK(name=="influenceIcon.wav");Event("S:"+std::to_string(Bits(x))+':'+std::to_string(Bits(y))); };
        h.project=[this](float x,float y) { const auto p=netstorm::client::SoundToScreen({origin[0],origin[1]},x,y);return std::array<std::int32_t,2>{p.x,p.y}; };
        h.inverse=[this](std::int32_t x,std::int32_t y) { return netstorm::client::RawCursorPosition::ScreenToWorld(x,y,camera); };
        h.unpop=[this](Sid,std::uint32_t flags) { Event("U:"+std::to_string(flags)); };
        h.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { Event("P:"+std::to_string(Bits(x))+':'+std::to_string(Bits(y))+':'+std::to_string(flags));auto raw=pool.AllocatedBytes(sid);Store(raw,14,Bits(x));Store(raw,18,Bits(y)); };
        h.destroy=[this](Sid,std::uint32_t flags) { Event("K:"+std::to_string(flags)); };return h;
    }
};
}
// 196개의 원본 입력과 두 정밀도에서 같은 반환/전체 효과/최종 필드를 비교한다.
TEST_CASE(ManaBoltRegular_Native196) {
    BoltRegularScene scene;RawManaBoltRegular regular(scene.pool,scene.types,scene.frames,scene.shapes,scene.arrival,scene.state,scene.Hooks());std::size_t count=0;
    // 원본 결과를 읽고 구현에서 다시 예상 좌표를 계산하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_MANA_BOLT_REGULAR_FIXTURE).rows) {
        CHECK(row.size()==7);const auto input=Split(row[0],',');scene.Setup(input,row[6]);CHECK(regular.Handles(Sid{50}));
        CHECK(Bits(regular.Run(Sid{50},Number(input[0]),Number(input[1])))==Number(row[1]));const auto raw=scene.pool.Slot(Sid{50});CHECK(Get(raw,36)==Number(row[2]));CHECK(Get(raw,14)==Number(row[3]));CHECK(Get(raw,18)==Number(row[4]));CHECK(scene.events==row[5]);++count;
    }
    CHECK(count==196);
}
