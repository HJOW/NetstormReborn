# cpppj 단위731 — 설정 Load의 이름과 FileSpec 검색 수명

2026-11-02 / HJOW-Athlon / Windows 10. 마지막 전체 정밀 디컴파일은 HJOW-Athlon·2026-10-23이며 새 디컴파일은 없다. 실행 제품은 **731 / 11.01**이다.

## 실제 연결

원문440e00은 첫@ 전까지 인자를 본문으로 넣고, 그 뒤에는 비어 있지 않은 모든 인자를 파일로 처리한다. 각 파일은 실제 FileSpec 생성→459b20의 이름 한 칸→section 추가→원래 이름으로 내용 읽기→FileSpec 반납 순서다. 현재 `ConfigInterface`가 실제 Client/VFS의 같은 경로 소유자를 보유하도록 연결했다. CLI/자산 설정5곳도 각 VFS 소유자를 공유한다. 독립1인자 API의 기존 값형 계약은 유지하며 Load 인자는 원문처럼 첫NUL 뒤 물리를 읽지 않는다.

공통 `FileSpecQuery`는 접두 .\/구분자 처리와 원래 입력 패턴의 FindFirst, 정상/실패 핸들의 반납 수명을 소유한다. 설정 Reader가 반환하거나 예외로 나가기 전까지 검색 핸들이 남으며 생성 중 이름 할당 실패도 반납한다. 같은 역할의 실제 Help 내부 사본은 이 공통 코드로 교체했다. 원본 ANSI 대신 Windows10/11 UTF-8/W 검색을 쓰는 차이는 코드에 주석으로 명시했다.

와일드카드가 있는 입력은 정규화한 경로가 아니라 **원래 입력**으로 다시 검색한다. 따라서 `d/*.cfg`나 `*.cfg` 검색이 성공해도 현재 FileSpec에 backslash/colon이 없으면459b20은 빈 이름을 반환한다. 임의 정상화로 이 원문 결과를 바꾸지 않았다. 실패 FindFirst의 INVALID_HANDLE_VALUE도 비NULL이면 끝에서 FindClose를 요청한다.

## 원문과 집중 확인

[새 독립 도구](../../tools/decomp_config_path_load.py)는 실제440e00/45a220/459610/4595f0/459b20과 CRT를 실행했다. 본문/파일 전환·빈 인자·NUL·절대/UNC/상위BYTE/255BYTE·와일드카드 성공/실패의 **12입력×두FPCW=24관측**과 재관측이 일치했다. 원문 Clear/Section/Inline/Read/로그 및 FindFirst·FindClose는 명시 경계다. 실제 OS/설정 파서/예외unwind 전체 관측으로 확대하지 않는다. 최초12행은 새 fixture로 고정했다([원문 보고서](../../cpppj/config-path-load-native-20261102.json)).

새3개·기존19개 **집중22개/실패0**이다. 새 검사는12행의 section·Find→Read→Close 순서와 같은 고리 소모, Reader 예외 후 부분 section/한 칸 보존과 검색 반납, 실제 Unicode Client의 상대/미발견/직접 읽기 총9칸 및 W 검색을 확인한다. 기존 ConfigInterface12개·이름 getter3개·Help4개도 통과했다. 동결 행은12+64+84+64+28의 총252개이며 별도 인코딩256BYTE 확인을 이 행 수에 합산하지 않는다.

첫 빌드는 기존 익명 함수 FileNamePart와 공통 부품 구조체 FileNamePart의 이름 충돌로 실패했다. 보조 함수만 ConfigFileNamePart로 구분해 정정했고 최종 Release 경고·오류0이다. 실제 실행 파일의 `--config-get originals SCREENW`는 **1024·종료0**이며 CLI1프로세스/게임프레임0이다. EXE SHA-256은 `2220e48e62e789d1595c4f1c672290c37216cd904b85eeff6427fdb2d46176d0`다. 원시는 `extracted/cpp-config-path-load-731/`, 통합 근거는 [보고서](../../cpppj/config-path-load-20261102.json)다.

## 보호·다음 범위

시작 보호 **2645개(AGENTS1·원본1394·CD425·기존fixture825) 모두 불변**이다. 새fixture1개로 현재2646개/fixture826개다. 보호파일 수정은0이며 원문/CD는 읽기 참조만 했다.

이번은440e00의 파일명/검색 수명이다. Startup441790의 선행 파일 위치 탐색과 별도 파일명 작성 순서, Template·다른 getter·일반FILE/TAFF/보조 실패·부분I/O/SEH 수명은 남는다. FileSpecQuery의 현재 backslash 생성/조회 부분을 전체264BYTE ABI/FindNext·VFS 와일드카드 전체 수명 완료로 세지 않는다.

새게임/GUI·전체34/전체C++·ZIP·MP/통신 검사는0이다. 포획/희생·특정 적 공격/승리·공중3종 전용 검사는 하지 않았다. 마지막 실제 GUI는728의Edit/메인 왕복이며 자연 완주/전체 동일성은 사용자 확인 대상이다. 전체1~8차 목표는 진행 중이다.
