# cpppj Technical Help 가용성·파일 경로·실행 — 단위602

2026-10-30, HJOW-Athlon(192.168.0.94, Windows 10 19045). AGENTS.md/두 인계를 다시 읽었고 마지막 전체 정밀 디컴파일은 같은 PC의2026-10-23이다. 원본/CD·AGENTS·정밀 C와 함수 표는 읽기만 했다.

## 실제 메뉴 연결

원본00435ef0은 로컬 출력 버퍼 두 개로00435e40을 부르는 가용성 래퍼다. 실제 실행은 별도00435f10이다. 기존 About 메뉴는 두 Config 문자열의 ReadString만 확인하고 파일 해석을 생략했으며02000049 명령도 실행하지 못했다. 새 `TechnicalHelp` 부모와 현재 Client 장치를 공통 메뉴에 연결했다. 원래 스크립트 이름은 `TechHelp`이며54281c의02000049를 사용한다.

00435e40은 두 출력 첫문자를 먼저 비우고 `techHelpExe`, `techHelpFile`을 모두 조회한다. 반환은 short circuit 논리AND가 아닌 DWORD bitwise AND다. 두 반환이 겹칠 때만 실행 파일을0041ad10으로 해석해 먼저 복사하고, 다음 도움말 파일을 해석한다. 두 번째 실패에도 앞서 복사한 실행 파일은 남는다. null 경로 실패와 비null 빈 문자열을 구별한다.

00435f10은 위 가용성을 다시 계산하고 성공 때만004dd6b0(program,file,0)을 호출한다. program이 비어 있지 않으면 program+공백 하나+file, 비어 있으면 file만 실행한다. 추가 인용/본문 치환·브라우저 fallback을 넣지 않는다. 반환은 프로세스 시작 성공 여부가 아니라 앞선 가용성이다. 현재 Client는601의 비대기 Windows W 실행 장치를 공유하며 실행 장치의 실패도 이 반환을 바꾸지 않는다. release는 실행하지 않고 같은 명령으로 소비한다.

## 파일 경로 경계

0041ad10은 FileSpec 생성/Device 후 장치 접두가 있으면 실제 존재를 확인하지 않고 원래 입력을 반환한다(출처5). 그 외에는 기본 디스크 rb 열기/닫기(1)→기본 TAFF 이름(2)→보조/CD 디스크 rb 열기/닫기(3)→보조 TAFF 이름(4) 순서다. 실패하면 출처 출력의 이전 값을 유지한다. TAFF 결과는 추출된 임시 파일 경로가 아니라 원래 이름이다. 아카이브의 실행 가능 파일을 임의 추출하지 않는다.

`JoinExternalFileRoot`는00459c60의 -2/root/-3/name/0 호출만 복원한다. 기본 `.\`, 구분자 변환·선행 구분자 소비와 끝 구분자 한 개만 지우는 원문을 유지하며 `lexically_normal`로 다른 경로로 만들지 않는다. FileSpec 장치 판정도 콜론/UNC와 원래 패턴 분기를 따른다. 패턴이 있는 경우 실제 FindFirstFileW의 첫 이름과 원래 Find 핸들 수명을 사용한다.

현재 Client는 기본 `netstorm.tarc` 집합만 등록한다. 새 `BaseFileSystem::ContainsArchiveOnly`는 실제 등록된 이름 메타데이터만 조회하며 TryReadArchiveOnly처럼 내용/복호화를 수행하지 않는다. 현재 보조 TAFF 집합은 실제로 비어 있으므로 마지막 조회는 실패하며 CD 디스크는 현재 `CDDir`을 별도로 읽는다. 원본의 보조 TAFF 등록·전체 BaseFile 확장은 후속 경계다. 이번에 존재하지 않는 등록 집합이나 파일 추출 기능을 합성하지 않았다.

원본 ANSI/MAX_PATH/고정 버퍼 대신 UTF-8 경로를 W API/동적 문자열로 유지하는 Win10/11 차이는 코드에 명시했다. 이 변경은 DirectDraw 전체화면 수정 검증을 대신하지 않는다.

## 원문 검증

[Native 집계](../../cpppj/technical-help-native-20261030.json)와 [관측 도구](../../tools/decomp_technical_help.py)는435e40/435ef0/435f10의84사례,41ad10 선택 부모64사례, 별도459c60 조합28사례의 총176사례를 두 독립 emulator에서 실행했다. 총352관측/재관측 차이0이다. 첫 두 fixture는 확장 관측 때 덮지 않고 바이트가 같음을 검증했으며 세 번째 조합 fixture만 새로 만들었다.

부모 안의 Config/파일 경로/Filespec·조합/CRT 파일/TAFF/실행은 명시 응답이다. 별도 조합 자식은 실제 CPU로 검증했지만 파일 선택 부모와 모든 자식의 전체 통합·실제 OS 전체 동일성 증거로 확대하지 않는다. 효과 진입 중단의 trace/부분 출력, 출처 포인터 유무와 실패 시77 보존도 구별한다.

최종 C++/실제 로컬 도우미 결과는 [실행 집계](../../cpppj/technical-help-runtime-20261030.json)에 기록한다. 전체 C++ 검사를 실행하거나 게임 GUI/브라우저를 실행한 결과로 합산하지 않는다. 첫 게임 빌드는 성공했지만 새 TechnicalHelpClientTests.cpp의 netstorm::client 지정 누락으로 검사 빌드가 실패했다. 검사 소스만 고쳐 증분 재빌드하며 첫 오류 로그를 보존했다. 초기 보호 스냅샷 저장은 성공했지만 뒤의 인계 요약 콘솔 출력은 cp949 UnicodeEncodeError로 끝났으며 이후 `python -X utf8`로 교정했다. 보호 자료나 인계 내용을 손상시키지 않았다.

최종 게임/검사 빌드는 종료0·경고/오류0이다. 같은 최종 검사 EXE의 TechnicalHelp5/BaseFile4/Taff1/GameMenuBar2/RawMenuControls16/HelpLinkPolicy2인 고유30개가 실패0이며 등록3503개다. 실제 Client는 Run 없이 현재 Config/등록 TAFF를 사용했다. 한글 경로 로컬 도우미는 조회 전/뒤에 실행되지 않았고, 실행 직전 설정을 first.hlp→second.hlp로 바꾼 결과 실제 명령에는 second.hlp가 기록됐다. 존재하지 않는 상대 파일의 제외, 장치 접두가 있는 없는 파일의 가용성 유지, 실제 보조/CD 디스크의 first.hlp와 기본 TAFF priest.type 메타데이터도 확인했다. 자식 종료코드는 별도로 수집하지 않았고 검사/도우미 프로세스가 남지 않았음을 관측했다.

## CD와 후속 범위

정밀 C의 CD10.72 대응은 가용성00485950→00485970, 실행00485a60, 파일 경로004bb6d0이다. 이번 부모는10.78만 허용하고 CD는 명시 거부한다. CD 구현/실행을 완료한 것으로 세지 않는다.

원시 경로는 `extracted/cpp-technical-help-602-20261030/`다. 이번에 새 ZIP/실제 게임 GUI는 없으며 마지막 실제 검증 ZIP은600, 전체34 진입은595제품/596이다. 다음 기능 묶음 검증에서 실제 Help 메뉴와 배포본을 갱신한다. 보조 TAFF 등록과 남은 UI/Edit·Native·시나리오 조건/전체 동일성·Win11/PCM/활성 사용자 전체화면·자연 완주·후순위MP 무시험/4~7차는 미완료다. 포획/희생·특정 적 공격/승리 재현, 공중3종 전용/신규MP 검사는 하지 않았다.

보호2569개(기존 fixture746 포함)는 모두 불변이며 현재 fixture는749개다. 현재 게임 SHA256은 `8a977634fa15b47c7bfe7cf8c821c0a77118503645bfbca6ac155bc9dfd78763`이다. 검사 도우미는 배포 스크립트의 단일 게임 EXE/지정 DLL 목록에 포함되지 않는다.
