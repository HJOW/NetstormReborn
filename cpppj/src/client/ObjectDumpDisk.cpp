// 원본 텍스트FILE→바이너리BaseFile의 경계를 같은 실제 파일에서 처리한다.
#include "client/ObjectDumpDisk.h"
#include "o/BaseFile.h"
#include <cstdio>
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
// 원본24BYTE파일객체와짝을이루는호스트쓰기객체다. native필드배치를가장하지않는다.
struct Writer {
    FILE* stream{}; // 실제바이너리FILE이며소멸시한번닫는다.
    ~Writer() { if (stream) std::fclose(stream); } // 정상/호스트예외반납에서현재FILE을닫는다.
};
// 같은동기호출의경로/할당/파일실체를소유한다. 게임전역의모드나FILE을사본으로보관하지않는다.
struct DiskState {
    std::string path; // 이번호출이처음작성한같은절대UTF-8경로다.
    std::vector<std::uint8_t> allocation; // 원문요청뒤마지막NUL여유BYTE를포함한호스트할당이다.
    std::unique_ptr<Writer> writer; // 실제BaseFile생성/소멸경계와짝인쓰기객체다.
    const std::uint8_t* identity{}; // caller의24BYTE주소이며필드내용을직렬화하지않는호스트식별자다.
    // 모든파일메서드가생성된같은지역물리를빌리는지확인한다.
    void Same(std::span<std::uint8_t,24> file) const { if (!writer || identity!=file.data()) throw std::logic_error("객체 덤프 BaseFile 물리 불일치"); }
};
}
// 원본경로/열기/읽기/할당/생성/재쓰기/소멸의각장치를실제파일작업으로연결한다.
ObjectDumpHooks MakeObjectDumpDiskHooks(const std::filesystem::path& directory,std::function<void(std::uint32_t)> trace,std::function<void()> openFailure) {
    if (!trace || !openFailure) throw std::invalid_argument("객체 덤프 실제 trace/assert 장치 필요");
    auto state=std::make_shared<DiskState>();const auto path=std::filesystem::absolute(directory/"squids.txt").u8string();state->path.assign(path.begin(),path.end());
    ObjectDumpHooks hooks;hooks.trace=std::move(trace);hooks.openFailure=std::move(openFailure); // 게임계산을새객체목록으로대신하지않는다.
    hooks.path=[state] { return state->path; };
    hooks.open=[](auto name,auto mode) { if (mode!="wt") throw std::invalid_argument("객체 덤프 CRT파일모드");return reinterpret_cast<TraceLogStream>(Open(name,true)); };
    hooks.close=[](auto stream) { std::fclose(File(stream)); };
    hooks.read=[](auto name,auto first,auto second) { if (first || second) throw std::invalid_argument("객체 덤프 전체읽기인자");return o::ReadFileBytes(Utf8Path(name)); };
    hooks.allocate=[state](std::uint32_t size) { state->allocation.resize(static_cast<std::size_t>(size)+1);return std::span<std::uint8_t>(state->allocation); }; // 원본요청의끝NUL1BYTE초과위험만호스트물리에서방지한다.
    hooks.construct=[state](auto file) { if (state->writer) throw std::logic_error("객체 덤프 파일중복생성");state->writer=std::make_unique<Writer>();state->identity=file.data(); };
    hooks.openWrite=[state](auto file,auto name,auto mode,auto extra) {
        state->Same(file);if (mode!=2 || extra || state->writer->stream) throw std::invalid_argument("객체 덤프 BaseFile쓰기인자");
        state->writer->stream=Open(name,false);if (!state->writer->stream) throw ObjectDumpUnableToOpen{};
    };
    hooks.write=[state](auto file,auto bytes,auto offset) { state->Same(file);if (offset!=0xffffffffU || !state->writer->stream || bytes.size()>std::numeric_limits<std::uint32_t>::max()) throw std::invalid_argument("객체 덤프 BaseFile쓰기범위");return static_cast<std::uint32_t>(std::fwrite(bytes.data(),1,bytes.size(),state->writer->stream)); };
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
