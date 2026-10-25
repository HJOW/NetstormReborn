# 구름 갱신·행 표시·dirty 정리와 실제 Screen 픽셀

2026-10-25 단위425다. 이전424는 `345b5691 / 20261025-18 CP`로 같은 Client의 실제 Kencloud 수명과 전체 팔레트를 연결했다. AGENTS와 양 인계를 재독했다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 수행일은 **2026-10-23**으로 같다. 새 전체 디컴파일이나 원본 게임 실행 없이 기존 보호 PE의 실제 함수만 격리 실행한다. 보호1821파일 기준은 `extracted/cpp-kencloud-render-425-baseline.json`이다.

## 제품 변경 전 원문 고정

`decomp_kencloud_render.py`는 실제 **00470c50(갱신42BYTE),00470c80(전체 dirty),00470ca0(정리61BYTE),00470ce0(행 조회),004711d0/00471410(각554BYTE의 불연속 표시 몸체)**를 실행했다. 전체 정밀 함수표의 실제 조각만 허용한다. **472입력·944관측(정상910/중단34)**이며 두 x87 모드의 차이0이다. 생성/step/clip/잠금/복사는 명시 동기 장치로 분리했다. 이 관측 자체가 실제 픽셀이나 전체994b0/자연 Frame 실행의 증거는 아니다.

현재 cache0/존재, 생성 복귀 뒤 speed 재조회와 캡처한 반복 수, 곱의 하위32비트0 자연 생략, 양/음수·감김 좌표, 이전/현재 행/전체·부분 dirty를 고정했다. 생성·clip·lock·blit·unlock 뒤의 명시 상태 변경과 중단도 보존했다. 같은2054DWORD 전체 FNV, clip4DWORD, 현재 speed, 행 조회 반환과 전체 trace를 fixture에서 대조한다. 행 조회의 음수 나머지는 양수로 정규화하지 않으며 행-1은 실제 cache DWORD를 읽는다.

## 같은 실제 메모리와 장치

`RawKencloudRender`는 같은 packed 배열을 빌린다. 표시 부모는 현재 clip을 캡처하고 새 clip→종류별 잠금→전체/행별 복사→종류별 해제→이전 clip 복구를 수행한다. 중간 예외는 후속 해제/clip 복구를 가짜로 실행하지 않는다. dirty 행은 표시가 끝난 뒤 이전 행에 기록되며 정리는 실제 이전 표시 행만 지운다. 갱신은 같은 실제 생성/Step과 같은 GameRandom을 합성한다.

실제 `Screen::BlitBuffer`는004a1fe0→004a0c10의 DIB 경로에 따라 sourceLTRB를 대상 사각형으로 만들고 화면/현재 clip으로 자른 뒤 DWORD와 남은 BYTE를 앞으로 복사한다. 같은 생존 cache span을 읽고 이미지 사본을 만들지 않는다. `ClientKencloudGlobals::Draw`가 실제 Screen의 clip/잠금/픽셀 장치를 제공한다. 별도004a15d0/004a1650 시차 표면은 DirectDraw 미복원 경계로 남기며 같은 GDI 잠금으로 대신하지 않는다. raw 두 부모의 독립 Native 관측과 실제 지원 장치를 구별한다.

별도 `netstorm_cloud_render_probe`는 일반 CTest에서 창을 열지 않고 명시 실행한다. 실제 GIF 생성→전체 표시→이전 행 정리→실제 Update 두 step→부분 표시→정리→곱0 자연 생략→시차 잠금 미복원의 선행 clip 효과→800×600 재생성과 전체 표시를 검사한다. 실제 표면 전체 픽셀을 source 좌표 기반의 별도 타일 식과 비교하고 지원 장치의 clip·같은 cache/난수 수명을 확인한다. API 장치 입력이며 자연 사용자 플레이나 최종 Editor 설치로 세지 않는다.

## 실행과 남은 경계

Native944를 먼저 고정한 뒤11소스의 빌드·집중 검사와 실제 화면 픽셀·플레이 회귀·전체 검사를 진행한다. 최초 시도/실패도 원시 로그로 보존하며 최종 결과는 아래에 기록한다.

사용자는 진행 중 빌드가 끝나면 인계를 작성하고 목표를 일시 정지하도록 요청했다. 이어 긴 실제 플레이 검사를 여러 작업단위 뒤 묶어 실행하도록 검증 운영도 변경했다. 따라서 이번에는 빌드 종료 뒤 새 제품 집중 검사/GUI/전체 CTest를 시작하지 않는다. 원문944관측은 완료했지만 제품의 새2검사/소유자7·실제 화면 픽셀은 아직 미검증이다. 재개 시 먼저 변경 부위 집중 검사를 하고, 긴 회귀는 기능 묶음 또는3~5개 작업단위 뒤 같은 최종 제품에서 통합한다. 입력/타이머/저장의 중요한 변경은 해당 장면만 짧게 먼저 확인한다.

빌드1은 **종료0·컴파일 경고/오류0**으로 끝났다. `extracted/cpp-kencloud-render-425-source1/hashes.json`에11소스/네 실행 파일을 고정했다. 종료 뒤 보호1821파일·원본/CD 목록과 Native944 최초 SHA가 동일했다. [인계 보고서](../../cpppj/kencloud-render-handoff-20261025.json)는 실제 빌드 근거와 제품 검사의 미실행을 분리한다. 마지막 통합 검사는 이전345b5691 제품의2843개 통과이며 이번 예상2846개는 아직 실행 결과가 아니다.

재개 시 `netstorm_tests.exe --filter KencloudRender`의2검사, `--filter ClientKencloudGlobals`의7검사를 먼저 실행한다. 같은 캐시의 실제 전체/부분 픽셀은 `tools/cpp_kencloud_render_probe.py --output extracted/cpp-kencloud-render-425-device1`로, 기존 팔레트는 `tools/cpp_kencloud_client_probe.py --output extracted/cpp-kencloud-render-425-palette-device1`로 별도 실행한다. 이 경로들은 아직 없다. 첫 GIF 성공은 warm-up이 없어 현재 dirty0이며 실제 Update 두 step 뒤64행이 된다. 준비된 구식 단위별 전체 감사 스크립트는 미실행·main 비활성화했고 묶음 검사 시 최종 제품 SHA와 누적 변경 범위를 새로 대응해야 한다.

인계를 로컬 **20261025-19 CP**로 커밋한 뒤 사용자 요청대로 목표를 일시 정지한다. 원본 게임과 클론 GUI는 이번에 실행하지 않았고 push하지 않는다. 마지막 전체 정밀 디컴파일은 계속 **HJOW-Athlon / 2026-10-23**이다.

자연 Renderer **004994b0 전체2823BYTE**는 화면 flags 변경/세계 redraw→공통 Gump overrideHead00463fe0/00565dd8→animating bit0x20의 갱신→행별 변경 영역·시차/덮개→실제 표시→dirty 정리를 함께 소유한다. 이번 부모와 실제 장치의 연결을 곧바로 이 전체 Frame 순서의 완료로 확대하지 않는다. 기존 정적 GIF 배경은 자연 Frame 접합 때 교체하며 무조건 Step을 호출해 난수/override 관문을 우회하지 않는다. Native State1 정리·최종 Editor 편집/저장 UI와 전체 미션 사용자 완주/진행 저장·재실행·완전 동일성은 계속 남는다. `/goal`은 활성 상태다.

앞선424의 “공통 모달” 명칭은 이번에 호출자까지 확인해 **overrideHead**로 정정한다. 실제004654c0의 유일한 직접 호출자는 Moviegump의00484010이며, 일반 모달00466600을 호출한 다음 별도로 override를 설정한다. 현재 `GumpInput::SetOverride`와 `GumpCollectionState::overrideHead`가 이 토글을 이미 소유한다. 일반 dialogCount나 Guide 포인터를 이 관문으로 대신하지 않는다. 과거 고정 원시 증거는 수정하지 않는다.
