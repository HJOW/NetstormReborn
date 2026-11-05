// 사용자 홈 저장 루트의 Windows 경로 계산이다. 5차에서 새로 추가한 호환 장치이며 원본에는 대응 코드가 없다.
#include "platform/UserDataRoot.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <knownfolders.h>
#include <objbase.h>
#include <shlobj.h>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <system_error>

namespace netstorm::platform { // 게임 파일 저장 위치의 OS 경계다.
namespace { // 이 번역 단위 안에서만 쓰는 환경/알려진 폴더 조회 보조다.
// 환경 변수를 UTF-16으로 읽는다. 없거나 비어 있으면 빈 문자열이다(한글 사용자 이름도 손실 없이 얻는다).
std::wstring ReadEnvironment(std::wstring_view name) {
    const std::wstring key(name); // _wgetenv_s가 NUL 종료 문자열을 요구하므로 복사한다.
    size_t needed = 0; // 종료 NUL을 포함한 필요한 WCHAR 수다.
    if (::_wgetenv_s(&needed, nullptr, 0, key.c_str()) != 0 || needed <= 1) return {}; // 변수가 없거나 빈 값이면 무시한다.
    std::wstring value(needed, L'\0'); // 값을 받을 버퍼다.
    if (::_wgetenv_s(&needed, value.data(), value.size(), key.c_str()) != 0) return {}; // 두 번째 조회 실패는 없는 값으로 취급한다.
    value.resize(needed - 1); // 종료 NUL을 제거한다.
    return value;
}
// Windows가 사용자별로 정한 Saved Games 폴더를 조회한다. 실패하면 빈 경로이며 폴더를 만들지 않는다(KF_FLAG_DEFAULT).
std::filesystem::path KnownSavedGames() {
    PWSTR text = nullptr; // 셸이 CoTaskMem으로 할당해 돌려주는 경로다.
    const HRESULT result = ::SHGetKnownFolderPath(FOLDERID_SavedGames, KF_FLAG_DEFAULT, nullptr, &text);
    std::filesystem::path path; // 성공했을 때만 채우는 결과다.
    if (SUCCEEDED(result) && text) path = std::filesystem::path(std::wstring(text));
    ::CoTaskMemFree(text); // 실패 때도 셸이 준 포인터는 NULL 또는 유효 메모리이므로 한 번 반납한다.
    return path;
}
}

UserDataRoot::UserDataRoot(std::filesystem::path root) : root_(std::filesystem::absolute(std::move(root)).lexically_normal()) {}

// 설치 위치가 읽기 전용이어도 동작하도록 사용자 홈 쪽 후보만 순서대로 시도한다.
UserDataRoot UserDataRoot::Resolve() {
    if (const auto overridePath = ReadEnvironment(kUserDataOverrideVariable); !overridePath.empty()) // 검사/휴대 실행의 명시 재정의다.
        return UserDataRoot(std::filesystem::path(overridePath));
    if (const auto saved = KnownSavedGames(); !saved.empty()) return UserDataRoot(saved / L"Netstorm"); // 리디렉션된 Saved Games도 따른다.
    if (const auto profile = ReadEnvironment(L"USERPROFILE"); !profile.empty()) // 알려진 폴더를 못 얻은 환경의 문서화된 기본 위치다.
        return UserDataRoot(std::filesystem::path(profile) / L"Saved Games" / L"Netstorm");
    if (const auto local = ReadEnvironment(L"LOCALAPPDATA"); !local.empty()) return UserDataRoot(std::filesystem::path(local) / L"Netstorm"); // 마지막 사용자 전용 위치다.
    throw std::runtime_error("사용자 데이터 폴더(Saved Games\\Netstorm)를 결정할 수 없습니다");
}

const std::filesystem::path& UserDataRoot::Root() const { return root_; }
std::filesystem::path UserDataRoot::Saves() const { return root_ / std::filesystem::path(std::u8string(kUserSavesFolder.begin(), kUserSavesFolder.end())); }
std::filesystem::path UserDataRoot::CustomMaps() const { return root_ / std::filesystem::path(std::u8string(kUserCustomMapsFolder.begin(), kUserCustomMapsFolder.end())); }
std::filesystem::path UserDataRoot::Editor() const { return root_ / std::filesystem::path(std::u8string(kUserEditorFolder.begin(), kUserEditorFolder.end())); }
std::filesystem::path UserDataRoot::Captures() const { return root_ / std::filesystem::path(std::u8string(kUserCapturesFolder.begin(), kUserCapturesFolder.end())); }
std::filesystem::path UserDataRoot::Cache() const { return root_ / std::filesystem::path(std::u8string(kUserCacheFolder.begin(), kUserCacheFolder.end())); }
std::filesystem::path UserDataRoot::Diagnostics() const { return root_ / std::filesystem::path(std::u8string(kUserDiagnosticsFolder.begin(), kUserDiagnosticsFolder.end())); }
std::filesystem::path UserDataRoot::OptionsFile() const { return Saves() / L"options.cfg"; }
std::filesystem::path UserDataRoot::SavedStateFile() const { return Saves() / L"!state.dat"; }
std::filesystem::path UserDataRoot::FullScreenStateFile() const { return Saves() / L"fullscreenStateFile.dat"; }

// create_directories는 이미 있으면 false를 돌려주므로 실패 판정은 오류 코드와 최종 존재 여부로만 한다.
void UserDataRoot::EnsureDirectory(const std::filesystem::path& directory) {
    std::error_code error; // 예외 대신 코드로 받아 호출자에게 같은 경로 문맥의 메시지를 만든다.
    std::filesystem::create_directories(directory, error);
    if (error || !std::filesystem::is_directory(directory)) { // 같은 이름의 파일이 있거나 권한이 없으면 실패다.
        const auto utf8 = directory.u8string(); // 한글 경로가 ANSI 변환 예외를 내지 않도록 UTF-8로 메시지를 만든다.
        throw std::system_error(error ? error : std::make_error_code(std::errc::not_a_directory), "사용자 데이터 폴더를 만들 수 없습니다: " + std::string(utf8.begin(), utf8.end()));
    }
}
}
