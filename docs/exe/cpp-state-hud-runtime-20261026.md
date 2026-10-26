# 단위456: 실제 SP 표시 재부착과 World 이전 HUD

## 원본 근거

작업 PC는 **HJOW-Athlon**이며 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 이번에는 정밀 자료를 다시 생성하거나 수정하지 않았다. 기준 판본은 PATCH 10.78이며 CD 10.72의 동등성 증거로 확대하지 않는다.

원본 `0043dad0`은 현재 `00557d74` serial을 `00464020`으로 조회한다. 조회는 `00563e38` 등록표의 1~999칸에서 serial이 같고 dead 비트가 없는 첫 노드를 고른다. 부모는 그 노드의 깊이 `+30`을 보관하고 가상 `+18` Push, Push 후 현재 `+4c/+50` 좌표의 가상 `+20` Attach(x,y,0,0), 정상 반환 뒤 같은 깊이 복원만 수행한다. 금액·SP·문자열·Idle·전체 화면 갱신을 추가하지 않는다.

[독립 PE 관측 도구](../../tools/decomp_budget_refresh.py)는 부모와 serial 조회의 실제 x86 명령을 실행했다. **120관측/중단48관측**에 빈 표·루트0·첫/마지막 슬롯·죽은 노드·중복·serial0·상위 flags·두 x87 환경과 Push/Attach 중단을 포함했다. 두 가상 효과만 명시된 장치로 바꿨다. 그 몸체 전체의 동등성은 기존 GumpTree 근거와 실제 결합 검사를 사용한다. [최초 TSV](../../cpppj/tests/fixtures/budget-refresh-x86.tsv)와 [Native SHA 보고서](../../cpppj/budget-refresh-native-20261026.json)를 제품 출력으로 덮어쓰지 않았다. 원본 게임이나 OS 실행은 하지 않았다.

`004666d0`의 원문 등록 필터도 기존 GumpLifetime에 연결했다. 루트0을 건드리지 않고 signed BYTE `00540426=11`과 같은 타입 및 이미 죽은 노드를 제외해 Kill만 예약한다. 실제 scalar 정리 `004662b0`는 별도로 호출한다. `00466280 → 00466200` HUD 통지는 현재 focus, 현재 dialog, 루트의 같은 GumpInput 경로와 숨김 포함1을 사용한다.

## 제품 연결

`RawStormPowerRefresh`의 부모와 조회를 RawButtonControls의 같은 등록표·현재 노드·실제 Push/Attach에 연결했다. 예산 vtable `00506948`의 `+20=00466750`은 기본 부착이므로 생산 패널의 설정 조회를 거치지 않는다. CampaignClientServices와 SceneMusic의 기존 전체 재합성을 실제 예산 재부착으로 교체했다.

`EnsureHudFactories`는 Client의 장기 표면·Player·Gameplay·카메라·현재 화면/지도 사각형을 사용한다. 전체 HUD 생성은 World 존재를 요구하지 않는다. 미니맵의 실제 World Pop/Unpop 대여는 InstallHud에서 현재 생성 토큰에 한 번만 연결한다. 중심 이동·선택 이동·생산 정보처럼 현재 World가 필요한 실제 입력은 그 지점에서 World를 요구한다.

Native Kill/Cleanup 뒤에는 해제된 버튼의 호스트 ID 별칭·라벨·동작·입력 영역도 반납한다. 실제 전체 루트가 현재이면 이전 MainMenu 버튼을 다시 그리거나 만들지 않는다. `test-state-hud`는 이 자식들을 실제 창에서 검증하는 명시 검사 전용 조작이다. State1 예약을 소비하거나 Edit/loaded 성공을 만들어 내지 않는다.

1024×768의 원문 생성자는 미니맵 구분선을 일단 고정 y400에 놓는다. 같은 State 처리의 실제 mode3 통지가 최종 미니맵 y693/높이75를 완성한다. 생성자·bitmap 크기·범위 검사에 임의 보정을 넣지 않았다. mode4는 같은 원문 입력으로 미니맵과 생산 패널을 숨긴다.

## 검증 기록

첫 GUI는 생성 중간 기하를 프레임에 노출해 원시 bitmap 범위 검사에서 실패했다. 원문처럼 생성/모드 통지를 같은 프레임에 완료하도록 검사 호출을 고쳤다. 두 번째 GUI는 실제 정리 뒤 삭제된 메인 버튼 별칭을 화면 합성기가 읽어 실패했다. 이때 Native 기대값 대신 호스트 보관 정보 수명을 고쳤다. 세 번째는 World 생성 전 HUD 교환·SP 재부착·mode4와 정상 Early1 MORE/OK/Home을 통과했다. 화면 검토에서 World 이전의 현재 루트가 공통 그리기 제출에서 빠진 점도 확인했다. Source7에서 연결한 뒤 네 번째 GUI는 실제 일반/편집 모드 좌측50,022픽셀의 표시 차이까지 확인했다. 위 빌드·GUI 실패 원시는 `extracted/cpp-state-hud-456-*`에 유지한다.

빌드2는 예산 Attach 분기의 중복 삽입으로 컴파일5오류였으며 중복만 제거했다. 빌드4는 manifest 작성 완료 전 시작한 사전 실패로 컴파일하지 않았다. 두 원시 로그도 보존했다.

최종 **Source7(2045소스) / build8**은43.052초·경고/오류0, 집중4는283고유/283관측·신규6개·실패0이다. 전체 full2는335.756초·실제 C++ **3114개/실패0**, 기존3108개 이름과 신규6개가 정확히 일치했다. 앞선 Source6 full1도3114개/실패0이었으며 Draw 변경 뒤 최종 같은 제품으로 통합 검사를 마쳤다. 게임 SHA는 `17d6201ecb6c051fcf620db25986baa714970cd020818fccb6bf743d982aff4e`다. 같은 제품의 GUI4와 정상 Early1 GUI-normal2를 통과했고 initial/editor/ready/home 화면을 직접 검토했다. GUI는 원본 밖 복사본·음향 비활성 상태로 실행했으며 HJOW-Athlon의 AGENTS.md 실행 예외에 해당한다.

보호1821파일(원본1394·CD425·AGENTS1·정밀 C자료1)을 실행 전후 대조했고 모두 같다. 새 Native 근거는 함수표 SHA도 별도로 대조했다. [실행 보고서](../../cpppj/state-hud-runtime-20261026.json)의 소스/네 실행 파일/Native/GUI 해시와 `extracted/cpp-state-hud-456-{source7,build8,focused4,full2,gui4,gui-normal2}`를 함께 본다.

기존 fixture **650개(TSV647개)**는 Git checkout 필터를 적용한 내용과 전량 일치한다. 기존 3108개 검사 이름과 ProductionInitialContext의 기존 본문/단언을 보존했다. 해당 파일에는 include1개와 새 사례4개만 추가했고 별도 SP 부모 검사2개를 추가했다.

## 남은 범위

Native State1 전체34Effect/prefix, Runtime/Player 반납 coordinator와 실제 Tick의 Edit 예약 소비는 다음 작업이다. Early/Priest Training/전체 캠페인/데모의 자연 완주와 전체 메뉴·화면·음향의 최종 동일성은 계속 미완료다. 이 자식 호출 검사와 기존 checkpoint를 전체 Edit 설치 완료로 합산하지 않는다.
