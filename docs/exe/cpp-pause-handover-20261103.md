# 775 완료 후 일시정지 인계

**재개됨(2026-11-03):** 새 사용자 /goal로 아래775 일시정지 지시가 대체됐다.776·777·779·783·781·782·784·785·786·787·788·789·790·791·792·793·794·795·796·797·798·799·800·801·802·803을 완료했고 checking 잔여38와 비멀티UI/Edit/수명·호환성을 계속한다. 고정 준비본ID의 적용 순서는779→783→781→782→784→785다. 이 문서의 준비본·필수 교정·백업 목록은 계속 유효하며 현행 값은 [LEFT_JOBS.md](../../LEFT_JOBS.md)에 있다.

2026-11-03 사용자 요청: 현재 작업을 마무리하고 문서와 git 커밋까지 끝낸 뒤 목표를 일시 정지한다. 이번 완료 경계는775이며 전체 목표는 미완료다. 이 문서와775 소스·보고서를 함께 로컬 커밋하고 목표 상태를 paused로 전환한다. 사용자의 재개 요청 전에는776이나 다른 후속 구현·검사를 시작하지 않는다.

## 완료 제품과 검증

제품775/표시11.01, EXE SHA-256 `0b6b4abee79f0b00ef880af2a46a391f505c044d91c01208fa6936f002d23fa2`. 최종Release경고/오류0·집중12개(새6개)/실패0이다. 기존보호2706개 모두불변·새fixture4 포함2710개/fixture890, 다음 기준은 `extracted/cpp-resume-775/protected-after.json`이다. [775 기록](cpp-ai-debug-menu-20261103.md)·[보고서](../../cpppj/ai-debug-menu-20261103.json)를 따른다.

이 재개 이후769~775를단위별로갱신/커밋했다.775는실제ToggleReporting/ToggleDebug이름·첫NUL·조건AI메뉴·현재brain과장기진단DWORD를연결했다. 새기본메뉴를추가하거나실제게임에서개발자키를켜지않았다. checking98소비자/진단표시/전체메뉴동일성으로확대하지않는다.

새게임/GUI/독립CLI/MP·물리/PCM·공중3종전용검사·포획/희생·특정적공격·승리/자연완주는0이다. 마지막GUI746·전체34/ZIP645의644제품·전체C++549의7실패는각당시기록그대로다. 이번집중통과를전체검사통과로쓰지않는다. Win10캡처는762의실제환경변화전재시도금지상태를유지하며Windows11/자연완주/전체동일성의사용자확인은단계전환을막지않는다.

## 재개 절차

1. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를재독한다. 마지막전체정밀디컴파일은HJOW-Athlon/2026-10-23이며다른호스트면AGENTS대로재디컴파일한다. 원본/기존fixture/AGENTS수정과push금지를유지한다.
2. `git status`와최근커밋을읽고다른DN/WB세션변경을보존한다. 당일커밋번호는최근500subject의최대값다음으로계산한다. 병행DN과773의187중복은과거기록으로남기고다른커밋을재작성하지않는다.
3. 현재775동일EXE/보호기준을확인한뒤**776부터**계속한다. 한CMake/MSBuild만동작시키며기존MSVC권장인자를유지한다. 전체C++/34/ZIP은매단위반복하지않는다.
4. 패치는현재source에최소hunk로병합한다. `git -c core.autocrlf=false apply`로기존혼합CRLF/LF를보존하고전체옛사본을덮어쓰지않는다. 새Nativefixture는최초바이트를복사하고기존기대를제품에맞춰약화하지않는다.

## 미적용 준비와 반드시 반영할 교정

아래준비본은미빌드/미실행이며제품완료가아니다. 작업폴더기본위치는 `C:/Users/BUZZ/Documents/Codex/2026-11-02/agents-md-left-jobs-md-cpppj-4/work/`다.

| 순서 | 준비 폴더 | 재개 때 필수 사항 |
| --- | --- | --- |
| 776 | render-diagnostics-776 | 독립review의ReadOptions닫는중괄호주석누락·빈hash SearchCells false기대패치2개를적용한다. resolver-775.patch의추가5getter는현재775위에따로병합한다. 실제Work/BFS/표시owner를보존하고Lock실패전체복구로과장하지않는다. |
| 777 | type-live-reader-777 | 독립review확정0. 실제check0/extra0/현재VFS만접합하며CD/CLI/snapshot/Parse를보존한다. type-scanner-fidelity-780은정적감사일뿐가짜EOF/offset API를추가하지않는다. |
| 779 | checking-consumers-779 | 독립review의RawBuildingSpace.debug→RawCheckingFlag/include와SoundProcess의o::한정패치2개필수.62파일/공개39헤더영향으로기존전체대상compile/link를권장하나전체검사·신규MP실행은금지다.98중남은51부모를완료로세지않는다. |
| 781 | gif-live-reader-781 | 새GifFileReadTests에o/BaseFile.h를직접포함하고별도review-followup의명시빈FileReader생성도반영한다. 두번째는기존base부터있던컴파일결함의이전검토누락이다. Kencloud/Background두caller만이며title세caller는782다. |
| 782 | title-bitmap-lifetime-782 | title-bitmap-path-fix-782의3파일교정필수. Path는같은RawFileName의-2빈문자열/-2고정d/-3literal이며중복GameDirectory/nativeDataPrefix추가를제거한다. 8칸/WM_PAINT/State42→41/WM_CLOSE수명과부분실패·stride·초기host준비한계를보존한다. |
| 783 | checking-parent-followup-783 | 779의존Boardcoord41db70한부모/7파일이다.4b54e0을작은SP Login Update로선택한옛분류는폐기한다. 실제일반battle재접속save/Player직렬화는후순위MP다. |
| 784 | recent-command-tail-784 | Uber한파일/최근12명령의press꼬리보완이다. 현재makeAiHandled와aiDebugHandled까지포함해recent/createAt/aiUtility/makeAi/aiDebug다섯flag를late두guard에보존한다. 새검사0/창없는실제Uber seam부재의정적+기존집중회귀계획을과장하지않는다. |
| 785 | nonmp-io-gap-785 | 실제보조/CD효과음root전달누락의4파일준비다. alternate각패턴때같은VFS현재root를늦게읽으며기존공개생성자·음악·패턴/WAVE·음소거·재생을보존한다. 새Native/CPP/실제PCM/CD삽입0이며필요검증은재개후판단한다. |

Xlat/palette/fontcache/shape의나머지reader/writer는785분류자료를따른다. 버퍼보유시점만다르다는이유로관측가능공백을만들지않으며등록snapshot/CLI/독립OS계약을전역교체하지않는다. rank·4cfc10/448260→44fa00→4b54e0는missionPost0일반battle/재접속save의후순위MP다. 776~785이후남은비멀티/Win10호환성→MP무시험구현→4~8차요구순서를유지한다. Windows11실기·사용자자연플레이를기다리며불필요하게중단하지않는다.

## 로컬 원시 백업

위준비/기본·결과source/신규fixture/도구/독립교정은 `extracted/cpp-resume-775/resume-prepared-after-775.zip`에655파일/압축4856594BYTE로보관했다. SHA-256 `60aebb17941f651023c956bad62c7706be40c705f3d82a049848b2f7cd109c9f`. 세부파일해시는옆 `resume-prepared-manifest.json`이다. 재생성가능scratch Git/캐시만제외했고기존fixture를정규화하지않았다.

이원시는Git비추적이며다른PC에자동전달되지않는다. 다른PC재개에는zip/manifest를복사하고해시를검사한다. source패치가준비됐다는것을실행성공으로세지않는다.775 C++/Native로그도동일extracted에있다. 목표일시정지는사용자요청이고완료/blocked표시가아니다.
