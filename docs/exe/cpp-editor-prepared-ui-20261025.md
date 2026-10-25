# 기존 Editor 준비 상태와 UI 설치 완료 수명

2026-10-25 단위426이다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 원본 게임이나 새 디컴파일은 실행하지 않았다. 단위425의 집중9·실제 픽셀9단계·팔레트7단계 검증은 `6fcd82e8`에 기록했다.

## 실제 연결을 막던 중복 초기화

원문 State1의 `004b71a5`는 `004b2df0(0)`을 호출한다. 정리는 Pool/Player 재할당과 Core 선택을 소유하고, 이어 State1 prefix가 로컬 전송과 Editor 모드를 설치한다. 기존 Session 생성자는 별도로 Player/커서/모드를 초기화하고 다음 대여부터 Pool/Hash/spot을 지우며 Core1을 추가 선택했다. 따라서 이미 준비된 Native 상태를 그대로 넘길 수 없었다.

`AcquirePrepared()`는 같은 장기 게임 전역과 표면 배열을 초기화 없이 단일 대여한다. 게임 전역의 준비 대여는 반납할 때도 현재 Player/커서/Editor 값을 암묵 정리하지 않는다. 기존 `Acquire()`는 종전 Campaign/독립 검사 의미를 유지한다. `RawSurfaceWorld`의 명시 준비 경로는 제공된 PATCH 저장소를 요구하고 현재 연결 종류/모드를 보존한다. 새 모듈의 지역 장부·Graph 구성은 계속 남으며, 이를 전체 Native 정리가 제품에 연결됐다는 근거로 삼지 않는다.

Session의 `adoptPreparedWorld`는 지연 ExistingEditor, 같은 실제 게임 전역·표면·State·Player·Core와 초기화 완료 Player 대여를 요구한다. 이 경로는 준비된 owner/모드와 같은 Player1의 Core10BYTE 선택을 확인하고 중복 초기화·Core 선택을 생략한다. 전체 prefix 완료 판정과 실제 호출은 아직 메뉴 조립부의 후속 작업이다.

## checkpoint 이후 UI와 마지막 공개

기존 `InstallExistingEditorCheckpoint`가 정상 복귀한 뒤에도 공개 입력·Snapshot은 닫혀 있다. 새 설치 경로는 기존 Campaign.Load의 같은 후처리 여섯 하위를 한 번 준비하고, 필요한 Cursor/생산 동기화/GumpForm 조회만 동기 설치 범위에서 허용한다. 실제 UI 준비 뒤 기존 `ApplyExistingEditorAfterCheckpoint`가 HUD·활동 기한·음악·전체 팔레트·다음 State4 예약을 직접 소유한다. 단순 callback 정상 반환을 원문 tail 완료로 대신하지 않는다.

그 뒤 바깥 `RawLocalMissionState`의 정상 공통 확정이 끝나야 마지막 공개가 가능하다. 같은 State 소유자, 진입 요청144BYTE와 이전 phase, old/phase 전체 일치, current/immediate 첫 DWORD0, 다음 요청4와 현재 요청의 나머지140BYTE를 검사한다. 예외·공통 확정 없는 반환·즉시 다른 상태 실행은 완료로 받아들이지 않는다. 설치 범위의 예외는 범위만 닫고 앞서 발생한 파일/객체/UI 효과를 되돌리지 않는다.

현재 전체 HUD 공장은 이미 `UberHud.inc::InstallHud()`에 있다. 실제 제품 연결 시 이 공장을 재사용한다. Editor에 가짜 MissionRuntime이나 Campaign 로그인 완료를 만들지 않는다.

## 남은 제품 연결과 검증

이번 단위의 검증은 실제 공유 배열/소유자, 기존 Editor checkpoint와 실제 State 분배기의 조립에 한정한다. 메뉴의 `UberGump::Tick`은 아직 State1 전체를 소비하지 않는다. 실제 cleanup/prefix·로컬 listener/client9·HUD/입력 공개·편집/저장 메뉴를 연결한 사용자 GUI 검증이 남는다. 따라서 이번 설치 보호 API의 통과를 사용자의 Edit 진입/편집 완료로 표시하지 않는다.

집중 검사와 빌드 결과는 작업 종료 시 이 절에 기록한다. 보호1821파일 기준은 `extracted/cpp-editor-prepared-ui-426-baseline.json`이다. 긴 전체 검사/플레이는 사용자 결정대로 기능 묶음 완료 또는 기본3~5단위 뒤 통합한다.

빌드1에서 새 검사가 참조 필드를 가진 `UserSelectionState`를 빈 초기화한 오류를 확인했다. 실제 참조 입력으로 교정한 뒤 오류가 확인된 빌드를 중단했고, 원시 로그를 보존했다. 같은 준비 경로의 SP/난수/Kernel 필수 소유자와 캠페인 로그인 미사용 계약을 보강했다. 실제 생산 패널의 Form 부착/반납 검사도 추가했다. `extracted/cpp-editor-prepared-ui-426-source2/hashes.json`에 최종 변경10소스를 고정한 뒤 빌드2를 실행했다. 첫 빌드의 중단은 검증 통과로 세지 않는다.

빌드2는 **종료0·컴파일 경고/오류0**으로 끝났다. 집중 검사는 **49개·실패0**이다. 기존 Editor checkpoint19개, tail4개, 공유 표면5개, 초기 생산14개, 기존 Campaign Player/Core2개와 Player링크5개를 실행했다. 원문 State1/2의 정상 확정과 다음 Poll4, 설치 중 공개 거부, UI/tail 예외와 공통 확정 없는 반환, 준비 대여 보존을 확인했다. 실제 생산 패널이 같은 기술 부모에 Form을 부착하고 최종 확정 뒤 반납되는 경로도 통과했다.

보호1821파일과 고정10소스 SHA는 검사 전후 일치했다. [검증 보고서](../../cpppj/editor-prepared-ui-runtime-20261025.json)에 네 실행 파일 SHA와 두 빌드·일곱 집중 로그를 기록했다. GUI·전체 CTest·긴 사용자 플레이는 이번에 실행하지 않았으며 마지막 통합 제품은 계속345b5691이다. 실제 메뉴의 State1 소비와 전체 cleanup/prefix/로컬 전송·사용자 편집/저장 연결은 후속 범위다. **20261025-21 CP**로 인계 뒤 커밋하며 전체 목표를 계속 진행한다.
