# 거주 구역 좌표 복원 정정과 실제 연속 Kernel 검사

아래 수치는 단위81 당시 결과다. 후속 [오벨리스크 Handle 복원](cpp-buried-regular-20261016.md)으로 Tutorial5도400프레임을 통과했고 현재 실패는4개다.

현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-16**이다. AGENTS.md와 양 인수인계를 다시 읽었다. 보호 원본·AGENTS·dotnetpj를 변경하지 않았으며 게임/GUI를 실행하지 않았다. 사용자 요청으로 바뀐 DirectDraw 오류 수정은 cpppj 2차 목표 필수 항목이며 이번 1차 미션 복원 범위에서는 제외한다.

## 원본 오류로 잘못 설명했던 좌표

10.78 거주 구역(TYPE89·residence)의 `0049a4e0` event1에서 첫 범위는 `ftol(X-48)..ftol(X+32)`, 둘째 범위는 `ftol(Y-32)..ftol(Y+32)`다. 기존 문서/C++의 ‘패치만 X-32인 원본 결함’은 스택 변화를 놓친 복원 오류다. 원본의 나눗셈0 회피나 난수 소비를 바꾸지 않고 잘못된 축만 고쳤다.

`0049a674`에서 X/Y를 현재 ESP+14/+18에 저장한다. `0049a69b/0049a6a1`에서 두 인수를 push한 뒤 `004558f0`을 cdecl로 호출하므로 호출 후에도8바이트가 남는다. 따라서 `0049a6a7/0049a6bc`의 ESP+20은 처음의 ESP+18인 **Y**다. `0049a6c0`의 추가 push 전 Y를 읽고32를 뺀다. 원본 CRT ftol와 unsigned 나머지/helper는 그대로 실행한다. CD10.72의 별도 `0044a710`도 같은 좌표 관계다.

## 별도 원본 관찰과 비교

`tools/decomp_residence_coordinates.py`는 현재 정밀 함수표에서 기존 Handle/helper의 실제 불연속 범위만 선택한다. 과거 선택 내보내기가 현재 PC에 없는 경우를 다른 PC의 파일로 메우지 않는다. 기존 finder 입력 등록/실제 탐색과 실제 CRT·난수·Handle를 사용하고 표시/프레임 지정은 선언한 관찰 경계로 유지한다. 모든1×1 입력의 저장 해시 단계1을 명시한다.

좌표8쌍(축 차이64의2쌍·양 끝·소수 좌표 포함)×난수36개×장식 flags3개×판본2개 = **1728고유 입력**이다. 각 입력은 두 x87 정밀도에서 재생하며 이 반복을 새 입력으로 세지 않는다. 실제 두 난수 호출의 low/high와 탐색 사각형도 스택에서 기록한다. 결과는 반환 float 비트·현재 난수·최대 조사·모든 표시 효과·전체 부모50/48바이트·이웃3개의 프레임이다. C++ 추가 검사2개는 이 결과를 전수 비교한다. 과거1976개 fixture/보고서를 덮어쓰지 않는다.

첫 추가 C++ 검사는 이웃의 저장 단계가0인 합성 입력 때문에 실제 프레임 지정의 재Pop 경계에 도달했다. 원본/C++ 추가 입력 모두 현재1×1 프레임과 일치하는 저장 단계1을 공급한 후 다시 관찰했다. 기대값을 새 C++ 출력으로 만들거나 미복원 효과를 빈 성공으로 바꾸지 않는다.

## 실제 미션의 연속 프레임 범위

`CampaignMissionStartTests.cpp`의11개 독립 검사는 실제 보호 자원의 Load→Start 후 같은 Session/Pool/Kernel을400회 호출한다. 외부 시각은25ms 간격으로10초이며 원본 고정 FPS를 뜻하지 않는다. 예외에는 실제 미션/프레임/시각이 붙고 실패로 보고한다. Regular 경계는 부모SID/TYPE/event/count/payload를 기록한 뒤 같은 예외를 재전파한다.

| 실제 입력 | 수정 후400프레임 결과 |
| --- | --- |
| Tutorial1/2/3/4 | 통과. Tutorial4는 첫 프레임의 거주 구역 event1 범위0 복원 오류를 해결했다. |
| TheWarBegins / MasterOfWhirligigs | 통과 |
| Tutorial5 / DissolvedAlliance | 첫 프레임의 TYPE112 buried event0 미복원 |
| Tutorial6 / GuardMyBack | 첫 프레임의 TYPE136 Sun Barricade event0 미복원 |
| DemoStormRages |103프레임/2.575초의 실제 생성 TYPE144 rainFlyer 파생 Pop 미복원 |

이 검사는 Guide/미션 프레임 게이트·AI전략·GUI 입력/그림/소리·승패·저장·다음 미션 전환을 포함한 자연 완주의 증거가 아니다. 6개가10초 진행해도 목표는 active·미달성이다. 다음은 실제 buried Handle `004622f0`, 썬 바리케이드 Handle `00457a50`, rainFlyer Pop/BaseFlyer를 복원한다.

원본 출처와 정상 반환/고유 입력 감사는 `cpppj/residence-coordinates.json`이다. 로그는 `extracted/goal-kernel-20261016-{first-build,first-focused,trace-build,trace,coordinates-native,coordinates-final-build,coordinates-tests,coordinates-frames,ctest}.log`를 따른다. 최초 빌드/합성 입력 실패도 보존한다.

최종 집중6개·원본1976+1728행 전수 비교는 실패0이다. 전체 CTest는 **1353개 중1348통과·5실패,118.19초**로 위의 새 연속 검사5개가 실패한다. 기존1340개에 새 좌표2개는 모두 성공했다. 누락을 건너뛰거나 성공 기대 예외로 바꾸지 않았다. 현재2352개 요구에너지 SHA 감사도 통과했다. 과거 assetregular 보고서는 제품3개 SHA와 현재 호스트에 없는 선택 내보내기8개 때문에 현재 트리 감사와 구별한다. 해당 차이를 새 보고서에 기록했으며 원본 fixture/과거 보고서를 덮어쓰지 않았다. 추가 검사 소스의 CRLF→LF만 정규화했고 관찰 당시/현재 SHA와 바이트 변환 관계를 새 보고서에 보존했다.
