// 최초664본문관측과38개별도cleanup의현재전역/전체raw/검색순서를대조한다.
#include "TestSupport.h"
#include "o/RawFortTypeNamesLoad.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
using namespace netstorm::o;

namespace {
constexpr std::size_t kTypeNamesBufferBytes=1088; //최대255hash와pos경계를별도로관측하는물리범위다.
constexpr std::array<std::uint32_t,2> kTypeNamesBuffers{0x12001000,0x12002000}; //원문명시data주소를보존하는토큰이다.
constexpr std::array<std::uint32_t,4> kTypeNamesBlocks{0,0x12004000,0x12004200,0x12004400}; //0/기존/새할당/교체주소다.
const std::array<std::string,8> kTypeNamesNames{"Alpha","Beta","",std::string("\xff" "A\x80\x7f",4),"Alpha","LongNameWithMoreThanTwentyCharacters123","Case","case"}; //원시signedchar/NUL/긴이름/중복입력이다.
// 마지막빈trace칸도포함해fixture를분리한다.
std::vector<std::string> TypesFields(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t start{}; //현재분리위치와결과다.
    // 구분자뒤빈칸을버리지않는다.
    for (;;) { const auto end=text.find(separator,start);result.emplace_back(text.substr(start,end==text.npos ? text.npos : end-start));if (end==text.npos) return result;start=end+1; }
}
// 명시원문필드의DWORD만little endian으로읽는다.
std::uint32_t TypesRead(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t result{}; //정확한네BYTE의합이다.
    // 낮은BYTE부터조립한다.
    for (std::size_t byte=0;byte<4;++byte) result|=std::uint32_t(bytes[offset+byte])<<(byte*8);return result;
}
// 초기입력/callback의지정DWORD만쓴다.
void TypesPut(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    // 다른필드/패딩은유지한다.
    for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8));
}
// 전체물리BYTE를NUL뒤도보존해hex로관측한다.
std::string TypesHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view kDigits="0123456789abcdef";std::string result; //한BYTE의두글자표현이다.
    // 실제원문순서로관측한다.
    for (const auto byte:bytes) { result+=kDigits[byte>>4];result+=kDigits[byte&15U]; }return result;
}
// TYPE장치의명시NUL앞바이트만관측한다.
std::span<const std::uint8_t> TypesText(std::span<const std::uint8_t> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("TypeNames장치NUL범위");return bytes.first(static_cast<std::size_t>(end-bytes.begin()));
}
// 저장hash입력의네종류를고정한다.제품해시나예상변환표에서값을얻지않는다.
std::array<std::uint32_t,4> TypesHashes(std::uint32_t mode) {
    constexpr std::uint32_t kAlpha=0x68706ca2,kBeta=0x61746542,kSigned=0x7e8040ff; //각원문입력문자열에대응하는독립저장DWORD다.
    return mode==0 ? std::array{kAlpha,kBeta,0U,kSigned} : mode==1 ? std::array{kAlpha,kAlpha,kBeta,0U} : std::array{0xffffffffU,0x12345678U,0x80000000U,0x7fffffffU};
}
// 실제메모리에서지정BYTE만써서이름getter후변경을만든다.
void TypesInput(std::span<std::uint8_t> bytes,std::string_view name) {
    if (name.size()>=bytes.size()) throw std::out_of_range("TypeNames장치이름크기");std::copy(name.begin(),name.end(),bytes.begin());bytes[name.size()]=0;
}
// 명시하위중단을제품의미등록예외와구별한다.
struct TypesStop { std::string cause; }; //본문/cleanup을같은성공으로합치지않는다.
// 기대열을읽지않고원문과동일한입력/하위효과만구성한다.
struct TypesDevices {
    std::uint32_t slot{},selected{},depth{},conversion{},allocated{},currentType{},selects{},asserts{},allocations{},records{},buffersRead{},conversionAccesses{}; //현재전역/실제경계횟수다.
    std::int32_t count{}; //현재signedtypeCount이며TYPE callback이바꿀수있다.
    std::string mutation,trace,physical; //명시효과와실제순서/접근대상이다.
    std::array<std::uint8_t,180> headers{}; //고정this의9×20BYTE실제표다.
    std::array<std::array<std::uint8_t,16>,2> streams{}; //서로다른선택stream의실제pos/end/data다.
    std::array<std::array<std::uint8_t,kTypeNamesBufferBytes>,2> data{}; //count/hash의명시물리입력이다.
    std::array<std::array<std::uint8_t,255>,3> blocks{}; //기존/새할당/교체변환표를모두관측한다.
    std::array<std::array<std::uint8_t,128>,262> names{}; //count증가전에모든실제이름버퍼가존재한다.

    // 파일hash는입력으로만설치하고어느칸이갱신될지는본문이결정한다.
    explicit TypesDevices(std::string_view input) {
        const auto fields=TypesFields(input,',');if (fields.size()!=13) throw std::runtime_error("TypeNames입력칸수");slot=static_cast<std::uint32_t>(std::stoul(fields[0]));const auto manager=static_cast<std::uint32_t>(std::stoul(fields[1]));selected=static_cast<std::uint32_t>(std::stoul(fields[2]));const auto present=std::stoul(fields[3]);const auto end=static_cast<std::uint32_t>(std::stoul(fields[4]));const auto position=static_cast<std::uint32_t>(std::stoul(fields[5]));const auto stored=static_cast<std::uint32_t>(std::stoul(fields[6]));const auto mode=static_cast<std::uint32_t>(std::stoul(fields[7]));count=std::stoi(fields[8]);conversion=kTypeNamesBlocks.at(std::stoul(fields[9])/11);allocated=kTypeNamesBlocks.at(std::stoul(fields[10])/11);depth=static_cast<std::uint32_t>(std::stoul(fields[11]));mutation=fields[12];
        // 고정header의비대상바이트도원문패턴을그대로공급한다.
        for (std::size_t byte=0;byte<headers.size();++byte) headers[byte]=static_cast<std::uint8_t>(0x70+byte);TypesPut(headers,slot*20,manager);
        // 두stream의data주소와저장hash입력을구별한다.
        for (std::size_t index=0;index<streams.size();++index) {
            TypesPut(streams[index],0,present ? kTypeNamesBuffers[index] : 0);TypesPut(streams[index],4,position);TypesPut(streams[index],8,end);TypesPut(streams[index],12,kTypeNamesBufferBytes);
            for (std::size_t byte=0;byte<kTypeNamesBufferBytes;++byte) data[index][byte]=static_cast<std::uint8_t>(0xa0+byte+index*7); //NUL이아닌주변입력도보존한다.
            if (position<kTypeNamesBufferBytes) {
                data[index][position]=static_cast<std::uint8_t>(stored);
                for (std::uint32_t number=0;number<stored;++number) { const auto target=std::uint64_t(position)+1+number*4U;if (target+4<=kTypeNamesBufferBytes) TypesPut(data[index],static_cast<std::size_t>(target),TypesHashes(mode)[(number+index)%4]); } //물리안의입력만설치한다.
            }
        }
        // 새할당도원문이0으로만들기전에는독립비영입력을가진다.
        for (std::size_t index=0;index<blocks.size();++index) for (std::size_t byte=0;byte<255;++byte) blocks[index][byte]=static_cast<std::uint8_t>(0x60+(index+1)*11+byte);
        // 긴이름은20자에서자르지않고원본NUL위치를공급한다.
        for (std::size_t index=0;index<names.size();++index) { names[index].fill(0xa7);TypesInput(names[index],kTypeNamesNames[index%kTypeNamesNames.size()]); }
    }
    // 실제하위진입순서를연결한다.
    void Record(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 각경계에서지정한현재DWORD/버퍼만바꾸며본문의재조회를복제하지않는다.
    void Mutate(std::string_view boundary) {
        auto& stream=streams.at(selected); //명시callback진입시점에선택된실제주소다.
        if (mutation=="select-stream1" && boundary=="select") selected=1;
        else if (mutation=="select-manager0" && boundary=="select") TypesPut(headers,slot*20,0);
        else if (mutation==std::string(boundary)+"-depth7") depth=7;
        else if (mutation==std::string(boundary)+"-depthffffffff") depth=0xffffffffU;
        else if (mutation==std::string(boundary)+"-conversion0") conversion=0;
        else if (mutation==std::string(boundary)+"-conversion11") conversion=kTypeNamesBlocks[1];
        else if (mutation==std::string(boundary)+"-conversion22") conversion=kTypeNamesBlocks[2];
        else if (mutation==std::string(boundary)+"-data1") TypesPut(stream,0,kTypeNamesBuffers[1]);
        else if (mutation==std::string(boundary)+"-pos9") TypesPut(stream,4,9);
        else if (mutation==std::string(boundary)+"-endffffffff") TypesPut(stream,8,0xffffffffU);
        else if (mutation==std::string(boundary)+"-count0") count=0;
        else if (mutation==std::string(boundary)+"-count260") count=260;
        else if (mutation=="record1-name" && boundary=="record1") TypesInput(names[0],"Beta");
        else if (mutation=="record1-bufferhash" && boundary=="record1") TypesPut(data[0],1,TypesHashes(0)[1]);
        else if (mutation==std::string(boundary)+"-stop") throw TypesStop{std::string(boundary)};
    }
    // 두현재DWORD를제품과같은실체로빌린다.
    FortTypeNamesLoadGlobals Globals() { return {depth,conversion}; }
    // Select/할당/TYPE만경계로두고실제검색/zero/pos/end는제품이수행한다.
    FortTypeNamesLoadHooks Hooks() {
        FortTypeNamesLoadHooks hooks;
        hooks.manager=[this](std::int32_t fixed) { return TypesRead(headers,static_cast<std::size_t>(fixed)*20); };
        hooks.select=[this](std::uint32_t manager,std::string_view name,std::int32_t writable) { if (name!="TypeNames" || writable!=1) throw std::logic_error("TypeNames실제Select인수");++selects;Record("S:"+std::to_string(manager)+":TypeNames:1");Mutate("select");return selected; };
        hooks.stream=[this](std::uint32_t token)->std::span<std::uint8_t> { return streams.at(token); };
        hooks.buffer=[this](std::uint32_t token)->std::span<const std::uint8_t> { physical=++buffersRead==1 ? "count-read" : "hash-read";const auto found=std::find(kTypeNamesBuffers.begin(),kTypeNamesBuffers.end(),token);if (found==kTypeNamesBuffers.end()) throw std::out_of_range("TypeNames현재data주소");return data[static_cast<std::size_t>(found-kTypeNamesBuffers.begin())]; };
        hooks.allocate=[this](std::uint32_t size) { if (size!=255) throw std::logic_error("TypeNames실제255BYTE할당");++allocations;Record("N:255");Mutate("allocate");return allocated; };
        hooks.conversionBuffer=[this](std::uint32_t token)->std::span<std::uint8_t> { ++conversionAccesses;if (!token) throw TypesStop{conversionAccesses==1 ? "allocation0" : "conversion-write"};const auto found=std::find(kTypeNamesBlocks.begin()+1,kTypeNamesBlocks.end(),token);if (found==kTypeNamesBlocks.end()) throw std::out_of_range("TypeNames현재conversion주소");return blocks[static_cast<std::size_t>(found-kTypeNamesBlocks.begin()-1)]; };
        hooks.typeCount=[this] { Record("C:"+std::to_string(count));return count; };
        hooks.typeName=[this](std::int32_t index)->std::span<const std::uint8_t> { currentType=static_cast<std::uint32_t>(index);++records;Record("R:"+std::to_string(index)+':'+TypesHex(TypesText(names.at(currentType))));Mutate("record"+std::to_string(records));return names.at(currentType); };
        hooks.assertion=[this](std::uint32_t line) { if (line!=608) throw std::logic_error("TypeNames실제진단행");++asserts;Record("A:608");Mutate("assert"); };
        hooks.loadingDepthChanged=[this](std::uint32_t value) { Record("L:"+std::to_string(value)); };return hooks;
    }
    // 본문/별도cleanup시점의같은현재값을관측한다.
    std::string State() const {
        const auto found=std::find(kTypeNamesBlocks.begin(),kTypeNamesBlocks.end(),conversion);if (found==kTypeNamesBlocks.end()) throw std::logic_error("TypeNames관측pointer");return std::to_string(depth)+','+std::to_string((found-kTypeNamesBlocks.begin())*11)+','+std::to_string(count)+','+std::to_string(selected)+','+std::to_string(selects)+','+std::to_string(asserts)+','+std::to_string(allocations)+','+std::to_string(records);
    }
    // CPU/callback이바꾸는전체raw를NUL뒤도대조한다.
    std::string Raw() const { return TypesHex(headers)+':'+TypesHex(streams[0])+':'+TypesHex(streams[1])+':'+TypesHex(data[0])+':'+TypesHex(data[1])+':'+TypesHex(blocks[0])+':'+TypesHex(blocks[1])+':'+TypesHex(blocks[2])+':'+TypesHex(names[0])+':'+TypesHex(names[1]); }
};
}

// 최초332입력의664본문과38별도cleanup을원문전체BYTE/순서와직접대조한다.
TEST_CASE(FortTypeNamesLoad_IndependentNative664) {
    std::ifstream input(NETSTORM_FORT_TYPE_NAMES_LOAD_FIXTURE);if (!input) throw std::runtime_error("TypeNames최초fixture없음");std::string line;std::size_t compared{},stopped{};std::set<std::pair<std::string,std::string>> unique; //중복/누락입력도확인한다.
    // 입력열만으로실행한뒤제품작성전에고정한기대열을읽는다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;const auto row=TypesFields(line,'\t');if (row.size()!=7) throw std::runtime_error("TypeNames관측칸수");CHECK(unique.emplace(row[0],row[1]).second);
        TypesDevices devices(row[1]);auto hooks=devices.Hooks();std::string completion="1";
        try { LoadFortTypeNames(static_cast<std::int32_t>(devices.slot),devices.Globals(),hooks); }catch (const TypesStop& stop) { completion=stop.cause; }catch (const std::out_of_range&) { if (devices.physical.empty()) throw;completion=devices.physical; }
        const auto body=devices.State(),raw=devices.Raw(),trace=devices.trace;std::string cleanup="-";
        if (completion!="1") { ++stopped;devices.trace.clear();UnwindFortTypeNamesLoad(devices.Globals(),hooks);cleanup=devices.State()+'/'+devices.trace;CHECK(devices.Raw()==raw); }
        if (completion!=row[2] || body!=row[3] || raw!=row[4] || trace!=row[5] || cleanup!=row[6]) {
            std::fprintf(stderr,"TypeNames input=%s\ncompleted=%s expected=%s\nstate=%s expected=%s\ntrace=%s\nexpected=%s\ncleanup=%s expected=%s\n",row[1].c_str(),completion.c_str(),row[2].c_str(),body.c_str(),row[3].c_str(),trace.c_str(),row[5].c_str(),cleanup.c_str(),row[6].c_str());CHECK(completion==row[2]);CHECK(body==row[3]);CHECK(raw==row[4]);CHECK(trace==row[5]);CHECK(cleanup==row[6]);return;
        }
        ++compared;
    }
    CHECK(compared==664 && stopped==38);
}

// 빈stream의미도달경계/부분zero/원시NUL누락을원문fixture와별도로검사한다.
TEST_CASE(FortTypeNamesLoad_SelectedBoundariesAndPartialState) {
    TypesDevices empty("1,11,0,0,1,0,4,0,7,11,22,1,none");auto hooks=empty.Hooks();hooks.allocate={};hooks.typeName={};hooks.typeCount={};hooks.assertion={};hooks.buffer={};hooks.conversionBuffer={};LoadFortTypeNames(1,empty.Globals(),hooks);CHECK(empty.conversion==0 && empty.depth==1 && empty.allocations==0);
    TypesDevices missing("1,11,0,1,1,0,4,0,7,11,22,1,none");hooks=missing.Hooks();hooks.assertion={};bool rejected{};try { LoadFortTypeNames(1,missing.Globals(),hooks); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && missing.depth==2 && TypesRead(missing.streams[0],4)==1 && missing.allocations==0 && missing.conversion==kTypeNamesBlocks[1]);UnwindFortTypeNamesLoad(missing.Globals(),hooks);CHECK(missing.depth==1);
    TypesDevices shortBlock("1,11,0,1,1,0,4,0,7,0,22,1,none");hooks=shortBlock.Hooks();hooks.conversionBuffer=[&](auto)->std::span<std::uint8_t> { return std::span(shortBlock.blocks[1]).first(254); };rejected=false;try { LoadFortTypeNames(1,shortBlock.Globals(),hooks); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected && shortBlock.depth==2 && shortBlock.conversion==kTypeNamesBlocks[2] && shortBlock.blocks[1][0]==0 && shortBlock.blocks[1][253]==0 && shortBlock.blocks[1][254]!=0);UnwindFortTypeNamesLoad(shortBlock.Globals(),hooks);CHECK(shortBlock.depth==1);
    TypesDevices noNul("1,11,0,1,1,0,4,0,1,0,22,1,none");hooks=noNul.Hooks();constexpr std::array<std::uint8_t,2> kMissingNul{'A','B'};hooks.typeName=[&](auto)->std::span<const std::uint8_t> { return kMissingNul; };rejected=false;try { LoadFortTypeNames(1,noNul.Globals(),hooks); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected && noNul.depth==2 && noNul.conversion==kTypeNamesBlocks[2]);UnwindFortTypeNamesLoad(noNul.Globals(),hooks);CHECK(noNul.depth==1);
}
