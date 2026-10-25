// 월드 유무와 무관하게 같은 실제 선택 owner와 raw Clear/Highlight를 연결한다.
#pragma once
#include <cstdint>
#include <functional>

namespace netstorm::client { // 실제 게임 효과는 필요할 때만 현재 Session에서 요구한다.
class Client;class ClientGameplayGlobals;class RawSurfaceStorage; // 같은 Client 물리의 현재 소유자만 빌린다.
void ClearClientUserSelection(ClientGameplayGlobals& globals,RawSurfaceStorage& storage,std::function<void(std::uint32_t)> selectCursor={}); //004d5c60의 실제 enabled/다중정리/선택0/강조0이다.
void ClearClientUserSelection(Client& client); // 실제 현재 하드웨어 커서와 Client 장기 Pool/TYPE를 조립한다. 창을 만들거나 게임을 실행하지 않는다.
}
