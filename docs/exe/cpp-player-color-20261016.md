# cpppj AI 색 표와 섬 프레임 변경 복원 — 2026-10-16

현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-16**이다. 원본10.78의0043ca70·0040eb40·004421a0을 대상으로 하며 원본 게임/GUI 실행은 없다. 단위77에서 `GuardMyBack`이 Load를 통과한 뒤 AI 색 변경에서 멈추던 경계를 복원한다.

## 같은 저장소와 실제 프레임

`RawPlayerColor.h`의 `ChangePlayerColor`는 기존 `core.bridgeState.ownerColors`의 같은9DWORD를 먼저 쓴다. 별도 색 표를 만들지 않는다. 원본 표00531b08의 초기값은0~8이다. 이후 실제 풀에서 받침(TYPE94·island), 종유석(TYPE95·islandStalag)을 찾아 현재 소유자/색 표/전투 여부로 프레임을 계산하고 같은 `core.frame.Set(sid,frame,0)`을 호출한다.

원본은 색이 바뀐 소유자의 섬만 고르지 않는다. 모든 대상의 현재 색을 다시 읽어 갱신한다. iterator는 초기값1을 먼저 증가하므로 SID2부터 시작하며 free 비트1만 건너뛴다. dead·void·abstract 비트를 추가 제외하지 않는다. iterator의 끝은 진입 때 풀 길이, 본문의 포인터 상한은 현재 풀 길이다. 전투가 아니거나 소유자가0이면 중립 프레임8, 그 외는 색 DWORD-1의 감김이다.

같은 AI 생성기의 색 변경은 기존 brain 유무 검사보다 앞서 실행한다. `CampaignAiServices.changeColor`는 복원한 계산 뒤 화면이 선택적으로 받는 완료 통지다. 실제 게임 계산을 외부의 빈 callback으로 대신하지 않는다. 선택된 프레임 효과의 실패는 먼저 쓴 색 표를 되돌리지 않는다. 원본 표 밖 owner와 다른 판본은 호스트 예외로 명시 거부한다.

## 독립 원본 관찰

`tools/decomp_player_color.py`는9소유자×4색(0/1/8/0xffffffff)×2전투 조건×3배치의 **216개 고유 입력**을 원본 명령으로 실행한다. 전체24000슬롯 iterator와 실제 색 프레임 계산을 실행한다. 선택된 큰004acee0은 thiscall 인자/호출 순서/Frame DWORD 쓰기의 명시 외부 대체다. 원본 Frame 내부의 공간 재등록/렌더링까지 관찰한 결과는 아니다.

독립 fixture는 색 표36BYTE, 입력16슬롯800BYTE, 순서 있는 프레임 호출을 보존한다. 입력 대상 외 풀의 불변, 함수 범위, 표/스택 밖 원문 쓰기 금지, 정상 반환/스택/비휘발 레지스터도 확인한다. `PlayerColor_` 두 C++ 검사는 원본216행과 부분 실패의 표 쓰기 보존을 대조한다.

`CampaignDemoStart_GuardMyBack`은 외부 색 callback 없이 실제 Script/Fort/Session/Start를 호출한다. 시작 뒤 표 전체가 `0,1,5,8,2,5,6,7,8`인지, 전체 대상 프레임이 현재 색에 맞는지, 실제4번 받침이 프레임1로 바뀌었는지를 검사한다. 원본 스크립트의 ai2=purple, ai3=orange, ai4=red에서 독립적으로 정한 기대값이다.

첫 추가 검사에서는 모든 참여 소유자에게 TYPE94/95가 있다는 잘못된 전제를 넣어 실패했다. 실제 장면은 중립50개/소유자4의6개이며2·3번의 대상은 없다. TYPE94/95는 별도 받침이고 자연 지형97/98과 다르다. 임시 진단으로 확인한 뒤 모든 대상 검사는 유지하고 **실제로 존재하는4번 받침의 전후 변화**를 요구하도록 고쳤다. 다른 소유자/종류/상태 조건은 독립216입력에서 확인한다. 진단 로그는 `extracted/goal-color-20261016-diagnostic.log`이며 임시 출력은 제거했다.

## 검증과 남은 일

최종 Release 빌드는 경고0·오류0이며 색 변경 집중2개가 통과했다. 실제 데모 시작3개 중 **GuardMyBack·DissolvedAlliance 두 개가 통과**했고 DemoStormRages는 기존 TYPE143 후처리에서 실패한다. 전체 CTest는 **1335개 중1334개 통과·1개 실패, exit8**, 검사118.07초·전체118.12초다. 이 실패 검사는 유지한다. 기존8미션 시작과 색 변경 전 기존 모든 회귀 검사는 통과했다.

새216입력과 인접 Graph 초기화16·총 각도518·Graph 재구성288회·휘리베이스1734개의 보존 SHA, Python 구문/UTF-8/문서 링크/공백을 확인했다. CMake의 이전/현재 SHA는 요구에너지 보고서 이력에 남겨 기존2352입력 감사도 통과한다. 새 헤더 두 개를 원본 모듈의 IDE 목록에도 등록했다.

로그는 `extracted/goal-color-20261016-{native,build,focused,diagnostic-build,diagnostic,final-build,final-demo,ctest,tests}.log`, 제품 검증은 `cpppj/player-color.json`을 따른다. 캠페인/데모의 자원 대응은 [후속 캠페인 기록](../gameplay/demo-campaign-map.md)을 따른다.

이 단위는 같은 실제 표와 raw 섬 프레임을 연결한다. GUI의 `GameWorld::Players()` 및 Remap은 아직 기존 화면 snapshot을 읽으므로 일반 유닛의 화면 색까지 일치한다고 주장하지 않는다. 실제 GUI Start/StartDemo·Guide/프레임 상태와 현재 Player/색 조회를 연결하고 자연 재생을 확인해야 한다. 맨 오 워 풀/데빌 메이커·플라이어와 데모1의 후처리도 남아 있다. 전체 목표는 미달성이다.
