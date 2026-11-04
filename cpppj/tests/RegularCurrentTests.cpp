// 실제 Host의 현재 번호/프로세스 finder와 공유 생성 진단을 원문 입력으로 비교한다.
#include "RawSceneSupport.h"
#include "o/SquidProcess.h"
#include <filesystem>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 실제 Kernel/풀/Host만 사용하며 게임/전투 Frame은 실행하지 않는다.
struct RegularCurrentScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 로컬 Form 번호를 실제 client 영역에서 할당한다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 부착의 물리 TYPE 표다.
    SquidHash hash;std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 공통 삭제기의 실제 공간이다.
    SquidUnpop unpop{pool,hash,spots};RawSquidDestroy destroy{pool,unpop,types}; // 같은 풀의 Form 반납을 담당한다.
    Kernel kernel{16};SquidProcessState state;SquidProcessHost host{pool,types,kernel,destroy,state}; // 프레임 없이 부착/조회만 수행한다.
    RegularCurrentScene() { const auto raw=Raw(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=74; } // 부모의 물리 입력만 만들고 서버 객체 할당은 하지 않는다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // free 상태와 무관하게 원문의 같은 슬롯을 읽고 쓴다.
};
}
TEST_CASE(RegularCurrent_Native1232FinderReturnsRespectFullDwordAndSignedProcessRange) { // 두 실제 finder의 현재 번호/상한/자산 선행 필터를 재생한다.
    RegularCurrentScene scene;std::array<RegularProcess*,3> processes{};const std::array<std::uint32_t,3> events{0x5101,0x5102,0x5203}; // 실제 부모 목록의 앞 순서와 사건이다.
    for (int index=2;index>=0;--index) processes[index]=scene.host.AddRegular(kSource,events[index],0.05F); // 머리 삽입이므로 역순으로 만들어 Native1→2→3과 같은 순서를 얻는다.
    const auto fixture=LoadFixture((std::filesystem::path(__FILE__).parent_path()/"fixtures/regular-current-find-x86.tsv").string().c_str());std::size_t count=0; // 행 누락과 예상 반환을 대조한다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==9);if (row.size()!=9) return;scene.state.regularType=Number(row[1]);scene.state.sharedRegularType=Number(row[2]);scene.state.processTypeEnd=Number(row[3]);
        for (std::size_t index=0;index<processes.size();++index) scene.Raw(processes[index]->Form())[10]=static_cast<std::uint8_t>(Number(row[4+index])); // 실제 연결/PID는 보존하고 TYPE BYTE만 교차한다.
        auto* found=Number(row[0]) ? scene.host.FindEventMasked(kSource,0xfff0,0x5100) : scene.host.FindEvent(kSource,0x5102);const auto expected=Number(row[7]);CHECK(found==(expected ? processes.at(expected-1) : nullptr));++count; // Native의 Kernel getter 방문열은 별도 원문 증거이며 C++에서 방문 횟수를 합성하지 않는다.
    }
    scene.state.regularType=46;scene.state.sharedRegularType=61;scene.state.processTypeEnd=63;for (auto* process:processes) scene.Raw(process->Form())[10]=46;CHECK(count==1232); // 실제 Host/Kernel 반납 전에 검사 입력을 원래 분류로 돌린다.
}
TEST_CASE(RegularCurrent_Native32ConstructorAttachmentsAndSharedDiagnosticReturn) { // 64원문 중 부착 장치 시각 변경이 없는32행의 실제 C++ 부착/후단 필드를 비교한다.
    const auto fixture=LoadFixture((std::filesystem::path(__FILE__).parent_path()/"fixtures/regular-current-constructor-x86.tsv").string().c_str());std::size_t count=0,externalTime=0; // Native 외부 부착 장치가 시각을 바꾸는32행은 별도 관찰로 유지한다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==10);if (row.size()!=10) return;if (Number(row[4])) { ++externalTime;continue; }RegularCurrentScene scene;scene.state.now=17.5;scene.state.regularType=scene.state.sharedRegularType=Number(row[1]);scene.state.postPopDepth=Number(row[2]);std::uint32_t diagnostics=0;
        scene.state.assertion=[&](auto expression,auto file,auto line) { CHECK(expression=="basePostPop == 0" && file==R"(\Ns\O\Regular.cpp)" && line==0x70);++diagnostics;if (Number(row[3])) scene.state.sharedRegularType=48; }; // 진단이 정상 복귀한 뒤의 번호 선택을 확인한다.
        auto* process=Number(row[0]) ? scene.host.AddSharedRegular(kSource,0x5102,0.05F) : scene.host.AddRegular(kSource,0x5102,0.05F);CHECK(process);if (!process) return;const auto calls=Split(row[5],'|');const auto attach=Split(calls.back(),':');CHECK(attach.size()==5 && scene.Raw(process->Form())[10]==Number(attach[1]) && process->Parent()==kSource && process->Event()==Number(row[6]) && std::bit_cast<std::uint32_t>(process->Payload())==Number(row[7]) && process->Count()==Number(row[8]) && process->Time()==std::stod(row[9]));CHECK(diagnostics==(Number(row[0]) && Number(row[2]) ? 1U : 0U) && scene.kernel.Size()==1);++count; // 시전/일반 Kernel Frame을 호출하지 않는다.
    }
    CHECK(count==32 && externalTime==32);
}
TEST_CASE(RegularCurrent_SharedDiagnosticStopsBeforeRegistrationAndSurvivesSelfUnbinding) { // 선택 장치의 실패/정상 복귀 앞후 부분 효과와 callable 수명이다.
    RegularCurrentScene scene;scene.state.postPopDepth=1;const auto free=scene.pool.FreeCount();CHECK(Throws([&] { scene.host.AddSharedRegular(kSource,1,0.05F); }) && scene.kernel.Size()==0 && scene.pool.FreeCount()==free); // 원문 진단 이전에 Form/Kernel을 만들지 않는다.
    auto keep=std::make_shared<int>(0);std::weak_ptr<int> lifetime=keep;scene.state.assertion=[&,keep](auto,auto,auto) { scene.state.assertion={};CHECK(!lifetime.expired());++*keep;scene.state.sharedRegularType=47;scene.state.now=33.25; };keep.reset();auto* process=scene.host.AddSharedRegular(kSource,1,0.05F);CHECK(process && lifetime.expired() && scene.Raw(process->Form())[10]==47 && process->Time()==33.25 && scene.state.postPopDepth==1); // 진단은 깊이를 임의로 보정하지 않는다.
}
TEST_CASE(RegularCurrent_MissingPidIsNotSilentlySkippedBeforeLaterValidReservation) { // Native4afbb1의null+18읽기를정상빈검색으로대체하지않는다.
    RegularCurrentScene scene;auto* later=scene.host.AddRegular(kSource,0x5102,0.05F);auto* first=scene.host.AddRegular(kSource,0x5101,0.05F);const auto raw=scene.Raw(first->Form());const auto saved=Get(raw,18); // 머리 후보의PID만 바꾸고 뒤의정상예약은보존한다.
    for (const auto pid:{0U,16U,0xffffffffU}) { Put(raw,18,pid);CHECK(Throws([&] { scene.host.FindEvent(kSource,0x5102); }) && scene.kernel.Size()==2 && Get(raw,18)==pid);scene.state.regularType=302;CHECK(scene.host.FindEvent(kSource,0x5102)==nullptr);scene.state.regularType=46; } // 원문분류에서선택되지않은TYPE는PID를읽지않는다. 나머지두PID는호스트물리범위거부다.
    Put(raw,18,saved);CHECK(scene.host.FindEvent(kSource,0x5102)==later); // 같은실제슬롯/Kernel을복원한뒤에는원래정상검색을유지한다.
}
