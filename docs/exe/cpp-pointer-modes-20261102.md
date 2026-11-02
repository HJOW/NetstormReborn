# 편집 객체 취득과 우클릭 취소·폴링 좌표 분리 (단위749)

2026-11-02 / HJOW-Athlon / Windows 10. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 내보내기는 바꾸지 않았다. 이전748의 좌클릭 소유자/다리 순환 구현·검증·커밋을 유지한다. 현재 제품은 **749 / 표시11.01**이고 전체 목표는 진행 중이다.

## 원문과 수정

- **모드1 좌클릭 `004d9ff8..004da043`:** force0으로 Pick한 대상이 있으면 `447340(source,0)` 뒤 selectTab.wav를 재생하고, 빈 후보도 마지막 Highlight0을 실행한다. 기존 코드는 일반 Select로 바꿔 현재 선택을 지우거나 강조 모드를 남겼다. 현재는 실제 단일 커서의 `TakeObject`를 사용하며 selected를 일반 Select로 바꾸지 않는다.
- **객체 취득 `00447340..0044738b`:** 실제 가상+48 Unpop(source,0) 뒤 현재 raw의 argument WORD, TYPE BYTE를 unsigned로 다시 읽는다. `CreateContained(source,type,argument,80,10,0)`의 반환 Form을 `Take(form,flags|100)`에 전달한다. 생성 반환0도 원문 호출 경계로 전달하며 임의 성공/무동작을 합성하지 않는다. 기존 실제 World/Contained/Cursor 몸체와 같은 풀을 사용한다.
- **비기본 모드 우클릭 `004d9c54..004d9d44`:** 먼저 Highlight0하고 enabled/현재 demo 관문의 기존 선택 정리를 실행한다. `ResetSelection()`의 실제 `Select(0,0,0)`이 같은 ClearMultiple/프로세스 통지/+7c(0)/+88/selected0/Highlight0을 실행한다. number0은 paused 관문으로 거부되지 않는다. 기존 코드는 모드와 관계없이 Pick/객체 메뉴 후보를 반환했다.
- **기본 우클릭:** Ctrl&&(special||Editor)의 원문 force를 Pick에 전달한다. 빈 후보이고 비메타 또는 playerKnown0이면 같은 선택 정리를 실행하고 객체 메뉴 후보는 반환하지 않는다. 메타/known의 항해 메뉴는 후속 미연결 경계로 유지한다. 실제 객체의 편집 선택/Core 추적은747의 같은 부모를 유지한다.
- **현재 좌표와 사건 좌표:** `005cae94/98`은 앞선 폴링 좌표다. Event가 이를 큐 사건의 x/y로 덮어쓰지 않게 했다. 모드5 강조와 모드6의 지도 내부 판정은 현재 폴링을 읽고, 실제 클릭 투영은 사건 좌표를 사용하는 원문 차이를 보존한다. 폴링 writer인 UpdateCursor는 유지한다.
- **사건 종류:** 마우스 비교는 원문의 `03ffffff` 마스크다. 명령 비트를 버린 뒤 같은 하위값을 마우스로 해석하지 않는다.

## 모드6 정정과 정적 연결

이전748 문서/인계의 `4239c0` “섬 조회” 표기는 잘못된 해석이었다. 실제 함수는 **LTRB 사각형의 right/bottom 제외 내부 판정**이며 현재 마우스 DWORD를 `005ca9cc` 지도 영역에 대조한다. 명령 전역 `005428a8`의 실제 이름은 **ChalSailPos / 0200006c**다(`html-command-inventory-20261102.json`).

`004da05a..004da0d6`을 따라 현재 폴링점이 지도 안이면 사건 좌표를 현재 카메라로 역투영하고 ftol의 하위 DWORD를 사용해 **X 상위WORD/Y 하위WORD**를 조립하여 `(ChalSailPos,0,packed)`를 큐에 넣고 강조0으로 바꾸도록 코드만 연결했다. 이 항목은 **멀티플레이 정적 사전 연결**이다. 새 CPU 관측/단위 검사/게임 실행/실제 통신 검사를 추가하지 않았고 항해 명령 소비자/빈점 항해 메뉴 완료로 세지 않는다. 싱글플레이 검사에서 모드6은 제외했다.

## 검증

새 관측기 `tools/decomp_cursor_object_pickup.py`는 게임/창 없이 보호 PE의447340 몸체를 실행한다. flags4×TYPE3×WORD2×Unpop 변경2×생성 반환2의 **96행**이다. 가상48·4afa70 생성·447000 취득은 명시 외부 경계이며 호출 순서/인자와 Unpop 뒤 현재 필드 재조회를 원문에서 관측한다. 최종 도구로 새 fixture의 바이트 재현 일치와 코드 불변을 확인했다. 전체 실제 타입/외부 하위/일반 입력/게임 실행의 증거로 확대하지 않는다.10.72 대응은 기존 내보내기 `0043e8e0`로 기록하되 이번 실행은10.78이다.

새 실제 입력 검사는 **창 없는 Client→UserInput→World→Session**과 같은 Gameplay/Kencloud/Subscriber/Core/Player를 사용한다. 모드1 빈 후보가 기존 selected를 유지하고 강조만 해제하는 것, enabled0에서도 우클릭1~5의 선행 강조0, paused1에서도 비메타 빈점 우클릭의0번 선택 정리, 사건과 다른 폴링(320,200)의 보존을 확인했다. 이 원래 기본 부분은 수정 전 **1검사/23단언 실패**였으며 `tests-before.log`에 보존했다.

같은 검사에 편집용 워크샾을 빈 좌표(220,220)에 실제 생성/Pop하고 실제 카메라 setter를 적용했다. 실제 해시/SHP 투명픽셀 Pick에서 찾은 클릭 좌표를 Event에 전달해 **Unpop→Form6 생성→같은 커서 취득→취소**를 확인했다. source의 void 비트, Form의 TYPE, costFlags100, lockDepth0, 현재 강조/선택0과 SP 보존을 검증했다. 장치 경계는 기존 보호 글꼴/메뉴 트리의 창 없는 장치이며 OS GUI/실제 PCM 검사는 아니다. 사제 포획/희생·특정 적 공격·승리·공중3종 전용 행동은 실행하지 않았다.

최종 **집중67개/실패0**이며 새3개(실제 입력1·96행/누락 경계2)와 기존 메뉴/Session·비용Form·수집/부착/카메라·소유자/다리 관측을 포함한다. 실행 대상66개/전체71대상·주 검사 캐시ON을 유지했다. 전체 C++ 재실행은 아니다. 첫 빌드는 비공개 GameWorld 카메라 접근1진단으로 실패했고 같은 Client Gameplay 카메라 접근자로 정정했다(`build-v1.log`). 최종 제품/집중 빌드 `build-v2.log`는 경고/오류0, `tests-v2.log`는67개 통과다.

EXE SHA-256은 `3063ffb5c15f4db35ace855dee2aa924d8e3db3742a9a34560a564cdd733594d`다. 기존 보호 **2658개(AGENTS1·원본1394·CD425·fixture838)** 모두 불변이고 새fixture1을 포함해 **2659개/fixture839**다. 다음 기준은 Git 제외 `extracted/cpp-pointer-modes-749/protected-after.json`이다.

이번 새 게임 프로세스/독립CLI/GUI/전체34/새ZIP/전체C++/신규MP 검사는0이다. 마지막 GUI746과 전체34/ZIP645·644는 별도 제품 근거다. 물리 입력/PCM·자연 완주와 제외 재현은 반복하지 않았다.

## 다음 연결

Ctrl+special 영역의 가상+70, challenge의 선행 클릭/빈점 항해 메뉴, 객체 우클릭의 반환값/음향을 별도 대조한다. 모드6 정적 배선을 멀티플레이 검증 완료로 처리하지 않는다. 나머지 UI/Edit/시나리오/파일·수명/호환성과3~8차를 이어 간다. 보고서는 `cpppj/pointer-modes-20261102.json`, 원시는 Git 제외 `extracted/cpp-pointer-modes-749/`다.
