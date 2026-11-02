// 원문41baa0의실제기대값과명시물리경계를동일한FILE/키상태에서검사한다.
#include "RawSceneSupport.h"
#include "o/RawBaseFileWholeRead.h"
#include <bit>

namespace { // Native 입력/기대는분리하고OS효과는검사 장치로만제공한다.
using namespace netstorm::o;using namespace netstorm::test::rawscene; // 실제 raw 모듈과기존fixture BYTE해석을사용한다.
constexpr std::uintptr_t kWholeFile=0x12200000,kWholeKey=0x12300100,kWholeAllocation=0x12302000; // Native 격리FILE/키/할당과같은원문DWORD다.
constexpr std::uint32_t kPoison=0x65656565,kLengthSentinel=0xdeadbeef; // 입력미작성필드와선택출력값의명시seed이며원문미정값자체일치주장이아니다.
struct WholeBoundaryFailure { std::string kind; /* 실제 도달한 Open/new/fread/fclose 장치의 실패 종류다. */ }; // Native의 명시 예외경계를 구별한다.
std::string WholeFields(const RawBaseFileState& state) { // 여섯필드의원문DWORD비트를포인터폭과구별해직렬화한다.
    const std::array<std::uintptr_t,6> values{{state.file,state.flags,static_cast<std::uint32_t>(state.offset),static_cast<std::uint32_t>(state.length),state.key,state.keyLength}};std::string result;
    for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); } // Native와같은필드순서를유지한다.
    return result;
}
std::string WholeTextHex(std::span<const std::uint8_t> bytes) { return Hex(bytes.first(static_cast<std::size_t>(std::find(bytes.begin(),bytes.end(),std::uint8_t{})-bytes.begin()))); } // callback직전에현재NUL까지만읽는다.
std::string WholeEvents(const std::vector<std::string>& events) { //장치 인자를 Native 원래 순서로 기록한다.
    std::string result;
    // 도달한 각 효과를 같은 순서로 연결한다.
    for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; }
    return result;
}
}

TEST_CASE(BaseFileWholeRead_Native133ProbeAllocationReadOutputAndCompletedCtorLifetime) { // 실제funclet까지대조한전체행을원문출력과비교한다.
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_BASEFILE_WHOLE_READ_FIXTURE).rows) { // 입력9열만장치로공급하고마지막6열은기대값으로사용한다.
        CHECK(row.size()==15);const auto length=Number(row[0]);const auto extra=static_cast<std::int32_t>(std::stoll(row[1]));const auto key=Number(row[2]),subfile=Number(row[3]),hasOutput=Number(row[4]),check=Number(row[5]),probe=Number(row[6]),scenario=Number(row[8]);auto available=Number(row[7]);std::uint32_t used{},position{};std::vector<std::string> events;std::vector<std::uint8_t> allocation;std::array<std::uint8_t,4096> scratch;scratch.fill(101);std::array<std::uint8_t,256> name{};const std::string initialName="sample.bin";std::copy(initialName.begin(),initialName.end(),name.begin());
        RawBaseFileState state{kPoison,kPoison,static_cast<std::int32_t>(kPoison),static_cast<std::int32_t>(kPoison),kPoison,kPoison};RawBaseFileIOHooks io;BaseFileWholeReadHooks hooks;std::int32_t output=std::bit_cast<std::int32_t>(kLengthSentinel); // 원문지역File와출력변수의미작성초기물리다.
        hooks.probe=[&](auto physical) { events.push_back("P:"+WholeTextHex(physical));if (scenario==6) { name.fill(0);const std::string changed="changed.bin";std::copy(changed.begin(),changed.end(),name.begin()); }return probe; };
        hooks.open=[&](auto& file,auto physical,auto mode,auto create) { events.push_back("O:"+WholeTextHex(physical)+":"+std::to_string(mode)+":"+std::to_string(create)+":"+WholeFields(file));file={scenario==2?0:kWholeFile,0x80U|subfile,subfile?37:0,subfile?static_cast<std::int32_t>(length):0,key?kWholeKey:0,key?3U:0U};position=subfile?37U:0U;if (scenario==1) throw WholeBoundaryFailure{"open"}; };
        hooks.allocate=[&](auto requested) { events.push_back("N:"+std::to_string(requested));if (scenario==3) throw WholeBoundaryFailure{"allocation"};allocation.assign(requested,205);return BaseFileWholeAllocation{scenario==7?0:kWholeAllocation,allocation}; };
        io.tell=[&](auto token) { events.push_back("T:"+std::to_string(token));return static_cast<std::int32_t>(position); };io.seek=[&](auto token,auto offset,auto origin) { events.push_back("S:"+std::to_string(token)+":"+std::to_string(static_cast<std::uint32_t>(offset))+":"+std::to_string(origin));position=origin==2?length:static_cast<std::uint32_t>(offset); };
        io.read=[&](auto token,auto destination,auto size,auto elements) { const auto actual=std::min(size*elements,available);events.push_back("R:"+std::to_string(token)+":"+std::to_string(size)+":"+std::to_string(elements)+":"+std::to_string(actual));if (scenario==4) throw WholeBoundaryFailure{"read"};for (std::uint32_t index=0;index<actual;++index) destination[index]=static_cast<std::uint8_t>((used+index)*29+7);used+=actual;available-=actual;position+=actual; }; // 실제fread가작성하지않은BYTE와이전scratch를유지한다.
        const std::array<std::uint8_t,3> keyBytes{{'a','b','c'}};io.key=[&](auto token,auto size) { CHECK(token==kWholeKey && size==3);return std::span<const std::uint8_t>(keyBytes); };io.close=[&](auto token) { events.push_back("C:"+std::to_string(token));if (scenario==5) throw WholeBoundaryFailure{"close"}; };io.freeKey=[&](auto token) { events.push_back("F:"+std::to_string(token)); };
        std::uintptr_t result{};std::string outcome="ok";try { result=ReadWholeBaseFile(state,name,check,hasOutput?&output:nullptr,extra,io,hooks,scratch); }catch (const WholeBoundaryFailure& failure) { outcome=failure.kind; }
        CHECK(outcome==row[9] && result==Number(row[10]) && static_cast<std::uint32_t>(output)==Number(row[11]) && WholeFields(state)==row[12] && WholeEvents(events)==row[13] && (allocation.empty()?"-":Hex(allocation))==row[14]);++count;
    }
    CHECK(count==133);
}

TEST_CASE(BaseFileWholeRead_UnsafeNegativeLengthOrSmallAllocationIsExplicitHostBoundary) { // 원문할당밖쓰기를정상BYTE로합성하지않고완료File만반납한다.
    for (const auto length:{-1,5}) { // 음수freadDWORD와extra=-1로부족해진할당을별도로구별한다.
        RawBaseFileState state{};RawBaseFileIOHooks io;BaseFileWholeReadHooks hooks;std::uint32_t closes{},reads{},requested{};std::vector<std::uint8_t> bytes;std::int32_t position{},output=99;hooks.open=[&](auto& file,auto,auto,auto) { file.file=kWholeFile; };hooks.allocate=[&](auto size) { requested=size;bytes.resize(size);return BaseFileWholeAllocation{kWholeAllocation,bytes}; };io.tell=[&](auto) { return position; };io.seek=[&](auto,auto offset,auto origin) { position=origin==2?length:offset; };io.close=[&](auto) { ++closes; };io.read=[&](auto,auto,auto,auto) { ++reads; };const std::array<std::uint8_t,2> name{{'x',0}};bool rejected{};
        try { static_cast<void>(ReadWholeBaseFile(state,name,0,&output,length<0?0:-1,io,hooks)); }catch (const std::out_of_range&) { rejected=true; }
        CHECK(rejected && requested==4 && closes==1 && reads==0 && state.file==0 && output==99);
    }
}
