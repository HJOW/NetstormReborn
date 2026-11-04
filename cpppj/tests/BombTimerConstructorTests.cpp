// timer57 기본 cdecl의 초기화 필드와 미작성 flags를 독립 Native 관찰로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawBombEffectTimer.h"
#include <filesystem>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 실제 원문 바이트만 읽으며 객체 생성 기대값은 계산하지 않는다.
std::vector<std::uint8_t> TimerCtorBytes(const std::string& hex) { // 전후 원문40BYTE를 little endian 그대로 푼다.
    std::vector<std::uint8_t> result; // 원문 바이트 순서다.
    for (std::size_t index=0;index<hex.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(index,2),nullptr,16))); // 두 문자마다 한BYTE를 읽는다.
    return result;
}
}
TEST_CASE(BombTimerConstructorCurrent_Native12EmptyListZeroDeadlineAndUnwrittenFlags) { // 현재 시각0/100.25/NaN에서도 생성자가 Init을 실행하지 않음을 대조한다.
    const auto fixture=LoadFixture((std::filesystem::path(__FILE__).parent_path()/"fixtures/bomb-timer-constructor-x86.tsv").string().c_str());std::size_t count=0; // 실제 입력 재생 수다.
    for (const auto& row:fixture.rows) { CHECK(row.size()==5);if (row.size()!=5) return;const auto native=TimerCtorBytes(row[3]);CHECK(native.size()==40);if (native.size()!=40) return;const auto fill=static_cast<std::uint32_t>(std::stoul(row[0]));const auto inputFlags=fill*0x01010101U;double now=std::bit_cast<double>(static_cast<std::uint64_t>(std::stoull(row[2])));SidPool pool(OriginalEdition::Patch1078,24000);std::vector<RiftTypeRecord> types(188);const auto free=pool.FreeCount();BombEffectTimerProcess process(pool,types,now,BombEffectTimerHooks{},inputFlags);const auto& fields=process.Fields(); // flags 입력은 Native의 미작성 heap이며 기대 출력에서 가져오지 않는다.
        CHECK(fields.members.empty() && fields.members.size()==Get(native,0x14) && fields.members.capacity()>=100 && fields.capacity==Get(native,0x10) && fields.flags==Get(native,0x20) && process.Form().value==Get(native,4) && process.Parent().value==Get(native,8) && pool.FreeCount()==free);CHECK(std::bit_cast<std::uint64_t>(fields.endTime)==(static_cast<std::uint64_t>(Get(native,0x1c))<<32|Get(native,0x18)));const auto members=TimerCtorBytes(row[4]);CHECK(members.size()==400 && std::all_of(members.begin(),members.end(),[](auto byte) { return byte==0xd7; })); // 전체 미사용 목록은 Native 도구가 쓰기 금지와400BYTE 보존을 별도로 확인한다.
        now=99999.5;CHECK(fields.endTime==0 && fields.flags==inputFlags);++count; // 생성 후 현재 시각을 바꿔도 기한을 뒤늦게 초기화하지 않는다.
    }
    CHECK(count==12);
}
