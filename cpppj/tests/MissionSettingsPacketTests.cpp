// 실제 설정 송신/수신/자연 observer의 전체bytes와 현재 읽기·쓰기 순서를 독립 PE와 비교한다.
#include "RawSceneSupport.h"
#include "o/RawMissionSettingsPacket.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::size_t kSettingsNativeCases=164; // 송신117/수신45/observer2의 독립 고유입력 개수다.
// 원본에 공급한 명시 현재값만 만들며 packet 기대출력을 생성하지 않는다.
std::array<std::uint32_t,7> SettingsValues(std::uint32_t profile,std::uint32_t seed) {
    std::array<std::uint32_t,7> values{};
    if (profile==1) values.fill(0xffffffffU);
    else if (profile==2) values={0,1,0x80000000U,0x7fffffffU,0xffffffffU,3,16};
    else if (profile==3) {
        // 각 현재 필드를 다른 DWORD로 구분한다.
        for (std::uint32_t index=0;index<values.size();++index) values[index]=0x12345678U+index*0x1010101U+seed;
    } else if (profile>=4) { const std::array<std::uint32_t,3> bits{1,0x80000000U,0xffffffffU};values[(profile-4)/3]=bits[(profile-4)%3]; }
    return values;
}
// 명시 효과가 먼저 읽은 값 이외의 현재 원문을 변경한다.
void SettingsChange(std::array<std::uint32_t,7>& words,std::size_t field,std::uint32_t step,std::uint32_t seed) {
    // 다음 getter/read의 현재값을 바꾸되 이미 반환한 값은 보존한다.
    for (std::uint32_t index=0;index<words.size();++index) if (index!=field) words[index]^=0x9e3779b9U+seed+step*17U+index;
}
// 원본 입력의7DWORD를 host 정렬과 무관한 little endian bytes로 표시한다.
std::string SettingsHex(const std::array<std::uint32_t,7>& words) {
    std::array<std::uint8_t,28> bytes{};
    // 각 현재 전역을payload번호순서로정렬해관찰한다.
    for (std::size_t index=0;index<words.size();++index) Put(bytes,index*4,words[index]);return Hex(bytes);
}
// 실제 분배기 경계를 관찰하며 선택되지 않은 효과는 trace가 비어야 한다.
PacketDispatchHooks SettingsDispatchHooks(std::string& trace) {
    PacketDispatchHooks hooks;hooks.now=[&] { trace+='N';return 1U; };hooks.record=[&](PacketConnection&,std::span<const std::uint8_t>,std::uint16_t) { trace+='D'; };
    hooks.terminate=[&](PacketConnection&) { trace+='T'; };hooks.close=[&](PacketConnection&) { trace+='C'; };hooks.release=[&](PacketConnection&) { trace+='F'; };return hooks;
}
}

// 독립164행의 현재 설정/packet 전체와 효과 순서를 전수 대조한다.
TEST_CASE(MissionSettingsPacket_Native164) {
    std::size_t count=0;
    // 각 행에 새로운 현재 설정과 수신 원문을 설치한다.
    for (const auto& row:LoadFixture(NETSTORM_MISSION_SETTINGS_PACKET_FIXTURE).rows) {
        if (row[0]=="input") continue;CHECK(row.size()==4);const auto fields=Split(row[0],',');CHECK(fields.size()==5);
        const auto profile=Number(fields[1]),seed=Number(fields[2]),target=Number(fields[3]),mode=Number(fields[4]);auto current=SettingsValues(profile,seed);std::array<std::uint8_t,32> packet{};
        // 수신/observer의 원문 헤더도 native의 무해석 입력을 그대로 보존한다.
        for (std::size_t index=0;index<packet.size();++index) packet[index]=static_cast<std::uint8_t>(index*37+11);
        if (fields[0]!="S") {
            // 수신의 기대결과가 아니라 원본에 전달한 현재packet입력이다.
            for (std::size_t index=0;index<current.size();++index) Put(packet,4+index*4,current[index]);
        }
        if (fields[0]=="R") {
            // 수신전 현재전역은packet과독립된stale DWORD다.
            for (std::uint32_t index=0;index<current.size();++index) current[index]=0xcccc0000U+index;
        }
        std::string trace;std::uint32_t step=0;
        const auto event=[&](std::string text) { if (!trace.empty()) trace+=';';trace+=text; }; // 실제 선택된 외부 읽기/쓰기/송신을 관찰한다.
        const auto change=[&](std::size_t field,bool receive) {
            ++step;if (mode!=2 && !(mode==1 && step==1)) return;
            if (!receive) SettingsChange(current,field,step,seed);
            else {
                std::array<std::uint32_t,7> words{};
                // callback이바꾸는나머지현재payload를원문span에서읽는다.
                for (std::size_t index=0;index<words.size();++index) words[index]=Get(packet,4+index*4);
                SettingsChange(words,field,step,seed);
                // 후속Receive는수정된같은원문에서각DWORD를다시읽어야한다.
                for (std::size_t index=0;index<words.size();++index) Put(packet,4+index*4,words[index]);
            }
        }; // 현재효과입력변경은packet기대출력작성과구분한다.
        MissionSettingsPacketHooks hooks;
        hooks.current=[&](MissionSettingsField field) { const auto index=static_cast<std::size_t>(field);const auto value=current[index];event("R,"+std::to_string(index)+","+std::to_string(value));change(index,false);return value; };
        hooks.setCurrent=[&](MissionSettingsField field,std::uint32_t value) { const auto index=static_cast<std::size_t>(field);event("W,"+std::to_string(index)+","+std::to_string(value));current[index]=value;change(index,true); };
        hooks.send=[&](std::int32_t recipient,std::span<const std::uint8_t> bytes) { event("S,"+std::to_string(std::bit_cast<std::uint32_t>(recipient)));CHECK(bytes.size()==packet.size());std::copy(bytes.begin(),bytes.end(),packet.begin()); };
        RawMissionSettingsPacket settings(OriginalEdition::Patch1078,std::move(hooks));
        if (fields[0]=="S") settings.Send(std::bit_cast<std::int32_t>(target));
        else if (fields[0]=="R") settings.Receive(packet);
        else { std::string dispatchTrace;RawPacketDispatch dispatch(OriginalEdition::Patch1078,SettingsDispatchHooks(dispatchTrace));RegisterMissionSettingsPacket(dispatch,settings);dispatch.Registration(0x32).observer(mode);CHECK(dispatchTrace.empty()); }
        if (Hex(packet)!=row[1] || SettingsHex(current)!=row[2] || (trace.empty() ? "-" : trace)!=row[3]) throw std::runtime_error("Settings native 행 "+std::to_string(count)+" 현재 "+trace+" 기대 "+row[3]);
        ++count;
    }
    CHECK(count==kSettingsNativeCases);
}

// 실제고정길이/버전/observer와같은현재설정에쓰는0x32수신을분배기에등록한다.
TEST_CASE(MissionSettingsPacket_ActualDispatchAndCurrentState) {
    auto values=SettingsValues(2,0);std::array<std::uint8_t,32> packet{};MissionSettingsPacketHooks hooks;
    hooks.current=[&](MissionSettingsField field) { return values[static_cast<std::size_t>(field)]; };
    hooks.setCurrent=[&](MissionSettingsField field,std::uint32_t value) { values[static_cast<std::size_t>(field)]=value; };
    hooks.send=[&](std::int32_t target,std::span<const std::uint8_t> bytes) { CHECK(target==9);std::copy(bytes.begin(),bytes.end(),packet.begin()); };
    RawMissionSettingsPacket settings(OriginalEdition::Patch1078,hooks);settings.Send(9);const auto first=packet;values=SettingsValues(3,7);settings.Send(9);CHECK(first!=packet);const auto expected=values;values.fill(0);
    std::string trace;RawPacketDispatch dispatch(OriginalEdition::Patch1078,SettingsDispatchHooks(trace));RegisterMissionSettingsPacket(dispatch,settings);const auto& registration=dispatch.Registration(0x32);
    CHECK(registration.length==32 && registration.version==1 && registration.mismatch==0 && bool(registration.receive) && bool(registration.observer));registration.observer(0);registration.observer(1);CHECK(trace.empty());
    PacketConnection connection;connection.sender=9;CHECK(dispatch.Dispatch(connection,packet));CHECK(values==expected && trace=="ND");CHECK(connection.received==1 && connection.depth==0);
    packet[2]=31;CHECK(!dispatch.Validate(packet));
}

// 선택된getter/store/send누락을가짜성공으로채우지않고물리부족전까지원본부분쓰기를보존한다.
TEST_CASE(MissionSettingsPacket_SelectedBoundaries) {
    MissionSettingsPacketHooks hooks;CHECK(Throws([&] { RawMissionSettingsPacket(OriginalEdition::Patch1078,hooks); }));hooks.current=[](MissionSettingsField) { return 0U; };
    CHECK(Throws([&] { RawMissionSettingsPacket(OriginalEdition::Cd1072,hooks); }));RawMissionSettingsPacket missing(OriginalEdition::Patch1078,hooks);CHECK(Throws([&] { missing.Send(9); }));
    std::array<std::uint8_t,32> packet{};CHECK(Throws([&] { missing.Receive(packet); }));std::vector<std::uint32_t> writes;
    hooks.setCurrent=[&](MissionSettingsField field,std::uint32_t) { writes.push_back(static_cast<std::uint32_t>(field)); };RawMissionSettingsPacket partial(OriginalEdition::Patch1078,hooks);
    CHECK(Throws([&] { partial.Receive(std::span(packet).first(12)); }));CHECK(writes==std::vector<std::uint32_t>({0,1}));
    hooks.send=[](std::int32_t,std::span<const std::uint8_t>) { throw std::runtime_error("검사용 실제 연결 gate 거부"); };RawMissionSettingsPacket rejected(OriginalEdition::Patch1078,hooks);CHECK(Throws([&] { rejected.Send(9); }));
}
