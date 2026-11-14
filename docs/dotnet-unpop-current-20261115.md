# dotnetpj 0.545 — 현재 Unpop 부모와 실제 공간 소비자

2026-11-15,HJOW-Athlon. 직전0.544는530dee3aa다. 근거는cpppj o/RawSquidUnpopParent.h/.cpp ea848459b5b9b29d98b98adf5deab6519bca997e, o/SquidUnpop.cpp 2c6ab7cc1851d34a9e936af1d4333d2bf5155d38, client/RawSurfaceWorld.cpp c102cdbf36fca4e8661d0699ea2ce5f2f44b2934다. SquidUnpop.h는2c6ab7cc기준의내용이현재와같다(마지막변경6ff267a1). 원본/디컴파일은변경하지않았다.

## 같은 실제 공간 부모

RawSquidUnpopParent는원문순서의Form/contained/유효SID진단→free/void복귀→void선기록→처음선택한물리TYPE/저장genus→현재미니맵→저장범위의순차점유/옆면→현재contained분리또는실제4단계체인제거→늦은표시를실행한다. 각진단이복귀하면원문처럼현재TYPE/좌표/checking을다시읽고, 저장한genus·범위·점유BYTE·next는해당소비시점대로보존한다.

TYPE보기는처음고른표/칸을보유하므로현재TYPE표를다시연결해도처음물리칸의발자국을다른표에서읽지않는다. 각셀의실효genus/표면Flags는현재TYPE에서조회한다. 점유진단이원래BYTE나전체지도를바꿔도원문은저장BYTE에마스크를적용한다. Side는실제표면WORD가있는중간옆면에만요구한다. 직접체인머리저장은next를추가역참조/자기연결선검사하지않으며, 이전노드의next가이전노드이면원문진단이복귀한뒤저장next를쓴다. PATCH미발견은found진단,CD미발견은진단없이표시로이어진다.

실제RawUnpop은옛전수선검사/별도지도·체인몸체를같은부모로대체했다. 현재진단함수/종속분리는호출의사본을유지하고TYPE표는소비위치에서다시선택한다. 미니맵이없는독립입력은원문helper처럼자연복귀한다. 실제옆면/종속분리/지원VT의부재는도달위치에서실패하며앞선void/지도/해시를되돌리지않는다. 표시Validate를원문에없는선검사로부르지않고실제공간제거뒤현재VT의Update를선택한다.

현재PATCH World는같은Client의현재checking/TYPE/복귀진단과현재Session또는World기본ContainedUnpop을연결한다. 이결선을전체NativeStorage의장기기본공간/공장·전체World소유자복원으로세지않는다.

## 원본 관측과 실제 장치

PATCH squid-unpop-parent-hardware1300행과CD cd-squid-unpop-current-hardware1402행,합계2702개최초CPU관측을읽기전용으로연결했다. 전체129raw·네해시172544BYTE·점유65536BYTE의체크섬과현재진단/실제옆면/외부경계사건을대조했다. 기대값은수정하지않았다. 독립자식의명시D/Q·U요청은실제게임UI/표시완료로합산하지않는다.

CD관리풀은여전히50BYTE슬롯이므로원문36BYTE prefix129개를조밀하게합쳐4644BYTE를대조하고추가14BYTE패딩불변도검사한다. PATCH는실제50BYTE/6450BYTE다. 현재부모의두판본관측통과를전체CD관리풀/게임복원완료로세지않는다.

추가6경우는실제공장/공간을사용해늦은미지원표시의선행변경, 실제표시선검사없이제거뒤Update, 선택된옆면누락앞의부분점유, 현재TYPE/처음물리칸·진단함수사본/저장BYTE·현재checking, 종속함수사본/현재contained분기, 실제World의현재Client진단을검사한다. 원본재생2+추가6=새8검사,기존관련256포함264건통과다.

최초소비자258검사중3합성실패는옛원자적거부/표시Validate선행/옆면표면입력가정이었다. 원문처럼void/해시제거를보존하고실제표면WORD를명시했다. 지도밖실패후void재호출은자연복귀하며머리의자기next도추가가드없이저장한다. 기존unpop-x86/가상표등의원본기대값은그대로통과했다.

저장점유BYTE재조회2/8·현재sidegenus재조회2/8·현재종속함수재조회1/8실패로3변이를검출하고바이트/SHA를복원했다. 최종전체비증분Release는경고0·오류0,Core회귀2393/2393(3분30초)·전체Client일반1232/1232(2분53초)·각종료0이다. 0.542전체통과한대형독립저장관측1건은반복하지않았고,Client실장치오디오Explicit1은미실행이다. 제품5파일의바이트/SHA일치를확인했고OriginalUnpopDisplay의옛선검사설명도현재의늦은지원판정으로고쳤다(주석만변경). 자료는extracted/dotnet-unpop-0545-initial/·-mutations/·-final/(Git제외)에둔다.

## 실제 대표 미션 진입

실행확인예외PC HJOW-Athlon에서실제개발호스트의thewarbegins를120프레임·영어·창모드/음소거·fixed unix1760000000/25ms·별도빈사용자루트로실행했다. 조작입력은없고frame90의session-dump만수집했다. 종료0/world1,0.544의2140행덤프와차이는frame91→90한줄뿐이며1856슬롯/255그래프·Game/CRT와나머지행은같다. 원본1394파일이름/해시전후동일과두명시옵션경로의원래바이트복원을확인했다. 자료는extracted/dotnet-unpop-0545-live/thewarbegins다. 포획/희생·특정적공격·승리/MP를재현하지않았으며이번대표1미션을31미션전체재실행으로세지않는다.

## 남은 연결과 인수인계

전체NativeStorage의장기공장/공간/World수명,표면connector/파생Form현재몸체·제품피해/해방·사제상위배치/삭제·Native활성화/AI결선은잔여다. 실제Renderer/HUD/카메라와호스트Kernel프레임루프도잔여라새경로는플레이불가,M1전체0/34다. PORT_MAP886(L1 422+port464),fixture299(신규2),기존297행/해시·floor/reflected는유지한다.

이단위의검증·문서작성을마쳤으며사용자의2026-11-15요청에따라커밋직후목표를일시중지한다. 보호원본/다른프로젝트/디컴파일변경·금지실제플레이/MP는없다. 재개시LEFT_JOBS.dotnetpj.md의다음우선순위를따른다.
