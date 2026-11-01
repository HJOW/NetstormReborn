//실제HTML메타→동일편집/모달→후속Tell과기존공통수명을검사한다.멀티플레이는실행하지않는다.
#include "client/HtmlWindowMetadata.h"

TEST_CASE(HtmlInputDialog_MetadataActualTextAndTell) {
    SaveNameScene scene;SaveFortNameDialogContext context;context.editor.now=[] { return 0.0; };context.translate=[](auto text) { return std::string(text); };std::string told;context.tell=[&](auto section) { told=section; };HtmlWindowGumpNode node{};HtmlWindowMetadataHooks hooks;std::uint32_t token{},closed{};
    hooks.node=[&](auto root) { CHECK(root==1);return &node; };hooks.clearButtons=[](auto) {};hooks.commands.lookup=[](auto command) { CHECK(command=="1");return 0U; };hooks.commands.unknown=[](auto) {};hooks.storeText=[](auto) { return 77U; };
    hooks.input=[&](auto mode,auto title,auto continuation) { CHECK(mode==1 && title=="Prompt" && continuation=="next");token=scene.menus->ShowHtmlInputDialog(context,static_cast<std::uint32_t>(mode),title,continuation); };hooks.read=[](auto address) { CHECK(address==0x5429ec);return 0x020000beU; };hooks.queue=[&](auto code,auto first,auto second) { CHECK(code==0x020000beU && !first && !second);++closed; };
    std::array<char,8192> source{};const std::string text="$Input=Prompt,1,next \nTAIL";std::copy(text.begin(),text.end(),source.begin());CHECK(HtmlWindowMetadata(hooks).Process(1,source)==0 && std::string(source.data())=="TAIL" && closed==1 && token);
    const auto* root=scene.menus->SaveFortNameNode(token);CHECK(root && root->vtable==0x00517d00U && root->fortMode==1 && root->baseHeight==104 && scene.buttons.CurrentDialog()==token);const auto entry=root->entry;const auto* editor=scene.menus->TextEntryControls()->Node(entry);CHECK(editor && editor->maxLength==41 && !editor->multiline && !editor->excludedChars);CHECK(scene.menus->TextEntryControls()->Storage().Find(editor->buffer)->Text()=="Enter Text");
    for (const auto ch:std::string("Hello  ")) scene.buttons.RouteInput({static_cast<std::uint32_t>(ch),0,0}); //실제초기선택문자를교체하며끝공백도보존한다.
    CHECK(scene.menus->Node(root->title)->alignFlags==4 && scene.menus->Point("OK") && scene.menus->Point("Cancel"));const auto pixels=scene.Draw();CHECK(std::any_of(pixels.begin(),pixels.end(),[](auto value) { return value!=0x5a; }));scene.buttons.RouteInput({13,0,0});CHECK(told=="next(Hello  )" && (root->flags&0x10000U));scene.buttons.CleanupGumps();CHECK(!scene.buttons.CurrentDialog() && !scene.menus->SaveFortNameNode(token) && !scene.menus->TextEntryControls()->Node(entry));
}
//숫자모드는원문처럼숫자필터가없으며40문자제한과취소시후속Tell생략을유지한다.
TEST_CASE(HtmlInputDialog_ActualNumberModeLimitAndCancel) {
    SaveNameScene scene;SaveFortNameDialogContext context;context.editor.now=[] { return 0.0; };context.translate=[](auto text) { return std::string(text); };std::uint32_t tells{};context.tell=[&](auto) { ++tells; };const auto token=scene.menus->ShowHtmlInputDialog(context,0,"Number prompt","done");const auto* root=scene.menus->SaveFortNameNode(token);const auto entry=root->entry;const auto* editor=scene.menus->TextEntryControls()->Node(entry);CHECK(scene.menus->TextEntryControls()->Storage().Find(editor->buffer)->Text()=="Enter Number" && !editor->excludedChars);
    for (const auto ch:std::string("A-12 ")) scene.buttons.RouteInput({static_cast<std::uint32_t>(ch),0,0}); //임의숫자필터/정규화를넣지않았음을실제경로에서확인한다.
    CHECK(scene.menus->TextEntryControls()->Storage().Find(editor->buffer)->Text()=="A-12 ");
    for (std::uint32_t index=0;index<60;++index) scene.buttons.RouteInput({'x',0,0}); //원문length+1<41이허용하는40문자까지만쓴다.
    CHECK(scene.menus->TextEntryControls()->Storage().Find(editor->buffer)->Text().size()==40);scene.buttons.HandleGump(token,0x0200002cU,scene.menus->Node(root->cancel)->serial,0);CHECK(!tells && (root->flags&0x10000U));scene.buttons.CleanupGumps();CHECK(!scene.buttons.CurrentDialog() && !scene.menus->TextEntryControls()->Node(entry));
}
