// generic whole read 없이 원본의 두 길이 조회와 같은 저장 칸 직접 쓰기를 수행한다.
#include "client/PaletteFileRead.h"
#include "o/BaseFile.h"
#include "platform/Utf8Path.h"
#include <array>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace netstorm::client { // 원본 File 부모와 실제 호스트 자원 수명을 구별한다.
namespace { // 도달한 필수 장치와 같은 FILE/키의 현대 소유 경계다.
template<class Function> const Function& Need(const Function& function) { // 미선택 장치를 앞서 요구하지 않는다.
    if (!function) throw std::logic_error("팔레트 파일의 선택된 실제 장치 미연결");return function;
}
struct PaletteFileReader { // 완료된 원문 정리와 미완성 Open의 현대 자원 회수를 소유한다.
    o::RawBaseFileState state{}; // 포인터를32비트로 절단하지 않는 현재 FILE/키 필드다.
    o::RawBaseFileIOHooks io; // 검색은 VFS에 남기고 실제 CRT 경계만 연결한다.
    std::vector<std::uint8_t> key; // 같은 File이 후단/닫기까지 NUL 포함 키를 보관한다.
    PaletteFileReader() { // 생성 시 파일을 열지 않으며 기존 읽기/키 계약만 제공한다.
        o::ConstructBaseFile(state);
        io.open=[](auto name,auto mode) { // UTF-8 게임 이름을 Windows의 wide CRT 경로로 연다.
            const auto path=platform::PathFromUtf8(name);
#ifdef _WIN32
            const std::wstring text(mode.begin(),mode.end());return reinterpret_cast<std::uintptr_t>(::_wfopen(path.c_str(),text.c_str()));
#else
            const std::string text(mode);return reinterpret_cast<std::uintptr_t>(std::fopen(path.c_str(),text.c_str()));
#endif
        };
        io.seek=[](auto file,auto offset,auto origin) { static_cast<void>(std::fseek(reinterpret_cast<FILE*>(file),offset,origin)); }; // 원문처럼 CRT 반환은 검사하지 않는다.
        io.tell=[](auto file) { return static_cast<std::int32_t>(std::ftell(reinterpret_cast<FILE*>(file))); }; // Windows long의 signed32 결과다.
        io.read=[](auto file,auto bytes,auto size,auto count) { static_cast<void>(std::fread(bytes.data(),size,count,reinterpret_cast<FILE*>(file))); }; // 부분 읽기의 남은 saved_/작업 BYTE를 보존한다.
        io.close=[](auto file) { static_cast<void>(std::fclose(reinterpret_cast<FILE*>(file))); }; // 후단 적용과 색 표 갱신이 끝난 뒤 정상 닫기를 수행한다.
        io.allocateKey=[this](auto size)->std::uintptr_t { key.resize(size);return 2; }; // 같은 동기 reader의 키 할당 토큰이다.
        io.mutableKey=[this](auto token,auto size) { if (token!=2 || size>key.size()) throw std::out_of_range("팔레트 키 쓰기 물리 범위");return std::span(key).first(size); };
        io.key=[this](auto token,auto size) { if (token!=2 || size>key.size()) throw std::out_of_range("팔레트 키 읽기 물리 범위");return std::span<const std::uint8_t>(key).first(size); };
        io.freeKey=[this](auto token) { if (token!=2) throw std::out_of_range("팔레트 키 반납 토큰");key.clear(); }; // 원문처럼 Close 뒤 키만 반납한다.
        io.assertion=[](auto line) { throw std::logic_error("팔레트 BaseFile 원문 진단 "+std::to_string(line)); };
    }
    ~PaletteFileReader() { o::DestructBaseFile(state,io); } // 이미 정상/예외 반납한 필드는0이다. 미완성 Open의 OS 자원 추가 회수는 현대 호스트 경계다.
};
}
void ReadPaletteFile(o::RawBaseFileState& state,std::string_view name,std::span<std::uint8_t,1024> saved,
    const o::RawBaseFileIOHooks& io,const PaletteFileReadHooks& hooks,std::span<std::uint8_t> scratch) { // 직접 File 부모의 부분 효과를 보존한다.
    state.key=0;state.keyLength=0;
    Need(hooks.open)(state,name,0,1); // 완료 전 예외는 원문의 File cleanup 상태에 들어가지 않는다.
    try {
        if (o::BaseFileLength(state,io)==0x308) {
            o::SeekBaseFile(state,8,0,io);
            for (std::size_t index=0;index<256;++index) { // 각1BYTE Read의 현재 키/부분 읽기 계약과 BGRX 예약 BYTE를 유지한다.
                o::ReadBaseFile(state,saved.subspan(index*4+2,1),-1,io,scratch);
                o::ReadBaseFile(state,saved.subspan(index*4+1,1),-1,io,scratch);
                o::ReadBaseFile(state,saved.subspan(index*4,1),-1,io,scratch);
            }
        } else if (o::BaseFileLength(state,io)==0x400) o::ReadBaseFile(state,saved,-1,io,scratch);
        else Need(hooks.assertion)(0x940);
        Need(hooks.apply)(); // 같은 파일을 열어 둔 채 기존 Screen 후단을 끝까지 실행한다.
    } catch (...) { o::DestructBaseFile(state,io);throw; } // saved_·loaded·화면의 선행 효과를 정상 성공으로 되돌리지 않는다.
    o::DestructBaseFile(state,io); // 정상 소멸 자체가 실패했을 때 다시 소멸하지 않는다.
}
void ReadNativePaletteFile(const o::BaseFileSystem& files,std::string_view name,std::span<std::uint8_t,1024> saved,
    const std::function<void()>& apply) { // 현재 VFS의 metadata와 실제 backing FILE을 함께 사용한다.
    PaletteFileReader reader;std::array<std::uint8_t,4096> scratch{};PaletteFileReadHooks hooks; // 미작성 암호 작업 tail의0은 원문 미정 stack과 같다는 주장이 아니다.
    hooks.open=[&](auto& state,auto path,auto mode,auto create) { files.Open(state,path,mode,create,reader.io); };
    hooks.apply=apply;hooks.assertion=[](auto line) { throw std::logic_error("팔레트 파일 부모의 원문 진단 "+std::to_string(line)); };
    ReadPaletteFile(reader.state,name,saved,reader.io,hooks,scratch);
}
}
