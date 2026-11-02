# cpppj 단위732 — Startup의 위치 확정 뒤 파일명 작성

2026-11-02 / HJOW-Athlon / Windows 10. 마지막 전체 정밀 디컴파일은 HJOW-Athlon·2026-10-23이며 새 디컴파일은 없다. 실행 제품은 **732 / 11.01**이다.

## 실제 복원

확정한 dataPrefix에서 Startup이 원문459c60으로 각 경로를 만들고 즉시 지역 소유 문자열로 복사하도록 연결했다. 순서는 **identity→options→user→dev→setup→demo→guild**다. 실제 Load 인자는 identity→user→options→dev→guild→setup이며 demo는 복사/한 칸 예약만 하고 읽지 않는다. identity가 비어 있으면 그 작성 한 칸을 생략한다. identity는 파일/-3과 cfg확장자/-4를 구별해 `.odd.` 같은 입력도 단순 연결의 `.odd..cfg` 대신 원문 `.odd.cfg`로 작성한다.

731의 같은 Client/VFS 소유자와 실제 Load에 연결했다. 경로를 고리 뷰로 보관하지 않으므로 Reader가 고리를 열 번 덮어도 준비한 다른 인자가 바뀌지 않는다. 독립 값형 ConfigInterface API의 기존 읽기 장치 이름 계약은 유지한다.

## 원문 구간과 검증 범위

[새 원문 도구](../../tools/decomp_startup_file_names.py)는 **441a44~441c5e**의 위치 확정 뒤 구간과 실제440e00/FileSpec/이름getter/CRT를 실행했다. prefix는 이미 정한 입력이며 getenv·고정sprintf·설정파서/읽기/로그·Find/Close만 명시 경계다. 앞선6후보 위치 탐색·CWD/TAFF등록과 뒤 Set/정리·함수 전체 ABI·실제OS 관측은 포함하지 않는다.

12개의 prefix/IDENTITY/USER 입력×초기번호0/9의 **24행**을 두 FPCW에서 관측한 **48회**와 재관측이 일치했다. 환경 fallback/로컬 인자 사본·작성 순서·전체10×256BYTE 고리를 고정했다. 이름 작성6/7칸과 실제Load의 이름5/6칸을 합친 상대 소모는 **11/13칸**이다. 이는 전체Startup의 선행 구간까지 합친 총수가 아니다. C++StartupOptions.identity는 호출자가 이미 선택한 값이며 Native의 getenv 응답과 대응해 검사했다([원문 보고서](../../cpppj/startup-file-names-native-20261102.json)).

독립 원문 기준의 검사는 기존731제품에서 **3개/262단언 실패**로 누락을 검출했다. 연결 뒤 새3개·기존22개 **집중25개/실패0**이다. 새 검사는24행의 읽기 인자/전체10칸, Reader의10칸 재진입, 실제 Unicode Client의 Startup→옵션저장→재적재를 확인한다. 실제 파일6개가 모두 기본 디스크에 있는 명시 조건에서 Startup은 작성7+각파일(이름1/읽기2)×6=25칸, 후속 저장4칸/읽기2칸이다. 동결 행은 기존252+새24의 총276개다.

Release 최종 경고·오류0이다. 실제 실행 파일의 설정CLI는 `SCREENW=1024`·종료0이며1프로세스/게임프레임0이다. EXE SHA-256은 `d25df1aec10ae397355ec4f9e9842a1e2318e2890474712cadbe8721985821e6`다. 원시는 `extracted/cpp-startup-names-732/`, 통합 근거는 [보고서](../../cpppj/startup-file-names-20261102.json)다.

## 보호와 남은 작업

시작 보호 **2646개(AGENTS1·원본1394·CD425·기존fixture826) 모두 불변**이다. 새fixture1개로 현재2647개/fixture827개다. 이 작업의 보호파일 수정은0이다.

선행 위치 탐색/CWD/TARC 등록의 고리·다른FileSpec getter·Template·일반FILE/TAFF/보조 실패/부분I/O/SEH 수명은 남는다. 이번 위치 확정 뒤 경로를 전체Startup 완료로 세지 않는다. Windows10/11의 기존UTF8/W 경계는 그대로 유지한다. 10.78 기준이며10.72의 대응Startup구간은 이번에 새로 관측하지 않았다.

새게임/GUI·전체34/전체C++·ZIP·MP/통신 검사는0이다. 포획/희생·특정 적 공격/승리·공중3종 전용 검사도 하지 않았다. 마지막 실제GUI는728의Edit/메인 왕복이며 자연 완주/전체 동일성은 사용자 확인 대상이다. 전체1~8차 목표는 진행 중이다.
