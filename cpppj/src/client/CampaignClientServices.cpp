// 검사 관찰 콜백을 사용하지 않고 현재 클라이언트의 실제 서비스를 빌린다.
#include "client/CampaignClientServices.h"
#include "client/ClientAudio.h"
#include "client/ClientMain.h"
#include "client/RawButtonControls.h"
#include "client/RawChatChannels.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <bit>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 원본 생성자가 초기화하지 않는 바이트는 현재 CRT 할당의 실제 입력을 복사한다. 할당은 복사 뒤 즉시 반환한다.
void CurrentAllocationBytes(std::span<std::uint8_t> bytes) {
    if (bytes.empty()) return;
    void* allocation=std::malloc(bytes.size()); // 원본 malloc 계층처럼 zero 플래그 없는 실제 힙을 사용한다.
    if (!allocation) throw std::bad_alloc();
    std::memcpy(bytes.data(),allocation,bytes.size());std::free(allocation);
}
}
// Player SetAlliance의 실제 채널 수정과004410f0(defaultAiName)의 현재 원본 바이트를 같은 Client에 연결한다.
void BindCampaignAiClientServices(Client& client,CampaignAiServices& ai) {
    // 원본 Clear는 채널 BYTE만 지우고 Add는 현재 자기 subscriber의 자연 -1 반환까지 수행한다.
    ai.participation.clearChat=[&client](std::uint32_t channel) { client.ChatChannels().Clear(channel); };
    ai.participation.addChat=[&client](std::uint32_t subscriber,std::string_view name,std::uint32_t channel) {
        static_cast<void>(client.ChatChannels().AddToChannel(subscriber,name,channel));
    };
    // Xlat 키로 바꾸지 않는다. 현재 언어 Config/치환 결과를 매 요청마다 원본 Windows-1252로 인코딩한다.
    ai.defaultName=[&client] {
        const auto bytes=o::EncodeOriginalText(client.Configuration().PathSpec("defaultAiName")); // 현재 언어 교체/설정 변경을 반영한 실제 좁은 바이트다.
        return std::string(bytes.begin(),bytes.end());
    };
}
// 원본 time32/번역/설정/음향/입력을 동일 클라이언트의 저장소와 장치에 연결한다.
RawCampaignSessionServices MakeCampaignClientServices(Client& client,RawButtonControls& controls,CampaignClientUi ui) {
    if (!ui.refreshWindowTitle || !ui.tell)
        throw std::invalid_argument("실제 캠페인 클라이언트 UI 경계 누락");
    auto& audio=client.WorldAudio(); // noAudio도 같은 실제 이름표/비활성 재생 계층을 사용한다.
    auto& player=audio.Sounds(); // 반환한 콜백이 빌리는 장기 실제 SoundPlayer다.
    RawCampaignSessionServices services; // 게임 규칙 경계는 실제 세션의 명시 누락 판정을 유지한다.
    services.ai.allocationBytes=CurrentAllocationBytes; // 실제 AI 생성자가 보존하는 바이트의 현재 힙 입력이다.
    services.production.allocationBytes=CurrentAllocationBytes; // Deck 생성자도 초기화하는 5DWORD 이외의 입력을 보존한다.
    services.production.effects.configInt=[&client](std::string_view key) {
        std::int32_t value{};client.Configuration().ReadInt(key,value);return value;
    };
    services.production.effects.modeAssertion=[] { std::clog<<"생산 재계산 원본 진단: inFortMode || inBattleMode\n"; };
    // 원본 _time32의 지원 범위만 허용한다. 날짜를 DWORD로 감아 다른 난수 시드로 바꾸지 않는다.
    services.unixSeconds=[] {
        const auto now=std::time(nullptr); // 실제 시스템의 현재 Unix 초다.
        if (now<0 || now>std::numeric_limits<std::int32_t>::max())
            throw std::out_of_range("실제 캠페인 time32 지원 날짜 범위 초과");
        return static_cast<std::int32_t>(now);
    };
    auto you=std::make_shared<std::vector<std::uint8_t>>(); // 원본 부모가 NUL까지 복사하는 동안 유지할 번역 바이트다.
    // 번역은 요청 시 다시 조회한다. 설치 사이에 바뀐 언어를 이전 문자열로 고정하지 않는다.
    services.install.translateYou=[&client,you]() -> std::span<const std::uint8_t> {
        *you=o::EncodeOriginalText(client.Translate("You"));you->push_back(0);return *you;
    };
    // 원본 strcpy 입력의 첫 NUL까지만 UTF-8 설정 저장소에 넣고 뒤의 물리 바이트는 무시한다.
    services.install.setConfig=[&client](std::string_view key,std::span<const std::uint8_t> bytes) {
        const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{}); // 좁은 문자열의 실제 종료 위치다.
        if (end==bytes.end()) throw std::invalid_argument("캠페인 설정 문자열 NUL 누락");
        client.Configuration().Set(key,o::DecodeOriginalText(bytes.first(static_cast<std::size_t>(end-bytes.begin()))));
    };
    services.install.refreshWindowTitle=std::move(ui.refreshWindowTitle); // 월드 설치 중 현재 미션 제목도 갱신한다.
    // 원본 0047d890의 사건을 같은 실제 Gump 트리로 분배한다. 생산 노드 자체의 구현은 별도다.
    services.ownership.refreshProduction=[&controls] { controls.RefreshProduction(); };
    services.assets.postPop.refreshProduction=services.ownership.refreshProduction; // 신전 PostPop도 동일한 사건을 보낸다.
    // Player 부모 통지는 현재 등록표와 대상 가상+28을 직접 사용한다.
    services.playerContent.gump=[&controls](std::uint32_t id) { return controls.RegisteredGump(id); };
    services.playerContent.handle=[&controls](std::uint32_t token,std::uint32_t event,std::uint32_t form,std::uint32_t reason) {
        static_cast<void>(controls.HandleGump(token,event,form,reason));
    };
    // 원본의 철자가 theammode이며 각 요청마다 기본값 1에서 현재 설정을 읽는다.
    services.ownership.themeMode=[&client] {
        std::int32_t value=1; // 설정 누락 시 원본이 유지하는 기본값이다.
        client.Configuration().ReadInt("theammode",value);return std::bit_cast<std::uint32_t>(value);
    };
    services.soundNames=&audio.Names(); // TYPE 소리와 보호막 프로세스가 공유하는 유일한 이름표다.
    // 정지와 무관한 실제 벽시계 및 실제 장치의 재생·정지·재생 여부를 사용한다.
    services.sound=MakeSoundProcessHooks(player,[&client] { return client.WallSeconds(); });
    // 원본 공통 preDestroy의 전역 재생 인자(반복0/음량0/좌우0/우선1/한도0)를 유지한다.
    services.unitLost=[&player](o::Sid) { player.PlayByName("unitLost.wav",0,0,0,1,0); };
    // 내용물 너겟 삭제는 같은 실제 입력 큐와 위치 효과음을 사용한다.
    services.contentLifetime.localNotice=[&client](std::uint32_t command) { client.Input().PushCommand(command,0,0); };
    services.contentLifetime.sound=[&player](float x,float y) { player.PlayNameAt(x,y,"guy.wav",0); };
    services.priest.shield=MakePriestShieldNoticeHooks(player,{}); // 원본 안내 소리의 우선 인자 1을 유지한다.
    // 00493d30은 004a89c0으로 인자 없는 Tell 키를 포맷한다. Xlat으로 섹션 이름을 바꾸지 않는다.
    services.priest.shield.translate=[](std::string_view key) { return std::string(key); };
    services.priest.shield.tell=std::move(ui.tell); // 실제 Tell이 안내 창과 게임 시계 정지를 처리한다.
    services.priest.fallSound=MakePriestFallSound(player); // 사제 낙하의 현재 위치/파일명/반복 없음이다.
    // Vortex PostPop의 TYPE+74 전역 재생은 반복·음량·좌우·우선·한도가 모두 0이다.
    services.assets.postPop.playImpactSound=[&player](std::uint32_t sound) { player.Play(sound,0,0,0,0,0); };
    // 실제 총 명령을 원본 마스크/순서가 복원된 단일 입력 큐에 추가한다.
    services.assets.queueCommand=[&client](std::uint32_t command,std::int32_t first,std::int32_t second) {
        client.Input().PushCommand(command,first,second);
    };
    // Missile ctor/충돌과 Path Step은 실제 위치 한 번 재생에 반복 0·우선 0을 넘긴다.
    services.archer.missile.firingSound=services.archer.missile.impactSound=
        [&player](float x,float y,std::uint32_t sound) { player.PlayOnceAt(x,y,sound,0,0); };
    services.motion.path.stepSound=[&player](o::Sid,float x,float y,std::uint32_t sound) {
        player.PlayOnceAt(x,y,sound,0,0);
    };
    // 원본 Player 진단 그룹과 문자열을 실제 프로세스 로그 스트림에 순서대로 출력한다.
    services.player.log=[](std::uint32_t group,std::string_view text) { std::clog<<"["<<group<<"] "<<text; };
    // 원본 predictableGeysers의 기본값을 유지하고 호출 때마다 현재 Config를 다시 읽는다.
    services.construction.configInt=[&client](std::string_view key,std::int32_t fallback) {
        auto value=fallback;client.Configuration().ReadInt(key,value);return value;
    };
    services.construction.wallSeconds=[&client] { return client.WallSeconds(); }; // 비결정적 TimeSeed만 실제 벽시각을 읽는다.
    services.construction.refresh=[&client] { client.GetRenderer().InvalidateAll(); }; // 장치 재생성 뒤에도 현재 Renderer를 갱신한다.
    // 원본 진단 뒤 계속하는 계약을 보존하며 실제 로그 스트림에 진단을 남긴다.
    services.construction.negativeQuantity=[] { std::clog<<"로컬 간헐천 원본 진단: reps >= 0\n"; };
    services.construction.boundsAssertion=[](o::PlayerGeyserBoundsAssertion assertion) {
        constexpr std::array<std::string_view,5> descriptions{"참여하지 않은 Player","metamap PlayerId","전투 밖 배치","Player1..8 범위","Canon 프레임 부재"}; // 원본 진단 분류와 같은 순서의 실제 로그 설명이다.
        std::clog<<"Player 간헐천 범위 원본 진단: "<<descriptions.at(static_cast<std::size_t>(assertion))<<'\n';
    };
    BindCampaignAiClientServices(client,services.ai); // 장치 없는 검사와 실제 GUI 월드가 동일한 장기 Client 경계를 사용한다.
    return services;
}
}
