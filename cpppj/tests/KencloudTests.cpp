// 제품전고정한v2 원문246관측의같은난수/전체할당/인접전역/파일scratch/trace를대조한다.
#include "TestSupport.h"
#include "o/RawKencloud.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace netstorm::o;
constexpr std::uint32_t kState=0x5697f0,kStateBytes=0x2018,kBytes=0x40000; //인접전역과세512×512BYTE전체물리를비교한다.
constexpr std::uint32_t kCache=0x569ffc,kEnd=0x569ff0,kTop=0x569ff8,kBottom=0x569ff4,kPhase=0x56b804,kAll=0x56b7fc,kPart=0x56b800,kDirty=0x56a000,kSeeds=0x56a800; //격리원문과같은실제전역주소다.
constexpr std::array<std::uint32_t,3> kBuffers{0x12001000,0x12051000,0x120a1000}; //명시파일/할당장치의세실제주소토큰이다.
constexpr std::array<std::uint32_t,16> kRows{0,8,4,12,1,11,5,15,3,10,2,7,14,9,6,13}; //보호PE에저장된최초행순서다.
static_assert(sizeof(KencloudFileScratch)==264&&offsetof(KencloudFileScratch,height)==0&&offsetof(KencloudFileScratch,width)==4&&offsetof(KencloudFileScratch,path)==8); //원문파일부모의실제물리배치다.
// 끝빈trace칸도유지하는독립fixture파서다.
std::vector<std::string> Fields(std::string_view text,char delimiter) {
    std::vector<std::string> result;std::size_t position=0; //다음분리위치다.
    // 원시값을정규화하거나중복행을제거하지않는다.
    for (;;) { const auto end=text.find(delimiter,position);result.emplace_back(text.substr(position,end==text.npos?text.npos:end-position));if (end==text.npos) return result;position=end+1; }
}
// 숫자목록을원문과같은십진표현으로연결한다.
std::string Numbers(std::span<const std::uint32_t> values) { std::string result;for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result; }
// 현재CString만trace에표시하는소문자hex다.
std::string Hex(std::span<const std::uint8_t> bytes) { constexpr std::string_view digits="0123456789abcdef";std::string result;for (const auto value:bytes) { result+=digits[value>>4];result+=digits[value&15]; }return result; }
// 전체메모리를Pythonhashlib의최초SHA256과비교한다.큰버퍼를fixture에반복삽입하지않는다.
std::string Hash(std::span<const std::uint8_t> input) {
    constexpr std::array<std::uint32_t,64> constants{ //SHA256의고정64개라운드상수다.
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    std::array<std::uint32_t,8> state{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19}; //표준최초해시상태다.
    std::vector<std::uint8_t> bytes(input.begin(),input.end());const auto bits=static_cast<std::uint64_t>(input.size())*8U; //입력전체를비트길이와함께패딩한다.
    bytes.push_back(0x80);while (bytes.size()%64!=56) bytes.push_back(0); //마지막8BYTE는길이용이다.
    for (int shift=56;shift>=0;shift-=8) bytes.push_back(static_cast<std::uint8_t>(bits>>shift)); //길이는big-endian이다.
    // 모든64BYTE블록을고정순서로압축한다.
    for (std::size_t block=0;block<bytes.size();block+=64) {
        std::array<std::uint32_t,64> words{}; //현재블록의확장메시지다.
        for (std::size_t i=0;i<16;++i) for (std::size_t j=0;j<4;++j) words[i]=(words[i]<<8)|bytes[block+i*4+j]; //원시입력의big-endianDWORD다.
        for (std::size_t i=16;i<64;++i) { const auto a=words[i-15],b=words[i-2];words[i]=words[i-16]+(std::rotr(a,7)^std::rotr(a,18)^(a>>3))+words[i-7]+(std::rotr(b,17)^std::rotr(b,19)^(b>>10)); } //메시지확장이다.
        auto a=state[0],b=state[1],c=state[2],d=state[3],e=state[4],f=state[5],g=state[6],h=state[7]; //현재8개라운드레지스터다.
        for (std::size_t i=0;i<64;++i) { const auto first=h+(std::rotr(e,6)^std::rotr(e,11)^std::rotr(e,25))+((e&f)^(~e&g))+constants[i]+words[i];const auto second=(std::rotr(a,2)^std::rotr(a,13)^std::rotr(a,22))+((a&b)^(a&c)^(b&c));h=g;g=f;f=e;e=d+first;d=c;c=b;b=a;a=first+second; } //모든합은low32다.
        const std::array<std::uint32_t,8> result{a,b,c,d,e,f,g,h};for (std::size_t i=0;i<8;++i) state[i]+=result[i]; //누적상태에합친다.
    }
    std::array<std::uint8_t,32> digest{};for (std::size_t i=0;i<8;++i) for (std::size_t j=0;j<4;++j) digest[i*4+j]=static_cast<std::uint8_t>(state[i]>>(24-j*8)); //최종big-endianBYTE다.
    return Hex(digest);
}
// 현재입력물리의객체표현을모두읽는다.타깃Windows/x86관측은little-endian이다.
template<class T> std::span<const std::uint8_t> ObjectBytes(const T& value) { return {reinterpret_cast<const std::uint8_t*>(&value),sizeof value}; }
// 명시외부장치중단의원인을그대로보존한다.
struct Stop { std::string cause; }; //정상return과별개결과다.
// 제품의부모계산은포함하지않는명시파일/할당/관측장치다.
struct Device {
    std::array<std::uint32_t,kStateBytes/4> state{}; //실제인접전역의고정물리이며모든참조는이배열을빌린다.
    std::array<std::vector<std::uint8_t>,3> buffers; //주소가다른실제세할당을독립소유한다.
    std::array<std::uint8_t,1024> pathSource{}; //원문PathSpec이돌려주는NUL물리다.
    std::array<std::uint32_t,16> rows=kRows; //현재phase행표다.
    std::uint32_t color{},speed{},fury{},count{},spacer{29},image{},exists{},allocate{},fileReturn{}; //관측필드와명시파일장치입력이다.
    std::int32_t width{},height{}; //디코더의명시반환치수다.
    GameRandom random; //독립fixture의현재상태입력이며제품Build는이동일객체를참조한다.
    std::string kind,mutation,fileSnapshot{"-"}; //선택진입점/하위효과/마지막파일stack관측이다.
    std::unique_ptr<KencloudFileScratch> scratch; //현재파일부모의명시264BYTE입력/부분출력수명이다.
    std::map<std::string,std::uint32_t> calls;std::vector<std::string> events; //실제하위횟수와현재순서를보존한다.
    // 실제전역주소의같은DWORD를반환한다.
    std::uint32_t& Word(std::uint32_t address) { return state.at((address-kState)/4); }
    // 테스트입력의0/11/22/33을native주소로바꾼다.
    static std::uint32_t Token(std::uint32_t logical) { if (logical==0) return 0;return kBuffers.at(logical/11-1); }
    // signed표현입력도원문low32로읽는다.
    static std::uint32_t Number(const std::string& text) { return static_cast<std::uint32_t>(std::stoll(text)); }
    // 이름의현재NUL까지읽어명시파일장치에전달한다.
    static std::span<const std::uint8_t> CString(std::span<const std::uint8_t> bytes) { const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("검사CString물리");return bytes.first(static_cast<std::size_t>(end-bytes.begin())); }
    // 명시하위가C문자열을바꾸되나머지물리는보존한다.
    static void Put(std::span<std::uint8_t> target,std::string_view text) { if (text.size()+1>target.size()) throw std::out_of_range("검사이름쓰기");std::copy(text.begin(),text.end(),target.begin());target[text.size()]=0; }
    // 모든입력을새현재상태/물리로초기화한다.
    explicit Device(const std::vector<std::string>& input):random(0) {
        if (input.size()!=16) throw std::runtime_error("Kencloud입력칸수");
        kind=input[0];image=Token(Number(input[2]));width=std::bit_cast<std::int32_t>(Number(input[3]));height=std::bit_cast<std::int32_t>(Number(input[4]));exists=Number(input[5]);allocate=Number(input[6]);color=Number(input[7]);speed=Number(input[8]);fury=Number(input[9]);count=Number(input[10]);mutation=input[14];random.SetState(Number(input[12]));
        state.fill(0xcdcdcdcdU);const auto cache=Token(Number(input[1])),end=Number(input[13]); //현재초기캐시와명시end거리다.
        Word(kCache)=cache;Word(kEnd)=cache+end;Word(kTop)=cache+512;Word(kBottom)=cache+end-512;Word(kPhase)=Number(input[11]);Word(kAll)=17;Word(kPart)=19;
        // 이전dirty표/현재dirty표/seed표의전체미사용DWORD도입력으로보존한다.
        for (std::uint32_t i=0;i<512;++i) { Word(kState+i*4)=0x80000000U+i;Word(kDirty+i*4)=0x90000000U+i; }
        for (std::uint32_t i=0;i<1000;++i) Word(kSeeds+i*4)=0x70000000U+i; //원문물리seed표와같은크기다.
        for (std::size_t b=0;b<buffers.size();++b) { buffers[b].resize(kBytes);for (std::size_t byte=0;byte<buffers[b].size();++byte) buffers[b][byte]=static_cast<std::uint8_t>(byte*13+b*31); } //서로다른전체할당입력이다.
        pathSource.fill(0xde);
        if (input[15]=="2") { std::fill_n(pathSource.begin(),256,std::uint8_t{'Q'});pathSource[256]=0; }
        else Put(pathSource,input[15]=="0"?"Cloud/File.gif":"");
    }
    // 도달한실제할당주소부터남은물리를빌린다.one-past는길이0이동을위해유효하다.
    std::span<std::uint8_t> Memory(std::uint32_t address) {
        for (std::size_t index=0;index<kBuffers.size();++index) if (address>=kBuffers[index]&&address-kBuffers[index]<=kBytes) return std::span(buffers[index]).subspan(address-kBuffers[index]);
        throw std::out_of_range("검사미등록물리주소");
    }
    // 같은현재전역/표의참조만제공한다.필드사본의후동기화는없다.
    KencloudGlobals Globals() { return {Word(kCache),Word(kEnd),Word(kTop),Word(kBottom),Word(kPhase),Word(kAll),Word(kPart),spacer,color,speed,fury,count,rows,std::span(state).subspan((kDirty-kState)/4,512),std::span(state).subspan((kSeeds-kState)/4,1000)}; }
    // callback복귀직전명시변화만적용한다.난수도같은실제객체를직접변경한다.
    void Mutate(const std::string& boundary) {
        if (mutation==boundary+"-stop") throw Stop{boundary};
        if (mutation==boundary+"-cache33") Word(kCache)=Token(33);
        else if (mutation==boundary+"-cache22") Word(kCache)=Token(22);
        else if (mutation==boundary+"-cache0") Word(kCache)=0;
        else if (mutation==boundary+"-count0") count=0;
        else if (mutation==boundary+"-count4") count=4;
        else if (mutation==boundary+"-state17") random.SetState(17);
        else if (mutation==boundary+"-fury0") fury=0;
        else if (mutation==boundary+"-fury20000") fury=20000;
        else if (mutation==boundary+"-color17") color=17;
        else if (mutation==boundary+"-end17") Word(kEnd)=Word(kCache)+17;
        else if (mutation==boundary+"-source") Put(pathSource,"ChangedSource");
        else if (mutation==boundary+"-name"&&scratch) Put(scratch->path,"ChangedName");
    }
    // 명시파일장치만연결하며부모복사/치수검사/free결정은새제품이실행한다.
    std::uint32_t File() {
        events.push_back("FILE:"+std::to_string(++calls["file"]));
        std::array<std::uint8_t,256> path;path.fill(0xcd); //원문의초기stack입력이다.임의0초기화가아니다.
        scratch=std::make_unique<KencloudFileScratch>(KencloudFileScratch{std::bit_cast<std::int32_t>(0xcdcdcdcdU),std::bit_cast<std::int32_t>(0xcdcdcdcdU),path});
        KencloudFileHooks hooks; //미연결장치의기본성공을제공하지않는다.
        hooks.pathSpec=[this](std::string_view key,std::uint32_t a,std::uint32_t b,std::uint32_t c)->std::span<const std::uint8_t> { CHECK(key=="CloudFileSpec"&&a==0&&b==0&&c==0);events.push_back("PATH:CloudFileSpec:0,0,0");Mutate("path");return pathSource; };
        hooks.exists=[this](std::span<std::uint8_t> name) { events.push_back("EXISTS:"+Hex(CString(name)));Mutate("exists");return exists; };
        hooks.decode=[this](std::span<std::uint8_t> name,std::int32_t& outputWidth,std::int32_t& outputHeight) { events.push_back("DECODE:"+Hex(CString(name)));outputWidth=width;outputHeight=height;Mutate("decode");return image; };
        hooks.release=[this](std::uint32_t selected) { ++calls["free"];events.push_back("FREE:"+std::to_string(selected));Mutate("free"); };
        try { const auto result=LoadKencloudFile(*scratch,hooks);fileSnapshot=Hash(ObjectBytes(*scratch));return result; }
        catch (...) { fileSnapshot=Hash(ObjectBytes(*scratch));throw; }
    }
    // 할당/주소조회와실제memmove·난수뒤관측만공급한다.
    KencloudHooks Hooks() {
        KencloudHooks hooks; //빌더는기존GameRandom참조를별도인자로받는다.
        hooks.loadFile=[this] { return File(); };
        hooks.allocate=[this](std::uint32_t bytes) { CHECK(bytes==kBytes);const auto call=++calls["alloc"],result=allocate==0?0:kBuffers[std::min<std::size_t>(call-1,2)];events.push_back("ALLOC:"+std::to_string(result));Mutate("alloc");return result; };
        hooks.memory=[this](std::uint32_t address) { return Memory(address); };
        hooks.afterMove=[this](std::uint32_t target,std::uint32_t source,std::uint32_t bytes) { const auto index=++calls["move"];const std::array<std::uint32_t,4> fields{index,target,source,bytes};events.push_back("MOVE:"+Numbers(fields));Mutate("move"+std::to_string(index)); };
        hooks.afterRandom=[this](std::uint32_t limit,std::uint32_t before,std::uint32_t result) { const auto index=++calls["random"];const std::array<std::uint32_t,5> fields{index,limit,before,result,random.State()};events.push_back("RANDOM:"+Numbers(fields));Mutate("random"+std::to_string(index)); };
        return hooks;
    }
    // 제품의세선택진입점과중단을구별한다.
    std::string Run() {
        const auto globals=Globals();const auto hooks=Hooks(); //현재소유물리의참조는실행동안안정적이다.
        try {
            if (kind=="build") BuildKencloud(globals,random,hooks);
            else if (kind=="step") StepKencloud(globals,hooks);
            else if (kind=="file") fileReturn=File();
            else throw std::runtime_error("Kencloud진입점");
        } catch (const Stop& stop) { return stop.cause; }
        catch (const std::out_of_range& error) { const std::string message=error.what();if (!message.starts_with("Kencloud ")) throw;return message.substr(9); }
        catch (const std::domain_error& error) { CHECK(std::string_view(error.what())=="Kencloud divide-zero");return "divide-zero"; }
        return "return";
    }
    // 정상/부분중단뒤현재12개필드를fixture와동일순서로기록한다.
    std::string Values() { const std::array<std::uint32_t,12> fields{fileReturn,random.State(),color,speed,fury,count,spacer,calls["file"],calls["alloc"],calls["free"],calls["random"],calls["move"]};return Numbers(fields); }
    // 인접전역전체/세할당전체/현재파일stack을hashlib최초값과비교한다.
    std::string Raw() const { std::string result=Hash(ObjectBytes(state));for (const auto& buffer:buffers) result+=":"+Hash(buffer);return result+":"+fileSnapshot; }
    // 실제기록한호출순서만연결한다.
    std::string Trace() const { std::string result;for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; }return result; }
};
}

// 실제게임난수와전체버퍼의원문v2 관측246개를정상/부분중단까지전부대조한다.
TEST_CASE(Kencloud_OriginalV2X86Fixture) {
    CHECK(Hash({})=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");const std::array<std::uint8_t,3> abc{'a','b','c'};CHECK(Hash(abc)=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"); //독립표준해시표현을먼저확인한다.
    std::ifstream input(NETSTORM_KENCLOUD_FIXTURE);CHECK(input.good());if (!input) return; //누락자료를통과로처리하지않는다.
    std::string line;std::size_t compared=0,normal=0; //정상반환과명시/물리중단을별도계수한다.
    // 양정밀도도매번동일독립입력으로시작한다.
    while (std::getline(input,line)) {
        if (line.empty()||line[0]=='#') continue;
        if (line.back()=='\r') line.pop_back();
        const auto fields=Fields(line,'\t');CHECK(fields.size()==6);if (fields.size()!=6) return; //원문형식그대로다.
        Device device(Fields(fields[1],','));const auto cause=device.Run(); //제품본체가실행한결과다.
        const auto values=device.Values(),raw=device.Raw(),trace=device.Trace();const bool equal=cause==fields[2]&&values==fields[3]&&raw==fields[4]&&trace==fields[5]; //어느필드도기대에맞춰생략하지않는다.
        if (!equal) std::fprintf(stderr,"Kencloud row %zu %s\ncause=%s expected=%s\nvalues=%s\nraw=%s\ntrace=%s\n",compared,fields[1].c_str(),cause.c_str(),fields[2].c_str(),values.c_str(),raw.c_str(),trace.c_str());
        CHECK(equal);++compared;if (cause=="return") ++normal;
    }
    CHECK(compared==246);CHECK(normal==208);
}
// 자연생략은미연결장치를요구하지않고도달한필수경계는실제부분효과뒤예외를낸다.
TEST_CASE(Kencloud_RequiredDevicesAndSameRandomReference) {
    Device current(Fields("build,11,11,512,512,1,11,228,1,10000,3,0,305419896,262144,none,0",','));const auto globals=current.Globals();const auto before=current.random.State();KencloudHooks empty; //현재캐시비영입력은하위를자연생략한다.
    BuildKencloud(globals,current.random,empty);CHECK(current.random.State()==before);CHECK(current.Word(kCache)==kBuffers[0]);CHECK(current.Word(kPart)==0&&current.Word(kAll)==1);
    Device missing(Fields("build,0,11,512,512,1,11,228,1,10000,3,0,305419896,262144,none,0",','));const auto missingGlobals=missing.Globals();bool rejected=false; //같은GameRandom참조만제공한미연결파일경계다.
    try { BuildKencloud(missingGlobals,missing.random,empty); } catch (const std::bad_function_call&) { rejected=true; }
    CHECK(rejected);CHECK(missing.random.State()==305419896U&&missing.Word(kCache)==0&&missing.Word(kAll)==17&&missing.Word(kPart)==19);
    std::array<std::uint8_t,256> path;path.fill(0xcd);KencloudFileScratch scratch{17,19,path};const std::array<std::uint8_t,2> name{'A',0};KencloudFileHooks file; //caller현재stack입력은반드시명시한다.
    file.pathSpec=[&](std::string_view,std::uint32_t,std::uint32_t,std::uint32_t)->std::span<const std::uint8_t> { return name; };file.exists=[](std::span<std::uint8_t>) { return 0U; };
    CHECK(LoadKencloudFile(scratch,file)==0&&scratch.width==19&&scratch.height==17&&scratch.path[0]=='A'&&scratch.path[1]==0&&scratch.path[2]==0xcd);
}
