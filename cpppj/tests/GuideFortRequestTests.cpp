// 최초 PE의 요청144/Subscriber54입력과 State1 미션 조기 반환을 대조한다.
#include "client/MissionFortRequest.h"
#include "o/RawFortSubscriberChange.h"
#include "o/RawExistingEditorInstallOuter.h"
#include "RawSceneSupport.h"
#include <algorithm>
#include <array>
#include <stdexcept>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 원문 입력/출력을 읽고 명시 외부 효과만 공급한다.
// 평면 ASCII JSON 문자열의 인용부만 제거한다.
std::string GuideText(std::string_view value) { if (value.size()<2 || value.front()!='"' || value.back()!='"') throw std::logic_error("Guide 입력 인용부");return std::string(value.substr(1,value.size()-2)); }
// 기대값의 전체 물리 바이트를 해석한다. 제품 필드로 기대값을 재계산하지 않는다.
std::vector<std::uint8_t> GuideBytes(std::string_view text) {
    std::vector<std::uint8_t> result;
    for (std::size_t at=0;at<text.size();at+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(at,2)),nullptr,16))); // 두16진 문자마다 한BYTE다.
    return result;
}
// 검사 물리 버퍼의 첫 NUL까지 빌린다.
std::string_view GuideCString(std::span<const std::uint8_t> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::logic_error("Guide 검사 NUL 누락");return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())};
}
// 원문과 같은 현재값을 보유하지만 Template/Config/제목은 명시 경계다.
struct GuideDevices {
    std::uint32_t count{},my{0xffffffffU},owner{1},fort{},port{},gate{},variant{}; // 원문 전역 및 선택된 외부 변이 입력이다.
    std::array<std::uint8_t,64> handle{}; // 문자열 뒤 stale 바이트도 최초 기대값과 대조한다.
    std::vector<std::uint8_t> address,before; //9BYTE 주소와 전체 요청/양옆160BYTE 입력이다.
    netstorm::o::LocalMissionStateBlock next{}; // 실제144BYTE와 별도 주변8BYTE씩을 유지한다.
    MissionFortLoadRequest image; // observer 뒤 실제 복제 바이트를 소유한다.
    std::string trace,packet; // 외부 순서/전체512BYTE 출력만 기록한다.
    // 변경된 이름 뒤의 기존 바이트를 지우지 않는다.
    void SetHandle(std::string_view text) { std::copy(text.begin(),text.end(),handle.begin());handle[text.size()]=0; }
    // 원문의 현재값 재조회 시험만 수행하고 게임 하위 결과를 대신 계산하지 않는다.
    netstorm::o::FortSubscriberChangeHooks Hooks() {
        netstorm::o::FortSubscriberChangeHooks hooks;hooks.owner=[this] { return owner; };hooks.manager=[this](auto slot) { return gate==0 ? 0U : 0x12340000U+slot; };
        hooks.fortMode=[this] { return fort; };hooks.handle=[this] { return std::span(handle); };
        hooks.configure=[this](auto key,auto value) { CHECK(key=="fort.mySubHandle");trace="Config,"+Hex({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()});if (variant==1) { owner=8;my=91;fort=0;SetHandle("AfterConfig");const auto changed=GuideBytes("414243444546474849");std::copy(changed.begin(),changed.end(),address.begin());port=12345;count=0xfffffffe; } };
        hooks.title=[this] { trace+=";Title"; };
        hooks.subscriber=[this](auto slot,auto id,auto bytes) { if (!trace.empty()) trace+=';';const auto name=GuideCString(bytes);trace+="Subscriber,"+std::to_string(slot)+','+std::to_string(id)+','+Hex({reinterpret_cast<const std::uint8_t*>(name.data()),name.size()});if (variant==2) { owner=7;my=0xabababab;SetHandle("AfterSubscriber"); } };
        hooks.notify=[this](auto slot) { trace+=";Notify,"+std::to_string(slot); };return hooks;
    }
    // 모든 비교 입력은 최초 fixture에서 읽으며 제품의 초기화 기본값에 의존하지 않는다.
    void Initialize(const std::vector<std::string>& row) {
        const auto fields=Split(std::string_view(row[1]).substr(1,row[1].size()-2),',');CHECK(fields.size()==9);if (fields.size()!=9) throw std::logic_error("Guide 원문 입력 폭");
        address=GuideBytes(GuideText(fields[1]));port=Number(fields[2]);count=Number(fields[3]);gate=Number(fields[4]);variant=Number(fields[5]);fort=gate!=1;handle.fill(0xa5);SetHandle(GuideText(fields[6]));before=GuideBytes(row[2]);
        for (std::size_t index=0;index<next.size();++index) next[index]=Get(before,8+index*4); // 전체 staleDWORD를 보존한다.
    }
    // 요청/Subscriber의 출력과 부분 변경 전역을 같은 최초 원문 행에 대조한다.
    void Check(const std::vector<std::string>& row) {
        auto after=before;for (std::size_t index=0;index<next.size();++index) Put(after,8+index*4,next[index]); // 양쪽 이웃을 포함해 비교한다.
        CHECK(Hex(after)==row[3]);CHECK(std::to_string(count)+','+std::to_string(my)+','+std::to_string(owner)+','+std::to_string(fort)+','+std::to_string(port)==row[4]);CHECK(Hex(handle)==row[5]);CHECK((trace.empty()?"-":trace)==row[8]);
    }
};
}
// 살아 있는 Guide 요청은 일반 Fort 재선택/네 이름 작성 없이 현재 한 슬롯과 전체 물리를 보존한다.
TEST_CASE(GuideFortRequest_Native144) {
    const auto rows=LoadFixture(NETSTORM_GUIDE_FORT_REQUEST_FIXTURE).rows;std::size_t count{};
    for (const auto& row:rows) { // 실제 요청144개만 선택하며 Subscriber 별도54개와 합산해 중복 실행하지 않는다.
        if (row[0]!="R") continue;++count;GuideDevices devices;devices.Initialize(row);auto effects=devices.Hooks();GuideFortRequestHooks hooks;
        hooks.subscriber=[&](auto id,auto name) { netstorm::o::ChangeFortSubscriber(id,std::span(reinterpret_cast<const std::uint8_t*>(name.data()),name.size()+1),devices.my,effects); };
        hooks.handle=[&] { return GuideCString(devices.handle); };hooks.duplicate=[&](auto& packet) { if (!devices.trace.empty()) devices.trace+=';';devices.trace+="Allocate,86";if (devices.variant==3) { devices.count=0xffffffff;devices.port=9876;devices.next[0x60/4]=0x76543210; }CHECK(CaptureMissionFortZone(devices.image,packet)==1);devices.packet=Hex(packet.storage);return 0x12200000U; };
        WriteCurrentGuideFortRequest({devices.next,devices.count,devices.address,devices.port},hooks);devices.Check(row);CHECK(devices.packet==row[6] && Hex(devices.image.zone)==row[7]);
    }CHECK(count==144 && rows.size()==198);
}
// 이름 부재와 두 자연 관문도 원문 전체54입력으로 확인한다.
TEST_CASE(FortSubscriberChange_Native54) {
    const auto rows=LoadFixture(NETSTORM_GUIDE_FORT_REQUEST_FIXTURE).rows;std::size_t count{};const std::array<std::uint8_t,9> name{'S','e','l','e','c','t','e','d',0}; // 실제 원문 입력의 고정 문자열이다.
    for (const auto& row:rows) { // Subscriber 선택만 실행하며 요청/World 설치를 앞당기지 않는다.
        if (row[0]!="S") continue;++count;GuideDevices devices;devices.Initialize(row);const auto fields=Split(std::string_view(row[1]).substr(1,row[1].size()-2),',');
        netstorm::o::ChangeFortSubscriber(Number(fields[7]),fields[8]=="true" ? std::optional<std::span<const std::uint8_t>>(name) : std::nullopt,devices.my,devices.Hooks());devices.Check(row);
    }CHECK(count==54);
}
// 설정/실제 writer가 중단한 지점까지의 이름/My 쓰기를 보존하고 뒷 통지를 호출하지 않는다.
TEST_CASE(FortSubscriberChange_PartialEffectsAndShortCircuit) {
    GuideDevices devices;devices.gate=2;devices.fort=1;devices.SetHandle("Old");const std::array<std::uint8_t,4> name{'N','e','w',0};auto hooks=devices.Hooks();std::size_t titles{},writes{},notifies{};
    hooks.configure=[](auto,auto) { throw std::runtime_error("config-stop"); };hooks.title=[&] { ++titles; };hooks.subscriber=[&](auto,auto,auto) { ++writes; };hooks.notify=[&](auto) { ++notifies; };
    bool failed{};try { netstorm::o::ChangeFortSubscriber(1,name,devices.my,hooks); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && GuideCString(devices.handle)=="New" && devices.my==0xffffffffU && !titles && !writes && !notifies);
    hooks.configure=[](auto,auto) {};hooks.subscriber=[&](auto,auto,auto) { ++writes;throw std::runtime_error("writer-stop"); };failed=false;try { netstorm::o::ChangeFortSubscriber(7,std::nullopt,devices.my,hooks); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && devices.my==7 && writes==1 && !titles && !notifies);
    hooks.manager=[](auto) { return 0U; };hooks.fortMode=[]()->std::uint32_t { throw std::logic_error("선택되지 않아야 하는 fortMode"); };netstorm::o::ChangeFortSubscriber(2,name,devices.my,hooks);CHECK(devices.my==7 && writes==1);
}
// 파일의 실제 앞선 효과는 남기고 미션 요청 뒤 편집 World/Deck/HUD/State 꼬리에 진입하지 않는다.
TEST_CASE(ExistingEditorFort_MissionReturnWithoutEditorCheckpoint) {
    netstorm::o::ExistingEditorFortHooks hooks;std::string trace;hooks.currentName=[] { return std::string_view("Same"); };hooks.pendingName=hooks.currentName;hooks.compare=[](auto,auto) { return 0; };hooks.inMission=[] { return 1U; };
    hooks.file=[&] { trace="File"; };hooks.requestMission=[&] { trace+=";Mission7"; };hooks.world=[] { throw std::logic_error("선택되지 않아야 하는 Editor World"); };CHECK(!netstorm::o::InstallExistingEditorFort(hooks) && trace=="File;Mission7");
    hooks.requestMission=[&] { trace+=";Stop";throw std::runtime_error("request-stop"); };bool failed{};try { static_cast<void>(netstorm::o::InstallExistingEditorFort(hooks)); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && trace=="File;Stop");
}
