// DataManager의 원문 전체 읽기 할당을 논리 길이/미발견과 구별한다.
#pragma once
#include "o/RawBaseFileWholeRead.h"
#include <optional>
#include <string_view>
#include <vector>

namespace netstorm::o { class BaseFileSystem; } // 실제 현재 root/TAFF/이름 고리 소유자를 빌린다.
namespace netstorm::client { // 기존 값형 reader를 유지한 명시 Native DataManager 읽기 경계다.
struct TemplateFileLoadHooks; // 실제 Template 부모의 나머지 권한/서명/현재 슬롯 효과를 보존한다.
struct DataManagerFileAllocation { // optional 없음은 미발견이며 length0인 존재 파일도 최소4BYTE를 소유한다.
    std::vector<std::uint8_t> storage; // 실제 parser가 빌릴 물리 할당이며 DataManager 반납까지 유지한다.
    std::int32_t length{}; // 원문41baa0의 별도 출력 길이이며 storage.size와 같다고 가정하지 않는다.
};
std::uintptr_t ReadDataManagerFileBytes(o::RawBaseFileState& state,std::span<const std::uint8_t> name,std::int32_t& length,const o::RawBaseFileIOHooks& io,const o::BaseFileWholeReadHooks& hooks,std::span<std::uint8_t> scratch={}); //0044d980의 length0/check1/extra0을 같은41baa0으로 실행한다.
std::optional<DataManagerFileAllocation> ReadDataManagerFileImage(const o::BaseFileSystem& files,std::string_view name); // 현재 backing FILE·키를 반납한 뒤 실제 할당만 넘긴다. typed 실패/부분I/O/파서 실패를 미발견으로 바꾸지 않는다.
TemplateFileLoadHooks BindDataManagerFileRead(TemplateFileLoadHooks hooks,const o::BaseFileSystem& files); // 실제 Native caller의 readWhole만 설치하며 기존 독립 read와 namespace를 일괄 변경하지 않는다.
}
