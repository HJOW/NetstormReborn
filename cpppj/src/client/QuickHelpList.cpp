// 원문 파일/타입/UI 효과의 순서와 실패 이전 부분 효과를 보존한다.
#include "client/QuickHelpList.h"
#include "o/OriginalText.h"
#include <stdexcept>

namespace netstorm::client { // 게임/OS를 시작하지 않고 같은 실제 Help 장치를 조립한다.
namespace { // 원문이 도달한 경계만 검사하며 사용하지 않은 장치는 선행 요구하지 않는다.
template<class T> const T& Need(const T& value) { if (!value) throw std::logic_error("CreateHelpList 실제 경계 미연결");return value; } //실패를 빈 파일/정상 결과로 숨기지 않는다.
}
// OpenHelp/지역 생성/열기/stream/순회/Close/Kill/Tell/소멸의 원문 순서다.
void CreateQuickHelpList(const QuickHelpListContext& context,const QuickHelpListWindowHooks& window) {
    const auto help=Need(window.openEmpty)();if (!help) return;
    std::array<std::uint8_t,24> storage; //원문의 초기화되지 않은 지역 물리다. 장치는 내용 대신 같은 주소의 소유권만 사용한다.
    const QuickHelpListFile file(storage);Need(context.file.construct)(file);bool attached=false; //생성 성공 뒤부터 원문의 SEH 파일 소멸이 유효하다.
    try {
        Need(context.file.open)(file,"NetStormPieceList.html",1,1);
        Need(window.attachStream)(help,[&](std::string_view bytes) { Need(context.file.write)(file,bytes,-1); });attached=true; //파서가 같은 살아 있는 지역파일에 즉시 기록한다.
        Need(context.file.write)(file,"<html>",-1);
        std::uint32_t type{}; //원문의 ESI는0에서 시작하며 현재 signed 개수와 비교한다.
        if (Need(context.typeCount)()>0) do { //모든 반복에서 제외 여부와 이름을 별도 현재 TYPE 조회로 읽는다.
            if (Need(context.flags2)(type)) {
                auto topic=Need(context.typeName)(type);topic.resize(topic.find('\0')==std::string::npos ? topic.size() : topic.find('\0'));
                if (topic.size()>251) throw std::length_error("CreateHelpList의256BYTE %sType 지역 서식 범위");topic+="Type";
                Need(window.show)(help,type,topic);Need(context.file.write)(file,"<p>",-1);
            }
            Need(context.cleanup)();++type;
        } while (static_cast<std::int32_t>(type)<Need(context.typeCount)()); //원문의 signed 비교이며 실제 TYPE 표는 물리 범위 안이다.
        Need(context.file.write)(file,"</html>",-1);Need(window.detachStream)(help);attached=false;
        Need(context.file.close)(file);Need(window.kill)(help);Need(context.tell)("pieceDump");
    } catch (...) {
        if (attached) Need(window.abandonStream)(help); //원문 dangling +bc를 역참조하지 않도록 callback 수명만 반납한다. 현재 +bc값/앞선 효과는 보존한다.
        Need(context.file.destruct)(file);throw; //원문 SEH 소멸의 호스트 대응이며 기존 쓰기/등록/Show를 롤백하지 않는다.
    }
    Need(context.file.destruct)(file); //정상 경로도 명시 Close와 별개인 원문 지역 소멸을 한 번 호출한다.
}
// Guide/Gump 뒤 일반 분배 위치에서 호출하며 정지/월드/편집 모드 관문을 추가하지 않는다.
bool DispatchCreateHelpList(InputEvent event,const std::function<void()>& create) {
    if ((event.code&0x03ffffffU)!=kCreateHelpListCommand) return false;
    if (!(event.code&InputCode::kRelease)) Need(create)();return true;
}
//원문543068의 단일 이름이며 입력 문자열의 첫NUL까지만 비교한다.
std::uint32_t CreateHelpListScriptCode(std::string_view name) { return o::AsciiLower(name.substr(0,name.find('\0'))) == "createhelplist" ? kCreateHelpListCommand : 0xffffffffU; }
}
