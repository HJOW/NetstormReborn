# 기존 Fort Editor의 선택 설치 외곽

2026-10-24 **단위383 후속·신규 독립 준비·공용 미등록**이다. AGENTS/공통·닷넷 인계 상단을 재확인했고 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 기존 C/PE를 읽었으며 원본/CD/AGENTS/정밀 산출물·기존 제품·CMake·LEFT·공유빌드·git은 변경하지 않았다. 게임/GUI/원본 OS 실행은 없고 신규 격리 콘솔만 실행했다.

핵심 순서는 **파일 설치 안의 State|1/Notify→Subscriber/handle 설정/제목→Player/World→Deck/카메라/생산→전체Snapshot→State&fe/Notify→startupStateCheck→현재 Editor일 때 lastEditFort/EditorPlayers**다. Snapshot 뒤004b5ba0은 Subscriber가 아니다. 앞선 [generic 설계](cpp-generic-editor-install-20261024.md)와 [Client 설계](cpp-editor-client-next-20261023.md)의 해당 요약은 이 근거를 우선한다. Subscriber는004b5ca0의 정상 꼬리004b5e80에서 이미 읽는다.

## 최초 PE 관측과 한계

[도구](../../tools/decomp_existing_editor_install_outer.py)로 제품 외곽 작성 **전에** [fixture](../../cpppj/tests/fixtures/existing-editor-install-outer-x86.tsv)/[보고서](../../cpppj/existing-editor-install-outer-native.json)를 고정했다. 전체함수004b6dd0의 실제 진입/prologue에서 시작하되 **004b743b,즉 명시 EditorPlayers 하위가 복귀한 뒤·후속 UI 전**에서 멈춘다. 이 checkpoint를 전체004b6dd0의 정상ret나 최종 loaded라고 표현하지 않는다.

| 항목 | 결과 |
|---|---|
|고정 context|진행 요청 State1/동일case2,이전 상태4,Battle0,명시cleanup/비영manager/기존 파일 경로|
|입력/관측|129입력×027f/037f 두정밀도=258관측,정밀도차이0|
|완료 분포|선택하위 복귀 checkpoint202,다른 경로 도달10,명시 하위/물리 중단46|
|실제 명령|86,696개|
|포함 원문|004b6dd0의 선택 범위,정상 기존파일004b5ca0,004bd340의31BYTE,실제startup004b5ba0의관문·경과분 비교·Tell분기|
|최초 fixture SHA256|`eeb11836fe3ddbbb0228c11f40bfdd8461bf0a45221ebb225667aa86f6a51f4a`|
|보호 PE SHA256|`a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`|

실제 State144BYTE 두번 복사/모드 쓰기/기존파일 분기/State bit0의 설정·해제/현재 owner·이름 재조회/startup x87 분기는 PE가 실행한다. cleanup/CRT비교/파일/Guide/Player·World 등 큰 하위/State getter·Notify/Subscriber/시계·저장상태읽기004b5890/번역·format·Tell/EditorPlayers는 **명시 입력 경계**다. 명시 format은 `ExplicitButtons`/`ExplicitWarning`을 반환하므로 실제 번역 문자열·서식기·창의 완료 검사가 아니다. 파일 없음/유효성 실패/빈·점이름/현재 Mission은 범위 밖 첫 명령 직전에 별도 표시하며 NewFort·삭제·미션으로 들어가지 않는다.

원문0048f3b0(1,1)도 이 fixture에서는 명시 하위다. 장치가 입력 owner1/8,battleOwner7을 유지시켜 후속 재조회를 구별하며, **실제 setter가 owner8/7을 설정한다는 뜻이 아니다.** 실제0048f3b0은 두 owner를1로 쓰고 PlayerKnown00595344와 현재 CoreData 주소도 갱신한다. 실제 Client 기본 진입은 두 owner1이며 callback 뒤의 현재값 변경을 별도로 반영해야 한다. 전체cleanup/owner setter/Player 본문의 복원 검사가 아니다.

최초 실행 전 탐색에서는 누락된565e70 첫BYTE 쓰기를 좁은 CPU쓰기 guard가 거부했다. 그 범위를 추가한 뒤 최초 fixture를 생성했으며 최초 생성 이후 tool/fixture/report는 수정하지 않았다. `--verify`가 PE/C/functions/helper/tool/fixture SHA를 확인한다.

**최초 관측 정오표:** 부가 `INSTALL:alternate-before=805433324`는 call명령004b7355에서 함수진입형 `ESP+4` 해석을 사용해 실제 인자 대신 인접 스택의 포인터값을 읽은 계측 오류다. CPU실행과 실제 `WORLD:0` 인자는 영향을 받지 않았다. 해당 최초 자료를 재생성하거나 고치지 않고, 이 debug항목만 C++ 대조에서 명시 제외했다. alternate0은 실제004b72f8의 지역DWORD0 쓰기와004b7392~7397의 전달 인자로 별도 확인한다. `CLOCKRESET`/`PRESERVE` 같은 trace 이름도 관측용 레이블이며 하위의 전체 의미를 확정한 명칭이 아니다.

## 실제 선택 상태와 생성 전 prefix

정밀 C `Netstorm.c:145526~145805`와 PE를 대조했다. AskEditFort004d7408은 **목록 mode005cae84=2**를 쓰지만,선택 ReadFort→004b3560은 **nextState005c8878=1**이다. 현재 [FortSelection](../../cpppj/src/client/FortSelection.cpp)도 이 차이를 보존한다. mode2를 진행 State2로 해석하지 않는다. State2는 같은 switch case에 들어가는 별도 대조 입력이다.

| 원문 위치 | 정확한 효과/인자 |
|---|---|
|004b6dfc~004b6e5b|Poll004437f0→현재 요청005c87e8의144BYTE를next005c8878/work005c86c8에 각각 복사→두 복사본의첫DWORD0→trace→**현재 요청 state를 다시 읽어**switch한다.전체State전환은선착순UI큐로치환하지않는다.|
|004b7169~004b7177|현재 요청3/6이면편집분기0,그외 이case1/2는편집분기1이다.이전Battle 알림은별도004b7177~71a5이며고정context Battle0에서는자연생략한다.|
|004b71a5~004b71e1|004b2df0(0)→0042e440→004e22c0→Fort00594fb8=1→편집분기면Editor005c85a4=1/missionPost00594fa4=1/server00542640=ffffffff/fullDeck0054db14=1.첫객체보다앞이다.|
|004b71e7~004b7219|00540bc0/00540bc4=1→0048f3b0(1,1)→현재0059a8bc에004993c0(1)→00432360(&0054c8a0)→0054c8a0첫DWORD1→0041cc10.두owner와PlayerKnown/CoreData도같은실제저장소를쓴다.|
|004b721e~004b723e|00594fc4=0→00466280(현재005427ec,1,0,1)→**현재 요청 state재조회**.3이면다른분기,기존선택1/2는004b72eb다.|

**0055a480(playerState.special)**은 PE004b6dd0~004b743b의 직접 명령에서 읽거나 쓰지 않는다. 편집 prefix도 이를1로 설정하지 않는다. 전체 정밀 C의 직접0 대입은 옵션 해석00435220의 `Netstorm.c:43309`이며 다음0055a484도0으로 쓴다. 공통cleanup004b2df0 본문에도 해당 직접 대입은 없다. 현재 Session의 ExistingEditor가 special=false를 유지하는 것과 맞고,Editor라는 이유로1을 추가할 근거는 없다. 옵션 callback을 포함한 모든 간접 전이를 복원했다는 주장은 아니다.

## 선택 본문과 파일 설치

| 위치 | 원문 순서/현재 조회 |
|---|---|
|004b72eb~004b7338|비교 시점의current/pending으로stricmp.다르면현재Mission을읽고,비영일때현재pending의00460c40이0이면Guide00463b20(00562ca0).복귀뒤**현재pending**으로00460980을호출한다.|
|004b733b~004b7364|**현재curFort 첫BYTE**가0/점이면00460c20경로.그외 지역alternate0주소를004b5ca0에전달한뒤**현재Mission 재조회**.비영이면004b8708의미션설치후반환이어서현재일반경로는진행하지않는다.|
|004b5ca0~004b5ce6|현재경로00460680→0041ab10존재→현재경로재조회→**현재owner**004bf550→**현재owner**0041aad0유효성.존재0/유효0은현재모듈의다른경로다.|
|004b5d57~004b5e23|현재owner의004bd340은현재manager0이면Template.cpp410진단후에도**항상0반환**한다.그래서정상기존파일에서도**현재owner State첫BYTE|1**→**현재owner Notify**가실행된다.다른header의isMine을읽어이단계를생략하지않는다.|
|004b5e80~004b5eb3|현재handle주소/현재owner로004bd450(this,&00540c74,handle)→**현재handle**을`fort.mySubHandle`에설정→00460730제목.등록ID/handle은World보다먼저실제파일에서읽는다.|
|004b736a~004b7397|현재battleOwner00490250→현재templateOwner004beca0(0)→004b5ec0회복→World004b5f70(alternate0).기존Session.LoadExistingEditorWorld의한번실행단계와같다.|
|004b739c~004b73e5|**현재owner**Deck004bf190→사제004923e0→카메라004340d0→생산0047d890→**현재owner**CoreData쓰기004beb30→**현재owner**전체Snapshot004bf9a0.|
|004b73ea~004b7415|**현재owner**State004bd550반환주소의첫BYTE&fe→**현재owner**Notify004bd540→startupStateCheck004b5ba0.서로사이의callback변경을한슬롯으로캐시하지않는다.|
|004b741a~004b7436|**현재Editor**가비영일때만**현재curFort**로lastEditFort설정→004904b0과끝0046e980.이후004b743b에서이번독립관측을멈춘다.|

사제004923e0의 자연 관문은 **missionPost00594fa4==0일때만 본문**이다. CoreData읽기004beca0은 missionPost/Editor 모두0일때만 본문이다. 그러나 CoreData쓰기004beb30은 **manager0 진단1119와Metamap 진단1120을 먼저** 실행한 뒤 missionPost/Editor 관문으로 기록을 생략한다. 상위가Editor라고이호출을빈성공으로치환하면진단을잃는다. 생산0047d890은00466200(현재00542708,0,0,1)에연결되는 실제UI부모다.

## startupStateCheck와 후속 UI

004b5ba0에는 **Editor/missionPost 자연 생략 관문이 없다.** 현재 `ReadInt("startupStateCheck")`가0이면즉시0반환한다. 비영이면004b5890이저장상태파일을읽은뒤현재게임시각00460d70을읽고,`(현재double시각-signed저장시각)/60`을float로저장한다. 저장시각비영이고 **0<경과분<45**일때Yes/No번역→StateReconnect/DelStateFile버튼format→안내format→004cf550(text,0,0)을호출하고1을반환한다. 경계0/45분은안내를띄우지않는다. 외곽은그반환값을무시하고현재Editor를다시읽는다.

명시하위004b5890의 실제 C(`144364~144493`)는state파일의서명/이름/현재pending/등록ID/Player이름·flags·기술값/현재기술획득상태등을바꿀수있다. Editor가설정된상태에서도이부수효과가허용되는원문이다. 신규외곽API는이전체를 **필수외부경계**로남긴다. 실제현재설정0을읽은경우의자연생략만허용하며,미복원이라는이유로설정을0으로고정하거나모든호출을빈성공으로반환하지않는다. 최초PE258에는옵션0/비영+저장시각0/10분/0분/45분과저장파일하위가현재pending/owner를바꾸는명시입력도포함된다.

다음은이번checkpoint이후의 **정적확인**이며제품/동적검사완료가아니다.

1. 004b743b~7453:현재005427ec로00466280(...,1,0,1)→004180d0(현재게임시각+00501450을00544d58에기록)→`ser22.mus`요청00469db0.
2. 004b7458~74b6:현재요청state6의BattleOver분기를구별한다.기존선택1/2는PathSpec004410f0("fortPal",0,0,0)→004b6100(palette).이부모는현재palette명대소문자비교/필요시실제그래픽재설정과GamePalSpec을수행한다.
3. 004b74b9~74e7:nextState005c8878=4를쓴뒤현재요청3이아니고0046ee50(&00568b60)==0일때00452fe0(현재00542780,0,0)로원래40칸입력큐에명령을넣는다.0046ee50은그객체첫DWORD읽기다.
4. 004b86bd~004b8748:다시Poll→이전current144BYTE를history에복사→진행요청144BYTE를current로복사→요청첫DWORD0.**work005c86c8**가비영이면그요청을다시진행영역에복사해함수안에서반복한다.보통next4만보관된경우이함수는여기서ret하고다음루프가next를소비한다.전체전환을UI큐한개소비와같다고가정하지않는다.

## 신규 API와 후속 Session 연결

[RawExistingEditorInstallOuter.h](../../cpppj/src/o/RawExistingEditorInstallOuter.h)/[구현](../../cpppj/src/o/RawExistingEditorInstallOuter.cpp)은두함수다.

```cpp
InstallExistingEditorFile(fileHooks); // 정상기존파일/State|1/Notify/Subscriber/config/title
InstallExistingEditorFort(fortHooks); // 004b72eb선택본문부터004b743b부분checkpoint
```

`fortHooks.file`은첫함수에,`world`는현재Session.LoadExistingEditorWorld에한번연결한다. 따라서Player/회복/World를외곽에서중복생성하지않는다. `snapshot`은이미분리한private SnapshotFortBody에연결하고,State는같은RawPlayerTemplateState.Get/Notify를쓴다. `deck`은현재변환표의실제Deck부모,`editorPlayers`는실제InitializeEditorPlayers와현재Pool의FinishEditorMap을연결한다. `camera`는현재카메라소비자,`production`은실제등록UI장치를요구한다. `startupStateCheck`는위실제관문/부수효과의필수부모다. 현재owner/Editor/이름getter는순수현재조회이며장치별복제값을소유하지않는다.

이름/path/handle의string_view는해당동기하위가끝날때까지실제소유문자열이유지되어야한다. getter가임시string을만들어그view를반환하지않는다. State도반환후첫BYTE쓰기까지같은할당수명을유지하며,그span을모듈밖의더긴수명에보관하지않는다.

NewName/NewFort/InvalidFile/Mission은`ExistingEditorInstallOutside`로범위밖첫위치에서인계한다. hook가예외를던지면같은예외/부분효과를보존하고,hook가복귀해도이한정모듈은logic_error로거부한다. 다른분기의정상완료·삭제·새Fort를실행하지않는다. 없는선택경계는도달위치에서logic_error,State반환물리가0BYTE면out_of_range다. 앞선파일/State/설정/Snapshot효과는rollback하지않고모듈은어떤loaded값도쓰지않는다.

첫통합단위는이두부모를 **이미명시prefix가끝난기존Fort작업본**에연결하는것이다. CurrentName/Guide/파일진입은Client수명의현재이름/등록ID/handle을공유하고,원본경로를직접저장대상으로만들지않고EditorFiles작업본을먼저준비한다. 실제File.Save/CurrentName의삭제요청도같은보호작업루트로만연결한다. Session의editorWorldStarted/Ready와최종loaded는별개이며,외곽도중실패한같은Session을자동재설치하지않는다. privateSnapshot/State/EditorPlayers뒤host입력·프레임소비자를붙이는완료부모가필요하다. visibleClient전체완료는위후속UI/State순환도연결한뒤판정한다.

## C++ 검증과 검증 범위

[ExistingEditorInstallOuterTests.cpp](../../cpppj/tests/ExistingEditorInstallOuterTests.cpp)는최초258중 **선택본문진입·startup옵션0인148관측**을대조한다(부분checkpoint98/다른경로10/부분중단40). 초반poll/cleanup중단과미복원startup비영본문 **110관측은분석자료로만보존**한다. C++가초기prefix/004b5890·Tell를복원했다고표현하지않는다.

입력열만으로명시prefix완료context를만든뒤선택본문current8필드/전체9헤더·9State·현재이름·pending·handle/설정/COMPARE이후trace를대조한다. 초기요청144BYTE복사는C++모듈범위에서제외하고,앞서설명한부가INSTALL계측오류1항목도명시제외한다. 모든다른선택trace와원시는정규화하지않는다. World는현재Session과같은한개하위경계이며그안의네child trace는명시장치입력이다.

격리경로는`extracted/existing-editor-install-outer-20261024-isolated`다. `/std:c++20 /EHsc /utf-8 /W4`빌드1은종료0/관측경고0이었고,최초test1은부가INSTALL항목때문에trace142불일치였다. 원문자료를고치지않고대조범위를위와같이바꿨다. test2는빌드2완료확인전에이전실행파일을실수로실행한로그여서검증무효로보존한다. **빌드2의실제종료0/관측경고0을확인한뒤test3은148대조·110제외,2검사/실패0**이다. 선택된startup미연결이앞선Snapshot/State/Notify를보존하고lastEditFort를쓰지않는것,범위밖hook의빈복귀가정상성공으로이어지지않는것도별도검사했다.

이후실제통합검사는원본유효Fort를EditorFiles작업본으로읽어첫State|1/Notify/Subscriber가World앞인것,현재owner변경의각재조회,실제Deck/생산UI/privateSnapshot/State/Notify/현재이름/EditorPlayer맵후처리를관측해야한다. 저장파일의재파싱·별도Editor Session재로드와실제메뉴/ESC/Return·사용자변경/SaveAs는각기별도완료기준이다. 이번자료는전체거대한004b6dd0/NewFort/최종GUI완료나사용자완주판정이아니다.10.72대응은기존정밀C의State0042d630/파일부모00432e30/startup00432d40표기만기록하며이번독립실행은10.78이다.
