// 원문 경로 작성 순서는 보존하고 Windows10/11의Unicode/cwd경계만 OS 경로로 대체한다.
#include "client/StartupFiles.h"
#include "o/FileSpecQuery.h"
#include "o/RawFileName.h"
#include "o/StartupArchiveFiles.h"
#include "platform/Utf8Path.h"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace netstorm::client { // 실제 원문 부모의 경로 수명을 같은 장기 소유자에 연결한다.
std::string LocateStartupData(std::string_view gameRoot,o::RawFileName& names,const StartupFileProbe& probe) {
    if (!probe) throw std::invalid_argument("Startup의 실제 파일 존재 조회 장치 없음");
    constexpr std::array<std::string_view,6> paths{{".\\",".\\d\\","..\\d\\","..\\..\\d\\","..\\","..\\..\\"}}; //0053256c의원문순서이며성공뒤다른후보를찾지않는다.
    for (const auto prefix:paths) { // 원문은 지역 복사를 정규화해Probe에 주고 성공하면 원래 고리 뷰로FileSpec을 만든다.
        const std::array<o::FileNamePart,3> parts{{{-2,gameRoot},{-2,prefix},{-3,"setup.cfg"}}};const auto formatted=names.Format(parts);auto candidate=std::string(formatted);std::replace(candidate.begin(),candidate.end(),'/','\\');
        if (!probe(candidate)) continue;
        const o::FileSpecQuery spec(formatted);const auto directory=names.Directory(spec.Text());const auto device=names.Device(spec.Text());const std::array<o::FileNamePart,2> result{{{-1,device},{-2,directory}}};return std::string(names.Format(result));
    }
    throw std::logic_error("\\Ns\\O\\Configinterface.cpp:195: i<COUNTOF(searchPaths)");
}

StartupFileLocations PrepareStartupFiles(o::BaseFileSystem& files,const std::filesystem::path& gameRoot,const std::function<std::string()>& findCd) { // 초기 파일 위치를 확정하고 실제 Client의 reader가 참조하는 같은 VFS 객체를 갱신한다.
    const auto owner=files.FileNameOwner();StartupFileLocations result;
    const auto absoluteRoot=std::filesystem::absolute(gameRoot).lexically_normal(); // 원본과 다른 Windows10/11 호환: 상대 --run도 호출자의 cwd에 대해 한 번 확정해 VFS 기본 경로를 중복해서 붙이지 않는다.
    result.dataPrefix=LocateStartupData(platform::PathToUtf8(absoluteRoot),*owner,[&files](auto name) { return files.Probe(name); });
    const auto cd=findCd ? findCd():std::string{};const std::array<o::FileNamePart,2> parentParts{{{-2,result.dataPrefix},{-2,"..\\"}}};const auto parent=std::string(owner->Format(parentParts));
    // 원본과 다른 Windows10/11 호환: chdir/getcwd 대신 Unicode 절대 파일 루트를 사용해 다른 스레드/로컬 도구의 프로세스 cwd를 바꾸지 않는다.
    result.installDirectory=std::filesystem::absolute(platform::PathFromUtf8(parent)).lexically_normal();if (result.installDirectory.filename().empty()) result.installDirectory=result.installDirectory.parent_path();
    if (!std::filesystem::is_directory(result.installDirectory)) throw std::logic_error("\\Ns\\O\\Configinterface.cpp:215: result == 0");
    result.cdDirectory=cd.empty() ? platform::PathToUtf8(result.installDirectory):cd;
    // cpppj의독립Client생성자가미리읽은목록을최초Startup에가져오지않는다. 원문최초BSS의두빈목록에서선택한root만등록하며경로고리소유자는유지한다.
    files=o::BaseFileSystem(result.installDirectory,{},owner);o::StartupArchiveHooks hooks;
    hooks.registerFile=[&files,&result](auto name,bool secondary) { auto path=platform::PathFromUtf8(name);if (path.is_relative()) path=result.installDirectory/path;static_cast<void>(files.RegisterArchive(path,secondary)); }; // 원본chdir뒤상대입력도같은Unicode가상기본root에서연다.
    hooks.setSecondary=[&files](auto directory) { files.SetSecondaryDirectory(platform::PathFromUtf8(directory)); };
    const auto primary=platform::PathToUtf8(result.installDirectory);result.archiveSearch=o::BeginStartupArchives(primary,cd.empty()?std::nullopt:std::optional<std::string_view>{cd},*owner,hooks);
    return result;
}
}
