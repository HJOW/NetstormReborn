// 최초876원문관측의현재값·모든raw바이트·두Snapshot/Notify/삭제요청순서를직접대조한다.
#include "TestSupport.h"
#include "o/RawFortCurrentName.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
using namespace netstorm::o;

namespace {
constexpr std::array<std::uint32_t,3> kManagerTokens{0,0x12001000,0x12002000}; //원문가상manager주소를직접보존하는명시토큰이다.
// fixture의빈마지막trace칸까지순서대로분리한다.
std::vector<std::string> NameFields(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t start{}; //분리된필드와현재위치다.
    // 마지막빈필드를버리지않는다.
    for (;;) { const auto end=text.find(separator,start);result.emplace_back(text.substr(start,end==text.npos ? text.npos : end-start));if (end==text.npos) return result;start=end+1; }
}
// 고정원문DWORD를비정렬little endian으로읽는다.
std::uint32_t NameRead(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t result{}; //정확한네BYTE의결과다.
    // 낮은BYTE부터조립한다.
    for (std::size_t byte=0;byte<4;++byte) result|=std::uint32_t(bytes[offset+byte])<<(byte*8);return result;
}
// 명시입력/callback효과의DWORD폭만쓴다.
void NamePut(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    // 필드밖바이트는유지한다.
    for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8));
}
// NUL뒤도버리지않고모든BYTE를소문자hex로관측한다.
std::string NameHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view kDigits="0123456789abcdef";std::string result; //각BYTE를두글자로표현한다.
    // 전체물리범위의순서를유지한다.
    for (const auto byte:bytes) { result+=kDigits[byte>>4];result+=kDigits[byte&15U]; }return result;
}
// 하위장치의NUL종료입력만읽고미종료를임의빈문자열로바꾸지않는다.
std::span<const std::uint8_t> NameText(std::span<const std::uint8_t> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("FortName장치NUL범위");return bytes.first(static_cast<std::size_t>(end-bytes.begin()));
}
// 하위PathSpec/이름초기입력의NUL뒤바이트까지복사한다.
void NameInput(std::span<std::uint8_t> target,std::string_view value) {
    if (value.size()>=target.size()) throw std::out_of_range("FortName장치입력크기");std::copy(value.begin(),value.end(),target.begin());target[value.size()]=0;
}
// 원본하위callback에서의명시중단표식이다.
struct NameStop { std::string cause; }; //제품의알수없는예외는이표식으로삼키지않는다.
// 기대열을읽지않고원문과동일초기입력/명시하위응답만공급한다.
struct NameDevices {
    std::string mode,mutation,trace,physical; //독립입력과실제경계순서/물리대상이다.
    std::uint32_t owner{},other{},current{},dirty{0xfffffffeU},fort{},editor{},mission{},pathMode{},kind{},newKind{},lastHeader{}; //현재전역과독립입력이다.
    std::uint32_t snapshots{},notifies{},paths{},deletes{},configs{}; //실제하위경계횟수다.
    bool specPointer{}; //005327e0이FileSpec버퍼를선택했는지다.
    std::array<std::uint8_t,180> headers{}; //고정9×20BYTE헤더의실제물리저장소다.
    std::array<std::array<std::uint8_t,276>,2> managers{}; //두manager의이름외주변바이트까지관측한다.
    std::array<std::array<std::uint8_t,256>,2> names{}; //현재포인터가선택하는서로다른실제이름쓰기버퍼다.
    std::array<std::uint8_t,512> argument{}; //외부새이름입력이며callback의실제쓰기대상이다.
    std::array<std::uint8_t,9> spec{}; //선택포인터와달리내용은callback이바꿀수있다.
    std::array<std::uint8_t,256> scratch{}; //원문미초기화oldpath는명시BYTE입력으로공급한다.
    std::array<std::array<std::uint8_t,28>,9> states{}; //Stategetter반환후+17을읽는실제저장소다.
    std::vector<std::uint8_t> path; //다음PathSpec전까지유효한실제반환버퍼다.

    // 원문고정입력만설치하고이후본문효과를선계산하지않는다.
    explicit NameDevices(std::string_view input) {
        const auto fields=NameFields(input,',');if (fields.size()!=13) throw std::runtime_error("FortName fixture입력칸수오류");mode=fields[0];owner=static_cast<std::uint32_t>(std::stoul(fields[1]));other=(owner+1)%9;
        const auto manager=std::stoul(fields[2]);const auto readonly=static_cast<std::uint32_t>(std::stoul(fields[3]));const auto duplicate=static_cast<std::uint32_t>(std::stoul(fields[4]));const auto old=std::stoul(fields[5]);newKind=static_cast<std::uint32_t>(std::stoul(fields[6]));pathMode=static_cast<std::uint32_t>(std::stoul(fields[7]));fort=static_cast<std::uint32_t>(std::stoul(fields[8]));editor=static_cast<std::uint32_t>(std::stoul(fields[9]));mission=static_cast<std::uint32_t>(std::stoul(fields[10]));kind=static_cast<std::uint32_t>(std::stoul(fields[11]));mutation=fields[12];
        // 주변헤더입력은원문과동일하게유지한다.
        for (std::size_t byte=0;byte<headers.size();++byte) headers[byte]=static_cast<std::uint8_t>(0x70+byte);
        // 사용하지않는슬롯도manager0과독립State입력을갖는다.
        for (std::uint32_t slot=0;slot<9;++slot) {
            NamePut(headers,slot*20,0);for (std::size_t byte=0;byte<28;++byte) states[slot][byte]=static_cast<std::uint8_t>(0x80+byte); //주변State입력이다.
            states[slot][17]=static_cast<std::uint8_t>(kind);
        }
        NamePut(headers,other*20,kManagerTokens[2]);NamePut(headers,other*20+4,0);NamePut(headers,other*20+8,0);NamePut(headers,owner*20,kManagerTokens[manager/11]);NamePut(headers,owner*20+4,duplicate);NamePut(headers,owner*20+8,readonly);
        // 각manager의+c만NUL이름으로덮고그밖276BYTE입력을보존한다.
        for (std::size_t index=0;index<managers.size();++index) {
            const auto key=static_cast<std::uint32_t>((index+1)*11);for (std::size_t byte=0;byte<276;++byte) managers[index][byte]=static_cast<std::uint8_t>(0x91+byte+key); //서로다른주변입력이다.
            NameInput(std::span(managers[index]).subspan(12),"C:/prior"+std::to_string(key)+".fort");
        }
        constexpr std::array<std::string_view,3> kOldNames{"Old","","oLd"}; //대소문자/빈현재이름의명시입력이다.
        // 현재포인터를바꿔도이전버퍼의주소와NUL뒤바이트는유지한다.
        for (std::size_t index=0;index<names.size();++index) { names[index].fill(0xa7);NameInput(names[index],index ? std::string_view("Other") : kOldNames.at(old)); }
        argument.fill(0xb6);const std::array<std::string,8> newNames{"New","Old","oLd","Bad.Name","",std::string(255,'M'),std::string(256,'M'),std::string("New\0.after",10)}; //각입력의물리BYTE를따로구성한다.
        if (newKind==7) std::copy(newNames[7].begin(),newNames[7].end(),argument.begin());else if (newKind<8) NameInput(argument,newNames.at(newKind));spec.fill(0);NameInput(spec,"fortSpec");scratch.fill(0xcd);NameInput(scratch,"Prior");
    }
    // 동일argument주소를callback과본문에공급한다.8번은실제현재이름0의자기참조다.
    std::span<std::uint8_t> Argument() { return newKind==8 ? std::span<std::uint8_t>(names[0]) : std::span<std::uint8_t>(argument); }
    // 헤더의원문manager토큰을관측용명시번호로해석한다.
    std::uint32_t Manager(std::uint32_t slot) const {
        const auto token=NameRead(headers,slot*20);const auto found=std::find(kManagerTokens.begin(),kManagerTokens.end(),token);if (found==kManagerTokens.end()) throw std::logic_error("FortName장치manager토큰");return static_cast<std::uint32_t>(found-kManagerTokens.begin())*11;
    }
    // 실제진입순서를연결한다.
    void Record(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 명시callback효과만적용하고부모의관문/호출순서를대신판정하지않는다.
    void Mutate(std::string_view boundary,std::uint32_t fixed=9) {
        const auto selected=owner;const auto target=fixed<9 ? fixed : selected; //고정setterthis와현재owner를구별한다.
        if (mutation==std::string(boundary)+"-owner") owner=other;
        else if (mutation==std::string(boundary)+"-current1") current=1;
        else if (mutation=="snapshot1-manager0" && boundary=="snapshot1") NamePut(headers,selected*20,0);
        else if ((mutation=="snapshot1-readonly1" || mutation=="snapshot2-readonly1" || mutation=="notify-readonly1") && boundary==mutation.substr(0,mutation.find('-'))) NamePut(headers,target*20+8,1);
        else if (mutation=="snapshot1-name" && boundary=="snapshot1") NameInput(Argument(),"Changed");
        else if ((mutation=="notify-manager22" || mutation=="assert429-manager22" || mutation=="assert430-manager22" || mutation=="assert88-manager22") && boundary==mutation.substr(0,mutation.find('-'))) NamePut(headers,target*20,kManagerTokens[2]);
        else if ((mutation=="title-editor1" || mutation=="state-editor1" || mutation=="config1-editor1") && boundary==mutation.substr(0,mutation.find('-'))) editor=1;
        else if (mutation=="title-dirty" && boundary=="title") dirty=0xffffffffU;
        else if (mutation=="state-mission1" && boundary=="state") mission=1;
        else if (mutation=="path1-specsolo" && boundary=="path1") NameInput(spec,"SOLOSpec");
        else if (mutation=="path1-pointerother" && boundary=="path1") specPointer=false;
        else if (mutation==std::string(boundary)+"-stop") throw NameStop{std::string(boundary)};
    }
    // 조회함수는순수현재값을반환하며dirty는실제동일DWORD를빌린다.
    FortCurrentNameGlobals Globals() { return {[this] { return owner; },[this] { return fort; },[this] { return editor; },[this] { return mission; },dirty}; }
    // 실제하위장치만구성하고이름복사/헤더+8쓰기/두snapshot선택은제품본문이수행한다.
    FortCurrentNameHooks Hooks() {
        FortCurrentNameHooks hooks;
        hooks.header=[this](std::uint32_t slot)->std::span<std::uint8_t> { lastHeader=slot;if (slot>=9) throw std::out_of_range("FortName장치header");return std::span(headers).subspan(slot*20,20); };
        hooks.currentName=[this]()->std::span<std::uint8_t> { physical="current-size";return names.at(current); };
        hooks.managerName=[this](std::uint32_t token)->std::span<std::uint8_t> { if (!token) throw NameStop{"manager0"};const auto found=std::find(kManagerTokens.begin()+1,kManagerTokens.end(),token);if (found==kManagerTokens.end()) throw std::logic_error("FortName장치현재manager주소");physical="manager-name-size";return std::span(managers[static_cast<std::size_t>(found-kManagerTokens.begin()-1)]).subspan(12,256); };
        hooks.selectFileSpec=[this] { specPointer=true; };
        hooks.pathSpec=[this](std::span<const std::uint8_t> source)->std::span<const std::uint8_t> {
            const auto text=NameText(source),key=NameText(spec);const std::string name(reinterpret_cast<const char*>(text.data()),text.size());std::string result; //선택한장치입력으로반환경로만만든다.
            if (pathMode==1) result="C:/fixed.fort";
            else if (pathMode==3 || pathMode==4) { const auto length=pathMode==3 ? 255U : 256U;result=std::string(length>name.size()+5 ? length-name.size()-5 : 0,'P')+name+".fort"; }
            else result=(NameHex(key)=="666f727453706563" ? "C:/work/d/" : "C:/solo/d/")+name+".fort";
            if (pathMode==2) result+=std::string("\0TAIL",5);++paths;path.assign(result.begin(),result.end());path.push_back(0);Record("P:"+NameHex(key)+':'+NameHex(text)+':'+NameHex(std::span(path).first(path.size()-1)));physical="old-path-size";Mutate("path"+std::to_string(paths));return path;
        };
        hooks.assertion=[this](std::uint32_t line) { Record("A:"+std::to_string(line));Mutate("assert"+std::to_string(line),line==429 || line==430 ? lastHeader : 9); };
        hooks.snapshot=[this] { ++snapshots;Record("S:"+std::to_string(owner)+':'+std::to_string(Manager(owner)));Mutate("snapshot"+std::to_string(snapshots)); };
        hooks.notify=[this](std::uint32_t slot) { ++notifies;const auto manager=Manager(slot);const auto text=NameText(std::span(managers.at(manager/11-1)).subspan(12));Record("N:"+std::to_string(slot)+':'+std::to_string(manager)+':'+NameHex(text));Mutate("notify",slot); };
        hooks.samePath=[this](std::span<const std::uint8_t> old,std::span<const std::uint8_t> next) {
            old=NameText(old);next=NameText(next);Record("E:"+NameHex(old)+':'+NameHex(next));bool same=old.size()==next.size(); //명시C로케일CRT하위경계의ASCII대소문자비교다.
            // A..Z만소문자로바꾸며원래버퍼는수정하지않는다.
            for (std::size_t index=0;same && index<old.size();++index) { const auto a=old[index],b=next[index];same=(a>='A' && a<='Z' ? a+32 : a)==(b>='A' && b<='Z' ? b+32 : b); }
            Record(std::string("Q:")+(same ? '1' : '0'));return same;
        };
        hooks.deleteRequest=[this](std::span<const std::uint8_t> old) { ++deletes;Record("D:"+NameHex(NameText(old)));Mutate("delete"); }; //실제파일조작은절대수행하지않는다.
        hooks.refreshTitle=[this] { Record("T:"+NameHex(NameText(names.at(current))));Mutate("title"); };
        hooks.state=[this](std::uint32_t slot)->std::span<const std::uint8_t> { Record("G:"+std::to_string(slot));Mutate("state");return states.at(slot); };
        hooks.config=[this](std::string_view key,std::span<const std::uint8_t> value) { ++configs;Record("C:"+std::string(key)+':'+NameHex(NameText(value)));Mutate("config"+std::to_string(configs)); };return hooks;
    }
    // 현재전역/하위횟수만관측열로변환한다.
    std::string State() const {
        const std::array<std::uint32_t,12> values{owner,current,dirty,fort,editor,mission,specPointer ? 1U : 0U,snapshots,notifies,paths,deletes,configs};std::string result; //원문열의순서다.
        // 지정한12필드만직렬화한다.
        for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result;
    }
    // 헤더/manager주변/current/NUL뒤입력/FileSpec/지역oldpath를모두직접대조한다.
    std::string Raw() const { return NameHex(headers)+':'+NameHex(managers[0])+':'+NameHex(managers[1])+':'+NameHex(names[0])+':'+NameHex(names[1])+':'+NameHex(argument)+':'+NameHex(spec)+':'+(mode=="current" ? NameHex(scratch) : "-"); }
    // 실제본문만실행하고도달한물리접근/callback중단만분리한다.
    std::string Run() {
        try { auto hooks=Hooks();if (mode=="current") SetCurrentFortName(Argument(),scratch,Globals(),hooks);else SetFortManagerFileName(owner,Argument(),hooks);return "1"; }
        catch (const NameStop& stop) { return stop.cause; }
        catch (const std::out_of_range&) { if (physical.empty()) throw;return physical; }
    }
};
}

// 최초438입력/두정밀도876관측과명시중단156건까지모든변경을대조한다.
TEST_CASE(FortCurrentName_IndependentNative876) {
    std::ifstream input(NETSTORM_FORT_CURRENT_NAME_FIXTURE);if (!input) throw std::runtime_error("Fort현재이름최초fixture없음");std::string line;std::size_t compared{},stopped{};std::set<std::pair<std::string,std::string>> unique; //중복/누락관측을검사한다.
    // 입력열만으로본문을실행한뒤고정원문출력과대조한다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;const auto row=NameFields(line,'\t');if (row.size()!=6) throw std::runtime_error("Fort현재이름관측칸수오류");CHECK(unique.emplace(row[0],row[1]).second);
        NameDevices devices(row[1]);const auto completion=devices.Run();if (completion!="1") ++stopped;
        if (completion!=row[2] || devices.State()!=row[3] || devices.Raw()!=row[4] || devices.trace!=row[5]) {
            std::fprintf(stderr,"FortName input=%s\ncompletion=%s expected=%s\nstate=%s expected=%s\ntrace=%s\nexpected=%s\n",row[1].c_str(),completion.c_str(),row[2].c_str(),devices.State().c_str(),row[3].c_str(),devices.trace.c_str(),row[5].c_str());
            CHECK(completion==row[2]);CHECK(devices.State()==row[3]);CHECK(devices.Raw()==row[4]);CHECK(devices.trace==row[5]);return;
        }
        ++compared;
    }
    CHECK(compared==876 && stopped==156);
}

// 도달하지않는경계미연결/중단전부분효과/실제입력물리범위를원문fixture와별도로검사한다.
TEST_CASE(FortCurrentName_SelectedBoundariesAndPhysicalStop) {
    NameDevices minimal("current,1,0,0,0,0,0,0,0,0,0,0,none");auto hooks=minimal.Hooks();hooks.pathSpec={};hooks.snapshot={};hooks.notify={};hooks.deleteRequest={};hooks.state={};hooks.config={};SetCurrentFortName(minimal.Argument(),minimal.scratch,minimal.Globals(),hooks);CHECK(minimal.dirty==0xffffffffU && minimal.paths==0 && minimal.snapshots==0 && minimal.configs==0);
    NameDevices missing("current,1,11,0,0,0,0,0,1,0,0,0,none");hooks=missing.Hooks();hooks.notify={};bool rejected{};try { SetCurrentFortName(missing.Argument(),missing.scratch,missing.Globals(),hooks); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && missing.snapshots==1 && missing.notifies==0 && missing.dirty==0xfffffffeU);CHECK(NameHex(NameText(std::span(missing.managers[0]).subspan(12)))=="433a2f776f726b2f642f4e65772e666f7274"); //Notify미연결도이미복사한파일명을되돌리지않는다.
    NameDevices physical("current,1,0,0,0,0,0,0,0,0,0,0,none");hooks=physical.Hooks();std::array<std::uint8_t,2> shortName{};hooks.currentName=[&]()->std::span<std::uint8_t> { return shortName; };rejected=false;try { SetCurrentFortName(physical.Argument(),physical.scratch,physical.Globals(),hooks); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected && shortName[0]=='N' && shortName[1]=='e' && physical.dirty==0xfffffffeU);
    NameDevices unterminated("current,1,0,0,0,0,0,0,0,0,0,0,none");hooks=unterminated.Hooks();constexpr std::array<std::uint8_t,2> kNoNul{'A','B'};rejected=false;try { SetCurrentFortName(kNoNul,unterminated.scratch,unterminated.Globals(),hooks); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected && unterminated.snapshots==0 && unterminated.dirty==0xfffffffeU);
}
