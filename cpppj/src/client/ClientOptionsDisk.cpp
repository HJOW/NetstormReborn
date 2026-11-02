// 원본은 매번 w+b로 설정을 잘랐다. Windows10/11에서 다른 프로세스가 파일을 매핑했을 때의1224만 교체 저장으로 보완한다.
#include "client/ClientOptionsDisk.h"
#include "client/ClientOptionsFile.h"
#include "o/RawBaseFileReadPolicy.h"
#include "platform/Utf8Path.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <io.h>
#include <mutex>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace netstorm::client { // 제품 설정 저장의 실제 OS 경계이며 인코딩과 changed 수명은 기존 호출자가 담당한다.
namespace { // 교체 파일은 같은 볼륨의 독립 임시 폴더 안에서만 만들고 원본 파일을 직접 지우지 않는다.
// 충돌 없는 임시 폴더 이름을 현재 프로세스 안에서 구분한다. 파일 내용/게임 난수와 관계없는 호스트 값이다.
std::atomic<std::uint64_t> nextOptionsSave{};

// 이번 저장이 생성한 세 경로만 정리한다. 매핑된 이전 바이트의 삭제 거부는 재시도 대상으로 남긴다.
bool RemoveOptionsSaveFiles(const std::filesystem::path& directory,const std::filesystem::path& next,const std::filesystem::path& previous) {
    ::DeleteFileW(next.c_str());::DeleteFileW(previous.c_str());
    return ::RemoveDirectoryW(directory.c_str())!=0;
}

// 성공한 저장의 임시 파일만 다음 저장/프로세스 종료 때 다시 정리한다. 실패한 교체의 복구 파일은 등록하지 않는다.
struct CompletedOptionsSaves {
    struct Files { // 같은 저장이 독점 생성한 경로를 값으로 보존한다.
        std::filesystem::path directory,next,previous; // 정리 중 다른 경로를 열거하거나 재귀 삭제하지 않는다.
    };
    std::mutex mutex; // 여러 설정 저장 호출의 정리 목록 접근을 직렬화한다.
    std::vector<Files> pending; // 외부 매핑이 아직 닫히지 않아 삭제가 거부된 성공 저장만 보관한다.
    void Collect() { // 현재 삭제 가능한 성공 저장을 정리하고 나머지는 그대로 보존한다.
        const std::lock_guard lock(mutex); // 삭제와 목록 변경의 같은 임계구역이다.
        for (auto item=pending.begin();item!=pending.end();) { // 등록된 이번 프로세스의 명시 경로만 다시 확인한다.
            if (RemoveOptionsSaveFiles(item->directory,item->next,item->previous)) item=pending.erase(item);
            else ++item;
        }
    }
    void Add(const std::filesystem::path& directory,const std::filesystem::path& next,const std::filesystem::path& previous) { // 정상 교체의 정리 실패만 등록한다.
        const std::lock_guard lock(mutex);pending.push_back({directory,next,previous});
    }
    ~CompletedOptionsSaves() { Collect(); } // 프로세스 종료 때도 아직 남은 명시 성공 저장만 한 번 더 정리한다.
};
CompletedOptionsSaves completedOptionsSaves; // 파일 매핑이 저장 호출보다 오래 사는 실제 Windows 수명을 보완하는 장기 정리 목록이다.

// 진단에 경로를 현재 ANSI 코드 페이지의 손실 없이 기록한다.
std::string DiskPathText(const std::filesystem::path& path) {
    const auto bytes=path.u8string(); // Windows native 경로의 실제 UTF-8 표현이다.
    return {bytes.begin(),bytes.end()};
}

// 스트림의 바이트/flush/close를 모두 확인한다. 실패해도 FILE을 닫고 원래 오류를 호출자에게 전달한다.
void FinishOptionsStream(FILE* file,std::span<const std::uint8_t> bytes,bool durable) {
    const auto written=std::fwrite(bytes.data(),1,bytes.size(),file); // 짧은 쓰기는 정상 저장으로 취급하지 않는다.
    const int flushed=std::fflush(file); // 교체용 파일은 버퍼를 먼저 비운 뒤 실제 디스크 flush를 요청한다.
    const int committed=durable && written==bytes.size() && flushed==0 ? ::_commit(::_fileno(file)) : 0;
    const int closed=std::fclose(file); // 앞선 실패 여부와 무관하게 같은 FILE을 한 번 반납한다.
    if (written!=bytes.size() || flushed!=0 || committed!=0 || closed!=0) throw std::runtime_error("Unable to write d/options.cfg");
}

// 같은 부모 안에 충돌 없는 폴더를 만든다. 부모 자체는 만들지 않으며 기존 폴더나 파일을 재사용하지 않는다.
std::filesystem::path MakeOptionsSaveDirectory(const std::filesystem::path& path) {
    for (unsigned attempt=0;attempt<64;++attempt) { // 다른 프로세스/이전 중단과 이름이 겹칠 때만 새 번호를 시도한다.
        const auto serial=nextOptionsSave.fetch_add(1,std::memory_order_relaxed); // 게임 난수는 소비하지 않는다.
        const auto name=path.filename().wstring()+L".save-"+std::to_wstring(::GetCurrentProcessId())+L"-"+std::to_wstring(serial); // 이번 저장 전용 이름이다.
        const auto directory=path.parent_path()/name; // 교체/백업이 다른 볼륨으로 이동하지 않게 같은 부모를 사용한다.
        if (::CreateDirectoryW(directory.c_str(),nullptr)) return directory;
        const auto error=::GetLastError(); // 다른 API를 호출하기 전에 실제 실패 원인을 보존한다.
        if (error!=ERROR_ALREADY_EXISTS) throw std::system_error(static_cast<int>(error),std::system_category(),"Create options save directory: "+DiskPathText(directory));
    }
    throw std::runtime_error("Unable to allocate unique options save directory");
}

// 성공한 교체나 교체 전 실패의 임시 파일만 반납한다. 실패한 ReplaceFile의 복구 자료는 의도적으로 보존한다.
struct OptionsSaveFiles {
    std::filesystem::path directory; // 이번 저장이 독점 생성한 같은 볼륨 폴더다.
    std::filesystem::path next; // 끝까지 기록하고 닫은 뒤 교체에 넘길 새 설정이다.
    std::filesystem::path previous; // ReplaceFile이 보존하는 교체 전 바이트이며 실패 시 복구에 쓴다.
    bool retain{}; // 교체 실패의 실제 파일 상태를 임의로 삭제하지 않게 한다.
    explicit OptionsSaveFiles(const std::filesystem::path& path):directory(MakeOptionsSaveDirectory(path)),next(directory/L"next.cfg"),previous(directory/L"previous.cfg") {} // 원본은 여기서 열거나 자르지 않는다.
    ~OptionsSaveFiles() { // 정상 저장 후 백업/새 파일과 빈 폴더만 반납한다. 삭제 실패가 저장 성공을 뒤집지는 않는다.
        if (retain) return;
        if (RemoveOptionsSaveFiles(directory,next,previous)) return;
        try { completedOptionsSaves.Add(directory,next,previous); }
        catch (...) { std::fprintf(stderr,"Saved options cleanup could not be queued\n"); } // 기록 실패에서도 이미 저장한 설정이나 이전 매핑을 지우지 않는다.
    }
};

// 매핑 중인 원래 파일을 자르지 않고 완성된 바이트를 설치한다. 백업은 메타데이터/공유 권한 오류의 복구용이다.
void ReplaceMappedOptions(const std::filesystem::path& path,std::span<const std::uint8_t> bytes) {
    OptionsSaveFiles files(path); // 교체 전 오류에서는 원래 파일이 그대로 남는다.
    FILE* output=::_wfopen(files.next.c_str(),L"w+b"); // 새 임시 파일만 원문 모드로 연다.
    if (!output) throw std::system_error(errno,std::generic_category(),"Open replacement options");
    FinishOptionsStream(output,bytes,true);
    if (::ReplaceFileW(path.c_str(),files.next.c_str(),files.previous.c_str(),0,nullptr,nullptr)) return;
    const auto error=::GetLastError(); // 원래 ReplaceFile 오류를 복구 시도의 오류와 혼동하지 않는다.
    files.retain=true; // 드문1176/1177에서도 이전/다음 바이트를 버리지 않는다.
    // ReplaceFile이 이전 파일을 백업 이름으로 옮긴 뒤 실패했으면, 사라진 목표 이름만 덮어쓰기 없이 복원한다.
    if (::GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES && ::GetFileAttributesW(files.previous.c_str())!=INVALID_FILE_ATTRIBUTES)
        static_cast<void>(::MoveFileExW(files.previous.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH));
    throw std::system_error(static_cast<int>(error),std::system_category(),"Replace mapped options; recovery files: "+DiskPathText(files.directory));
}
}
void SaveClientOptionsInGameDirectory(o::ConfigInterface& config,const std::filesystem::path& gameDirectory,o::RawFileName& names) { // 원본 chdir/ANSI 대신 Windows10/11의 Unicode 설치 경로를 W 파일 장치에 전달한다.
    SaveClientOptions(config,[&gameDirectory,&names](std::string_view name,std::span<const std::uint8_t> bytes) {
        const bool device=!o::ReadBaseFileDevice(name,names).empty();const auto selected=device ? std::string(name):o::JoinBaseFileRoot(platform::PathToUtf8(gameDirectory),name,names); //41b4c0의 성공 쓰기도 장치 한 칸·선택된 기본 경로 한 칸을 사용한다.
        const auto path=std::filesystem::absolute(platform::PathFromUtf8(selected)).lexically_normal(); // 선택한 root를 다시 붙이지 않으며 상대 게임 디렉터리도 원래 위치다. 사용자 홈 이동은5차다.
        WriteClientOptionsDisk(path,bytes); // 매핑1224의 기존 호환 정책과 부모 폴더 미생성을 그대로 쓴다.
    },names);
}

// 원본 w+b 성공 경로는 유지한다. 매핑 오류를 다른 권한/경로/디스크 오류와 구별해 유일한 호환 경로로 보낸다.
void WriteClientOptionsDisk(const std::filesystem::path& path,std::span<const std::uint8_t> bytes) {
    completedOptionsSaves.Collect(); // 이전 저장의 매핑이 해제됐다면 다음 일반 저장에서도 같은 전용 폴더를 정리한다.
    FILE* file=::_wfopen(path.c_str(),L"w+b"); // Windows10/11의 한글/보충 문자 경로를 보존하는 기존 호환 열기다.
    if (file) { FinishOptionsStream(file,bytes,false);return; }
    const auto lastError=::GetLastError(); // CRT가 갱신하지 않았을 수 있으므로 별도 보조 진단으로만 기록한다.
    const int openError=errno; // 뒤의 진단/API 전에 CRT 오류를 보존한다.
    unsigned long dosError{}; // _get_doserrno 성공 여부와 실제 Windows 오류를 구별한다.
    const auto dosStatus=::_get_doserrno(&dosError); // 저장된 실제 DOS 오류만 폴백 선택에 사용한다.
    if (dosStatus==0 && dosError==ERROR_USER_MAPPED_FILE) { ReplaceMappedOptions(path,bytes);return; }
    std::fprintf(stderr,"SaveOptions fopen failed: path=\"%s\" errno=%d doserrno_read_status=%d doserrno_valid=%d doserrno=%lu last_error=%lu last_error_may_be_stale=1\n",
        DiskPathText(path).c_str(),openError,dosStatus,dosStatus==0?1:0,dosError,static_cast<unsigned long>(lastError));
    throw std::runtime_error("Unable to open d/options.cfg");
}
}
