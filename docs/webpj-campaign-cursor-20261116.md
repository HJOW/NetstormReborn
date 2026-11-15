# 캠페인 Cursor 실제 하위 조립 (WB04-E569)

기준선 `225553df`의 `client/RawCampaignCursor.cpp`198줄·헤더86줄(총284줄)을 옮겼다.
`RawCampaignSession::Load`는 ConnectBuilding/Motion→runtime.Load→Devastation/CarrierDrop/Prayer→
Builder/GameplayPlacement→ConnectCursor 순서로 이 본체를 반드시 만든다. 따라서 6단계의 커서를
4-D C 선행으로 앞당겼다. 표시기 RawCampaignCursorDisplay는 E407에서 이미 이식되어 재사용한다.
기준선 뒤 대상 소스·검사 변경은0이다. cpppj·보호 원본·공통 인계·dotnetpj는 수정하지 않았다.

## 연결 내용

- 현재 실제 World/Core/Factory·내용물/정렬 Form·HP·건설 배치의 같은 풀을 검사한다.
  data의 참조 묶음과 game/effects의 함수 묶음 값 사본을 구별하고 같은 게임 상태를 빌린다.
  실제 ClientCursorGlobals를 빌리거나 기존 독립 호출용 지역 소유자를 만든다.
- 기존 Placement·Previews·Lifecycle·Position·ReturnPiece·ConstructionLocal·Drop·Actions와
  Display를 한 번 구성한다. 실제 Canon/SHP/프레임·HP·다리 연결·Owner·Factory·가상 삭제를
  기존 몸체에 위임한다. 마나/사정거리·소리·게임 준비·에너지/전체 MayPlace·Builder는 상위 게임
  소비자가 공급한다. 누락은 선택 위치에서 실패하며 성공 판정으로 채우지 않는다.
- 공개 Take/Cancel/Update/Lock/Unlock/Rotate/Drop/PlaceLocal은 현재 Prepare를 먼저 호출한다.
  CancelForState는 Native leaf만 실행한다. 잠금/release 필터와 outside를 별도 거부하지 않는
  HandleInput, 정지 마우스의 GumpsChanged, PriestMenu의 Take 바인딩을 연결한다.
- setEffects는 장치 함수 사본을 교체한다. 기존 하위 모듈은 다음 호출에 새 장치를 조회한다.
  GUI Drop은 같은 Form Host에 재부착하며 종료 통지는 UI closeGump 대신 game의 현재 Process에
  보낸다. 팔레트의 경고 흰색/가격 노랑과 현재 카메라 원점·스냅도 같은 소유자를 사용한다.
- 로컬 예측 배치의 현재3e 생성자 헤더를 GameAux의 장기4BYTE로 추가했다.
  보호 PE0052ede6/7의3e/1·0052ede8의WORD25를 직접 대조하고 reset/독립 인스턴스와 WORD 범위를
  검사한다. 물리25BYTE 송신과 선언 길이WORD를 구별하고 매 송신에 현재 값을 읽는다.
- 반환의 native 할당/재구성은 JS 예약 토큰→실제 Regular 생성/같은 Host 부착으로 표현했다.
  부착 뒤 현재 clock으로 ResetTask를 실행한다. JS 엔진의 메모리 소진을 복구 가능한 native
  bad_alloc 관측처럼 보고하지 않는다. 편집 전용 TakeObject API는 웹 범위에 맞게 제외했다.

## 검증과 실제 범위

새 조립 검사11건과 GameAux1건을 더했다. `campaign-cursor-support.mjs`는 보호 TYPE188/SHP와
실제 World/Factory/Canon·ContentFormHost/수명·건설 Place/Local·HP/SP·Regular/Kernel·표시기를
사용한다. UI/마우스/송신은 관찰 장치이며, 에너지/전체 MayPlace는 바인딩의 외부 함수 계약에 대한
명시 입력이다. 이 장면을 실제 저장 미션의 전체 게임 판정/플레이로 세지 않는다.

1. 같은 풀/필수 게임 거부, 실제 장기 상태/fallback·현재30DWORD 보존, data/game/UI 값 사본.
2. 실제 Form 취득→SHP/Canon 미리보기→현재 카메라 위치→취소/가상 삭제/SID 반납.
3. dirty 누락 거부와 장치 재연결, State leaf에 외곽 Prepare를 추가하지 않는 계약.
4. 메뉴 Take와 잠금/해제·회전·release/outside/화살표/ESC 입력 분배.
5. 정지 마우스의 현재 Gump 재조회, 같은 Form의 GUI 재부착·수신과 게임 Process 통지.
6. 명시 TYPE84 Form의 실제 Local 예측 Pop·반환 Form/Regular→25BYTE 송신→상위2000SP 차감.
   송신 시 잔액5000, 반환 품질2/flags108, 실제 Slow의10초를 확인했다.
7. AI 직접 Local 경로는 취득·소리·상위 비용을 추가하지 않는다. 현재 생성 헤더/선언 길이와
   Fast1초·유지 옵션을 읽으며 같은 생성/부착/공간을 사용한다.
8. 실제 팔레트/공유 카메라/스냅·kind4 수집과 SHP 픽셀, 실제 가상 삭제 거부 및 미연결 마나 실패.

첫 집중6건은 모두 통과했다. 뒤 검사16건 중2건은 시험의 잘못된 잠금 기대(1)와 존재하지 않는
팔레트 경로 때문에 실패했다. 원문의 Local 경로는 Lock을 두 번 실행하며 하위 원본 대조도 깊이2를
확인한다. 기대를2로 교정하고 실제 GIFCLOUD.COL을 사용했다. 다음59건 중1건은 표시를 처음
연결한 시험 장면에서 SP 추적 offset owner를 빠뜨려 실패했다. 같은 실제 ScrambledSpStore에
추적 상자를 연결해 해결했다. 이 교정은 시험 구성/기대이며 게임 몸체 알고리즘 변경은 없다.

최종 집중59건 통과, 실패·건너뜀0. 새 Cursor 모듈 coverage 줄100%/분기93%/함수90.28%다.
로그는 `extracted/wb-e569-focus.log`에 있다(Git 제외).
Node v22.23.3·v26.13.0 전체 각각 **5,384건 통과, 실패·취소·건너뜀0**이다.
로그는 `extracted/wb-e569-node22.log`·`extracted/wb-e569-node26.log`에 있다(Git 제외).
대응표1,160행 오류0, port837·partial33·todo134·제외156, session/oracle0을 확인했다.

원본 하위 fixture 대조는 기존 검사에서 계속 수행한다. 예: Lifecycle565관측(파일566행),
Placement573관측(574행), Previews522관측(523행), Actions289관측(290행), Position687관측(688행).
그중 Lifecycle 정규화SHA-256은 `153f07eb922205fa0a2adf2a4d365bbbffe82d4822d4814c79fdd539d5a4c53a`,
Position은 `0c72511664e3837087714999c900e27ea6b70b1f15a0cc121d71988b3072927c`이다.
manifest의 기존801항목·실제 cpppjBasis를 보존한다. 새 원본 관측/게임·브라우저 실행은 없다.

## 남은 통합과 다음 시작점

현재 부품 port이며 실제 Session/앱 설치는 아직 없다(session/oracle0). CampaignSessionTests의
저장 thewarbegins·준비75 생산 칸/사제 메뉴·실제 에너지/전체 MayPlace·Builder 완료·마나/Range·
GameWorld/UserInput/Client 설정 시험은 D/E·6단계에서 실제 부모로 재대조한다. 여기의 명시
TYPE84 Form과 허용 입력은 생산 칸/게임 전체 판정의 대체가 아니다.

다음은 A 구획 ClientGameplayGlobals(현재258줄)이다. E334에서 막힘으로 기록한 커서/선택·SID 진단·
지형 Bitmap/Stamp 캐시 선행은 모두 이식되어 있다. 최소 Session/World 계약을 명시해 실제 장기
owner의 대여/반납·현재 Runtime 공개/pin부터 연결하고 D Session1을 공개 경계로 분할 이식한다.
Devastation/CarrierDrop/Prayer는 별도 .cpp 바인딩이 아니라 Session Impl에 포함되는 .inc로,
해당 소유 상태와 함께 연결한다. 실제 저장 Fort 설치/Load와 L2를 이어 붙인다.
