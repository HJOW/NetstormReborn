# cpppj 미션 승패 조건·사제 상태·사건 잠금

작업일 **2026-10-13**, 마지막 디컴파일 수행 PC **VM-W11-CODEX**. AGENTS.md·두 LEFT_JOBS를 확인했고 현재 호스트와 정밀 디컴파일 호스트가 일치한다. 원본10.78 캠페인 1-1 실행 경로를 준비하는 복원이다. 게임/GUI를 실행하지 않았으며 보호 원본·AGENTS.md·dotnetpj를 수정하지 않았다. 전체화면 DirectDraw 오류 수정은 제외한다.

## 실제 행동

`RawMissionOutcome`가 현재 Player flags/신전 SID/동맹 표, 공유 `PriestPostPopState`의 사제 목록·소유자 상태, 미션의 DWORD 사건 잠금을 읽는다. 저장소를 복사하지 않으므로 스크립트/UI 효과가 바꾼 값은 원본의 다음 읽기 위치에서 바로 보인다.

사제 사망 함수 `004922e0/CD 0040da20`는 raw 사제의 dead 비트를 읽지 않는다. 플레이어 +0x50의 활성 mask `0x22`가 꺼져 있거나 번호가 범위 밖이면 사망으로 보고, 활성 플레이어는 `PriestPostPopState.ownerState[player]==2`에서만 사망으로 본다. 포획 함수는 활성 소유자의 상태 1에서 참, 비활성에서 거짓, 범위 밖에서 참이다. 패치판의 범위 assert는 선택적인 진단 효과로 전달하며 CD판은 해당 진단을 부르지 않는다.

미션은 플레이어 1~8을 원래 순서대로 검사한다. 활성 플레이어 한 명마다 신전 절반 미만 HP → 신전 사망 → 사제 사망 → 사제 포획 → 내 영역으로 사제 회수 사건을 처리한 후 그 플레이어의 사제 생존을 팀 개수에 더한다. 같은 팀은 현재 로컬 플레이어↔대상 플레이어의 두 동맹 DWORD가 모두 비제로일 때다. 한쪽만 지정된 동맹을 여기서 임의로 대칭화하지 않는다. 그 뒤 `GoodTeamDead` → `BadTeamDead` → 로컬 `Failed`를 검사한다.

신전이 없으면 halfDead/dead 조건이 모두 참이다. 신전이 있으면 halfDead는 현재 부호 있는 HP가 최대 HP의 0 방향 절삭 절반보다 작은지 검사하며, 정확히 절반이면 거짓이다. 패치판은 현재 HP를 최대 HP 함수 전에 저장하고 CD판은 그 뒤 읽는다. 신전 dead는 raw 상태 bit 2만 읽으며 free/void를 추가로 제외하지 않는다. 최대 HP는 같은 풀의 실제 `SquidReward::MaxHitPoints`로 연결한다.

사제 saved는 기존 실제 `RawPriestLookup`으로 **현재 소유자**의 첫 dead bit2가 없는 항목을 찾은 뒤 현재 좌표에서 영역 소유자를 묻는다. ownerState·free/void·타입을 여기에 덧붙이지 않는다. 영역 조회 callback 직전의 localPlayer를 저장해 비교하고, 그 callback 뒤 팀 집계에는 다시 현재 localPlayer를 읽는다.

사건 helper는 조건이 참이고 기존 잠금이 0이며 현재 Tell blocked DWORD가 0일 때만 실행한다. demo 값이 비제로면 이름 앞에 `Demo`를 붙인다. 처리 순서는 **섹션 실행 → 발견된 경우 임시 Gump 정리 → 미션 가상 +0x28 → 원래 잠금 참조에 1 쓰기**다. 섹션이 없더라도 gate가 열려 있으면 잠금은 1이 되고 helper는 참을 반환한다. gate가 막혔으면 잠금을 건드리지 않아 다음 검사에서 다시 시도한다. 조건이 거짓으로 돌아와도 잠금을 초기화하지 않는다. 콜백 안의 재진입을 차단하지 않으며 최종 잠금을 앞당겨 쓰지 않는다.

## 원본 근거

| 역할 | 패치 10.78 | CD 10.72 |
| --- | --- | --- |
| 전체 승패 조건 | 004c36c0 | 00410380 |
| 조건/사건 잠금 | 004c3940 | 00410a90; 일부 조건은 전체 함수 안에 inline |
| 사제 사망 | 004922e0 | 0040da20 |
| 사제 포획 | 00492340 | 0040da70 |
| 현재 소유 사제 | 004921f0 | 0040d940 |
| 현재 최대 HP | 004adcb0·0049fce0·0049a840 | 004aed30·00444720·004abae0 |
| Tell gate | 004c8e90 | 004c3940 |
| CD GoodTeamDead의 섹션 helper | 전체 helper 사용 | 004102d0 |

`tools/ghidra/missionoutcome-functions.json`으로 패치 9개·CD 10개 함수를 읽기 전용 내보냈다. 첫 CD 내보내기의 잘못된 보조 주소를 실제 호출 주소로 교정했으며 실패 로그를 삭제하지 않았다. 최종 내보내기는 모두 성공했다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-13**.

`tools/decomp_missionoutcome_oracle.py`는 C++ 코드를 읽거나 호출하지 않고 두 PE를 별도 Unicorn 주소 공간에서 정상 반환까지 실행한다. 판본당 364개의 명시 입력은 사제 상태 쿼리 160개, 단일 사건 helper 32개, 전체 미션 검사 172개다. 합계 **728개**이며 준비/짧은 smoke/중복 재실행은 새 독립 입력에 더하지 않는다.

원본 조건·사제 현재 목록·소유자 상태·전체 최대 HP·gate·잠금은 실제 기계어다. **서식기, 영역 소유자, 스크립트/UI 효과, 패치 범위 assert의 기록**은 명시 경계다. 현재 로컬/활성 플래그/신전 SID/사제 상태/blocked/demo/잠금을 바꾸는 효과 입력을 사용해 이후 재조회와 latch 마지막 쓰기를 확인한다. 패치 범위 밖 쿼리에 대한 assert 기록 96회는 의도된 입력 결과이며 CD에는 없다. 실제 OS/IAT/허용 함수 밖 실행 및 스택/미션 잠금 밖 native 쓰기는 차단한다. 두 판본 모든 루트의 정상 EIP/ESP·비휘발 레지스터 보존을 검사했다.

`MissionOutcomeTests.cpp`는 같은 명시 입력에서 효과 당시 상태/43개 DWORD 잠금/결과를 fixture와 직접 비교한다. 최대 HP와 현재 사제 조회에는 기존 실제 모듈을 사용한다. 같은 helper callback의 재진입/최종 쓰기 순서와 필수 효과 누락/다른 풀 연결 거부는 별도 C++ 검사다. 재진입 전용 C++ 검사를 독립 PE 실행 수에 포함하지 않는다.

자료는 `cpppj/tests/fixtures/missionoutcome-x86.tsv`, `cpppj/recovery-missionoutcome-evidence.json`, `extracted/missionoutcome/`, `extracted/missionoutcome-export*.log`다. CMake의 source/test/fixture 등록은 상위 작업자가 맡았다. 첫 Release 빌드에서 검사 공용 namespace의 `kCapacity`와 이름이 충돌한 것을 확인해 고유 상수 `kOutcomeCapacity`로 교정했다. 정정 빌드의 두 새 C++ 파일 컴파일과 검사 EXE 링크 뒤 첫 집중 실행을 진행했으나 실제 `SidPool`이 native 합성 VM의 128슬롯 용량을 허용하지 않아 세 검사 모두 생성에서 실패했다. 테스트의 실제 풀만 **32768**로 교정했고 fixture/명시 입력/관찰 범위는 **128슬롯** 그대로 유지했다. 추가 슬롯을 결과 대조에 섞지 않는다. 최초 집중 실패 로그 `extracted/missionoutcome-cpp-focused.log`와 빌드 실패 로그를 보존했다.

교정 뒤 최종 Release 빌드는 종료0·경고/오류0이다(`extracted/campaign-runtime-build-validated-final.log`, 상위 작업자의 빌드 handle 9980). **MissionOutcome 3개·실패0**이며 그 안의 **728개 실제 PE/C++ 입력/효과/상태 대조**가 통과했다. 기존 **PriestLookup 8개·PriestPostPopTail 6개·실패0**, 원본 **728개 최종 SHA/입력 계획/정상 반환 수 감사 통과**다. 최종 고유 로그는 `extracted/missionoutcome-cpp-focused-final.log`, `missionoutcome-priestlookup-final.log`, `missionoutcome-priestpostpop-final.log`, `missionoutcome-verify-final.log`다. native 재실행/검증 및 C++ 집중 대조를 새 원본 독립 입력에 중복 합산하지 않는다. 새 코드/문서/함수목록 7개의 UTF-8 엄격 디코딩도 확인했다.

## 남은 실제 연결

이 모듈을 Client/GameWorld의 실제 미션 process에 아직 등록하지 않았다. 전체 게임의 Player 상태·신전 SID·사제 ownerState·동맹 표를 공유하고, 미션 section 실행/임시 UI 정리/가상 +0x28/영역 조회의 실제 효과를 연결해야 한다. 실제 골렘 운반/알타 의식이 ownerState를 원본처럼 변경한 뒤 `BadTeamDead`가 발생해야 한다. `<$Config,DoneTheWarBegins=1>`·성공 창·저장·다음 미션 해제와 원본10.78 실제 1-1 진행/완주는 별도 목표로 남아 있다. 신전 파괴 즉시 승리라는 대체 조건은 추가하지 않았다.
