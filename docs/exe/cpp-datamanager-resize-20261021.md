# 짧은 State의 전체 최소 확장·임시 생성·이름 교체 연결 — 단위267

시작 커밋은 `78625d3d`다. 직전 턴은 공통 MemStream/DataManager 처리의 구현·빌드·문서·로컬 커밋을 마친 진행 턴이었다. `AGENTS.md`와 양 인수인계를 다시 읽었다. 현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-CODEX / 2026-10-21**로 같다. 새 디컴파일·원본/CD/AGENTS/dotnetpj 수정·게임/별도 창 실행은 없다. 기존 사용자 지시대로 C++ 검사/CTest/실제 플레이 검증은 통합 단계로 이월한다.

## 변경과 실제 소비자

기존 실제 Session의 짧은 State 확장은 새 벡터 할당과 복사/0 채움만 수행했다. 원본의 임시 섹션 생성·목록 추가·기존 State 제거·이름 교체는 생략되어 있었다. 이번에는 `RawDataManagerResize`의 세 본문과 실제 Session getter를 연결했다.

| 원본 | 공통 함수 | 원본 처리 |
| --- | --- | --- |
| `0044d070` | `ConstructDataManagerStream` | 첫16BYTE 초기화→signed 크기 최소1 보정→실제 할당→이름/NUL만 복사→next0→같은 this 핸들 반환 |
| `0044d160` | `RenameDataManagerStream` | 대상 이름 검색/기존 노드 제거→원래 이름 재검색→대상 문자열/NUL만 복사→현재 핸들 반환 |
| `0044d290` | `EnsureDataManagerStreamSize` | signed end가 최소보다 작을 때 임시 생성→생성 뒤 현재 원문/길이 복사→남은 공간0→현재 alloc로 pos/end→추가/이름 교체 |

임시 이름은 원본의 `T$e$m$p`다. 생성 뒤 원문 data/end와 실제 임시 alloc을 다시 읽는다. State가 목록 마지막이면 원문을 복사한 다음 기존 마지막 State를 압축하고, 이후 이름 교체 과정에서 그 압축된 기존 State를 반납한다. 기존 이름/NUL 뒤의 미정 패딩은 지우지 않는다.

`RawPlayerTemplateState.BindMinimumSize`가 실제 부모가 돌려준 새 현재 stream을 받는다. 실제 Session은 `CampaignTemplateStorage.EnsureStateSize`에 이 소비자를 연결하고, 기존 `GrowState/CommitStateGrowth` 분리 할당/복사 콜백은 등록하지 않는다. 따라서 새 버퍼를 받은 뒤 같은 데이터에 중복 복사하지 않는다. 분리 stream 입력과 CD의 기존 좁은 검사 어댑터는 유지하며, 이를 실제 Session 연결로 보고하지 않는다. CD에 새 전체 부모를 근거 없이 설치하지 않는다.

실제 저장소는 임시 노드/버퍼를 단독 소유하면서 같은 검색·목록·MemStream 함수를 실행한다. 완료한 새 노드/할당을 그대로 원시 State 섹션에 넘기고 typed cursor도 같은 버퍼에 연결한다. 외부에 노출하지 않는 임시 토큰은 완료 시 같은 이미지의 State 토큰으로 매핑하고 head/next/current를 함께 갱신한다. **원본 힙 주소나 외부 raw 노드 포인터 동일성 전체를 주장하는 구현은 아니다.** 현재 공개 소비자는 같은 State 바이트/cursor와 섹션 이름/순서를 사용한다.

빈 State 생성도 같은 `ConstructDataManagerStream`을 사용하도록 연결했다. 기존16,384BYTE 할당·28BYTE 초기 기록 쓰기와 보존 바이트 처리는 유지하며, 실제 생성자는 할당 뒤 이름을 쓴다.

## 제품 수정 전 원본 관측

`tools/decomp_datamanager_resize.py`로 **286고유 입력·286정상 반환·80,912명령**을 관측했다. 최소 확장174행(실제 확장120·추가 확장 없는54), 이름 교체32행, 독립 생성자80행이다. 새 부모3개와 기존 MemStream/목록/검색 하위를 합친 **열 실제 본문을 직접 실행**했다. CRT ASCII 비교/new/free/진단만 명시 경계다. 실제 CRT/힙/OS/게임 실행과 Windows 예외 실행은 아니다.

원본 진입 횟수는 할당314·쓰기234·압축120·추가120·소멸136·제거136·검색478·생성200·이름 교체152·최소 확장174회다. 이는286입력 안의 중첩 실행이며 입력 수에 중복 합산하지 않는다. 전체796BYTE 전후·각 외부 경계 직전 메모리·반환EAX·스택 정리·보존 레지스터·x87 제어어·정상 반환 시 격리 SEH(FS0) 복구·허용 쓰기 범위를 확인했다.

입력은 State가 처음/중간/끝인 목록, 길이0/1/3/8/27/28/32와 최소28, signed 최소-1/0/1, NULL/빈/일반/임시 생성자 이름, 이름 대소문자/기존 이름 유무를 포함한다. 생성 중 원문 data/end 변경, 임시 alloc 변경, free 뒤 current 변경을 명시 경계 입력으로 주어 원본 재조회 순서를 확인했다. OOM/예외·없는 노드의 잘못된 메모리·모든 가능한 크기를 증명한 관측은 아니다.

제품 수정 전에 최초 도구/fixture/JSON을 `extracted/datamanager-resize-20261021-first-capture/`에 동결했다. 새 원문에는 공용 실행기 기반의 SHA도 포함했다. 최초 기대값을 제품으로 만들거나 관측을 다시 생성하지 않았다. 같은 PC 정밀 산출물과 보호 PE 격리 실행을 사용했다. 기존 REA H: 속성 실패 호출/설정 변경은 반복하지 않았다.

## 빌드와 검증 상태

새 C++ 검사4개를 작성·컴파일했다. 실행하지 않았으므로 통과 수로 보고하지 않는다.

- 독립286행 전체796BYTE·반환 핸들·각 비교/할당/free 경계 상태 대조.
- 실제 미션 파일의 짧은 State에서 새 현재 버퍼/빌린 헤더/목록 끝 이동과 분리 할당·중복 복사 미호출 확인.
- 이미 마지막인 큰 State 할당을 줄인 뒤 최소 확장하여 원문 복사→기존 tail 압축→원래 State 반납 순서의 현재 버퍼 확인.
- signed 최소/충분한 State의 할당·순서·cursor 보존과 CD/필수 연결 거부.

최종 Release **종료0·경고0·오류0**이다. 생성자 연결과 최종 소스를 모두 반영한 `extracted/cpp-datamanager-resize-20261021-build2.log`·`.raw.log`·`-result.json`에 최종 로그/원래 stdout/실제 종료 코드를 보존했다. 새 session 보고서는 현재 소스/속성/빌드/세 바이너리 해시를 고정하며, 새4검사의 최종 등록 문자열도 읽기 전용으로 확인한다. **C++ 검사/CTest/게임은 실행하지 않았다.**

`python tools/decomp_datamanager_resize.py --verify`, `python tools/cpp_datamanager_resize_audit.py`, 기존 stream404/State 초기화2560/선택864/Template v2 수명240행 원문 보존과 `git diff --check`는 종료0이다. 기존 session 소스/빌드 핀은 갱신하지 않았다. 원본 관측/정적 감사/컴파일을 현재 C++ 실행 검증과 구분한다.

## 남은 작업

DataManager 전체 생성·파일 읽기/저장·오류·전체 섹션 생성/복제 부모, 외부 raw 노드 포인터 소비자의 소유권/정체성, 전체 State7/전투 정리004b2df0·Fort 선택/Guide 공장/가상 소멸은 남는다. 이번에 완료한 실제 짧은 State의 임시 생성/복사/추가/기존 제거/이름 교체·새 현재 stream 연결은 더 이상 전부 미복원으로 이월하지 않는다.

전체11미션 자연 완주·전략AI·지형/포획/희생/대피·외관/음향/동작 동일성·통합 실행 검증은 미완료다. 목표를 active로 유지한다. 인수인계 뒤 **20261021-31 CP**로 로컬 커밋하며 push하지 않는다. 마지막 전체 정밀 디컴파일 PC는 계속 **VM-W11-CODEX / 2026-10-21**이다.
