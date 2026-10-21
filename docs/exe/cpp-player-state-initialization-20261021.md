# 빈 State 초기화와 실제 섹션 재생성 — 단위265

시작 커밋은 `a728b811`이다. 직전 턴은 State/Territory 선택 연결과 두 로컬 커밋을 마친 진행 턴이었다. `AGENTS.md`와 양 인수인계의 최신 기록을 다시 확인했다. 현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-CODEX / 2026-10-21**로 같다. 새 디컴파일·원본/CD/AGENTS/dotnetpj 수정·게임/별도 창 실행은 없다. 기존 사용자 지시대로 C++ 검사/CTest/실제 플레이 검증은 통합 단계로 이월한다.

## 문제와 변경

`RawPlayerTemplateState.Get`은 State의 data가 없거나 end가0이면 미복원 오류를 던졌다. 원본10.78 `004bd550`은 같은 조건에서 State 섹션을 다시 만들고 초기28BYTE를 쓴다. 이번에는 이 실제 생성 소비자를 연결했다.

초기 기록은 원본이 쓰는 필드만 변경한다. 원본의 미정 스택 바이트를 원래0이었다고 가정하지 않는다.

| 위치 | 원본이 수행하는 쓰기 |
| --- | --- |
| BYTE0 | `(기존값 & 0xf9) | 1`: bit0 설정, bit1/2 해제, 상위5비트 보존 |
| BYTE4~9,12~17,22~25 | 원본 DWORD/WORD/BYTE 쓰기와 같은 범위만0 |
| BYTE1~3,10~11,18~21,26~27 | 입력11BYTE 그대로 보존 |

실제 Client는 기존 `CurrentAllocationBytes`를 통해 현재 호스트의 초기화되지 않은 malloc 바이트를 공급한다. 이는 원본이 정의하지 않은 바이트의 명시 호스트 입력이며 **원본 실행 당시 스택 바이트와 같다고 주장하지 않는다**. 검사는 모든 flags BYTE와 명시 패턴을 입력으로 사용한다.

`CampaignTemplateStorage.InitializeState`는 기존 State를 목록에서 먼저 unlink하고 실제 버퍼/노드를 반납한다. 새 State는0으로 초기화한16,384BYTE 버퍼를 확보해 목록 끝에 붙인 뒤 같은28BYTE를 쓴다. 논리 end/pos는28이며 실제 버퍼 할당은 유지한다. 마지막 기존 stream을 붙이기 전에 end에 맞춰 압축하며, Territory 버퍼가 교체되면 같은 지형 cursor도 새 버퍼를 가리킨다.

목록의 현재 순서를 별도로 유지하도록 `SyncStreams`를 바꿨다. State 교체 후의 순서를 다음 조회/소멸 때 파일 번호순으로 되돌리지 않는다. 새 `SectionOrder`는 보조 벡터가 아니라 실제 head/next를 따라 현재 순서를 읽는다. 빌린 헤더도 같은 manager/current/원시 State와 cursor를 공유한다. getter는 생성 소비자가 돌려준 새 현재 stream을 사용한다.

이 연결은 호스트 노드/벡터/불투명 핸들 어댑터다. 원본 생성자의 힙 주소·노드의 미정 이름 패딩·전체 DataManager 생성/파일/오류 처리를 모두 복원한 것은 아니다. 기존 비어 있지 않은 State와 짧은 State 확장 경로는 유지한다. CD10.72 빈 State 초기화는 새 소비자 설치를 명시 거부하며 이번 근거로 완료하지 않는다.

## 제품 수정 전 원본 관측

`tools/decomp_player_state_initialization.py`로 원본 `004bd550`과 중첩 `0044cce0`을 **2,560고유 입력·2,560정상 반환·168,960명령** 관측했다. 부모 진입2,560회, 실제 MemStream 쓰기1,536회다. 원래 스택28BYTE 전후·현재 stream16BYTE·대상 버퍼64BYTE·선택/생성/최소확장 경계 순서·반환EAX·스택 정리·보존 레지스터·x87 제어어·쓰기 범위를 확인했다.

입력은 flags256값·일정/비균일 스택 패턴·data0/end0, data0/end28, data존재/end0, 기존28/32BYTE를 교차한다. 생성되는1,536행과 기존 자연 반환1,024행을 구분한다. 검색/선택 `0044ceb0`, 섹션 생성 `0044d1b0`, 최소 확장 `0044d290`은 명시 경계다. 특히 **부모 관측이 이 세 하위 본문의 전체 실행을 증명하지 않는다**. 선택은 직전 단위263의 별도864행 근거를 유지하며, 실제 호스트 재생성 연결은 현재 소스와 새 C++ 검사에 기록했다.

제품 수정 전에 최초 도구/fixture/JSON을 `extracted/player-state-initialization-20261021-first-capture/`에 동결했다. 관측 결과나 기대값을 제품 구현으로 생성하거나 최초 자료를 다시 생성하지 않았다. 원본 EXE SHA-256은 계속 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다. 같은 PC 정밀 디컴파일/보호 PE 격리 명령 관측을 사용했으며, 이전에 확인한 REA H: 속성 Win32 87 실패 호출/설정 변경은 반복하지 않았다.

## 빌드와 검증 상태

새 C++ 검사4개를 작성·컴파일했다. 실행하지 않았으므로 통과 개수로 보고하지 않는다.

- 독립2,560행의 생성 조건·미정 바이트/flags 보존·기존 자연 반환·버퍼/pos/end·경계 순서 대조.
- 실제 미션 파일의 빈 State 재생성·16,384BYTE 할당·한 번 생성·목록 끝 이동·원시 바이트/빌림 공유·반납.
- 마지막 Territory 버퍼 압축 뒤 원문 보존과 같은 typed cursor/새 버퍼 연결.
- 필수 생성 효과 누락과 CD 미지원 거부.

첫 Release 빌드는 종료0이지만 새 로컬 `name`이 기존 멤버를 가리는 C4458 경고1개가 있었다. `sectionName`으로 수정했다. 경고 수정과 새 검사 소스를 모두 반영한 최종 `build2`는 **종료0·경고0·오류0**이다. 최종 로그/원래 stdout/실제 종료 코드는 `extracted/cpp-player-state-initialization-20261021-build2.log`·`.raw.log`·`-result.json`에 보존했다.

`python tools/decomp_player_state_initialization.py --verify`, `python tools/cpp_player_state_initialization_audit.py`, 기존 섹션 선택864행·DataManager 소멸392행·Template v2 수명240행 보존 확인과 `git diff --check`는 종료0이다. 새 감사는 최초 자료 고정·고유 입력·보존11BYTE·실제 Client/Session/저장소 소스 연결과 현재 소스/바이너리 해시를 확인한다. 원본 명령 관측·정적 감사·컴파일을 C++ 실행 검증과 구분한다. 기존 session 보고서의 과거 소스/빌드 핀은 변경하지 않았다.

## 다음 작업

전체 DataManager 생성·파일 읽기/저장·오류, 원본 append/remove/compact의 전체 기계어 대조와 일반 소비자 연결, 기존 짧은 State 확장의 임시 노드 추가/rename/목록 순서 복원이 남는다. 전체 State7/전투 정리004b2df0·Fort 선택/Guide 공장/가상 소멸도 남는다. 이번에 새28BYTE의 원본 필드 초기화와 실제 빈 State 재생성을 연결했으므로 이 항목을 더 이상 전부 미복원으로 이월하지 않는다.

전체11미션 자연 완주·전략AI·지형/포획/희생/대피·외관/음향/동작 동일성·통합 실행 검증은 미완료다. 목표는 active로 유지한다. 인수인계 후 **20261021-29 CP**로 로컬 커밋하며 push하지 않는다. 마지막 전체 정밀 디컴파일 PC는 계속 **VM-W11-CODEX / 2026-10-21**이다.
