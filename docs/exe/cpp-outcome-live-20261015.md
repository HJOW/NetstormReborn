# 현재 raw 세션의 미션 승패 입력과 영역 소유자

2026-10-15, `VM-W11-Codex`. 현재/마지막 정밀 디컴파일 PC는 **VM-W11-Codex**다. 같은 PC의 기존 정밀 C와 실제 두 PE 명령을 읽었다. 새 Ghidra 내보내기나 실제 원본/클론 GUI 실행은 없다.

`RawMissionOutcome`가 현재 Player9 원문·방향별 동맹·사제 목록/상태·HP·지면 소유자와 연결되도록 구현했다. callback 뒤의 다음 조건은 같은 실제 저장소를 다시 읽는다. **실제 로그인/AI/동맹 초기화 전 자동 승패 판정은 추가하지 않았다.** UI/미션 게이트의 호출과 실제 초기화는 별도 작업이다.

## 같은 현재 저장소를 사용하는 입력

`RawMissionOutcome::BindCurrentInputs`는 Player 원문 span, 동맹 DWORD81칸 span과 현재 local/Demo/Tell차단 조회 콜백을 보관한다. 프레임 진입 때 값을 복사하지 않는다. 연결하지 않은 기존 독립 테스트는 종전 `MissionOutcomeState` 입력 backend를 그대로 사용한다.

- Player 활성 BYTE는 각 조회의 `+0x50`, 신전 SID DWORD는 PATCH `+0x7c`/CD `+0x74`에서 읽는다. Player 기록 폭은 PATCH `0xb4`/CD `0xac`이며9개를 요구한다.
- 팀 판정은 `[local][player]`와 `[player][local]`의 현재 DWORD가 둘 다 비영일 때만 같은 팀이다. 한쪽 동맹을 자동 대칭화하거나 bool/비트마스크 사본으로 대체하지 않는다.
- 실제 사제 목록과 `ownerState`는 기존 `RawPriestWorldBindings::Priests()`를 직접 공유한다. HP는 같은 Pool의 기존 `SquidReward::MaxHitPoints`다.
- 43개 사건 잠금은 Session의 장기 `MissionOutcomeState`에 남는다. section→임시 Gump 정리→현재 미션 통지→원래 latch=1의 순서와 재진입을 유지한다. 없는 섹션도 gate가 열려 있으면 잠금은 확정한다.
- PATCH의 신전 HP는 maxHP callback 전, CD는 callback 뒤 읽는다. `PriestSaved`의 로컬 번호는 영역 조회 callback 전에 읽은 값 하나만 보존한다. 다음 조건/플레이어에서는 현재 상태를 다시 읽는다.

## 실제 영역 소유자

새 `RawRegionOwner`는 다음 전체 몸체를 복원했다.

| 판본 | 정수 getter | float wrapper | snap | CRT |
| --- | --- | --- | --- | --- |
| 10.78 | `0046f710` | `0046f770` | `0041d7a0` | `004e49c0` |
| CD 10.72 | `004bddb0` | `004bde10` | `00440a80` | `004f161c` |

float 좌표에 원본 `0.99999f`를 x87 정밀도로 더해 ftol→float 저장→ftol을 적용한다. 정수 칸이256×256 밖이면0, hash0의 WORD SID가0이면0, 현재 표면 타입이 실제 isle 타입 전역과 다르면0이다. 일치하면 PATCH raw `+34`/CD `+32`의 현재 소유자 BYTE를 반환한다. dead/void/free 필터는 추가하지 않는다. 따라서 **섬 번호→섬 목록 Owner나 GameWorld의 관찰용 지역 배열을 거치는 조회가 아니다.**

무효 CRT 정수 결과는 실제64비트 결과의 하위 DWORD0을 보존하며 큰 유한 값의 하위 DWORD 감김도 유지한다. 이 안전한 호스트 해석을 실제 두 PE의 비유한/큰 값 입력과 대조했다. 원본이 Pool 밖을 참조하는 손상된 표면 SID는 호스트에서 명시 오류로 거부한다.

## Session과 root UI의 API

`RawCampaignSession::Impl`은 canonical 방향별 `std::array<uint32_t,81>`과 승패 latch·getter·판정기를 소유한다. `PlayerRecords()`와 `Alliances()`는 실제 초기화 모듈이 직접 수정할 같은 저장소를 빌린다. 현재 로컬 번호는 같은 `RawPlayerWorld::State().battleOwner` DWORD를 읽는다. 지면 조회도 같은 Pool/hash0와 Session의 isle 타입97을 사용한다.

root가 공급할 `services.missionOutcome`의 필드는 다음과 같다.

| 필드 | 역할 |
| --- | --- |
| `demo()` | 현재 미션 Demo DWORD 조회 |
| `tellBlocked()` | 현재 실제 Tell 차단 DWORD 조회 |
| `section(key) -> bool` | 실제 섹션 실행과 발견 여부 |
| `closeTransient()` | 성공한 섹션 뒤 원본 임시 Gump 정리 |
| `onSection()` | 성공한 섹션 뒤 현재 미션 통지/정지 소유권 처리 |

미공급 서비스는 생성 시 호출하지 않는다. 해당 효과/조회가 실제 선택되었을 때 구체적 미연결 오류를 반환한다. 기본 성공/가짜 빈 callback은 넣지 않았다.

`GameWorld::CheckMissionOutcome()` → `RawCampaignSession::CheckMissionOutcome()`는 원본 `RawMissionFrameGate::checkOutcome` 위치에서 명시 호출하는 API다. Session의 Kernel `RunFrame`에는 자동 호출을 추가하지 않았다. 구 GameWorld 어댑터에서 이 API를 호출하면 실제 캠페인 미설치 오류다.

실제 초기화는 별도 `RawPlayerParticipation` 작업과 조율했다. 현재 flags와 canonical 동맹 표를 원본 순서로 채우기 전 gate의 자동 Check를 켜면 모든 플레이어가 inactive로 보일 수 있다. 이전 `MissionPlayers::Load`는 동맹을 자동 대칭화하므로 실제 방향별 표의 대체 입력으로 사용하지 않는다.

## 검증

- 새 [독립 원본 도구](../../tools/decomp_region_owner.py)는 C++를 읽거나 실행하지 않고 두 PE의 위 네 함수·실제 CRT를 실행했다. **고유 입력1,632개, 두 x87 제어어의 정상 최상위 반환3,264회**, 외부 대체 경계0·OS 호출0이다. 정수 안/밖 경계, float 경계/이웃값·NaN/Inf/큰 값, 표면0/비영, 현재 타입 일치/불일치, 소유자 BYTE0/7/255, dead/void 입력을 포함한다. 모든 반환에서 스택/비휘발 레지스터/x87 제어어·빈 스택을 확인했다.
- 새 RegionOwner C++ fixture 대조1,632행과 현재 type/owner/표면 변경·경계/계약 검사를 통과했다. 새2개 및 기존 RegionOwnership5개가 포함되는 `--filter RegionOwner` **7개 실패0**이다.
- 기존 승패 native fixture의 **callback 변화8개와 방향별 동맹8개만** 새 raw span backend로 다시 대조했다. 독립용 state 입력은 일부러 비활성/차단/잘못된 로컬 값으로 오염시켜 stale 사본 사용을 검출한다. 기존 native 원본 관찰 수에는 재합산하지 않는다.
- 실제 보호1-1 자료를 읽어 같은 Session을 설치한 콘솔 통합 검사에서 명시 raw 참여/동맹 입력→현재 신전 HP→첫 section callback의 Player/사제/Demo 변경→후속 현재 사건/영구 latch를 확인했다. 생성/Load에서 승패 callback이 자동 실행되지 않는 점도 검사한다. 이 명시 참여 입력은 실제 로그인/AI 초기화 복원의 증거가 아니다.
- 기존 승패 전체 fixture·재진입/계약과 위 새 backend·Session 검사를 포함한 `--filter MissionOutcome` **5개 실패0**이다. 최종 Release 빌드 경고/오류0, 새 native `--verify` 감사와 `git diff --check` 통과다. 상위 통합 CTest는 **1/1·내부1,183개·실패 체크0**, 검사114.74초/전체114.85초다. 이 실행은 Early2와 Player 참여606입력 당시 변경도 포함하며, 이후 이름32바이트/608입력 보강은 별도 최종 집중 검사로 확인했다. 최종 GUI는 새로 실행하지 않았다. 통합 로그는 `extracted/run41-42-{build,ctest}.log`다.

근거는 `cpppj/region-owner.json`, `cpppj/tests/fixtures/region-owner-x86.tsv`, `extracted/region-owner-{native,tests}.log`, `extracted/outcome-live-{build,build2,build3,tests}.log`에 있다. 새 원본 도구의 해시를 갱신한 뒤의 감사 결과는 별도 실행 로그에 보존한다. 기존 `missionoutcome-x86.tsv`는 변경하지 않았다.

## 한계

실제 원본/클론 게임 실행은0회이며 새 UI/음향/픽셀 대조는 없다. 실제 Player 로그인/AI·동맹 초기화, root 게이트/Tell/정지소유권 연결, 모든 미션 사건/승패 후 다음 미션·진행 저장 및 완주 동일성은 이번 완료 범위에 포함되지 않는다. CD는 getter/native 입력과 live backend의 기존 승패 입력을 대조했으며 실제 Session 조립은 여전히10.78 전용이다. dotnetpj·AGENTS.md·보호 원본은 수정하지 않았다. LEFT 갱신과 로컬 커밋은 상위 작업자가 수행한다.
