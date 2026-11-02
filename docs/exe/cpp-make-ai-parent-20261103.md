# 773 — MakeAi 일반 부모와 owner0 객체 수명

2026-11-03 HJOW-Athlon. 마지막 전체 정밀 디컴파일 HJOW-Athlon / 2026-10-23 유지. 단위773 완료·전체 목표 미완료다.

MakeAi/005429d0/020000b7을 같은 이름·HTML·ScriptStringRing·일반 입력에 연결했다. Guide/Gump와 공통뗌 뒤 현재Editor OR missionPost에서 param2 전체DWORD를 현재 Session.MakeAi로 보낸다. battle0은 정상반환하며 실제battle1인데World가없으면 명시거부한다. 새기본UI/로컬AI특례/임시World는 없다.

417fa0은battle0이면600BYTE설정이나하위장치를 읽지 않는다. 실제40ef80 기본Initialize 뒤 현재missionPost0이면220=0/224=float1/228=float2/230=3/234=6을 쓴다. 미션 시작의공통/owner키 Prepare를 호출하지 않는다. owner비영은PlayerKnown→tech첫BYTE0→Known이면그때localOwner의현재SP float→실제_ftol signed64절삭하위DWORD→기존두인자CreateAi 순서다. 큰유한값/NaN/범위밖 indefinite의하위0도 구별한다.

owner0은local+1을 한 번 캡처하고기존brain이있으면반환한다. 없으면4c27c0의현재비로컬섬객체이전→417e50(owner,settings) 두인자생성→414c80(mask2000) 가상삭제다. 0x2000을세번째생성인자로만들지 않는다. 이전은free1만거르는iterator/현재owner·TYPE·flags/로컬두장부·첫region·현재좌표/Player행을원문각위치에서읽는다. 첫Owner뒤flags를재조회하고priest비트면같은loadingDepth를올려두번째Owner를부른다. 두번째Owner실패에감소를합성하지않는다. 같은깊이를World.SetLoadingDepth로실제사제/자산소비자에전파한다. Transfer의Next는효과뒤,Clear의Next는flags/가상삭제앞이라는차이를보존한다.

동결준비본의두경계차이는독립읽기검토와대조해보완했다. MakeAi press후조기return을없애현재Help/Console/정지/세계꼬리를진행하고release만반환한다. makeAiHandled를기존CreateAt/AIUtility와같은늦은dispatcher두곳에서검사한다. 직접이름helper도원문stricmp의첫NUL을보존한다. 기존QueueScriptCommand는이미RawTextToken에서잘랐으므로그caller의새가시게임오류로주장하지않는다. 추가확정parent/core결함0이며읽기결론과실행검사를구별한다.

최종원문부모770/객체2592/입력64의6852관측과이름12,root독립반복6864관측의4fixture가바이트단위로일치했다. 실제초기화/ftol/iterator/현재flags/카운터감소몸체이며SP·실제생성/가상Owner/가상삭제/섬getter는명시경계다. 최초유효4548관측/1440객체행도보존했고최종2592에같은값으로포함됐다. 확장첫시도는MakeClear라벨의ret4검증누락으로fixture작성전에실패했다. 검증조건만교정한v2/반복성공과실패로그/observedtool소스를구별한다. 이후한국어loop주석추가로관측toolSHA가현재소스와다른이유도observations에보존했다.

최종 Release 경고/오류0, 집중 **12개(새6개)/실패0**. 실제Runtime1은battle0과기존brain이있는owner2/owner0만실행해같은SP/시각/본체/할당/패킷과반납을확인했다. owner0양성이전/가상삭제·Kernel/전략프레임·공중3종·공격·포획/희생/승패/MP는실행하지않았다. 이전772의직접writer/일반입력·실제AI메모리회귀도통과했다. 새target0/기존66개·전체71대상유지다.

기존 보호2701개 모두 불변·새fixture4 포함2705개/fixture885. 다음 기준은 extracted/cpp-resume-773/protected-after.json이며 제품 SHA-256은 `570cf765ca815d72d90a3f145431e4b1b1cde1a6064211b989011ac16bf039c7`다. [보고서](../../cpppj/make-ai-parent-20261103.json)와같은extracted의초기/확장실패/최종/반복Native·독립검토·build/tests를따른다.

미작성600BYTE의host입력과Native205seed/미정stack는구별한다. 감긴Player주소/정상Pool밖/손상TYPE/실제32비트주소/전체SEH·ABI는완료로확대하지않는다. 실제local+1 Player9밖도기존계약대로거부한다. CD1072/조건AI메뉴/전체UI는별도다. 새게임/GUI/CLI/MP·물리/PCM·금지플레이·자연완주·전체C++/34/ZIP검사는0이다.
