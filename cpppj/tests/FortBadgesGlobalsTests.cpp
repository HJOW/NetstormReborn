// 실제CRT/reset의전체6160BYTE와같은장부를쓰는MetaLoader/getter/writer연결을검사한다.
#include "TestSupport.h"
#include "o/RawFortBadgesGlobals.h"
#include "o/RawFortBadgesSave.h"
#include "o/RawFortMetaLoad.h"
#include <algorithm>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::o;
namespace {
// 독립TSV의입력과전체원문관측열을분리한다.
std::vector<std::string> BadgeGlobalsFields(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { // 끝의빈칸도남겨실제열수를보존한다.
        const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1;
    }
}
// 원본에공급한명시프로필만만들고출력의기대초기화는계산하지않는다.
std::vector<std::uint8_t> BadgeGlobalsInitial(std::uint32_t profile) {
    std::vector<std::uint8_t> bytes(kFortBadgesGlobalsBytes);
    for (std::size_t index=0;index<bytes.size();++index) { // type/count/padding도같은초기BYTE패턴으로채운다.
        switch (profile) {
        case 0:break;
        case 1:bytes[index]=255;break;
        case 2:bytes[index]=0xa5;break;
        case 3:bytes[index]=static_cast<std::uint8_t>(index);break;
        case 4:bytes[index]=static_cast<std::uint8_t>(255U-index);break;
        case 5:bytes[index]=static_cast<std::uint8_t>(index*37U+19U);break;
        case 6:bytes[index]=static_cast<std::uint8_t>(((static_cast<std::uint32_t>(index/4)*0x10003U+0x80000000U)>>((index%4)*8)));break;
        default:CHECK(profile==7);bytes[index]=static_cast<std::uint8_t>(1+index%255);break;
        }
    }return bytes;
}
// 실제원문전체바이트의16진수관측을손실없이읽는다.
std::vector<std::uint8_t> BadgeGlobalsUnhex(std::string_view hex) {
    std::vector<std::uint8_t> bytes;CHECK(!(hex.size()%2));
    for (std::size_t offset=0;offset<hex.size();offset+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(hex.substr(offset,2)),nullptr,16)));return bytes; // 원문모든바이트를기대자료로만읽는다.
}
// 비정렬little endianDWORD를현재기록에서읽는다.
std::uint32_t BadgeGlobalsRead(std::span<const std::uint8_t> bytes,std::size_t at=0) {
    std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value; // 네바이트폭만합친다.
}
// 명시stream필드만써실제writer에공급한다.
void BadgeGlobalsPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 인접자료는보존한다.
}
// 물리계약오류를검사실패와구분한다.
template<class Function> bool BadgeGlobalsRejects(Function function) { try { function(); } catch (const std::invalid_argument&) { return true; }return false; }
// 선택하지않는Technology경계는성공noop대신명시거부한다.
FortMetaLoadHooks BadgeGlobalsMetaHooks(std::size_t& assertions) {
    FortMetaLoadHooks hooks;hooks.assertion=[&](auto kind) { CHECK(kind==FortMetaAssertion::BadgeCountRange);++assertions; };
    hooks.loadingDepthChanged=[](auto) { throw std::logic_error("패치Badges가로딩깊이를바꾸면안됩니다"); };
    hooks.technologyParent=[]()->Sid { throw std::logic_error("Badges검사에서기술부모조회금지"); };
    hooks.prepareTypeNames=[] { throw std::logic_error("Badges검사에서타입변환준비금지"); };
    hooks.contents=[](auto,auto&,auto,auto) { throw std::logic_error("Badges검사에서기술내용물실행금지"); };
    hooks.releaseTypeConversion=[] { throw std::logic_error("Badges검사에서타입변환반납금지"); };
    hooks.refreshTechnologyUi=[] { throw std::logic_error("Badges검사에서기술UI실행금지"); };return hooks;
}
}

// 실제원본32입력의초기0/비0전체장부와두함수순서를전수비교한다.
TEST_CASE(FortBadgesGlobals_IndependentNativeCrtAndReset) {
    std::ifstream input(NETSTORM_FORT_BADGES_GLOBALS_FIXTURE);CHECK(input.good());std::string line;std::size_t compared{};
    while (std::getline(input,line)) { // 제품자료를기대로쓰지않고뒤의실제PE출력을비교한다.
        if (line.empty() || line.starts_with('#')) continue;const auto row=BadgeGlobalsFields(line,'\t');CHECK(row.size()==10);if (row.size()!=10) continue;
        auto bytes=BadgeGlobalsInitial(static_cast<std::uint32_t>(std::stoul(row[2])));const auto failures=netstorm::test::FailureCount();
        for (const auto& stage:BadgeGlobalsFields(row[1],'-')) { // 원본과같은명시두함수순서만적용한다.
            if (stage=="crt") InitializeFortBadgesRecords(bytes);else { CHECK(stage=="reset");ResetFortBadgeCounts(bytes); }
        }
        CHECK(row[3]=="1" && bytes==BadgeGlobalsUnhex(row[4]));CHECK(std::stoi(row[6])==0 && std::stoul(row[7])>0 && std::stoul(row[8])>0);
        CHECK(std::stoul(row[9])==(row[1]=="reset" ? 112U : 6152U));++compared;
        if (netstorm::test::FailureCount()!=failures) std::fprintf(stderr,"Badges 전역 입력: %s\n",row[0].c_str());
    }
    CHECK(compared==32);
}

// 새소유자는BSS0→CRT한번이며span주소를보존하고reset은count외6048BYTE를바꾸지않는다.
TEST_CASE(FortBadgesGlobals_OwnerAddressAndCountOnlyReset) {
    RawFortBadgesGlobals first,second;auto mutableView=first.Records();const auto& constant=first;const auto readonly=constant.Records();
    CHECK(mutableView.data()==readonly.data() && mutableView.data()!=second.Records().data() && mutableView.size()==6160);
    std::ifstream input(NETSTORM_FORT_BADGES_GLOBALS_FIXTURE);CHECK(input.good());std::string line;
    while (std::getline(input,line)) { // 최초BSS0→CRT원문한행을소유자생성결과와직접비교한다.
        if (line.empty() || line.starts_with('#')) continue;const auto row=BadgeGlobalsFields(line,'\t');const auto expected=BadgeGlobalsUnhex(row[4]);CHECK(std::equal(mutableView.begin(),mutableView.end(),expected.begin(),expected.end()));break;
    }
    const auto pattern=BadgeGlobalsInitial(7);std::copy(pattern.begin(),pattern.end(),mutableView.begin());const auto previous=std::vector<std::uint8_t>(mutableView.begin(),mutableView.end());first.ResetCounts();
    for (std::size_t index=0;index<mutableView.size();++index) CHECK(mutableView[index]==(index%220>=216 ? 0 : previous[index])); // 모든count외바이트를전부대조한다.
    CHECK(readonly.data()==mutableView.data() && readonly[4]==previous[4]);
    auto shortBytes=std::vector<std::uint8_t>(6159,0xa5),longBytes=std::vector<std::uint8_t>(6161,0xa5);
    CHECK(BadgeGlobalsRejects([&] { InitializeFortBadgesRecords(shortBytes); }));CHECK(BadgeGlobalsRejects([&] { ResetFortBadgeCounts(longBytes); }));
    CHECK(std::all_of(shortBytes.begin(),shortBytes.end(),[](auto value) { return value==0xa5; }) && std::all_of(longBytes.begin(),longBytes.end(),[](auto value) { return value==0xa5; }));
}

// 기존실제MetaLoader와새getter/writer가같은소유자장부를직접사용하고빈/부분로드를보존한다.
TEST_CASE(FortBadgesGlobals_SharedMetaLoaderGetterAndWriter) {
    auto owner=std::make_shared<RawFortBadgesGlobals>();auto observer=owner;auto records=owner->Records();const auto original=std::vector<std::uint8_t>(records.begin(),records.end());
    SidPool pool(OriginalEdition::Patch1078,24000);std::uint32_t depth=5;std::size_t assertions{};FortMetaLoadState state;RawFortMetaLoad meta(pool,records,depth,state,BadgeGlobalsMetaHooks(assertions));
    const std::array<std::uint8_t,5> load{2,71,7,0,9};FortMetaStream input{load,0,5,true};meta.LoadBadges(input);
    CHECK(input.position==5 && depth==5 && !assertions && observer->Records().data()==records.data());CHECK(ReadFortBadgeCount(observer->Records(),71)==7 && ReadFortBadgeCount(observer->Records(),0)==9);
    meta.SetBadge(72,256);CHECK(assertions==1 && ReadFortBadgeCount(observer->Records(),72)==256);FortMetaStream empty{{},0,0,false};meta.LoadBadges(empty);
    const std::array<std::uint8_t,3> partial{1,71,8};FortMetaStream changed{partial,0,3,true};meta.LoadBadges(changed);CHECK(ReadFortBadgeCount(records,71)==8 && ReadFortBadgeCount(records,0)==9 && ReadFortBadgeCount(records,72)==256);
    for (std::size_t index=0;index<records.size();++index) if (index%220<216) CHECK(records[index]==original[index]); // setter가원본다른필드/패턴을건드리지않는다.
    std::array<std::uint8_t,16> fields{};std::array<std::uint8_t,64> buffer{};BadgeGlobalsPut(fields,0,77);BadgeGlobalsPut(fields,12,64);FortBadgesSaveHooks hooks;
    hooks.manager=[](auto slot) { CHECK(slot==1);return 11U; };hooks.create=[](auto manager,std::string_view name) { CHECK(manager==11 && name=="Badges");return 9U; };
    hooks.assertion=[](auto) { throw std::logic_error("충분한실제Badges출력에진단이발생했습니다"); };
    hooks.count=[observer](auto type) { return ReadFortBadgeCount(observer->Records(),type); };
    hooks.access=[&](auto stream,const auto& operation) { CHECK(stream==9);MemStreamHooks memory;memory.buffer=[&](auto token)->std::span<std::uint8_t> { CHECK(token==77);return buffer; };operation(fields,memory); };
    const auto beforeSave=std::vector<std::uint8_t>(records.begin(),records.end());SaveFortBadges(1,hooks);const std::array<std::uint8_t,7> expected{3,0,9,71,8,72,0};
    CHECK(std::equal(expected.begin(),expected.end(),buffer.begin()) && BadgeGlobalsRead(fields,4)==7 && BadgeGlobalsRead(fields,8)==7);CHECK(std::equal(records.begin(),records.end(),beforeSave.begin(),beforeSave.end()));
    owner.reset();CHECK(observer->Records().data()==records.data() && ReadFortBadgeCount(records,71)==8);observer->ResetCounts();CHECK(ReadFortBadgeCount(records,71)==0 && ReadFortBadgeCount(records,72)==0 && ReadFortBadgeCount(records,0)==0);
}
