// 독립 원본392입력의 전체 바이트/free 순서와 실제 Template 저장소의 섹션 반납을 검사한다.
#include "o/RawDataManagerLifetime.h"
#include "client/CampaignTemplateStorage.h"
#include "RawSceneSupport.h"
using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kNodes=0x12101000,kBuffers=0x12200000; // 독립 원문에 사용한 불투명 입력 주소다.
constexpr std::size_t kStride=76,kNodeCount=5; // 각60BYTE 노드의 양쪽8BYTE와 노드 수다.
// 최초 원문의 전체16진 바이트를 읽되 기대 결과를 계산하지 않는다.
std::vector<std::uint8_t> ManagerBytes(std::string_view text) {
    std::vector<std::uint8_t> result;
    // 입력과 기대값의 각 두 글자를 한 바이트로 옮긴다.
    for (std::size_t at=0;at<text.size();at+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(at,2)),nullptr,16)));
    return result;
}
}
// 전체412BYTE·매 free 직전 상태·외부 효과 뒤 head 재조회와 stale 필드를 대조한다.
TEST_CASE(DataManagerLifetime_Native392) {
    const auto rows=LoadFixture(NETSTORM_DATAMANAGER_LIFETIME_FIXTURE).rows;CHECK(rows.size()==392);
    // 각 행의 원본 입력은 제품 출력이나 직전 행의 결과와 독립적이다.
    for (const auto& row:rows) {
        CHECK(row.size()==4);if (row.size()!=4) return;
        const auto input=Split(row[0],',');CHECK(input.size()==6);if (input.size()!=6) return;
        auto bytes=ManagerBytes(row[1]);CHECK(bytes.size()==32+kStride*kNodeCount);if (bytes.size()!=412) return;
        auto manager=std::span(bytes).subspan(8,16);const auto variant=Number(input[5]);std::string trace; // 입력 전체 바이트를 같은 저장소에서 변경한다.
        const auto stream=[&](std::uint32_t handle) { const auto index=(handle-kNodes-8)/kStride;CHECK(index<kNodeCount);return std::span(bytes).subspan(32+index*kStride+8,60); }; // 현재 불투명 node만 같은60BYTE로 해석한다.
        const auto release=[&](char kind,std::uint32_t handle) {
            if (!trace.empty()) trace+=';';trace+=kind;trace+=','+std::to_string(handle)+','+Hex(bytes); // 외부 효과 전에 원문과 같은 전체 메모리를 기록한다.
            const auto index=kind=='B' ? (handle-kBuffers)/0x1000U : (handle-kNodes-8)/static_cast<std::uint32_t>(kStride);auto node=std::span(bytes).subspan(32+index*kStride+8,60);
            if (variant==1 && kind=='B') { Put(node,0,0xabcdef01);Put(node,4,0x87654321);Put(node,0x38,0x98765432); }
            if (variant==2 && kind=='N') { Put(manager,4,0);Put(manager,8,0x87654321); }
            if (variant==3 && kind=='B') { const auto following=Get(manager,4);if (following) Put(stream(following),0x38,0); }
        }; // 원본 관측의 명시 free 경계 변이만 적용하며 제품 판단을 대체하지 않는다.
        DataManagerLifetimeHooks hooks;hooks.stream=stream;hooks.releaseBuffer=[&](auto handle) { release('B',handle); };hooks.releaseStream=[&](auto handle) { release('N',handle); }; // free 종류별 현재 상태만 관측한다.
        if (input[0]=="S") DestroyMemStream(stream(kNodes+8),hooks.releaseBuffer);else DestroyDataManager(manager,hooks);
        CHECK(Hex(bytes)==row[2] && (trace.empty() ? "-" : trace)==row[3]);
    }
}
// 실제 폭이 부족하면 쓰기 전에 거부하고 빈 버퍼/목록은 외부 장치 없이 자연 반환한다.
TEST_CASE(DataManagerLifetime_PhysicalContractsAndEmptyStorage) {
    std::array<std::uint8_t,16> bytes{};bytes.fill(0xa5);const auto before=bytes;bool rejected{};
    try { DestroyMemStream(std::span(bytes).first(15),{}); } catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected && bytes==before);rejected=false;
    try { DestroyDataManager(std::span(bytes).first(11),{}); } catch (const std::invalid_argument&) { rejected=true; }
    CHECK(rejected && bytes==before);Put(bytes,0,0);DestroyMemStream(bytes,{});CHECK(std::all_of(bytes.begin(),bytes.end(),[](auto value) { return value==0; }));
    Put(bytes,8,0x87654321);Put(bytes,12,0x12345678);DestroyDataManager(bytes,{});CHECK(Get(bytes,0)==0x507dd8 && Get(bytes,4)==0 && Get(bytes,8)==0x87654321 && Get(bytes,12)==0x12345678);
}
// 설치 뒤 State가 새로 생기거나 빈 버퍼가 할당돼도 실제 소멸은 현재 벡터/노드를 반납한다.
TEST_CASE(CampaignTemplates_ReleaseCurrentAndLateSectionAllocations) {
    FortTemplate fort;fort.sections.resize(1);fort.sections[0].reserve(37);CampaignTemplateStorage storage;storage.Install(1,std::move(fort),"lateState");storage.Borrow(8,1);
    const auto names=FortTemplate::SectionNames();const auto at=static_cast<std::size_t>(std::find(names.begin(),names.end(),"State")-names.begin());
    auto& current=storage.Template(1);current.sections.resize(at+1);current.sections[at]={1,2,3,4};storage.State(1)={current.sections[at],0,4,true};
    const auto replacement=storage.GrowState(8,28);CHECK(replacement.size()==28 && storage.Template(1).Section("State").data()==replacement.data());storage.CommitStateGrowth(1);
    storage.Clear(8);CHECK(storage.OwnedImages()==1 && storage.Template(1).Section("State").size()==28);storage.ClearAll();CHECK(!storage.OwnedImages() && Get(storage.Headers(),20)==0);
    FortTemplate shrink;shrink.sections.resize(3);shrink.sections[2]={7,8};storage.Install(1,std::move(shrink),"shrink");storage.Template(1).sections.clear();storage.ClearAll();CHECK(!storage.OwnedImages());
    storage.Install(1,FortTemplate{},"empty");storage.ClearAll();CHECK(!storage.OwnedImages());
}
