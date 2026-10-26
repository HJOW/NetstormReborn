// 원본 창의184개 실제 기하/드래그 관측을 같은 Image/MetaImage 저장소로 재생한다.
#include "RawSceneSupport.h"
#include "client/EditorIslandLayout.h"
#include <cstdio>

using namespace netstorm::client;
using namespace netstorm::test::rawscene;

// 자식 가상 효과만 명시하고 원문 float 비트/절삭/record/순서를 전체 비교한다.
TEST_CASE(EditorIslandLayout_ReplaysNativeGeometryAndMotion) {
    const auto fixture=LoadFixture(NETSTORM_EDITOR_ISLAND_LAYOUT_FIXTURE);std::uint32_t checked{};
    for (const auto& row:fixture.rows) { // 두 x87 정밀도의 실제 관측을 각각 재생한다.
        if (row[0]=="op") continue;
        GumpCreateNode root;std::array<MetaImageGumpNode,8> children;RawImage current;EditorIslandDragState interaction;std::vector<std::string> trace;
        if (Number(row[3]) && Number(row[4])) current.Convert({static_cast<std::int32_t>(Number(row[3])),static_cast<std::int32_t>(Number(row[4])),std::vector<std::uint8_t>(static_cast<std::size_t>(Number(row[3]))*Number(row[4]),7)},false);
        EditorIslandLayoutFields fields{&current,100,101,102,103,104,105,106,&interaction};interaction.drag=107;interaction.record={9,3,0x76,37,41,43};
        root.baseWidth=std::bit_cast<float>(Number(row[1]));root.baseHeight=std::bit_cast<float>(Number(row[2]));
        for (std::size_t index=0;index<children.size();++index) { // 자식0..6과 드래그의 원문 명시 입력이다.
            children[index].scale=std::bit_cast<float>(Number(row[5]));children[index].baseWidth=std::array{257.0F,225.0F,64.5F,11.0F,128.0F,77.0F,96.0F,32.5F}[index];children[index].baseHeight=std::array{257.0F,225.0F,24.5F,11.0F,18.5F,77.0F,96.0F,48.25F}[index];
        }
        const auto node=[&](std::uint32_t token)->GumpCreateNode* { return token==1 ? &root : token>=100 && token<=107 ? &children[token-100] : nullptr; };
        const auto label=[](std::uint32_t token) { return token==1 ? std::string("root") : token==107 ? std::string("drag") : std::to_string(token-100); };
        GumpCreateState create;create.collection.edition=OriginalEdition::Patch1078;GumpCreate factory(create,{node,{}});MetaImageGumpHooks mh;mh.node=[&](auto token) { return token>=100 && token<=107 ? &children[token-100] : nullptr; };MetaImageGump meta(create,factory,std::move(mh));
        EditorIslandLayoutHooks hooks;hooks.fields=[&](auto)->EditorIslandLayoutFields& { return fields; };hooks.node=node;hooks.image=[&](auto token) { return token>=100 && token<=107 ? &children[token-100] : nullptr; };hooks.resizeImage=[&](auto token) { meta.Resize(token); };
        hooks.baseAttach=hooks.attach=[&](auto token,float x,float y,auto parent,auto mode) {
            trace.push_back("attach:"+label(token)+":"+std::to_string(std::bit_cast<std::uint32_t>(x))+":"+std::to_string(std::bit_cast<std::uint32_t>(y))+":"+(parent ? label(parent) : "-")+":"+std::to_string(mode));node(token)->x=x;node(token)->y=y;
            if (token==1 && Number(row[9])==1) { root.baseWidth=500.25F;root.baseHeight=480.75F; }
            if (token==100 && Number(row[9])==3) { children[0].baseWidth=199.25F;children[0].baseHeight=201.75F;children[4].baseHeight=19.25F; }
        };
        hooks.push=[&](auto token) { trace.push_back("push:"+label(token));if (token==104 && Number(row[9])==2) { children[0].baseWidth=241.25F;children[0].baseHeight=253.5F;children[2].baseHeight=29.75F; }if (token==107 && Number(row[9])==4) { children[7].baseWidth=33.75F;children[7].baseHeight=51.25F; } };
        hooks.bounds=[&](auto) { trace.push_back("bounds");return ScreenRect{100,-37,324,187}; };hooks.position=[&](auto) { trace.push_back("position");return ScreenPoint{103,-41}; };hooks.root=[] { return 1U; };
        hooks.canPlace=[&](std::span<const std::uint8_t,6> record) { std::uint32_t word{};for (std::size_t byte=0;byte<4;++byte) word|=static_cast<std::uint32_t>(record[byte])<<(8U*byte); // 실제 값 전달 DWORD/WORD를 기록한다.
            trace.push_back("can:0:0:14:14:6:"+std::to_string(word)+":"+std::to_string(record[4]|(static_cast<std::uint32_t>(record[5])<<8U)));return Number(row[8])!=0; };
        EditorIslandLayout body(std::move(hooks));if (row[0]=="Attach") body.Attach(1,2.25F,-3.5F,0,0);else body.Motion(1,std::stoi(row[6]),std::stoi(row[7]));
        std::vector<std::uint8_t> geometry(9*28);
        for (std::size_t index=0;index<9;++index) { const auto& currentNode=index==0 ? root : static_cast<GumpCreateNode&>(children[index-1]);const std::array values{currentNode.scale,currentNode.baseWidth,currentNode.baseHeight,currentNode.width,currentNode.height,currentNode.x,currentNode.y};for (std::size_t field=0;field<values.size();++field) Put(geometry,index*28+field*4,std::bit_cast<std::uint32_t>(values[field])); } // 창/일곱 자식/드래그의 현재 저장 비트를 비교한다.
        std::string joined;for (const auto& item:trace) { if (!joined.empty()) joined+=';';joined+=item; } // 효과 순서도 동일해야 한다.
        const bool same=Hex(geometry)==row[11] && Hex(interaction.record)==row[12] && children[7].colorMode==Number(row[13]) && children[7].depth==Number(row[14]) && joined==row[15];
        if (!same) std::fprintf(stderr,"IslandLayout Native행%u %s 기하/순서 불일치\n",checked,row[0].c_str());CHECK(same);++checked;
    }
    CHECK(checked==184);
}
