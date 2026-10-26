// 이름/manager/섹션을 복제하지 않고 기존 전체 부모의 선택 장치만 연결한다.
#include "client/FortCurrentNameDevices.h"
#include "o/BaseFile.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 파일 쓰기/삭제는 명시 작업 namespace의 동일 장치만 사용한다.
namespace { // 외부 C문자열 장치의 물리 조회를 공유한다.
// 반환값을 쓰는 장치만 현재 NUL까지 읽으며 임의 미종료 입력은 거부한다.
std::string_view Text(std::span<const std::uint8_t> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("Fort 이름 장치의 NUL 범위");
    return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())};
}
}
// 기존 세부 게임 효과는 보존하고 물리/경로/CRT/설정 장치만 같은 owner에 연결한다.
FortCurrentNameDevices::FortCurrentNameDevices(ClientFortGlobals& names,CampaignTemplateStorage& storage,o::ConfigInterface& config,o::FortCurrentNameGlobals globals,o::FortCurrentNameHooks hooks,const EditorFiles* work,std::function<std::filesystem::path(std::string_view)> resolve)
    :names_(names),storage_(storage),config_(config),globals_(std::move(globals)),hooks_(storage.BindCurrentName(std::move(hooks))),work_(work) {
    if (&globals_.dirty!=&names_.Catalog().dirty) throw std::invalid_argument("Fort 이름과 목록의 dirty 물리 불일치");
    hooks_.currentName=[this] { return names_.CurrentBytes(); };hooks_.selectFileSpec=[this] { names_.SelectFileSpec(); };
    hooks_.pathSpec=[this,resolve=std::move(resolve)](auto name)->std::span<const std::uint8_t> {
        path_=config_.PathSpec(names_.SelectedFileSpec(),Text(name));
        if (work_) path_=(resolve ? resolve(path_) : work_->ResolvePath(std::filesystem::path(path_).is_absolute() ? path_ : o::NormalizeGamePath(path_))).string(); //절대 source 매핑은 명시 장치만 허용하며 기존 writer의 보호 범위를 넓히지 않는다.
        return {reinterpret_cast<const std::uint8_t*>(path_.c_str()),path_.size()+1};
    };
    hooks_.samePath=[](auto first,auto second) { const std::string left(Text(first)),right(Text(second));return _stricmp(left.c_str(),right.c_str())==0; }; // 원문 CRT의 현재 NUL/대소문자 비교다.
    hooks_.deleteRequest=[this](auto path) { if (!work_) throw std::logic_error("현재 Fort 삭제의 실제 작업 namespace 미연결");work_->Delete(Text(path)); };
    hooks_.config=[this](auto key,auto value) { const auto text=Text(value);config_.Set(key,o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()})); }; // 같은 Config의 실제 현재 대상/즉시 해석을 사용한다.
}
void FortCurrentNameDevices::Set(std::span<const std::uint8_t> name) { std::array<std::uint8_t,256> scratch;Set(name,scratch); } // 원문 지역 미작성 scratch이며 선택된 oldPath 복사가 먼저 채운다.
void FortCurrentNameDevices::Set(std::span<const std::uint8_t> name,std::span<std::uint8_t,256> oldPathScratch) { o::SetCurrentFortName(name,oldPathScratch,globals_,hooks_); } // 두 snapshot/Notify/삭제와 부분 효과는 같은 기존 본체가 소유한다.
}
