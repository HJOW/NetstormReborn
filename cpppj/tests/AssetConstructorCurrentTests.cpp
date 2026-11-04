// 자산 bare 생성자의 물리 슬롯 전체와 선택 진단의 수명/반환 순서를 실제 원문으로 대조한다.
#include "RawSceneSupport.h"
#include "o/SquidFactory.h"
#include <filesystem>
#include <memory>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // Native 입력/출력 해석과 순수 물리 메모리 접근만 담당한다.
constexpr std::uint32_t kDaisConstructor=0x44a420U; // 원문169의 진단 뒤 VT 저장 주소다.
std::vector<std::uint8_t> AssetCtorBytes(const std::string& hex) { // Native 슬롯의 전체 BYTE를 그대로 푼다.
    std::vector<std::uint8_t> result; // 순서 있는 원문 바이트다.
    for (std::size_t index=0;index<hex.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(index,2),nullptr,16))); // 두 문자마다 한BYTE다.
    return result;
}
std::span<std::uint8_t> PhysicalAssetBytes(SidPool& pool,Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 입력 payload용이며 제품의 할당 검사를 대체하지 않는다.
}
TEST_CASE(AssetConstructorCurrent_Native1036PhysicalSlotsAndSelectedDiagnosticReturn) { // 새 공중3종/Outpost 실행을 제외한 양판본145recipe와 PATCH 두 특수 함수를 대조한다.
    const auto fixture=LoadFixture((std::filesystem::path(__FILE__).parent_path()/"fixtures/asset-constructor-current-x86.tsv").string().c_str());std::size_t cases=0; // 실제 원문 재생 수다.
    for (const auto& row:fixture.rows) { CHECK(row.size()==8);if (row.size()!=8) return;const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;SidPool pool(edition,65535,false);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188U : 171U);SquidFactory factory(pool,types);const auto sid=Sid{static_cast<std::uint16_t>(std::stoul(row[2]))};const auto before=AssetCtorBytes(row[4]),expected=AssetCtorBytes(row[5]);auto raw=PhysicalAssetBytes(pool,sid);CHECK(raw.size()==before.size() && raw.size()==expected.size());if (raw.size()!=before.size() || raw.size()!=expected.size()) return;std::copy(before.begin(),before.end(),raw.begin());std::uint32_t diagnoses=0;const auto free=pool.FreeCount(); // 각 입력은 별도 풀로 SID0 머리 변화를 격리한다.
        factory.BindDiagnostics({{},[&](std::string_view expression,std::string_view file,std::uint32_t line) { CHECK(expression=="false" && file==".\\Dais.cpp" && line==0x484U);++diagnoses;if (std::stoul(row[3])) { raw[10]=75;raw[34]=4; } },{}}); // Native에 공급한 정상 복귀/현재 payload 변경만 공급한다.
        CHECK(factory.ConstructByAddress(static_cast<std::uint32_t>(std::stoul(row[1])),sid));CHECK(std::equal(raw.begin(),raw.end(),expected.begin()) && diagnoses==std::stoul(row[7]) && pool.FreeCount()==free && raw[11]==before[11]);++cases; // TYPE/가상 초기화/할당을 따로 추가하지 않는다.
    }
    CHECK(cases==1036);
}
TEST_CASE(AssetConstructorCurrent_NullUnknownAndDiagnosticCallableLifetimeStayPhysical) { // 잘못된SID의 NULL/미지원은 풀을 읽지 않고 진단은 주소 조회보다 먼저 실행한다.
    SidPool pool(OriginalEdition::Patch1078,24000,false);std::vector<RiftTypeRecord> types(188);SquidFactory factory(pool,types);const Sid sid{50};auto raw=PhysicalAssetBytes(pool,sid);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa7});raw[10]=74;raw[11]=3;const auto before=std::vector<std::uint8_t>(raw.begin(),raw.end());const auto free=pool.FreeCount(); // free/dead/TYPE74도 직접 생성자 저장의 입력이다.
    CHECK(factory.ConstructByAddress(0x4429e0U,Sid{65535}) && !factory.ConstructByAddress(0,Sid{65535}) && !factory.ConstructByAddress(0x123456U,Sid{65535}));CHECK(pool.FreeCount()==free && std::equal(raw.begin(),raw.end(),before.begin()));
    CHECK(Throws([&] { static_cast<void>(factory.ConstructByAddress(kDaisConstructor,sid)); }));CHECK(std::equal(raw.begin(),raw.end(),before.begin())); // 미연결 진단 앞에서 물리 저장을 하지 않는다.
    auto keep=std::make_shared<int>(7);std::weak_ptr<int> lifetime=keep;std::uint32_t diagnoses=0; // 자기 진단 연결 해제 중 선택 callable의 소유권이다.
    factory.BindDiagnostics({{},[&,keep](auto expression,auto file,auto line) { factory.BindDiagnostics({});CHECK(!lifetime.expired() && *keep==7 && expression=="false" && file==".\\Dais.cpp" && line==0x484U);++diagnoses;raw[10]=75;raw[34]=4; },{}});keep.reset();CHECK(!lifetime.expired());CHECK(factory.ConstructByAddress(kDaisConstructor,sid));CHECK(lifetime.expired() && diagnoses==1 && Get(raw,0)==0x507950U && raw[10]==75 && raw[34]==4 && pool.FreeCount()==free);
    Put(raw,0,0x11223344U);factory.BindDiagnostics({{},[&](auto,auto,auto) { ++diagnoses;raw[34]=9;throw std::runtime_error("선택 진단 중단 입력"); },{}});CHECK(Throws([&] { static_cast<void>(factory.ConstructByAddress(kDaisConstructor,sid)); }));CHECK(diagnoses==2 && Get(raw,0)==0x11223344U && raw[34]==9 && pool.FreeCount()==free); // 중단 전 외부 쓰기는 남고 원문 다음 VT 저장은 하지 않는다.
    factory.BindDiagnostics({{},[&](auto,auto,auto) { ++diagnoses; },{}});CHECK(Throws([&] { static_cast<void>(factory.ConstructByAddress(kDaisConstructor,Sid{65535})); }));CHECK(diagnoses==3 && Get(raw,0)==0x11223344U); // 호스트 밖 SID도 진단 정상 복귀 뒤에야 주소 오류를 낸다.
}
