// 검사 관찰 콜백을 사용하지 않고 현재 클라이언트의 실제 서비스를 빌린다.
#include "client/CampaignClientServices.h"
#include "client/ClientAudio.h"
#include "client/ClientMain.h"
#include "client/RawButtonControls.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <bit>
#include <ctime>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
// 원본 time32/번역/설정/음향/입력을 동일 클라이언트의 저장소와 장치에 연결한다.
RawCampaignSessionServices MakeCampaignClientServices(Client& client,RawButtonControls& controls,CampaignClientUi ui) {
    if (!ui.refreshWindowTitle || !ui.tell)
        throw std::invalid_argument("실제 캠페인 클라이언트 UI 경계 누락");
    auto& audio=client.WorldAudio(); // noAudio도 같은 실제 이름표/비활성 재생 계층을 사용한다.
    auto& player=audio.Sounds(); // 반환한 콜백이 빌리는 장기 실제 SoundPlayer다.
    RawCampaignSessionServices services; // 게임 규칙 경계는 실제 세션의 명시 누락 판정을 유지한다.
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
    // 원본의 철자가 theammode이며 각 요청마다 기본값 1에서 현재 설정을 읽는다.
    services.ownership.themeMode=[&client] {
        std::int32_t value=1; // 설정 누락 시 원본이 유지하는 기본값이다.
        client.Configuration().ReadInt("theammode",value);return std::bit_cast<std::uint32_t>(value);
    };
    services.soundNames=&audio.Names(); // TYPE 소리와 보호막 프로세스가 공유하는 유일한 이름표다.
    // 정지와 무관한 실제 벽시계 및 실제 장치의 재생·정지·재생 여부를 사용한다.
    services.sound=MakeSoundProcessHooks(player,[&client] { return client.WallSeconds(); });
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
    return services;
}
}
