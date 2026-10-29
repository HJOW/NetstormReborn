// UI가 공급하는 실제 장치와 아직 연결되지 않은 원본 경계를 구별한다.
#include "client/CampaignUserInputServices.h"
#include "client/ClientMain.h"
#include "client/GameWorld.h"
#include <iostream>
#include <stdexcept>
#include <utility>
namespace netstorm::client {
// 현재 Client/월드 수명 안에서만 호출하며 모든 callback은 같은 객체를 빌린다.
void BindCampaignUserInputServices(Client& client,GameWorld& world,std::function<void(std::uint32_t)> selectCursor) {
    UserSelectionHooks selection;selection.paused=[&client] { return client.Paused() ? 1U : 0U; }; //원본460de0의 같은 현재 게임 시계다.
    selection.dump=[&client](o::Sid sid) { client.TraceObject(sid.value); }; // 실제 현재 Pool/VT/재귀와 공유 trace FILE에 연결한다.
    selection.assertion=[] { throw std::logic_error("원본 Userinput.cpp selectedSid 진단"); };
    selection.previewAssertion=[] { throw std::logic_error("원본 Carrier.cpp !s.isForm 진단"); };
    UserHighlightHooks highlight;highlight.selectCursor=selectCursor;CarrierInputHooks carrier;carrier.selectCursor=std::move(selectCursor); //선택강조/폴링/건설커서가 동일한 실제 장치를 쓴다.
    carrier.assertion=[](std::uint32_t line) { throw std::logic_error("원본 Carrier.cpp 사용자입력 진단 "+std::to_string(line)); };
    // 운반물 내려놓기427070의 게임 계산과 전송은 같은 Session의 MakeCarrierInput이 연결한다.
    carrier.earlyLocal=[] { throw std::logic_error("원본4cf960 조기 희생 안내 UI 미연결"); };carrier.earlyRemote=[](std::uint32_t) { throw std::logic_error("원본 EarlySacrifice 원격 안내 미연결"); };
    // TODO: 원본4cfc10의 SacrificeMenu는 일반/멀티플레이 보상 선택 창이다. 캠페인/튜토리얼은 Session의 현재missionPost 관문에서 생략한다.
    // 사용자 지시로 멀티플레이 UI 복원은 해당 구현 단계까지 임시 생략한다. 의식 이동·룬·캠페인 보상 처리에는 성공값을 주입하지 않는다.
    carrier.showSacrifice=[](std::uint32_t) {};
    carrier.marker=[](o::RawPathPoint,std::uint32_t) { throw std::logic_error("원본434970 이동 진단 표시 미연결"); };carrier.refreshCursor=[] { throw std::logic_error("원본4a46d0 이동 진단 커서 갱신 미연결"); };
    world.BindUserInput(std::move(selection),std::move(highlight),std::move(carrier));
}
}
