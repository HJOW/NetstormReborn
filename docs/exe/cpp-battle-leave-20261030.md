# 실제 중단의 Native State3 복귀 — 단위588

2026-10-30 / HJOW-Athlon / Windows10 19045. 마지막 전체 정밀 디컴파일 HJOW-Athlon / 2026-10-23과 현재 PC가 같다. 원본/정밀 소스는 변경하지 않았다. 587의34경로 검사는 고정한586 제품으로 별도 완료했으며, 이번 새 제품의 검증과 구별한다.

일반 중단은 사건 수신 중 호스트 `MainMenu()`에서 즉시 World를 지우던 경로 대신, 원본의 ScriptBreak/다음 상태 예약을 거쳐 다음 Poll에서 실제 State3를 실행한다. State3는 살아 있는 Session의 로컬9에 종료 통지를 예약하고 공통 정리→HUD/이름/Guide/안내→기존 tail→공통 상태 확정→메인 표시를 실행한다. 원본에 없는 강제 Flush/동기 수신은 넣지 않았다.

## 원문과 연결

- 생성자 `0040aa60`와 규격 `0052ee16`은 type52/version1/length5다. 생성자는 앞4BYTE만 쓰고 State3가 다섯째 BYTE를0으로 쓴다. 실제 통지는 `01 52 05 00 00`이다.
- 수신기 `004a71a0`는 본문을 읽지 않고 현재 sender의 Player+50 DWORD에4를 합친다. 같은 Login/Edit 분배 표에 빈 observer/버전1/정책0/실제 수신기를 등록했다. Edit의 Session 대여 반납 전에52h receive도 해제하며 다른 소유자 충돌은 등록 전에 거부한다.
- `004b3540`은 next[0]만3으로 쓴다. LeaveBattle 이유2는 ScriptBreak1 뒤 이 예약만 실행하고 저장 삭제/공통 abort를 건너뛴다.
- 실제 ABORT의 “Main Menu”는 이유1이다. 공통 `004b6350`은 AbortPending1, 모드별 next3/6 및 outcome, next/immediate/current의+84를 원문 순서로 쓴 뒤 저장 파일 경로/삭제 장치를 호출한다. 단독/편집은 next3/outcome0이다.
- 이유3/9는 원문처럼 선행 저장 삭제→SourceType0→공통 부모의 다시 삭제를 보존했다. 실제 Editor 상태는 DWORD 참조를 사용한다. 원격 페널티가 선택되면 미연결을 명시하며 즉시 메뉴 성공으로 바꾸지 않는다. 이유0 이하 Tell/CD판 전체와 원격 State6/재접속은 잔여다.

## 독립 관측과 검사

제품 작성 전에 [첫 도구](../../tools/decomp_battle_leave.py)로 헤더 꼬리/로컬 Player1 전체/다섯 상태 전체/State3 송신16행을, [추가 도구](../../tools/decomp_battle_abort_state.py)로 단독/편집 공통 부모와 path/delete 중단8행을 고정했다. 각각 두 차례 재관측 차이0으로 **24행/총72관측**이다. 원본 게임/OS/창/네트워크/파일 삭제는 실행하지 않았다.

추가 도구의 최초 실행은 디컴파일 이름 `thunk_FUN_004e58ba`를 실제 호출 주소로 잘못 지정해 멈췄다. 기존 State3 관측기와 실제 호출이 사용하는 `004e58e4`로 경계를 수정했고 최초 실패를 보존했다. 성공 후 고정한 fixture는 변경하지 않았다.

새 검사3개와 인접 상태/정리/수명/입력/전송/Edit 회귀77개, **고유80개/실패0**이다. 등록 검사는3475개다. 첫 빌드는 진행 중 추가한 fixture 정의가 CMake에 재생성되기 전이라 실패했고, 둘째는 Editor DWORD를 bool참조로 빌린 축소 변환 오류였다. 수정 후 셋째 검사/게임 빌드와 기본 배포 스크립트의 증분 게임 빌드가 통과했으며 최종 컴파일 경고/오류0이다. 초기 로그는 남겼다.

## 같은 새 배포본의 실제 실행

[실행 도구](../../tools/cpp_battle_leave_smoke.py)로 새 ZIP 압축 해제본의 TheWarBegins에서 실제 ESC→Game→Leave Mission→Main Menu를 눌렀다. 창 모드4.427829초/전체화면4.282323초, 각각 종료0이다. 이전 실제 Session/phase4에서 State3→연결 반납→State4를 거쳐 메인 phase0/page main/정지0, 미션/Session 부재를 확인했다. 창 모드 최종 화면도 직접 확인했다.

전체화면 종료 뒤 `startInFullScreen`/`--window`를 덮어쓰지 않은 새 프로세스에서도 저장된 전체화면으로 실행됐다. 처음 보조 판정은 실제 UserNotUpdated 안내를 닫기 전 메인 화면과 이전 실행의 임시1024 해상도를 요구해 실패했다. 제품은 그때도 종료0/전체화면 플래그139/저장800×600으로 정상 실행됐다. 최초 자료를 보존하고 실제 OK 버튼을 누르는 재검사에서1.093678초/종료0·메인 메뉴·같은 전체화면 플래그/1920×1080 창을 확인했다. 저장된 논리 해상도800×600은 유지했다. Windows11이나 모든 DirectDraw 오류 경로의 검증으로 확대하지 않는다.

배포 ZIP은1365항목/134146116BYTE이며 Visual C++ 런타임3DLL과 자산을 포함하고 사용자 options/fullscreen 상태는 제외했다. 압축 해제본의 루트 `Netstorm.exe`는 새 클론이고 `game/Netstorm.exe`는 원본 리소스다. 실행한 것은 클론이다.

- 게임 SHA-256: `0e33558cbc3e947ccabd95460a158ec7bf591ed6626a96e7ab5dd059424723d9`.
- ZIP SHA-256: `45c8ffb9d99ddd12956d9100d620d566df74fd08cc172b813d84ecc5cf7e820f`.
- 보호2597개/기존fixture736은 불변이며 새 fixture 두 개를 더한 현재 개수는738이다.

[집계](../../cpppj/battle-leave-runtime-20261030.json)와 원시 `extracted/cpp-battle-leave-588-20261030/`에 관측/빌드/검사/배포/실행 자료를 구분했다. 새 제품 전체 C++/34경로·자연 완주/승패는 이번에 실행하지 않았다. 포획/희생/특정 적 공격/승리·공중3종 전용·신규 멀티플레이 검사는 하지 않았다. 나머지 UI/Edit·시나리오·존재 저장 reader·Native 선택과 후순위4~6차가 남아 있다.
