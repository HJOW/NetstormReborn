// 독립 실제 x86 기대값과 raw→실제 Renderer 변경 표/픽셀을 검사한다.
#include "TestSupport.h"
#include "o/SquidDisplay.h"
#include "o/SquidFactory.h"
#include "o/SquidPop.h"
#include "o/SquidUnpop.h"
#include "client/SquidRenderer.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>

using namespace netstorm;
namespace {
// TSV 구분자 이외의 raw 문자열은 보존한다.
std::vector<std::string> Fields(const std::string& line) {
    std::istringstream input(line); std::vector<std::string> result; std::string field;
    // 빈 필드를 포함해 원본 열 순서를 읽는다.
    while (std::getline(input,field,'\t')) result.push_back(field);
    return result;
}
// 원본 경계/변경 표의 정수 목록을 읽는다.
std::vector<int> Numbers(std::string text) {
    std::replace(text.begin(),text.end(),',',' '); std::replace(text.begin(),text.end(),';',' ');
    std::vector<int> result; std::istringstream input(text); int n;
    // 모든 항목을 순서대로 유지한다.
    while (input>>n) result.push_back(n);
    return result;
}
// raw little endian 정수를 쓰며 CD frame 옆 바이트를 보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 지정한 폭만 쓴다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 비트 패턴으로 저장한 float 입력을 되살린다.
float Float(const std::string& text) { return std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(text))); }
// 전체 슬롯 바이트 비교용 hex다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    // raw 출력에서 각 바이트의 앞 0을 보존한다.
    constexpr char digits[]="0123456789abcdef"; std::string result;
    // 대상 슬롯의 모든 바이트를 비교한다.
    for (auto byte:bytes) { result.push_back(digits[byte>>4]); result.push_back(digits[byte&15]); }
    return result;
}
// 원본 표시 억제와 Renderer의 100항목 넘침을 독립 변경 표에 연결한다.
struct Sink final:o::SquidDisplaySink {
    client::DirtyRegions dirty; bool forced{};
    // 첫 화면 표시 억제도 fixture의 full 전역으로 표현한다.
    Sink(int w,int h,bool suppressed):dirty(w,h),forced(suppressed) {}
    // 표 넘침 뒤 후속 표시 호출은 생략한다.
    bool Suppressed() const override { return forced || dirty.FullRedraw(); }
    // 원본 main/shadow 순서를 유지한다.
    void Invalidate(o::SquidDisplayRect r,std::uint32_t f) override { dirty.Add({r.left,r.top,r.right,r.bottom},f); }
};
// 안전 경로는 원본 손상 메모리 접근 대신 예외를 반환한다.
template<class F> bool Throws(F operation) { try { operation(); } catch (const std::exception&) { return true; } return false; }
// 선택한 raw 타입 번호에 대응하는 최소 타입 표다.
std::vector<o::RiftTypeRecord> Types(o::OriginalEdition edition) {
    std::vector<o::RiftTypeRecord> result(edition==o::OriginalEdition::Patch1078 ? 188 : 171);
    // walker 일반 공간 경로는 지도에 spot을 쓰지 않는다.
    for (auto& t:result) { t.footX=t.footY=1; t.flags2=0x10000; t.maxHitPoints=91; t.zOrder=7; }
    return result;
}
// 원본 변경 표의 버려진 항목도 위치/플래그까지 검사한다.
void CheckDirty(const Sink& sink,const std::string& full,const std::string& text) {
    CHECK(sink.Suppressed()==(std::stoi(full)!=0)); const auto expected=Numbers(text);
    CHECK(expected.size()==sink.dirty.Entries().size()*5);
    // 기대 표와 실제 표 크기가 달라도 범위 밖을 읽지 않는다.
    for (std::size_t i=0;i<std::min(expected.size()/5,sink.dirty.Entries().size());++i) {
        const auto& e=sink.dirty.Entries()[i]; const auto n=i*5;
        CHECK(e.rect.left==expected[n]); CHECK(e.rect.top==expected[n+1]);
        CHECK(e.rect.right==expected[n+2]); CHECK(e.rect.bottom==expected[n+3]); CHECK(e.flags==static_cast<std::uint32_t>(expected[n+4]));
    }
}
}

// 두 판본의 독립 실제 표시/경계/Pop/Unpop과 raw 슬롯/dirty 결과를 대조한다.
TEST_CASE(SquidDisplay_X86_ActiveDirtyBoundsAndRawLifecycle) {
    std::ifstream input(NETSTORM_DISPLAY_FIXTURE); CHECK(input.good());
    std::unique_ptr<o::SidPool> pool; std::unique_ptr<o::SquidHash> hash;
    std::unique_ptr<o::SquidFactory> factory; std::unique_ptr<o::SquidDisplay> display;
    std::unique_ptr<o::SquidPop> pop; std::unique_ptr<o::SquidUnpop> unpop; std::unique_ptr<Sink> sink;
    std::vector<o::RiftTypeRecord> types; std::vector<o::SquidDisplayShape> shapes; std::vector<std::uint8_t> spots;
    o::Sid sid{}; std::uint32_t vtable=0; int begins=0,updates=0,bounds=0,pops=0,unpops=0,lineNumber=0; std::string line;
    // Begin/Type/View/Slot은 원본 로더를 완료했다고 주장하지 않는 명시적 준비 입력이다.
    while (std::getline(input,line)) {
        ++lineNumber; if (line.empty() || line[0]=='#') continue; const auto row=Fields(line);
        const auto failuresBefore=test::FailureCount();
        if (row[0]=="Begin") {
            pop.reset(); unpop.reset(); display.reset(); factory.reset(); sink.reset();
            const auto edition=row[1]=="originals" ? o::OriginalEdition::Patch1078 : o::OriginalEdition::Cd1072;
            types=Types(edition); shapes.assign(types.size(),{}); spots.assign(o::kWorldCells*o::kWorldCells,0);
            pool=std::make_unique<o::SidPool>(edition,32768,false); hash=std::make_unique<o::SquidHash>();
            factory=std::make_unique<o::SquidFactory>(*pool,types); sid=factory->Create(74,2); CHECK(sid.value==std::stoul(row[3]));
            const auto bytes=pool->Slot(sid); vtable=static_cast<std::uint32_t>(bytes[0])|(static_cast<std::uint32_t>(bytes[1])<<8)|
                (static_cast<std::uint32_t>(bytes[2])<<16)|(static_cast<std::uint32_t>(bytes[3])<<24); ++begins;
        } else if (row[0]=="Type") {
            auto& type=types[74]; auto& shape=shapes[74]; type.flags1=static_cast<std::uint32_t>(std::stoul(row[1]));
            shape.frameCount=std::stoi(row[2]); shape.loaded=std::stoi(row[3])!=0; type.maxHitPoints=std::stoi(row[4]); shape.frames.clear();
            const auto values=Numbers(row[5]); CHECK(values.size()%4==0);
            // 각 물리 프레임의 signed short 헤더를 순서대로 입력한다.
            for (std::size_t i=0;i+3<values.size();i+=4) shape.frames.push_back({static_cast<std::int16_t>(values[i]),
                static_cast<std::int16_t>(values[i+1]),static_cast<std::int16_t>(values[i+2]),static_cast<std::int16_t>(values[i+3])});
        } else if (row[0]=="View") {
            pop.reset(); unpop.reset(); display.reset();
            sink=std::make_unique<Sink>(std::stoi(row[1]),std::stoi(row[2]),std::stoi(row[7])!=0);
            const auto v=Numbers(row[6]); CHECK(v.size()==4);
            display=std::make_unique<o::SquidDisplay>(pool->Edition(),types,shapes,*sink,
                o::SquidDisplayView{std::stoi(row[3]),std::stoi(row[4]),std::stoi(row[5]),{v[0],v[1],v[2],v[3]}});
            pop=std::make_unique<o::SquidPop>(*pool,*hash,spots,display.get()); unpop=std::make_unique<o::SquidUnpop>(*pool,*hash,spots,display.get());
        } else if (row[0]=="Slot") {
            hash->Reset(); auto bytes=pool->AllocatedBytes(sid); std::fill(bytes.begin(),bytes.end(),std::uint8_t{});
            Put(bytes,0,vtable); bytes[10]=74; bytes[11]=static_cast<std::uint8_t>(std::stoul(row[3]));
            const bool patch=pool->Edition()==o::OriginalEdition::Patch1078;
            Put(bytes,patch ? 36 : 34,static_cast<std::uint32_t>(std::stoll(row[1])),patch ? 4 : 1);
            bytes[patch ? 40 : 35]=static_cast<std::uint8_t>(std::stoul(row[2]));
            Put(bytes,14,static_cast<std::uint32_t>(std::stoul(row[4]))); Put(bytes,18,static_cast<std::uint32_t>(std::stoul(row[5])));
        } else if (row[0]=="Update") {
            const auto before=Hex(pool->Slot(sid)); display->Update(pool->Slot(sid),static_cast<std::uint32_t>(std::stoul(row[1])));
            CHECK(Hex(pool->Slot(sid))==before); CheckDirty(*sink,row[2],row[3]); ++updates;
        } else if (row[0]=="Bounds") {
            const auto r=display->Bounds(74,std::stoi(row[1]),Float(row[2]),Float(row[3]));
            CHECK(r.left==std::stoi(row[4])); CHECK(r.top==std::stoi(row[5])); CHECK(r.right==std::stoi(row[6])); CHECK(r.bottom==std::stoi(row[7])); ++bounds;
        } else if (row[0]=="Pop") {
            CHECK(pop->Pop(sid,types[74],1,1,Float(row[2]),Float(row[3]),static_cast<std::uint32_t>(std::stoul(row[1])))==o::RawPopResult::Registered);
            CHECK(Hex(pool->Slot(sid))==row[4]); CheckDirty(*sink,row[5],row[6]); ++pops;
        } else if (row[0]=="Unpop") {
            unpop->Unpop(sid,types[74],static_cast<std::uint32_t>(std::stoul(row[1])));
            CHECK(Hex(pool->Slot(sid))==row[2]); CheckDirty(*sink,row[3],row[4]); ++unpops;
        } else CHECK(false);
        if (test::FailureCount()!=failuresBefore) { std::cerr<<"표시 fixture 행: "<<lineNumber<<'\n'; break; }
    }
    CHECK(begins==4); CHECK(updates==400); CHECK(bounds>300); CHECK(pops==480); CHECK(unpops==480);
}

// 실제 Renderer로 보낸 raw old/new 위치가 부분 Draw/Present 픽셀에 반영되는지 검사한다.
TEST_CASE(SquidDisplay_RendererAdapter_PaintsOldAndNewRawPositions) {
    const auto edition=o::OriginalEdition::Patch1078; auto types=Types(edition);
    std::vector<o::SquidDisplayShape> shapes(types.size()); shapes[74]={1,true,{{11,17,12,16}}};
    o::SidPool pool(edition,32768,false); o::SquidFactory factory(pool,types); const auto sid=factory.Create(74,2);
    o::SquidHash hash; std::vector<std::uint8_t> spots(o::kWorldCells*o::kWorldCells,0);
    client::Renderer renderer(640,480); client::SquidRenderer sink(renderer);
    o::SquidDisplay display(edition,types,shapes,sink,{0,0,65536,{0,0,640,480}});
    o::SquidPop pop(pool,hash,spots,&display); o::SquidUnpop unpop(pool,hash,spots,&display);
    std::vector<std::uint8_t> pixels(640*480); renderer.Draw(pixels,640,7); renderer.Present([](client::ScreenRect){});
    CHECK(!sink.Suppressed()); CHECK(pop.Pop(sid,types[74],1,1,20,20)==o::RawPopResult::Registered);
    renderer.Draw(pixels,640,9); renderer.Present([](client::ScreenRect){});
    CHECK(pixels[204*640+308]==9); CHECK(pixels[203*640+308]==7); CHECK(pixels[222*640+308]==7);
    unpop.Unpop(sid,types[74]); CHECK(pop.Pop(sid,types[74],1,1,30,30,0x2000)==o::RawPopResult::Registered);
    const auto rects=renderer.Draw(pixels,640,11); CHECK(rects.size()==2); renderer.Present([](client::ScreenRect){});
    CHECK(pixels[204*640+308]==11); CHECK(pixels[314*640+468]==11); CHECK(pixels[300*640+400]==7);
}

// 일반 VFX와 SHP 추가 헤더를 구분하고 signed short/float 원본 비트를 보존한다.
TEST_CASE(SquidDisplay_ShpMetrics_ReadSeparatePrefixAndRejectPlainVfx) {
    std::vector<std::uint8_t> bytes(79); Put(bytes,0,0x30312e31); Put(bytes,4,1); Put(bytes,8,52);
    Put(bytes,16,std::bit_cast<std::uint32_t>(0.6875f)); Put(bytes,20,std::bit_cast<std::uint32_t>(1.5454545f));
    Put(bytes,40,11,2); Put(bytes,42,17,2); Put(bytes,44,static_cast<std::uint32_t>(-12),2); Put(bytes,46,16,2);
    bytes[76]=2; bytes[77]=9; bytes[78]=0;
    client::ShapeDatabase db(bytes); const auto m=db.SquidMetrics(0,0);
    CHECK(m.cellWidth==0.6875f); CHECK(m.cellHeight==1.5454545f); CHECK(m.width==11); CHECK(m.height==17); CHECK(m.hotspotX==-12); CHECK(m.hotspotY==16);
    std::vector<std::uint8_t> plain(43); Put(plain,0,0x30312e31); Put(plain,4,1); Put(plain,8,16); plain[40]=2; plain[41]=9;
    client::ShapeDatabase vfx(plain); CHECK(Throws([&]{ vfx.SquidMetrics(0,0); }));
}

// 잘못된 표시 연결은 공간 쓰기를 시작하기 전에 거부한다.
TEST_CASE(SquidDisplay_InvalidFrameAndEdition_RejectBeforeRawMutation) {
    const auto edition=o::OriginalEdition::Cd1072; auto types=Types(edition);
    std::vector<o::SquidDisplayShape> shapes(types.size()); shapes[74]={1,true,{{11,17,12,16}}}; Sink sink(640,480,false);
    o::SquidDisplay display(edition,types,shapes,sink,{0,0,65536,{0,0,640,480}});
    o::SidPool pool(edition,32768,false); o::SquidFactory factory(pool,types); const auto sid=factory.Create(74,2);
    o::SquidHash hash; std::vector<std::uint8_t> spots(o::kWorldCells*o::kWorldCells,0); o::SquidPop pop(pool,hash,spots,&display);
    pool.AllocatedBytes(sid)[34]=255; const auto before=Hex(pool.Slot(sid));
    CHECK(Throws([&]{ pop.Pop(sid,types[74],1,1,20,20); })); CHECK(Hex(pool.Slot(sid))==before); CHECK(hash.Bucket(1,20,20)==0); CHECK(sink.dirty.Entries().empty());
    CHECK(Throws([&]{ display.Validate(o::OriginalEdition::Patch1078,pool.Slot(sid),0); }));
    CHECK(Throws([&]{ display.SetView({0,0,0,{0,0,640,480}}); }));
}
