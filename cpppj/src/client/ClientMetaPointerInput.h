//GameWorld가 없는 메타 화면도 현재 Session/Pool의 원본 선택·픽셀 판정·객체 입력을 공유한다.
#pragma once
#include "client/InputEvent.h"
#include "client/Screen.h"
#include <functional>
#include <memory>
#include <string_view>

namespace netstorm::client { //빌린 Client/Session/표시기/메뉴는 입력 처리기보다 오래 살아야 한다.
class Client;class RawCampaignSession;class RawCampaignFactoryDisplay;class RawMenuControls; //같은 실제 메타 화면의 장기 소유자다.
class ClientMetaPointerInput { //선택 몸체와 비소유 화면 장치를 같은 메타 수명에 묶는다.
public:
    ClientMetaPointerInput(Client& client,RawCampaignSession& session,RawCampaignFactoryDisplay& display,RawMenuControls& menus,std::function<ScreenRect()> view,std::function<std::uint32_t()> hide,std::function<void(std::string_view)> tell); //생성은 공통 입력 장치만 연결하고 선택/통신/프레임을 실행하지 않는다.
    ~ClientMetaPointerInput(); //같은 Session보다 먼저 선택/커서 입력의 비소유 참조를 반납한다.
    ClientMetaPointerInput(const ClientMetaPointerInput&)=delete;ClientMetaPointerInput& operator=(const ClientMetaPointerInput&)=delete; //활성 선택의 this/Session 연결을 복제하지 않는다.
    bool Event(InputEvent event); //4d9c23~da477의 메타 좌/우 클릭·모드6·빈 공간 항해를 현재 입력 큐에 연결한다.
    void Poll(InputEvent pointer); //Overlay 뒤의 현재 이동 강조/실제 활성 커서만 갱신한다. outside이면 갱신하지 않는다.
private:
    struct Impl;std::unique_ptr<Impl> impl_; //실제 RawUserSelection/Highlight/Carrier 입력과 빌린 화면 장치를 소유한다.
};
}
