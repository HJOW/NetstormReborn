# 기존 Editor checkpoint 이후의 UI와 State 반환

2026-10-24 **읽기 분석·후속 연결 인계**다. AGENTS/공통·닷넷 인계 상단을 확인했고 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 보호10.78 PE/기존 정밀 C와 현재 관련API를 읽었으며 새 원문 실행·게임·GUI·빌드·제품/CMake/LEFT/git 변경은 없다. 신규 산출물은 이 문서 한 개다.

[앞선 외곽](cpp-existing-editor-install-outer-20261024.md)의004b743b checkpoint 이후만 다룬다. 앞선7파일/tool/fixture/report는 그대로이며 해당 문서 SHA256 `6e53ff744c4c980df439d320d4b7a84044620be36e9f27b99198b7761285303d`도 보존한다. 기존 fixture258은이tail을실행하지않았으므로아래는 **PE 명령/정밀 C의 정적확인**이며새 동적검증 결과가아니다.

## 정상 기존 선택1/2의 외곽 순서

진입 context는앞선초기prefix가유지한 **ESI=1/EBP=0**이며004904b0과끝맵후처리까지복귀했다는것이다. 아래각UI/음향/palette하위는현재State요청이나다른전역을바꿀수있으므로분기직전원문의현재값재조회를유지한다. 현재요청은005c87e8,확정current는005c8758,next는005c8878,즉시반복work는005c86c8로구별한다.

| 위치 | 실행/인자와자연관문 |
|---|---|
|004b743b~004b7444|현재명령ID005427ec를읽어 **00466280(id,1,0,1)**.초기ID는0200003d(HUD mode)다.상위에Editor관문은없다.|
|004b7449|004180d0:00460d70을그시점호출하고반환double에상수00501450=**10.0초**를더해00544d58에쓴다.분기없이실행한다.|
|004b744e~004b7453|00469db0("ser22.mus").상위는항상요청하되실제하위의같은곡/결과곡보호관문을유지한다.|
|004b7458~004b7463|음악요청복귀뒤 **현재요청state005c87e8를재조회**.6이면BattleOver하위분기004b7465~7499이고,정상기존1/2는004b749e다.|
|004b749e~004b74ac|004410f0("fortPal",0,0,0)의반환문자열을004b6100에전달한다.모드가Editor여도이호출은생략하지않는다.|
|004b74b1~004b74c6|palette부모복귀뒤 **현재요청state를재조회**하고 **nextState005c8878=4를무조건덮어쓴다**.재조회한요청이3이면이후Add Island예약을생략해004b86bd로간다.|
|004b74cc~004b74d8|0046ee50(&00568b60)의현재첫DWORD를읽는다.이는 **실제IslandList의섬개수**다.비영이면Add Island예약을생략한다.미니맵/Gump수나별도bool이아니다.|
|004b74de~004b74ec|섬개수0이면 **현재명령ID00542780**을읽어00452fe0(id,0,0)으로원래40칸입력큐에예약한다.초기ID02000022는 **Add Island**다.즉시섬생성이나대화창동기실행을하지않고004b86ba에서cdecl스택12BYTE를정리한다.|
|004b86bd~004b8748|아래State기록/즉시반복관문뒤SEH레코드·보존레지스터·frame을복원하고정상ret.004b8708의미션조기반환경로와구별한다.|

**시각 정정:**00460d70은timeGetTime을매호출읽는벽시각초다(`Netstorm.c:78590~78607`).정지가능한게임진행시각/프레임에고정한Time()으로치환하지않는다.앞선문서에서이를현재게임시각이라부른표현은이근거로구별한다.현재 `Client::WallSeconds()`가같은종류의조회이며 `RawLocalMissionStateGlobals::activityEnd`/실제Login의activityEnd는같은00544d58소비경계다. Editor만의복제타이머를임의생성하지않고Client/State수명의같은double을공유해야한다.

## HUD 분배는 숨은 자식도 포함한다

00466280은00466200으로네인자를그대로보내는래퍼다. 실제00466200은다음순서를갖는다(`Netstorm.c:83187~83218`).

1. 현재캡처00565dd0가비영이면필터/트리검색없이그객체의가상+28에 `(id,1,0)`을전달한다.실패하면다른대상으로재전달하지않고0반환한다.성공하면 **callback뒤현재캡처를다시읽고그전역을0으로쓴다**.처음캡처의캐시를반환하지않는다.
2. 캡처0이면현재dialog00565dd4가비영일때그대상,아니면현재root00563e38을선택한다.둘다0이면자연반환이다.실제선택대상에서004651b0(event,1,0,**includeHidden1**)을실행한다.
3.004651b0은자식우선재귀이며includeHidden1은자식visible관문을우회한다.다음형제주소는각단계현재필드에서읽고,자식이없으면자신의가상+34필터뒤가상+28처리기를호출한다.성공처리기의전체반환은그대상Gump토큰으로바뀐다.단순전체트리Broadcast와다르다.외곽은그반환값을사용하지않아도캡처해제/하위효과는보존한다.

현재제품의 `GumpInput::Route(id,first,second,includeHidden)`가이부모다. `RawButtonControls::RouteInput(InputEvent)`는includeHidden기본false를써이호출의완전한어댑터가아니다. `DispatchGump(root,...)`는실제캡처/dialog우선관문을생략하므로그대로대신쓰지않는다. `RefreshProduction()`의기존방식처럼같은실제GumpInput에 `(현재HudModeId,1,0,true)`를보내는명시API/효과가필요하다.생산갱신사건02000003과HUD mode0200003d도구별한다.

**현재loaded연결의구체적경계:** `RawProductionControls.cpp`의HUD mode처리기는두grid를현재TechnologyParent에다시Bind한후first==1이면 `session.SynchronizeProductionContents()`를호출한다. 이공개메서드는현재!loaded를거부한다.생산패널생성자도HUD mode1처리기를호출한다. 최종loaded를이tail정상완료뒤에설정하려면이미실제World/Deck가끝난설치중에도 **같은productionSync본문**을호출할수있는private본문/설치단계경계가필요하다.공개loaded계약을전부풀거나,loaded를tail앞에쓰거나,HUD통지를빈소비자로채우는방법은정확한연결이아니다.해당UI가아직없으면원래트리의미처리반환을유지하고,실제UI가있는경우의본문을구별한다.

## 음악·palette의 자체 관문과 다음 하위

00469db0(327BYTE)은현재곡0054dd60과대소문자무시비교가같으면아무일도하지않는다.다르면00469d30이현재fanfare/defeat곡의보호종료시각을확인한다.보호기간중새곡이anticipation이아니면반환하므로 **ser22요청도실제전환없이끝날수있다**.그외00435200을통해음악옵션/실제선택을수행하고현재곡길이와벽시각으로songEnd00565e68을갱신한다. 현재 `ClientAudio::RequestMusic("ser22.mus")`가이부모에대응한다. `StartScene(false)`는별도00469fc0효과를추가하므로이번요청을그것으로치환하지않는다.

004b6100(114BYTE)은항상29번`init palette\n`로그후입력이름과현재palette이름005c8910을stricmp한다.같으면로그외자연반환이다.다르면입력이름주소를ESI에유지하고다음순서로진행한다.

1. 현재palette이름005c8910에NUL포함입력이름복사→00470c30(현재00569ffc가비영이면그할당free후0).
2.00471190(name):현재대상이름0056b7a0복사→현재00569ffc가0이면00471040으로생성→현재00540a80횟수만큼00470b20.
3. **PathSpec004410f0("GamePalSpec",name,0,0)→004a4850(path,1)**.PE004b6148의push1은PathSpec의다섯번째인자가아니라그뒤palette로더의두번째인자다.정밀C의스택추적표시만따라5인자PathSpec으로조립하지않는다.
4.0043c010→004973c0.후자는현재0059a8bc가비영일때paletteDirty0059a8b0=1만쓴다.004b6100의끝은이함수로tail jump하며그하위의ret가원래004b74b1로돌아온다.

이하 계산 본체는 기존 구현을 재사용한다. 남은 일은 004b6100 부모의 실제 연결과 Editor의 arg2=1 차이 대조다. 최초 읽기 때 두 함수 전체를 미복원으로 표현한 것은 범위가 과도해 현재 구현을 대조한 뒤 교정했다.

| 다음 하위 | 실제 크기/근거 | 필요한 실제 범위 |
|---|---|---|
|004a4850|1768BYTE,불연속004a4850~48ac/004a48b0~4a4f3a,정밀C130556이후|Screen::LoadPalette에 COL/BGRX·예약BYTE·전체적용·58색/날씨표·명암 두 표가 구현돼 있다. 현재 함수는 원문 arg2=0 경로이며 Editor의 arg2=1 적용 전 화면 정리·장치 경계가 미대조다.|
|0043c010|1232BYTE,0043c010~0043c4df,정밀C49195이후|PaletteShade/PreviewColorMaps/SpriteColorMaps/ProductionColorMaps에 여러 출력 표가 분산 구현돼 있다. 전체 재작성 대상으로 보지 않으며 기존 계산 본체의 현재 호출 순서와 소비자를 연결한다.|

현재 `Client::LoadScenePalette(name)`는 DataDir경로→Screen::LoadPalette→음향 색/소프트웨어커서→UberGump::PaletteChanged→World/커서/Factory·생산·미니맵 전파와 색표 재계산을 연결한다. 이 몸체는 재사용한다. 별도로 현재 palette 이름005c8910의 비교/NUL 복사/같은 이름 자연 반환, Kencloud 구름 캐시 수명, 실제 GamePalSpec(name), arg2=1 화면 장치 효과, 조건부 paletteDirty의 순서를 연결해야 한다. 기존 메서드 호출만으로004b6100 전체 완료라고 표현하지 않는다. Screen::Clear도 주석상 일부 원문 화면 효과를 생략하므로 그것만 추가해 완료로 판정하지 않는다. 초기Client생성의GamePalSpec(fortPal)도 현재 전환의 동일성 근거를 대신하지 않는다.

후속 PE 정적 조사에서00569ffc/00471040/00470b20의 대상은 512×512 Kencloud 구름 시뮬레이션으로 확인했다. 이를 ColorSort 캐시로 부른 초안의 의미 해석은 교정한다. 00470bb0의 CloudFileSpec 이미지/실패시0x40000 할당, 현재 cloud색00540a34, 행 갱신과 cloudSpeed00540a80은 PaletteShade의 색검색 ColorSort와 별개다. 앞서 동결한 주소·호출 순서 자료를 고치거나 색검색 객체로 구름 계산을 대체하지 않는다.

## Add Island 예약의 정확한 경계

00568b60은 `RawIslandRuntime`/`ChunkMap`이같이읽는실제IslandList count다. 현재카운트0의판정후 **현재00542780**으로예약한다. 초기명령02000022는원문Edit메뉴`Netstorm.c:161324~161325`의Add Island와동일하다.

00452fe0은 `(id&0x0100ffff)|0x02000000`,first0,second0을현재40칸고리에기록하고tail을증가시킨다.한칸충돌이면원래tail되돌림관문을보존한다.현재 `InputQueue`의원문예약primitive와같은고리를사용하며 `State::Post`의선착순UI전환큐로바꾸지않는다.클릭없이빈섬을즉시추가하지않고,정상ret후입력소비때대화창을연다.

그명령의실제소비자는 `Netstorm.c:169243~169248`의 **00480090→반환객체가상+20(0)→00466600**이다.이하실제Add Island편집창/입력/Placement는현재tail의다음하위다.현재 `GameMenuCommands`는00542780→02000022매핑을갖지만,매핑이나예약만으로창/사용자섬추가가완료되었다고하지않는다.실제기존맵이이미섬을가지면예약장치/창하위는이번설치에서자연미선택이다.

## State 기록·ret와 loaded 완료 자리

004b86bd의004437f0은functions.tsv/PE기준 **1BYTE ret**다.예전trace의POLL이라는레이블과달리이판본실제본문에OS펌프/추가UI효과는없다.

004b86c2는 **복사전에현재work005c86c8첫DWORD와0을비교**해ZF에보관한다.그다음다음모든작업은mov/rep movsd로ZF를바꾸지않는다.

1.004b86c8~86d7:확정current005c8758의144BYTE→history005c8630.
2.004b86d9~86e8:현재진행요청005c87e8의144BYTE→확정current005c8758.
3.004b86ea:진행요청005c87e8첫DWORD0.요청의나머지140BYTE는보존한다.
4.004b86f0:앞서캡처한work0조건이면004b8733으로간다.비영이면work144BYTE→진행요청144BYTE를복사한뒤004b6df7로돌아가 **같은큰State함수안에서다음상태를실행**한다.분기시work를다시읽거나next와혼동하지않는다.
5.004b8733~004b8748:프레임에보존된이전FS exception주소복원→보존EDI/ESI/EBP/EBX와원래frame복원→ret.004b8708의현재Mission에따른다른조기ret는이번tail과별도다.

정상기존요청1/2·work0이면ret뒤 **확정current=1/2,진행요청첫DWORD0,next=4**다.next4는이함수에서즉시소비하지않는다.상위004b88c0의다음호출이next비영을확인해next144BYTE→진행요청144BYTE,next첫DWORD0후004b6dd0을다시부른다.그부모의Metamap관련두전환관문005c85dc/5c85d8은Editor고정경로와구별한다.현재로더가임의로next4까지동기드레인하여확정current4를만든뒤반환하는것은이정상ret와다르다.

현재 `RawLocalMissionState::Run`은같은다섯144BYTE블록과공통복사/즉시반복형태를이미사용하지만Step의Editor1/2와이번UI/palettetail이완료된것은아니다.이기존전체Run에Editor상태를강제투입해미연결분기를우회하지않는다. 공통블록의실제주소/Client수명을공유하는선택외곽연결이필요하다.

따라서Session의최종loaded는 **앞선외곽전체→위HUD/현재벽시각deadline/음악요청/palette부모/조건부Add Island예약/실제State기록과정상ret에해당하는완료** 뒤에확정한다.먼저loaded를켜필수UI나productionSync관문을통과시키지않는다.중간장치/본문오류는앞선부분효과를보존하고loadedfalse를유지한다.work의새요청이들어왔다면전체State분배기가그다음상태를마칠때까지선택Editor만의완료로고정하지않는다.그다음루프의next4소비와사용자대화/섬배치는별도효과다.

이번인계의실제미완료필수경계는 **includeHidden1 HUD통지와설치중productionSync의동일본문,Client수명의deadline,현재음악부모,이름/실제표/장치를공유한palette부모,현재섬count의조건부실제입력예약,같은State144BYTE수명/즉시반복/ret완료**다.단순표시갱신이나한개bool로합쳐성공처리하지않는다. palette 전환 부모/arg2=1 장치 경계와 Add Island창은 위 주소에서 후속으로 남기며, 이 문서는 GUI/사용자 실제 저장 완료 판정이 아니다.
