// 원문의 r+b→필요한 경우 w+b, 명시 Close, 지역 소멸을 별개로 유지한다.
#include "client/HelpListDisk.h"
#include "o/BaseFile.h"
#include "o/RawBaseFileIO.h"
#include <cstdio>
#include <limits>
#include <memory>
#include <stdexcept>

namespace netstorm::client { //원본24BYTE와 호스트FILE 포인터의 크기 차이는 이 장치에서만 처리한다.
namespace { //원문의 게임 계산/이름 고리는 기존 BaseFile 몸체에 맡긴다.
//한국어 경로를 OS ANSI 코드페이지로 축소하지 않는 기존 Windows10/11 호환 경계다.
std::filesystem::path Utf8Path(std::string_view text) { return std::filesystem::path(std::u8string(text.begin(),text.end())); }
//한 동기 지역파일의 같은 BaseFile 필드/키/실제 FILE을 소유한다.
struct DiskFile {
    o::RawBaseFileState state{}; //포인터 토큰은 host uintptr이며 게임의 DWORD 물리를 가장하지 않는다.
    o::RawBaseFileIOHooks hooks; //기존 전체열기/쓰기/소멸 몸체의 실제 CRT 장치다.
    std::vector<std::uint8_t> key; //TAFF fallback에서 선택할 수 있는 같은 키를 소멸까지 유지한다.
    std::filesystem::path directory; //프로세스 chdir 대신 이 장치의 상대경로에서만 적용하는 게임 CWD다.
    DiskFile(std::filesystem::path path):directory(std::move(path)) { //원문 construct의 세 필드만 초기화하고 실제 열기는 부모가 결정한다.
        o::ConstructBaseFile(state);
        hooks.open=[this](auto name,auto mode) { const auto logical=Utf8Path(name),physical=logical.is_absolute() ? logical : directory/logical; //원문이 선택한 정확한 모드이며 기존파일을 truncate하지 않는다.
#ifdef _WIN32
            const std::wstring wide(mode.begin(),mode.end());return reinterpret_cast<std::uintptr_t>(::_wfopen(physical.c_str(),wide.c_str()));
#else
            const std::string narrow(mode);return reinterpret_cast<std::uintptr_t>(std::fopen(physical.c_str(),narrow.c_str()));
#endif
        };
        hooks.write=[](auto token,auto bytes,auto size,auto count) { static_cast<void>(std::fwrite(bytes.data(),size,count,reinterpret_cast<FILE*>(token))); }; //원문 한 element의 짧은 쓰기/반환을 재시도하지 않는다.
        hooks.close=[](auto token) { static_cast<void>(std::fclose(reinterpret_cast<FILE*>(token))); };
        hooks.seek=[](auto token,auto offset,auto origin) { static_cast<void>(std::fseek(reinterpret_cast<FILE*>(token),offset,origin)); };hooks.tell=[](auto token) { return static_cast<std::int32_t>(std::ftell(reinterpret_cast<FILE*>(token))); }; //키/부분 파일의 같은 signed32 위치 경계다.
        hooks.allocateKey=[this](auto size)->std::uintptr_t { key.resize(size);return 1; };hooks.mutableKey=[this](auto token,auto size) { if (token!=1 || size>key.size()) throw std::out_of_range("Help 목록 키 쓰기 물리 범위");return std::span(key).first(size); }; //전체열기에서 실제 키 할당을 선택한 경우에만 사용한다.
        hooks.key=[this](auto token,auto size) { if (token!=1 || size>key.size()) throw std::out_of_range("Help 목록 키 읽기 물리 범위");return std::span<const std::uint8_t>(key).first(size); };hooks.freeKey=[this](auto token) { if (token!=1) throw std::out_of_range("Help 목록 키 반납 토큰");key.clear(); };
        hooks.assertion=[](auto line) { throw std::logic_error("원문 Basefile.cpp 진단 "+std::to_string(line)); }; //TAFF 부분파일 쓰기 같은 진단을 정상파일로 숨기지 않는다.
    }
    ~DiskFile() { o::DestructBaseFile(state,hooks); } //명시 Close 뒤에는FILE0이어서 중복 fclose하지 않으며 키는 여기서 반납한다.
};
//지역24BYTE 주소와 실제 writer의 하나뿐인 생존 구간을 묶는다.
struct DiskState {
    std::filesystem::path directory; //호스트의 절대 정규화 게임 CWD다.
    const o::BaseFileSystem* source{}; //실제 Client와 같은 VFS/경로 고리이며 반환 hooks보다 오래 살아야 한다.
    std::unique_ptr<DiskFile> file; //부분 열기/예외도 동일 원문 소멸로 정리한다.
    const std::uint8_t* identity{}; //지역 물리의 주소만 빌리며 내용을 읽거나 직렬화하지 않는다.
    void Same(QuickHelpListFile value) const { if (!file || value.data()!=identity) throw std::logic_error("Help 목록의 지역 BaseFile 물리 불일치"); } //다른 지역/이미 반납한 파일은 사용하지 않는다.
};
}
//같은 실제 전체열기 정책을 사용하되 OS 상대경로만 가상 게임 CWD로 옮긴다.
QuickHelpListFileHooks MakeHelpListDiskHooks(const std::filesystem::path& directory,const o::BaseFileSystem* source) {
    auto state=std::make_shared<DiskState>();state->directory=std::filesystem::absolute(directory).lexically_normal();state->source=source;
    QuickHelpListFileHooks hooks;
    hooks.construct=[state](auto file) { if (state->file) throw std::logic_error("Help 목록 파일 중복 생성");state->file=std::make_unique<DiskFile>(state->directory);state->identity=file.data(); };
    hooks.open=[state](auto file,auto name,auto mode,auto create) { state->Same(file);if (mode!=1 || create!=1 || state->file->state.file) throw std::invalid_argument("Help 목록의 원문 열기 인자");
        if (state->source) state->source->Open(state->file->state,name,mode,create,state->file->hooks); //같은 FileName10칸/TAFF/보조경로/예외 부모를 우회하지 않는다.
        else if (!o::OpenBaseFileDisk(state->file->state,name,mode,create,state->file->hooks)) throw std::runtime_error("Help 목록 파일 열기 실패"); //독립 장치 검사도 같은 r+b/create 모드를 사용한다.
    };
    hooks.write=[state](auto file,auto bytes,auto offset) { state->Same(file);if (!state->file->state.file || offset!=-1 || bytes.size()>std::numeric_limits<std::uint32_t>::max()) throw std::invalid_argument("Help 목록의 원문 쓰기 범위");o::WriteBaseFile(state->file->state,{reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size()},offset,state->file->hooks); };
    hooks.close=[state](auto file) { state->Same(file);o::CloseBaseFile(state->file->state,state->file->hooks); };
    hooks.destruct=[state](auto file) { state->Same(file);state->file.reset();state->identity=nullptr; };
    return hooks;
}
}
