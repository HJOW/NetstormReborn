// 배너 원문2066입력·일반 F7 208입력과 같은 현재 색표의 수명을 대조한다.
#include "TestSupport.h"
#include "client/OwnerColorTableState.h"
#include "client/ViewKeyInput.h"
#include "o/RawColorSortPrimary.h"
#include <bit>
#include <fstream>
#include <string>
#include <vector>

using namespace netstorm::client;
namespace { // 이 검사의 장치는 입력 열만 사용하며 기대 관측으로 분기를 정하지 않는다.
std::vector<std::string> SplitBanner(std::string_view text,char separator) { // 마지막 빈 필드도 남기는 구분자 분리다.
    std::vector<std::string> output;std::size_t begin{}; // 현재 필드 시작과 원래 순서의 결과다.
    for (;;) { const auto end=text.find(separator,begin);output.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return output;begin=end+1; } // 원문 구분자의 다음 위치만 읽는다.
}
std::uint32_t BannerNumber(std::string_view text) { return static_cast<std::uint32_t>(std::stoull(std::string(text))); } // signed 축소 없이 원시 DWORD를 읽는다.
std::uint32_t BannerToken(std::uint32_t type) { return 0x100000U+type*2304U; } // native 명시 할당 장치와 같은 고정 별칭이다.
std::string BannerNumbers(std::span<const std::uint32_t> values) { // 전체 현재 포인터/소스 순서를 직렬화한다.
    std::string output; // 빈 목록도 원래 빈 문자열이다.
    for (const auto value:values) { if (!output.empty()) output+=',';output+=std::to_string(value); } // 지정 배열 순서를 유지한다.
    return output;
}
}

TEST_CASE(BannerOption_Native2066AndInitialRegistration) { // 설정 쓰기와 전체 초기화의 실제188포인터·효과 순서다.
    std::ifstream input(NETSTORM_BANNER_OPTION_FIXTURE);CHECK(input.good());if (!input) return;std::string line;std::size_t count{}; // 실제 파일과 고유 비교 수다.
    while (std::getline(input,line)) { // 입력9열을 먼저 공급하고 실행 뒤 관측6열을 대조한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.starts_with("name\t")) continue;
        const auto row=SplitBanner(line,'\t');CHECK(row.size()==15);if (row.size()!=15) continue;++count;
        auto ready=BannerNumber(row[2]),enabled=BannerNumber(row[3]),dirty=BannerNumber(row[4]);std::vector<std::uint32_t> pointers(188);pointers[111]=BannerToken(111);OwnerColorTableTypes types;std::string trace; // Native 시작 입력만 공급한다.
        const auto typeInput=SplitBanner(row[6],',');CHECK(typeInput.size()==5);if (typeInput.size()!=5) continue;
        for (std::size_t index=0;index<5;++index) { types.banner[index]=BannerNumber(typeInput[index]);if (BannerNumber(row[5])&(1U<<index)) pointers[types.banner[index]]=BannerToken(types.banner[index]); } // 현재 타입 전역과 존재 마스크다.
        const auto append=[&](std::string event) { if (!trace.empty()) trace+=';';trace+=event; }; // 실제 제품 효과만 기록한다.
        OwnerColorTableHooks hooks; // 할당 장치와 free 장치의 명시 응답이다.
        hooks.standard=[&](auto type) { append("standard:"+std::to_string(type));pointers[type]=BannerToken(type); };
        hooks.special=[&](auto type,auto offset,auto sources) { append("special:"+std::to_string(type)+':'+std::to_string(std::bit_cast<std::uint64_t>(offset))+':'+BannerNumbers(sources));pointers[type]=BannerToken(type); };
        hooks.release=[&](auto pointer) { append("free:"+std::to_string(pointer)); };
        for (const auto& request:SplitBanner(row[7],',')) { // 연속 요청 사이에 현재 포인터/설정을 재생성하지 않는다.
            if (row[1]=="init") InitializeOwnerColorTables({ready,enabled,dirty,pointers},BannerNumber(row[8]),types,hooks);
            else SetBannerOption({ready,enabled,dirty,pointers},BannerNumber(request),types,hooks);
        }
        CHECK(ready==BannerNumber(row[9]));CHECK(enabled==BannerNumber(row[10]));CHECK(dirty==BannerNumber(row[11]));CHECK(BannerNumbers(pointers)==row[12]);CHECK((trace.empty() ? "-" : trace)==row[13]);CHECK(BannerNumber(row[14])>0);
    }
    CHECK(count==2066);
}

TEST_CASE(BannerKey_Native208CurrentValueAndLazyDevices) { // 정지 뒤 현재 DWORD의 논리 반전과 직접 Config 쓰기를 비교한다.
    std::ifstream input(NETSTORM_BANNER_KEY_FIXTURE);CHECK(input.good());if (!input) return;std::string line;std::size_t count{}; // 명시 native 파일과 비교 수다.
    while (std::getline(input,line)) { // 상위 관문과 설정 장치 응답은 입력 열에서만 읽는다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.starts_with("name\t")) continue;
        const auto row=SplitBanner(line,'\t');CHECK(row.size()==14);if (row.size()!=14) continue;++count;
        auto enabled=BannerNumber(row[6]);std::uint32_t hide{},dirty{};std::string trace; // 다른 표시 전역은 바뀌지 않아야 한다.
        const auto append=[&](std::string text) { if (!trace.empty()) trace+=';';trace+=text; }; // 실제 호출만 순서대로 남긴다.
        ViewKeyInputHooks hooks;hooks.paused=[&] { append("paused:"+row[4]);return std::bit_cast<std::int32_t>(BannerNumber(row[4]))>0; };hooks.useBanners=[&] { append("banners:"+std::to_string(enabled));return enabled; };
        hooks.writeBanners=[&](auto value) { append("config:useBanners=\""+std::to_string(value)+'"');if (BannerNumber(row[7])) enabled=value; };
        for (const auto& text:SplitBanner(row[1],';')) { // 이미 Guide/Gump/console이 소비한 사건은 이 하위 몸체에 전달하지 않는다.
            if (BannerNumber(row[2]) || BannerNumber(row[3]) || BannerNumber(row[5])) continue;
            const auto event=SplitBanner(text,':');static_cast<void>(DispatchViewKeyInput({BannerNumber(event[0]),std::bit_cast<std::int32_t>(BannerNumber(event[1])),std::bit_cast<std::int32_t>(BannerNumber(event[2]))},{hide,dirty},hooks));
        }
        std::string selected; // native 전체 추적에서 이 하위 모듈이 담당하는 현재 읽기/효과만 선택한다.
        for (const auto& event:SplitBanner(row[10],';')) { if (!event.starts_with("paused:") && !event.starts_with("banners:") && !event.starts_with("config:")) continue;if (!selected.empty()) selected+=';';selected+=event; } // 서식 함수는 명시 장치로 구별한다.
        CHECK(trace==selected);CHECK(enabled==BannerNumber(row[8]));CHECK(row[9]=="-" && row[11]=="1");CHECK(!hide && !dirty);
    }
    CHECK(count==208);
}

TEST_CASE(OwnerColorTableState_CurrentSpansSurviveToggle) { // 실제 primary와 같은 표 주소를 화면/이미지 소비자가 계속 빌릴 수 있어야 한다.
    std::array<std::uint8_t,netstorm::o::kColorSortPrimaryBytes> bytes{};auto primary=std::make_shared<netstorm::o::RawColorSortPrimary>(bytes);std::array<std::uint32_t,256> rgb{}; // 이 검사가 명시한 최초 물리/RGB 장치다.
    for (std::uint32_t index=0;index<256;++index) rgb[index]=index|(index<<8)|(index<<16); // 명시 회색 입력을 같은 색 번호로 구성한다.
    primary->Construct({reinterpret_cast<const std::uint8_t*>(rgb.data()),sizeof(rgb)});OwnerColorTableState owner;std::uint32_t dirty{};
    owner.SetEnabled(0,dirty);CHECK(owner.Enabled()==1 && !owner.Ready() && dirty==0);owner.Initialize(primary,188,82,dirty);const auto span=owner.Tokens();const auto* address=span.data();const auto* enabled=&owner.Enabled();
    CHECK(owner.Ready() && dirty==1 && *enabled==1);CHECK(span[82]!=0 && span[111]!=0 && span[141]!=0 && span[137]!=0);CHECK(!span[94] && !span[97] && !span[85] && !span[71] && !span[128]);
    owner.SetEnabled(0,dirty);CHECK(dirty==2 && !*enabled && span[94]!=0 && span[97]!=0);const auto* color=owner.Color(97,2).data();CHECK(owner.ColorToken(span[97]+512).data()==color);
    owner.SetEnabled(0,dirty);CHECK(dirty==2 && owner.Color(97,2).data()==color && owner.Tokens().data()==address);
    owner.SetEnabled(1,dirty);CHECK(dirty==3 && *enabled==1 && !span[94] && !span[97]);CHECK(span[82]!=0 && span[111]!=0 && owner.Tokens().data()==address);
}
