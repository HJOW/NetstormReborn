// 원본 텍스트FILE→바이너리BaseFile의 경계를 같은 실제 파일에서 처리한다.
#include "client/ObjectDumpDisk.h"
#include "o/BaseFile.h"
#include "o/RawBaseFileIO.h"
#include "o/RawBaseFileReadPolicy.h"
#include "o/RawFileName.h"
#include "platform/Utf8Path.h"
#include <cstdio>
#include <array>
#include <limits>
#include <memory>
#include <stdexcept>

namespace netstorm::client { // 장치의 호스트 포인터/수명은 게임 슬롯에 직렬화하지 않는다.
namespace { // 플랫폼 파일 API만 이 경계에서 대체한다.
// UTF-8바이트를C++20의char8_t경로로변환하여시스템ANSI코드페이지를거치지않는다.
std::filesystem::path Utf8Path(std::string_view value) { return std::filesystem::path(std::u8string(value.begin(),value.end())); }
// Windows10/11의UTF-8경로를wide CRT로연결한다. 원본ANSI경로와다른호환처리이며파일모드는같다.
FILE* Open(std::string_view path,bool text) {
    const auto physical=Utf8Path(path);
#ifdef _WIN32
    return ::_wfopen(physical.c_str(),text ? L"wt" : L"w+b");
#else
    return std::fopen(physical.c_str(),text ? "wt" : "w+b");
#endif
}
// NULL FILE은 원문 CRT의미정의오류경로이므로앞선효과를유지하고호스트에서거부한다.
FILE* File(TraceLogStream stream) { if (!stream) throw std::logic_error("객체 덤프 NULL FILE 장치");return reinterpret_cast<FILE*>(stream); }
// 원본24BYTE파일객체와짝을이루는호스트읽기/쓰기수명이다. native필드배치를가장하지않는다.
struct Writer {
    o::RawBaseFileState state{}; // 실제 FILE은 호스트 포인터 토큰으로 같은 BaseFile 필드에 보관한다.
    o::RawBaseFileIOHooks hooks; // 이 소비자가 선택하는 원문의 일반 쓰기/소멸 CRT 장치다.
    std::vector<std::uint8_t> key; // TAFF fallback에서 실제 할당한 NUL 포함 키이며 현재 writer가 끝까지 소유한다.
    bool readComplete=true; // 실제 전체읽기에서 부분 fread를 정상파일로 반환하지 않기 위한 호스트 경계다.
    Writer() { // 부분 쓰기를 임의 재시도하지 않고 원문의 size/element 인자를 그대로 사용한다.
        o::ConstructBaseFile(state); // 원문 생성자의 세 필드0을 같은 실제 호스트 상태에 적용한다.
        hooks.write=[](auto token,auto bytes,auto size,auto count) { static_cast<void>(std::fwrite(bytes.data(),size,count,reinterpret_cast<FILE*>(token))); };
        hooks.read=[this](auto token,auto bytes,auto size,auto count) { const auto actual=std::fread(bytes.data(),size,count,reinterpret_cast<FILE*>(token));readComplete=readComplete && (!size || !count || actual==count); }; // 원문 fread 호출은 유지하고 호스트 반환 전에 실제 짧은 읽기를 구별한다.
        hooks.close=[](auto token) { static_cast<void>(std::fclose(reinterpret_cast<FILE*>(token))); };
        hooks.open=[](auto name,auto mode) { // 전체 부모의 일반mode2와 TAFF의rb를 원문의 CRT 모드로 연다.
            const auto path=Utf8Path(name);const std::wstring wide(mode.begin(),mode.end());return reinterpret_cast<std::uintptr_t>(::_wfopen(path.c_str(),wide.c_str()));
        };
        hooks.seek=[](auto token,auto offset,auto origin) { static_cast<void>(std::fseek(reinterpret_cast<FILE*>(token),offset,origin)); };hooks.tell=[](auto token) { return static_cast<std::int32_t>(std::ftell(reinterpret_cast<FILE*>(token))); }; // 부분파일/암호 위치의 원문signed32 CRT 경계다.
        hooks.allocateKey=[this](auto size)->std::uintptr_t { key.resize(size);return 2; };hooks.mutableKey=[this](auto token,auto size) { if (token!=2 || size>key.size()) throw std::out_of_range("덤프 키 쓰기 물리 범위");return std::span(key).first(size); }; // 실제 할당 실패는 앞선 필드 효과를 보존한 채 전파한다.
        hooks.key=[this](auto token,auto size) { if (token!=2 || size>key.size()) throw std::out_of_range("덤프 키 읽기 물리 범위");return std::span<const std::uint8_t>(key).first(size); };hooks.freeKey=[this](auto token) { if (token!=2) throw std::out_of_range("덤프 키 반납 토큰");key.clear(); }; // Close와 키해제를 섞지 않고 실제 writer 소멸 때 반납한다.
        hooks.assertion=[](auto line) { throw std::logic_error("원문 Basefile.cpp 진단 "+std::to_string(line)); }; // 부분파일 쓰기의 원문 진단을 빈 성공으로 바꾸지 않는다. 실제 디버그 UI/SEH는 호스트 진단 경계다.
    }
    ~Writer() { o::DestructBaseFile(state,hooks); } // 정상/호스트 예외 반납에서 원문 Close/키 분리 순서로 현재 FILE을 한 번 닫는다.
};
// 같은동기호출의경로/할당/파일실체를소유한다. 게임전역의모드나FILE을사본으로보관하지않는다.
struct DiskState {
    std::string path; // 이번호출이처음작성한같은절대UTF-8경로다.
    std::filesystem::path directory; // 원문의 chdir 대신 실제 Unicode 게임 CWD를 파일 장치에서만 적용한다.
    const o::BaseFileSystem* source{}; // 실제 Client의 같은 읽기/TAFF/보조 경로이며 동기 덤프보다 오래 살아야 한다.
    std::shared_ptr<o::RawFileName> names; // 실제 VFS와 같은10칸 소유자이며 독립 직접 디스크 호출에서는 비어 있다.
    std::vector<std::uint8_t> allocation; // 원문요청뒤마지막NUL여유BYTE를포함한호스트할당이다.
    std::unique_ptr<Writer> writer; // 실제BaseFile생성/소멸경계와짝인쓰기객체다.
    const std::uint8_t* identity{}; // caller의24BYTE주소이며필드내용을직렬화하지않는호스트식별자다.
    // 모든파일메서드가생성된같은지역물리를빌리는지확인한다.
    void Same(std::span<std::uint8_t,24> file) const { if (!writer || identity!=file.data()) throw std::logic_error("객체 덤프 BaseFile 물리 불일치"); }
};
}
// 원본경로/열기/읽기/할당/생성/재쓰기/소멸의각장치를실제파일작업으로연결한다.
ObjectDumpHooks MakeObjectDumpDiskHooks(const std::filesystem::path& directory,std::function<void(std::uint32_t)> trace,std::function<void()> openFailure,const o::BaseFileSystem* source) {
    if (!trace || !openFailure) throw std::invalid_argument("객체 덤프 실제 trace/assert 장치 필요");
    auto state=std::make_shared<DiskState>();state->directory=std::filesystem::absolute(directory).lexically_normal();const auto path=std::filesystem::absolute(directory/"squids.txt").u8string();state->path.assign(path.begin(),path.end());state->source=source;if (source) state->names=source->FileNameOwner(); // 준비만으로 고리를 소비하거나 파일을 열지 않는다.
    ObjectDumpHooks hooks;hooks.trace=std::move(trace);hooks.openFailure=std::move(openFailure); // 게임계산을새객체목록으로대신하지않는다.
    hooks.path=[state] { return state->names ? o::JoinBaseFileRoot("","squids.txt",*state->names) : state->path; }; //004d6020의 빈00540cec를 실제 한 칸에 작성한다. 논리 `.\\squids.txt`를 호스트 절대 이름으로 대체하지 않는다.
    hooks.open=[state](auto name,auto mode) { if (mode!="wt") throw std::invalid_argument("객체 덤프 CRT파일모드");const auto path=Utf8Path(name);return reinterpret_cast<TraceLogStream>(Open(platform::PathToUtf8(state->source && !path.is_absolute() ? state->directory/path : path),true)); }; // 원문 CRT의 상대 열기만 가상 게임 CWD에서 수행하고 고리는 추가 소비하지 않는다.
    hooks.close=[](auto stream) { std::fclose(File(stream)); };
    hooks.read=[state](auto name,auto first,auto second) { // 실제Client의41baa0도 같은 전체열기/현재TAFF FILE/키를 사용하며 값형VFS snapshot으로 대신하지 않는다.
        if (first || second) throw std::invalid_argument("객체 덤프 전체읽기인자");if (!state->source) return o::ReadFileBytes(Utf8Path(name));
        Writer reader;state->source->Open(reader.state,name,0,0,reader.hooks);const auto length=o::BaseFileLength(reader.state,reader.hooks);if (length<0) throw std::runtime_error("객체 덤프 전체읽기의 signed 길이 실패");
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));std::array<std::uint8_t,4096> scratch{};o::ReadBaseFile(reader.state,bytes,-1,reader.hooks,scratch);if (!reader.readComplete) throw std::runtime_error("객체 덤프 전체읽기의 실제 부분 I/O");return bytes; // 원문 미정 tail을0으로 합성한 정상파일로 내보내지 않는 호스트 경계다.
    };
    hooks.allocate=[state](std::uint32_t size) { state->allocation.resize(static_cast<std::size_t>(size)+1);return std::span<std::uint8_t>(state->allocation); }; // 원본요청의끝NUL1BYTE초과위험만호스트물리에서방지한다.
    hooks.construct=[state](auto file) { if (state->writer) throw std::logic_error("객체 덤프 파일중복생성");state->writer=std::make_unique<Writer>();state->identity=file.data(); };
    hooks.openWrite=[state](auto file,auto name,auto mode,auto extra) {
        state->Same(file);if (mode!=2 || extra || state->writer->state.file) throw std::invalid_argument("객체 덤프 BaseFile쓰기인자");
        if (state->source) { state->source->Open(state->writer->state,name,mode,extra,state->writer->hooks);return; } // 실제 Client는 일반mode2 실패의 TAFF/보조/키까지 같은 전체41b4c0 부모로 처리한다.
        if (!o::OpenBaseFileDisk(state->writer->state,name,mode,extra,state->writer->hooks)) throw ObjectDumpUnableToOpen{}; // 기존 독립 직접 디스크 API의 절대이름/typed 실패 계약은 유지한다.
    };
    hooks.write=[state](auto file,auto bytes,auto offset) { state->Same(file);if (offset!=0xffffffffU || !state->writer->state.file || bytes.size()>std::numeric_limits<std::uint32_t>::max()) throw std::invalid_argument("객체 덤프 BaseFile쓰기범위");o::WriteBaseFile(state->writer->state,bytes,-1,state->writer->hooks);return 0U; }; // 부모는 원문 void 쓰기의 반환 길이를 사용하지 않는다.
    hooks.destruct=[state](auto file) { state->Same(file);state->writer.reset();state->identity=nullptr; };
    return hooks;
}
// 현재FILE과그시점의C문자열출력은fprintf("%s")와같고텍스트모드의CRLF변환은CRT가수행한다.
TraceLogHooks MakeObjectDumpDiskLogHooks(std::function<void(std::string_view)> console) {
    if (!console) throw std::invalid_argument("객체 덤프 실제 console 장치 필요");TraceLogHooks hooks;
    hooks.standardOut=reinterpret_cast<TraceLogStream>(stdout);hooks.standardError=reinterpret_cast<TraceLogStream>(stderr);hooks.console=std::move(console);
    hooks.write=[](auto stream,auto bytes) { std::fwrite(bytes.data(),1,bytes.size(),File(stream)); };
    hooks.flush=[](auto stream) { std::fflush(File(stream)); };
    return hooks;
}
}
