// UI가 공급하는 실제 장치와 아직 연결되지 않은 원본 경계를 구별한다.
#include "client/CampaignUserInputServices.h"
#include "client/ClientMain.h"
#include "client/GameWorld.h"
#include <iostream>
#include <stdexcept>
#include <utility>
namespace netstorm::client {
// 현재 Client/월드 수명 안에서만 호출하며 모든 callback은 같은 객체를 빌린다.
void BindCampaignUserInputServices(Client& client,GameWorld& world,std::function<void(std::uint32_t)> selectCursor,std::function<void(std::string_view)> tell) {
    UserSelectionHooks selection;selection.paused=[&client] { return client.Paused() ? 1U : 0U; }; //원본460de0의 같은 현재 게임 시계다.
    selection.dump=[&client](o::Sid sid) { client.TraceObject(sid.value); }; // 실제 현재 Pool/VT/재귀와 공유 trace FILE에 연결한다.
    selection.assertion=[] { throw std::logic_error("원본 Userinput.cpp selectedSid 진단"); };
    selection.previewAssertion=[] { throw std::logic_error("원본 Carrier.cpp !s.isForm 진단"); };
    UserHighlightHooks highlight;highlight.selectCursor=selectCursor;CarrierInputHooks carrier;carrier.selectCursor=std::move(selectCursor); //선택강조/폴링/건설커서가 동일한 실제 장치를 쓴다.
    carrier.assertion=[](std::uint32_t line) { throw std::logic_error("원본 Carrier.cpp 사용자입력 진단 "+std::to_string(line)); };
    // 운반물 내려놓기427070의 게임 계산과 전송은 같은 Session의 MakeCarrierInput이 연결한다.
    carrier.earlyLocal=[tell] { if (!tell) throw std::logic_error("원본4cf960 조기 희생 안내의 실제 Tell 미연결");tell("EarlySacrifice"); }; // 원문 42a538: 로컬 사제 소유자에게 4cf960("EarlySacrifice")=4cf730 Tell을 연다(M7.3, 1271). tell은 아래 showSacrifice가 이동하기 전에 복사한다.
    carrier.earlyRemote=[&world](std::uint32_t owner) { world.Session().SendPlayerTell(owner,"EarlySacrifice"); }; // 원문 42a545: 다른 소유자에게 4c9150(owner,"EarlySacrifice")의 70h Tell 패킷을 보낸다(M7.3, 1271). MP 송수신 실행은 하지 않는다.
    carrier.showSacrifice=[&world,tell=std::move(tell)](auto target) { world.Session().ShowSacrificeMenu(target,tell); }; //원문미션관문을그대로두고비미션의같은Tell/전체기술메뉴를연결한다.신규MP검사는하지않는다.
    carrier.marker=[&client](o::RawPathPoint point,std::uint32_t color) { client.DrawNativePathMarker(point.x,point.y,color); }; // 원문 434970 이동 진단 표시다(M7.4, 1271).
    carrier.refreshCursor=[&client] { client.PresentNativeCursorClear(); }; // 원문 4a46d0 즉시 viewport 출력이다(M7.4, 1271).
    world.BindUserInput(std::move(selection),std::move(highlight),std::move(carrier));
}
}
