# Builder 접근점과 같은 실제 경로 — 단위130

AGENTS와 양 LEFT를 다시 읽고 깨끗한0e6574f에서 시작했다. 현재 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**으로 같다. 기존 정밀 소스를 읽었으며 새 내보내기·원본/클론 게임·GUI 실행은 없다. 전체화면 DirectDraw는 제외한다. 로그/원문 접두사는 `extracted/goal-builder-path-20261017-`다.

## 실제 접근점

`RawBoardApproach`는 원본41db70과41d400이다. 사각형 중심과 목적지의 실제41ce90 방향을 사용하고, flags4에서는 사각형을1씩 확장한다. 방향별 가장자리 중간점→1~255 보정→flags2/방향0의 X 끝점 선택→현재 사각형 보정을 순서대로 실행한다. 세계 보정 뒤 사각형 보정이 다시 값을 바꿀 수 있으므로 이를 하나의 clamp로 합치지 않는다.

현재 목적지 Graph와 가장자리 후보 Graph가 같으면 원래 사각형을 복구하고 반환한다. 반복 횟수는 **둘레가 아니라 폭×높이를 _ftol로 절삭한 DWORD**다. 같은 정수 top에서는 left와의 동등 여부에 따라 아래/왼쪽으로, 그 외에는 right→bottom→left 순서로 위/오른쪽/아래로 움직인다. checking의 비정상 경계는 원래 assertion 경계와 좌상단 복귀를 보존한다. 찾지 못하면 **확장 전 사각형**의 안쪽1칸 면적이4보다 클 때 재귀하고, 그렇지 않으면 마지막 점을 반환한다. 실패 경로의 원래 사각형 확장 유지도 관측했다. Graph254를 임의 실패 조건으로 추가하지 않는다.

Session은 현재 `RawPlayerPlacementAnchor::GraphAt`의 같은 표면/Graph 전역에 연결한다. 공장 발자국 사각형은 원본처럼 임시 값이며 다른 월드 지도나 독립 직선 접근점으로 바꾸지 않는다.

## 현재 경로와 도착 시각

실제443000의 `piVar5[piVar5[4]*2+0x2714]`는 **nVert×8+0x9c50**이다. Runtime 시각 배열은0x9c58에서 시작하므로 의미는 **Time(nVert-1)**이다. +0x10을 현재 진행 인덱스(+0x186c0)로 읽거나 Runtime.Time(nVert)로 옮기지 않는다.

기존 `ConstructionBuilderPath.currentStep/arrivalTimes` 값형 fixture 경계를 유지하고 실제 경로의 `readArrival`/`prepare` view를 추가했다. `RawConstructionBuilder::Arrival`은 실제 view가 있으면 현재 경로를 읽는다. Session은 SID별 안정된 view를 보관하고 같은23/39 Start가 반환한 실제 Kernel 객체의 등록을 확인한 뒤 마지막 시각을 읽거나 Prepare한다. 속도 조정 뒤와 공급원 처리 뒤의 최종 반환도 같은 경로에서 현재 값을 읽는다. 별도 시간표나 이동 진행 복사본을 만들지 않는다.

`MakeConstructionBuilder`는 실제 접근점·StartConstructionMovement와 같은23/39 시작·현재 TYPE speed/turningSpeed Prepare를 조립한다. 기존 같은 공급원 선택/정렬/소비·Factory/Owner/Pop·Regular0 예약과 단위129의105 주기 분배를 사용한다. 실제 첫 객체 Pop flags10과 운반 객체 Pop0을 유지한다.

## 독립 원본 근거와 검사

새 `decomp_board_approach.py`가 보호 PE의41db70/41d400/41ce90/41d0d0/CRT를 실행하고 GraphAt와 선택된 assertion만 명시 입력 경계로 둔다. **696고유 입력 / 두 정밀도1,392정상 반환 / 3,408,122명령 / 정밀도 차이0**이다. 방향/동률·현재 사각형·세계 모서리·분수 좌표·flags·Graph 정책·Graph254·재귀/확장·debug 조회를 교차했다. C++가 반환 좌표, 변경 뒤 사각형4DWORD, 모든 Graph 좌표/응답/assertion 순서를 대조해 모두 통과했다. 과거 Builder fixture의 기대값을 변경하지 않았다.

실제 Session 검사에서 소유자1 공장 목록이 비어 있는 오류를 발견했다. 전역 공장에는 실제 SID15002/type124와 SID15017/type123이 있었지만 owner1 목록은 **capacity0/count0**이었다. 실제 **4910a0**은 Player9개에 각각 **new0x78/용량0x1e/개수0**을 기록한다. 새 독립 초기화 관측은 **9기록/두 정밀도2정상 반환/1,404명령/차이0**이며 확보/동맹 설정만 외부 경계다. Session에서 첫 저장 객체 Pop 전에 소유자별30칸을 준비해 실제 등록을 복구했다. 초기화 전 상태9개와 원본 fixture도 C++가 대조한다.

원래thewarbegins의 rainVortex에는 sunWalker만 준비되어 있으므로 windBattery75 요청이 거부되는 것은 정상이다. 공장별 준비 내용물 조건을 우회하지 않았다. 실제 Builder 흐름 검사는 **메모리에서 복제한 Fort 저장 내용물 형식**에 준비된 windBattery 한 개를 넣고 기존 실제 설치 로더로 적용했다. 원본 파일은 변경하지 않았고 생산 완료 자체를 이 검사로 검증했다고 주장하지 않는다.

이 입력에서 공장 기준점/Graph 접근점→실제134/23 BFS와 현재 마지막 도착 시각 반환, 비영점 요청 시각→같은 Prepare→재준비 뒤 실제 시각 반환을 확인했다. 같은 현재 공급원 선택/정렬/소비가 실제105/39와 Regular0를 생성하는 것도 검사했다. 호출자 모형 훅을 공급하지 않고 `MakeConstructionBuilder(state,{})`를 사용했다. 실제 생산/일반 서버 건설 시작/자연 플레이는 별도 남은 작업이다.

읽기 전용 초기 상태 조사에서 TheGreatBattleDemo/tnrondemo/vndemo의 공장 설치도 확인했다. 공장에 준비된 일반 건물 부품은 아직 없었다. DEMOFw 별도 파일은 TYPE156의 파생 Pop 미복원에서 중단됐으며 이 조사 결과를 데모 플레이 완료로 확대하지 않는다. 임시 조사 코드는 제거했고 로그만 분석 경로에 보존했다.

최종 **Release 빌드10 종료0·경고0·오류0**, 새 집중3개·기존Builder10개 통과, 전체 **1,525개 모두 통과·실패0·CTest 종료0·226.59초(전체226.63초)**다. 현대 `--verify` **25개**와 스테이징44파일/현재제품9개/보호3개 SHA·UTF-8/LF·새 문자·CMake 기존 접두를 감사했다. 기존26보고서의 현재 HEAD 핀66개만 이력과 갱신하고 과거128핀은 보존했다. 기존 fixture/AGENTS/DN/보호 원본은 불변이다.

## 남은 실제 플레이

이 단위는 일반 건설 완료나 사용자 자연 플레이 완료를 뜻하지 않는다. 실제 서버 건설 시작 소비자와 일반 Construction Process/Frame/Visual, Client 전체 DWORD 메뉴→ContentForm6 Take/위치/수집/그리기/내려놓기, 생산·경제·AI 전략, GUI 시작/진행/승패·저장·다음 미션·자연 완주와 전체 동일성은 계속 검증해야 한다. CD의440400/구조체 시각 오프셋은 기록하지만 이번 새 접근점 구현은10.78을 대상으로 한다. 전체 목표는 active다.
