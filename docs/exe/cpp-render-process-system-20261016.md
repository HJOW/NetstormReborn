# 공통 표시 목록과 Fiery Tail의 실제 Host·Kernel 연결

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**이다. 단위100은 이미 원본 대조한 생성/프레임/Draw/삭제 몸체를 실제 공통 표시 목록과 process11 수명에 연결했다. 새 디컴파일·원본/클론 게임·GUI는 실행하지 않았다. CD10.72의 Fiery Tail은 이번 연결 범위가 아니다.

## 하나의 표시 목록과 실제 소유권

기존 `ConstructionVisualSystem`은 전용 registry와 토큰을 소유했다. 다른 효과가 별도 목록을 만들면 같은 부모의 종류별 등록/수집 순서와 슬롯 재사용이 달라진다. 새 `RenderProcessSystem`은 **하나의 `RawRenderRegistry`·토큰 발급·생성 완료 조회·kind3 수집/Draw 분배**를 제공한다. 기존 원본 목록의 첫 빈 칸 검색·40칸 확장·삭제 구멍을 바꾸지 않는다.

`ConstructionVisualSystem`은 단독 사용 시 이 공통 저장소 하나를 직접 소유하고, 여러 효과가 있는 월드에서는 외부 저장소를 받는다. 두 생성 경로는 같은 기존 몸체와 같은 필수 경계를 사용한다. 별도의 건설 표시 복제나 프레임 알고리즘을 만들지 않았다. 기존 `DrawVisibleScene(ConstructionVisualSystem&)`도 같은 공통 저장소로 분배하며 새 `RenderProcessSystem&` 오버로드는 Fiery Tail만 있는 월드에서도 사용할 수 있다. 기존 `construction` 콜백 이름은 공통 kind3 표시 분배라는 기존 의미를 유지한다.

실제 소유권은 각 효과의 pending `unique_ptr`에서 같은 Host의 **Kernel로 한 번만 이동**한다. 공통 저장소는 등록 필드와 콜백을 조회할 뿐 객체를 중복 소유하지 않는다. 각 효과 시스템이 먼저 남은 client Form을 삭제하고, 공통 저장소·Host·풀은 그보다 오래 살아야 한다. 종류가 다른 객체도 같은 토큰 공간을 쓰므로 삭제 후 재사용된 슬롯의 새 효과와 옛 수집 토큰이 혼동되지 않는다.

생성 중인 객체는 실제 목록과 Kernel에는 존재하지만 초기화 완료 전 Run/Draw/수집에서 제외한다. 실제 외부 초기화 경계가 Kernel과 renderer에 재진입해도 아직 준비되지 않은 버퍼를 읽지 않는다. 공통 Draw 콜백은 호출 전에 복사하므로 출력 중 자기 삭제로 조회 항목이 제거돼도 콜백 자체의 수명을 유지한다. 기존 건설의 자기 삭제·목록40→80 확장 통합 검사도 통과했다.

## Fiery Tail process11

`FieryTailVisualSystem::Add`는 공통 토큰 조회를 준비한 뒤 `RawFieryTailLifetime`의 **표시 등록→Host client Form 부착→120입자/공유 색 초기화**를 실행한다. 파생 생성 완료 후에만 ready를 세운다. 실패하면 실제 Host/Form/Kernel/표시 등록도 정리한다. 실제 부모 없는0번은 이 Host 어댑터에서 거부하며, 원본 생성자 몸체의 부모0 독립 관찰과 구별한다.

`FieryTailVisualProcess::RunFrame`은 기존 `RawFieryTailFrame::Step`을 같은 난수·게임 시각으로 호출한다. Draw는 실제 끝점 경계에서 현재 부모의 화면 좌표를 읽어 기존 선 요청 몸체에 전달한다. 이 경계는 원본 `00497220`에 해당하며 카메라/정수화 방식을 어댑터가 추측해 다시 적용하지 않는다. 프레임 배율과 공통 collector 배율은 원본의 같은 전역이므로 어긋난 연결은 명시적으로 거부한다.

실제 `SquidProcessHost`의 Form 삭제 가상 통지를 통해 `00487960`에 해당하는 dirty를 먼저 제출한다. 이어 원본 소멸 몸체가 입자→표시 등록→객체 반납을 수행하며 공유 색 표는 남긴다. 부모 공통 종속 삭제와 시스템 종료도 같은 Host/Kernel 수명을 사용한다.

## Form 삭제 통지의 원본 대조

실제 `00487960`은 일반 프레임의 dirty 변환과 다르다. 네 변을 **먼저 CRT 정수화**하고 카메라를 뺀 뒤1픽셀 여유를 적용한다. left/right/bottom의 중간 float 저장과, top의 카메라 감산값을 x87에 유지하는 차이를 보존했다. 이를 일반 프레임의 영역 무효화 함수로 대체하지 않았다.

`tools/decomp_render_removal.py`는 실제10.78 `00487960`과 실제 `__ftol`을 실행하고 `00499310` dirty 큐 소비만 외부 요청 경계로 관찰한다. 음수/소수/0영역·큰 float 좌표·카메라·flags의 **고유48입력·두 정밀도96정상 반환·결과 차이0**이다. C++ `RenderProcessRemovalBounds`의 네 DWORD와 flags0이 기본53비트48행 전체와 일치했다. 비교037f는 fixture에 보존하며 같은 입력의 다른 정밀도를 새 고유 입력으로 세지 않는다.

반환 ESP·비휘발 레지스터·x87 제어/TOP/tag·스택 sentinel과 쓰기 범위를 기존 격리 실행기의 검증으로 확인했다. 원본 등록 객체는 호출 전후 전체 바이트가 같고, 요청은 정확히 한 번 발생했다. 보고서는 `cpppj/render-removal.json`, 기대값은 `cpppj/tests/fixtures/render-removal-x86.tsv`다. 이 관찰을 실제 Host/게임/장치 실행으로 세지 않는다.

## 통합 검사와 남은 연결

새 통합 검사는 같은 실제 raw 풀·Host·Kernel·공통 삭제 몸체 위에 합성 부모와 명시 장치 경계를 공급한다. 실제 미션/SHP나 픽셀 동일성 검사가 아니다.

- 건설→Fiery Tail→건설의 공통 토큰1/2/3·슬롯0/1/2와 client process11 Form을 확인했다.
- 같은 Kernel의 최초10입자/후속 프레임과 공통 수집·깊이 정렬을 거쳐 두 건설 출력 뒤 실제 Fiery Tail 선10개가 제출됐다.
- 확보 실패·dead 부모 실패·생성 중 프레임/Draw/수집 재진입과 중첩 생성 거부 뒤 Form/Kernel/등록 잔재가 없음을 확인했다.
- 부모 삭제 때 두 종류 효과가 모두 해제되고 Fiery Tail 삭제 dirty가 원본 몸체와 같은 영역을 제출했다.
- 슬롯 재사용 후 오래된 토큰이 새 효과를 그리지 않고, 어긋난 배율이 거부되며 시스템 종료 뒤 부모 종속 체인이 비는 것을 확인했다.

최종 Release 전체 빌드는 경고0·오류0·종료0이고 집중 **25개 통과**다: FieryTail8·기존 ConstructionVisualSystem9·RenderCollection7·새 삭제 영역1. 새 입력은 삭제 통지48개뿐이며 기존 Fiery Tail 원본 입력·통합 검사의 재사용을 새 관찰로 더하지 않는다. 전체 CTest 내부 **1424개 중1422통과·기존2실패,199.49초,종료8**이다. 데모1의145프레임/3.625초 TYPE148 stage5와 데모3의113프레임/2.825초 TYPE128 effect11은 그대로이며 새 실패는 없다. 실행 중이거나 다시 기다려야 하는 검사는 없다.

새 삭제 통지48입력과 기존 수명31입력·프레임65입력·색 띠48행·필요 에너지2352행의 SHA/고유 입력 감사가 통과했다. 필요 에너지 보고서의 공유 CMake2개 유효 핀만 실제 변경 전후 이력과 함께 갱신했고 기존 TYPE 제품/원본2352행은 보존했다. 새 코드/주석의 UTF-8/한국어와 공백도 확인했다. 로그 접두사는 `extracted/goal-render-process-system-20261016-`이다. 문서/인수인계 뒤 `20261016-37 CP 공통 표시 목록과 Fiery Tail 실제 프로세스 연결`로 로컬 커밋하며 목표는 active를 유지한다. 보호 원본·AGENTS.md·dotnetpj는 수정하지 않았다.

통합 검사의 색수11 다음 `0xabcdef01`은 **명시 입력**이며 실제 게임 heap의 관찰값이 아니다. 실제 allocator/정상 첫 Draw의 읽기 근거와 Client/GameWorld의 단일 수집·정렬/dirty/VFX 선 픽셀 연결은 남아 있다. 이번에는 Session의 `flightEffect`를 채워 데모3의113프레임을 넘기지 않았다. 다음은 이 장치/메모리 근거를 확보한 뒤 실제 **표시 Pop→effect11→Missile 부착** 순서로 연결하는 작업이다. 건설 표시의 삭제 가상 통지와 CD 대응도 실제 판본별 근거/연결이 필요하며 이번 Fiery Tail 삭제 통지 완료와 구별한다.

반더148 stage5·전체 GUI/입력·AI 전략·건설/경제·승패/저장/다음 미션·사용자 자연 완주도 미완료다. 전체화면 DirectDraw 오류 수정 제외는 요청대로 유지한다. 실제 원본 동일성과 전체 목표 달성으로 표시하지 않는다.
