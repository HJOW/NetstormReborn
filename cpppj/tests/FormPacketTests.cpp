// 새 실제 PE 관찰과 현재 Form writer의 전체 버퍼/삭제 표식 결과를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawFormPacket.h"
#include "o/SquidForms.h"
using namespace netstorm::test::rawscene;
namespace {
// fixture의16진 바이트를 원래 길이대로 읽는다. 기대 패킷은 계산하지 않는다.
std::vector<std::uint8_t> Hex(std::string_view text) {
    if (text.size()%2) throw std::invalid_argument("홀수 길이 Form fixture 바이트");
    std::vector<std::uint8_t> bytes; // 입력 또는 실제 PE가 돌려준 전체 바이트다.
    // 두 글자마다 한 바이트를 복원한다.
    for (std::size_t i=0;i<text.size();i+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(i,2)),nullptr,16)));
    return bytes;
}
// 두 판본의576개 입력을 같은 writer에 전수 적용한다. wrapper 행은 실제 wrapper를 거친 writer 결과다.
void CheckFormPackets(OriginalEdition edition) {
    SidPool pool(edition,24000,true);SquidForms forms(pool); // 실제 SID 경계의 큰 풀과 실제 Form 생성자다.
    constexpr Sid kForm{20000},kOther{20001},kOther2{20002}; // 독립 원본 입력과 같은 server 슬롯 번호다.
    forms.Take(1,kForm);forms.Take(1,kOther);forms.Take(1,kOther2);
    auto raw=pool.AllocatedBytes(kForm); // 입력 준비용 span은 free 상태 BYTE도 포함한 전체 native 입력을 수용한다.
    // 상대 Form의 getter는 실제 생성자 VT와 현재 parent WORD를 읽는다.
    for (const auto sid:{kOther,kOther2}) Put(pool.AllocatedBytes(sid),16,20101,2);
    std::size_t count=0; // 이 판본에 직접 대조한 새 입력 수다.
    // fixture의 현재 행에 해당하는 입력/출력만 대조한다.
    for (const auto& row:LoadFixture(NETSTORM_FORM_PACKET_FIXTURE).rows) {
        if (row[0]!=(edition==OriginalEdition::Patch1078 ? "originals" : "originalCD")) continue;
        CHECK(row.size()==16);const auto kind=row[1][0]; // 원본 진입 경로의 종류다.
        const bool dependency=kind=='D' || kind=='W',deleting=kind=='X' || kind=='Z'; // 실제 가상 writer 분류다.
        const auto input=Hex(row[3]),initial=Hex(row[4]),expected=Hex(row[13]); // C++와 무관하게 저장된 원본 입력/출력이다.
        CHECK(input.size()==raw.size());std::copy(input.begin(),input.end(),raw.begin());
        FormPacketHeaders headers;FormDeletionStamp stamp{Number(row[10]),Number(row[11])}; // 현재 헤더와 표식 전역의 입력이다.
        auto& header=deleting ? headers.deletion : dependency ? headers.dependency : headers.content; // 선택한 실제 등록 전역이다.
        header={static_cast<std::uint8_t>(Number(row[5])),static_cast<std::uint8_t>(Number(row[6])),static_cast<std::uint16_t>(Number(row[7]))};
        auto packet=initial;RawFormPacket writer(pool,headers,stamp); // 실제 출력 폭 뒤 stale 바이트도 남긴다.
        const auto size=deleting ? writer.WriteDeletion(kForm,Number(row[9]),packet) : writer.WriteData(kForm,packet);
        CHECK(size==Number(row[12]) && packet==expected && stamp.sid==Number(row[14]) && stamp.token==Number(row[11]));
        CHECK(std::equal(raw.begin(),raw.end(),input.begin()));
        // wrapper의 실제 방송 요청 수는 native 관찰 범위다. C++ 특정 수신자 wrapper 구현으로 주장하지 않는다.
        CHECK(Number(row[15])==static_cast<std::uint32_t>(kind=='W' || kind=='V' || kind=='Z'));++count;
    }
    CHECK(count==576);
}
}
// PATCH 원본 정상576행의 전체64바이트/슬롯/표식 결과다.
TEST_CASE(FormPacket_PatchNativeWritersAndWrapperResults) { CheckFormPackets(OriginalEdition::Patch1078); }
// CD의 다른 Content WORD/BYTE 위치와 삭제 표식 helper도 같은 방법으로 대조한다.
TEST_CASE(FormPacket_CdNativeWritersAndWrapperResults) { CheckFormPackets(OriginalEdition::Cd1072); }
// 물리 버퍼 부족은 출력/표식 변경 전에 거부하는 호스트 경계다. 원본 assert 대조 수에 넣지 않는다.
TEST_CASE(FormPacket_ShortDeleteBufferPreservesStamp) {
    SidPool pool(OriginalEdition::Patch1078,24000,true);FormPacketHeaders headers;FormDeletionStamp stamp{20000,0x1234};
    RawFormPacket writer(pool,headers,stamp);std::array<std::uint8_t,7> packet{};bool rejected=false;
    try { writer.WriteDeletion(Sid{20000},0xffffffff,packet); } catch (const std::out_of_range&) { rejected=true; }
    CHECK(rejected && stamp.sid==20000 && stamp.token==0x1234 && std::all_of(packet.begin(),packet.end(),[](auto b) { return b==0; }));
}
