# 일반 게임 메뉴 명령의 원문 몸체

2026-10-23, 현재 PC와 마지막 전체 정밀 디컴파일 PC는 모두 `VM-W11-Codex`다. 보호 10.78 PE SHA256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다. 이 작업에서는 보호 파일 변경, 게임/GUI 실행, 새 디컴파일, 빌드/커밋을 하지 않았다.

새 `GameMenuActions.h/.cpp`는 Guide/Gump가 소비하지 않은 일반 명령의 효과 몸체를 분리한다. `Handle(event,first,second)`의 `nullopt`는 미대상, `0`은 처리한 일반 명령 또는 뗌, `1`은 QuitApp의 실제 반환이다. 메뉴 명령 ID는 원문 전역 주소를 이름으로 삼고 순수 `command` getter가 공급한다. 이 단위는 서로 다른 불변 ID라는 원문 조건을 유지한다. 명령 ID 자체가 효과 콜백 도중 변경되거나 별도 명령과 같은 값이 되는 장치는 이 계약에 포함하지 않는다.

읽기 전역은 `globals.read(address)`로 실제 세션의 현재 값을 매번 조회한다. bool 소유 모드도 DWORD 지역 사본으로 복제하지 않는다. 건물 숨김 `0059a8ac`와 월드 다시 그리기 `0059a8b0`만 실제 장기 소유 DWORD 참조다. Player/Form span은 해당 조회와 필드 읽기 동안 살아 있어야 한다. 모든 효과는 같은 실제 Client/Session/큐로 연결해야 하며 필요한 효과가 없으면 예외를 던진다.

| 원문 명령 전역/초깃값 | 실제 몸체와 호출 순서 |
| --- | --- |
| QuitApp `542700 / 02000001` | `004da4be`: second가99가 아니면 UserInput.cpp line993 진단. 진단 복귀 뒤 `selectTab.wav,0,0,0,0,0`. 실제 epilogue가 EAX1을 반환한다. 정밀 C168841행. |
| RequestQuitApp `542944 / 02000094` | `004d6f23`: 현재 editor면 Tell `QuitEditMode`; 그 외 fort도 metamap도 아니면 `QuitAbandon`; 나머지는 QuitApp/0/99를 같은 큐에 예약. C168849행. |
| Sound/MusicVolume `542a8c/90 / 020000e6/e7` | `004d706a`: second의 signed32 `%d`로 `soundVolume=` 또는 `musicVolume=` 문장 생성 → 실제 Config `00441580` → `priestmove.wav,0,0,0,0,0`. C168888행. 이 몸체가 volume DWORD를 직접 변경하는 것은 아니다. |
| Pause `542a30 / 020000cf` | `004d8257`: 현재 mission 또는 editor인 때만 현재 signed 정지 깊이>0을 한 번 질의. status(11,0,paused?0:1,0,빈문자열). captured paused이면 ResumeClock→ResumeSound; 아니면 CursorCancel→PauseClock→MuteSound. status가 모드를 바꿔도 이미 고른 분기를 유지. C169471행. |
| ViewHome `5429ac / 020000ae` | `004d7eb5`: playerKnown이고 metamap이 아니면 owner0을 line1895로 진단. 진단 뒤 owner를 다시 읽어 현재 Player를 구한다. +7c SID 우선, 없으면 +84 SID를 `00433ba0`에 전달. C169345행. |
| ViewYourPriest `5429d4 / 020000b8` | `004d7f9c`: second 전체DWORD를 `004950d0`에 무조건 전달. C169372행. |
| HideBuildings `542ab8 / 020000f1` | `004d8a6d`: redraw=1, hide=(기존hide==0). C169698행. |
| ReloadTheams `542b54 / 02000123` | `004d79d2`: metamap이 아니면 첫 목록 `55a70c/55a714`를 끝낸 뒤 둘째 `5e189c/5e18a4`. 각 회차 count signed32 재조회, 현재 목록 SID와 현재 Pool의 SID*50 Form에서 비정렬 +0e/+12 DWORD를 읽어 `00455380(xBits,yBits,0)`. C169172행. |

MenuJustClosed `5429ec / 020000be`는 이 일반 명령 몸체의 대상이 아니다. `004c79b0`의 공통 HUD 부모가 `00466480`을 호출하고1을 반환한다. 메뉴가 닫히면서 serial을 사용해 방송하는 실제 부모 경로를 유지한다. `01000000` 플래그의 Form 가상+6c 분배도 앞선 UserInput 경계에 남으며 이번 메뉴 입력에는 공급하지 않는다.

## 독립 PE 관측

`tools/decomp_game_menu_actions.py`는 기존 `decomp_production_shortcuts.py`의 이미지 적재/Guide/명령 큐 실행기를 가져온다. **전체 `004d62b0` 함수 진입에서 시작하는 도구가 아니다.** OS/폴링/큐Pop이 끝나 이미 확보한 현재 사건 프레임의 `004d6e61`부터 실제 Guide와 Gump, release, 일반 명령, 일반 입력 tail을 실행한다. 일반 사건은 `004da477` 직전에 끝난다. QuitApp은 별도 진입점으로 점프하지 않고 같은 관문을 거쳐 실제 `004da4be..004da506`과 SEH 복구/RET를 실행한다. 상위 프레임 주소와 EDI0은 명시 진입 입력이다.

305입력 × 제어어 `027f/037f` = 610행이며 정밀도 차이는0이다. 명령 수는 각75,471, 합150,942다. 일반 정상행 스택 변화는0, QuitApp16행은 공급한 상위 프레임으로 RET하여12296이다. 진단/효과 경계에서 멈춘26행은 의미 반환값을 주장하지 않는다. 보호 실행 범위 바이트는 매 실행 뒤 원문과 비교한다.

앞10열은 `name,event,guideResult,gumpResult,globals,devices,read,filled,mutation,stop`이다. 뒤11열은 `completed,quit,globalsout,devicesout,readout,writeout,physicalqueue,bodytrace,envelopetrace,stackdelta,nativeinstructions`다. `globals`는 JSON의16개 실제 전역 주소 순서다. `devices`는 `01010000`부터2048BYTE의 원시 Player/Form/목록 전체다. `mutation`은 `경계이름+1기반호출번호/십진주소/DWORD`를 세미콜론으로 연결한다. 경계 기록 직후 같은 메모리를 바꾸고, `stop`에 해당 경계 호출번호가 있으면 복귀 전에 중단한다.

Guide 가상, Gump, 일반 입력, Config, Tell, 음향, 시계 변경, 커서 취소, 카메라, Reload, 진단 및 표준 정수 문자열 형식화는 명시 외부 경계다. Shift가 붙은 Reload 사건의 일반 tail에서 발생하는 `004d53f0` 단축 위치 질의도 명시 경계이며 반환0이다. 시계 경계는 명시 pauseDepth++/비영-- 효과를 먼저 적용한다. 실제 시각 누적, 실제 화면 이동, 실제 음향/설정 적용을 이 oracle이 실행한 것으로 확대하지 않는다.

새 `GameMenuActionsTests.cpp`의 두 검사는 앞10열만 장치 입력으로 삼는다. 같은 전역/전체 장치/40칸 전체 큐/효과순서/부분중단을610행과 비교한다. Guide/Gump 입력과 그 콜백 변경은 명시 선행 장치로 공급하며 제품 UserInput 전체를 호출한 결과라고 주장하지 않는다. `envelopetrace`는 실제 native 관문의 감사 자료이고 C++ subset이 실행하지 않은 일반 tail을 재구현해 맞추지 않는다. 큐는 본체 바깥 명시 장치다. 포화 쓰기는 실제452fe0에서 독립 관측한 슬롯 쓰기와 커서 복구를 공급한다.

## 후속 실제 연결 경계

루트가 같은 Session 읽기, 실제 명령 큐, Config 적용/음향 재생, GameClock/Cursor, 카메라와 실제 Reload를 연결해야 한다. 미연결 효과를 빈 함수로 성공 처리하면 안 된다. 이 새 모듈·검사는 아직 공유 CMake에 등록하거나 빌드하지 않았다. 별도 UI 어댑터가 실제 UserInput/Gump 경로를 통과하는 검증은 루트 작업에서 수행한다. 전체 ESC/Chat/NSInfo/Editor 또는 전체 캠페인이 완성됐다는 의미가 아니다.
