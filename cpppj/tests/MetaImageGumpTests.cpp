// 원본 MetaImage의 기본 생성과 배율/ftol/가상 효과 순서를 독립 관측과 비교한다.
#include "RawSceneSupport.h"
#include "client/MetaImageGump.h"
#include <cstdio>

using namespace netstorm::client;
using namespace netstorm::test::rawscene;

// 동일 Native 입력의 두 제어 모드 결과를 비트 그대로 비교한다.
TEST_CASE(MetaImageGump_ReplaysNativeConstructorGeometryAndDraw) {
    const auto fixture=LoadFixture(NETSTORM_META_IMAGE_FIXTURE);std::uint32_t checked{};
    for (const auto& row:fixture.rows) { // 기대 기하/호출 인자는 원본 행에서 직접 읽는다.
        if (row[0]=="op") continue;
        RawImage image;const auto width=static_cast<std::int32_t>(Number(row[1])),height=static_cast<std::int32_t>(Number(row[2]));
        if (Number(row[9]) && width>0 && height>0) image.Convert({width,height,std::vector<std::uint8_t>(static_cast<std::size_t>(width)*height,7)},false);
        MetaImageGumpNode node;node.image=&image;node.colorMode=Number(row[6]);node.scale=std::bit_cast<float>(Number(row[3]));node.baseWidth=std::bit_cast<float>(Number(row[4]));node.baseHeight=std::bit_cast<float>(Number(row[5]));
        GumpCreateState state;state.collection.edition=OriginalEdition::Patch1078;GumpCreate factory(state,{[&](auto token)->GumpCreateNode* { return token==1 ? &node : nullptr; },{}});std::vector<std::string> trace;
        MetaImageGumpHooks hooks;hooks.node=[&](auto token) { return token==1 ? &node : nullptr; };
        hooks.position=[&](auto) { trace.push_back("position");return ScreenPoint{std::stoi(row[7]),std::stoi(row[8])}; };
        hooks.attach=[&](auto,float x,float y,auto parent,auto mode) {
            trace.push_back("attach:"+std::to_string(std::bit_cast<std::uint32_t>(x))+":"+std::to_string(std::bit_cast<std::uint32_t>(y))+":"+std::to_string(parent)+":"+std::to_string(mode));
            node.baseWidth=std::bit_cast<float>(Number(row[4]));node.baseHeight=std::bit_cast<float>(Number(row[5]));
            if (Number(row[10])) image.Convert({37,19,std::vector<std::uint8_t>(37*19,7)},false);
        };
        hooks.color=[&](auto token) { trace.push_back("color:"+std::to_string(token)); };
        hooks.draw=[&](MetaImageDrawRequest request) {
            CHECK(request.image==&image);trace.push_back(std::string("draw:")+(request.scaled ? "scaled" : request.mapped ? "mapped" : "plain")+":"+std::to_string(static_cast<std::uint32_t>(request.position.x))+":"+std::to_string(static_cast<std::uint32_t>(request.position.y))+":"+std::to_string(static_cast<std::uint32_t>(request.scale)));
        };
        MetaImageGump body(state,factory,std::move(hooks));
        if (row[0]=="Construct") body.Construct(1,image);else if (row[0]=="Resize") body.Resize(1);else if (row[0]=="Attach") body.Attach(1,1.25F,-2.5F,0xaabbccddU,0x80000000U);else body.Draw(1);
        const auto joined=trace.empty() ? std::string("-") : [&] { std::string text;for (const auto& item:trace) { if (!text.empty()) text+=';';text+=item; }return text; }(); // 원본 관측과 같은 구분자를 사용한다.
        const bool same=std::bit_cast<std::uint32_t>(node.scale)==Number(row[12]) && std::bit_cast<std::uint32_t>(node.baseWidth)==Number(row[13]) && std::bit_cast<std::uint32_t>(node.baseHeight)==Number(row[14]) && std::bit_cast<std::uint32_t>(node.width)==Number(row[15]) && std::bit_cast<std::uint32_t>(node.height)==Number(row[16]);
        if (!same || joined!=row[22]) std::fprintf(stderr,"MetaImage Native행%u %s 기하/순서 불일치\n",checked,row[0].c_str());
        CHECK(same && joined==row[22]);CHECK(node.command==Number(row[17]) && node.vtable==Number(row[18]) && node.flags==Number(row[19]));CHECK(state.count==Number(row[20]) && state.serial==Number(row[21]));++checked;
    }
    CHECK(checked==326);
}
