// 원본 요청 전체144BYTE·Zone512/복제86BYTE와 실제 소유권을 검사한다. 실행은 후속 통합 단계다.
#include "client/MissionFortRequest.h"
#include "RawSceneSupport.h"
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// 독립 원문의 소문자16진 바이트를 입력/기대 영역으로 읽는다.
std::vector<std::uint8_t> RequestBytes(std::string_view text) {
    std::vector<std::uint8_t> result;
    // 한 바이트마다 두 자리만 소비하며 제품의 요청 필드 판단을 계산하지 않는다.
    for (std::size_t at=0;at<text.size();at+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(at,2)),nullptr,16)));return result;
}
// 이 원문의 평면 ASCII JSON 문자열에는 escape/쉼표가 없으며 양 끝 인용부만 제거한다.
std::string RequestText(const std::string& text) { if (text.size()<2 || text.front()!='"' || text.back()!='"') throw std::logic_error("Fort 원문 입력 인용부 오류");return text.substr(1,text.size()-2); }
}
// 네 슬롯/전체 요청·이웃/예약·복제·호출 순서·감김과 효과 뒤 재조회를 최초144행에 대조한다.
TEST_CASE(MissionFortRequest_Native144) {
    const auto rows=LoadFixture(NETSTORM_MISSION_FORT_REQUEST_FIXTURE).rows;CHECK(rows.size()==144);
    // 각 행은 새 물리 블록/현재 외부 전역으로 원본과 같은 입력을 받는다.
    for (const auto& row:rows) {
        CHECK(row.size()==7);if (row.size()!=7) return;const auto input=Split(std::string_view(row[0]).substr(1,row[0].size()-2),',');CHECK(input.size()==6);if (input.size()!=6) return;
        auto address=RequestBytes(RequestText(input[1]));auto port=Number(input[2]),count=Number(input[3]);const auto variant=Number(input[5]);auto result=RequestBytes(row[1]);netstorm::o::LocalMissionStateBlock next{};MissionFortLoadRequest image;std::string trace,storage; // 최초 원문의 현재 입력과 별개 출력 저장소다.
        // 원문144BYTE를DWORD 저장소로 읽으며 양쪽 이웃8BYTE는 그대로 둔다.
        for (std::size_t index=0;index<next.size();++index) next[index]=Get(result,8+index*4);
        MissionFortRequestHooks effects;effects.selectFort=[&](auto name) { trace="Fort,"+Hex({reinterpret_cast<const std::uint8_t*>(name.data()),name.size()});if (variant==1) { const auto changed=RequestBytes("414243444546474849");std::copy(changed.begin(),changed.end(),address.begin());port=12345;count=0xfffffffe; } };
        effects.duplicate=[&](auto& packet) { trace+=";Allocate,86";if (variant==2) { count=0xffffffff;port=9876;next[0x60/4]=0x76543210; }CHECK(CaptureMissionFortZone(image,packet)==1);storage=Hex(packet.storage);return 0x12200000U; }; // 원문의 명시 힙 입력 주소를 불투명 핸들로 공급한다.
        WriteMissionFortRequest(RequestText(input[4]),{next,count,address,port},effects);
        // 전체DWORD를 주변8BYTE와 함께 같은160BYTE 출력으로 대조한다.
        for (std::size_t index=0;index<next.size();++index) Put(result,8+index*4,next[index]);
        CHECK(Hex(result)==row[2] && std::to_string(count)==row[3] && storage==row[4] && Hex(image.zone)==row[5] && trace==row[6]);
    }
}
// 단일 소유 복제는 스택 패킷 수명이 끝난 뒤에도 바이트를 유지하며 중복 캡처를 거부한다.
TEST_CASE(MissionFortRequest_OwnedImageAndCurrentFields) {
    MissionFortLoadRequest request;request.next.fill(0xa5a5a5a5);std::array<std::uint8_t,9> address{'z','l','o','c','a','l',',','*',0};std::uint32_t port=6799; // 현재 Client와 같은 원본 주소9BYTE다.
    MissionFortRequestHooks effects;effects.selectFort=[&](auto name) { request.fort=name; };effects.duplicate=[&](auto& packet) { CHECK(request.next[0]==7 && request.next[31]==6799 && (request.next[23]>>8)==0xa5a5a5);return CaptureMissionFortZone(request,packet); };
    WriteMissionFortRequest("Tutorial1",{request.next,request.sequence,address,port},effects);CHECK(request.fort=="Tutorial1" && request.next[32]==1 && request.sequence==1 && request.zone.size()==86 && !request.consumed);
    auto extra=netstorm::zacket::MakeLocalMissionZone();bool rejected{};try { CaptureMissionFortZone(request,extra); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected && request.zone.size()==86);
}
