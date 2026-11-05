// 5차 요구: 게임이 실행 중 자동으로 만드는 모든 파일(설정·진행·커스텀 맵·캡처·진단 등)을 설치 위치가 아닌
// 사용자 홈의 %USERPROFILE%\Saved Games\Netstorm\ 아래에 둔다. 관리자 권한이 필요한 위치(Program Files 등)나
// 읽기 전용 설치에서도 게임이 정상 동작하게 하려는 Windows 10/11 호환 처리이며 원본에는 없는 새 장치다.
#pragma once
#include <filesystem>
#include <string_view>

namespace netstorm::platform { // 게임 데이터 파일의 사용자 홈 저장 정책을 OS 경계에서 계산한다.
// 사용자 데이터 루트 아래 하위 폴더 이름이다. 설정/진행은 Saves, 새 커스텀 맵은 CustomMaps가 사용자 요구다.
inline constexpr std::string_view kUserSavesFolder = "Saves";          // options.cfg·!state.dat 등 설정/진행 파일 폴더 이름이다.
inline constexpr std::string_view kUserCustomMapsFolder = "CustomMaps"; // Edit 메뉴에서 새로 만든 맵(.fort/.english 등) 폴더 이름이다.
inline constexpr std::string_view kUserEditorFolder = "Editor";        // Edit 장부(catalog)·잠금 같은 편집 보조 metadata 폴더 이름이다.
inline constexpr std::string_view kUserCapturesFolder = "Captures";    // GIF 스크린샷·연속 BMP 캡처 폴더 이름이다.
inline constexpr std::string_view kUserDiagnosticsFolder = "Diagnostics"; // squids.txt·procSumm.txt 같은 진단 덤프 폴더 이름이다.
// 개발/검사에서 실제 사용자 홈 대신 임시 폴더를 쓰게 하는 환경 변수 이름이다. 값이 비어 있으면 무시한다.
inline constexpr std::wstring_view kUserDataOverrideVariable = L"NETSTORM_USER_DATA";

// 사용자 데이터 루트와 그 하위 폴더 경로를 계산하는 값 객체다. 경로 계산만 하며 생성자는 파일/폴더를 만들지 않는다.
class UserDataRoot {
public:
    // 명시한 절대 루트를 그대로 사용한다(검사나 이식 도구용). 상대 경로는 현재 작업 폴더 기준으로 절대화한다.
    explicit UserDataRoot(std::filesystem::path root);
    // 환경 변수 재정의 → Windows Saved Games 알려진 폴더 → %USERPROFILE%\Saved Games → %LOCALAPPDATA% 순으로 루트를 정한다.
    // 모두 실패하면 설치 폴더로 되돌아가지 않고 예외를 던진다(설치 폴더에 자동 생성 파일을 쓰지 않기 위함이다).
    static UserDataRoot Resolve();
    // 사용자 데이터 루트(…\Saved Games\Netstorm) 경로다.
    const std::filesystem::path& Root() const;
    // 설정/진행 폴더(Root\Saves)다.
    std::filesystem::path Saves() const;
    // 새 커스텀 맵 폴더(Root\CustomMaps)다.
    std::filesystem::path CustomMaps() const;
    // 편집 장부/잠금 metadata 폴더(Root\Editor)다.
    std::filesystem::path Editor() const;
    // 캡처(스크린샷/연속 BMP) 폴더(Root\Captures)다.
    std::filesystem::path Captures() const;
    // 진단 덤프 폴더(Root\Diagnostics)다.
    std::filesystem::path Diagnostics() const;
    // 옵션 파일(Root\Saves\options.cfg)이다. 원본의 <설치>\d\options.cfg를 대신한다.
    std::filesystem::path OptionsFile() const;
    // 전투 체크포인트(Root\Saves\!state.dat)다. 원본의 <설치>\d\!state.dat를 대신한다.
    std::filesystem::path SavedStateFile() const;
    // 전체화면 시작 표식(Root\Saves\fullscreenStateFile.dat)이다. 호환판은 이 파일을 만들지 않고 정리만 한다.
    std::filesystem::path FullScreenStateFile() const;
    // 폴더가 없으면 상위까지 만든다. 이미 있으면 아무 일도 하지 않으며 만들 수 없으면 예외를 던진다.
    static void EnsureDirectory(const std::filesystem::path& directory);
private:
    std::filesystem::path root_; // 절대화한 사용자 데이터 루트다.
};
}
