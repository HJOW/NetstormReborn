# cpppj 너겟 내용물 삭제·공유 Form 분배

작업일 **2026-10-13**, 마지막 디컴파일 수행 PC **VM-W11-CODEX**. AGENTS.md·두 LEFT_JOBS와 현재 작업 트리를 다시 확인하고 앞선 변경을 보존했다. 캠페인 1-1 완주를 위한 경제/수명 연결 단계이며 전체 원본 동일성·자동 raw UI/건설·미션 완주는 아직 미완료다. 전체화면 DirectDraw 오류 수정은 제외한다.

## 구현과 원본 의미 교정

`ContentFormLifetime`을 기존 `SidPool`/`RawSquidDestroy`/`ContentFormHost`에 연결했다. 커서가 이 내용물을 가리키거나 내용물 부모가 자기 SID인 경우 원본 커서 취소 몸체를 먼저 요청한다. 패치판의 너겟(타입154) 내용물이고 부모가 dead가 아니면 로컬 수입 명령 또는 boss의 다른 소유자 알림을 요청한다. +22 BYTE가 0일 때만 **AI 지갑 선가산→SP 가산→현재 부모 중심에서 guy.wav 요청**을 수행한다. CD판은 이 특수 지급/알림 없이 공통 pre 깊이만 반납한다.

kind 검사는 커서 취소 뒤, paid 검사는 수입 알림 뒤의 현재 슬롯을 읽는다. 지갑 가산 전 소유자/비용, 지갑 효과 뒤 현재 소유자/비용/로컬 소유자, SP 효과 뒤 현재 좌표/발자국을 각각 다시 읽는다. 이미 통과한 부모 dead/kind 조건을 알림 뒤 임의로 재평가하지 않는다. 지갑 금액은 float 비용의 실제 CRT 절삭/하위 DWORD이고 SP 금액은 float 비용이다.

`SquidReward::CreditWallet`은 기존 AI 지갑의 비활성/부착/최솟값/DWORD 감김 몸체를 공개 경계로 사용한다. 기존 실제 `AddSp`/SP 저장소와 연결하면 너겟의 선가산과 SP 가산 내부의 지갑 가산을 모두 유지한다. 패치 실제 SP 검사에서 비용 50.75를 지급하면 SP 1000→1050.75, AI 지갑 0→100이다. CD의 같은 내용물 삭제는 SP/지갑을 지급하지 않는다.

앞 단계에서 생산 보충으로 설명한 `004927f0/CD 0040df80`은 **사제 포획 알림**이다. 사제 내용물(0x200000)을 워커/벌룬(0x30000)에 담는 전투 부착에서 호출하며, 내용물 +12의 하위7비트는 이전 사제 소유자다. API를 `ContentPriestNotice(parent,previousOwner)`로 교정했다. 명시 경계 안의 실제 포획 경고·소리/표시/재알림 타이머 연결은 남았다.

`004ae080/CD 004af1c0`은 목록 flags가 아닌 **부모 종속 체인 안에 자기 SID가 있는지** 검사한다. `ContentFormHost::Linked`는 전체 next 체인을 따라가며 상태/타입 필터를 추가하지 않고 자기 SID 뒤의 무효 번호도 false로 처리한다. 전투 부착 알림 뒤 이 구조를 재조회한다. 기존 가상 +60의 WORLD/0x34 목록 flags 검사(`Robust`)와 구별한다.

`SquidFormDispatch`는 Depend/Gump/Content/실제 ProcessForm 타입의 pre/post/release/Unpop을 현재 raw 타입으로 분배한다. 종속 삭제는 같은 루트로 재진입하고 공통 전송·선택 해제는 자산 경계로 바로 보낸다. 다른 fallback 사슬이 같은 타입을 반복 분배하는 순환을 피하며 미등록 가상 타입은 거부한다. 자산의 가상 삭제 재정의도 유지한다.

**실제 `RawSurfaceWorld`**의 자산/프로세스 삭제를 이 분배기로 전환했다. 감시·Gump·Content·프로세스가 혼합된 부모에서 각 알림이 현재 자기 Form을 삭제하는 별도 검사도 통과했다. 부모가 선저장한 next와 dead/free 재조회·공통 깊이·UI 소유 DWORD·Kernel 제거·SID 반납을 공유한다. 전체 Client UI와 비표면 건물/사제 등에는 실제 호스트 등록/효과 공급을 계속해야 한다.

## 독립 근거와 검증

`tools/ghidra/contentlifetime-functions.json`으로 패치 **9개**, CD **7개** 함수를 읽기 전용 내보내기했다. 원본 특수 preDestroy·커서 취소 조건·부모 구조 검사·중심 좌표/발자국·CRT 절삭·genus 조회다. 실제 사제 포획 함수의 정밀 출력을 읽어 경계의 의미/인자를 확인했다. 마지막 디컴파일 수행 PC: **VM-W11-CODEX, 2026-10-13**.

| 역할 | 패치 10.78 | CD 10.72 |
| --- | --- | --- |
| Content preDestroy | 004aeeb0 | 004b0300 |
| 내용물 커서 취소 조건 | 00447390 | 0043d4c0 |
| 부모 구조 포함 검사 | 004ae080 | 004af1c0 |
| 소리 중심 / 발자국 | 004adea0 / 0049a900 | 004aef30 / 004442b0 |
| 사제 포획 알림 경계 | 004927f0 | 0040df80 |

`tools/decomp_contentlifetime_oracle.py`는 C++를 읽거나 호출하지 않고 두 실제 PE를 별도 Unicorn 주소 공간에서 정상 반환까지 실행한다. **판본당 198개·합계 396개** 독립 루트는 삭제158·구조 검사24·전투/사제/워커 조건 부착16이다. 준비 초기화는 이 수에 포함하지 않는다. 원본 common destroy·Content preDestroy·커서 조건·Unpop·SID 반납·삭제 기록·실제 중심/발자국/CRT·구조/부착 몸체를 실행한다.

kind/paid/부모 dead/소유자/boss/커서 전체 조합, 콜백이 바꾼 kind/paid/owner/cost/localOwner/좌표/dead, 소수/음수/하위 DWORD 감김 비용, 자기 부모, 목록 flags와 무관한 구조 검사·자기 뒤 무효 next, 전투/사제/워커 조건과 이전 소유자 low7 전달을 포함한다. 실제 몸체 밖 실행·허용 범위 밖 쓰기를 차단하고 정상 EIP/ESP·보존 레지스터·x87 53비트 제어 입력을 감사했다. 두 판본 assert 도달은 0회다.

명시 경계는 커서 취소 전체 몸체·UI/빈 패킷 생성/송신·AI 지갑/SP·소리 및 사제 포획 알림이다. 이 경계의 실제 원본 소켓/장치/전체 UI 실행 대조를 완료했다는 뜻은 아니다. C++에서는 같은 명시 mutation과 콜백 당시 Form/부모 전체 바이트·현재 전역/비용을 기록해 직접 비교한다. watched 슬롯 전체는 바이트 대조, 그 밖의 풀은 Adler-32다. Adler-32는 암호학적 전체 동일성 증명이 아니다.

**최종 Release 라이브러리/게임/검사 빌드 경고·오류0**, 새 수명 검사 **4개**, 기존 Form 검사 **4개**, 표면 월드 검사 **4개** 통과, **396개 PE/C++ 대조·저장 SHA/사례 감사 통과**, 최종 전체 CTest **867개·실패0**(81.52초)다. 첫 전체 검사도 통과했으며 최종 사제 포획 부착 확장 뒤 다시 검증했다. 첫 빌드의 auto 형식/테스트 macro 오류와 CD 경제 검사 입력 오류를 수정한 뒤 통과했다. 실패 로그는 보존했다. SOURCE_MAP 내용은 동일하고 원래 줄바꿈을 유지했다. 이전 누적에 더한 새 원본 입력 **525,542개**, C++ 대조 **525,282개**이며 중복 재실행을 새 입력으로 세지 않는다.

자료는 `cpppj/recovery-contentlifetime-evidence.json`, `cpppj/tests/fixtures/contentlifetime-x86.tsv`, `extracted/contentlifetime/`와 `extracted/contentlifetime-*.log`다. 보호 두 PE의 SHA는 기존 기록과 같고 원본 게임/GUI·AGENTS.md/보호 원본/dotnetpj 변경·에이전트의 커밋/푸시는 없다.

## 다음 작업

초기화 전 UI/Kernel의 실제 정리·전체 Client/월드 Form 분배와 내부 재진입 등록, 사용 중인 지정 ProcessForm의 재등록, 현재 명령 큐/동적 메뉴·원본 사제 포획/커서 취소 효과·자동 raw UI가 남았다. 비표면 객체의 실제 공간/수명·게임 명령·건설/경제/전투·미션 스크립트 실행과 원본 캠페인 1-1 완주 대조를 계속한다.
