# 저장 다이스·룬의 최초 Pop 등록 — 단위240

작업일은 **2026-10-20**, 시작 커밋은 `0fee4104`다. 현재 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-20**이다. 기존 정밀 디컴파일 산출물과 복원 모듈을 읽었고 새 디컴파일이나 원본 자료 변경은 없다.

## 원인과 변경

`RawCampaignSession::Load`는 저장 월드를 먼저 읽고 `ConnectBuilding`·`ConnectMotion`을 실행했다. 의식 모듈과 `SetDaisPostPop`·`SetRunePostPop`은 `ConnectMotion`의 마지막 부분에서 설치되므로 저장된 다이스165·룬166의 최초 Pop에는 전체 파생 처리기가 없었다. 이 순서 문제는 소스에서 확인했으며 수정 전 게임 실행으로 관측한 실패라고 주장하지 않는다.

건설·이동·의식 **처리기 구성**을 `runtime->Load()` 앞으로 옮겼다. 구성 시 게임 객체를 만들거나 프레임을 진행하지 않는다. 원래 로더의 Player 생성→지형 생성→저장 객체→Graph 재구성 순서는 유지한다. 깊이 변경 콜백에서 설치된 의식 상태에 현재 `missionPost`와 Player flags를 다시 공급하고, 종료 시 다이스 파생 포인터도 명시적으로 해제한다.

실제 효과는 기존 `RawDaisLifecycle`·`RawRunePostPop`·Factory·Owner·Host 몸체를 그대로 사용한다. 다이스 Player SID 등록, 제단 생성·전체 Pop, 제단 로컬21 예약을 새 구현으로 대체하지 않는다. 근거는 10.78 다이스 `00449320`·제단 `00448ad0`와 [기존 판본별 복원 기록](cpp-dais-lifecycle-20261014.md)이다. CD10.72의 대응 주소는 `00481360`·`004835c0`이며 이번 Session 제품 범위는 10.78이다.

## 준비한 검사와 검증 범위

`CampaignDais_SavedRecordFirstPopRegistersAltar`를 추가했다. 원본 `thewarbegins.fort`를 읽고 **메모리 사본의 Terr00 거주 구역89 한 개**를 다이스165로 치환한다. 원본 파일은 쓰지 않는다. 기존 좌표·선택 청크·순서를 사용하는 저장 로더가 실제 다이스와 제단을 생성하는지, 현재 Player SID 두 필드·제단 소유자/동일 위치·로컬21 예약·삭제 깊이를 확인하도록 작성했다. 이것은 명시 입력의 연결 검사이며 원본 맵의 자연 플레이 증거가 아니다.

정적 자산 확인에서 원본 맵의 Terr00 청크5·칸(5,14)에 소유자1 거주 구역이 존재함을 확인했다. 초기 검사 초안에서 워크샾을 선택했으나 이 맵에 해당 저장 객체가 없어 거주 구역으로 고쳤다. 첫 빌드의 선택 필드 초기화 C4244 경고도 원래 저장 폭의 명시 타입으로 수정했다. 각 빌드 원문은 `extracted/cpp-dais-load-20261020-build*.log`에 보존한다.

최종 빌드·해시·핀 보존 결과는 `cpppj/dais-load-session.json`과 `python tools/cpp_dais_load_audit.py --verify`를 따른다. 사용자의 통합 검증 이월 지시를 유지하여 **게임·단위 검사·CTest·전체 회귀를 실행하지 않는다.** 새 검사의 성공이나 전체 동일성을 빌드 성공으로 대신하지 않는다. 기존 원본 기대값·과거 검사/바이너리·보고서 이력은 보존한다.

## 다음 작업

Devastation의 표면 삭제·다리 끝 변환은 `MakeCampaignBombBlastHooks`에서 현재 `World.Destroy`·`core.bridgeEvents.ConvertEnd`에 이미 연결되어 있다. 이번 정적 확인을 실제 지형 효과 통과로 표시하지 않는다. Early5의 자연F 이후 실제 UI 시전·적/지형 효과·포획/Drop·제단/희생과 나머지 지정 미션/F8 Guide의 통합 플레이는 남아 있다. 제단 해방의 가상 소유자 연결·홈섬 밖 경고·초기 다이스 추가 그림도 후속이다. Devastation 외 주문의 3차 이월을 유지한다.
