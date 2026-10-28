# 객체 덤프 부모와 실제 디스크 장치 복원 — 단위558

2026-10-29 / HJOW-Athlon. AGENTS.md와 최신 인수인계를 다시 확인했다. 마지막 완료된 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 현재 PC와 같다. 이전557은 코드·원문 관측·커밋을 완료한 진척이다. 이번에 디컴파일 산출물이나 보호 원본은 변경하지 않았다.

## 복원 범위

`RawObjectDump`는10.78의 `004d6020` 전체 부모다. 기존 공유로그/재귀/가상/CRT서식과 함께 같은 물리 풀을 읽는다. 현재level을보관하고30으로, 덤프모드를1로 쓴 뒤 경로→`fopen(wt)`→현재슬롯/현재TYPE 출력→`fclose`→전체읽기→할당→마크업제거→BaseFile생성/열기/바이너리쓰기→모드0/이전level→파일소멸 순서다. 첫FILE0이면 assert가복귀한 뒤 현재FILE을 다시 검사하고 여전히0일 때모드/level을복구하지않고 반환한다. 임의의 정상값을 넣지 않는다.

최초iterator는cursor1의Next부터여서 **SID1은 방문하지 않는다**. free건너뛰기는 최초count snapshot을, 실제바깥끝검사는그때의현재count를쓴다. 따라서 외부콜백으로끝번호가늘면 최초끝이후free후보도원문처럼방문할수있다. TYPE0/contained는부모출력에서제외하고 현재97/102/121 전역에해당하면전체TYPE이름과 `_ftol`의int64절삭/하위DWORD좌표만기록한다. 다른종류는실제재귀뒤구분개행을쓴다. 범위밖좌표의원문indefinite 하위DWORD도0으로보존했다.

후처리는물리wt파일의CRLF를읽고, 모든`~`와그뒤한BYTE를무조건건너뛴다. 입력중NUL뒤도끝까지처리하지만최종쓰기는첫NUL전길이다. 할당요청은원문처럼`signedSize<4 ? 4 : size`다. 호스트장치는마지막NUL용1BYTE를추가로확보하여원문의마크업없는파일에서생길수있는끝1BYTE초과쓰기를방지한다. 실제요청인자/최종출력바이트는원문과같다.

`ObjectDumpDisk`는명시한디렉터리의같은`squids.txt`에실제`wt`출력과`w+b`재쓰기를수행한다. 상위폴더를임의생성하지않는다. 원본과다른Windows10/11경로호환은UTF-8→char8_t filesystem→wide CRT에만두고한국어주석으로명시했다. 실제24BYTE파일물리의주소와호스트쓰기객체수명을연결하며호스트포인터를게임슬롯에직렬화하지않는다. 실제stdout/stderr/FILE의로그장치도제공한다.

`TraceRawObjectToLog`는현재풀/전역→재귀→실제가상분배→같은공유로그를연결하는제품의공통함수다. 가상본문뿐아니라재귀의END/free줄도같은선행마크업규칙으로전달한다. 최초검사의별도조립기는중첩END의접두를0으로전달했으나추가원문사례에서차이를확인해제거하고이공통함수로교체했다.

**현재 Client의 장기로그/객체덤프 호출과 Kernel/ThunderProcess·Ctrl Shift F7은 아직 연결하지 않았다.** 이번실제파일검증은격리된헤드리스장치검사이며게임진입/사용자플레이검증으로세지않는다. 게임실행파일은552그대로다.

## 예외 범위의 원본 근거

원본EH함수`004fc168`은`00525670`의FuncInfo를사용한다. try범위는state1..1이고handler는`0052564c`의TypeDescriptor `0052f460`이며실제이름은`.?AVEX_UNABLE_TO_OPEN_FILE@@`다. **catch-all이 아니다.** 이정확typed예외만최종`openWrite/write`에서잡아`004d62a8→004d6275`로복귀한뒤모드복구/파일소멸한다. 앞선read/할당/생성예외는그범위밖이다. 다른예외는state0 unwind `004fc160→0041b410`으로같은File을소멸한뒤전파하고30/덤프1상태를남긴다. 정상소멸은모드복구뒤state-1에서호출하므로그예외때두번소멸하지않는다.

기존색상캐시의동일원본typed표현`ColorCacheUnableToOpen`을별칭으로재사용했다. 모든파일오류를이형으로바꾸지않는다. 위EH주소/범위는정적원문metadata근거이고, C++의6경계×2예외검사는이계약을검증한다. 실제원본OS예외/대화상자를실행했다고주장하지않는다. unwind중두번째소멸예외는이검증범위밖이다.

## 검증과 남은 작업

- `decomp_object_dump.py`: 전체덤프·실제재귀/가상·공유로그·게임formatter·원본CRT를실행했다. 경로/CRT FILE/할당/BaseFile/console와기본C locale thread context만장치경계다. **56고유입력×두정밀도=112관측**, C++의모든장치/로그순서·현재전역·최종파일바이트가일치했다. 번호1/끝/snapshot증가뒤free, TYPE/상태조합,모드/열기실패후현재FILE재조회,종속/중첩END재귀,좌표절삭/범위밖,마크업/끝~·내부NUL·빈파일을포함한다.
- 실제격리디스크의ASCII/한글디렉터리두곳에서같은전체재귀/로그/파일파이프라인을실행해원본최종파일바이트와일치했다. 없는상위디렉터리에서열기실패/assert복귀/모드유지도확인했다. 생성파일은`extracted/cpp-object-dump-558-20261029/disk/`와`disk-한글/`뿐이다.
- 최종Release검사target 경고/오류0. 신규4+RawTraceLog4+ObjectTrace8+ProcessSummary2+Kernel5+Contained15의 **고유집중38/실패0**, 등록3416이다. 전체C++/CTest·실제게임/원본/네트워크실행은0이다. 기존549 자동전술실패와자연완주/사용자개입보류를해제하지않는다.
- 최초build-v1은파일장치namespace닫힘누락과C++20 u8path폐기경고, v2는닫힘위치오류로실패했다. 이를고치고char8_t경로생성으로바꿨다. v3와한글경로추가v4가통과했다. 이후중첩END를56번째원문입력으로추가하자이전검사제품은행수변경1개/실제바이트차이1개의2단언이실패했다(`test-dump-nested-before.log`). 조립기를위공통제품함수로교체한v5가최종이며모두실패0이다. 원문기대값을제품에맞춰약화하지않았다. 최초빌드/검사원시는보존했다.
- 보호1820파일·기존fixture719개(합계2539), 게임552는전후바이트동일하다. 원격push는하지않았다.

보고서는 `cpppj/object-dump-20261029.json`, 원시는 `extracted/cpp-object-dump-558-20261029/`의 `before.json`, `native-v1.log`(54입력), `native-v2.log`(끝증가사례추가55입력), `native-v3.log`/`native.json`(중첩END추가56입력), `build-v1..v5.log`, `test-dump-v1.log`, `test-dump-nested-before.log`, 여섯`test-*-final.log`다. 후속은 **실제Client 장기로그/재귀전역·현재Kernel Form/부모와ThunderProcess·Ctrl Shift F7 조립**이다. 기존스펠도움말그림·연녹색직사각형·언어파일부재보류, 공중3종정적제한·MP신규검사금지를유지한다.
