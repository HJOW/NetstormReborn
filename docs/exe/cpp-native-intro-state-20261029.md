# 새 설치의 Native 인트로 복귀와 배포 런처 — 단위563

2026-10-29 / HJOW-Athlon / Windows10 19045. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이며 이번에 산출물을 바꾸지 않았다. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 다시 읽고 시작했다. 실제 창 실행 확인 예외 호스트다.

562에서 실패한 **설정 없는 배포본의 첫 실행을 복원했다.** 같은 최종 제품의 새 설치·저장된 전체화면 재실행·기본 창 런처가 모두 메인 메뉴에 도달하고 종료0했다. 전체 목표·원본 완전 동일성·Windows11 실기 완료를 뜻하지 않는다.

## 원문과 구현

- `004b6dd0`의 State39(`004b6ebd`)는 current+8c가 비영일 때 cleanup0→Chat 입력 숨김→Chat 숨김→fort/server/boss=1→Core(1,1)→Renderer(1)→HUD1→introMovie 경로를 실행한다. State3 prefix와 달리 listener/Chunk/옵션을 추가 초기화하지 않는다.
- 영상 경로가 있으면 ConfigSet(intro,1)→MovieOpen→immediate40이다. 원본은 영상 열기 반환값을 검사하지 않는다. 경로가 없거나 준비 플래그0이면 immediate41이다.
- State40은 실제 MovieGump가 있으면 next40으로 다음 Poll까지 기다리고, 없으면 immediate41이다. State41은 pending Fort 이름 첫BYTE만 지운 뒤 immediate3이다. 다섯144BYTE 공통 확정은 기존 RawLocalMissionState가 수행한다.
- 디컴파일에서 인자가 생략된 `004b6f72`를 실제 기계어와 대조했다. 요청은 **41**이며, State41의 `00460bb0` 인자는 **빈 pending 이름**이다. 시계 초기화 함수가 아니다.
- [NativeIntroState](../../cpppj/src/client/NativeIntroState.cpp)와 실제 Uber 부모를 연결했다. State40 대기에서는 메인 메뉴를 먼저 공개하지 않는다. 영상 부재는 원본 SmackOpen 실패처럼 재생 객체 없이40→41→3으로 복귀한다.

**사용자 확인(이번 대화):** 인트로 영상은 CD에만 있었으며, CD를 드라이브에 넣은 경우에만 실행 시 재생됐다. 10.78 배포판에는 영상 자료가 없다. 실제 파일 열거도 `originalCD/MOVIE/englishintro2x.smk`, `germanintro2x.smk`만 찾았다. 기본 10.78 배포에 CD 영상을 복사하거나 intro 초기 설정을 강제로 넣지 않았다.

CD 삽입 시의 드라이브 탐색·존재하는 SMK의 디코더/MovieGump·재생/입력/정리는 **미복원**이다. 현재 존재하는 영상 경로가 선택되면 명시 예외를 유지한다. 독립 부모의 active Movie40 재예약 검사를 실제 영상 재생 증거로 확대하지 않는다. UTF-8 영상 경로의 filesystem 변환만 원본 ANSI와 다른 Windows10/11 호환 처리이며 이유를 한국어 주석에 명시했다.

## 검증

[독립 CPU 도구](../../tools/decomp_native_intro_state.py)의17입력은 원본 PE의 State39~41·요청·Movie 존재 조회·다섯 상태 블록 복사를 실행했다. 큰 cleanup/Chat/Core/Renderer/HUD/설정/영상/pending/Guide는 명시 장치이며 State3 진입에서 멈춘다. 정상 분기와9개 선택 경계 중단의 **다섯144BYTE·모드·이름 꼬리·호출 순서**를 C++과 대조했다. 최초 관측과 `--verify` 재관측의 차이는0이다. 게임/OS를 실행한 관측이 아니다.

Release 게임/검사 최종 빌드는 종료0·경고/오류0이다. 첫 빌드의 C++20 u8path 경고1은 UTF-8 path 생성자로 수정했으며 최초 로그를 보존했다. 집중 검사는 NativeIntroState2·NativeMainMenuState6·LocalMissionState3·ClientMainMenuGlobals4의 **15개/실패0**이며 등록3427이다. 전체 C++ 검사는 실행하지 않았다. 새 설치의 실제 State3→39→40→41→3→4와 pending/intro 변경은 별도 검사로 함께 확인했다.

기본 배포 스크립트(NoBuild 없음)도 실제 Windows PowerShell5.1에서 빌드→ZIP 종료0했다. ZIP1365항목·게임 자료1356개·VC DLL3개, 모든 게임 자료 바이트와 원본, 제품 EXE와 BUILD_INFO SHA를 대조했다. 사용자별 options.cfg가 없는 새 추출본을 한글/공백 경로에 만들었다.

1. 최초3실행은 환영 안내→저장 재실행→기본 창 런처가 모두 종료0했다. 최초는 Welcome2까지였고 이를 메인 메뉴 입력 통과로 세지 않는다.
2. 별도 v2 새 추출본의 최종3실행은 첫 실행의 Welcome2에서 실제 Main Menu 버튼을 누른 뒤 메인 메뉴 도달/종료0했다. 두 후속 실행에는 해상도/전체화면 설정을 재주입하지 않았다. 실제 화면 flags는139/139/138로 전체화면/저장 전체화면/강제 창을 확인했고 intro=1 디스크 재조회와 설정 생성도 확인했다.

최종 게임 SHA256은 `3f1c871a68ec5f6e7e5a4ef3721c7ae531fb3ff526d2953320d92de9329bc185`다. 원본/CD·AGENTS·기존fixture **2540파일이 동일**하다(기존fixture720). 실제 실행6개/각3관측은 모두 종료됐고 살아 있는 게임/빌드 자식이 없다. 원본 게임·네트워크·포획/희생·특정 적 공격·승리 재현은 하지 않았다. 원시는 `extracted/cpp-native-intro-563-20261029/`, [집계](../../cpppj/native-intro-runtime-20261029.json)와 [독립 관측](../../cpppj/native-intro-state-native-20261029.json)을 따른다.

562 새 설치 차단은 해결했다. 현재563 제품의 전체34경로 통합은 미실행이며 마지막34/34 근거는560제품/561단위다. 이후 남은 UI/Edit·시나리오·수명·호환성을 진행한다. 비영 traceToChat 스타일·메타맵 번개, 저장상태 복구·남은 Native3 선택, 데모360초 반복도 남는다. 자연 완주/사용자 개입·공중3종 전용 검사·신규 MP/통신 검사와 기존 사용자 보류는 유지한다.
