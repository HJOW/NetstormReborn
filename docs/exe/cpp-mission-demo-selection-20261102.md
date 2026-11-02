# 현재 미션 demo와 실제 사용자 선택 관문 (단위743)

2026-11-02 / HJOW-Athlon / Windows10. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. AGENTS.md의 수정된5차 요구와 현재 인계를 다시 읽었다.742의 ESC 선택 정리·실제 검증은 완료한 진척으로 유지한다. 현재 실행 제품은 **743 / 표시11.01**이며 전체1~8차 목표는 진행 중이다.

## 문제와 원문

실제 `GameWorld::Input`은 선택기의 `blocked`를 한 번도 갱신되지 않는 `consoleFocus`에 연결했다. 원문 선택기와 ESC가 호출하는 `00463b50`은 콘솔 상태를 읽는 함수가 아니다. `005634a0`의 현재 미션 포인터가 있고 그 객체의 `+0x68` demo DWORD가 비영이면1을 반환한다.

기존 Client는 이미 `missionRuntime_ ? missionRuntime_->gate.demo : 0U`를 Session 서비스에 제공하며, AI/생산/세계 간헐천도 같은 현재 조회를 사용한다. 별도 상태나 Script demo 키로 모드를 추정할 필요가 없다. 기존 선택기의 명시 관문 검사는 옳았지만 실제 World/Session 접합이 누락돼 있었다.

원본10.78 PE의 `00463b50` 몸체를 격리 실행했다. 미션 포인터 NULL/유효와 demo DWORD `0,1,2,0x80000000,0xffffffff`의10입력에서 NULL은 모두0, 유효 포인터의0은0, 나머지는1이었다. 기존 허용 범위/정상 반환/스택 검사를 사용했다. 원문이나 기존 fixture는 바꾸지 않았으며 원본 게임 프로세스도 실행하지 않았다. 이10입력을 전체 미션/선택의 동등성으로 확대하지 않는다.

## 변경

`RawCampaignSession::MakeUserSelection`이 `blocked`를 같은 장기 Session의 현재 `missionOutcome.demo` 조회에 연결한다. 생성 시점 값을 저장하지 않으며 비영 DWORD를1로 정규화한다. 공급되지 않은 서비스는 기존 `FillMissing` 오류 경계를 유지한다. World의 임시 `consoleFocus`와 그 가짜 연결은 제거했다.

`RawUserSelection::Select`의 앞쪽 정지/비활성 검사, 효과 순서와 독립 `Clear` 몸체는 수정하지 않았다. 따라서 demo 상태의 일반 선택과 ESC의 `ResetSelection→Select(0,0,0)`은 차단되지만 미션 전환용 `ClearSelection→Clear`는 그대로 정리한다. 이번 변경으로 Console/모달 전체 입력 처리를 복원했다고 판단하지 않는다.

## 검증과 제품

기존 `CampaignObjectMenuTests.cpp`에 실제 Session/World를 거치는 검사2개를 추가했다. 일반 미션의 실제 사제 선택 비트, 실행 중 demo 변경, 비영 DWORD4종, 유효하지 않은 대상보다 먼저 차단, 기존 선택/다중 목록/강조 보존, 정지·비활성의 앞쪽 단락 평가, demo 중 독립 정리, demo0 복귀를 확인한다. 명시 UI 장치만 공급하며 선택/가상 통지/표시/프로세스는 실제 제품 몸체를 사용한다. 테스트용 일반 미션 서비스도 demo0을 명시 제공한다.

- 수정 전 실제 관련 대상 빌드 뒤 새2개 검사에서 **20단언 실패**를 검출했다. 원시 결과는 보존했다.
- 수정 후 Release 제품과 기존 집중 대상2개를 빌드했고 경고/오류0이었다. 메뉴·Session **28개/실패0**, 기존 선택 **23개/실패0**, 합계51개다. 메뉴의 기존478/1024행, 선택의 기존1074/87/154행 및 다중/다음/사제 선택 회귀를 포함한다.
- 새 실행 대상·fixture·CLI·게임 프로세스는 모두0이다. 테스트 실행66개/전체71대상은 유지한다. 전체 C++ 또는 전체34 재실행 결과가 아니다.
- 보호 **2654개(AGENTS1·원본1394·CD425·fixture834)**는 모두 불변이다. 다음 보호 기준은 `extracted/cpp-mission-demo-selection-743/protected-after.json`이다.

EXE SHA-256은 `777a62d333b82b356e9d08566453b8baeb26f833b59eccc945cae6543c10748f`다. 마지막 실제 GUI는742 제품의 ESC 전후 검사이며 이번 제품의 GUI 결과로 합산하지 않는다. 전체34/ZIP645는644제품의 근거다. 자연 완주/전체 동일성, 물리 입력·전체Console/모달·픽셀/PCM, 제외된 포획/희생·특정 적 공격·승리, 공중3종 전용 행동, MP 검사는 수행하지 않았다.10.72의 대응 관문도 새 관측 범위 밖이다.

보고서는 `cpppj/mission-demo-selection-20261102.json`, 원시 native/보호/빌드/전후 검사와 재현 스크립트는 Git 제외 `extracted/cpp-mission-demo-selection-743/`다.739의 선택 빌드와 양쪽 README의 컴파일 안내,733의5차 전체 자동 파일 이관·일반 권한 요구를 유지한다.
