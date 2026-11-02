// 원문441790의 설정 위치 탐색과 실제 시작 파일 루트를 공유한다.
#pragma once
#include "o/BaseFile.h"
#include "o/FileSpecQuery.h"
#include <functional>

namespace netstorm::client { // 같은 Client의 경로 고리/파일 시스템을 실행 초기화에 연결한다.
using StartupFileProbe=std::function<std::uint32_t(std::string_view)>; // 실제41ab10 또는 명시 원문 재생 장치를 호출한다.
std::string LocateStartupData(std::string_view gameRoot,o::RawFileName& names,const StartupFileProbe& probe); // 원문의 여섯 setup.cfg 후보 중 첫 성공에서 Directory/Device/Format의 소유 복사를 반환한다. 전부 실패는 원문195행 진단이다.
struct StartupFileLocations { // 설정 해석 전 확정한 원문 지역 경로들의 소유권이다.
    std::filesystem::path installDirectory; // 원문 chdir(prefix+..)/getcwd와 대응하는 실제 기본 파일 루트다.
    std::string dataPrefix; // setup.cfg가 있는 원문 작성 경로이며 Load까지 별도 복사로 유지한다.
    std::string cdDirectory; // 실제 탐색 결과 또는 기본 루트로 정한 보조 루트다.
    std::unique_ptr<o::FileSpecQuery> archiveSearch; // 원문마지막TAFF검색핸들을설정Load/InstallDir/CDDir작성뒤까지소유한다.
};
StartupFileLocations PrepareStartupFiles(o::BaseFileSystem& files,const std::filesystem::path& gameRoot,const std::function<std::string()>& findCd); // 탐색→CD→부모 경로→기본/보조 TAFF 순서로 실제 VFS를 준비한다. 프로세스 cwd는 변경하지 않는다.
}
