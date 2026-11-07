// R의 실제 전역 초기화와 이미 복원한 수신 부모를 동일한 현재 표에 결합한다.
#include "root/PacketRegistration.h"
#include "root/RootControl.h"
#include "root/RootDirectory.h"
#include "root/LoginRequest.h"
#include "root/Routing.h"
#include "root/ChallengeRuntime.h"
#include "o/PacketRegistrationTable.h"
#include <array>
#include <stdexcept>

namespace netstorm::root { // 생성/IO와 별개로 원문 초기화 위치에서만 호출한다.
namespace { // 00442950의 등록 순서와 NULL/빈 함수 구별을 보존하는 읽기 전용 자료다.
struct ObserverEntry { // 원문 한 observer/version 등록이며 receiver는 실제 기존 부모가 소유한다.
    std::uint8_t type; // 실제 패킷 BYTE 번호다.
    bool hasObserver; // false는 원문 NULL이며 true의 빈 함수도 NULL로 바꾸지 않는다.
};
constexpr std::array<ObserverEntry,104> kObserverEntries{{ // 실제 104등록의 원문 순서이며83은 수신만 있어서 들어 있지 않다.
    {0x0b,true}, // 00443a00
    {0x0c,true}, // 00443b10
    {0x0f,true}, // 00443c20
    {0x10,true}, // 00443fd0
    {0x11,true}, // 00444060
    {0x12,true}, // 004440f0
    {0x16,true}, // 00444180
    {0x17,true}, // 00444290
    {0x1a,true}, // 00444330
    {0x1b,true}, // 00444440
    {0x1c,true}, // 004444f0
    {0x1d,true}, // 00444600
    {0x1e,true}, // 004446a0
    {0x1f,true}, // 00444730
    {0x20,true}, // 004447f0
    {0x88,true}, // 00444a00
    {0x21,true}, // 00444ad0
    {0x22,true}, // 00444c70
    {0x23,true}, // 00444d20
    {0x24,true}, // 00444db0
    {0x25,true}, // 00444e40
    {0x26,true}, // 00444ed0
    {0x27,true}, // 00445050
    {0x28,true}, // 004452a0
    {0x29,true}, // 00445440
    {0x2a,true}, // 004454d0
    {0x2b,true}, // 004455e0
    {0x2c,true}, // 00445700
    {0x2d,true}, // 00445790
    {0x2e,true}, // 00445820
    {0x2f,true}, // 004458c0
    {0x30,true}, // 00445950
    {0x31,true}, // 00445a60
    {0x32,true}, // 00445b70
    {0x33,true}, // 00445c00
    {0x34,true}, // 00445ca0
    {0x35,true}, // 00445d30
    {0x37,false}, // 원문 NULL
    {0x38,false}, // 원문 NULL
    {0x39,false}, // 원문 NULL
    {0x3a,false}, // 원문 NULL
    {0x3b,false}, // 원문 NULL
    {0x3c,false}, // 원문 NULL
    {0x3d,true}, // 00445dc0
    {0x3e,true}, // 00445e70
    {0x3f,true}, // 00445f20
    {0x40,true}, // 00446040
    {0x87,true}, // 00443e40
    {0x86,true}, // 004460d0
    {0x41,true}, // 00446160
    {0x42,true}, // 00446360
    {0x4a,true}, // 004463f0
    {0x4b,true}, // 00446480
    {0x4d,true}, // 00446510
    {0x4f,true}, // 004465a0
    {0x50,true}, // 00446630
    {0x51,true}, // 004466c0
    {0x52,true}, // 00446750
    {0x53,true}, // 004467e0
    {0x54,true}, // 00446870
    {0x55,true}, // 00446900
    {0x56,true}, // 00446a10
    {0x57,true}, // 00446b20
    {0x58,true}, // 00446c40
    {0x5a,true}, // 00446cd0
    {0x5b,true}, // 00446d60
    {0x5c,true}, // 00446df0
    {0x5f,true}, // 00446e80
    {0x60,true}, // 00446f90
    {0x61,true}, // 00447020
    {0x62,true}, // 004470b0
    {0x63,true}, // 004471c0
    {0x64,true}, // 00447250
    {0x65,true}, // 004472e0
    {0x66,true}, // 00447370
    {0x67,true}, // 00447400
    {0x85,true}, // 00447510
    {0x68,true}, // 00447620
    {0x69,true}, // 004476b0
    {0x6a,false}, // 원문 NULL
    {0x6b,true}, // 004477c0
    {0x6c,true}, // 00447850
    {0x6d,true}, // 004478e0
    {0x6e,true}, // 00447970
    {0x6f,true}, // 00447a00
    {0x70,true}, // 00447a90
    {0x71,true}, // 00447ba0
    {0x72,true}, // 00447cb0
    {0x73,false}, // 원문 NULL
    {0x74,false}, // 원문 NULL
    {0x75,true}, // 00447dd0
    {0x76,true}, // 00447ee0
    {0x77,true}, // 00447f70
    {0x78,true}, // 00448000
    {0x79,true}, // 00448090
    {0x7a,true}, // 00448120
    {0x7b,true}, // 00448230
    {0x7c,true}, // 00448340
    {0x7d,true}, // 004483e0
    {0x7e,true}, // 00448500
    {0x7f,true}, // 00448590
    {0x80,true}, // 004486a0
    {0x81,true}, // 00448730
    {0x84,true}, // 004451d0
}};
}
// 각 설치는 IO/콜백 실행 없이 현재 함수표만 작성하므로 실제 수신 부모를 결합한 뒤 최종 원문 관찰기 표를 확정한다.
void InitializePackets(TransceiverTransport& transport,RootControl& control,RootDirectory& directory,LoginRequest& login,Routing& routing,ChallengeRuntime& challenges) {
    if (transport.CallbacksActive()) throw std::logic_error("R 전송 콜백 안에서 전역 패킷 초기화 금지");auto& dispatch=transport.PacketHandlers();dispatch.Registrations()->ResetCallbacks();
    control.Install();directory.Install();login.Install();routing.Install();challenges.InstallPacketObservers(); // 원문18개 중 폐기80을 제외한 실제17수신이다. 같은 호출의 직접/일반 입구를 공유한다.
    for (const auto entry:kObserverEntries) { // 고정 길이는 별도 정적 표다. 전체 함수/버전을 지운 뒤 원문의 선택 슬롯만1/0으로 등록한다.
        if (entry.type==0x7f || entry.type==0x80) continue; // 폐기 외부 클라이언트 검증은 관찰/수신/송신 모두 비활성 상태로 남긴다.
        if (entry.hasObserver) dispatch.ObservePacket(entry.type,[&transport,type=entry.type](auto packet,auto sending) { ObservePacketFields(type,packet,sending,transport.Lifecycle().assertion); });
        else dispatch.Observe(entry.type,{});
        dispatch.Version(entry.type,1,0);
    }
    transport.InstallTerminationReceiver(); //477e10은 전역442950 뒤 실제2번 종료 수신을 추가한다.83의 버전/관찰기0도 그대로 남는다.
}
}
