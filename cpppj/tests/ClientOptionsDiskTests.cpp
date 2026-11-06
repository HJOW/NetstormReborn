// 실제 Windows 매핑으로1224를 재현해 설정 바이트/Unicode/changed/실패 복구를 검사한다. 게임/창/음향 장치는 열지 않는다.
#include "TestSupport.h"
#include "client/ClientOptionsDisk.h"
#include "client/ClientOptionsFile.h"
#include "client/ClientOptionsSaveIO.h"
#include "o/ConfigInterface.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <fcntl.h>
#include <io.h>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <thread>
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

// 실제 파이프 핸들을 한 번만 닫는다. CRT로 소유권을 넘긴 쓰기 핸들은 이 소유자에서 해제한다.
struct OptionsPipeHandle {
    HANDLE value{INVALID_HANDLE_VALUE}; // 이 검사만 소유한 읽기/쓰기 장치다.
    ~OptionsPipeHandle() { if (value!=INVALID_HANDLE_VALUE) ::CloseHandle(value); } // 예외 경로에서도 읽기 끝을 닫아 쓰기 대기를 해제한다.
    HANDLE Release() { const auto released=value;value=INVALID_HANDLE_VALUE;return released; } // CRT 또는 읽기 스레드에 소유권을 한 번 넘긴다.
};

// Windows 쓰기 핸들을 실제 CRT FILE로 바꾸며 실패 시 중간 fd까지 반납한다.
std::unique_ptr<FILE,decltype(&std::fclose)> OpenOptionsPipe(OptionsPipeHandle& read,OptionsPipeHandle& write) {
    if (!::CreatePipe(&read.value,&write.value,nullptr,4096)) throw std::runtime_error("옵션 검사 파이프 생성 실패");
    const auto fd=::_open_osfhandle(reinterpret_cast<intptr_t>(write.value),_O_WRONLY|_O_BINARY); // 성공한 fd부터 CRT가 같은 핸들을 소유한다.
    if (fd<0) throw std::runtime_error("옵션 검사 파이프 fd 생성 실패");
    write.Release();
    FILE* stream=::_fdopen(fd,"wb"); // 제품과 같은 fwrite/fflush/fclose 장치를 사용한다.
    if (!stream) { ::_close(fd);throw std::runtime_error("옵션 검사 파이프 FILE 생성 실패"); }
    return {stream,&std::fclose};
}

// 제품 함수가 FILE을 닫은 뒤 OS 핸들도 남지 않았는지 확인한다.
void CheckOptionsHandleClosed(HANDLE handle) {
    DWORD flags{};::SetLastError(ERROR_SUCCESS); // 핸들 조회 실패 원인을 직전 API와 구분한다.
    CHECK(!::GetHandleInformation(handle,&flags) && ::GetLastError()==ERROR_INVALID_HANDLE);
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

// 같은 제품 종료 함수를 일반/내구성 저장에 직접 적용해 Unicode 경로의 바이트와 실제 핸들 반납을 확인한다.
TEST_CASE(ClientOptionsDisk_StreamOwnerClosesNormalAndDurableFiles) {
    for (const auto& root:OptionsDiskRoots()) { // 두 실제 볼륨을 구분한다.
        OptionsDiskDirectory directory(root);const auto path=directory.path/u8"설정😀.cfg"; // 이번 검사만의 Unicode 파일이다.
        const std::vector<std::uint8_t> bytes{0,1,2,3,0xff,0xfe}; // 텍스트/빈문자 처리로 바뀌면 안 되는 원시 값이다.
        for (const bool durable:{false,true}) { // 일반 저장과 교체 전 commit을 각각 실행한다.
            FILE* stream=::_wfopen(path.c_str(),L"w+b");if (!stream) throw std::runtime_error("옵션 검사 FILE 열기 실패");
            const auto handle=reinterpret_cast<HANDLE>(::_get_osfhandle(::_fileno(stream))); // 제품에 넘기기 전의 같은 OS 핸들이다.
            options_detail::FinishOptionsStream(stream,bytes,durable);
            CheckOptionsHandleClosed(handle);CHECK(ReadOptionsDisk(path)==bytes);
        }
    }
}

// 실제 파이프에 접두 바이트가 전달된 뒤 읽기 끝을 닫아 CRT 쓰기 실패와 changed/FILE 수명을 확인한다.
TEST_CASE(ClientOptionsDisk_ActualPartialPipeWriteKeepsChangesAndClosesStream) {
    OptionsPipeHandle read,write;auto stream=OpenOptionsPipe(read,write); // 디스크 공간이나 보호 파일을 손상하지 않는 실제 OS 장치다.
    if (std::setvbuf(stream.get(),nullptr,_IONBF,0)!=0) throw std::runtime_error("옵션 검사 무버퍼 설정 실패");
    const auto handle=reinterpret_cast<HANDLE>(::_get_osfhandle(::_fileno(stream.get()))); // fclose 뒤 반납을 확인할 값만 빌린다.
    netstorm::o::ConfigInterface config({});config.Startup({});config.Set("InstallDir",std::string(65536,'x')); // 파이프 용량보다 큰 실제 설정을 만든다.
    std::vector<std::uint8_t> submitted,received(1024);DWORD count{};BOOL readOk{}; // 본문 전체와 실제 읽힌 접두를 구분한다.
    const auto readerHandle=read.Release(); // 읽기 스레드가 이 핸들을 독점 반납한다.
    std::jthread reader([&] { // 한 번의 실제 부분 읽기 뒤 닫아 진행 중인 큰 동기 쓰기가 실패하도록 한다.
        readOk=::ReadFile(readerHandle,received.data(),static_cast<DWORD>(received.size()),&count,nullptr);
        ::CloseHandle(readerHandle);
    });
    bool failed=false; // 제품 예외와 실제 전달량을 별도로 관측한다.
    try { SaveClientOptions(config,[&](auto,auto bytes) { // changed 수명도 실제 저장 부모를 거친다.
        submitted.assign(bytes.begin(),bytes.end());options_detail::FinishOptionsStream(stream.release(),bytes,false);
    }); }catch (const std::runtime_error&) { failed=true; }
    stream.reset();reader.join(); // 예외가 writer 진입 전에 발생해도 쓰기 끝을 닫은 뒤 읽기 스레드를 회수한다.
    CHECK(failed && config.Configuration().changed && readOk && count>0 && count<submitted.size());
    if (count<=submitted.size()) CHECK(std::equal(received.begin(),received.begin()+count,submitted.begin()));
    CheckOptionsHandleClosed(handle);
    std::printf("Actual options pipe: submitted=%zu received=%lu failed=%d changed=%d\n",submitted.size(),count,failed?1:0,config.Configuration().changed?1:0);
}

// 닫힌 읽기 끝과 실제 CRT 버퍼로 flush 실패를 만들고 제품의 실패/핸들 반납을 확인한다.
TEST_CASE(ClientOptionsDisk_BufferedBrokenPipeFailsAndClosesStream) {
    OptionsPipeHandle read,write;auto stream=OpenOptionsPipe(read,write); // FILE은 성공 경로와 같은 실제 CRT 구현이다.
    const auto handle=reinterpret_cast<HANDLE>(::_get_osfhandle(::_fileno(stream.get()))); // 최종 fclose의 OS 반납을 확인한다.
    std::array<char,8192> buffer{}; // Finish 함수가 돌아올 때까지 살아 있는 출력 버퍼다.
    if (std::setvbuf(stream.get(),buffer.data(),_IOFBF,buffer.size())!=0) throw std::runtime_error("옵션 검사 버퍼 설정 실패");
    ::CloseHandle(read.Release());const std::vector<std::uint8_t> bytes(32,0x5a);bool failed=false; // 버퍼보다 작은 입력과 이미 닫힌 장치다.
    try { options_detail::FinishOptionsStream(stream.release(),bytes,false); }catch (const std::runtime_error&) { failed=true; }
    CHECK(failed);CheckOptionsHandleClosed(handle);
}

// 문서의1176/1177 파일 이름 상태를 실제 두 볼륨에 구성한다. ReplaceFile 자체의 해당 오류 발생을 주장하지 않는다.
TEST_CASE(ClientOptionsDisk_ReplacementFailureStatesPreserveOriginalErrorAndBytes) {
    for (const auto& root:OptionsDiskRoots()) { // C/H의 실제 이동/공유거부 의미를 각각 실행한다.
        for (unsigned state=0;state<5;++state) { // 기존목표/백업복원/둘다존재/복원공유거부/둘다부재를 구분한다.
            OptionsDiskDirectory directory(root);const auto target=directory.path/u8"설정😀.cfg",previous=directory.path/"previous.cfg",next=directory.path/"next.cfg"; // 독립 복구 이름이다.
            const std::vector<std::uint8_t> old{1,2,3},fresh{9,8,7,6},other{5,4}; // 기존/새/경합 목표를 바이트로 구분한다.
            WriteClientOptionsDisk(next,fresh);
            if (state==0) WriteClientOptionsDisk(target,old);
            if (state>=1 && state<=3) WriteClientOptionsDisk(previous,old);
            if (state==2) WriteClientOptionsDisk(target,other);
            OptionsPipeHandle locked; // 복원 공유거부 사례에서만 실제 백업 파일 핸들을 소유한다.
            if (state==3) {
                locked.value=::CreateFileW(previous.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
                if (locked.value==INVALID_HANDLE_VALUE) throw std::runtime_error("옵션 복구 백업 잠금 실패");
            }
            const unsigned long originalError=state==0?ERROR_UNABLE_TO_MOVE_REPLACEMENT:ERROR_UNABLE_TO_MOVE_REPLACEMENT_2; // Windows 문서의 원래 교체 오류를 명시 주입한다.
            bool failed=false; // 복원 성공도 저장 성공으로 바꾸면 안 된다.
            try { options_detail::ThrowOptionsReplacementFailure(target,previous,directory.path,originalError); }
            catch (const std::system_error& error) {
                failed=true;CHECK(error.code()==std::error_code(static_cast<int>(originalError),std::system_category()));
                if (state==3) CHECK(std::string(error.what()).find("restore error: 32")!=std::string::npos);
            }
            CHECK(failed && ReadOptionsDisk(next)==fresh);
            if (state==0 || state==1) CHECK(ReadOptionsDisk(target)==old);
            if (state==1) CHECK(!std::filesystem::exists(previous)); // 성공한 이름 복원은 기존 바이트를 목표로 이동한다.
            if (state==2) CHECK(ReadOptionsDisk(target)==other && ReadOptionsDisk(previous)==old);
            if (state==3) CHECK(!std::filesystem::exists(target) && ReadOptionsDisk(previous)==old);
            if (state==4) CHECK(!std::filesystem::exists(target) && !std::filesystem::exists(previous));
        }
    }
}
