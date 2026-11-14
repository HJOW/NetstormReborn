# dotnetpj 0.538 — 제품 공통표시/연소·회복과 실제 캡처 연결

2026-11-15, HJOW-Athlon. 직전0.537은00490e09e다. 조립 기준은 cpppj RawCampaignSession.cpp의382ec33feca7a418dccd62c599abdbebb88e07e8, RawCampaignParticles.h/.cpp의247855746f40366e6c45c9c2b97dc378f0cb8bf8, RawPlainUnitCapture.h/.cpp의253e6a9981e5e50ecbb4881255ba302534834c1e와 SquidRenderer.cpp의aa506ebb8d367ea74a97baf71c800546f8713606이다. 원본/디컴파일 산출물을 변경하지 않았다.

## 실제 조립

HostSession은 실제 사제/소리와 동일한 PlayerProcesses/풀/Kernel에 RenderProcessSystem·BurningVisualSystem·RawDamageableBurning을 연결한다. 일반저HP의현재보상HP를읽고같은부모종속체인의14를찾는다. 필요할때실제mode1생성자를호출하며 실제확보실패만0으로복귀한다. 절반경계에서는실제Form가상삭제→등록/Kernel소멸을실행한다. 사제회복효과의기본Damageable은세션owner를지연조회해사제효과사본을먼저구성한뒤연소를붙여도같은실제몸체를사용한다. 명시제공한상위효과는보존한다.

연소점의현재약화/사제TYPE/다이스TYPE는같은장기GameAux를빌린다. 캡처/프레임/연소점의현재checking은같은전체DWORD뷰이며 상세SID는기존원문몸체를관리풀의상대byte offset에연결했다. 고정네이티브주소로위장하지않고각선택필드접근때현재Pool.Bytes를읽는다. TYPE선행조회→논리범위→각필드/최근실패비트의순서를유지하고게임세대/void사전필터를추가하지않는다. 최근실패/TYPE는ClientGameplayGlobals가소유해미션반납뒤에도보존한다.

HostMissionBurning은제품StartMission에서실제TYPE/정의/SHP/게임COL·동일소리재생기에연결된다. 첫색띠/실제색수heap과인접DWORD를0.537몸체로만들고dirty는보존한다. 실제사제/일반유닛13종의빈+a4는TYPE와VT둘다맞을때만선택한다. 일반소유자색표는같은현재OwnerColors를읽는다. 전체공통OwnerColorTable·특례색표/다른파생추가캡처는아직없으므로선택시명시실패한다. 실제카메라/월드viewport도R5에서SetView로연결해야하며, 미연결제품뷰를전체화면크기나0사각형성공으로대체하지않는다. 커널호스트프레임루프는이번에도켜지않았다. 현재시각/프레임준비만실제호스트시계에연결한다.

종료/적재실패는같은Host프로세스반납→연소조회반납→외부실제팔레트/소리순서다. 자기수명만비우며다른Host의Kernel항목은보존한다. 새실행파일/새외부접속은없다.

## 실제 캡처가 찾은 기존 오류

TerrainDisplayShapes.Build가Squid표시헤더폭/높이대신VFX압축경계의폭/높이를넘기던오류를수정했다. 실제70번캡처가전부255로잘리던검사가이를검출했다. 원본SquidMetrics의DisplayWidth/DisplayHeight를직접쓴다. 셀/포구/보행전체메타데이터와전체SquidRenderer이식완료를뜻하지않는다.

## 집중 검증

새제품결선6검사와기존사제6·지형Pop5, 합계17검사가통과했다. 저HP생성/중복방지/반절삭제, 실제SHP/첫팔레트/Kernel3단계와이동입자/픽셀/dirty, 외부Kernel보존, 사제효과사본후행연소결선과회복뒤삭제, 명시효과보존, 현재전체checking/실제raw와장기SID진단, 실제일반소유자색표/정확한사제VT/빈+a4를확인했다. 회복테스트의사제damageEffect=1은복제한현재TYPE객체에준합성입력이며원본사제설정이나실제게임플레이검증으로세지않는다. 추가캡처의baseFrame=-1은원문빈그림을유지하며실제SHP에서보이는프레임도독립자산Decoder로선택해대조한다.

초기11중3실패는테스트의client사제/serverPop조합2건을server할당입력으로교정하고, 나머지는위제품SHP크기수정으로해소했다. 수정후이동입자검사는첫visible프레임의시각차0뒤한프레임더입력하도록교정했다. 추가사제검사의초기0/기본-1빈그림기대와독립Decoder의잘못된호출인수도바로잡았다. 초기플랫폼경고4개는Windows전용생성계약과실제OS관문으로정리했다. 원본/fixture/golden보정은없다.

회복의연소호출생략은12검사중1실패, 상세SID검사생략은12중1실패, 이전VFX크기사용복원은12중2실패로검출했다. 세제품소스를바이트/SHA단위로복원했다. 복원뒤전체비증분Release빌드는경고0·오류0이다. 실제표시폭/높이변경영향을확인한최종관련Core236/236·전체Client일반1232/1232는실패0·각종료0으로통과했다. Client목록1233중실장치Explicit1개는미실행이며물리재생으로세지않는다. 자료는extracted/dotnet-host-burning-0538-initial/·-mutations/·-final/에있고Git제외다.

## 실제 단순 미션 진입과 L2

HJOW-Athlon(AGENTS실행확인예외PC)에서실제제품개발호스트의일반미션31개를각120프레임실행해모두종료0·world1을확인했다. 고정unix1760000000/프레임25ms·영어·창모드·음소거·빈개별사용자루트·사본extracted/dotnet-oracle-data를사용했다. 조작명령은보내지않았고공격/사제포획·희생/승리/MP를재현하지않았다. originals1,394파일의이름/해시는전후동일하다. 자료는extracted/dotnet-host-burning-0538-live/다(Git제외).

thewarbegins는1856슬롯/공통1856·cpp_only85/dn_only0·타입1633/소유자464/프레임126/full80·그래프240/255·Game2317693072/CRT296677442로직전상태를유지했다. 0.529실제dn과전체1856슬롯/255그래프·난수가같고, 전체행의유일한차이는덤프관측frame92→90이다. 즉SHP표시헤더교정은이대표미션의저장슬롯/그래프/난수에중립이다. cpp전체L2일치는0/31, 데모3은미연결로전체M1은0/34다.

## 남은 범위

전체OwnerColorTable과최초ColorSort공유수명·다른파생캡처, 실제카메라/Renderer/HUD·Kernel호스트프레임루프·전체피해/삭제/배치·brain/다른파생과데모3경로는잔여다. 제품에연소owner를조립했다고화면/자연플레이/M1일치로세지않는다. 사제포획/희생·특정적공격·시나리오승리/MP실행은제외한다.

PORT_MAP857행(L1 416+port441), fixture294개(이번신규0). 새7쌍은조립부분/Source포트이며새독립PE완료L1로세지않는다. 기존짝의전체base/floor/reflected는올리지않았다. 보호원본/CD·AGENTS.md·README개요·cpppj·LEFT_JOBS.md·다른프로젝트변경없음. 목표active·M1전체0/34다.
