// 실제 원본 신전 분배/윈드 타워 기하432입력의 반환과 효과 순서를 전부 대조한다.
#include "RawSceneSupport.h"
#include "o/RawSpecialDamage.h"
#include "o/SquidFactory.h"
#include <bit>
#include <cstdio>
#include <filesystem>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

// 전체 기대값은 새 C++ 전에 동결한 보호 원본에서 읽는다.
TEST_CASE(SpecialDamage_Native432) {
    SidPool pool(OriginalEdition::Patch1078,24000);std::vector<RiftTypeRecord> types(188);SquidFactory factory(pool,types); // 명시 raw 입력용 실제 단일 저장소다.
    const Sid victim{15000},attacker{15001};CHECK(factory.Take(86,victim)==victim && factory.Take(86,attacker)==attacker);
    std::size_t count{}; // 전체 독립 원본 행 수다.
    // 실제 입력마다 같은 raw 상태를 다시 설치하고 실제 반환/순서를 대조한다.
    for (const auto& row:LoadFixture(NETSTORM_SPECIAL_DAMAGE_FIXTURE).rows) {
        CHECK(row.size()==3);if (row.size()!=3) return;const auto parts=Split(row[0],',');std::vector<std::uint32_t> input; // 현재 동결 입력의 정수 비트다.
        // 문자열은 부호 확장 없이 원본 DWORD로 읽는다.
        for (const auto& value:parts) input.push_back(Number(value));
        auto target=pool.AllocatedBytes(victim),source=pool.AllocatedBytes(attacker);std::fill(target.begin(),target.end(),std::uint8_t{});std::fill(source.begin(),source.end(),std::uint8_t{});
        target[10]=142;source[10]=128;Put(target,14,std::bit_cast<std::uint32_t>(50.0f));Put(target,18,std::bit_cast<std::uint32_t>(50.0f));types[142].footX=types[142].footY=2;
        std::vector<std::string> events;std::uint32_t actual{}; // 실제 경계 호출과 반환만 모으며 기대 결과를 계산하지 않는다.
        if (input[0]==0) {
            target[40]=static_cast<std::uint8_t>(input[1]);VortexDamageHooks hooks; // 원본의 명시 효과 경계와 같은 입력/반환이다.
            hooks.constructionType=[] { return 30U; };
            hooks.find=[&](Sid sid,std::uint32_t type,std::uint32_t index,bool remove) { events.push_back("F:"+std::to_string(sid.value)+','+std::to_string(type)+','+std::to_string(index)+','+std::to_string(remove));return Sid{static_cast<std::uint16_t>(input[2])}; };
            hooks.resetEnd=[&](Sid sid,std::uint32_t amount) { events.push_back("C:"+std::to_string(sid.value)+','+std::to_string(amount)); };
            hooks.refresh=[&](Sid sid) { events.push_back("R:"+std::to_string(sid.value)); };
            hooks.general=[&](Sid sid,std::int32_t amount,std::uint32_t owner,Sid firing) { events.push_back("H:"+std::to_string(sid.value)+','+std::to_string(std::bit_cast<std::uint32_t>(amount))+','+std::to_string(owner)+','+std::to_string(firing.value));return input[5]!=0; };
            actual=RawVortexDamage(pool,victim,std::bit_cast<std::int32_t>(input[3]),input[4],attacker,hooks);
        } else {
            source[10]=static_cast<std::uint8_t>(input[2]);Put(source,14,input[4]);Put(source,18,input[5]);types[input[2]].flags2=input[1];types[input[2]].footX=static_cast<int>(input[6]);types[input[2]].footY=static_cast<int>(input[7]);
            std::vector<RiftTypeFrames> frames;frames.reserve(188); // 현재 원본 코드 표의 방어 프레임 입력이다.
            // 타입별 표는 같은 개수를 유지하고 실제 방어자의 첫side BYTE만 설치한다.
            for (std::size_t type=0;type<188;++type) frames.emplace_back(std::vector<FrameCode>{{static_cast<std::uint8_t>(type==142 ? input[3] : 0),0,0,0}});
            actual=RawWindTowerDefense(pool,types,frames,victim,attacker);
        }
        std::string observed; // 원본 사건 구분자와 같은 순서 문자열이다.
        // 반환 이후에도 각 경계의 중복 호출과 누락을 그대로 검출한다.
        for (const auto& event:events) { if (!observed.empty()) observed+=';';observed+=event; }
        if (actual!=Number(row[1]) || observed!=row[2]) std::printf("특수 피해 차이 %s 반환%u/%s 사건%s/%s\n",row[0].c_str(),actual,row[1].c_str(),observed.c_str(),row[2].c_str());
        CHECK(actual==Number(row[1]) && observed==row[2]);++count;
    }
    CHECK(count==432);
}

// 예약 피해의 공격자0도 원본처럼 현재 풀 슬롯을 읽으며 기존432행 기대값은 보존한다.
TEST_CASE(SpecialDamage_WindTowerZeroAttackerNative216) {
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/wind-tower-zero-x86.tsv").string();std::size_t count{};
    for (const auto& row:LoadFixture(path.c_str()).rows) { // 새로 관측한0슬롯의 타입/기하/반환을 그대로 대조한다.
        CHECK(row.size()==3);if (row.size()!=3) continue;std::vector<std::uint32_t> a;for (const auto& part:Split(row[0],',')) a.push_back(Number(part)); // 동결 DWORD 입력이다.
        SidPool pool(OriginalEdition::Patch1078,24000);std::vector<RiftTypeRecord> types(188);const Sid victim{15000};
        const auto writable=[&](Sid sid) { const auto raw=pool.Slot(sid);return std::span<std::uint8_t>(const_cast<std::uint8_t*>(raw.data()),50); }; // 0슬롯을 포함한 명시 원문 입력이다.
        auto target=writable(victim),source=writable(Sid{});std::fill(target.begin(),target.end(),std::uint8_t{});std::fill(source.begin(),source.end(),std::uint8_t{});target[10]=142;source[10]=static_cast<std::uint8_t>(a[2]);Put(target,14,std::bit_cast<std::uint32_t>(50.F));Put(target,18,std::bit_cast<std::uint32_t>(50.F));Put(source,14,a[4]);Put(source,18,a[5]);types[142].footX=types[142].footY=2;types[a[2]].flags2=a[1];types[a[2]].footX=static_cast<std::int32_t>(a[6]);types[a[2]].footY=static_cast<std::int32_t>(a[7]);
        std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));frames[142]=RiftTypeFrames({{static_cast<std::uint8_t>(a[3]),0,0,0}});const auto actual=RawWindTowerDefense(pool,types,frames,victim,Sid{});
        CHECK(actual==(Number(row[1])!=0) && row[2].empty());++count;
    }
    CHECK(count==216);
}
