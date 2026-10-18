// 현재 Client의 객체 메뉴를 같은 Session/원본 Gump 트리/정수 명령 큐에 연결한다.
#pragma once
#include "client/InputEvent.h"
#include "client/Screen.h"
#include "o/SidPool.h"
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace netstorm::client {
class Client;
class GameWorld;
class RawButtonControls;
struct InventoryItemNode; // 같은 현재 생산 항목의 내용물/객체 필드를 빌린다.
// Client의 메뉴 수명은 모든 월드보다 길다. Clear는 Session을 반납하기 전에 호출한다.
class CampaignObjectMenuClient {
public:
    CampaignObjectMenuClient(Client& client,RawButtonControls& buttons,std::function<void(std::string_view)> tell); // 실제 장치/트리와 지연 Tell을 빌린다.
    ~CampaignObjectMenuClient(); // 월드가 살아 있는 동안 Clear로 객체 Form을 먼저 반납한다.
    void Open(GameWorld& world,o::Sid sid,const InputEvent& event); // 현재 사건 참조와 원본 파생 메뉴 슬롯을 사용한다.
    std::uint32_t OpenProduction(GameWorld& world,const InventoryItemNode& item,ScreenPoint point,const std::function<bool(o::Sid)>& affordable); //43d330의 실제 내용물 정보 메뉴를 연다.
    void Clear(); // 팝업→메뉴 체인→현재 월드 순서로 참조를 해제한다.
    bool Active() const; // 마지막 실제 루트의 현재 visible/dead를 읽는다.
    bool Command(const InputEvent& event); // 원본 연구/사제 건설/회수 선택을 처리하며 다른 명령은 false다.
private:
    struct Impl; // 실제 UI 경계/메뉴 체인/정적 문자열의 고정 주소 소유자다.
    std::unique_ptr<Impl> impl_; // Client/Gump 트리보다 먼저 반납한다.
};
}
