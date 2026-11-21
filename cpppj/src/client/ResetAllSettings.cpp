// 15차의 초기화는 사용자 Saves의 명시 파일만 대상으로 하며 원본 파일의 이름 탐색을 사용하지 않는다.
#include "client/ResetAllSettings.h"
#include "client/ClientOptionsDisk.h"
#include <array>
#include <stdexcept>
#include <system_error>

namespace netstorm::client { // 사용자 요청으로 새로 추가한 설정 초기화이며 원본 소스 대응은 없다.
namespace { // 삭제 전에 경로 종류를 검사하여 디렉터리/심볼릭 링크를 따라가지 않는다.
// 존재하지 않는 경로는 허용하고, 존재하는 명시 경로는 요구한 디렉터리 또는 일반 파일이어야 한다.
void CheckResetPath(const std::filesystem::path& path,bool directory) {
    std::error_code error; // 조회 실패를 파일 없음으로 오인하지 않게 구별한다.
    const auto status=std::filesystem::symlink_status(path,error); // 링크의 대상 대신 링크 자체의 종류를 읽는다.
    if (error==std::errc::no_such_file_or_directory) return;
    if (error) throw std::filesystem::filesystem_error("설정 초기화 경로 조회 실패",path,error);
    if (!std::filesystem::exists(status)) return;
    if (std::filesystem::is_symlink(status) || (directory ? !std::filesystem::is_directory(status):!std::filesystem::is_regular_file(status)))
        throw std::filesystem::filesystem_error("설정 초기화 경로 종류가 올바르지 않습니다",path,std::make_error_code(std::errc::operation_not_permitted));
}
}
// 저장 파일 부재도 정상 초기화다. 빈 options.cfg는 사용자 우선 로더의 값으로 남겨 설치 d/options.cfg 재유입을 막는다.
void ResetAllUserSettings(const platform::UserDataRoot& user) {
    CheckResetPath(user.Root(),true);CheckResetPath(user.Saves(),true);
    const std::array<std::filesystem::path,3> discarded{user.SavedStateFile(),user.FullScreenStateFile(),user.Saves()/L"!rootservers.dat"}; // 재접속 진행·옛 화면 표식·사용자 서버 선택 캐시다.
    CheckResetPath(user.OptionsFile(),false);
    for (const auto& path:discarded) CheckResetPath(path,false); // 어느 경로가 디렉터리/링크인지 쓰기 전에 전부 확인한다.
    platform::UserDataRoot::EnsureDirectory(user.Saves());
    WriteClientOptionsDisk(user.OptionsFile(),{}); // 같은 Unicode/매핑 호환 저장 경계로 현재 설정·진행 키를 모두 비운다.
    for (const auto& path:discarded) { // 명시한 일반 파일만 삭제하며 다른 Saves 파일이나 하위 폴더는 열거하지 않는다.
        std::error_code error; // 일부 파일이 잠겼으면 성공으로 숨기지 않고 경로와 원인을 호출자에게 전달한다.
        std::filesystem::remove(path,error);
        if (error) throw std::filesystem::filesystem_error("설정 초기화 파일 삭제 실패",path,error);
    }
}
}
