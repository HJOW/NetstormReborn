// 독립 원본 관측으로 편집/단독 맵의50개 캐시·행·호출 순서를 대조한다.
#include "o/RawSoloMapList.h"
#include "RawSceneSupport.h"
#include <filesystem>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;

namespace {
// 빈 문자열 표식과 원래 좁은 문자열 BYTE를 복원한다.
std::string Decode(std::string_view value) {
    std::string text;if (value=="_") return text;
    for (std::size_t index=0;index<value.size();index+=2) text.push_back(static_cast<char>(std::stoul(std::string(value.substr(index,2)),nullptr,16))); //한 쌍을한 BYTE로읽는다.
    return text;
}
// 포인터 토큰 대신 실제 문자열 원문을 독립 관측과 대조한다.
std::string Encode(std::string_view value) {
    constexpr char digits[]="0123456789abcdef"; //관측과동일한소문자16진수표다.
    std::string text;
    for (const auto byte:value) { const auto number=static_cast<unsigned char>(byte);text+=digits[number>>4];text+=digits[number&15]; } //각원문BYTE를두글자로보존한다.
    return text.empty() ? "_" : text;
}
// 지정 순서의 관측만 연결하며 새 기대 결과를 계산하지 않는다.
std::string Join(const std::vector<std::string>& values,char separator) {
    std::string text;for (const auto& value:values) { if (!text.empty()) text+=separator;text+=value; }return text; //중간구분자만넣는다.
}
// 파일/Template 반환은 명시 외부 입력이고 목록 판단은 제품이 수행한다.
struct ListScene {
    struct File { std::string name,subscriber;std::uint8_t kind{},state{};bool present{}; }; //한원본파일의명시읽기입력이다.
    SoloMapListState state; //같은실제50개캐시다.
    std::vector<File> files; //원본실행기에공급한파일순서다.
    std::vector<std::string> trace; //하위요청의실제순서다.
    std::string user; //현재사용자비교입력이다.
    std::int32_t option{}; //설정조회가반환하는원문정수다.
    const File* active{}; //가장최근실제Load가선택한명시Template이다.
    // 입력 열만 설치하고 기대 출력 열은 쓰지 않는다.
    explicit ListScene(const std::vector<std::string>& row) {
        option=std::stoi(row[2]);state.dirty=static_cast<std::uint32_t>(std::stoul(row[3]));state.count=std::stoi(row[4]);user=Decode(row[8]);
        const auto names=Split(row[5],','),flags=Split(row[6],',');
        for (std::size_t index=0;index<50;++index) { if (names[index]!="-") state.names[index]=Decode(names[index]);state.flags[index]=static_cast<std::uint32_t>(std::stoul(flags[index])); } //같은50개초기칸을복원한다.
        if (row[7]!="-") for (const auto& text:Split(row[7],';')) { const auto fields=Split(text,'/');files.push_back({Decode(fields[0]),Decode(fields[4]),static_cast<std::uint8_t>(std::stoul(fields[1])),static_cast<std::uint8_t>(std::stoul(fields[3])),fields[2]=="1"}); } //파일별외부반환만읽는다.
    }
    // 실제 선택된 이름에 대응하는 입력만 조회한다.
    const File& Find(std::string_view name) const {
        const auto at=std::find_if(files.begin(),files.end(),[&](const auto& file) { return file.name==name; });if (at==files.end()) throw std::logic_error("검사 파일 입력 누락");return *at;
    }
    // 게임 계산을 대신하지 않는 파일/장치 경계를 명시한다.
    SoloMapListHooks Hooks() {
        SoloMapListHooks hooks;hooks.mayEditMissions=[&] { trace.push_back("O");return option; };
        hooks.enumerate=[&] { trace.push_back("E");std::vector<std::string> names;for (const auto& file:files) names.push_back(file.name);return names; }; //원래입력순서만반환한다.
        hooks.kind=[&](auto name) { trace.push_back("K:"+std::string(name));return Find(name).kind; };hooks.construct=[&] { trace.push_back("C");active=nullptr; };
        hooks.load=[&](auto name) { trace.push_back("L:"+std::string(name));active=&Find(name); };hooks.present=[&] { trace.push_back("P");return active->present; };
        hooks.subscriber=[&] { trace.push_back("S");return active->subscriber; };hooks.stateByte=[&] { trace.push_back("B");return active->state; };
        hooks.userName=[&] { trace.push_back("U");return user; };hooks.reserved=[&] { trace.push_back("R");return 0U; };
        hooks.translate=[&](auto key) { trace.push_back("T");return std::string(key); };return hooks;
    }
};
}
// 양정밀도에서일치한실제216입력의전체표/행/외부순서를대조한다.
TEST_CASE(SoloMapList_OriginalCacheFilterAndRows) {
    const auto fixture=LoadFixture((std::filesystem::path(__FILE__).parent_path()/"fixtures/solo-map-list-x86.tsv").string().c_str());CHECK(fixture.rows.size()==216);
    for (const auto& row:fixture.rows) { //각원본입력은새캐시와파일수명을갖는다.
        CHECK(row.size()==18);if (row.size()!=18) continue;ListScene scene(row);RawSoloMapList list(scene.state,scene.Hooks());CHECK(list.Refresh(std::stoi(row[1]))==std::stoi(row[11]));CHECK(scene.state.dirty==std::stoul(row[12]));
        const auto names=Split(row[13],','),flags=Split(row[14],',');
        for (std::size_t index=0;index<50;++index) { CHECK(scene.state.names[index].has_value()==(names[index]!="-"));if (scene.state.names[index]) CHECK(*scene.state.names[index]==Decode(names[index]));CHECK(scene.state.flags[index]==std::stoul(flags[index])); } //반환밖모든표칸도확인한다.
        std::vector<std::string> observed;
        for (const auto& value:Split(row[15],',')) { const auto item=list.Row(std::stoi(value),Decode(row[9]),Decode(row[10]));observed.push_back(!item ? "n" : std::to_string(item->checked)+","+std::to_string(item->enabled)+","+Encode(item->label)+","+Encode(item->name)); } //실제행반환만직렬화한다.
        CHECK(Join(observed,';')==row[16]);CHECK(Join(scene.trace,',')==row[17]);
    }
}
// 설정조회효과의dirty는다음갱신에만적용하며번역뒤현재이름을다시읽는다.
TEST_CASE(SoloMapList_CurrentCacheAndTranslationEffects) {
    SoloMapListState state;state.names[0]="Old";state.count=1;state.flags[0]=2;SoloMapListHooks hooks;
    hooks.mayEditMissions=[&] { state.dirty=1;return 0; };hooks.translate=[&](auto) { state.names[0]="New";state.flags[0]=8;return std::string("%s (~rCORRUPT~.)"); };
    RawSoloMapList list(state,std::move(hooks));CHECK(list.Refresh(2)==1 && state.dirty==1);const auto row=list.Row(0,"old","");CHECK(row && row->checked && row->enabled && row->name=="New" && row->label=="New (~rCORRUPT~.)");CHECK(!list.Row(1,"","") && Throws([&] { list.Row(-1,"",""); }));
}
