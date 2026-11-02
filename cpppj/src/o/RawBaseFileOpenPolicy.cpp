// FileSpec/10칸/실제 FILE 몸체의 호출 순서와 부분 효과를 같은 상태에서 유지한다.
#include "o/RawBaseFileOpenPolicy.h"
#include "o/RawBaseFileReadPolicy.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::o { // 원문 장치 분기와 호스트 OS 장치는 명시 콜백 경계에서 만난다.
namespace { // 실제 선택한 하위만 요구하며 원문 C문자열 읽기를 물리 범위 안에서 수행한다.
template<class F> const F& Need(const F& callback) { if (!callback) throw std::logic_error("BaseFile 열기 부모의 선택된 장치 미연결");return callback; } // 미선택 경계는 선행 요구하지 않는다.
std::string_view Text(std::span<const std::uint8_t> bytes) { // callback이 현재 NUL 위치를 바꿨으면 다음 원문 소비 시점에 다시 찾는다.
    const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("BaseFile 열기 이름의 NUL 물리 누락");return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())};
}
std::string_view Text(std::string_view value) { return value.substr(0,value.find('\0')); } // 전역 root도 현재 첫NUL까지만 사용한다.
[[noreturn]] void Unable(std::string_view name,const BaseFileOpenPolicyHooks& hooks) { Need(hooks.unableToOpen)(name);throw std::logic_error("BaseFile의 원문 비복귀 예외 장치가 복귀함"); } // 원문 throw를 빈 성공으로 바꾸지 않는다.
constexpr std::string_view kArchiveKey="mydoghasfleas"; // 원문 키 리터럴의 뒤NUL까지 SetBaseFileKey에 공급한다.
}
// FileSpec을 끝까지 소유하며 원래 이름의 디스크/TAFF/보조 분기와 선택된 키 설치를 순서대로 수행한다.
void OpenBaseFileByPolicy(RawBaseFileState& state,std::span<const std::uint8_t> name,std::uint32_t mode,std::uint32_t create,RawFileName& names,const RawBaseFileIOHooks& io,const BaseFileOpenPolicyHooks& hooks) {
    FileSpecQuery spec(Text(name),hooks.search);const bool device=!names.Device(spec.Text()).empty(); // 실제 FileSpec 문자열만 장치 getter에 넘기며 원래 열기 이름은 바꾸지 않는다.
    const auto archive=[&](std::span<const std::uint8_t> query,bool secondary) { // 파일명/offset/length/encrypted는 해당41a340 한 번의 지역 결과다.
        const auto candidate=Need(hooks.archive)(Text(query),secondary);if (!candidate) return false;
        if (!OpenBaseFileSubfile(state,candidate->fileName,candidate->offset,candidate->length,io)) Unable(candidate->fileName,hooks);
        if (candidate->encrypted) SetBaseFileKey(state,std::span(reinterpret_cast<const std::uint8_t*>(kArchiveKey.data()),kArchiveKey.size()+1),io);
        return true;
    };
    if (!device) {
        const auto primary=JoinBaseFileRoot(Text(Need(hooks.primaryRoot)()),Text(name),names);if (OpenBaseFileDisk(state,primary,mode,create,io)) return;
        if (archive(name,false)) return;
        const auto secondary=JoinBaseFileRoot(Text(Need(hooks.secondaryRoot)()),Text(name),names);if (OpenBaseFileDisk(state,secondary,mode,create,io)) return;
        if (archive(name,true)) return;Unable(Text(name),hooks);
    }
    if (OpenBaseFileDisk(state,Text(name),mode,create,io)) return;
    if (mode) Unable(Text(name),hooks); // 원문 장치 경로의 비읽기 실패는 TAFF/보조 디스크를 조회하지 않는다.
    const auto root=Text(Need(hooks.primaryRoot)());const auto current=Text(name);std::size_t common{}; // 구분자/전체접두 관문 없이 첫 불일치 BYTE까지 이동한 원문 포인터다.
    while (common<current.size() && common<root.size() && current[common]==root[common]) ++common; // 대소문자나 UTF-8 문자 경계를 정규화하지 않는다.
    const auto suffix=name.subspan(common);if (archive(suffix,false)) return;
    const auto secondary=JoinBaseFileRoot(Text(Need(hooks.secondaryRoot)()),Text(suffix),names);if (OpenBaseFileDisk(state,secondary,0,create,io)) return;
    if (archive(suffix,true)) return;Unable(Text(name),hooks);
}
}
