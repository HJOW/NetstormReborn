// 무인자 실행이 cwd의 다른 게임 대신 자기 배포 자료를 선택하는 실제 파일 경계를 검사한다.
#include "TestSupport.h"
#include "platform/PortableStartup.h"
#include <chrono>
#include <fstream>
#include <stdexcept>

namespace { // 임시 배포본만 만들며 보호 원본 자료를 열거나 수정하지 않는다.
class PortableFixture { // 테스트 하나가 소유하는 고유 임시 폴더와 원래 cwd의 수명을 묶는다.
public:
    PortableFixture():root(std::filesystem::temp_directory_path()/std::filesystem::path(u8"netstorm-휴대-😀")/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())),previous(std::filesystem::current_path()) { std::filesystem::create_directories(root); }
    ~PortableFixture() { std::filesystem::current_path(previous);std::filesystem::remove_all(root); } // 자신이 생성한 고유 하위 경로만 정리한다.
    void Archive(const std::filesystem::path& directory) { std::filesystem::create_directories(directory);std::ofstream(directory/"netstorm.tarc",std::ios::binary).put('x'); } // 선택 검사에 필요한 실제 일반 파일이다.
    std::filesystem::path root; // 이 테스트만의 한글/보충 평면 문자 배포 경로다.
    std::filesystem::path previous; // 예외/검사 종료 뒤 복구할 프로세스 cwd다.
};
}

TEST_CASE(PortableStartup_OwnUnicodePackageWinsOverCwdAndAdjacentArchive) { // 다른 cwd에서 실행해도 휴대 배포본의 game을 우선한다.
    PortableFixture fixture; // 실제 임시 파일과 cwd 복구를 소유한다.
    fixture.Archive(fixture.root/"package"/"game");fixture.Archive(fixture.root/"package");fixture.Archive(fixture.root/"decoy");
    std::filesystem::current_path(fixture.root/"decoy");
    CHECK(netstorm::platform::PortableGameDirectory(fixture.root/"package"/"Netstorm.exe")==fixture.root/"package"/"game");
}
TEST_CASE(PortableStartup_AdjacentLayoutAndMissingPackageNeverUseCwd) { // 원본식 옆 자료는 허용하되 cwd 자료나 같은 이름의 디렉토리는 거부한다.
    PortableFixture fixture; // 자신이 만든 파일만 정리하는 독립 수명이다.
    fixture.Archive(fixture.root/"decoy");std::filesystem::current_path(fixture.root/"decoy");
    std::filesystem::create_directories(fixture.root/"package"/"game"/"netstorm.tarc");
    bool failed=false; // 누락 시 cwd 자료를 암묵 선택하지 않았는지 확인한다.
    try { static_cast<void>(netstorm::platform::PortableGameDirectory(fixture.root/"package"/"Netstorm.exe")); } catch (const std::runtime_error&) { failed=true; }
    CHECK(failed);fixture.Archive(fixture.root/"package");
    CHECK(netstorm::platform::PortableGameDirectory(fixture.root/"package"/"Netstorm.exe")==fixture.root/"package");
}
