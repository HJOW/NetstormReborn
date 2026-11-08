// Session과 무관한 건설 설정/표시/진단은 실제 현재 Client 장치를 조회한다.
#include "client/ClientConstructionGlobals.h"
#include "client/ClientMain.h"
#include "client/MessageGump.h" //원본과 같은 비용 안내 서식 몸체를 공유한다.
#include "client/Renderer.h" //현재 Renderer의 실제 InvalidateAll을 호출한다.
#include "client/UberGump.h" //현재 Tell은 기존 Tell/PauseForDialog 경로를 그대로 사용한다.
#include "o/OriginalText.h" //원문 표시 바이트만 현재 UTF-8 UI 경계에서 변환한다.
#include <array>
#include <iostream>
#include <stdexcept>

namespace netstorm::client { //실제 Client 장치를 빌리며 World/Session을 생성하거나 보유하지 않는다.
// 구성 시 조회/입력/그리기 없이 현재 Client를 선택하는 함수만 만든다.
ClientConstructionDevices MakeClientConstructionDevices(Client& client) {
    ClientConstructionDevices devices; //실제 current getter를 보유하며 Renderer/UI 객체 자체를 고정하지 않는다.
    devices.configInt=[&client](std::string_view key,std::int32_t fallback) { client.Configuration().ReadInt(key,fallback);return fallback; }; //누락 시 전달값을 보존하는 원문 ReadInt다.
    devices.wallSeconds=[&client] { return client.WallSeconds(); }; //같은 실제 벽시각이며 새로운 시계/시드를 만들지 않는다.
    devices.refresh=[&client] { client.GetRenderer().InvalidateAll(); }; //장치 재생성 뒤에도 현재 Renderer를 선택한다.
    devices.negativeQuantity=[] { std::clog<<"로컬 간헐천 원본 진단: reps >= 0\n"; }; //기존 진단 출력/복귀 계약을 유지한다.
    devices.boundsAssertion=[](o::PlayerGeyserBoundsAssertion assertion) {
        constexpr std::array<std::string_view,5> descriptions{"참여하지 않은 Player","metamap PlayerId","전투 밖 배치","Player1..8 범위","Canon 프레임 부재"}; //기존 다섯 진단과 같은 순서다.
        std::clog<<"Player 간헐천 범위 원본 진단: "<<descriptions.at(static_cast<std::size_t>(assertion))<<'\n';
    };
    devices.format=[](auto pattern,auto name) { return FormatWarningMessage(pattern,name); }; //기존 ClientCheated/ServerCheated의 문자열 처리다.
    devices.tell=[&client](std::string_view text) {
        if (!client.Menu()) throw std::logic_error("건설 비용 안내의 실제 현재 Tell UI 미연결");
        client.Menu()->InspectTell(o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()})); //기존 Tell의 표시 변환/창 생성/튜토리얼 정지 경로를 그대로 호출한다.
    };
    return devices;
}
}
