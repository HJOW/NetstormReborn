# 실제 캠페인 세션의 공통 삭제·기술 내용물 연결

현재 PC 및 마지막 정밀 디컴파일 PC는 **VM-W11-Codex, 2026-10-15**다. 이 단위는 기존 복원 모듈을 실제 `RawCampaignSession`의 같은 풀/장부/SP/Player에 설치한다. 별도 관찰 월드를 만들거나 초기화 결과를 프레임마다 복사하지 않는다.

`sp.Initialize()` 전에 기존 사제 상태가 소유한 추적 offset 주소를 연결한다. Player 내용물 통지는 그 저장소의 현재 실제 offset/배열 주소/순열과 같은 사제 상태의 진단 코드·검사 잔여를 읽는다. 현재 battleShown은 실제 AssetPostPopState의 비제로 조건을 getter로 조회하며 임시 DWORD 참조를 보관하지 않는다.

ContentHost를 첫 객체 생성과 `BindRuntime` 이전으로 옮겼다. 같은 `RawCampaignLifetimeBindings`가 emptyGeyser84의 입증된 base VT와 실제 Form6의 수명을 등록한다. `RawPlayerTechnology`도 같은 실제 PlayerWorld·Carrier finder·ContainedCreate·가상 삭제를 사용한다. `AddTechnology`/`ClearTechnology`/`FindTechnology`는 그 같은 모듈에 대한 실제 호출 경계다.

공통 삭제는 실제 `SquidReward`와 현재 GameWorld 선택을 사용한다. ClearSelection은 같은 표시 선택을 해제하고, unitLost는 실제 SoundPlayer에서 원본 반복0/음량0/좌우0/우선1/한도0으로 재생한다. PATCH PE의 초기 DWORD `005412a4=147`, `005427bc=02000031`, `005429c4=020000b4`를 읽어 silentType/내용물 사건/수입 명령에 공급했다. 기존 TYPE/Graph/생산 장부를 임의로 초기화하지 않는다.

실제 클라이언트의 Player UI 통지는 `RawButtonControls`가 소유한 현재 등록표를 조회하고 대상 자신의 가상 처리기에 직접 보낸다. 생산 Refresh 사건이나 전체 트리 재귀로 바꾸지 않는다. 등록 없는0은 원본처럼 생략한다. 내용물 수입 명령은 같은 InputQueue에 넣고 위치 효과음은 원본 `guy.wav` 이름으로 실제 SoundPlayer에 보낸다.

내용물 커서 취소/다른 소유자 수입 패킷/AI 지갑 등 아직 공급하지 않은 선택 경계는 명시 오류로 남긴다. 이번 기술 삭제는 그 너겟 경로를 선택하지 않는다. 실제 GumpForm의 생성/수명과 생산 패널 전체가 완성됐다는 뜻도 아니다. 기존 RawPriestRegen의 observed/gate/sentinel 값형 backend는 별도 현재 getter 연결이 필요하며, 이번 Player 통지의 직접 조회와 혼동하지 않는다.

검증에는 실제 보호 `thewarbegins.fort`를 같은 생산 세션에 설치한 기술 생성/중복/삭제/부모 체인·SID 반납·삭제 깊이와 empty84 생성/Pop/삭제/반납, 현재 UI 등록표 직접 처리 검사를 추가했다. 최초 통합 빌드는 병행 편집 중의 DialogScript overload와 이전 object가 섞여 LNK2019로 실패했다. 안정된 소스에서 다시 빌드하고 아래 결과를 갱신한다.

최종 **Release 게임/검사 경고0·오류0**, 새 Session 집중2개 및 UI 직접 등록 집중1개가 모두 통과했다(`extracted/pause-final-build.log`, `extracted/pause-final-focused.log`). 페이지의 이전 object 강제 재컴파일도 경고·오류0이다(`extracted/pause-metadata-final-build.log`). 최종 실행 파일의 대화상자97상태·Early1/2 안내23상태·완료값 저장/다음Tutorial9상태가 모두 통과하고 원본 전체 파일명/SHA와 허용 설정을 복구했다(`extracted/pause-settings-final-backup/`). 처음의 일시 설정 접근/복구 실패와 복구 과정도 로그에 보존했다. 전체 회귀 최종 결과는 중단 인수인계에 기록한다. 새 native 입력은0이며 기존 수명/Player88/SP61 근거를 재사용한다. 보호 원본/AGENTS/dotnetpj는 변경하지 않는다.

최종 전체 CTest는 **1/1·내부1,233개·실패0, 112.41초**다(`extracted/pause-final-ctest.log`). 기존 실제 사제 이동/교전/HP1·감시 해제 등 연관 회귀를 포함한다. 같은 소스의 검사이며 GUI 상태 수나 native 입력 수와 합산하지 않는다.

사용자 요청으로 이 단위 검증·인수인계·로컬 커밋 뒤 작업을 일시 중지한다. 실제 로그인/Ready/Stage3, AI 생성·초기 갱신의 세션 설치와 전략, Confirm→Notice→Clear/Place, Early Missions 단계 게이트·정지 소유권, 건설/다리/수확/운반/희생·승패 저장과 완주 동일성은 남는다. DirectDraw 전체화면 오류는 요청대로 제외한다.
