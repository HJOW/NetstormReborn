// 파일마다 최초 Config를 등록한 뒤 고유 prefix를 정렬하고 cur 설정 안에서 실제 메뉴를 만든다.
#include "client/TellFileMenus.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::client { //4ce1b0과4ce580의 공통 수명과 제목 폴백 차이를 유지한다.
namespace { // 원문 좁은 바이트와 실제 Config의 UTF-8 경계만 변환한다.
std::string_view CString(std::string_view value) { return value.substr(0,value.find('\0')); } // 첫NUL 뒤의 입력은 사용하지 않는다.
std::string Decode(std::string_view value) { return o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); } // Config로 들어갈 때만 원문 바이트를 변환한다.
std::string Narrow(std::string_view value) { const auto bytes=o::EncodeOriginalText(value);return {bytes.begin(),bytes.end()}; } // 실제 메뉴/고리에는 원문 글꼴 바이트를 쓴다.
void Limit(std::string_view value,std::size_t capacity) { if (Narrow(value).size()>=capacity) throw std::length_error("Tell 파일 목록의 원본 지역 NUL 범위 초과"); } // 원문의 물리 오버런 입력만 거부한다.
std::string FileName(std::string_view path) { //59c40(1)의 첫 점부터 제거와59b20(1)의 마지막 경로 구분자를 보존한다.
    const auto separator=path.find_last_of("\\/:");auto name=std::string(path.substr(separator==path.npos ? 0 : separator+1));const auto dot=name.find('.');if (dot!=name.npos) name.resize(dot);return name;
}
}
TellFileMenus::TellFileMenus(o::ConfigInterface& config,std::function<std::vector<std::string>(std::string_view)> enumerate):config_(config),enumerate_(std::move(enumerate)) {} // 생성 시 파일이나 메뉴를 만들지 않는다.
TellFileMenus::~TellFileMenus()=default; // 실제 Config 소멸자가 등록을 반납한다.
void TellFileMenus::Append(std::string_view pattern,std::uint32_t command,std::string_view argument,bool emptyTitleUsesName,
    ScriptStringRing& strings,const std::function<void(const TellFileMenuItem&)>& emit) {
    if (!enumerate_) throw std::logic_error("Tell 파일 목록의 실제 열거 미연결");const auto spec=Decode(CString(pattern));Limit(spec,256);std::vector<o::Config*> objects;std::uint32_t index=1;
    for (const auto& path:enumerate_(spec)) { // 정렬 전 실제 열거 순서로 Config의 index와 첫 파일 경로를 확정한다.
        Limit(path,256);const auto name=FileName(path);Limit(name,256);auto* object=config_.Registry().FindByName(name);
        if (!object) {
            auto created=std::make_unique<o::Config>(config_.Registry(),name);object=created.get();owned_.push_back(std::move(created)); // 적재 중 실패해도 이미 등록한 원문 객체의 수명을 보존한다.
            object->Append("path="+path);object->Append("fileName="+name);object->Append("index="+std::to_string(index));if (config_.Reader()) if (const auto bytes=config_.Reader()(path)) object->LoadBytes(*bytes);
        }
        if (std::find(objects.begin(),objects.end(),object)==objects.end()) { if (objects.size()>=40) throw std::length_error("Tell 파일 목록의 원본40개 prefix 범위 초과");objects.push_back(object);++index; }
    }
    for (std::size_t first=0;first<objects.size();++first) for (std::size_t next=first;next<objects.size();++next) { // 원문 두 겹 비교/교환 순서를 유지하며 표시 제목으로 정렬하지 않는다.
        if (o::AsciiLower(Narrow(objects[first]->Prefix()))>o::AsciiLower(Narrow(objects[next]->Prefix()))) std::swap(objects[first],objects[next]);
    }
    for (const auto* object:objects) { // 정렬한 실제 prefix에서 마지막 점 하나만 제거해 cur.file과 제목 조회에 사용한다.
        auto name=object->Prefix();if (name.empty()) throw std::logic_error("Tell 파일 목록의 실제 이름 prefix 없음");name.pop_back();o::Config current(config_.Registry(),"cur");current.Append("file="+name);
        auto label=config_.SubstituteLocal("{{local.1}.title}",name);Limit(label,80);if (emptyTitleUsesName && label.empty()) { label=name;Limit(label,80); }
        TellFileMenuItem item;item.label=Narrow(label);item.separator=item.label.starts_with("--");if (!item.separator) { auto parameter=config_.SubstituteLocal(Decode(CString(argument)));Limit(parameter,1024);item.command=command;item.parameter=strings.Store(Narrow(parameter)); }
        if (!emit) throw std::logic_error("Tell 파일 목록의 실제 메뉴 작성 미연결");emit(item); // 구분선도 같은 cur 객체의 수명 안에서 생성한다.
    }
}
}
