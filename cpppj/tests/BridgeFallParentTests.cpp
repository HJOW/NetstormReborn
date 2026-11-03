// 현재checking/다리타입의원문부모·실제예약/대여수명을대조한다.
#include "TestSupport.h"
#include "RawSceneSupport.h"
#include "o/RawBridgeFallParent.h"
#include "o/SquidProcess.h"
#include "client/GameAssets.h"
#include "client/RawSurfaceWorld.h"
#include <array>
#include <bit>
#include <fstream>
#include <string>
#include <vector>

using namespace netstorm::o;
namespace { // 최초Native입력/출력과실제호스트를분리한다.
constexpr std::uintptr_t kPool=0x12000000; // Native의논리풀주소이며호스트포인터로역참조하지않는다.
constexpr std::array<std::array<std::uint32_t,2>,3> kCoords{{{0x80000000,0x41780000},{0x42fa8000,0xc0600000},{0x7fc12345,0x7f800000}}}; // 원래인자의binary32입력이다.
std::string TextHex(std::string_view text) { return netstorm::test::rawscene::Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); } // 진단조건/파일의원문BYTE다.
template<class Operation> bool Failed(Operation operation) { try { operation(); }catch (const std::exception&) { return true; }return false; } // 선택진단을빈예약성공으로만들지않는다.
}
TEST_CASE(BridgeFallParent_Native200CurrentCheckingArgumentsAndResume) { // 모든효과열/현재DWORD와원래좌표비트를최초관측에대조한다.
    using namespace netstorm::test::rawscene;std::ifstream input(NETSTORM_BRIDGE_FALL_PARENT_FIXTURE);CHECK(input.good());std::string line;std::getline(input,line);std::size_t rows{};
    while (std::getline(input,line)) { // 제품출력으로기대값을다시쓰지않는다.
        const auto row=Split(line,'\t');CHECK(row.size()==4);if (row.size()!=4) return;const auto p=Split(row[0],',');CHECK(p.size()==6);if (p.size()!=6) return;
        const Sid sid{static_cast<std::uint16_t>(Number(p[0]))};const auto initial=Number(p[1]),genus=Number(p[2]),coordinate=Number(p[3]),resume=Number(p[4]),mutate=Number(p[5]);std::int32_t word=std::bit_cast<std::int32_t>(initial);RawCheckingFlag checking{word};BridgeFallParentHooks hooks;std::string events,outcome="ok";
        const auto event=[&](std::string value) { if (!events.empty()) events+=';';events+=value; }; // 실제도달한원문효과만기록한다.
        hooks.genus=[&](auto current) { CHECK(current==sid);event("G:"+std::to_string(kPool+current.value*50)+":"+std::to_string(genus));if (mutate) word=0;return genus; };
        hooks.assertion=[&](auto condition,auto source,auto sourceLine) { event("D:"+TextHex(condition)+":"+TextHex(source)+":"+std::to_string(sourceLine));if (mutate) word=std::bit_cast<std::int32_t>(0xabcdef10U);if (!resume) throw std::runtime_error("assertion"); };
        hooks.schedule=[&](auto current,float x,float y) { CHECK(current==sid);event("F:"+std::to_string(kPool+current.value*50)+":"+std::to_string(std::bit_cast<std::uint32_t>(x))+":"+std::to_string(std::bit_cast<std::uint32_t>(y))); };
        try { RunBridgeFallParent(sid,std::bit_cast<float>(kCoords.at(coordinate)[0]),std::bit_cast<float>(kCoords.at(coordinate)[1]),checking,hooks); }catch (const std::runtime_error& failure) { outcome=failure.what(); }
        CHECK(outcome==row[1] && std::to_string(std::bit_cast<std::uint32_t>(word))==row[2] && events==row[3]);++rows;
    }
    CHECK(rows==200);
}
TEST_CASE(BridgeFallParent_ActualHostChecksBeforeRegistrationAndKeepsChild) { // 진단실패는등록전이며복귀/정상예약은기존포장과Regular부착을쓴다.
    SidPool pool(OriginalEdition::Patch1078,32768,true);std::vector<RiftTypeRecord> types(188);SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;SquidProcessState state;state.now=9;SquidProcessHost host(pool,types,kernel,destroy,state);
    const auto parent=pool.Allocate();auto raw=pool.AllocatedBytes(parent);raw[10]=82;raw[11]=0;raw[40]=0;std::int32_t word{};RawCheckingFlag checking{word};
    auto* first=ScheduleBridgeFallChecked(host,pool,types,parent,20.75F,21.9F,checking);CHECK(first && first->Event()==kBridgeFallEvent && first->Payload()==5396.0F && first->Time()==9 && first->Count()==0 && kernel.Size()==1);
    word=1;const auto free=pool.FreeCount();CHECK(Failed([&] { ScheduleBridgeFallChecked(host,pool,types,parent,20,21,checking); }) && kernel.Size()==1 && pool.FreeCount()==free); // checking0에서허용한같은현재타입을다음호출은다시진단한다.
    types[82].flags2=TypeFlag2::kBridge;auto* second=ScheduleBridgeFallChecked(host,pool,types,parent,20.75F,21.9F,checking);CHECK(second && second->Payload()==first->Payload() && kernel.Size()==2);
    types[82].flags2=0;word=std::bit_cast<std::int32_t>(0x80000000U);std::uint32_t assertions{};auto* resumed=ScheduleBridgeFallChecked(host,pool,types,parent,20.75F,21.9F,checking,[&](auto condition,auto source,auto sourceLine) { CHECK(condition=="s.getGenus() & gBRIDGE" && source=="\\Ns\\O\\Bridge.cpp" && sourceLine==0x323);++assertions;types[82].flags2=TypeFlag2::kBridge; });CHECK(resumed && assertions==1 && kernel.Size()==3 && resumed->Payload()==5396.0F);kernel.Clear(); // 프레임/낙하행동은실행하지않고검사소유C++프로세스만반납한다.
}
TEST_CASE(BridgeFallParent_StorageViewRetainsDwordAcrossWorldUses) { // 실제공유저장소의현재뷰가Acquire/반납후에도같은owner를읽는다.
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";BaseFileSystem files(root);files.RegisterArchives();netstorm::client::GameAssets assets(files,OriginalEdition::Patch1078);const auto storage=netstorm::client::MakeRawSurfaceStorage(assets);std::int32_t word{};storage->BindChecking(RawCheckingFlag{word});const auto view=storage->Checking();CHECK(!view);
    auto first=storage->Acquire();word=std::bit_cast<std::int32_t>(0x80000000U);CHECK(view.Read()==0x80000000U && storage->Checking().Read()==0x80000000U);first.reset();word=-1;auto second=storage->Acquire();CHECK(view.Read()==0xffffffffU && storage->Checking().Read()==0xffffffffU && storage->Generation()==2);second.reset();word=0;CHECK(!view && !storage->Checking());
}
