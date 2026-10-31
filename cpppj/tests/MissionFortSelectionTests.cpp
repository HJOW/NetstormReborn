// 미션 파일 선택이 I/O 없이 이름을 고정하고 같은 source의 지연 읽기를 보유하는지 검사한다.
#include "TestSupport.h"
#include "client/Mission.h"
#include <memory>
#include <stdexcept>

using namespace netstorm::client;
using namespace netstorm::o;
namespace { // 실제 Config 치환을 사용하고 파일 장치는 명시 메모리 입력으로 제한한다.
template<class Function> std::string SelectionFailure(Function function) { // 지연 파일 부재와 reader 예외가 선택 시점의 성공과 구별되는지 진단 문자열로 확인한다.
    try { function(); }catch (const std::exception& error) { return error.what(); }return {};
}
}

TEST_CASE(MissionFortSelection_SeparatesChoiceFromReadAndFreezesSelectedPath) { // 선택 뒤 Config가 바뀌어도 보관한 read는 앞선 경로를 읽으며 새 선택만 현재 경로를 쓴다.
    std::vector<std::string> reads,mapped;ConfigInterface config([](auto)->std::optional<std::vector<std::uint8_t>> { throw std::logic_error("전역 reader 우회 호출"); });
    config.Configuration().Assign("missionSpec=\"d/{local.1}.english\"\nfortSpec=\"first/{local.1}.fort\"\n[END]\n");
    MissionScript mission(config,"Choice",{},[&](std::string_view path)->std::optional<std::vector<std::uint8_t>> {
        reads.emplace_back(path);return std::vector<std::uint8_t>(path.begin(),path.end());
    }); // 생성자의 script 읽기는 선택기 검사 시작 전에 분리한다.
    reads.clear();auto select=mission.FortFileSelector([&](std::string_view path) { mapped.emplace_back(path);return "physical/"+std::string(path); });
    auto first=select("Picked");CHECK(reads.empty() && mapped==std::vector<std::string>{"first/Picked.fort"} && first.fileName=="physical/first/Picked.fort");
    config.Set("fortSpec","second/{local.1}.fort");const auto bytes=first.read();CHECK(reads==std::vector<std::string>{"first/Picked.fort"} && std::string(bytes.begin(),bytes.end())=="first/Picked.fort");
    const auto second=select("Picked");CHECK(reads.size()==1 && mapped.back()=="second/Picked.fort" && second.fileName=="physical/second/Picked.fort");
    const auto next=second.read();CHECK(reads.size()==2 && reads.back()=="second/Picked.fort" && std::string(next.begin(),next.end())=="second/Picked.fort");
}

TEST_CASE(MissionFortSelection_ReadOwnsSourceAfterScriptAndConfigRetirement) { // 이미 선택한 지연 읽기는 Script와 Config를 빌리지 않고 source의 소유 수명만 유지한다.
    MissionFortFileSelection selected;std::weak_ptr<std::vector<std::string>> weak;
    {
        auto owner=std::make_shared<std::vector<std::string>>();weak=owner;ConfigInterface config({});
        config.Configuration().Assign("missionSpec=\"d/{local.1}.english\"\nfortSpec=\"chosen/{local.1}.fort\"\n[END]\n");
        MissionScript mission(config,"Choice",{},[owner](std::string_view path)->std::optional<std::vector<std::uint8_t>> { owner->emplace_back(path);return std::vector<std::uint8_t>{7,8,9}; });
        selected=mission.FortFileSelector()("Held");CHECK(selected.fileName=="chosen/Held.fort");
    }
    CHECK(!weak.expired() && selected.read()==std::vector<std::uint8_t>({7,8,9}) && weak.lock()->back()=="chosen/Held.fort");selected={};CHECK(weak.expired());
}

TEST_CASE(MissionFortSelection_DefersMissingEmptyAndThrowWithoutFallback) { // 파일 부재·빈 파일·reader 예외는 실제 read를 호출한 순간에 기존 계약대로 구별된다.
    std::size_t reads{};ConfigInterface config([](auto)->std::optional<std::vector<std::uint8_t>> { throw std::logic_error("선택 source의 전역 fallback"); });
    config.Configuration().Assign("missionSpec=\"d/{local.1}.english\"\nfortSpec=\"d/{local.1}.fort\"\n[END]\n");
    MissionScript mission(config,"Choice",{},[&](std::string_view path)->std::optional<std::vector<std::uint8_t>> {
        ++reads;if (path=="d/empty.fort") return std::vector<std::uint8_t>{};if (path=="d/throw.fort") throw std::runtime_error("selected reader failure");return std::nullopt;
    });
    reads=0;const auto select=mission.FortFileSelector();const auto missing=select("missing"),empty=select("empty"),failure=select("throw");CHECK(!reads);
    CHECK(SelectionFailure([&] { static_cast<void>(missing.read()); })=="Game file not found: d/missing.fort" && reads==1);
    CHECK(empty.read().empty() && reads==2);CHECK(SelectionFailure([&] { static_cast<void>(failure.read()); })=="selected reader failure" && reads==3);
}
