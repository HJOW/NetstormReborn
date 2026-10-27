# 현재 카메라 적용 본체 — 단위499

현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon, 마지막 전체 수행일은2026-10-23이다. 정밀 산출물은 변경하지 않았다. [원문 관측](../../cpppj/camera-commit-native-20261027.json), [추가 원문 관측](../../cpppj/camera-commit-supplement-native-20261027.json), [검증 보고서](../../cpppj/camera-commit-runtime-20261027.json)를 함께 본다.

## 복원

`RawCameraCommit`은 원본497960을 현재 Renderer view/전역 snapshot의 독립64BYTE, 목표/배율, 전체 redraw와 현재 dirty 표에 적용한다. 앞 [498 Present 제한](cpp-camera-present-target-20261027.md)의 뒤 적용 호출 경계에 연결할 본체다.

같은 zoom이면 목표와 현재 위치가 다를 때 커서 Lock(0)→Gump redraw를 호출한다. 그 효과 뒤 현재값으로 계산한 dx/dy는 화면 이동과 이후 dirty 처리에 보존한다. 실제 viewport 화면 이동→좌측/하단 영역→선택된 시차 처리 뒤 두497490을 Renderer→snapshot 순서로 수행한다. 화면 이동 효과가 목표를 바꾸면 이 두 setter는 새 목표를 읽는다. 보존한 dx/dy가 비영이면 현재 마우스 갱신 뒤 현재 dirty 표를 처리하고, 노출 띠→Unlock→Gump redraw→Renderer+5c=1 순서로 마친다.

dirty flags1은 그대로 보존한다. flags2는 실제4834c0처럼 이동 전 사각형과 이동 후 사각형의 합집합을 저장한다. 나머지는 좌표 네 개를32비트 감김으로 이동한다. 마우스 효과가 새로 추가한 표도 그 뒤 현재 개수로 처리한다. 노출 띠는 원문처럼 음수 길이에서 끝 좌표 자체를 NEG하며, 단순 사각형 정규화로 바꾸지 않는다. viewport Y가0이 아닌 입력도 원문 전체 호출 인자에 대조했다.

시차16이 꺼졌으면 배경 경계를 호출하지 않는다. 시차16/DIB2이면 이동할 때 별도 전체 viewport 사각형을 dirty에 추가한다. 시차16/비DIB이면 현재 signed 분모로 oldX→targetX→oldY→targetY를 나누고 차이를 배경 이동에 전달한다. IDIV의0/overflow는 C++ 미정의 동작을 실행하지 않고 선택된 나눗셈 위치에서 예외로 거부한다. 정상 분모의 계산은 원문과 같다.

다른 zoom이면 Gump redraw 뒤 현재 목표 zoom을 읽어 signed FILD의 float/Q16, 하위32비트 곱→SAR16의 칸 크기를 저장한다. Renderer와 snapshot은 각각 자신의 baseCellPixels를 사용하며 서로의 초기값을 복사하지 않는다. 두 setter→Gump redraw→worldRedraw1을 수행한다. 이 분기는 화면 이동/커서/dirty 이동을 호출하거나 Renderer+5c를 다시 쓰지 않는다.

## 검증

`tools/decomp_camera_commit.py`는 보호497960, 두497490,4834c0의 실제 명령을106입력/두 x87 정밀도212회 실행해 정상 반환·차이0을 확인했다. [fixture](../../cpppj/tests/fixtures/camera-commit-x86.tsv)는 원문 출력만으로 생성했고 변경하지 않았다. 커서/Gump/화면·배경 이동·기존 dirty 추가·마우스 장치는 명시 경계이며 그 호출/인자/순서를 기록한다. Native 안에서 이 기존 하위 몸체 전체를 실행한 것으로 합산하지 않는다.

Native 행은 두64BYTE 전체, 목표/한계/전체 redraw, dirty 표의 모든 좌표와 flags, 현재 전역11DWORD와 호출 순서를 비교한다. 무이동/상하좌우·시차/DIB·두 viewport 원점·signed 배율/Q16·서로 다른 기본 칸 크기·dirty0/1/2/3 및 고비트·100항목·정수 감김·효과 뒤 현재 목표/viewport/표 읽기를 포함한다. 추가 원문4입력/두 정밀도8실행은6정상 반환과2개의 의도한 IDIV 예외를 확인했다. 이 추가 관측은106행 fixture를 수정하지 않는다. 원본/클론 게임이나 GUI 실행은 없다.

신규4검사는106행 전체 대조, 화면 이동 효과 뒤 다른 목표로 두 setter가 적용되는 합성 계약, 같은 위치/별도 배율 분기의 선택된 장치, 실제 시차 분기에서만 IDIV 오류가 발생하는 경계를 다룬다. 빌드/집중 결과·보호 입력과 실행 파일 해시는 위 검증 보고서에 기록한다.

Release build1 경고/오류0, 신규4 포함 집중31고유/36관측·실패0, 등록3,323개다. 전체 검사는 반복하지 않았으며 마지막 전체는498의3,319개/실패0이다. 보호3,215파일 중3,214개와 AGENTS·기존 제품/검사 본문은 전체 바이트가 같고 두 CMake의 기존 접두부165,814/16,467BYTE를 유지했다. 빌드 전후 소스/검사2,885파일과 최종 실행 파일도 일치한다. 최종 게임 SHA256은 `582ea2aa2ec3a0dec2ae33289819d6dc65100a04d880f23b7cefc0fc8dd8f17a`이다.

집계기가 끝 등록 대상 src/CMake를 기존 본문 전체 해시와 비교한 첫 실패는 원시/제품/검사를 보존한 채 두 CMake의 기존 접두부를 별도로 대조하도록 교정했다. 빌드/검사 실패는 없고 재실행도 없다. 원시 자료는 `extracted/cpp-camera-commit-499/`에 보존한다. 커밋 전 pull은 종료0·추가 변경 없음이며 push하지 않았다.

## 다음 실제 연결

이 단위는 적용 본체와 원문 대조다. 실제 Frame이나 가장자리 입력에 아직 연결하지 않았다. 다음에는 같은 Client의 세계 크기/현재 Renderer dirty 저장소·커서/Gump/화면 장치를 연결하고 Present 뒤·마지막 snapshot 복사 앞에서 적용한다. 이어 Guide 뒤의 가장자리 본체에 현재 장치와 장기 시간값을 연결해 같은 최종 게임으로 짧은 실제 이동·버튼·메뉴·재진입을 확인한다. Windows10/11 DIB 호환성 처리가 원본 화면 이동 장치와 달라지면 이유와 범위를 코드 주석에 명시한다.

스테이지 완주/사용자 개입 검증과 Test Battle english 부재 개선은 계속 보류한다. 멀티플레이어는 사용자 결정대로 이후 디컴파일만 참고해 테스트 없이 사전 구현한다.
