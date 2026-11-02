# 771 — Config 실제 파일 읽기와 Append 수명

2026-11-03 HJOW-Athlon. 마지막 전체 정밀 디컴파일 HJOW-Athlon / 2026-10-23 유지. 단위771 완료·전체 목표 미완료다.

실제 Patch1078 Client의 ConfigInterface에만 ConfigFileLoadHooks를 설치했다. 공유 Reader는 그대로 유지하고 Config만 현재 VFS의 실제 FILE/키를 사용한다. startup/일반설정·언어·Registry 임시미션·일반 Mission script·TellFileMenus 새등록/EmbeddedTell/Tell fallback이 같은 LoadFile을 쓴다. 명시 Editor Mission source와 CD1072·독립 값형 reader/공유Fort snapshot은 기존 namespace와 계약을 유지한다. 검색은 find시점의 현재 게임root를 빌린다.

실제40380은length0/check1/extra1로41baa0을 부르고 FILE/키 반납 뒤에도 같은 소유 할당을 유지한다. 물리 첫BYTE0→세번째BYTE0→signed length>0이면 논리본문을XOR하고 buffer[length]=0→같은 바이트 Append→정상복귀 뒤 free 순서다. raw 부모의 Append 예외에 free를 합성하지 않는다. host 바깥vector/FILE RAII의 추가 예외 정리와 부분 fread를 Append 전에 거부하는 경계는 별도다.

기본 Config.LoadBytes의 size>=3 XOR/서명mQdsT/BOM/첫NUL/Windows1252 해석은 보존하고 뒤처리만 AppendDecodedBytes로 추출했다. Native40380은 이미 해독한 바이트를 그 함수로 보내므로 두 번 XOR하지 않는다. 길이1/2의 미작성 세번째 물리BYTE·host0padding·Native205seed/부분암호scratch101의 차이를 전체 미정메모리 동등성으로 확대하지 않는다.

40410의@토큰은@를 보존한 바깥Probe 뒤40380의안쪽Probe를 수행한다. 파일본문을 수집한 inline글보다 먼저 더한다. 기본 값형 reader는 기존 한 번 읽기를 유지한다. 실제Client ConfigPathLoad의 고리 기대9→2는 상대성공의이름1+Probe2+Open2, 상대부재의이름1+Probe3, 직접성공의이름1+Probe1+Open1로 총12칸이 되어 정당하다. 기존12행 원문fixture와 기본reader 기대는 바꾸지 않았다.

원문68입력/136관측과 준비 재관측136·root 추가136회가 일치했다. 실제40380/41baa0/조건XOR/끝NUL/Append후free와marker0의4f93a0을 실행하며 FILE/Open/Probe/new/ConfigAppend본문은 명시 경계다. 독립 읽기 감사는 원문 직접caller10개와40410의부모, 준비19파일SHA를 대조해 확정 결함0이었다. 컴파일/실행한 결론과 구별한다.

최종 Release 경고/오류0, 설정·경로·시작파일 집중 **38개(새7개)/실패0**. 실제현재TAFF와 공유snapshot분리·언어/동적미션 재읽기·명시Editor source·짧은암호I/O 정리·빈/LF/NUL/해독본문과 기존 Config·Mission/Fort 선택·FileName/TechnicalHelp·StartupIO를 확인했다. 실제 창/외부Help프로그램을 열거나 게임/통신을 시작하지 않았다.

최초38개 중1실패는 검사파일명 nul.cfg가 Windows 예약 NUL 장치로 해석되어 쓰기가 버려진 입력 오류였다. [Microsoft 예약명 규칙](https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file)과 실제 파일이 없고 읽은 본문은 빈 CRLF뿐인 진단을 보존했다. 이름을 nul-content.cfg로 바꿨으며 제품/기존fixture/원문68은 유지했다. v2의 단일 NUL 진단은1실패로 남기고 암호회귀 성공으로 세지 않는다. 그 진단 추가행의 주석에 붙었던 후행 encoded 확인은 v3 전에 개행해 복구했다. 최종38개는 이 확인까지 실행한다. 병합 과정에서 정규화된 기존10파일의 mixed CRLF/LF도 같은 줄 바이트/내용 동등성을 검증해 복구하고 최종v3를 다시 빌드했다(eol-restoration.json).

기존 보호2697개 모두 불변·새fixture1 포함2698개/fixture878. 다음 기준은 extracted/cpp-resume-771/protected-after.json이며 제품 SHA-256은 `201c6a0dab30ba8fce0e4a0eb68b5797b4fd633b42f3693439a573b9bf131d84`다. [보고서](../../cpppj/config-live-read-20261103.json)와 extracted의준비/반복Native·독립감사·build/tests를 따른다. 새 실행대상0/기존66개·전체71대상 유지다.

새 게임/GUI/CLI/MP·물리/PCM·공중3종전용검사·포획/희생·특정적공격·승리·자연완주·전체C++/34/ZIP 검사는0이다. 다음은 일반 AI 명령과 남은 파일/진단/호환성이다.
