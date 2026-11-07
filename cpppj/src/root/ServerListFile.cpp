// R 문자열 표의 실제 binary 줄 읽기와 사용자 홈 저장을 복원한다.
#include "root/ServerList.h"
#include "root/SharedFiles.h"
#include "platform/Utf8Path.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <system_error>

namespace netstorm::root { // 원문 상대 경로 입력과 사용자 Saves 출력을 구별한다.
namespace { // 실제 wide FILE의 정상/예외 반납과 OS 오류를 처리한다.
using File=std::unique_ptr<std::FILE,decltype(&std::fclose)>; // BaseFile 소멸처럼 자기 FILE 하나를 반드시 반납한다.
File Open(const std::filesystem::path& path,const wchar_t* mode) { auto* file=::_wfopen(path.c_str(),mode);if (!file) throw std::system_error(errno,std::generic_category(),"R 서버 목록 파일 열기 실패");return File(file,&std::fclose); } // 원문 선택된 파일 열기 실패를 빈 목록 성공으로 바꾸지 않는다.
}
void ServerList::Reload() {
    const std::array<o::FileNamePart,3> parts{{{-2,".."},{-2,"d"},{-3,"rootservers.dat"}}};const auto name=std::string(names_.Format(parts));const auto path=storage_.WorkingFile(platform::PathFromUtf8(name));std::error_code error;const auto present=std::filesystem::exists(path,error);Clear();if (!present || error) return;auto file=Open(path,L"rb");
    while (!std::feof(file.get())) { // 원문의 EOF 판정은 실패한 마지막 fread 뒤에야 참이 된다.
        std::array<char,257> line{};char byte{};std::uint32_t offset{};
        for (;;) { // 첫256BYTE만 보관하되 긴 줄도 실제 LF/EOF까지 끝까지 소비한다.
            static_cast<void>(std::fread(&byte,1,1,file.get()));if (offset<256) line[offset]=byte;if (offset==static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())) throw std::out_of_range("R 서버 목록 줄 길이의 원문 signed 범위");++offset;if (byte=='\n' || std::feof(file.get())) break;if (std::ferror(file.get())) throw std::system_error(errno ? errno:static_cast<int>(std::errc::io_error),std::generic_category(),"R 서버 목록 줄 읽기 실패"); // EOF가 난 fread는 이전 byte를 보존하므로 개행 없는 마지막 글자가 한 번 더 복사된다.
        }
        if (flags_&1U) for (auto& value:line) if (value=='\r' || value=='\n') value=0; // 원문은 뒤쪽부터257BYTE 전체 CR/LF를0으로 만들며 첫 내부CR도 문자열을 끝낸다.
        if (line[0]) Add(line.data(),false);
    }
    Recalculate();
}
void ServerList::Save() {
    auto file=Open(storage_.WritableFile("rootservers.dat"),L"w+b");
    for (const auto& entry:entries_) { // 현재 순서의 비NULL C문자열만 쓰고 LF가 없는 행에만 LF 하나를 덧붙인다.
        if (!entry) continue;const auto text=std::string_view(*entry).substr(0,entry->find('\0'));std::fwrite(text.data(),text.size(),1,file.get());if (text.find('\n')==text.npos) std::fwrite("\n",1,1,file.get());
    }
} // 5차 정책상 저장은 기존 설치 파일의 존재를 요구하지 않고 사용자 Saves에 새로 만든다.
}
