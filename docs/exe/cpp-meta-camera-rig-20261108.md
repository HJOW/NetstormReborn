# 1126 — 실제 카메라 Rig와 공유 메타 갱신

제품1126 / 표시·파일·제품버전11.01이다. 이전 단위는 `739fc118 / 20261108-76 CP`로 커밋됐다.

`00419190/419210/419600/419730/419840` 전체·각 다음 함수까지 기계어와 현재 객체470d20/479650, 실제 가상표501ae8을 읽었다. 기존 Spring3d를 같은0x70 필드에 연결한 실제 Kernel/ProcessForm CameraRig와 Client 바인더·intro wrapper를 컴파일했다. **참가자 생성 caller에서 intro를 호출하는 연결과 전체 메타 State는 후속**이다.

1096의 최초 host 표현에서 예약 float로 두었던 카메라+0xc/00544d78은 실제 애니메이션 개수 DWORD다. 세 float와uint32 순서로 정정하고 기존 Poll의 비영 관문도 같은 DWORD를 읽게 했다. 카메라 완료/이동의005c85d8/dc는 기존 LocalState의MetaRefresh/MetaRebuild이므로 별도 통계나 갱신 사본을 만들지 않는다.

상위는 현재 owner가0이면 Kernel/Host 전에 반환한다. 현재 TYPE53/첫 같은 부모가 있으면 초기 높이/개수를 바꾸지 않고 목표만 재설정한다. 없으면 spring·초기/목표 높이·개수 증가 뒤 현재 owner를 다시 조회한다. 두 번째 owner가0이면 새 객체를 반납해 개수를 감소시키며, 비영이면 실제 Host에flags10h로 부착한다. 이 두 조회를 합치거나 가짜 객체로 대체하지 않는다.

프레임은 이전 완료 상태를 먼저 읽어 실제 MetaRefresh를 증가시키고 Kill한다. 이동 중에는 Spring 반환 비영일 때만 MetaRebuild를 증가시키고, 이전 카메라 높이로 실제 Renderer 분모를 선택한 뒤 새 높이를 저장한다. 소멸은 현재 개수를 감소시키고ffffffff 감김을 원문 위치에서 진단한다. 실제 빈 OnAttached/Notify에 해당하는 부분에는 원문 외 게임 효과를 추가하지 않는다.

실제 바인더는 같은 Client의 game/CS/시각/LocalState/구름 owner를 마지막 반납까지 유지하며 현재 Pool·Kernel·Host/현재 기술 부모·메타 자기 섬·실제 분모/월드 다시 그리기를 사용한다. 첫 빌드는 서로 다른shared_ptr를 같은auto 선언에 묶은 C3538 한 건으로 종료1이었다. 선언을 분리한 뒤 최종 두 Release를 재빌드했다. 최초 실패를 최종 성공 기록으로 덮지 않는다. x87 확장정밀도·전체 비정상 입력의 동등성과 실제 카메라 실행은 검증하지 않았다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 컴파일을 전체 메타 장면·MP 실행 성공으로 쓰지 않는다.

다음은 67h 전체 State 부모/일반 CS hook·메타 객체 그림/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출 → 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오 UI이다. 전체 목표는 미완료다.

Netstorm.exe 12,719,616바이트/SHA-256 `0cba8d2eb4cae56212190ff9c4fb6a4d7f2f829525b95017c79ca8f00a492f3d`, R.exe 669,696바이트/SHA-256 `ff1d588cfab51116b44892ee0c2d4c7f635ca1076c17887cec66c11503386039`다. 보고서는 `cpppj/meta-camera-rig-20261108.json`, 원문/기계어·빌드·정적 감사는 `extracted/cpp-resume-1126/`(Git 제외)다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 생성 소스 변경0이다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
