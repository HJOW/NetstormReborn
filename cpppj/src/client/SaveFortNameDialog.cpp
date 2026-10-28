// 원본 입력 대화창의 순서와 실패 접두를 보존하며 파일 쓰기는 전체 이름 부모가 소유한다.
#include "client/SaveFortNameDialog.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 공통 GUI 계산과 별개인 원문 부모를 복원한다.
namespace { // 실제 선택한 장치와 원문 DWORD/float 산술을 지역화한다.
// 도달한 외부 효과만 요구하며 미연결을 정상 반환으로 바꾸지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("Save as의 실제 장치 미연결");return function; }
// 같은 raw header의 첫DWORD를 포인터 토큰으로 읽는다.
std::uint32_t Word(std::span<const std::uint8_t> bytes,std::size_t offset) { if (offset+4>bytes.size()) throw std::out_of_range("Save as Template header 폭");return std::uint32_t(bytes[offset])|(std::uint32_t(bytes[offset+1])<<8)|(std::uint32_t(bytes[offset+2])<<16)|(std::uint32_t(bytes[offset+3])<<24); }
// 원문 _ftol의 유한 GUI 크기 입력을0방향으로 절삭한다.
std::int32_t Size(float value) { return static_cast<std::int32_t>(value); }
}
// 입력256BYTE의 첫LF/문자치환/존재/readonly/전체이름 변경 순서를 그대로 실행한다.
std::uint32_t ConfirmSaveFortName(const std::uint32_t& fortMode,const SaveFortNameConfirmHooks& hooks) {
    std::array<char,256> text;Need(hooks.copyText)(text); // 원문 지역 미작성 scratch이며 실제 CopyText가 NUL을 포함해 채운다.
    auto end=std::find(text.begin(),text.end(),'\0');if (end==text.end()) throw std::out_of_range("Save as 실제 편집문자열 NUL 누락");
    const auto newline=std::find(text.begin(),end,'\n');if (newline!=end) { *newline='\0';end=newline; }
    if (text.front()) {
        // 원문은 _isalnum을 먼저 실행하고 underscore/공백을 추가로 허용한다. trim/collapse를 넣지 않는다.
        for (auto cursor=text.begin();cursor!=text.end() && *cursor;++cursor) if (!Need(hooks.alnum)(static_cast<std::int8_t>(*cursor)) && *cursor!='_' && *cursor!=' ') *cursor=' ';
        if (text.front()) {
            end=std::find(text.begin(),text.end(),'\0');if (end==text.end()) throw std::out_of_range("Save as 치환 뒤 NUL 범위");
            const std::string_view name(text.data(),static_cast<std::size_t>(end-text.begin()));const auto path=Need(hooks.path)(name);
            if (!Need(hooks.exists)(path)) {
                if (fortMode) {
                    auto header=Need(hooks.header)();if (Word(header,0)) { static_cast<void>(Word(header,8));std::fill_n(header.begin()+8,4,std::uint8_t{}); }
                    Need(hooks.setName)(name);return 1;
                }
                Need(hooks.readFort)(name);return 1;
            }
            Need(hooks.tell)("NameRequired");
        }
    }
    return 0;
}
// 같은 canCancel DWORD를현재읽고거부안내가정상복귀한경우에만0을반환한다.
std::uint32_t CancelSaveFortName(const std::uint32_t& canCancel,const std::function<void(std::string_view)>& tell) {
    if (canCancel) return 1;Need(tell)("NameRequired");return 0;
}
// 현재 장치는 선택된 생성/입력에서 검사하며 생성자에서 게임/파일을 실행하지 않는다.
SaveFortNameDialog::SaveFortNameDialog(GumpCreateState& state,SaveFortNameDialogHooks hooks):state_(state),hooks_(std::move(hooks)) {}
// 원문 유효this에 대응하는 같은 파생 주소만 조회한다.
SaveFortNameDialogNode& SaveFortNameDialog::Node(std::uint32_t token) const { const auto node=Need(hooks_.node)(token);if (!node) throw std::logic_error("Save as 실제 부모 없음");return *node; }
// 같은 자식이 아직 반납되지 않았는지 확인한다.
GumpCreateNode& SaveFortNameDialog::Base(std::uint32_t token) const { const auto node=Need(hooks_.base)(token);if (!node) throw std::logic_error("Save as 실제 자식 없음");return *node; }
// 제목과편집다음의현재크기를읽어두버튼을원문의가운데간격10으로배치한다.
void SaveFortNameDialog::Buttons(std::uint32_t token,float y) const {
    auto& root=Node(token);root.ok=Need(hooks_.allocate)(0x170);Need(hooks_.stone)(root.ok,Need(hooks_.translate)("OK"));
    root.cancel=hooks_.allocate(0x170);hooks_.stone(root.cancel,hooks_.translate("Cancel"));
    if (!(Base(root.ok).flags&8U)) Need(hooks_.push)(root.ok);if (!(Base(root.cancel).flags&8U)) Need(hooks_.push)(root.cancel);
    Need(hooks_.fitStone)(root.ok);hooks_.fitStone(root.cancel);
    const auto total=Base(root.cancel).baseWidth+Base(root.ok).baseWidth+10.0F;
    Need(hooks_.attach)(root.ok,(root.baseWidth-total)*0.5F,y,token,0);
    hooks_.attach(root.cancel,(root.baseWidth-total)*0.5F+Base(root.ok).baseWidth+10.0F,y,token,0);
}
// 생성 중 실제 포인터 대입/가상 호출 순서를 유지하고 공통 모달 등록에서만 입력을 연다.
std::uint32_t SaveFortNameDialog::Show(std::uint32_t fortMode,std::uint32_t canCancel,std::string_view current,std::optional<std::string_view> initial) const {
    const auto token=Need(hooks_.allocate)(0xdc);Need(hooks_.decoration)(token,200.0F,100.0F);auto& root=Node(token);root.vtable=0x00517bc8U;root.command=32;root.fortMode=fortMode;root.canCancel=canCancel;
    root.entry=hooks_.allocate(0x110);Need(hooks_.textEntry)(root.entry,Size(root.baseWidth*0.8F),18,initial.value_or(current));
    Need(hooks_.excluded)(root.entry,".?=()[]~\"`;:^&%$#@!*-+'{}|\\/<>,");
    auto* entry=Need(hooks_.entry)(root.entry);if (!entry) throw std::logic_error("Save as 실제 편집 자식 없음");entry->maxLength=0x28;entry->multiline=0;
    Need(hooks_.attach)(root.entry,(root.baseWidth-Base(root.entry).baseWidth)*0.5F,45.0F,token,0);
    const auto border=hooks_.allocate(0xb8);Need(hooks_.background)(border,Base(root.entry).baseWidth+8.0F,Base(root.entry).baseHeight+4.0F);
    hooks_.attach(border,Base(root.entry).x-4.0F,Base(root.entry).y-2.0F,token,0);
    const auto title=Need(hooks_.translate)(fortMode ? "Save Fortress As?" : "New Save-Game Name?");root.title=hooks_.allocate(0xa4);Need(hooks_.title)(root.title,title);
    Base(root.title).flags|=4U;hooks_.attach(root.title,Base(root.entry).x,10.0F,token,0);Need(hooks_.fitTitle)(root.title);
    root.baseWidth=Base(root.title).baseWidth*1.25F;root.width=root.baseWidth*root.scale;
    Buttons(token,Base(root.entry).y+Base(root.entry).baseHeight+15.0F);root.baseHeight+=4.0F;root.height=root.baseHeight*root.scale;
    hooks_.attach(token,(static_cast<float>(state_.screenWidth)-root.baseWidth)*0.5F,(static_cast<float>(state_.screenHeight)-root.baseHeight)*0.5F,state_.entries[0],0);
    Need(hooks_.depth)(token,std::bit_cast<std::int32_t>(root.depth)-76);Need(hooks_.modal)(token);return token;
}
// 숫자 창은 같은 장식/입력/두 버튼 몸체를 사용하고 원래0xd8 크기와 대상+d4를 보존한다.
std::uint32_t SaveFortNameDialog::ShowMoney(std::uint32_t target) const {
    const auto token=Need(hooks_.allocate)(0xd8);Need(hooks_.decoration)(token,200.0F,100.0F);auto& root=Node(token);root.vtable=0x00517ca0U;root.command=68;root.fortMode=target;root.canCancel=1;
    root.entry=hooks_.allocate(0x110);Need(hooks_.textEntry)(root.entry,Size(root.baseWidth*0.8F),18,"0");
    auto* entry=Need(hooks_.entry)(root.entry);if (!entry) throw std::logic_error("송금 숫자 창의 편집 자식 없음");entry->maxLength=10;entry->multiline=0;
    std::string excluded; // 원문1..255 중 ASCII 숫자 외 모든 문자를 입력 금지 문자열에 넣는다.
    for (std::uint32_t value=1;value<256;++value) if (value<'0' || value>'9') excluded.push_back(static_cast<char>(value)); // 숫자의 범위와 원본 순서를 유지한다.
    Need(hooks_.excluded)(root.entry,excluded);Need(hooks_.attach)(root.entry,(root.baseWidth-Base(root.entry).baseWidth)*0.5F,45.0F,token,0);
    const auto border=hooks_.allocate(0xb8);Need(hooks_.background)(border,Base(root.entry).baseWidth+8.0F,Base(root.entry).baseHeight+4.0F);hooks_.attach(border,Base(root.entry).x-4.0F,Base(root.entry).y-2.0F,token,0);
    const auto title=Need(hooks_.translate)("Send X Storm Power");root.title=hooks_.allocate(0xa4);Need(hooks_.title)(root.title,title);hooks_.attach(root.title,Base(root.entry).x,10.0F,token,0);Need(hooks_.fitTitle)(root.title);Need(hooks_.push)(root.title);Base(root.title).alignFlags=4U;hooks_.attach(root.title,Base(root.title).x,Base(root.title).y,token,2); // 원문+8은 정렬 필드다. 가상+18로 먼저 분리한 뒤 가운데 정렬로 재부착한다.
    Buttons(token,Base(root.entry).y+Base(root.entry).baseHeight+15.0F);root.baseHeight+=4.0F;root.height=root.baseHeight*root.scale;
    root.baseWidth=std::max(root.baseWidth,Base(root.title).baseWidth+32.0F);root.width=root.baseWidth*root.scale;
    hooks_.attach(token,(static_cast<float>(state_.screenWidth)-root.baseWidth)*0.5F,(static_cast<float>(state_.screenHeight)-root.baseHeight)*0.5F,state_.entries[0],0);Need(hooks_.depth)(token,std::bit_cast<std::int32_t>(root.depth)-76);Need(hooks_.modal)(token);return token;
}
// 실제 버튼 serial(+2c)과 Enter를 판정하고 callback 비영 복귀에서만 지연 Kill한다.
std::uint32_t SaveFortNameDialog::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    const auto code=event&0x03ffffffU;auto& root=Node(token);
    if ((code==0x0200002cU && first==Base(root.ok).serial) || code==0x02000038U) {
        if (Need(hooks_.accept)(token)) { Need(hooks_.kill)(token);return 1; }
    } else {
        if (code!=0x0200002cU || first!=Base(root.cancel).serial) return Need(hooks_.baseHandle)(token,event,first,second);
        if (Need(hooks_.cancel)(token)) Need(hooks_.kill)(token);
    }
    return 1;
}
}
