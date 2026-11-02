# 762 — 설정 명령과 raw priority 메인루프

2026-11-03 HJOW-Athlon. 전체정밀디컴파일 HJOW-Athlon / 2026-10-23 유지. 단위762 완료, 전체목표 미완료다.

## 제품 변경과 실제 부모

ChangePal(54279c/02000029)은 battlePal 우선,아니면chalPal/fortPal의현재키를Set한뒤같은키를다시조회해기존Client::ChangeFortPalette→현재reader/구름/팔레트·색표부모에전달한다. Set알림뒤모드가바뀌어도먼저선택한키를유지한다. chalPal양성은정적연결만이며신규검사는없다.

SetFortType(542a10/020000c7)은Editor0진단이정상복귀하면계속현재owner의State28BYTE getter+0x11에param2낮은BYTE를쓰고owner를재조회해기존Notify/저장부모를호출한다. Meta에쓰지않는다. 실제현재manager교체·확장·저장이미지와SoloMapCatalog::Kind가같은State를읽는다. World없는State/Notify는명시Runtime경계다.

ChangePriority(54291c/0200008a)는공유highPriority_에signed DWORD쓰기→현재스레드조회/OS1·15·0요청(BOOL무시)→공유값재조회→highPriorityValue="%d"의동기설정해석이다. 실제 Client onChanged에이키의ReadInt가없던누락을집중검사로확인해같은DWORD에즉시반영하도록고쳤다. 기존전체InterpretOptions와메인루프도같은주소를쓴다.

이세명령을GameMenuCommand에만추가하면Event의HelpOpen 조기반환뒤에가려진다. Guide/Gump/release뒤,HelpOpen/모달/Console/공통정지앞에서이세명령만명시분배하고기존부모를재사용한다. 이름/숫자/원문전역허용표도함께연결한다. 기본UI활성caller는입증하지않았으므로새메뉴를노출하지않았다.

## 메인루프 우선순위 교정

기존WinMain은highPriority_를0..2로clamp하여3/7/INT_MAX요청을다음프레임TIME_CRITICAL로바꿨다. 원문00439983..004399ef는challenge또는비활성때wanted0,그외raw highPriority를ESI에두고현재OS등급과비교한다. 현재OS1/2→등급1,15→등급2,그외→0이다. 두값이다르면wanted1→OS1,2→OS15,그외→OS0을요청한다. **wanted자체를0으로정규화하지않는다:** 범위밖raw값은현재OS가NORMAL이어도매프레임NORMAL설정요청을반복한다.

PlanNativeThreadPriority는이raw비교만계산한다. 실제부모는항상GetCurrentThread→GetThreadPriority,설정필요때다시GetCurrentThread→SetThreadPriority를호출하고BOOL반환을무시한다. 별도Native96행은두핸들이달라지는입력과FALSE반환도관측했다. 실제OS/API성공검사가아닌격리장치다.

## 검증과 최초 기록

명령238입력/714관측,이름30입력,메인루프96입력/288관측및root별도재관측의각fixture전체바이트가일치했다. 명령238·메인루프96의challenge입력은모두0이며C++도chalPal양성을실행하지않았다. 일반명령Native의전체앞단과C++의명시선행소비/국소bodytrace를실제Uber전체GUI검증으로합산하지않는다.

첫C++6개중3개/8단언실패를보존했다. 우선순위6단언은실제동기reader누락을고쳐통과했다. 나머지2단언은준비Config경로입력이$1을사용해실제원문서식{local.1}과달랐다. 입력만교정하고current.fort/substituted.COL기대와원본fixture는유지했다. std::fill의int→BYTE경고두유형은명시uint8_t로고쳤다. 최종Release경고/오류0, **집중6개/새6개/실패0**이다. 새실행대상은없다.

보호 2673개 모두불변,새fixture3개포함 2676개/fixture856. 다음기준은 extracted/cpp-resume-762/protected-after.json. 제품SHA-256 `3b2fb7d83363830201493da9b59fdbd1b0d6b8eecfa07c59bd39bdb64434b8ca`. 원시build-v1/v2,tests-configuration-v1/v2,Native/재관측은같은extracted경로, [보고서](../../cpppj/configuration-command-input-20261103.json)에최초실패와최종범위를구별했다.

## 새 Windows환경 확인과 캡처 실패

읽기전용감사에서Win10 22H2/19045.7807,부팅11-02 20:24:13·RDP세션1/로그인20:25:09,전경창비NULL·원격오디오2채널1개를확인했다. 이전669의11-01 세션2와594의waveOut0개환경과다르다. 현재모니터는1개1920×1080·96DPI다. 원시조사는environment-audit/에보존했다.

computer-use스킬의Sky node_repl로기존VSCode창66822를fresh선택해캡처했으나FrameArrived timed out이었다. fresh list_apps/get_window로다시선택한1회재시도도window capture timed out이었다. activation/입력/새창/게임/PCM열기는0이다. 다음실제환경변화전같은캡처를반복하지않는다. waveOut1개열거를PCM출력이나청취성공으로쓰지않는다.

새게임/CLI/MP·물리입력/다른앱포커스/DPI/실제PCM·포획/희생·특정적공격·승리·자연완주·전체C++/34/ZIP검사는0이다. 760분류전MP명령관측은그기록에보존한다. 다음은763~769와남은비멀티/호환성이다.
