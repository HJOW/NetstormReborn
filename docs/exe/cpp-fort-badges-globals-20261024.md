# Badges 단일 장부와 실제 CRT/reset 독립 관측

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. AGENTS·공통/닷넷 인수인계를 재독했다. 이 작업은 신규Badges장부소유모듈·검사·최초독립fixture/도구/보고서/문서만추가한다. 기존Client/Session/CMake/LEFT/원본/CD/디컴파일/git이나공유빌드산출물을변경하지않았고게임/GUI를실행하지않았다. [앞선읽기조사와Client주입안](cpp-badges-client-next-20261024.md)은정적분석이며,아래Unicorn관측은그뒤별도로수행했다.

## 관측을 먼저 고정한 순서

보호10.78 PE의 `004fcbe0~004fea0f` 전체CRT와 `0047d860~0047d882` 전체count reset을 `tools/decomp_fort_badges_globals.py`로독립실행했다. 제품소스/제품바이너리/기존장부초기화기대를읽어결과를만들지않는다. 두원문에는외부CALL이없으며명시성공stub도없다. 코드진입을이번함수하나에,원문쓰기를6160BYTE장부와별도스택에제한한다. 정상RET/callee-save/스택/x87제어어·TOP와두원문바이트불변을검사한다.

고유입력은 **32개**다. `crt`, `reset`, `crt-reset`, `reset-crt`의네순서와최초0/ff/a5/증가BYTE/반전BYTE/37배패턴/부호비트DWORD/전부비0BYTE의8프로필을교차한다. 같은입력을x87 `0x27f/0x37f`로실행해 **64회관측,96정상함수반환,112,656실제명령**을확인했다. 순서결합입력은실제각함수의정상반환뒤같은장부를다음함수에전달하며,고유입력을다시합산하지않는다. 전체6160BYTE와단계별SHA/stackdelta/명령수/쓰기수/쓰기된고유BYTE수를보존했다.

최초fixture `cpppj/tests/fixtures/fort-badges-globals-x86.tsv`와 `cpppj/fort-badges-globals-native.json`을**제품작성전에처음생성**했다. 기존파일이있으면덮어쓰지않는다. `--dry-run`은전수를다시실행하고파일을쓰지않으며,`--verify`는최초PE/정밀C/도구/fixture SHA만확인한다. 실제전체CRT결과는선행정적대입기대와일치했고다음두값을서로구별한다.

| 범위 | 전체6160BYTE SHA-256 |
|---|---|
| PE로더의최초가상0영역 | `155fb7024ecfd37be15220ff6dc1c60f798285b0e82dfedae4ea8e0255acdbce` |
| 선행정적CRT후기대 | `40e455f3941361b21cc236838aa108416bf859a5214aa76dbd7cbb23d159cbc8` |
| 독립실제CRT후관측 | `40e455f3941361b21cc236838aa108416bf859a5214aa76dbd7cbb23d159cbc8` |

CRT는6152고유BYTE를쓰며첫기록+4~11의8BYTE는비0입력에서도보존한다. 27typed기록의타입/ASCII/0/padding/count와28번째전체0이일치했다. reset은28countDWORD의 **112BYTE만** 쓰고나머지6048BYTE는비0입력에서도보존한다. 이초기화는비0 UI/heap/Gump포인터를생성하지않는다. 장부끝00594970의서버이름버퍼를포함하거나reset의다음count포인터00594a48을장부끝으로오해하지않는다.

## 신규 제품과 같은 span 계약

`cpppj/src/o/RawFortBadgesGlobals.h/.cpp`는6160BYTE배열한개를소유한다. 새소유자의배열은먼저BSS처럼0이고생성자가한번CRT구성을적용한다. `Records()`의mutable/const고정span은같은배열/같은주소를돌려준다. 이미빌린span을무효화하지않도록복사·이동을금지했다.

- `InitializeFortBadgesRecords(span<uint8_t>)`: 정확히6160BYTE를요구한다. CRT의27타입/ASCII구성·28번째0·유일한미작성8BYTE를재현한다.
- `ResetFortBadgeCounts(span<uint8_t>)`: 정확히6160BYTE를요구하고28countDWORD만0으로쓴다.
- `RawFortBadgesGlobals::ResetCounts()`: 같은소유자의위reset만명시호출하며CRT전체를반복하지않는다.

타입/문자행제품자료는최초독립fixture후고정했고선행정적전개와같았다. CRT에는중간외부효과가없으므로제품은원문최종자료와지정/미지정영역을재현한다. 개별MOV중간상태관측이나중단callback을공개하지않으며원본의1475정적명령순서를그대로C++명령으로복사했다고주장하지않는다. span크기오류는실제장부밖접근대신효과전에거부한다.

기존 `RawFortMetaLoad(pool,globals.Records(),...)`와 `ReadFortBadgeCount(globals.Records(),type)`/`SaveFortBadges`의count hook을같은장부에연결할수있다. 원시Fort섹션/별도count28개사본을소유자로사용하지않는다. 실제Client가한shared_ptr를소유하고Session서비스로빌리는변경은이번모듈의공유등록후후속이다. supplied owner를매Session/Template생성때다시초기화하지않아야하며,명시count reset의실제시작호출위치도별도다.

## 격리 C++ 검증과 한계

`FortBadgesGlobalsTests.cpp`의새3검사를자체 `extracted/cpp-fort-badges-globals-20261024-isolated`에서MSVC19.51 x64 `/std:c++20 /utf-8 /W4 /WX /O2`로컴파일했다. 최종build3은 **종료0/관측경고0**,실행은 **3검사/실패check0**이다.

1. 독립32입력의전체6160BYTE를비교하고초기0/비0·두몸체순서를검사한다.
2. 새소유자의실제최초CRT자료/같은mutable·const주소/다른소유자분리/reset6048BYTE보존과크기거부의무변경을검사한다.
3. 기존실제MetaLoader가type71/count7과누락형식type0/count9를같은장부에적용하고,직접setter72/count256의원본진단후전체DWORD저장·빈/부분로드보존·getter즉시관측을검사한다. 실제새writer출력은 `[3,0,9,71,8,72,0]`이며count256도비0DWORD여서포함되고저BYTE0으로저장된다. writer가장부를바꾸지않는것도전체비교한다.

기존MetaLoad/SidPool/BadgeSave의**변경하지않은소스**를새로컴파일했다. 격리의첫build1은/link인자전달경고와미선택Technology기호부족,build2는인자를바로잡은뒤동일미선택기호부족으로실패했다. 두로그를보존했다. build3은선택하지않는Technology어댑터의세함수(Template::Section,ObjectLoad::Contents/Pool)에만**호출즉시throw하는격리경계**를추가해링크했다. 성공noop이나임의자료를반환하지않으며예상밖호출은검사실패다. 이검사로Technology/그전체어댑터를실행했다고세지않는다. 기존라이브러리나공유build를혼용하지않았다.

최종검사실행SHA-256은 `f763d71a667f76c15946cf5db82c0c4d674bf8e7c800358468bcdf5591656f3d`다. `build1/2/3.log`, `tests.log/.json`,사용한response/main/fixture헤더/미선택경계를같은격리경로에보존했다. Python AST/UTF-8/공백검사와최초native SHA검증도통과했다.

- 보호PE: `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`
- 현재정밀C: `af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646`
- 최초fixture: `8e743060a2d555dea66ccadbcbb9075fd0ce8224cddf2e741f8a006dd7b8cf51`

CMake등록매크로는 `NETSTORM_FORT_BADGES_GLOBALS_FIXTURE`이며값은해당fixture의절대경로다. **실제Client/Session주입·공유target등록/전체CTest·GUI/전체snapshot파일저장·재로드는아직이하위작업에서검증하지않았다.** 전체Editor/사용자완주·외관/음향동일성도계속후속이다.
