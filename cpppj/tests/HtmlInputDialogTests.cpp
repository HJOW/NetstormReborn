// 실제 HTML 입력의 원문 생성/공장/확인109입력을 독립 UI/문자 경계에서 대조한다.
#include "RawSceneSupport.h"
#include "client/HtmlInputDialog.h"
#include "client/SaveFortNameDialog.h"
#include <bit>
#include <filesystem>
#include <map>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;

namespace { //기대값은 원문 fixture에서만 읽고 제품 구현으로 만들지 않는다.
std::string InputDecode(std::string_view text) { //fixture의BYTE문자열을그대로복원한다.
    std::string result;if (text=="-") return result;
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<char>(std::stoul(std::string(text.substr(index,2)),nullptr,16))); //두hex문자를한BYTE로읽는다.
    return result;
}
std::string InputHex(std::string_view text) { return text.empty() ? std::string{} : Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); } //trace의빈인자는빈hex로유지한다.
std::vector<std::uint32_t> InputNumbers(std::string_view text) { //snapshot의DWORD를원문순서로읽는다.
    std::vector<std::uint32_t> result;std::istringstream stream{std::string(text)};std::string field;while (std::getline(stream,field,',')) result.push_back(static_cast<std::uint32_t>(std::stoul(field)));return result;
}
struct InputConstruction { //부모와같은실제기본필드를명시UI경계에공급한다.게임/멀티플레이상태는없다.
    SaveFortNameDialogNode root{};TextEntryGumpNode entry{};std::map<std::uint32_t,GumpCreateNode> nodes;GumpCreateState state; //같은주소를생성기와관측이공유한다.
    std::uint32_t allocated{},border{},title{},depth{},modal{},titleWidth{},wantedWidth{};std::string trace; //현재독립관측값이다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; } //원문경계순서를누적한다.
    GumpCreateNode& Base(std::uint32_t token) { if (token==1) return root;if (token==2) return entry;return nodes.at(token); } //각자식의같은기본노드를반환한다.
    SaveFortNameDialogHooks Hooks() { //부모계산은제품을실행하고큰기존UIhelper만독립입력으로둔다.
        SaveFortNameDialogHooks hooks;hooks.node=[this](auto token) { CHECK(token==1);return &root; };hooks.base=[this](auto token) { return &Base(token); };hooks.entry=[this](auto token) { CHECK(token==2);return &entry; };
        hooks.allocate=[this](auto bytes) { const auto token=++allocated;if (token==1) CHECK(bytes==0x128);else if (token==2) CHECK(bytes==0x110);else { nodes.emplace(token,GumpCreateNode{});if (bytes==0xb8) border=token; }return token; };
        hooks.decoration=[this](auto token,float width,float height) { CHECK(token==1);root.baseWidth=root.width=width;root.baseHeight=root.height=height;root.depth=12;root.scale=1; };
        hooks.textEntry=[this](auto token,auto width,auto height,auto text) { CHECK(token==2);entry.baseWidth=static_cast<float>(width);entry.baseHeight=static_cast<float>(height);Event("entry:"+InputHex(text)); };
        hooks.background=[this](auto token,float width,float height) { Base(token).baseWidth=width;Base(token).baseHeight=height; };
        hooks.translate=[this](auto text) { if (text=="Prompt") Event("translate:"+InputHex(text));return std::string(text); };
        hooks.inputTitle=[this](auto token,float width,auto text) { title=token;titleWidth=static_cast<std::uint32_t>(width);auto& node=Base(token);node.baseWidth=static_cast<float>(titleWidth);node.baseHeight=12;Event("title:"+std::to_string(titleWidth)+':'+InputHex(text)); };
        hooks.fitTitle=[this](auto token) { Base(token).baseWidth=std::bit_cast<float>(wantedWidth); };hooks.push=[](auto) {};
        hooks.stone=[this](auto token,auto) { Base(token).baseWidth=70;Base(token).baseHeight=18; };hooks.fitStone=[](auto) {};
        hooks.attach=[this](auto token,float x,float y,auto,auto) { Base(token).x=x;Base(token).y=y; };hooks.depth=[this](auto,auto value) { depth=std::bit_cast<std::uint32_t>(value); };hooks.modal=[this](auto) { modal=1; };return hooks;
    }
    std::vector<std::uint32_t> Snapshot() { //이번원문부모가읽고쓴동일필드만비교하며공통helper전체검사로합산하지않는다.
        const auto word=[](float value) { return std::bit_cast<std::uint32_t>(value); };std::vector<std::uint32_t> result{root.vtable,root.command,root.fortMode,word(root.baseWidth),word(root.baseHeight),word(root.width),word(root.height),word(root.x),word(root.y),word(entry.baseWidth),word(entry.baseHeight),static_cast<std::uint32_t>(entry.maxLength),entry.multiline,word(entry.x),word(entry.y)};
        const auto& edge=Base(border);result.insert(result.end(),{word(edge.baseWidth),word(edge.baseHeight),word(edge.x),word(edge.y)});const auto& text=Base(title);result.insert(result.end(),{word(text.baseWidth),word(text.baseHeight),word(text.x),word(text.y),text.alignFlags,titleWidth,depth,modal});return result;
    }
};
}
//원문20생성/공장과89확인의필드·반환·실제경계순서를모두대조한다.
TEST_CASE(HtmlInputDialog_IndependentNative) {
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/html-input-dialog-x86.tsv";const auto fixture=LoadFixture(path.string().c_str());std::size_t count{};
    for (const auto& row:fixture.rows) { //실제원문기대fixture를수정하거나제품으로재생성하지않는다.
        CHECK(row.size()==11);if (row.size()!=11) continue;auto mode=Number(row[1]);const auto key=InputDecode(row[5]),input=InputDecode(row[6]);std::string trace;
        if (row[0]=="C") { InputConstruction scene;scene.state.screenWidth=static_cast<std::int32_t>(Number(row[3]));scene.state.screenHeight=static_cast<std::int32_t>(Number(row[4]));scene.state.entries[0]=99;scene.wantedWidth=Number(row[2]);CHECK(SaveFortNameDialog(scene.state,scene.Hooks()).ShowInput(mode,input,key)==1);CHECK(std::string(scene.root.inputSection.data())==key);CHECK(scene.Snapshot()==InputNumbers(row[9]));trace=scene.trace; }
        else { std::array<char,80> continuation{};std::copy(key.begin(),key.end(),continuation.begin());const auto event=[&](std::string value) { if (!trace.empty()) trace+=';';trace+=value; };HtmlInputConfirmHooks hooks;
            hooks.copyText=[&](auto output) { event("copy");std::copy_n(input.data(),std::min(input.size(),output.size()),output.data());if (Number(row[7])) { mode=1;continuation.fill(0);std::copy_n("changed",7,continuation.data()); } };
            hooks.space=[&](auto code) { event("space:"+std::to_string(code));return 0; };hooks.parseLong=[&](auto text) { event("atol:"+InputHex(text));return 0; };hooks.format=[&](auto section,auto text) { event("format:"+InputHex(section)+':'+InputHex(text));return std::string(section)+'('+std::string(text)+')'; };hooks.tell=[&](auto text) { event("tell:"+InputHex(text)); };CHECK(ConfirmHtmlInput(mode,continuation,hooks)==Number(row[8]));
        }
        if (trace!=row[10]) std::fprintf(stderr,"HTML 입력 %s/%s 기대 %s 실제 %s\n",row[0].c_str(),row[1].c_str(),row[10].c_str(),trace.c_str());CHECK(trace==row[10]);++count;
    }
    CHECK(count==109 && count==fixture.rows.size());
}
