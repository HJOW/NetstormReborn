// 실제 Windows 매핑으로1224를 재현해 설정 바이트/Unicode/changed/실패 복구를 검사한다. 게임/창/음향 장치는 열지 않는다.
#include "TestSupport.h"
#include "client/ClientOptionsDisk.h"
#include "client/ClientOptionsFile.h"
#include "o/ConfigInterface.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <system_error>
#include <vector>

namespace { // 검사 파일은 OS 임시 폴더와 작업공간 extracted의 새 전용 폴더에만 만든다.
using namespace netstorm::client;
// 병행/반복 검사에서 새 전용 폴더를 구분하는 호스트 번호다. 게임 난수는 소비하지 않는다.
std::atomic<unsigned> nextOptionsDiskTest{};

// 명시 부모 아래의 새 폴더만 만들고 절대 경계를 확인한 뒤 그 안의 검사 산출물만 반납한다.
struct OptionsDiskDirectory {
    std::filesystem::path base; // 삭제 가능 범위를 제한하는 기존 절대 부모다.
    std::filesystem::path path; // 이번 사례에서만 쓰는 생성 성공한 새 폴더다.
    explicit OptionsDiskDirectory(const std::filesystem::path& root):base(std::filesystem::absolute(root).lexically_normal()) {
        for (;;) { // 이전 검사 중단의 이름이 남아 있으면 덮지 않고 다음 새 번호를 선택한다.
            path=base/("netstorm-options-disk-"+std::to_string(::GetCurrentProcessId())+"-"+std::to_string(nextOptionsDiskTest.fetch_add(1)));
            if (std::filesystem::create_directory(path)) return;
        }
    }
    ~OptionsDiskDirectory() { // 절대 경로가 명시 부모 바로 아래의 이번 검사 이름일 때만 재귀 정리한다.
        const auto resolved=std::filesystem::absolute(path).lexically_normal(); // 삭제 직전의 실제 대상 경계다.
        if (resolved.parent_path()!=base || !resolved.filename().string().starts_with("netstorm-options-disk-")) return;
        std::error_code ignored;std::filesystem::remove_all(resolved,ignored);
    }
};

// 보호 디렉터리는 읽지도 쓰지도 않으며 작업볼륨과 OS 임시폴더에서 같은 회귀를 실행한다.
std::vector<std::filesystem::path> OptionsDiskRoots() {
    return {std::filesystem::temp_directory_path(),std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted"};
}

// 결과 파일의 원시 바이트를 재적재한다. 기대값을 제품 설정에서 재계산하지 않는다.
std::vector<std::uint8_t> ReadOptionsDisk(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary); // Unicode 경로의 실제 디스크 스트림이다.
    if (!input) throw std::runtime_error("검사 결과 설정 파일 열기 실패");
    return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}

// 실제 읽기 전용 section/view를 소유한다. 공유 삭제 허용 여부는 저장 성공/거부 입력으로 분리한다.
struct OptionsDiskMapping {
    HANDLE file{INVALID_HANDLE_VALUE}; // 원래 파일의 읽기 핸들이며 임의 전역 핸들을 빌리지 않는다.
    HANDLE section{}; // 실제 PAGE_READONLY 파일 매핑이다.
    const std::uint8_t* view{}; // 저장 뒤에도 원래 바이트를 확인하는 같은 view다.
    OptionsDiskMapping(const std::filesystem::path& path,bool shareDelete) {
        file=::CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|(shareDelete?FILE_SHARE_DELETE:0),nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if (file==INVALID_HANDLE_VALUE) throw std::runtime_error("검사 파일 매핑 열기 실패");
        section=::CreateFileMappingW(file,nullptr,PAGE_READONLY,0,0,nullptr);
        if (section) view=static_cast<const std::uint8_t*>(::MapViewOfFile(section,FILE_MAP_READ,0,0,0));
        if (!view) { if (section) ::CloseHandle(section);::CloseHandle(file);throw std::runtime_error("검사 파일 매핑 생성 실패"); }
    }
    void Release() { // 실제 매핑을 명시 반납해 다음 저장의 지연 정리를 확인한다. 반복 호출도 안전하다.
        if (view) { ::UnmapViewOfFile(view);view=nullptr; }
        if (section) { ::CloseHandle(section);section=nullptr; }
        if (file!=INVALID_HANDLE_VALUE) { ::CloseHandle(file);file=INVALID_HANDLE_VALUE; }
    }
    ~OptionsDiskMapping() { Release(); } // view→section→file의 역순으로 닫는다.
    OptionsDiskMapping(const OptionsDiskMapping&)=delete;OptionsDiskMapping& operator=(const OptionsDiskMapping&)=delete; // 실제 핸들의 중복 반납을 막는다.
};

// 제품 수정 전의 w+b 실패가 실제1224인지 확인한다. 성공할 경우에도 FILE은 반납한다.
void CheckOriginalMappedFailure(const std::filesystem::path& path) {
    FILE* original=::_wfopen(path.c_str(),L"w+b"); // 원본/672 제품과 같은 절삭 요청이다.
    unsigned long error{};const auto status=::_get_doserrno(&error); // 다른 API 전에 실제 실패 원인을 보존한다.
    if (original) std::fclose(original);
    CHECK(!original && status==0 && error==ERROR_USER_MAPPED_FILE);
}
}

// 설정을 매핑한 채 더 긴 Unicode 설정을 저장하고 새 재적재/이전 view/changed를 동시에 확인한다.
TEST_CASE(ClientOptionsDisk_MappedFileSavesAndReloadsOnBothVolumes) {
    for (const auto& root:OptionsDiskRoots()) { // 두 실제 파일시스템의 같은 사례를 생략하지 않는다.
        OptionsDiskDirectory directory(root);const auto parent=directory.path/u8"설정😀";std::filesystem::create_directory(parent);const auto path=parent/"options.cfg"; // 이번 사례의 Unicode 경로다.
        netstorm::o::ConfigInterface config({});config.Startup({});config.Set("InstallDir","old"); // 실제 설정 직렬화 본체다.
        SaveClientOptions(config,[&](auto,auto bytes) { WriteClientOptionsDisk(path,bytes); });
        const auto original=ReadOptionsDisk(path);OptionsDiskMapping mapping(path,true);CheckOriginalMappedFailure(path); // 실패하는 원문 조건을 실제 OS에서 먼저 확인한다.
        config.Set("InstallDir","H:\\한글\\더 긴 설정😀");CHECK(config.Configuration().changed);
        SaveClientOptions(config,[&](auto,auto bytes) { WriteClientOptionsDisk(path,bytes); });
        const auto saved=ReadOptionsDisk(path); // 새로운 이름으로 연 실제 디스크 파일이다.
        CHECK(!config.Configuration().changed && saved!=original);
        CHECK(netstorm::o::ConfigText::FromBytes(saved).GetRaw("InstallDir")=="H:\\한글\\더 긴 설정😀");
        CHECK(std::equal(original.begin(),original.end(),mapping.view)); // 기존 view는 원래 바이트를 유지한다.
        std::size_t backups{}; // 기존 view가 살아 있는 동안 실제 Windows가 삭제를 거부한 성공 백업 수다.
        for (const auto& entry:std::filesystem::directory_iterator(parent)) { // 이번 성공 저장의 이전 바이트만 남았는지 확인한다.
            if (!entry.is_directory()) continue;
            CHECK(ReadOptionsDisk(entry.path()/"previous.cfg")==original && !std::filesystem::exists(entry.path()/"next.cfg"));++backups;
        }
        CHECK(backups==1); // 두 볼륨의 실제 매핑 수명과 일치하며 임의 파일/변경된 이전 바이트는 허용하지 않는다.
        mapping.Release();WriteClientOptionsDisk(path,saved); // 이전 view 반납 뒤 같은 제품의 다음 일반 저장이 지연 목록을 수거한다.
        CHECK(ReadOptionsDisk(path)==saved && std::distance(std::filesystem::directory_iterator(parent),std::filesystem::directory_iterator())==1);
    }
}

// 삭제 공유 거부 시 교체 실패를 숨기지 않고 이전 설정과 미저장 변경/복구 파일을 보존한다.
TEST_CASE(ClientOptionsDisk_DeniedReplacementPreservesChangesAndOldBytes) {
    for (const auto& root:OptionsDiskRoots()) { // 두 실제 파일시스템의 공유 거부도 확인한다.
        OptionsDiskDirectory directory(root);const auto path=directory.path/"options.cfg"; // 원본과 무관한 검사 파일이다.
        netstorm::o::ConfigInterface config({});config.Startup({});config.Set("InstallDir","old");SaveClientOptions(config,[&](auto,auto bytes) { WriteClientOptionsDisk(path,bytes); });
        const auto original=ReadOptionsDisk(path);OptionsDiskMapping mapping(path,false);CheckOriginalMappedFailure(path); // 절삭1224와 교체 공유거부32를 구별한다.
        config.Set("InstallDir","unsaved");bool failed=false; // writer 예외가 기존 changed를 유지하는지 확인한다.
        try { SaveClientOptions(config,[&](auto,auto bytes) { WriteClientOptionsDisk(path,bytes); }); }
        catch (const std::system_error& error) { failed=true;CHECK(error.code().value()==ERROR_SHARING_VIOLATION); }
        CHECK(failed && config.Configuration().changed && ReadOptionsDisk(path)==original && std::equal(original.begin(),original.end(),mapping.view));
        std::size_t recovery{}; // 실제 복구 경로에 남은 새 바이트의 수다.
        for (const auto& entry:std::filesystem::directory_iterator(directory.path)) { // 이번 폴더의 실제 하위 항목만 조회한다.
            if (!entry.is_directory()) continue;
            const auto next=ReadOptionsDisk(entry.path()/"next.cfg");CHECK(netstorm::o::ConfigText::FromBytes(next).GetRaw("InstallDir")=="unsaved");++recovery;
        }
        CHECK(recovery==1);
    }
}

// 일반 파일의 신규/더 짧은 값/빈 파일 저장은 원문 w+b 의미대로 남는다.
TEST_CASE(ClientOptionsDisk_NormalTruncationAndEmptyBytesRemainSupported) {
    for (const auto& root:OptionsDiskRoots()) { // 기존 두 볼륨의 일반 저장 의미를 확인한다.
        OptionsDiskDirectory directory(root);const auto path=directory.path/"options.cfg"; // 기존 파일을 건드리지 않는 새 출력이다.
        const std::vector<std::uint8_t> longer{1,2,3,4,5},shorter{9}; // 길이가 달라져야 하는 명시 바이트다.
        WriteClientOptionsDisk(path,longer);WriteClientOptionsDisk(path,shorter);CHECK(ReadOptionsDisk(path)==shorter);
        WriteClientOptionsDisk(path,{});CHECK(ReadOptionsDisk(path).empty());
        CHECK(std::distance(std::filesystem::directory_iterator(directory.path),std::filesystem::directory_iterator())==1);
    }
}

// 경로 부재/읽기 전용은1224로 간주하지 않으며 부모 생성이나 오류 무시 없이 실패한다.
TEST_CASE(ClientOptionsDisk_OtherErrorsDoNotCreateFallbackOrClearChanges) {
    for (const auto& root:OptionsDiskRoots()) { // 실패 분기 역시 각 실제 볼륨에서 확인한다.
        OptionsDiskDirectory directory(root);const auto missing=directory.path/"missing"/"options.cfg"; // 없는 부모를 명시한다.
        const std::vector<std::uint8_t> original{1,2,3};bool failed=false; // 보존할 바이트와 실제 예외 관측이다.
        try { WriteClientOptionsDisk(missing,original); }catch (const std::runtime_error&) { failed=true; }
        CHECK(failed && !std::filesystem::exists(missing.parent_path()));
        const auto path=directory.path/"options.cfg";WriteClientOptionsDisk(path,original);CHECK(::SetFileAttributesW(path.c_str(),FILE_ATTRIBUTE_READONLY)); // 직접 열기를 거부하는 실제 속성이다.
        netstorm::o::ConfigInterface config({});config.Startup({});config.Set("InstallDir","changed");failed=false;
        try { SaveClientOptions(config,[&](auto,auto bytes) { WriteClientOptionsDisk(path,bytes); }); }catch (const std::runtime_error&) { failed=true; }
        CHECK(failed && config.Configuration().changed && ReadOptionsDisk(path)==original);
        CHECK(::SetFileAttributesW(path.c_str(),FILE_ATTRIBUTE_NORMAL)); // 정리 전에 이번 속성을 반납한다.
        CHECK(std::distance(std::filesystem::directory_iterator(directory.path),std::filesystem::directory_iterator())==1);
    }
}
