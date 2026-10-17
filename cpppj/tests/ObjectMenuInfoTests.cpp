// 원본객체메뉴정보의전체반환출력/효과순서/현재필드를독립fixture와대조한다.
#include "RawSceneSupport.h"
#include "client/RawObjectMenuInfo.h"
#include <bit>
#include <filesystem>
#include <stdexcept>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;

namespace {
// 바이트문자열을호스트인코딩변환없이원문hex로직렬화한다.
std::string InfoHex(std::string_view value) { return Hex({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); }
// fixture의20DWORD를unsigned십진수로읽어signed좌표/마스크비트를보존한다.
std::vector<std::uint32_t> InfoNumbers(std::string_view text) {
    std::vector<std::uint32_t> result; // 독립입력의원문비트다.
    // 모든숫자를원래순서대로읽으며기대분기를계산하지않는다.
    for (const auto& field:Split(text,',')) result.push_back(Number(field));return result;
}
// 같은실제풀/현재TYPE와명시외부경계입력의고정주소소유자다.
struct InfoScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 원본과같은50바이트물리슬롯이다.
    std::array<RiftTypeRecord,188> types{}; // 효과사이에서다시읽는현재TYPE다.
    std::array<std::string,188> classes; // 실제TYPE+aa의현재문자열입력이다.
    ObjectMenuInfoState state; // 현재로컬/편집/미션getter와경고그림TYPE다.
    std::array<std::uint32_t,81> alliances{}; // 원본owner행/local열입력이다.
    std::vector<std::uint32_t> row; // operation을포함한20개독립입력이다.
    std::uint32_t local{},editor{},mission{},titles{},headers{}; // 실제현재전역과선택된GUI경계횟수다.
    std::string events; // 원본과대조할외부효과의전체인자/바이트다.
    // 물리슬롯을실제목록에서확보하고현재값getter를단한번연결한다.
    InfoScene() {
        // 독립실행이사용할50번까지정상클라이언트영역을확보한다.
        for (std::uint16_t sid=5;sid<=50;++sid) CHECK(pool.Allocate(2).value==sid);
        state.localOwner=[this] { return local; };state.editor=[this] { return editor; };state.missionPost=[this] { return mission; };
    }
    // 이전실행의상태를독립입력으로다시설치하며기대값은참조하지않는다.
    void Prepare(std::vector<std::uint32_t> values) {
        row=std::move(values);CHECK(row.size()==20);events.clear();local=row[7];editor=row[8];mission=row[12];titles=headers=0;state.invisible=117;state.paralyzed=118;
        auto raw=pool.AllocatedBytes(Sid{50});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x12200000);raw[10]=static_cast<std::uint8_t>(row[1]);raw[34]=static_cast<std::uint8_t>(row[2]);raw[40]=static_cast<std::uint8_t>(row[10]);Put(raw,14,row[16]);Put(raw,18,row[17]);
        classes.fill({});types={};types[row[1]].flags2=row[5];types[row[1]].theme=std::bit_cast<std::int32_t>(row[6]);classes[row[1]]=row[4] ? "Class % value" : "";
        alliances.fill(0);alliances[row[2]*9+row[7]]=row[14];
    }
    // 경계가바꿀현재입력만적용하며제품의부모분기/문구선택은대신실행하지않는다.
    void Mutate(std::string_view boundary) {
        auto raw=pool.AllocatedBytes(Sid{50});const auto kind=row[1]; // 각독립입력의현재물리객체와원래정보TYPE다.
        if (row[15]==1 && boundary=="count") { raw[10]=101;raw[34]=2;raw[40]=0x30; }
        if (row[15]==2 && boundary=="description") classes[kind].clear();
        if (row[15]==3 && boundary=="firstTitle") { classes[kind]="Changed";types[kind].flags2=0x10000;types[kind].theme=4; }
        if (row[15]==4 && boundary=="relation") { editor=1;Put(raw,14,std::bit_cast<std::uint32_t>(-3.75F));Put(raw,18,std::bit_cast<std::uint32_t>(4.5F)); }
        if (row[15]==5 && boundary=="subId") { Put(raw,14,std::bit_cast<std::uint32_t>(-17.5F));Put(raw,18,std::bit_cast<std::uint32_t>(21.5F)); }
        if (row[15]==6 && boundary=="name") { local=2;alliances[row[2]*9+2]=1; }
        if (row[15]==7 && boundary=="invisibleHeader") raw[40]=0;
        if (row[15]==8 && boundary=="invisibleTranslation") { state.invisible=99;raw[40]=0x20; }
        if (row[15]==9 && boundary=="ground") { mission=1;raw[34]=2; }
        if (row[15]==10 && boundary=="alignment") types[kind].theme=5;
        if (row[15]==11 && boundary=="class") classes[kind]="Replacement % token";
        if (row[15]==12 && boundary=="firstHeader") classes[kind].clear();
        if (row[15]==13 && boundary=="lastHeader") raw[40]=0x30;
    }
    // 원본관찰과같은구분문자로현재경계의인자만직렬화한다.
    void Event(std::string_view letter,std::string_view argument) { if (!events.empty()) events+=';';events+=letter;events+=':';events+=argument; }
    // 현재번역입력은원본바이트그대로반환하며순서가선택한변경만공급한다.
    std::string Translate(std::string_view key) {
        const std::string text(key);Event("L",InfoHex(text));if (text=="Owner" || text=="Ally" || text=="Enemy") Mutate("relation");
        if (text=="~y~[I%d.*]Invisible!~.\n") Mutate("invisibleTranslation");if (text=="\nAlignment: ~i%s~.") Mutate("alignment");if (text=="\nClass: ~i%s~.") Mutate("class");
        return (row[18] ? std::string(1,static_cast<char>(0xe9))+":" : std::string{})+text;
    }
    // 순수고정printf경계만실행한다. 번역/경고/소유자분기는제품모듈에있다.
    std::string Print(std::string_view pattern,std::span<const PriestMenuArgument> arguments) {
        std::string result;std::size_t argument=0; // 현재서식의외부인자를원래순서로소비한다.
        // 원본명시경계와같은%s/%d/%%만해석한다.
        for (std::size_t index=0;index<pattern.size();++index) {
            if (pattern[index]!='%') { result+=pattern[index];continue; }
            if (++index==pattern.size()) throw std::logic_error("정보검사서식끝");if (pattern[index]=='%') { result+='%';continue; }
            if (argument>=arguments.size()) throw std::logic_error("정보검사서식인자부족");
            if (pattern[index]=='s') result+=std::get<std::string>(arguments[argument++]);else if (pattern[index]=='d') result+=std::to_string(std::get<std::int32_t>(arguments[argument++]));else throw std::logic_error("정보검사서식종류");
        }
        CHECK(argument==arguments.size());Event("F",InfoHex(result));return result;
    }
    // 실제몸체가선택한외부소비자의반환/입력변경만제공한다.
    ObjectMenuInfoHooks Hooks() {
        ObjectMenuInfoHooks hooks;hooks.count=[this](Sid sid) { Event("C",std::to_string(sid.value));Mutate("count");return row[3]; };
        hooks.description=[this](std::uint32_t kind) { Event("D",std::to_string(kind));Mutate("description");return "Type"+std::to_string(kind); };
        hooks.ownerName=[this](std::uint32_t owner) { Event("P",std::to_string(owner));Mutate("name");return row[19] ? "Player"+std::to_string(owner) : std::string("UNKNOWN"); };
        hooks.translate=[this](std::string_view key) { return Translate(key); };hooks.substitute=[this](std::string_view value) { Event("S",InfoHex(value));return '['+std::string(value)+']'; };
        hooks.print=[this](std::string_view pattern,std::span<const PriestMenuArgument> values) { return Print(pattern,values); };
        hooks.title=[this](std::string_view text) { Event("T",InfoHex(text));if (++titles==1) Mutate("firstTitle"); };
        hooks.header=[this](std::string_view text) { Event("H",InfoHex(text));if (++headers==1) Mutate("firstHeader");if (text.starts_with("\nSubID:")) Mutate("subId");if (text.find("Invisible!")!=std::string_view::npos) Mutate("invisibleHeader");if (text=="\n") Mutate("lastHeader"); };
        hooks.alliance=[this](std::uint32_t owner,std::uint32_t current) { return alliances.at(owner*9+current); };
        hooks.groundImmobile=[this](Sid sid) { Event("G",std::to_string(sid.value));Mutate("ground");return row[11]!=0; };hooks.ownerStatus=[this](std::uint32_t owner) { CHECK(owner<9);return static_cast<std::uint8_t>(row[13]); };return hooks;
    }
    // 원본반환뒤남은현재raw/전역/class/TYPE필드만출력한다.
    std::string Current() const {
        return Hex(pool.Slot(Sid{50}))+'|'+std::to_string(local)+','+std::to_string(editor)+','+std::to_string(mission)+','+std::to_string(state.invisible)+','+std::to_string(state.paralyzed)+'|'+InfoHex(classes[row[1]])+'|'+std::to_string(types[row[1]].flags2)+','+std::to_string(std::bit_cast<std::uint32_t>(types[row[1]].theme));
    }
};
}
// 전체원본입력의현재필드와모든번역/서식/GUI효과순서를재생한다.
TEST_CASE(ObjectMenuInfo_NativeWholeParentsAndStringHelpers) {
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/object-menu-info-x86.tsv";InfoScene scene;std::size_t count{},mismatches{}; // 기대값은독립PE출력TSV에서만읽고첫불일치인자를진단한다.
    // 원문행을빠뜨리지않고반환문자열/현재입력과전체효과를대조한다.
    for (const auto& fields:LoadFixture(path.string().c_str()).rows) {
        CHECK(fields.size()==4);if (fields.size()!=4) continue;scene.Prepare(InfoNumbers(fields[0]));RawObjectMenuInfo info(scene.pool,scene.types,scene.classes,scene.state,scene.Hooks());std::string result="-";
        if (scene.row[0]==0) info.Describe(scene.row[1],scene.row[3],scene.row[2],scene.row[9]);else if (scene.row[0]==1) info.Summary(Sid{50});else if (scene.row[0]==2) info.CaptureStatus(Sid{50});
        else if (scene.row[0]==3) result=InfoHex(RawObjectMenuInfo::LevelName(scene.row[3]));else result=InfoHex(scene.Translate(RawObjectMenuInfo::ThemeName(std::bit_cast<std::int32_t>(scene.row[6]))));
        if ((scene.events.empty() ? "-" : scene.events)!=fields[1] && mismatches++<2) std::printf("정보입력 %s\n실제 %s\n원본 %s\n",fields[0].c_str(),scene.events.c_str(),fields[1].c_str());
        CHECK((scene.events.empty() ? "-" : scene.events)==fields[1]);CHECK(result==fields[2]);CHECK(scene.Current()==fields[3]);++count;
    }
    CHECK(count==1434);
}
// 실제거짓492090/비영미션의short-circuit과미연결제목소비자오류를확인한다.
TEST_CASE(ObjectMenuInfo_SelectedBoundariesAndEditionContract) {
    InfoScene scene;std::array<RiftTypeRecord,188> types{};std::array<std::string,188> classes;ObjectMenuInfoState state;ObjectMenuInfoHooks hooks; // 선택하지않을외부효과는채우지않는다.
    hooks.groundImmobile=[](Sid) { return false; };RawObjectMenuInfo noCapture(scene.pool,types,classes,state,hooks);noCapture.CaptureStatus(Sid{50});
    hooks.groundImmobile=[](Sid) { return true; };state.missionPost=[] { return 1U; };std::string header;hooks.translate=[](std::string_view text) { return std::string(text); };hooks.header=[&](std::string_view text) { header=text; };
    RawObjectMenuInfo connected(scene.pool,types,classes,state,hooks);connected.CaptureStatus(Sid{50});CHECK(header=="~yCapturable!~.\n");
    bool rejected{};try { connected.Describe(100,2,1,50); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    SidPool cd(OriginalEdition::Cd1072,15000,true);rejected=false;try { RawObjectMenuInfo invalid(cd,types,classes,state,{}); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
