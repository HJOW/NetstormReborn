# 초기 미션 다음 단계 조건 표시 — 구현1372

2026-11-27, HJOW-Athlon. 마지막 디컴파일 호스트도 HJOW-Athlon이다. 기존 자료/복원 본체를 사용했으며 새 디컴파일과 원본 게임 실행은 없다. 조건 조사와40단계 표는 [1371](cpp-tutorial-objectives-1371-20261127.md)을 따른다.

## 동작과 구현

공식 Tutorial1~6에서 다음 단계의 알림창/완료창을 열기 위해 할 일을 현재 지도 영역 우측 하단에 계속 표시한다. 배경·패널 없이 보통체 글자와 한 픽셀 그림자만 그린다. 생산 패널/미니맵을 제외한 현재 지도 영역에8픽셀 여백을 두고, 최대440픽셀 폭으로 줄바꿈한다. 영문 단어와4바이트 한글/일본어 표시 문자 경계를 보존한다.

표시는 기존 TextGump/StyleText/FontStore/부분 갱신을 사용한다. 한국어는 기존 외부 갈무리11, 일본어는 기존 외부 글꼴을 사용한다. 기존 언어 선택/Translate에 맞춰 영어 문구38개와 대기 문구1개를 한국어·일본어 xlat에 추가했다(각536→575키, 중복0). 같은 문구를 공유하는 단계를 포함해 카탈로그는40행이다.

현재 gate의 단계/lockFrames/timerSeconds는 읽기만 한다. 원본 Advance·Tell·다이얼로그 정지·완료 판정의 순서와 값을 변경하지 않는다. 단계 예약 또는 Tutorial2 F/H·Tutorial5 C의 기존 타이머 대기에는 다음 안내를 기다리라는 문구를 쓴다. 안내창·도움말·지식창·게임 메뉴를 읽는 동안은 겹치지 않게 숨기고 닫으면 복원한다. 완료·이탈·재시작·다른 미션 전환·루트/해상도 변경은 기존 노드를 지우거나 다시 배치한다. 일반 캠페인의 Tutorial 유형만으로 표시하지 않고 공식 이름/번호·비데모·비편집 여부도 검사한다.

안내는 입력/도움말 선택 후보에서 제외한다. 원본 flags에 비트를 추가하는 최초 구현은 기존 독립 PE GumpEvents 입력119행과 충돌했다(세 판본의 검사6단언 실패). 최초 결과를 extracted/tests-tutorial-objective-1372-first.log에 보존했다. 이를 제거하고 공통 입력 처리기의 별도 토큰+생성serial 목록을 사용한다. 원본 플래그의 상위 비트도 보존하고 토큰 번호 재사용을 다른 객체로 구별한다. 기존 fixture/기대값은 수정하지 않았다.

## 검사 도구 사용법

Netstorm.exe 이외 새 검사 실행 파일은 netstorm_tutorial_objective_tests.exe다. 창 없이 신규 안내 범위/타이머/줄바꿈/입력 투과 검사와 기존 TutorialOne/Two/Later·MissionFrameGate·GumpEvents 독립 PE 회귀를 실행한다.

```powershell
cpppj/build/bin/Release/netstorm_tutorial_objective_tests.exe
cpppj/build/bin/Release/netstorm_tutorial_objective_tests.exe --list
cpppj/build/bin/Release/netstorm_tutorial_objective_tests.exe --filter TutorialObjective
```

실제 게임 화면 검사는 tools/cpp_tutorial_objective_verify.py를 쓴다. 실제 창이 열리므로 AGENTS.md의 실행 허용 규칙을 따른다. 새 output 디렉터리 아래 사용자 데이터·원시 MCP·보고·PNG·종료 결과를 보존한다. 실제 사용자 저장소/원본 파일을 쓰지 않는다. 단계나 승패를 주입하지 않고 초기 안내의 실제 다음/확인 버튼만 누른다.

```powershell
python -X utf8 tools/cpp_tutorial_objective_verify.py --output extracted/tutorial-check-new --language korean --progress
python -X utf8 tools/cpp_tutorial_objective_verify.py --output extracted/tutorial-check-wide-new --language japanese --width 1920 --height 1080
```

--mission(기본 tutorial1), --game(기본 보호 자료 사본 extracted/cpp-reset-all-1325/game), --exe, --width, --height, --fullscreen을 지정할 수 있다. --progress는 Tutorial1에서 실제 F4 입력으로 A→B를 확인한다. 포획/희생·특정 적 공격·승리 입력은 사용하지 않는다.

## 검증 결과

Release 제품/집중 대상 빌드 종료0, 집중22사례/실패0이다. 기존 TutorialOne/Two/Later·공통 gate·세 판본 GumpEvents 독립 PE 회귀를 그대로 통과했다. 신규 검사는40단계/완료/범위 제외·예약 상태 읽기 전용·다국어 줄바꿈·원본 상위 flags 보존·입력 투과·생성serial 재사용을 확인한다. 재컴파일 중 기존 TextClipboard.cpp의 C4244 경고가 있었고 이번 변경 파일의 새 컴파일 오류는 없다.

실제 GUI19경로 모두 종료0이다. Tutorial1의 영어·한국어·일본어 ×1024×768(4:3)·1920×1080(16:9)·1280×800(16:10)의 창 모드9경로, 전체화면3경로(영어16:10·한국어16:9·일본어4:3), Tutorial2~6 한국어 초기 진입5경로, 일반 캠페인 thewarbegins의 안내 비표시1경로, 한국어 재시작/전환/이탈1경로를 확인했다. 전체화면은 실제 screen_mode=139(전체화면 비트0x01)도 확인했다. 물리 DPI·다른 모니터·전체 화면/소리 동일성은 기존 사용자 확인 범위다.

영어 Tutorial1에서는 실제 F4 키로 A→B가 전환되고 안내가 다리 총8개로 갱신됐다. F8로 B 안내를 다시 열면 목표 글자가 숨겨지고 닫으면 같은 B 문구로 복원됐다. 한국어 실행에서는 같은 MissionRestart 경로의 재시작 뒤 초기 A 문구 복원을 추가로 확인했다. 기존 MissionBegin 명령으로 Tutorial2에 전환하면 신전 건설 문구가 나오고, LeaveBattle 이탈 뒤에는 안내가 비었다. 상태/완료/승패/비용 값을 주입하지 않았다. 첫 검증 도구의 MORE 라벨 누락과 제품에 없는 --fullscreen 인자는 도구만 교정했고 최초 실패 결과도 보존했다. 전체화면은 기존 startInFullScreen 설정으로 실행한다.

화면을 직접 확인해 배경 없는 우측 하단 글자·생산 창/미니맵 비겹침을 확인했다. [한국어](../screens/cpp-tutorial-objectives-1372-20261127/korean.png)·[영어](../screens/cpp-tutorial-objectives-1372-20261127/english.png)·[일본어16:10](../screens/cpp-tutorial-objectives-1372-20261127/japanese.png)·[한국어 전체화면16:9](../screens/cpp-tutorial-objectives-1372-20261127/korean-fullscreen.png).

보호 원본/CD·fixture·AGENTS.md·README.md의2,810파일은 실행 전후 SHA-256이 같다. EXE SHA-256은 b400a38a93a54c89d6b90e13fefa1f31d49656c203dbeb785e8bfd788a0618a2다. [기계 판독 결과](../../cpppj/tutorial-objectives-1372-20261127.json), 원시는 extracted/tutorial-objective-1372와 extracted/build-tutorial-objective-1372*.log·tests-tutorial-objective-1372*.log에 있다.

TUT-1~5의 구현·허용 범위 검증을 완료했다. 튜토리얼 후반 포획/희생·특정 적 공격/승리의 자연 전환은 사용자 지시로 실행하지 않았으며 사용자 확인으로 남긴다. LAN 후속과 최신 변경을 포함한 배포 ZIP은 다음 작업이다. 전체 cpppj 목표나 최신 M8 관문을 완료한 것으로 확대하지 않는다.
