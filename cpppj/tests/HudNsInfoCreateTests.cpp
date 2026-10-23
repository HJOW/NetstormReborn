// 원문NSInfo부모생성관측170행의현재노드/문자열전체/순서/부분중단을대조한다.
#include "TestSupport.h"
#include "client/HudNsInfoCreate.h"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::client;
namespace {
constexpr std::uint32_t kCreateNodes=0x01100000,kCreateStrings=0x01200000,kCreateTable=0x01000000,kCreateServer=0x00594970,kCreateFormat=0x01f00000; // 명시독립장치의가상주소다.
constexpr std::size_t kCreateRows=170,kCreateColumns=18; // 실제85입력의두정밀도행이다.
constexpr std::array<std::uint32_t,13> kCreateGlobals{0x540421,0x540440,0x540457,0x5ca920,0x5ca924,0x5467b8,0x5411c8,0x5bec3c,0x54db4c,0x540bac,0x594fc8,0x595344,0x593150}; // 원문현재전역순서다.
enum class CreateKind { Base,Ns,Player,Battle,Button }; // 같은기본필드가속한실제파생저장소다.
struct CreateStop {}; // 명시장치중단을정상생성RET와구별한다.

// 빈마지막칸도보존하는원문구분자판독이다.
std::vector<std::string> CreateSplit(std::string_view text,char separator) {
    std::vector<std::string> output;std::size_t begin{};
    for (;;) { const auto end=text.find(separator,begin);output.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return output;begin=end+1; } // 입력순서를보존한다.
}
// signed인자도하위DWORD그대로유지한다.
std::uint32_t CreateNumber(std::string_view text) { return static_cast<std::uint32_t>(std::stoll(std::string(text))); }
// 독립원문16진입력을계산없이바이트로읽는다.
std::vector<std::uint8_t> CreateBytes(std::string_view hex) {
    if (hex.size()%2) throw std::runtime_error("NSInfo16진폭");std::vector<std::uint8_t> result;
    for (std::size_t i=0;i<hex.size();i+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(hex.substr(i,2)),nullptr,16))); // 한쌍씩읽는다.
    return result;
}
// 실제원시바이트를변경없이관측한다.
std::string CreateHex(std::span<const std::uint8_t> bytes) {
    constexpr char digits[]="0123456789abcdef"; // fixture의동일소문자표다.
    std::string result;for (auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; }return result; // 메모리순서다.
}
// 좁은원본문자열을바이트16진으로관측한다.
std::string CreateTextHex(std::string_view text) { return CreateHex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
// 실제DWORD십진인자를기록한다.
std::string CreateValue(std::uint32_t value) { return std::to_string(value); }

// 한원문할당의실제raw와안정된형별주소를같이소유한다.
struct CreateRecord {
    std::uint32_t size{}; // 원문new요청폭이다.
    CreateKind kind{CreateKind::Base}; // 현재실제부모의공유기본필드선택이다.
    std::vector<std::uint8_t> raw; // 미작성/가드바이트를포함한0x180전체다.
    GumpCreateNode base;HudNsInfoNode ns;HudPlayerInfoNode player;HudBattleInfoNode battle;HudPlayerButtonNode button; // 실제생성기/가상분배가빌리는같은안정된주소다.
    std::uint32_t textFlags{},buttonFlags{}; // 하위기존장치의명시필드다.
};
// 앞10열의자료만으로하위장치와현재자료를공급한다.
struct CreateScene {
    std::array<std::uint32_t,13> globals{}; // 같은현재전역의소유값이다.
    std::map<std::uint32_t,CreateRecord> records; // 원문할당순서가주소순서와같은저장소다.
    std::map<std::uint32_t,std::vector<std::uint8_t>> strings; // 번역장치가반환한512BYTE전체다.
    std::vector<std::uint8_t> server; // 실제정적서버버퍼256BYTE다.
    std::string formatted,formatInput,serverInput,trace; // 명시형식화출력/독립입력/효과순서다.
    std::uint32_t fill{},typeResult{};std::size_t allocationCalls{}; // 원문new의명시실패선택이다.
    std::map<std::string,std::size_t> calls;std::set<std::string> stops;std::set<std::size_t> failures; // 경계별호출번호와부분중단이다.
    std::vector<std::array<std::string,3>> effects; // 같은주소에적용할입력변경이다.
    std::unique_ptr<HudNsInfoCreate> body; // 이장면의같은전역/파생주소를빌린다.

    // 원시비정렬DWORD를읽는다.
    static std::uint32_t Load(std::span<const std::uint8_t> bytes,std::size_t offset) { if (offset+4>bytes.size()) throw std::out_of_range("NSInfo장치읽기");return bytes[offset]|(std::uint32_t(bytes[offset+1])<<8)|(std::uint32_t(bytes[offset+2])<<16)|(std::uint32_t(bytes[offset+3])<<24); }
    // 같은물리장치에DWORD를쓴다.
    static void Store(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) { if (offset+4>bytes.size()) throw std::out_of_range("NSInfo장치쓰기");for (unsigned i=0;i<4;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i)); } // 원문littleendian이다.
    // 같은파생의기본주소를조회한다.
    static GumpCreateNode& Base(CreateRecord& record) {
        switch (record.kind) { case CreateKind::Ns:return record.ns;case CreateKind::Player:return record.player;case CreateKind::Battle:return record.battle;case CreateKind::Button:return record.button;default:return record.base; }
    }
    // 독립raw의현재필드만실제형별주소에반영한다.
    static void Import(CreateRecord& r) {
        auto& b=Base(r);b.vtable=Load(r.raw,0);b.flags=Load(r.raw,4);b.command=Load(r.raw,0x20);r.textFlags=Load(r.raw,0xd0);r.buttonFlags=Load(r.raw,0xac);
        if (r.kind==CreateKind::Ns) { r.ns.background=Load(r.raw,0xa0);for (std::size_t i=0;i<8;++i) r.ns.players[i]=Load(r.raw,0xa4+i*4);r.ns.roster=Load(r.raw,0xc4);r.ns.title=Load(r.raw,0xc8);r.ns.upperDivider=Load(r.raw,0xcc);r.ns.lowerDivider=Load(r.raw,0xd0);r.ns.battle=Load(r.raw,0xd4); } // 고정배열순서다.
        if (r.kind==CreateKind::Player) { r.player.text=Load(r.raw,0xa0);r.player.master=Load(r.raw,0xa4);r.player.button=Load(r.raw,0xa8);r.player.start=Load(r.raw,0xac);r.player.player=Load(r.raw,0xb0);r.player.buttonType=Load(r.raw,0xb4);r.player.reserved=Load(r.raw,0xb8); }
        if (r.kind==CreateKind::Battle) { r.battle.title=Load(r.raw,0xa0);r.battle.summary=Load(r.raw,0xa4);r.battle.body=Load(r.raw,0xa8);r.battle.battle=Load(r.raw,0xac);r.battle.changed=Load(r.raw,0xb0);for (std::size_t i=0;i<22;++i) { r.battle.settings[i][0]=Load(r.raw,0xb4+i*8);r.battle.settings[i][1]=Load(r.raw,0xb8+i*8); }std::copy_n(r.raw.begin()+0x164,4,r.battle.bytes.begin());r.battle.mode=Load(r.raw,0x168); } // 22대응쌍을그대로읽는다.
        if (r.kind==CreateKind::Button) { r.button.buttonFlags=Load(r.raw,0xac);r.button.untouchedC0=Load(r.raw,0xc0);r.button.entry=Load(r.raw,0xc4); }
    }
    // 제품이쓴필드만raw에반영하고다른바이트는보존한다.
    static void Export(CreateRecord& r) {
        const auto& b=Base(r);Store(r.raw,0,b.vtable);Store(r.raw,4,b.flags);Store(r.raw,0x20,b.command);
        if (r.kind==CreateKind::Base) { Store(r.raw,0xd0,r.textFlags);Store(r.raw,0xac,r.buttonFlags); }
        if (r.kind==CreateKind::Ns) { Store(r.raw,0xa0,r.ns.background);for (std::size_t i=0;i<8;++i) Store(r.raw,0xa4+i*4,r.ns.players[i]);Store(r.raw,0xc4,r.ns.roster);Store(r.raw,0xc8,r.ns.title);Store(r.raw,0xcc,r.ns.upperDivider);Store(r.raw,0xd0,r.ns.lowerDivider);Store(r.raw,0xd4,r.ns.battle); } // 실제자식주소순서다.
        if (r.kind==CreateKind::Player) { Store(r.raw,0xa0,r.player.text);Store(r.raw,0xa4,r.player.master);Store(r.raw,0xa8,r.player.button);Store(r.raw,0xac,r.player.start);Store(r.raw,0xb0,r.player.player);Store(r.raw,0xb4,r.player.buttonType);Store(r.raw,0xb8,r.player.reserved); }
        if (r.kind==CreateKind::Battle) { Store(r.raw,0xa0,r.battle.title);Store(r.raw,0xa4,r.battle.summary);Store(r.raw,0xa8,r.battle.body);Store(r.raw,0xac,r.battle.battle);Store(r.raw,0xb0,r.battle.changed);for (std::size_t i=0;i<22;++i) { Store(r.raw,0xb4+i*8,r.battle.settings[i][0]);Store(r.raw,0xb8+i*8,r.battle.settings[i][1]); }std::copy(r.battle.bytes.begin(),r.battle.bytes.end(),r.raw.begin()+0x164);Store(r.raw,0x168,r.battle.mode); } // 미작성뒤3BYTE도같은값으로보존한다.
        if (r.kind==CreateKind::Button) { Store(r.raw,0xac,r.button.buttonFlags);Store(r.raw,0xc0,r.button.untouchedC0);Store(r.raw,0xc4,r.button.entry); }
    }
    // 경계전에제품의같은필드를모든현재raw에반영한다.
    void ExportAll() { for (auto& [token,r]:records) { static_cast<void>(token);Export(r); } } // 노드이동/재할당은하지않는다.
    // 명시변경뒤같은주소의파생필드를갱신한다.
    void ImportAll() { for (auto& [token,r]:records) { static_cast<void>(token);Import(r); } } // 같은참조를유지한다.
    // 실제전역참조를원문주소로조회한다.
    std::uint32_t& Global(std::uint32_t address) { for (std::size_t i=0;i<kCreateGlobals.size();++i) if (kCreateGlobals[i]==address) return globals[i];throw std::out_of_range("NSInfo전역주소"); } // 현재원문표순서다.
    // 새원문할당블록을초기물리패턴으로추가한다.
    std::uint32_t Add(std::uint32_t size) { const auto token=kCreateNodes+static_cast<std::uint32_t>(records.size())*0x200;auto& r=records[token];r.size=size;r.raw.assign(0x180,static_cast<std::uint8_t>(fill));Import(r);return token; }
    // 원문고정주소를선택한같은파생종류로조회한다.
    CreateRecord& Kind(std::uint32_t token,CreateKind kind) { auto& r=records.at(token);if (r.kind!=kind) { Export(r);r.kind=kind;Import(r); }return r; }
    // 앞10열만장면에설치한다.
    explicit CreateScene(const std::vector<std::string>& row):fill(CreateNumber(row[3])),typeResult(CreateNumber(row[4])) {
        const auto values=CreateSplit(row[2],':');if (values.size()!=globals.size()) throw std::runtime_error("NSInfo전역폭");for (std::size_t i=0;i<globals.size();++i) globals[i]=CreateNumber(values[i]); // 원문입력값이다.
        const auto fmt=CreateBytes(row[5]),remote=CreateBytes(row[6]);formatInput.assign(fmt.begin(),fmt.end());serverInput.assign(remote.begin(),remote.end());server.assign(0x100,static_cast<std::uint8_t>(fill));
        if (row[7]!="-") for (const auto& item:CreateSplit(row[7],';')) { const auto v=CreateSplit(item,'/');if (v.size()!=3) throw std::runtime_error("NSInfo변경폭");effects.push_back({v[0],v[1],v[2]}); } // 독립현재변경이다.
        if (row[8]!="-") for (const auto& item:CreateSplit(row[8],';')) stops.insert(item); // 실제도달한경계에서만멈춘다.
        if (row[9]!="-") for (const auto& item:CreateSplit(row[9],':')) failures.insert(CreateNumber(item)); // 원문new실패호출번호다.
        if (row[1]!="hidden") Add(row[1]=="ns" ? 0xd8U : row[1]=="player" ? 0xbcU : row[1]=="battle" ? 0x16cU : 0xc8U);
    }
    // 현재번역버퍼/원문문자열/형식화결과를NUL앞만해석한다.
    std::string String(std::uint32_t token) const {
        if (token==0x50c894) return "~w~E%s\nPlayers in Zone %d\n";if (token==0x50cd3c) return "~r~EBATTLEMASTER";if (token==0x50cd2c) return "Start Battle";if (token==0x50d068) return "LAN Server";if (token==kCreateFormat) return formatted;
        const auto& bytes=token==kCreateServer ? server : strings.at(token);const auto end=std::find(bytes.begin(),bytes.end(),0);if (end==bytes.end()) throw std::runtime_error("NSInfo문자열NUL");return {bytes.begin(),end};
    }
    // 명시주소의동일물리메모리만변경한다.
    void Effect(std::uint32_t address,std::uint32_t value) {
        if (address>=kCreateNodes && address<kCreateNodes+0x10000) { const auto token=kCreateNodes+((address-kCreateNodes)/0x200)*0x200;Store(records.at(token).raw,address-token,value);return; }
        if (address>=kCreateStrings && address<kCreateStrings+0x10000) { const auto token=kCreateStrings+((address-kCreateStrings)/0x200)*0x200;Store(strings.at(token),address-token,value);return; }
        Global(address)=address==kCreateGlobals[0] || address==kCreateGlobals[1] || address==kCreateGlobals[2] ? value&255U : value;
    }
    // 기록뒤입력순서대로변경하고중단한다.
    void Event(std::string name,std::vector<std::string> values={}) {
        ExportAll();if (!trace.empty()) trace+=';';trace+=name;for (const auto& value:values) trace+=':'+value; // 실제인자순서다.
        const auto trigger=name+std::to_string(++calls[name]);for (const auto& e:effects) if (e[0]==trigger) Effect(CreateNumber(e[1]),CreateNumber(e[2])); // 같은호출의여러변경이다.
        ImportAll();if (stops.contains(trigger)) throw CreateStop{};
    }
    // 하위장치의기본필드초기화를명시한다.
    void Device(std::uint32_t token,std::uint32_t flags=1) { ExportAll();auto& raw=records.at(token).raw;Store(raw,0,kCreateTable);Store(raw,4,flags);Store(raw,0x20,0);ImportAll(); }
    // 실제번역토큰과전체512BYTE장치를만든다.
    std::uint32_t Translate(std::uint32_t source) {
        const auto key=String(source),value=key=="~w~E%s\nPlayers in Zone %d\n" ? formatInput : "T:"+key;
        const auto token=kCreateStrings+static_cast<std::uint32_t>(strings.size())*0x200;auto& bytes=strings[token];bytes.assign(0x200,static_cast<std::uint8_t>(fill));std::copy(value.begin(),value.end(),bytes.begin());bytes[value.size()]=0;
        Event("translate",{CreateTextHex(key),CreateValue(token)});return token;
    }
    // 제품본문에같은공통/텍스트/자산장치만연결한다.
    HudNsInfoCreateHooks Hooks() {
        HudNsInfoCreateHooks h;
        h.node=[this](auto token) { return &Kind(token,CreateKind::Ns).ns; };h.player=[this](auto token) { return &Kind(token,CreateKind::Player).player; };h.battle=[this](auto token) { return &Kind(token,CreateKind::Battle).battle; };h.button=[this](auto token) { return &Kind(token,CreateKind::Button).button; };
        h.base=[this](auto token) { return &Base(records.at(token)); };h.textFlags=[this](auto token)->std::uint32_t& { return records.at(token).textFlags; };h.buttonFlags=[this](auto token)->std::uint32_t& { auto& r=records.at(token);return r.kind==CreateKind::Button ? r.button.buttonFlags : r.buttonFlags; };
        h.read=[this](auto address) { return Global(address); };
        h.allocate=[this](auto size) { const auto token=failures.contains(++allocationCalls) ? 0U : Add(static_cast<std::uint32_t>(size));Event("alloc",{std::to_string(size),CreateValue(token)});return token; };
        h.baseConstruct=[this](auto token,auto command,auto width,auto height,auto flags) { Device(token,flags);Event("base",{CreateValue(token),CreateValue(command),CreateValue(std::bit_cast<std::uint32_t>(width)),CreateValue(std::bit_cast<std::uint32_t>(height)),CreateValue(flags)}); };
        h.background=[this](auto token,auto width,auto height,auto bg,auto flags,auto type,auto frame) { Device(token);Event("background",{CreateValue(token),CreateValue(std::bit_cast<std::uint32_t>(width)),CreateValue(std::bit_cast<std::uint32_t>(height)),CreateValue(bg),CreateValue(flags),CreateValue(type),CreateValue(frame)});return token; };
        h.scroll=[this](auto token,auto a,auto b,auto c,auto d) { Device(token);records.at(token).textFlags=0x100;Event("scroll",{CreateValue(token),CreateValue(std::uint32_t(a)),CreateValue(std::uint32_t(b)),CreateValue(c),CreateValue(d)});return token; };
        h.text=[this](auto token,auto a,auto b,auto c,auto d) { Device(token);Event("text",{CreateValue(token),CreateValue(std::uint32_t(a)),CreateValue(std::uint32_t(b)),CreateValue(c),CreateValue(d)});return token; };
        h.buttonBase=[this](auto token,auto a,auto b,auto flags) { Device(token);auto& r=records.at(token);r.button.buttonFlags=0x100;Event("buttonBase",{CreateValue(token),CreateValue(std::uint32_t(a)),CreateValue(std::uint32_t(b)),CreateValue(flags)}); };
        h.stone=[this](auto token,auto a,auto b,auto label,auto color,auto context) { Device(token);records.at(token).buttonFlags=0x100;Event("stone",{CreateValue(token),CreateValue(std::uint32_t(a)),CreateValue(std::uint32_t(b)),CreateValue(label),CreateValue(color),CreateValue(context)});return token; };
        h.scrollable=[this](auto token,auto value) { auto& flags=records.at(token).textFlags;flags=(flags&~4U)|((value&1U)*4U);Event("scrollable",{CreateValue(token),CreateValue(value)}); };
        h.slider=[this](auto token,auto value) { auto& flags=records.at(token).textFlags;flags=(flags&~2U)|((value&1U)*2U);Event("slider",{CreateValue(token),CreateValue(value)}); };
        h.wrap=[this](auto token,auto value) { Event("wrap",{CreateValue(token),CreateValue(value)}); };
        h.parseScroll=[this](auto token,auto text) { Event("parseScroll",{CreateValue(token),CreateTextHex(String(text))}); };h.parseText=[this](auto token,auto text) { Event("parseText",{CreateValue(token),CreateTextHex(String(text))}); };
        h.fitText=[this](auto token,auto a,auto b) { Event("fitText",{CreateValue(token),CreateValue(a),CreateValue(b)}); };h.fitStone=[this](auto token) { Event("fitStone",{CreateValue(token)}); };
        h.attach=[this](auto token,auto x,auto y,auto parent,auto mode) { const auto kind=records.at(token).kind;if (kind==CreateKind::Player) body->AttachPlayer(token,x,y,parent,mode);else if (kind==CreateKind::Battle) body->AttachBattle(token,x,y,parent,mode);else Event(kind==CreateKind::Button ? "baseAttach" : "attach",{CreateValue(token),CreateValue(std::bit_cast<std::uint32_t>(x)),CreateValue(std::bit_cast<std::uint32_t>(y)),CreateValue(parent),CreateValue(mode)}); }; // 특수 버튼의 실제 가상+10은00466750 공통Attach다.
        h.baseAttach=[this](auto token,auto x,auto y,auto parent,auto mode) { Event("baseAttach",{CreateValue(token),CreateValue(std::bit_cast<std::uint32_t>(x)),CreateValue(std::bit_cast<std::uint32_t>(y)),CreateValue(parent),CreateValue(mode)}); };
        h.translate=[this](auto source) { return Translate(source); };h.type=[this](auto source) { Event("type",{CreateValue(source),CreateValue(typeResult)});return typeResult; };
        h.serverName=[this] { if (Global(0x54db4c)==0xffffffffU) { const auto text=String(Translate(0x50d068));std::copy(text.begin(),text.end(),server.begin());server[text.size()]=0; }else { std::copy(serverInput.begin(),serverInput.end(),server.begin());server[serverInput.size()]=0;Event("remote",{CreateValue(Global(0x54db4c)),"0","0",CreateValue(kCreateServer),"80","0"}); }return kCreateServer; };
        h.formatZone=[this](auto format,auto name,auto zone) { const auto fmt=String(format),serverName=String(name);std::array<char,512> bytes{};const auto size=std::snprintf(bytes.data(),bytes.size(),fmt.c_str(),serverName.c_str(),zone);if (size<0 || std::size_t(size)>=bytes.size()) throw std::runtime_error("NSInfo명시형식화폭");formatted.assign(bytes.data(),std::size_t(size));Event("format",{CreateTextHex(fmt),CreateTextHex(serverName),CreateValue(std::bit_cast<std::uint32_t>(zone))});return kCreateFormat; };
        h.assertion=[this](auto line) { Event("assert",{CreateValue(line)}); };h.metamapLayout=[this](auto kind,auto token) { Event("meta",{kind==HudNsInfoChild::Player ? "player" : "battle",CreateValue(token)}); };
        return h;
    }
    // 전체현재노드/가드바이트를원문할당순서로관측한다.
    std::string Nodes() { ExportAll();std::string output;for (const auto& [token,r]:records) { if (!output.empty()) output+=';';output+=CreateValue(token)+':'+CreateValue(r.size)+':'+CreateHex(r.raw); }return output.empty() ? "-" : output; } // 50개이하의현재블록이다.
    // 번역/서버전체바이트와형식화내용을모두관측한다.
    std::string Strings() const { std::string output;for (const auto& [token,bytes]:strings) { if (!output.empty()) output+=';';output+=CreateValue(token)+':'+CreateHex(bytes); }return output+'|'+CreateHex(server)+'|'+CreateTextHex(formatted); } // 토큰순서가실제번역순서다.
    // 현재전역만원문순서로관측한다.
    std::string Globals() const { std::string output;for (auto value:globals) { if (!output.empty()) output+=':';output+=CreateValue(value); }return output; } // 원시13DWORD순서다.
};
}

// 실제부모함수의전체관측을앞10열만설치한같은제품본체에대조한다.
TEST_CASE(HudNsInfoCreate_Native170ParentsChildrenAndPartialEffects) {
    std::ifstream input(NETSTORM_HUD_NSINFO_CREATE_FIXTURE);CHECK(input.good());if (!input) return;std::string line;std::size_t rows{},partial{},full{};
    while (std::getline(input,line)) { // 정상EAX만실제의미반환값으로비교한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.starts_with('#')) continue;const auto row=CreateSplit(line,'\t');CHECK(row.size()==kCreateColumns);if (row.size()!=kCreateColumns) continue;++rows;
        CreateScene s(row);s.body=std::make_unique<HudNsInfoCreate>(s.Global(0x593150),s.Hooks());bool completed=true;std::uint32_t result{};
        try { if (row[1]=="hidden") result=s.body->CreateHidden();else if (row[1]=="ns") result=s.body->Construct(kCreateNodes);else if (row[1]=="player") result=s.body->ConstructPlayer(kCreateNodes);else if (row[1]=="battle") result=s.body->ConstructBattle(kCreateNodes);else result=s.body->ConstructButton(kCreateNodes,16,15); } catch (const CreateStop&) { completed=false; }
        if (!completed) ++partial;if (completed && (row[1]=="hidden" || row[1]=="ns") && result) { ++full;CHECK(s.records.size()==50); }
        CHECK(completed==(row[10]=="1"));if (completed) CHECK(result==CreateNumber(row[11]));CHECK(s.Globals()==row[12]);CHECK(s.Nodes()==row[13]);CHECK(s.Strings()==row[14]);
        const auto actualTrace=s.trace.empty() ? std::string("-") : s.trace; // 첫 차이부터 표시하여 원문 순서 오류를 입력 이름으로 추적한다.
        if (actualTrace!=row[15]) { const auto mismatch=std::mismatch(actualTrace.begin(),actualTrace.end(),row[15].begin(),row[15].end());const auto offset=std::size_t(mismatch.first-actualTrace.begin());std::fprintf(stderr,"NSInfo native %s/%s offset=%zu actual=%s expected=%s\n",row[0].c_str(),row[1].c_str(),offset,actualTrace.substr(offset,160).c_str(),row[15].substr(offset,160).c_str()); }
        CHECK(actualTrace==row[15]);CHECK(CreateNumber(row[17])>0);
        if (completed) CHECK(std::stoll(row[16])==(row[1]=="button" ? 12 : 4));
    }
    CHECK(rows==kCreateRows);CHECK(partial==50);CHECK(full>0);
}

// 전체메타맵후속이없으면일반Attach뒤관문통과시실패하고일반미션관문은반환한다.
TEST_CASE(HudNsInfoCreate_RequiredActualMetamapContinuation) {
    std::uint32_t current{},meta{},known{},attached{};HudNsInfoCreateHooks hooks;
    hooks.read=[&](auto address) { if (address==0x594fc8) return meta;if (address==0x595344) return known;throw std::out_of_range("NSInfo관문주소"); }; // 실제현재두전역만읽는다.
    hooks.baseAttach=[&](auto token,auto x,auto y,auto parent,auto mode) { CHECK(token==7 && x==1 && y==2 && parent==8 && mode==9);++attached; };
    HudNsInfoCreate body(current,hooks);body.AttachPlayer(7,1,2,8,9);meta=1;body.AttachBattle(7,1,2,8,9);known=1;
    bool missing{};try { body.AttachPlayer(7,1,2,8,9); } catch (const std::logic_error&) { missing=true; }CHECK(missing && attached==3);
}
