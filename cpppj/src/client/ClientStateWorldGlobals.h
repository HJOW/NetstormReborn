// 원본 State 정리의 주소별 접근을 같은 Client·표면·Template·공통 UI의 실제 저장소에 연결한다.
#pragma once
#include "o/RawStateWorldCleanup.h"

namespace netstorm::client { // Native 주소는 식별자이며 호스트 포인터로 역참조하지 않는다.
class Client;class ClientAudio;class CampaignTemplateStorage;class RawMenuControls;class RawSurfaceStorage;
// 같은 실제 owner들의 비소유 묶음이다. 모든 대상은 반환한 callback보다 오래 살아야 한다.
struct ClientStateWorldContext {
    Client& client; // 현재 Player/State/SP/난수/권한/표시 Client다.
    RawSurfaceStorage& surface; // 현재 Pool/TYPE/Chunk/모드를 소유하며 Client의 권한·보조 전역과 동일해야 한다.
    CampaignTemplateStorage& templates; // 현재 Uber/Session이 공유하는 실제9×20BYTE 헤더다.
    RawMenuControls& menus; // Help Back과 같은 실제 팝업 명령 DWORD를 빌린다.
    ClientAudio& audio; // 현재 Client의 실제 장면 음악 owner다. noAudio도 같은 비활성 묶음을 쓴다.
};
// 004b2df0의 전체 직접 접근(read/write32·22프레임·seed.low16·double9·현재Player/TYPE/manager)을 제공한다.
// 정의된 Native 폭/주소만 허용하고 bool 모델은 정상0/1 쓰기만 받는다. 하위 Effect·Runtime 반납·State1 prefix는 별도다.
o::StateWorldCleanupGlobals BindClientStateWorldGlobals(ClientStateWorldContext context);
}
