// NSInfo 일반Attach의 실제310관측을 같은 배경/현재포인터/전역/부분쓰기로 대조한다.
#include "GumpRawSupport.h"
#include "client/HudNsInfo.h"
#include <fstream>
#include <map>
#include <memory>
#include <string_view>

using namespace netstorm::client;
namespace {
namespace raw=netstorm::test::gumpraw;
constexpr std::uint32_t kNsNodeAddress=0x01000100,kNsNodeStride=0x200; // 격리원문 포인터를 검사토큰과 구별한다.
constexpr std::size_t kNsNodeCount=8,kNsRawSize=0x11c,kNsRows=310; // 기존격리장치의 보존영역까지 전부 비교한다.
struct NsExplicitStop {}; // 정상RET와 구별하는 명시callback중단이다.

// 입력의구분자를 그대로 보존하며 마지막필드도 읽는다.
std::vector<std::string> NsSplit(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t begin{}; // 현재필드와복사목록이다.
    for (;;) { // 다음구분자까지의원문만 복사한다.
        const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1;
    }
}
// signed/unsigned입력을같은DWORD비트로읽는다.
std::uint32_t NsNumber(std::string_view text) { return static_cast<std::uint32_t>(std::stoll(std::string(text))); }
// 원문주소를0또는1기반안정토큰으로바꾼다.
std::uint32_t NsToken(std::uint32_t address) {
    if (!address) return 0;if (address<kNsNodeAddress || (address-kNsNodeAddress)%kNsNodeStride || (address-kNsNodeAddress)/kNsNodeStride>=kNsNodeCount) throw std::runtime_error("NSInfo원문주소");return (address-kNsNodeAddress)/kNsNodeStride+1;
}
// 안정토큰을원문고정주소로관측한다.
std::uint32_t NsAddress(std::uint32_t token) { return token ? kNsNodeAddress+(token-1)*kNsNodeStride : 0; }
// 실제float32인자/필드를원시비트로관측한다.
std::uint32_t NsBits(float value) { return std::bit_cast<std::uint32_t>(value); }

// 앞6열만 입력으로 사용하며 기대출력에 의존하지 않는 명시장치다.
struct NsScene {
    HudNsInfoNode ns; // 부모처리기와공통노드가공유하는실제확장노드다.
    std::array<GumpCreateNode,kNsNodeCount> bases{}; // 나머지7노드의안정된기본주소다.
    std::array<std::vector<std::uint8_t>,kNsNodeCount> preserved; // 본체가읽지않는확장영역도보존해비교한다.
    std::array<std::uint32_t,24> globals{}; // 기존장치22개와현재meta/playerKnown이다.
    std::vector<std::vector<std::string>> effects; // 입력의명시콜백효과다.
    std::map<std::string,std::size_t> calls; // 경계별1기반실제호출번호다.
    std::string trace; // 외부효과의실제순서와인자다.
    std::unique_ptr<HudNsInfo> handler; // 입력을설치한뒤같은참조로생성한다.

    // 같은현재노드를반환하며NSInfo자신의상속주소를공유한다.
    GumpCreateNode& Base(std::uint32_t token) { if (!token || token>kNsNodeCount) throw std::runtime_error("NSInfo검사토큰");return token==1 ? static_cast<GumpCreateNode&>(ns) : bases[token-1]; }
    // 원문전체입력을같은기본필드와현재배경포인터로읽는다.
    void Decode(std::size_t index,std::span<const std::uint8_t> bytes) {
        preserved[index].assign(bytes.begin(),bytes.end());auto base=raw::Decode(bytes);base.parent=NsToken(base.parent);Base(static_cast<std::uint32_t>(index+1))=base;if (!index) ns.background=NsToken(raw::Get(bytes,0xa0));
    }
    // 실제현재필드만원문offset으로쓰고읽지않는확장은그대로보존한다.
    std::vector<std::uint8_t> Encode(std::size_t index) const {
        auto output=preserved[index];auto base=index ? bases[index] : static_cast<GumpCreateNode>(ns);base.vtable=0;base.parent=NsAddress(base.parent);const auto bytes=raw::Bytes(raw::Encode(base));std::copy(bytes.begin(),bytes.end(),output.begin());if (!index) raw::Put(output,0xa0,NsAddress(ns.background));return output;
    }
    // 기대기하계산없이앞6열의명시변경만실제현재저장소에적용한다.
    void Effect(const std::vector<std::string>& effect) {
        const auto value=NsNumber(effect[3]);
        if (effect[1].starts_with("node")) {
            static const std::map<std::string,std::size_t> fields{{"flags",4},{"baseWidth",0x3c},{"baseHeight",0x40},{"scale",0x38},{"background",0xa0}}; // 입력효과에서허용한실제raw필드다.
            const auto index=std::stoull(effect[1].substr(4));auto bytes=Encode(index);raw::Put(bytes,fields.at(effect[2]),value);Decode(index,bytes);
        } else if (effect[1]=="global") {
            if (effect[2]=="metamap") globals[22]=value;else if (effect[2]=="playerKnown") globals[23]=value;else throw std::runtime_error("NSInfo전역효과");
        } else if (effect[1]!="control") throw std::runtime_error("NSInfo효과대상");
    }
    // 기본장치효과뒤같은호출번호의입력변경과부분중단을공급한다.
    void Event(std::string name,std::initializer_list<std::uint32_t> values={}) {
        if (!trace.empty()) trace+=';';trace+=name;
        for (auto value:values) trace+=':'+std::to_string(value); // 실제전달한인자를모두기록한다.
        const auto trigger=name+std::to_string(++calls[name]);bool stop{};
        for (const auto& effect:effects) { // 같은경계의여러변경을입력순서대로실행한다.
            if (effect[0]!=trigger) continue;Effect(effect);if (effect[1]=="control" && effect[2]=="stop" && effect[3]=="1") stop=true;
        }
        if (stop) throw NsExplicitStop{};
    }
    // Native장치의기본Attach상태쓰기와같은최소외부계약이다.
    void Attach(std::string name,std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) {
        auto& node=Base(token);node.flags&=~8U;node.x=x;node.y=y;if (parent) node.parent=parent;Event(std::move(name),{token,NsBits(x),NsBits(y),parent,mode});
    }
    // 공통부착/가상Push/가상Attach및명시미복원메타맵경계를연결한다.
    HudNsInfoHooks Hooks() {
        HudNsInfoHooks hooks;hooks.node=[this](auto token) { return token==1 ? &ns : nullptr; };hooks.base=[this](auto token) { return &Base(token); };
        hooks.baseAttach=[this](auto token,auto x,auto y,auto parent,auto mode) { Attach("baseAttach",token,x,y,parent,mode); };hooks.push=[this](auto token) { Base(token).flags|=8;Event("push",{token}); };
        hooks.attach=[this](auto token,auto x,auto y,auto parent,auto mode) { Attach("attach",token,x,y,parent,mode); };hooks.metamapLayout=[this](auto token) { Event("metamapLayout",{token}); };return hooks;
    }
    // 앞6열만읽어원문실행과동일한장치/현재전역입력을만든다.
    explicit NsScene(const std::vector<std::string>& row) {
        const auto bytes=raw::Bytes(row[2]);if (bytes.size()!=kNsNodeCount*kNsRawSize) throw std::runtime_error("NSInfo입력raw폭");
        for (std::size_t i=0;i<kNsNodeCount;++i) Decode(i,std::span<const std::uint8_t>(bytes).subspan(i*kNsRawSize,kNsRawSize)); // 전체8노드를초기화한다.
        const auto fields=NsSplit(row[3],':');if (fields.size()!=globals.size()) throw std::runtime_error("NSInfo전역입력폭");
        for (std::size_t i=0;i<globals.size();++i) globals[i]=NsNumber(fields[i]); // 현재전역을원시DWORD로공급한다.
        if (row[5]!="-") for (const auto& field:NsSplit(row[5],';')) effects.push_back(NsSplit(field,'/')); // 기대출력을읽지않는변경입력이다.
        handler=std::make_unique<HudNsInfo>(HudNsInfoGlobals{globals[22],globals[23]},Hooks());
    }
    // 읽지않은영역을포함한모든현재노드를원문주소순서로관측한다.
    std::string Nodes() const { std::string output;for (std::size_t i=0;i<kNsNodeCount;++i) output+=raw::Hex(Encode(i));return output; /* 고정8노드순회다. */ }
    // 사용하지않은전역도변경되지않았는지모두관측한다.
    std::string Globals() const { std::string output;for (auto value:globals) { if (!output.empty()) output+=':';output+=std::to_string(value); } return output; /* 원문전역순서다. */ }
};
}

//310행전체를비교하되메타맵본문의완료검증으로확장하지않는다.
TEST_CASE(HudNsInfo_IndependentNativeNormalAttachment) {
    std::ifstream input(NETSTORM_HUD_NSINFO_ATTACH_FIXTURE);CHECK(input.good());std::string line;std::size_t rows{},stops{};
    while (std::getline(input,line)) { // 뒤8열은관측비교용으로만사용한다.
        if (line.empty() || line.starts_with("name\t")) continue;const auto row=NsSplit(line,'\t');CHECK(row.size()==14);if (row.size()!=14) continue;const auto failures=netstorm::test::FailureCount();NsScene scene(row);
        const auto parts=NsSplit(row[4],':');CHECK(parts.size()==4);if (parts.size()!=4) continue;bool completed=true;
        try { scene.handler->Attach(1,std::bit_cast<float>(NsNumber(parts[0])),std::bit_cast<float>(NsNumber(parts[1])),NsNumber(parts[2]),NsNumber(parts[3])); }
        catch (const NsExplicitStop&) { completed=false;++stops; }
        CHECK(completed==(row[6]=="1"));CHECK(scene.Nodes()==row[7]);CHECK(scene.Globals()==row[8]);CHECK((scene.trace.empty() ? "-" : scene.trace)==row[9]);
        if (completed) CHECK(row[11]=="20");CHECK(NsNumber(row[12])>0 && row[13]=="0");++rows; // void/미완료EAX는의미반환값으로비교하지않는다.
        if (netstorm::test::FailureCount()!=failures) std::fprintf(stderr,"NSInfo native 입력: %s\n",row[0].c_str());
    }
    CHECK(rows==kNsRows);CHECK(stops==8);
}

// 실제후속메타맵을공급하지않으면일반부착부분쓰기뒤명시거부한다.
TEST_CASE(HudNsInfo_MetamapNeedsActualContinuation) {
    HudNsInfoNode ns;ns.background=2;ns.baseWidth=80;ns.baseHeight=75;GumpCreateNode background;background.scale=1;const std::uint32_t metamap=1,known=1;std::size_t attached{};
    HudNsInfoHooks hooks;hooks.node=[&](auto) { return &ns; };hooks.base=[&](auto)->GumpCreateNode* { return &background; };hooks.baseAttach=[](auto,auto,auto,auto,auto) {};hooks.push=[](auto) {};hooks.attach=[&](auto,auto,auto,auto,auto) { ++attached; };
    HudNsInfo handler({metamap,known},std::move(hooks));bool rejected{};try { handler.Attach(1,0,0,0,0); } catch (const std::runtime_error&) { rejected=true; }
    CHECK(rejected && attached==1);CHECK(background.baseWidth==80 && background.baseHeight==75 && background.width==80 && background.height==75);
}
