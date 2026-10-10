// 장면별 배경음악 감독(원본 Sound/Look 쪽 00469c80~00469fc0 / CD 00436960~00436d00)을 복원한다.
// 메뉴·전투·희생 의식·대기실·결과 화면에 맞는 곡을 고르고, 곡이 끝나는 시각을 실시간 시계로 관리하며, 곡에 딸린 날씨 효과를 적용한다.
// 곡 열기/재생은 SoundMusicSelection, 효과음은 Sound.h가 맡고 이 모듈은 "언제 어떤 곡을 요청하는가"를 정한다.
#pragma once
#include "client/Sound.h"
#include "client/SoundMusicSelection.h"
#include "o/SpStore.h"
#include <array>
#include <functional>
#include <string_view>

namespace netstorm::client {
// 장면별로 요청하는 곡 이름이다. 이름 비교는 ASCII 대소문자를 구분하지 않으며 파일 이름은 원본 철자를 그대로 쓴다.
//   kMenuMusic         전투 밖(메인 메뉴 등)과 전투 결과 뒤 복귀
//   kSacrificeMusic    내 제단에서 희생 의식이 진행 중일 때(전투 안)
//   kAnticipationMusic 멀티플레이 대기실
//   kFanfareMusic      승리 결과 화면(결과 상태 1일 때만 요청이 받아들여진다)
//   kDefeatMusic       패배 결과 화면
// 원본: 문자열 0050b638·00504538·00505ac0·00505658·0050564c / CD 0051a5d4·0051a59c·0051a604·0051a5b0·0051a5c8.
// 이력: 2026-10-10 추가 — 장면별 음악 감독을 옮기면서.
inline constexpr std::string_view kMenuMusic="ser22.mus",kSacrificeMusic="sacrifice.mus",kAnticipationMusic="anticipation.mus",
    kFanfareMusic="fanfare.mus",kDefeatMusic="defeat.mus";
// 전투 중 순환하는 원소 곡 네 개다. 색인 순서는 바람 → 비 → 천둥 → 해이며 다음 곡은 색인 + 1을 4로 나눈 나머지다.
// 원본: 표 005409e8 / CD 0051a1c0 (문자열 포인터 4개).
// 이력: 2026-10-10 추가.
inline constexpr std::array<std::string_view,4> kElementMusic{"wind22.mus","rain22.mus","thu22.mus","sun22.mus"};
// 원소 곡마다 쓰는 팔레트 파일이다(옵션 ascendancyPalette가 켜졌을 때만 적용한다).
// 원본: 표 005409c8 / CD 0051a1a0.
// 이력: 2026-10-10 추가.
inline constexpr std::array<std::string_view,4> kAscendancyPalettes{"windy.col","rainy.col","thundery.col","sunny.col"};
// 원소 곡을 시작할 때 함께 내는 효과음이다. 천둥 곡(색인 2)만 있고 나머지는 빈 이름(없음)이다.
// 원본: 표 005409d8 / CD 0051a1b0 (문자열 포인터 4개, 없는 칸은 null).
// 이력: 2026-10-10 추가.
inline constexpr std::array<std::string_view,4> kWeatherSounds{"","","thunderCrack.wav",""};
// 천둥 곡의 색인과, 팔레트 날씨를 켜는 설정 키, 비전투(메뉴) 곡의 색인이다.
// 원본: 비교 `cmp eax, 2`(00469cf4), 문자열 0050b624 / CD 0051a570, `mov [0x5409c4], 3`(00469ff7).
// 이력: 2026-10-10 추가.
inline constexpr std::int32_t kThunderMusicIndex=2,kMenuMusicIndex=3;
inline constexpr std::string_view kAscendancyPaletteOption="ascendancyPalette";
// 길이가 이 값보다 긴 곡은 길이만큼 지난 뒤 다음 곡으로 넘어가고, 이하이거나 길이를 모르면 이 값(초)만큼 지난 뒤 다시 확인한다.
// 원본: 상수 00501708 = 30.0, 005019e0 = 180.0 / CD 005010e0·005010e8.
// 이력: 2026-10-10 추가.
inline constexpr double kLongSongSeconds=30.0,kShortSongSeconds=180.0;
// 전투 시작 때 첫 원소 곡을 고르는 난수의 범위(0~3999)와 한 곡 구간의 크기(1000)다.
// 원본: `FUN_004558f0(0, 4000)`과 `/ 1000`(00469fc9~00469fe8).
// 이력: 2026-10-10 추가.
inline constexpr std::uint32_t kFirstSongRandomLimit=4000,kFirstSongRandomStep=1000;

// 장면별 음악 감독이 읽고 쓰는 원본 전역이다. 값은 클라이언트가 소유하고 감독이 살아 있는 동안 유지해야 한다.
// 이름이 확인되지 않은 전역은 용도를 설명으로 적었다.
// 이력: 2026-10-10 추가.
struct SceneMusicState {
    // 지금 순환 중인 원소 곡의 색인이다(DAT_005409c4 / CD 0051a19c). 처음은 3(해)이다. 정상 흐름에서는 0~3이다.
    std::int32_t index=kMenuMusicIndex;
    // 승리 곡/패배 곡이 끝나는 실시간(초)이다(DAT_00565e58·00565e60 / CD 005650a0·00565098). 이 시각 전에는 다른 곡 요청을 막는다.
    double fanfareEnd{},defeatEnd{};
    // 지금 곡이 끝나는 실시간(초)이다(DAT_00565e68 / CD 005650a8). Frame이 이 시각이 지났는지 본다.
    double songEnd{};
    // 전투 결과 상태다(DAT_005c85ac / CD 00518910): 1 승리 대기, 2 패배 대기. 승리 곡은 1일 때만 요청이 받아들여진다.
    std::uint32_t resultState{};
    // 전투(미션) 중인지(DAT_00594fbc / CD 00540a20), 플레이어 표가 준비돼 있는지(DAT_00595344 / CD 0050f828, 이름 미확인),
    // 로컬 플레이어 번호(DAT_00540c70 / CD 0050f6c8)다. 모두 DWORD이며 0이 아니면 참이다.
    std::uint32_t battle{},playersReady{},localPlayer{};
    // 지금 적용 중인 날씨 색(DAT_00540cdc / CD 005203a0, 처음 167)과 원소 곡별 색 표(DAT_005b5da8 / CD 00549b40)다.
    // 색 표는 팔레트에서 가장 가까운 색 번호로 초기화 때 채워진다.
    std::uint32_t tint=167;
    std::array<std::uint32_t,4> tints{};
    // 팔레트가 바뀌어 화면을 다시 그려야 함을 알리는 카운터다(DAT_0059a8b0 / CD 0052039c). 날씨 팔레트를 적용하면 1을 쓴다.
    std::uint32_t paletteDirty{};
};

// 장면별 음악 감독의 외부 경계다. 모두 연결해야 한다(빠지면 생성자가 std::invalid_argument를 던진다).
// 사용: 실제 모듈에 잇는 네 개(currentName·select·duration·weatherSound)는 MakeSceneMusicHooks가 채우고,
//       월드·화면·설정 쪽 경계는 호출자가 채운다. 인자와 반환값은 원본 함수 그대로다.
// 이력: 2026-10-10 추가.
struct SceneMusicHooks {
    // 정지와 무관하게 흐르는 실시간 시계(초)다. 안내 창으로 게임이 멈춰도 계속 흐른다. 00460d70 / CD 004011b0.
    std::function<double()> wallSeconds;
    // 지금 선택된 곡 이름이다(NUL 앞까지). 0054dd60 / CD 0052eb38의 256바이트 저장 공간.
    std::function<std::string_view()> currentName;
    // 곡을 고른다. 이름이 다르면 이전 곡을 멈추고 새 곡을 시작하며 현재 이름을 바꾼다. 00435200 / CD 00484aa0.
    std::function<void(std::string_view)> select;
    // 기본 음악 채널의 곡 길이(초)다. 곡을 열지 못했거나 멈췄으면 0이다. 004aa5b0 / CD 004398f0.
    std::function<double()> duration;
    // 그 플레이어의 제단에서 희생 의식이 진행 중인지다(단계 1 이상). 월드 경계다. 00449220 / CD 00484400.
    std::function<bool(std::uint32_t player)> sacrificing;
    // 멀티플레이 대기실인지다(도전 모드이고 플레이어 표가 준비됐고 그 플레이어의 대기 표시가 켜짐). 월드 경계다. 0042cbd0 / CD 0048ebe0.
    std::function<bool()> waitingRoom;
    // 화면 갱신을 요청한다(창 개체의 가상 함수 두 개를 불러 다시 그리게 한다). 화면 경계다. 0043dad0 / CD 004ee9d0.
    std::function<void()> refresh;
    // 설정 ascendancyPalette가 1인지다(없으면 거짓). 설정 경계다. 00441470("ascendancyPalette", 1) / CD 004a98d0.
    std::function<bool()> ascendancyPalette;
    // 팔레트 파일을 두 디렉터리 아래에서 찾아 읽어 전역 팔레트에 적용한다. 화면 경계다. 00459c60 + 004a4850 / CD 0049c1a0 + 00424760.
    std::function<void(std::string_view paletteName)> loadPalette;
    // 천둥 번개 화면 효과를 시작한다. 화면 경계다. 00470fa0(0) / CD 004b10d0.
    std::function<void()> thunderFlash;
    // 효과음을 이름으로 한 번 내되 반복·음량·좌우·우선·한도를 모두 0으로 넘긴다. 004a9cb0 / CD 00438b50.
    std::function<void(std::string_view soundName)> weatherSound;
};

// 기본 음악 채널 상태에서 곡 길이(초)를 읽는다. 채널 +0x18의 double이며 파일을 열 때 쓰이고 닫으면 0이다.
// 원본: 004aa5b0 / CD 004398f0이 읽는 전역 005c7b78 / CD 005650d0(기본 채널 +0x18).
// 이력: 2026-10-10 추가.
double MusicChannelDuration(const MusicChannelState& primary);

// 장면별 음악 감독이다.
// 사용: state·random·hooks 대상은 이 객체보다 오래 살아야 한다. 주 스레드에서만 부른다.
//       Start는 전투/메뉴 진입 때, Frame은 매 프레임, Request는 결과 화면·제단 희생·대기실 등 장면 전환 때 부른다.
// 이력: 2026-10-10 추가.
class SceneMusic {
public:
    // 필수 경계가 하나라도 비어 있으면 std::invalid_argument를 던진다. OS 자원은 열지 않는다.
    SceneMusic(SceneMusicState& state,o::GameRandom& random,SceneMusicHooks hooks);
    // 승리/패배 곡이 아직 끝나지 않은 동안 참이다: 현재 곡이 fanfare이고 실시간이 fanfareEnd 전이거나,
    // 현재 곡이 defeat이고 실시간이 defeatEnd 전이다. 시각이 비교 불가(NaN)이면 참이다(x87 비교의 원본 동작).
    // 원본: 00469d30 / CD 00436a10.
    bool ResultMusicLocked() const;
    // 곡 재생을 요청한다. 지금 곡과 같은 이름(ASCII 대소문자 무시)이면 아무것도 하지 않고,
    // 결과 곡이 재생 중이면 anticipation 외 요청을 버리며, fanfare는 결과 상태가 1일 때만 받아들인다.
    // 받아들이면 곡을 고르고 fanfare/defeat의 끝 시각을 적고, 곡 길이가 30초보다 길면 "지금 + 길이", 아니면 "지금 + 180초"를 songEnd로 정한다.
    // 원본: 00469db0 / CD 00436a90.
    void Request(std::string_view name);
    // 다음 곡으로 넘어간다: 색인을 하나 올려 4로 나눈 나머지(C 나눗셈)로 정한다. 플레이어 표가 준비됐고 로컬 플레이어의 희생 의식이
    // 진행 중이면 희생 곡을 요청하고 끝낸다. 아니면 그 색인의 원소 곡을 요청하고 날씨 효과를 적용한다.
    // 색인이 0~3을 벗어나면 원소 곡 표를 읽을 수 없으므로 std::out_of_range를 던진다(원본은 표 앞 메모리를 읽는다).
    // 원본: 00469f00 / CD 00436c40.
    void Next();
    // 매 프레임 호출한다. 실시간이 songEnd에 이르렀을 때만(비교 불가면 건너뜀) 전투 중이면 다음 곡,
    // 대기실이면 anticipation, 아니면 메뉴 곡을 요청한다.
    // 원본: 00469f60 / CD 00436ca0.
    void Frame();
    // 전투 또는 메뉴에 진입할 때 곡을 시작한다: 전투 밖이면 색인 3과 메뉴 곡, 전투 중이면 난수(0~3999)/1000을 색인으로 두고 다음 곡을 요청한다.
    // 이어 Frame을 한 번 부르고 현재 색인의 날씨 색을 tint에 둔다. 색인이 0~3을 벗어나면 std::out_of_range를 던진다.
    // 원본: 00469fc0 / CD 00436d00.
    void Start();
    // 현재 색인의 날씨 효과를 적용한다: 화면 갱신 → (설정이 켜졌으면 날씨 색·팔레트 파일 적용, 팔레트 갱신 표시) →
    // (효과음이 있으면, 천둥 곡이면 번개 효과를 먼저 시작하고, 효과음 재생). 색인이 0~3을 벗어나면 std::out_of_range를 던진다.
    // 원본: 00469c80 / CD 00436960.
    void ApplyWeather();
private:
    SceneMusicState& state_;o::GameRandom& random_;SceneMusicHooks hooks_;
};

// 실제 음악 선택·기본 채널·효과음 재생 계층에 잇는 경계를 채운다. currentName·select·duration·weatherSound는 이 함수가 만들고
// wallSeconds·sacrificing·waitingRoom·refresh·ascendancyPalette·loadPalette·thunderFlash는 external의 것을 그대로 쓴다.
// 사용: selection·names·primary·sounds는 돌려준 hooks보다 오래 살아야 한다. names는 selection이 쓰는 같은 상태여야 한다.
// 이력: 2026-10-10 추가.
SceneMusicHooks MakeSceneMusicHooks(MusicSelection& selection,const MusicSelectionState& names,const MusicChannelState& primary,
    SoundPlayer& sounds,SceneMusicHooks external);
}
