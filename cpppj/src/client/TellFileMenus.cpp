// 파일마다 최초 Config를 등록한 뒤 고유 prefix를 정렬하고 cur 설정 안에서 실제 메뉴를 만든다.
#include "client/TellFileMenus.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::client { //4ce1b0과4ce580의 공통 수명과 제목 폴백 차이를 유지한다.
namespace { // 원문 좁은 바이트와 실제 Config의 UTF-8 경계만 변환한다.
class PhysicalLimitError final:public std::length_error { // 원본 버퍼 밖 입력의 명시 거부를 실제 행 처리 장치의 C++ 예외와 구별한다.
public:
    explicit PhysicalLimitError(const char* message):std::length_error(message) {} // 기존 length_error 계약을 유지하며 ERROR 링크로 성공 대체하지 않는다.
};
std::string_view CString(std::string_view value) { return value.substr(0,value.find('\0')); } // 첫NUL 뒤의 입력은 사용하지 않는다.
std::string Decode(std::string_view value) { return o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); } // Config로 들어갈 때만 원문 바이트를 변환한다.
std::string Narrow(std::string_view value) { const auto bytes=o::EncodeOriginalText(value);return {bytes.begin(),bytes.end()}; } // 실제 메뉴/고리에는 원문 글꼴 바이트를 쓴다.
void Limit(std::string_view value,std::size_t capacity) { if (Narrow(value).size()>=capacity) throw PhysicalLimitError("Tell 파일 목록의 원본 지역 NUL 범위 초과"); } // 원문의 물리 오버런 입력만 거부한다.
std::string FileName(std::string_view path) { //59c40(1)의 첫 점부터 제거와59b20(1)의 마지막 경로 구분자를 보존한다.
    const auto separator=path.find_last_of("\\/:");auto name=std::string(path.substr(separator==path.npos ? 0 : separator+1));const auto dot=name.find('.');if (dot!=name.npos) name.resize(dot);return name;
}
std::string ObjectName(const o::Config& object) { auto name=object.Prefix();if (name.empty()) throw std::logic_error("Tell 파일 목록의 실제 이름 prefix 없음");name.pop_back();return name; } // 정렬한 원문 prefix에서 마지막 점 하나만 제거한다.
std::string Title(o::ConfigInterface& config,std::string_view name,bool fallback) { auto label=config.SubstituteLocal("{{local.1}.title}",name);Limit(label,80);if (fallback && label.empty()) { label=name;Limit(label,80); }return label; } // 실제 지역80BYTE 복사 뒤 @/%만 빈 제목을 이름으로 바꾼다.
}
TellFileMenus::TellFileMenus(o::ConfigInterface& config,std::function<std::vector<std::string>(std::string_view)> enumerate):config_(config),enumerate_(std::move(enumerate)) {} // 생성 시 파일이나 메뉴를 만들지 않는다.
TellFileMenus::~TellFileMenus()=default; // 실제 Config 소멸자가 등록을 반납한다.
std::vector<o::Config*> TellFileMenus::Collect(std::string_view pattern,std::size_t capacity) {
    if (!enumerate_) throw std::logic_error("Tell 파일 목록의 실제 열거 미연결");const auto spec=Decode(CString(pattern));Limit(spec,256);std::vector<o::Config*> objects;std::uint32_t index=1;
    for (const auto& path:enumerate_(spec)) { // 정렬 전 실제 열거 순서로 Config의 index와 첫 파일 경로를 확정한다.
        Limit(path,256);const auto name=FileName(path);Limit(name,256);auto* object=config_.Registry().FindByName(name);
        if (!object) {
            auto created=std::make_unique<o::Config>(config_.Registry(),name);object=created.get();owned_.push_back(std::move(created)); // 적재 중 실패해도 이미 등록한 원문 객체의 수명을 보존한다.
            object->Append("path="+path);object->Append("fileName="+name);object->Append("index="+std::to_string(index));if (config_.Reader()) if (const auto bytes=config_.Reader()(path)) object->LoadBytes(*bytes);
        }
        if (std::find(objects.begin(),objects.end(),object)==objects.end()) { if (objects.size()>=capacity) throw PhysicalLimitError("Tell 파일 목록의 원본 prefix 배열 범위 초과");objects.push_back(object);++index; }
    }
    for (std::size_t first=0;first<objects.size();++first) for (std::size_t next=first;next<objects.size();++next) { // 원문 두 겹 비교/교환 순서를 유지하며 표시 제목으로 정렬하지 않는다.
        if (o::AsciiLower(Narrow(objects[first]->Prefix()))>o::AsciiLower(Narrow(objects[next]->Prefix()))) std::swap(objects[first],objects[next]);
    }
    return objects;
}
void TellFileMenus::Append(std::string_view pattern,std::uint32_t command,std::string_view argument,bool emptyTitleUsesName,
    ScriptStringRing& strings,const std::function<void(const TellFileMenuItem&)>& emit) {
    const auto objects=Collect(pattern,40);
    for (const auto* object:objects) { // 같은 실제 cur 객체 안에서 제목과 메뉴 인자를 처리한다.
        const auto name=ObjectName(*object);o::Config current(config_.Registry(),"cur");current.Append("file="+name);const auto label=Title(config_,name,emptyTitleUsesName);
        TellFileMenuItem item;item.label=Narrow(label);item.separator=item.label.starts_with("--");if (!item.separator) { auto parameter=config_.SubstituteLocal(Decode(CString(argument)));Limit(parameter,1024);item.command=command;item.parameter=strings.Store(Narrow(parameter)); }
        if (!emit) throw std::logic_error("Tell 파일 목록의 실제 메뉴 작성 미연결");emit(item); // 구분선도 같은 cur 객체의 수명 안에서 생성한다.
    }
}
std::string TellFileMenus::Inline(std::string_view pattern,std::string_view command,std::string_view argument) {
    const auto objects=Collect(pattern,80);std::string result; // 원문 String은 빈 본문으로 시작하며 구분선에는 링크를 추가하지 않는다.
    for (const auto* object:objects) { // 각 항목의 두 치환과 링크 조각 추가가 끝날 때까지 같은 cur.file을 유지한다.
        const auto name=ObjectName(*object);o::Config current(config_.Registry(),"cur");current.Append("file="+name);const auto label=Title(config_,name,true);if (label.starts_with("--")) continue;
        try { // 원문의 EH 상태6은 제목 처리 뒤 첫 인자 치환부터 현재 링크 작성 끝까지다.
            const auto source=Decode(CString(argument));const auto first=config_.SubstituteLocal(source);Limit(first,1024); // 원문 첫 조회의 반환값은 버리지만 파일/환경 조회 효과는 실행한다.
            result+="\n<a herf=\"cmd:";result+=CString(command);result+=',';const auto second=config_.SubstituteLocal(source);Limit(second,1024);result+=Narrow(second);result+="\">";result+=Narrow(label);result+="</a>"; // 원문 herf 철자·LF 위치와 인용/명령 대소문자를 수정하거나 escape하지 않는다.
        } catch (const PhysicalLimitError&) { throw; } // 원문 지역 오버런에 해당하는 무효 입력은 기존 명시 거부를 유지한다.
        catch (...) { result+="\n<a herf=\"cmd:";result+=CString(command);result+=",\">~rERROR ";result+=Narrow(label);result+="</a>"; } //4ce0ef는 이미 쓴 부분 문자열을 되돌리지 않고 오류 링크를 덧붙이며 다음 행으로 진행한다.
    }
    return result;
}
}
