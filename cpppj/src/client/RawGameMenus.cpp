// 원본 Game/Edit/View/Options/Players 메뉴부모를현재게임과공통Gump에연결한다.
#include "client/RawGameMenus.h"
#include "client/ScreenResolutions.h" // 원문 세 행과 명시적으로 추가한 호환 화면비를 구별한다.
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 선택된 실제 경계만 요구한다. 호출하지 않는 멀티플레이/편집 의존은 강제로 실행하지 않는다.
template<class T> const T& Need(const T& value) { if (!value) throw std::logic_error("게임 메뉴의 선택된 실제 게임/GUI 의존 누락");return value; }
// 원본CRT _ftol의signed64절삭뒤하위DWORD를취한다. 잘못된수의원본하위DWORD는0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 원본문자열의첫NUL뒤바이트는현재라벨에넣지않는다.
std::string CString(std::string value) { const auto end=value.find('\0');if (end!=value.npos) value.resize(end);return value; }
}
// 훅은원본문체가필요로하는시점에검사하므로조건상선택하지않은하위는요구하지않는다.
RawGameMenus::RawGameMenus(GameMenusHooks hooks):hooks_(std::move(hooks)) {}
// 주소는원문독립전역을구별하는식별자이며호스트포인터로역참조하지않는다.
std::uint32_t RawGameMenus::Global(std::uint32_t address) const { return Need(hooks_.global)(address); }
// 현재Player목록의같은몸체만읽고과거표시용레코드를재사용하지않는다.
std::uint32_t RawGameMenus::PlayerWord(std::uint32_t owner,std::size_t offset) const {
    const auto player=Need(hooks_.player)(owner);if (player.size()<180 || offset+4>180) throw std::out_of_range("메뉴의현재Player180BYTE범위");
    return static_cast<std::uint32_t>(player[offset]) | (static_cast<std::uint32_t>(player[offset+1])<<8) | (static_cast<std::uint32_t>(player[offset+2])<<16) | (static_cast<std::uint32_t>(player[offset+3])<<24);
}
// 번역결과를소유하여후속GUI호출이원문번역고리를바꾸어도현재라벨을유지한다.
std::string RawGameMenus::Text(std::string_view key) const { return CString(Need(hooks_.translate)(key)); }
// 원본은번역을마친뒤명령전역을읽는다. include/enabled의전체DWORD를경계까지보존한다.
void RawGameMenus::Item(std::string_view key,std::uint32_t commandAddress,GameMenuParameter parameter,std::uint32_t include,std::uint32_t enabled) const {
    const auto label=Text(key);Need(hooks_.item)(label,Global(commandAddress),parameter,include,enabled);
}
// 현재체크값과이미번역된문구를같은실제checked행에전달한다.
void RawGameMenus::Check(std::uint32_t check,std::string_view text,std::uint32_t commandAddress,GameMenuParameter parameter,std::uint32_t enabled) const { Need(hooks_.checked)(check,text,Global(commandAddress),parameter,enabled); }
// 설정반전문자열과번역뒤다시읽는체크값은원본의두별개조회다.
void RawGameMenus::Toggle(std::string_view key,std::string_view label,std::uint32_t enabledAddress) const {
    const auto parameter=std::string(key)+(Need(hooks_.config)(key) ? "=0" : "=1");const auto text=Text(label); // 반전선택→번역순서다.
    const auto enabled=enabledAddress ? Global(enabledAddress) : 1U,command=Global(0x5428ac); // 두번째GetInt보다먼저원본인자를고정한다.
    const auto checked=Need(hooks_.config)(key);Need(hooks_.checked)(std::bit_cast<std::uint32_t>(checked),text,command,parameter,enabled);
}
// 실제owner1..8의Player주소로호출하므로순번자체는같다. 현재메타맵진단은호출마다유지한다.
std::uint32_t RawGameMenus::Number(std::uint32_t owner) const { if (Global(0x594fc8)) throw std::logic_error("원본Player메뉴번호의!inMetamapMode진단");return owner&255U; }
// Game본문은미션/데모/Editor를구별하며현재멀티플레이leave판정도원본단락순서대로읽는다.
std::uint32_t RawGameMenus::Game() const {
    Need(hooks_.vertical)();
    if (Global(0x5c85a4)) {
        Item("Test Battle",0x5429fc);Need(hooks_.separator)();Item("Main Menu",0x542948,std::string("LeaveEditMode"));
    } else if (Global(0x594fa4)) {
        if (Need(hooks_.editingMission)()) { Item("Return to Editing",0x5429fc,1U);Need(hooks_.separator)(); }
        if (!Need(hooks_.demo)()) {
            Item("Review Mission Objectives - ~iF8~.",0x542938);Need(hooks_.separator)();Item("Restart Mission",0x542928);Item("Leave Mission",0x542924);
        } else { Item("Restart Demo",0x542928);Item("Exit Demo",0x542924,1U); }
    }
    if (!Global(0x59507c)) {
        if (Global(0x594fbc) && !Global(0x594fa4)) {
            // 원본의8번0048f7e0는순수+54읽기이며반환값은쓰지않는다.
            for (std::uint32_t owner=1;owner<=8;++owner) static_cast<void>(PlayerWord(owner,0x54)&0x400U);
            bool shame=false; // 원본두라벨의최종선택이며게임상태를별도저장하지않는다.
            if (std::bit_cast<std::int32_t>(Need(hooks_.loggedIn)())>=2 && Need(hooks_.priest)(Global(0x540c70))) {
                bool mayLeave=PlayerWord(Global(0x540c70),0x7c)!=0; // 현재플레이어재조회뒤실제표식을읽는다.
                if (!mayLeave) {
                    const auto funds=Global(0x595344) ? Need(hooks_.money)(Global(0x540c70)) : 0.0F; // PlayerKnown이0이면SP호출도하지않는다.
                    const auto cost=Need(hooks_.templeCost)(); // SP조회후현재신전타입을읽는다.
                    if (!(funds<cost)) mayLeave=!Need(hooks_.priestBusy)(Need(hooks_.priest)(Global(0x540c70))); // 원본x87 unordered도이분기로진입한다.
                }
                if (mayLeave && !Need(hooks_.ritual)() && !Need(hooks_.otherRitual)()) shame=true;
            }
            Item(shame ? "Leave Battle In Shame" : "Leave Battle Honorably",0x542948,std::string(shame ? "LeaveEarly" : "LeaveNormal"));
            const auto label=Text("Declare a Draw");const auto owner=Global(0x540c70); // 번역뒤현재로컬Player주소를고정한다.
            const auto player=Need(hooks_.player)(owner);if (player.size()<180) throw std::out_of_range("Draw의현재Player범위");
            const auto enabled=Need(hooks_.captured)(owner)==0; // 실제조회가바꾼같은Player+50을이후읽는다.
            const auto checked=static_cast<std::uint32_t>(player[0x50])&0x80U;Check(checked,label,0x542a60,0U,enabled ? 1U : 0U);
        }
        if (Global(0x594fc8)) Item("Main Menu",0x542a08,1U);
    }
    Need(hooks_.separator)();Item("Quit Game",0x542944);return Need(hooks_.take)();
}
// Editor행은SaveAs/NewFort 명령의인자1을보존하고다리수준0/1/2를별도하위메뉴로작성한다.
std::uint32_t RawGameMenus::Edit() const {
    Need(hooks_.vertical)();Item("Save as...",0x5428dc,1U);Item("Add Island",0x542780);
    Need(hooks_.vertical)();Item("Cracked",0x542a4c,0U);Item("Normal",0x542a4c,1U);Item("Hard",0x542a4c,2U);
    const auto bridge=Need(hooks_.take)();Need(hooks_.submenu)("Set All Bridge",bridge,1,0);return Need(hooks_.take)();
}
// View는원본상황에따라서만게임조작행을추가하며테마/배너체크값은같은설정을다시읽는다.
std::uint32_t RawGameMenus::View() const {
    Need(hooks_.vertical)();
    const auto chat=[&] { const auto label=Text("Hide/Show chat - ~iF3~.");const auto include=Global(0x594fa4)==0;Need(hooks_.item)(label,Global(0x542914),2U,include,1); }; // 번역이바꿀수있는현재미션조건은번역뒤읽는다.
    if (Global(0x594fc8)) { chat();Item("Refresh Zones - ~iF9~.",0x542a0c); }
    if (Global(0x594fb8) || Global(0x594fbc)) {
        Need(hooks_.separator)();auto label=Text("Hide buildings - ~iF2~.");Check(Global(0x59a8ac),label,0x542ab8);
        std::int32_t islands=0;Need(hooks_.readInt)("theammode",islands); // 원문철자theammode와초기0을보존한다.
        label=Text("Hide/Show Island Themes - ~iShift F3~.");Check(islands==0,label,0x542948,std::string(islands ? "SetIslands(0)" : "SetIslands(1)"));
        chat();Item("View Home Temple - ~iF4~.",0x5429ac);
        label=Text("View Your Priest - ~iF5~.");const auto owner=Global(0x540c70);Need(hooks_.item)(label,Global(0x5429d4),owner,1,1);
        if (!Global(0x5c85a4)) Item("View Netstorm Knowledge - ~iF6~.",0x542974);
        Toggle("useBanners","Hide Island Ownership - ~iF7~.");
        if (Global(0x594fa0)) Item("Review Mission Objectives - ~iF8~.",0x542938);
        Item("View Player List - ~iF9~.",0x542ab4,1U);
    }
    return Need(hooks_.take)();
}
// Options의원래화면/음량/설정/정지/회전/진단행과모든체크·활성조건을작성한다.
std::uint32_t RawGameMenus::Options() const {
    Need(hooks_.vertical)();
    if (!(Global(0x5c78d4)&1U)) { const auto label=Text("Direct Draw / Full Screen");const auto enabled=Global(0x5c7910);Need(hooks_.item)(label,Global(0x542948),std::string("DirectDrawManager"),1,enabled); }
    else { Item("Adjust Direct Draw",0x542948,std::string("AdjustDirectDraw"));Item("Window Mode",0x5427b8,10U); }
    Need(hooks_.vertical)();
    // 원본의해상도세행은현재폭을먼저읽고같으면높이를추가조회한다.
    for (const auto& size:kOriginalScreenResolutions) {
        const auto check=Global(0x531860)==size[0] && Global(0x531864)==size[1];const auto label=Text(std::to_string(size[0])+" by "+std::to_string(size[1]));
        Check(check,label,0x542948,std::string("SETSIZE(")+std::to_string(size[0])+','+std::to_string(size[1])+')');
    }
    for (const auto& size:hooks_.compatibilityResolutions) { // 원본에 없던 화면비는 같은 현재 폭/높이 체크와 원본 확인·저장·종료 Tell을 사용한다.
        const auto check=Global(0x531860)==size[0] && Global(0x531864)==size[1];const auto label=Text(std::to_string(size[0])+" by "+std::to_string(size[1]));
        Check(check,label,0x542948,std::string("SETSIZE(")+std::to_string(size[0])+','+std::to_string(size[1])+')');
    }
    auto label=Text("Resolution");auto child=Need(hooks_.take)();Need(hooks_.submenu)(label,child,1,0);Need(hooks_.separator)();
    Toggle("sound","Sound On",0x5c7b14);Toggle("music","Play Music",0x5c7b14);Toggle("playWind","Wind Noise",0x5c7b14);Toggle("swapLeftRightSpeakers","Speaker Swap L/R",0x5c7b14);
    // 각음량하위는현재전역을매행조회하고제목번역뒤현재장치활성을읽는다.
    for (std::uint32_t channel=0;channel<2;++channel) {
        Need(hooks_.vertical)();
        // 원본음량1..5의체크와실제숫자인자를보존한다.
        for (std::uint32_t volume=1;volume<=5;++volume) { label=Text("Volume "+std::to_string(volume));Check(Global(channel ? 0x531904 : 0x531900)==volume,label,channel ? 0x542a90 : 0x542a8c,volume); }
        label=Text(channel ? "Music Volume" : "Sound Effect Volume");child=Need(hooks_.take)();Need(hooks_.submenu)(label,child,Global(0x5c7b14),0);
    }
    Need(hooks_.separator)();Toggle("edgeScroll","Edge Scroll in Fullscreen");
    if (Global(0x594fbc) || Global(0x5c85a4)) Toggle("cursorSnapToGrid","Cursor Snap To Grid");
    Toggle("autoDemo","Auto-Demo");Toggle("tellTips","Tell Tips at Startup");
    if (Global(0x594fa4) || Global(0x5c85a4)) { label=Text("Pause - ~iShift-F9~.");Check(std::bit_cast<std::int32_t>(Global(0x55b4b0))>0,label,0x542a30); }
    Need(hooks_.separator)();
    if (!Global(0x594fb8) && !Global(0x594fc8)) Toggle("rotmode","Counterclockwise Piece Rotation - ~iC~.");
    else { std::int32_t passed=0;Need(hooks_.readInt)("passedDiags",passed);label=Text("Pass Server Diagnostic ");Check(std::bit_cast<std::uint32_t>(passed),label,0x542948,std::string("ChangeFirewall")); }
    return Need(hooks_.take)();
}
// Players는현재Player/사제/관전상태를읽으며각하위는기존전체RawPlayerMenu를사용한다.
std::uint32_t RawGameMenus::Players(bool fancy) const {
    if (Global(0x594fc8)) return 0;
    if (fancy) { Need(hooks_.fancy)();Need(hooks_.title)(Text("Player List\n")); }else Need(hooks_.vertical)();
    // 현재Player1..8만원본stride180순서로방문하고subscriber0은갱신도하지않는다.
    for (std::uint32_t owner=1;owner<=8;++owner) {
        if (!PlayerWord(owner,0x2c)) continue;Need(hooks_.refreshPriest)(owner);
        const auto checked=PlayerWord(owner,0x50)&0x21U;std::string label; // 원본은후속번역/하위효과전에체크를저장한다.
        if (!Global(0x594fa4)) label=CString(Need(hooks_.rankPicture)(Need(hooks_.rank)(Number(owner)-1U)));
        const auto player=Need(hooks_.player)(owner);if (player.size()<180) throw std::out_of_range("현재Player이름몸체범위");std::size_t length=0; // +c의최대32BYTE이름을그대로붙인다.
        // 원본이읽는첫NUL까지의원문바이트만사용한다.
        while (length<32 && player[0xc+length]) ++length;
        if (length==32) throw std::length_error("원본Player이름의NUL누락");label.append(reinterpret_cast<const char*>(player.data()+0xc),length);
        if (!Global(0x594fa4)) {
            const auto pattern=Text(", Level %d");label+=CString(Need(hooks_.format)(pattern,static_cast<std::uint8_t>(Need(hooks_.level)(owner))));
            if (!Global(0x594fa4)) {
                if (PlayerWord(owner,0x50)&8U) label+=Text(" - SERVER");
                if (!Global(0x594fa4) && Global(0x52f81e+Number(owner))) label+=Text(" OBSERVER");
            }
        }
        const auto enabled=Global(0x52f81e+Number(owner))==0;const auto child=Need(hooks_.playerMenu)(PlayerWord(owner,0x2c)); // 자식생성이바꾼관전/체크값을앞당겨다시읽지않는다.
        Need(hooks_.submenu)(label,child,enabled ? 1U : 0U,checked);
    }
    const auto menu=Need(hooks_.current)(); // 원본은take전에현재토큰을별도로기억한다.
    if (!fancy) return Need(hooks_.take)();
    const auto parent=Global(0x563e38);const auto height=std::bit_cast<std::int32_t>(Global(0x531864));const auto node=Need(hooks_.node)(menu); // 원본전체화면높이정수반과현재메뉴높이다.
    if (!node) throw std::logic_error("현재Player목록메뉴노드소멸");const auto y=Ftol(static_cast<double>(height/2)-static_cast<double>(node->baseHeight)*0.5);
    const auto width=std::bit_cast<std::int32_t>(Global(0x531860));const auto current=hooks_.node(menu);if (!current) throw std::logic_error("현재Player목록폭조회전소멸");
    const auto x=Ftol(static_cast<double>(width/2)-static_cast<double>(current->baseWidth)*0.5);Need(hooks_.attachCurrent)(x,y,parent);return menu;
}
// GUI경계는동일공통트리의원본몸체를사용하고숫자인자를문자열로바꾸지않는다.
void BindGameMenusControls(GameMenusHooks& hooks,const MenuGumpWidgets& widgets,const MenuGumpControls& controls,const MenuGump& menus,std::function<std::uint32_t(std::string_view)> storeText) {
    hooks.vertical=[&widgets] { static_cast<void>(widgets.BeginVertical(0)); };hooks.fancy=[&widgets] { static_cast<void>(widgets.BeginFancy()); };hooks.separator=[&controls] { controls.AddSeparator(); };hooks.title=[&controls](std::string_view text) { controls.AddTitle(text); };
    hooks.take=[&menus] { return menus.TakeCurrent(); };hooks.current=[&menus] { return menus.Current(); };hooks.attachCurrent=[&menus](std::int32_t x,std::int32_t y,std::uint32_t parent) { menus.AttachCurrent(x,y,parent); };
    const auto parameter=[storeText=std::move(storeText)](const GameMenuParameter& input) {
        if (std::holds_alternative<std::uint32_t>(input)) return std::get<std::uint32_t>(input);return Need(storeText)(std::get<std::string>(input));
    }; // 현재문자열인자만같은실제고리에저장한다.
    hooks.item=[&controls,parameter](std::string_view label,std::uint32_t code,const GameMenuParameter& input,std::uint32_t include,std::uint32_t enabled) { controls.AddText(label,code,parameter(input),include!=0,enabled!=0,false); };
    hooks.checked=[&controls,parameter](std::uint32_t check,std::string_view label,std::uint32_t code,const GameMenuParameter& input,std::uint32_t enabled) { controls.AddChecked(check!=0,label,code,parameter(input),true,enabled!=0); };
    hooks.submenu=[&controls](std::string_view label,std::uint32_t child,std::uint32_t enabled,std::uint32_t check) { controls.AddSubmenu(label,child,enabled!=0,check!=0); };
}
}
