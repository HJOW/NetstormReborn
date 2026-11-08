//원문 목록은 unsigned BYTE 문자열이며 CoreData/TYPE 전체 표나 ACK를 복사하지 않는다.
#include "o/RawTechnologyTypesPacket.h"
#include "o/RawPacketDispatch.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o { //선택한 현재 장치만 실행하며 sender/known/서버/phase 관문을 추가하지 않는다.
namespace { //실제 선택 위치의 물리 오류와 미연결 효과를 구별한다.
template<class F> const F& NeedTechnology(const F& effect) { if (!effect) throw std::logic_error("69h의 현재 기술표시/진단 장치 미연결");return effect; } //빈 목록에서는 표시 장치를 요구하지 않는다.
std::uint8_t ReadTechnologyByte(std::span<const std::uint8_t> bytes,std::size_t index) { if (index>=bytes.size()) throw std::out_of_range("69h 타입 목록의 선택된 현재 BYTE 물리 부족");return bytes[index]; } //다음 BYTE를 미리 검사하거나 이전 쓰기를 되돌리지 않는다.
}
//각 입력 BYTE마다 현재 목록의 첫NUL 또는 첫일치만 선택한다. 별도count/정렬/종료NUL 쓰기를 하지 않는다.
void AddTechnologyTypes(std::span<const std::uint8_t> input,std::span<std::uint8_t,256> typeUse) {
    for (std::size_t source=0;;++source) { //실제 첫NUL을 읽을 때까지 순차 접근하여 부족한 뒤쪽 입력 앞의 저장을 보존한다.
        const auto value=ReadTechnologyByte(input,source);if (!value) return; //BYTE80h..ffh도 그대로 중복/빈칸 판정에 사용한다.
        std::size_t index{}; //이번 입력이 선택할 현재 목록의 위치다.
        for (;;) { //정상255종+NUL을 넘는 손상 상태는 선택된 index256 read에서 오류가 된다.
            const auto current=ReadTechnologyByte(typeUse,index);if (!current || current==value) break;++index;
        }
        if (!ReadTechnologyByte(typeUse,index)) typeUse[index]=ReadTechnologyByte(input,source); //선택한 첫빈칸만 쓰며 현재 원천 BYTE를 다시 읽는다.
    }
}
//합집합이 끝난 뒤에만 전체 현재 목록을 표시하며 각 callback 뒤 다음 목록 BYTE를 다시 읽는다.
void ReceiveTechnologyTypesPacket(std::span<const std::uint8_t> packet,std::span<std::uint8_t,256> typeUse,const TechnologyTypesIndicate& indicate) {
    if (packet.size()<5) throw std::out_of_range("69h payload+5의 실제 물리 부족");AddTechnologyTypes(packet.subspan(5),typeUse); //lenBYTE+4를 제한/검증/복사 길이로 읽지 않는다.
    for (std::size_t index=0;;++index) { //신규값뿐 아니라 기존 목록도 현재 첫NUL까지 원문 순서대로 다시 전달한다.
        const auto current=ReadTechnologyByte(typeUse,index);if (!current) return; //표시 효과가 바꾼 다음 항목/종료점도 이 위치에서 선택한다.
        NeedTechnology(indicate)(static_cast<std::uint32_t>(current)); //MOVZX BYTE→DWORD이며188TYPE 사전검사를 추가하지 않는다.
    }
}
//수신방향은packet 필드를 읽지 않는다. 송신 진단 정상 복귀 뒤 현재len BYTE를 다시 읽는다.
void ObserveTechnologyTypesPacket(std::span<std::uint8_t> packet,std::uint32_t sending,const TechnologyTypesAssertion& assertion) {
    if (!sending) return;
    if (ReadTechnologyByte(packet,4)==0xffU) NeedTechnology(assertion)("len != (unsigned char)-1","\\Ns\\O\\_p_packets.cpp",0xa5f);
    const auto length=static_cast<std::uint32_t>(ReadTechnologyByte(packet,4))+5U; //ff가 그대로라면260이며 하위WORD에 그대로 쓴다.
    packet[2]=static_cast<std::uint8_t>(length);packet[3]=static_cast<std::uint8_t>(length>>8); //앞선+4 read가성공했으므로선언WORD물리도존재한다.
}
//실제 nullable 직접 호출과 framed 수신이 같은 callable을 사용하며 sender/connection은본문에서읽지않는다.
void RegisterTechnologyTypesPacket(RawPacketDispatch& dispatch,std::span<std::uint8_t,256> typeUse,TechnologyTypesIndicate indicate,TechnologyTypesAssertion assertion) {
    dispatch.ObservePacket(0x69,[assertion=std::move(assertion)](auto packet,auto sending) { ObserveTechnologyTypesPacket(packet,sending,assertion); });
    dispatch.Version(0x69,1,0);dispatch.RequireLength(0x69,0xffff);
    dispatch.RegisterCanonicalDirect(0x69,0x004b49e0U,[typeUse,indicate=std::move(indicate)](auto,auto*,auto packet) { ReceiveTechnologyTypesPacket(packet,typeUse,indicate); }); //목록/span owner와 각 표시의 실제 현재 Session 선택은 Client caller가 보장한다.
}
}
