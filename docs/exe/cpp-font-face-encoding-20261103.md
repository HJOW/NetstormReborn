# 774 — raw ACP 글꼴과 UTF-8 설정·디스크 경계

2026-11-03 HJOW-Athlon. 마지막 전체 정밀 디컴파일 HJOW-Athlon / 2026-10-23 유지. 단위774 완료·전체 목표 미완료다.

원문 FontStore의 raw ACP 이름·31BYTE 한계/첫NUL·7슬롯/18유효캐시·두번열거/순서/중복·GDI A/0..255글리프는그대로다. Client는설정내부UTF8이름을현재ACP로변환한뒤기존FontStore를만든다. ChangeFontFace는현재FaceAt/Rebuild/H/W/invalidate/dirty/rawgetter뒤실제Set직전에만ACP→UTF8을수행한다. 표현불가/잘린DBCS를물음표/임의글꼴성공으로바꾸지않으며선행효과뒤변환실패는Config Set/후속음향해석으로진행하지않는다. 한국어UI지원완료가아니다.

FontStore의CHFNT query와rawTAFF이름을보존하고새TryReadWithDiskName의디스크어댑터에만변환을적용했다. 원문공유RawFileName에는먼저raw JoinBaseFileRoot를써폭/내용/소비를보존한다. 성공한Unicode OS query만별도문자열 JoinUtf8DiskRoot로조합한다. UTF8확장을같은256BYTE고리에쓰거나고리를늘리지않는다. 변환실패도원래root칸을남기고rawTAFF로진행한다. default TryRead는빈어댑터로같은정책을한번만쓴다. Device1+기본disk1/보조disk1의2·3칸,기본disk→기본TAFF→보조disk→보조TAFF,공통접두/장치/실패경계는기존계약이다.

완전한31BYTE CP949 face(B1DB쌍15+ASCII1)의query는raw54/UTF869BYTE다. ASCII root190이면원래고리raw245는유효하지만UTF8경로260은넘친다. 독립교정03은raw245를고리에남긴뒤Unicode OS경로만별도로넘긴다. 새회귀는짧은검사소유경로에TAFF만만들고존재하지않는긴읽기root에서fallback과고리내용을확인한다. 긴디렉터리를새로만들거나공유폭guard를없애지않는다.

Windows NLS24명시입력/24관측과root추가24가fixture전체BYTE로일치했다(거부9/systemACP949조회만). 1252/949/932/65001·첫NUL·빈입력·잘린DBCS·잘못된UTF8·표현불가를독립API결과로고정한것이며원문게임x86관측이아니다. OS/글꼴/코드페이지설정을바꾸지않았다. 실제설치글꼴출력/다른ACP/Windows11실기나구형무BOM비1252설정자동이관은별도다.

원동결을독립검토해optional.Get→string_view컴파일결함,빈NLS'-'해석예외,UTF8고리확대회귀를교정했다. 원11파일/fixture를덮어쓰지않고별도01~04패치와현재773기준후보에반영했다.01은존재확인뒤값,02는새검사decoder만수정,03은raw고리/OS경계분리,04는긴경로회귀1개다. 현재771Configinitializer/773MakeAi·AIUtility·CreateAt guard/CMake누적과기존개행을보존했으며제품11파일은최종후보와BYTE동일이었다. 최초정적준비의혼합EOL guard중단도원시에보존하고실행실패로세지않는다.

최종 Release 경고/오류0, 집중 **26개(새6개)/실패0**. raw ACP/UTF8 설정저장·재읽기/changed·Unicode디스크/TAFF우선·공유고리·현재FontStore CHFNT·DBCS부분실패/긴949경로,기존Presentation·glyph주소재사용캐시와BaseFile정책/보조조회회귀를확인했다. 기존1252 XOR/비1252의유효UTF8 BOM writer는재사용했고바꾸지않았다. 새target0/기존66개·전체71대상유지다.

기존 보호2705개 모두 불변·새fixture1 포함2706개/fixture886. 다음 기준은 extracted/cpp-resume-774/protected-after.json이며 제품 SHA-256은 `7ee355ed807e79b58077e64327783551c7ae174d4084a73fba00707148d7b1a8`다. [보고서](../../cpppj/font-face-encoding-20261103.json)와같은extracted의독립교정·최종후보/정적실패·원NLS/반복·build/tests를따른다.

새게임/GUI/CLI/MP·물리/PCM·공중3종전용검사·포획/희생·특정적공격·승리·자연완주·전체C++/34/ZIP검사는0이다. 다음은조건AI메뉴의실제이름/진단/reader/checking/초기화면과남은비멀티·호환성이다.
