// 직접 caller가 만든40BYTE 지역 버퍼를 wire38BYTE로 자르거나 네트워크에 재전송하지 않는다.
#include "client/NativeJoinTestBattle.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientSubscriberGlobals.h"
#include "client/ClientCampaignTransport.h"
#include "client/ClientPacketSources.h"
#include "client/CurrentAllocationBytes.h"
#include <array>
#include <cstring>
#include <stdexcept>

namespace netstorm::client { // 원문 서버/State의 같은 실제 장기 주소만 쓴다.
void JoinNativeTestBattle(Client& client,const std::function<NativeBattleSource()>& selectSource) {
    const auto subscribers=client.GameplayGlobals()->Subscribers(); // PTR00540ca8의 현재 이름 owner다.
    const int simpson=::_stricmp(reinterpret_cast<const char*>(subscribers->Handle().data()),"zsimpson"); //기대 리터럴의 NUL까지 최대9BYTE라64BYTE 물리 안에서 비교한다.
    const int greer=::_stricmp(reinterpret_cast<const char*>(subscribers->Handle().data()),"jgreer"); //앞선 이름 일치 여부와 관계없이 두 번째 비교를 실행한다.
    const int demarest=::_stricmp(reinterpret_cast<const char*>(subscribers->Handle().data()),"kdemarest"); //같은 현재 handle을 세 번째 비교 위치에서 읽는다.
    const int bratton=::_stricmp(reinterpret_cast<const char*>(subscribers->Handle().data()),"tbratton"); //원문의 네 번째 비교까지 마친 뒤 결과를 판단한다.
    if (simpson && greer && demarest && bratton) { client.WriteDiagnosticLog(29,"To join test battle, you must be zsimpson, jgreer, tbratton, or kdemarest\n");return; }
    const auto game=client.GameplayGlobals();const auto local=client.LocalStateGlobals(); //이름 관문 뒤에만 같은 실제 packet/State 전역을 선택한다.
    std::array<std::uint8_t,40> packet;CurrentAllocationBytes(packet); //원문의40BYTE 지역 물리와 미작성 tail 입력을 유지하며0초기화로 가장하지 않는다.
    packet[0]=0;packet[1]=0;packet[2]=0;packet[3]=0;
    const auto& header=game->GameAux()->BattleStartPeerHeader(); //등록표의 현재 version/length와 다른 원문 생성자 전역이다.
    packet[1]=header.message;const auto length=header.length<4 ? std::uint16_t{4}:header.length;packet[2]=static_cast<std::uint8_t>(length);packet[3]=static_cast<std::uint8_t>(length>>8);packet[0]=header.protocol&0x7fU;
    packet[4]=0;packet[5]=0;packet[6]=0;packet[7]=0; //원문 local_24의 seed0만 쓰며 RNG/성공 값으로 바꾸지 않는다.
    for (std::size_t index=0;;++index) { //원문 현재 주소의 NUL도 한 BYTE씩 복사하며 빈 주소를 선행 거부하지 않는다.
        if (index>=local->TestBattleAddress().size() || index>=packet.size()-8) throw std::out_of_range("JoinTestBattle 현재 주소/지역32BYTE 물리 부족");
        const auto value=local->TestBattleAddress()[index];packet[8+index]=value;if (!value) break;
    }
    game->Server()=false;local->State().phase[0]=29; //005c8758은 phase이며 current[0]는 같은1bh 본문이7로 쓴다.
    ReceiveClientBattleStartPeer(client,packet,selectSource); //sender0/NULL 연결의 직접 본문 호출이며 Validate/observer/record/depth/queue를 추가하지 않는다.
}
}
