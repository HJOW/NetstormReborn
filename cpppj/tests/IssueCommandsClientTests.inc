// 기존 실제 ChatScene의 장기 표/Console/공유 로그로 초기 명령과 오류 경계를 접합한다.
TEST_CASE(IssueCommands_ClientRegistryAndCheatUseCurrentSharedState) {
    ChatScene scene;ChatIdentityInputs(scene);InitializeClientIssueCommands(scene.client);std::string name="LocalUser";auto context=ChatLocalSubmitContext(scene,name);const auto submit=context.sendChat;
    const auto token=scene.menus->CreateChatWindow(std::move(context));const auto table=scene.client.DotCommandGlobals()->Bytes();CHECK(ChatDword(table,0xa04)==10);
    CHECK(ChatDword(table,0x804)==0x4d5300 && ChatDword(table,0x808)==0x4d5300 && ChatDword(table,0x80c)==0x4d5340 && ChatDword(table,0x810)==0x4d5360);
    CHECK(ChatDword(table,0x904)==kDotCommandEmptyToken && ChatDword(table,0x908)==kDotCommandEmptyToken && ChatDword(table,0x910)==kDotCommandNamesToken+128);
    scene.client.Configuration().SetInt("userCheat",17);scene.client.GameplayGlobals()->energy.userCheat=17; //설정 변경의 원문 허용값 정규화 뒤 독립된 현재 DWORD 입력을 제공한다.
    submit(scene.client.ChatManagerGlobals()->Current(),"EVERYONE:.cheatorama wrong");CHECK(scene.client.ScriptMenuGlobal(0x5cab30)==17);
    submit(scene.client.ChatManagerGlobals()->Current(),"EVERYONE:.cheatorama \"8675309\"");CHECK(scene.client.ScriptMenuGlobal(0x5cab30)==0x845fed);
    CHECK(scene.client.Configuration().GetInt("userCheat")==17 && scene.client.GameplayGlobals()->world.editor==0 && !scene.client.GameplayGlobals()->player.special);
    CHECK(scene.menus->ChatWindowRoot()==token);scene.menus->CloseChatWindows();
}
// PlayerKnown0에서는 화면/부모를 조회하지 않아도 실제 음향 요청까지 계속한다.
TEST_CASE(IssueCommands_ClientFlashNaturalNoPlayerGate) {
    ChatScene scene;ChatIdentityInputs(scene);scene.client.InitializeNativeAudio();InitializeClientIssueCommands(scene.client);std::string name="LocalUser";auto context=ChatLocalSubmitContext(scene,name);const auto submit=context.sendChat;
    const auto token=scene.menus->CreateChatWindow(std::move(context));CHECK(!scene.client.GameplayGlobals()->player.playerKnown);
    submit(scene.client.ChatManagerGlobals()->Current(),"EVERYONE:.flash -17");CHECK(scene.client.ThunderFlashCount()==0 && scene.menus->ChatWindowRoot()==token);scene.menus->CloseChatWindows();
}
// 비영 하위 결과는 현재 Chat에 원문 소문자 hex 오류를 출력하며 현재0은 자연 반환한다.
TEST_CASE(IssueCommands_ClientCommandErrorUsesActualChatLog) {
    ChatScene scene;scene.menus->PrintChat("before root\n");CHECK(!scene.client.ChatWindowGlobals()->log.initialized);
    ChatIdentityInputs(scene);std::string name="LocalUser";auto context=ChatLocalSubmitContext(scene,name);unsigned messages{};
    context.message=[&](auto) { ++messages;return 0xfeedU; };BindClientChatSubmit(context,scene.client,*scene.menus);const auto submit=context.sendChat;
    scene.client.ChatWindowGlobals()->logFile=123;const auto token=scene.menus->CreateChatWindow(std::move(context));submit(scene.client.ChatManagerGlobals()->Current(),"EVERYONE:ordinary message");
    CHECK(messages==1 && scene.menus->ChatWindowRoot()==token);
    CHECK(std::any_of(scene.writes.begin(),scene.writes.end(),[](const auto& value) { return value.find("Error in command \".say\": code == feed\n")!=std::string::npos; }));
    scene.menus->CloseChatWindows();const auto writes=scene.writes.size();scene.menus->PrintChat("after root\n");CHECK(scene.writes.size()==writes);
}
