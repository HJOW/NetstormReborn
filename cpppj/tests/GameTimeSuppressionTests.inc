// 실제원문20칸진입억제와현재Client전역getter의연결순서를확인한다.
#include "client/ClientGameplayGlobals.h"

// getter는20칸최초초기화/검색보다먼저매호출실행되며그반환만해당호출의관문으로쓴다.
TEST_CASE(Warnings_CurrentSuppressionGatePrecedesBoardEffects) {
    MessageBoardState state;state.screenWidth=640;bool current=true,changeDuringCreate{};std::uint32_t reads{},creates{},attaches{},serial{700};std::string trace; //현재외부입력과실제로관측한효과수다.
    MessageBoardHooks hooks;hooks.suppressed=[&] { ++reads;trace+='S';const auto saved=current;current=!current;return saved; }; //읽기후명시입력변경을통해같은호출에서두번조회하지않는지검출한다.
    hooks.lookup=[&](auto)->std::optional<MessageBoardRecord> { trace+='L';return std::nullopt; };hooks.kill=[&](auto) { trace+='K'; };
    hooks.create=[&](auto kind,auto player,auto parameter,auto) { ++creates;trace+='C';if (changeDuringCreate) current=true;return MessageBoardRecord{1,++serial,kind,player,parameter}; };
    hooks.width=[&](auto) { trace+='W';return 50; };hooks.attach=[&](auto,auto,auto) { trace+='A';++attaches; };
    MessageBoard board(state,hooks);const auto initial=state.serials;board.Show(17,0,true,0,"blocked");CHECK(reads==1 && !creates && !attaches && state.serials==initial && trace=="S");
    state.suppressed=true;trace.clear();changeDuringCreate=true;board.Show(17,0,true,0,"allowed");CHECK(reads==2 && creates==1 && attaches==1 && state.serials[0]==701 && trace=="SCWA" && current);
    trace.clear();board.Show(17,0,false,0,"");CHECK(reads==3 && trace=="S" && state.serials[0]==701); //원문억제는기존행삭제/검색도막고이전행을소급정리하지않는다.
    current=true;trace.clear();board.Show(11,0,true,0,"");CHECK(reads==4 && creates==2 && attaches==2 && trace=="SLCWA");
    current=true;trace.clear();board.Show(16,1,true,0,"");CHECK(reads==5 && creates==3 && attaches==3 && trace=="SLCWA"); //정지/시간종류도같은진입getter를읽은뒤예외로허용한다.
    auto failing=hooks;failing.suppressed=[]()->bool { throw std::logic_error("suppression read stop"); };MessageBoard stopped(state,std::move(failing));const auto before=state.serials;bool failed{};
    try { stopped.Show(17,0,true,0,""); }catch (const std::logic_error&) { failed=true; }CHECK(failed && state.serials==before && reads==5 && creates==3 && attaches==3);
}

// 실제WarningMessages와공통StyleText/Gump표가현재Client미션값을읽으며11/16예외와부분수명을보존한다.
TEST_CASE(Warnings_ActualCurrentMissionBlocksWarningKeepsPauseAndTime) {
    ClientGameplayGlobals game;game.world.missionPost=true;std::size_t reads{},sounds{},clockReads{},wallReads{};auto context=PauseContext(); //Session대여없이실제장기전역owner를직접빌린다.
    context.gameTimeSeconds=[&] { ++clockReads;return 7; };WarningClient client([&] { ++wallReads;return 1.25; },[&] { ++sounds; },std::move(context),640,480,[&] { ++reads;return game.world.missionPost; });
    auto& messages=*client.messages;const auto fields=client.Fields();messages.Add("~E~ySuppressed");messages.Remove(17,0,0);CHECK(reads==2 && !sounds && !wallReads && !clockReads && client.Rows().empty() && client.Fields()==fields);
    messages.PauseStatus(true);messages.GameTimeStatus();CHECK(reads==4 && !sounds && clockReads==1 && wallReads==1);const auto rows=client.Rows();CHECK(rows.size()==2 && rows[0].source=="~E~wGame Paused!" && rows[0].deadline==0.0 && rows[1].source=="~E~yGame Time~r:~w 0:00:07" && rows[1].deadline==1.35);
    messages.PauseStatus(false);messages.Remove(16,1,0);CHECK(reads==6 && client.Rows().empty());client.buttons.CleanupGumps();
    game.world.missionPost=false;messages.Add("~E~yAllowed");CHECK(reads==7 && sounds==1 && client.Rows().size()==1);const auto before=client.Rows()[0].serial;
    game.world.missionPost=true;messages.Add("~E~yBlocked later");messages.Remove(17,0,0);CHECK(reads==9 && sounds==1 && client.Rows().size()==1 && client.Rows()[0].serial==before); //공장구성시의false사본을계속쓰지않고새값을읽는다.
    game.world.missionPost=false;messages.Remove(17,0,0);CHECK(reads==10 && client.Rows().size()==1); //원문kind17은중복일치/삭제검색대상이아니므로억제해제만으로기존Warning을삭제하지않는다.
    client.buttons.KillGump(client.buttons.SerialGump(before));client.buttons.CleanupGumps();CHECK(client.Rows().empty() && client.Fields()[1]==1);
}
