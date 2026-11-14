# dotnetpj 0.544 — 현재 Create 부모와 실제 편의 생성

2026-11-15,HJOW-Athlon. 직전0.543은b87a294ab다. 근거는cpppj o/RawSquidCreation.h/.cpp e6259e9755b33d28da9060d0d51aa986d20bb7b1, o/SquidFactory.h/.cpp be567d46b5eca30d2a1421af78d49125add1ff6a, o/RawGameAuxState.h 8defb347c7c894599f1c9ebf1ad7d55c98f35df0, client/RawSurfaceStorage.cpp 14fd017c3b6ca87880374e044e3ac00bf0575c2e다. 원본/디컴파일은 변경하지 않았다.

## 원문 부모와 실제 공장

RawSquidCreation은 PATCH의 현재fakeSurface TYPE→복귀진단→실제할당→signed SID/예측경계 진단→현재전체TYPE 생성자→실제반환객체→저장VT→TYPE BYTE→저장VT+28 순서를 보존한다. CD는 fakeSurface 조회를 요구하지 않는다. 전체TYPE/flags와 실제객체DWORD는 좁히지 않으며 선택하지 않은 장치를 미리 요구하지 않는다. 실패 앞의 할당/상태를 취소하지 않는다.

실제Factory.Create는 같은 부모를 사용하고 생성자를 할당 전에 고정하던 선검사를 제거했다. 같은Pool의 실제Allocate/현재TYPE/ConstructRaw/실제VT 후처리를 연결한다. 네Form의 실제VT는 빈+28, 지원 자산VT는 현재PostCreate를 사용한다. TYPE와 실제클래스의 쌍 일치를 요구하지 않는다. 미등록 가상분배는 도달 위치에서 명시 실패한다.

CreateContained/CreateGumpForm은 고정Form 생성자를 직접 쓰던 경로를 각각현재TYPE6/3의 일반Create로 바꿨다. 실제TYPE의 생성자가 다른Form을 반환하면 그실제VT의 빈후처리를 사용한다. 기존 직접RawSquidForms 편의경로 전체를 동시에 바꾸지는 않았다.

GameAux의같은005412e0 FakeSurfaceTypeBox를 처음162로 만들고 일반준비/World교체에서 유지한다. 현재World의공장은 그장기주소를 빌린다. C#은아직World별Factory이므로 이번연결을cpp NativeStorage의장기공장/전체World소유자 완료로 세지 않는다.

## 입력과 회귀

squid-creation-parent-x86.tsv의PATCH294+CD294=588개 원본관측에서전체추적/진단·실제반환객체·저장VT와payload를대조했다. 원본자식/할당 경계는같은명시장치이며실제자식구현을실행한것으로세지않는다. 독립CD버퍼는36BYTE이고PATCH는50BYTE다. 기대값은바꾸지않았다.

추가5검사는선택장치누락/실제반환객체VT의부분효과, fake진단뒤현재생성자, 실제Allocate 콜백뒤생성자/실패SID보존, 양판본실제Form·자산별칭·Contained/Gump현재생성자, 실제World들이같은GameAux fakeTYPE를유지함을검사한다. 새6+기존관련96=집중102건통과다.

최초집중은91/102였다. 환경NETSTORM_DATA누락1건을고쳤고, 합성TYPE6/3의실제내장생성자 입력과옛Create의원자적실패가정2건을현행화했다. CD관리풀의서버첫SID는여전히15000/stride50이므로CD보조생성자/새Form검사는client할당을선택한다. 기존Fort로더통합의CD서버15000입력은원문14000예측경계 진단을검증하고정상복귀시켜유지한다. 이경우를실제CD풀/전체게임완료로세지않는다. 기존creation482·derived3300·내용물/로더golden은변경하지않았다.

생성자선조회3/6·저장VT재조회1/6·고정Contained생성1/6실패로세변이를검출하고바이트/SHA를원복했다. 최종전체비증분Release는0경고0오류, 전체Client일반1232/1232(실장치오디오Explicit1미실행)통과다. 대형독립저장관측1건(0.542전체통과)을반복하지않은Core2385중2376통과/9실패가나왔고, 원인은추가Contained/FortMeta/Gump합성TYPE생성자입력8건과Process.Attach실패의할당수기대1건이었다. 실제원문계약에맞게수정한뒤같은2385건을한번의재검사로2385/2385통과했다(2분47초,종료0). 제품6파일의최종바이트/SHA도일치했다. 자료는extracted/dotnet-create-0544-initial/·-mutations/·-final/(Git제외)에둔다.

## 실제 단순 미션 진입

실행확인예외PC HJOW-Athlon에서실제개발호스트의thewarbegins를120프레임·창모드/음소거·영어·fixed unix1760000000/25ms·별도빈사용자루트로실행했다. 입력은frame90의session-dump한번뿐이며포획/희생·특정적공격·승리/MP는재현하지않았다. 종료0/world1, 2140행중0.538덤프와다른행은frame90→91한줄이다. 1856슬롯/255그래프·Game/CRT와나머지행은동일하다. originals1394파일이름/해시는전후동일하고실행전두명시옵션경로의바이트도복원했다. 자료는extracted/dotnet-create-0544-live/thewarbegins다. 이번은대표1미션검사이며0.538의31미션전체반복으로세지않는다.

## 잔여

다음은RawSquidUnpopParent/실제Unpop의현재TYPE·진단/void·contained/공간제거·표시순서다. 이후전체NativeStorage의장기공장/World수명, 표면connector·파생Form현재몸체와제품피해/해방·사제상위배치/삭제·Native활성화/AI결선은잔여다. 실제Renderer/HUD/카메라와프레임루프가미연결이라새경로는플레이불가,M1전체0/34다. PORT_MAP881(L1 421+port460),fixture297(신규588관측1),기존296행/해시와floor/reflected는유지한다. 보호파일/다른프로젝트/원본/디컴파일변경·금지실제플레이/MP없음. 실제게임창은대표1미션진입에만사용했다. 목표active다.
