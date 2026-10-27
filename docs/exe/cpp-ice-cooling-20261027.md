# 아이스 캐논 냉각 연기와 Test Battle 편집 복귀

단위492, HJOW-Athlon. 작업 전 AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md와 기준76c73be9를 확인했다. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23으로 현재 PC와 같다. 원본/CD·정밀 디컴파일 산출물은 수정하지 않았다.

491의 실제 TEST01 왕복을 중단한 `아이스 캐논의 실제 냉각 표시004a8770 미연결`을 해결했다. 원문004a8770은 현재 TYPE/SHP의 abstract 비트와 현재 프레임에서 signed WORD 포인트를 읽는다. 같은 Session의 `RawProjectileGeometry::Hotspot`을 통해 이 입력을 얻고 `RawCampaignParticles::AddCooling`을 실제 공통 Host/Kernel·표시 목록에 연결했다. 없는 효과를 빈 성공으로 처리하지 않는다.

004a8140의 생성자는 공통 등록 뒤 process21·signed 깊이 감소·200×24BYTE 입자·공유 색 객체를 초기화한다. 기본 방출 수는8이고, 냉각 wrapper004a86e0이 생성 완료 뒤10회/간격3으로 덮어쓴다. 공유005c7aa8의 색 표는 RGB 보간이 아니라 팔레트 번호77..88의12 DWORD다. 실제48BYTE 할당과 읽을 수 있는 인접 DWORD를 기존 `ParticlePaletteMemory`로 공급한다. 서로 다른 효과의 색 메모리를 합치지 않는다.

004a8240은 사용 꼬리를 먼저 확보하고 가득 찼으면 life<=0인 슬롯을 재사용한다. 슬롯 확보에 실패하면 속도 난수를 소비하지 않는다.004a82b0은 첫 호출에 시각/부모 영역만 초기화하고, 이후 전체 경과 시간을 한 번 처리한다. delta는 float에 저장하되 누적 elapsed는 기존 double과 더한다. 유한 방출 수>0이면 호출마다 하나를 방출하고, 무한(-1)일 때만 due<=elapsed를 검사한다. 방출 실패 시에도 위치/다음 간격 난수와 유한 횟수 소비는 원문 순서대로 유지한다.

각 입자의 수명 감소·분열·원래 수명의0.8배·이동·영역 합집합을 복원했다. 분열로 늘어난 꼬리 입자도 같은 프레임에서 처리한다. 이번에 수명이 끝난 입자도 이동/dirty를 요청하고 다음 프레임의 active 판정에서 제외된다. active가 없으면 flags0으로 같은 Host에 자기 삭제를 요청한다.004a85a0은 좌표를 먼저 정수화하고 카메라를 뺀 뒤 현재 색 번호의 가로2픽셀 선을 요청한다. 공통 Form 삭제의 영역 통지·입자 반납·registry/base·객체 반납과 공유 색 유지도 연결했다.

`RawSmokeEffect`가 이 본체를 소유하고 `SmokeVisualSystem`이 실제 Form 부착/Kernel·공통 수집/Draw·반납을 묶는다. 생성 중에는 Frame/Draw/수집이 미완성 객체를 읽지 않는다. 중첩 생성 거부와 실제 확보 실패 뒤 정리, 자기 삭제 뒤 필드 재접근 방지를 검사했다. `RawCampaignParticles`는 같은 시각·난수·배율·현재 카메라·dirty 큐·8비트 출력 표면을 사용한다. 메모리 확보 실패만 원문 wrapper처럼0으로 반환하고 미연결/계산 오류는 숨기지 않는다.

## 독립 원문과 검사

`tools/decomp_smoke_effect.py`는 보호된10.78 PE의 생성자/공통 등록·입자·시간·난수·Frame/Spawn/Draw/색·소멸을 실제 실행한다. 고유893입력은 Frame841·Spawn28·냉각 생성8·현재 SHP wrapper8·기본 생성8이다. 두 x87 정밀도에서 주 동작1,786반환과 후행Draw/소멸을 합한 루트3,572반환이 정상이며 정밀도 차이는0이다.112BYTE 의미 필드·입자 전체·공유 색 전체·난수·dirty/선·확보/반납 순서를 허용오차 없이 대조했다. vtable/실제 주소와 미사용 padding은 의미 필드 직렬화에서 제외하되 원문 쓰기 경계로 보호했다.

힙 성공·Host 부착/SHP 공급·kill/dirty/VFX 선 장치는 이 독립 fixture의 명시 경계다. 실제 Host/Kernel·공통 renderer 검사는 별도2개이며 생성 중 재진입·수집 비노출·확보 실패 정리·공유 표 재사용·Frame/Draw·자기 삭제를 확인했다. 원본 fixture 대조5개와 합계 신규7개가 최종 실패0이다. 기존 fixture/검사 본문은 수정하지 않았다.

최초 독립 입력의 부모 깊이를 raw12에 쓴 오류를 실제 signed BYTE raw35로 정정했다. 첫 공유 색의 입력도1000..1011로 두어77..88 쓰기를 실제 확인했다. 최초 자료는 `extracted/cpp-ice-cooling-492/native1-fixture.tsv`·`native1-report.json`에 보존했다. 그 뒤 최종 fixture를 생성했다.

최초 C++ Frame 대조는 누적 double을 float로 줄인 오류1개로 실패했다.00418454의 FADD QWORD를 확인해 제품 본체만 수정했고 최종 fixture 기대값은 그대로 유지했다. build1의 C4018 한 건은 부모 음수 검사 뒤 용량 비교의 명시 unsigned 변환으로 해결했다. build2/3은 경고/오류0이다. 등록 조회에 지원하지 않는 `--list-tests`를 사용한 호출도 종료1로 보존했고, 실제 `--list`로 다시 읽었다.

전체 C++ **3,300개/실패0**, CTest wrapper1개/실패0(335.49초)을 통과했다. 기존3,293등록을 모두 유지해3,300등록이다. 전체 성공3,300줄 중 이름3개가 stderr와 섞여 원시 혼입/누락 목록을 보존했다. 같은 최종 바이너리에서 해당3개를 stdout/stderr 분리 재실행해 각1개/실패0·종료0과 원래 이름을 확인했다. 이 확인을 포함한 전체 등록 집합이 일치한다.

보호3,204파일과 AGENTS.md의 바이트가 같다. 기존 tests/CMakeLists.txt의164,157BYTE 접두부를 유지하고 끝에 새 등록만 추가했다. 최종 동결 소스/검사2,867파일과 바이너리 해시가 같다. [실행 보고서](../../cpppj/smoke-effect-runtime-20261027.json)에 감사 수치를 기록했다. 게임 SHA256은 `51d6926af6f4a007ed96bb755e2382a05e0a5b5a6182d49bafc5761216717a0b`, 검사 프로그램은 `7621fc369f293e6e8c7bab1c2a5abaaeae7edec07f14d4b2b141a9bd8e52d12d`이다. 커밋 전 fetch/pull은 추가 변경 없음이며 push하지 않았다.

형식 검사에서 TSV의755행은 마지막19열의 이벤트가 빈 문자열이라 끝 TAB이 있다. 이를 제거하면18열이 되므로 독립 관측 바이트 그대로 보존했다.893행 모두19열임을 검사했고 해당 데이터 파일을 제외한 staged diff 형식 검사는 통과했다. 원시는 `format-check.json`이다.

## 실제 짧은 구동

같은 최종 게임으로 TEST01의 Edit→Test Battle→브리핑→Go!→잠시 진행→ESC/Game/Return to Editing을 실제 실행했다.365프레임/14.316초에 종료0·시간 초과 없음이며 originals 해시가 같다.260프레임에 TEST01/mission·정지0·활성 Session을 관측했고,330프레임에 editor·정지0·활성 Session 및 미션 시작 상태 해제를 관측했다. 공통 Kernel은 원래 편집51→전투127→복귀51이다.352프레임의 편집 Game 메뉴도 확인했다.

이 실행은 격리한 원본 복사와 형제 편집 작업본을 사용했다. 원문50칸 상한에 가려진 TEST01을 선택하기 위해 다른 느슨한 fort는 새 복사 대상에서 제외했다. TEST01.fort/english 바이트 자체는 바꾸지 않았다.1024×768 창 모드·무음 실행이며 스크린샷에서 좌측 덱·미니맵이 있는 전투 화면과 복귀 후 편집 덱을 확인했다. GUI 보고서는 process21 필드나 연기 픽셀 자체를 직접 덤프하지 않으므로, 이번 관측을 원본과 화면/음향 전체 동일성으로 확대하지 않는다.

스테이지 완주와 사용자 개입 검증은 계속 보류한다. `.english` 부재 시 Test Battle의 기존 게임 종료 문제도 참고 사항으로만 유지하며 개선하지 않았다. Windows10/11·전체화면/DirectDraw·전환 뒤 재실행은 현행 목표다. 클라우드 플로터146 회피·변위0 반사 Generate의 미초기화 지역점, Edit NewFort/AddIsland·전체 메뉴/시나리오 수명과34스테이지 전체 동일성은 남았다.10.72 Smoke의 새 복원은 이번 범위 밖이다.

원시 기록은 `extracted/cpp-ice-cooling-492/`와 같은 접두부의 감사/실행 스크립트다. 보호 원본·기존 검사/CMake 접두부·최종 동결/실제 바이너리·실제 왕복·커밋 전 pull을 함께 감사한다. push하지 않는다.
