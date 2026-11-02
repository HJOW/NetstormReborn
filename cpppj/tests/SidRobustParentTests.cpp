// SID진단두부모의실제원문결과와현재호스트Pool의물리/세대없는번호계약을대조한다.
#include "RawSceneSupport.h"
#include "o/RawSidRobust.h"
#include "o/SidPool.h"
#include <array>

namespace { // 일반raw필드만만들며생산/공격/포획/희생/승리와공중3종동작은실행하지않는다.
using namespace netstorm::o;using namespace netstorm::test::rawscene; // 원문모듈과기존fixture표준읽기만사용한다.
constexpr std::uint32_t kOldRobust=0xaaaa5555,kOldType=0x55bbdd66,kPool=0x14010000; // Native입력과같은최근진단seed/합성Pool토큰이다.
struct AssertionFailure {}; // 진단callee의명시예외입력이며실제원문UI/SEH를대신하지않는다.
std::string SidState(const RawSidRobustDiagnostics& state) { return std::to_string(state.lastRobust)+":"+std::to_string(state.type); } // 각선택읽기직전두현재DWORD를보관한다.
std::string Trace(const std::vector<std::string>& events) { // Native읽기/진단의원래순서를직렬화한다.
    std::string result;for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; } // 미선택장치는기록을만들지않는다.
    return result.empty()?"-":result;
}
std::uintptr_t Pointer(std::span<const std::uint8_t> bytes) { return reinterpret_cast<std::uintptr_t>(bytes.data()); } // 실제호스트주소를DWORD로절삭하지않는다.
SidRobustHooks Host(SidPool& pool,std::uint32_t& checking,std::uint32_t& assertions) { // 장기Pool의현재물리/현재checking을원문각선택시점에다시빌린다.
    SidRobustHooks hooks;hooks.addressMode=SidRobustAddressMode::HostPointer;hooks.poolBase=[&pool] { return Pointer(pool.Bytes()); };hooks.capacity=[&pool] { return pool.Capacity(); };hooks.checking=[&checking] { return checking; };
    hooks.memory=[&pool](auto token,auto width) { const auto raw=pool.Bytes();const auto base=Pointer(raw);if (token<base || token-base>raw.size() || width>raw.size()-static_cast<std::size_t>(token-base)) throw std::out_of_range("SID 검사 현재 Pool 물리");return raw.subspan(static_cast<std::size_t>(token-base),width); };
    hooks.assertion=[&assertions](auto expression,auto line) { CHECK(expression=="resolve( sid ).isRobust()" && line==0x218);++assertions; };return hooks;
}
}

TEST_CASE(SidRobustParent_Native1012BothBodiesCurrentReadsAndAssertionReturn) { // 두부모/모든실패bit/번호경계/선택된하위·32주소감김을독립fixture와대조한다.
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_SID_ROBUST_PARENT_FIXTURE).rows) { // 입력10열은장치로만쓰고마지막8열은원문기대값이다.
        CHECK(row.size()==18);const auto input=std::stoll(row[1]);const auto live=static_cast<std::int32_t>(std::stoll(row[2]));auto checking=Number(row[3]),base=Number(row[4]),capacity=Number(row[5]);const auto table=Number(row[6]),flags=Number(row[7]),kind=Number(row[8]),mode=Number(row[9]);const auto pointer=row[0]=="object"?static_cast<std::uint32_t>(input):base+static_cast<std::uint32_t>(input)*std::uint32_t{50};
        std::array<std::uint8_t,50> raw{};Put(raw,0,table);raw[10]=static_cast<std::uint8_t>(kind);raw[11]=static_cast<std::uint8_t>(flags);RawSidRobustDiagnostics diagnostics{kOldRobust,kOldType};std::vector<std::string> events;SidRobustHooks hooks;
        hooks.poolBase=[&] { events.push_back("P:"+std::to_string(base)+":"+SidState(diagnostics));return static_cast<std::uintptr_t>(base); };hooks.capacity=[&] { events.push_back("C:"+std::to_string(capacity)+":"+SidState(diagnostics));return capacity; };hooks.checking=[&] { events.push_back("K:"+std::to_string(checking)+":"+SidState(diagnostics));return checking; };
        hooks.memory=[&](auto token,auto width)->std::span<const std::uint8_t> { events.push_back("M:"+std::to_string(token)+":"+std::to_string(width)+":"+SidState(diagnostics));if (token<pointer || token-pointer>raw.size() || width>raw.size()-static_cast<std::size_t>(token-pointer)) throw std::out_of_range("SID Native 입력 물리");return std::span<const std::uint8_t>(raw).subspan(static_cast<std::size_t>(token-pointer),width); };
        hooks.assertion=[&](auto expression,auto line) { CHECK(expression=="resolve( sid ).isRobust()" && line==0x218);events.push_back("A:"+std::to_string(line)+":"+SidState(diagnostics));if (mode==1) throw AssertionFailure{};if (mode==2) { diagnostics.lastRobust=0x11223344;diagnostics.type=0x55667788;checking=0;base=kPool+50;capacity=7; } }; // 진단재진입이바꾼현재전역을복귀뒤되돌리지않는다.
        bool result{};std::string outcome="ok";try { result=row[0]=="object"?RawSquidIsRobust(pointer,live,diagnostics,hooks):RawIsSidRobust(static_cast<std::int32_t>(input),live,diagnostics,hooks); }catch (const AssertionFailure&) { outcome="assertion"; }
        CHECK(outcome==row[10] && result==static_cast<bool>(Number(row[11])) && diagnostics.lastRobust==Number(row[12]) && diagnostics.type==Number(row[13]));CHECK(checking==Number(row[14]) && base==Number(row[15]) && capacity==Number(row[16]) && Trace(events)==row[17]);++count;
    }
    CHECK(count==1012);
}

TEST_CASE(SidRobustParent_UnselectedNumberLiveAndCheckingNeverDemandPool) { // 1..65534외에는0이며live0/checking0은Pool범위·진단초기화를추가하지않는다.
    RawSidRobustDiagnostics diagnostics{kOldRobust,kOldType};SidRobustHooks hooks;CHECK(!RawIsSidRobust(-1,1,diagnostics,hooks));CHECK(!RawIsSidRobust(0,0,diagnostics,hooks));CHECK(!RawIsSidRobust(65535,1,diagnostics,hooks));CHECK(!RawIsSidRobust(65536,0,diagnostics,hooks));CHECK(RawIsSidRobust(1,0,diagnostics,hooks));CHECK(RawIsSidRobust(65534,0,diagnostics,hooks));hooks.checking=[] { return 0U; };CHECK(RawIsSidRobust(65534,1,diagnostics,hooks));CHECK(diagnostics.lastRobust==kOldRobust && diagnostics.type==kOldType);
}

TEST_CASE(SidRobustParent_CurrentHostPoolPointerResetAndNoGenerationBits) { // 같은숫자SID가재사용되면현재슬롯을읽으며옛포인터/별도세대를보관하지않는다.
    SidPool pool(OriginalEdition::Patch1078,24000);const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);Put(raw,0,0x12345678);raw[10]=7;std::uint32_t checking=1,assertions{};auto hooks=Host(pool,checking,assertions);RawSidRobustDiagnostics diagnostics;
    CHECK(RawIsSidRobust(sid.value,1,diagnostics,hooks) && diagnostics.lastRobust==0 && diagnostics.type==7 && assertions==0); // 원문은void비트4에별도거부관문을두지않는다.
    raw[11]=3;CHECK(RawIsSidRobust(sid.value,1,diagnostics,hooks) && diagnostics.lastRobust==12 && diagnostics.type==7 && assertions==1); // free/dead상세실패라도진단이복귀하면부모는1이다.
    pool.Reset(24500);CHECK(RawIsSidRobust(sid.value,1,diagnostics,hooks) && diagnostics.lastRobust==22 && diagnostics.type==0 && assertions==2);const auto reused=pool.Allocate(2);CHECK(reused==sid);raw=pool.AllocatedBytes(reused);Put(raw,0,0x12345678);raw[10]=15;CHECK(RawIsSidRobust(sid.value,1,diagnostics,hooks) && diagnostics.lastRobust==0 && diagnostics.type==15 && assertions==2);
    checking=0;CHECK(RawIsSidRobust(65534,1,diagnostics,hooks) && diagnostics.lastRobust==0 && diagnostics.type==15 && assertions==2); // 현재Pool상한보다커도미선택상세검사를합성하지않는다.
}

TEST_CASE(SidRobustParent_TypeReadPrecedesLogicalRangeAndMissingPhysicalStopsThere) { // 원문의잘못된물리읽기를정상값으로대신하지않고선행진단쓰기만보존한다.
    RawSidRobustDiagnostics diagnostics{kOldRobust,kOldType};SidRobustHooks hooks;hooks.memory=[](auto,auto) { return std::span<const std::uint8_t>{}; };bool rejected{};try { static_cast<void>(RawSquidIsRobust(1000,1,diagnostics,hooks)); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected && diagnostics.lastRobust==0 && diagnostics.type==kOldType); // poolBase/capacity가미연결이어도먼저부족물리에서끝난다.
    const std::array<std::uint8_t,1> type{{255}};hooks.memory=[&](auto token,auto width) { CHECK(token==1010 && width==1);return std::span<const std::uint8_t>(type); };hooks.poolBase=[] { return std::uintptr_t{1000}; };CHECK(!RawSquidIsRobust(1000,1,diagnostics,hooks));CHECK(diagnostics.lastRobust==1 && diagnostics.type==255); // 하한실패는capacity조차요구하지않고type현재값은이미쓴다.
}
