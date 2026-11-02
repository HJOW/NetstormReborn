# 767 — QuickHelp 최초 실시간 파일과 성공 캐시

2026-11-03 HJOW-Athlon. 마지막 전체 정밀 디컴파일 HJOW-Athlon/2026-10-23 유지. 단위767 완료·전체 목표 미완료다.

## 같은 Client의 최초 Help 파일

ReadClientQuickHelpFile은실제UberGump의help.load 한곳만명시41baa0에접합한다. ReadQuickHelpFileBytes는4c5380의초기length0/check0/extra2를같은현재VFS.Open/FILE/키/공유파일명고리로전달한다. 필수파일에사전Probe를붙이거나현재Open의typed예외를무진단optional미발견으로바꾸지않는다. 일반Files.Read/TryRead의snapshot·optional과OS직접ReadFileBytes는변경하지않았다.

기존QuickHelpWindow는성공한첫본문을소유하고그끝에LF를한번추가하며std::string의NUL을쓴다. 재생성은같은성공cache를재사용한다. 최초실패는cache/id를확정하지않고원문의이미생성된자식효과를남겨다음파일준비후재시도가가능하다. 제품창의기존LF/NUL/cache몸체는바꾸지않았다.

실제reader는논리본문만반환한다. 최소4/extra2의hostvector 0padding을외부본문에노출하지않는다. 짧은암호fread는명시실패를기록해현재FILE/키가반납된뒤예외로거부하며원문미정tail을정상Help로표시하지않는다. 바깥RAII는미완성Open의남은OS자원도추가반납하는Windows호환경계다.

## 검증/한계

원문62입력/두정밀도124관측과준비재관측124·root추가124회가일치했다. 실제4c5380→41baa0/길이/읽기/소멸·LF/NUL·성공cache를관측했고외부GUI/Config/Open/FILE/new는명시경계다. 예외는41baa0 marker0의실제4f93a0 funclet만호출하며전체SEH dispatcher/바깥창ctor부분GUI반납이나32비트FILE24BYTE자체의동일성은아니다.

최초Native는반환후의지역File스택을읽었으므로prepared-observations에보존하고최종관측을4c580e의전체읽기호출직후로교정했다. 고정fixture와두준비관측/root추가관측은이교정본이다. 부분암호scratch101seed/미정padding을호스트메모리와동일하다고주장하지않는다.

최초build-v1은검사의기존kSource와새using namespace netstorm::o가충돌해C2872/연쇄오류로실패했다. 검사상수만kWindowHelpSource로개명했고주소/기대fixture/제품을바꾸지않았다. 최종build-v2 Release경고/오류0,집중 **12개(새6개)/실패0**. 현재backingTAFF교체와기존snapshot분리·짧은암호읽기뒤rename/재시도·현재보조root·직접Unicode·필수missing/빈파일재시도·단일성공cache/embeddedNUL/LF와기존Startup I/O snapshot/deferred등록을확인했다. 기존QuickHelpWindow_Native98전체모드는새로실행하지않았다. 새실행대상0/기존66개·전체71대상이다.

기존보호2688개불변·새fixture1 포함2689개/fixture869. 다음기준extracted/cpp-resume-767/protected-after.json,제품SHA-256 `f1cff102b20417dccd8b6c4d2fec96864ea78197d3cc0e1600a1d692d74a3071`. [보고서](../../cpppj/quick-help-live-read-20261103.json)와같은extracted의build/tests/준비최초·교정·추가관측을따른다.

새게임/GUI/CLI/MP·물리/PCM·포획/희생·특정적공격·승리·자연완주·전체C++/34/ZIP검사는0이다. 다음768 표시와각직접reader/일반명령·호환성을이어간다.
