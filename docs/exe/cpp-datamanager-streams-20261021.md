# MemStream 전체 하위와 DataManager 추가·제거 연결 — 단위266

시작 커밋은 `0429e571`이다. 직전 턴은 빈 State 초기화/실제 섹션 재생성과 문서·로컬 커밋까지 마친 진행 턴이었다. `AGENTS.md`와 양 인수인계를 다시 확인했다. 현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-CODEX / 2026-10-21**로 같다. 새 디컴파일·원본/CD/AGENTS/dotnetpj 수정·게임/별도 창 실행은 없다. 기존 사용자 지시대로 C++ 검사/CTest/실제 플레이는 통합 검증으로 이월한다.

## 변경과 원본 근거

직전 단위의 실제 빈 State 재생성에는 호스트 저장소 안에서 직접 작성한 압축/링크 처리가 남아 있었다. 이번에는 공통 `RawMemStream`과 `RawDataManagerList`를 복원하고 실제 State 재생성을 그 함수들에 연결했다.

| 원본 본문 | 이번 공통 함수 | 보존하는 처리 |
| --- | --- | --- |
| `0044cc20` | `AllocateMemStream` | alloc 선행 쓰기→할당→현재 alloc 재조회/0 채움→pos/end0 |
| `0044cce0` | `WriteMemStream` | signed 용량 진단→현재 data/pos 재조회→한DWORD씩 앞에서부터 복사→나머지BYTE→현재 pos/end 갱신 |
| `0044cd40` | `CompactMemStream` | end==alloc 무동작, 양의 end 재할당/현재 길이 복사/옛 버퍼 반납, 나머지 실제 소멸 |
| `0044ce10` | `AppendDataManagerStream` | 현재 마지막 노드 압축 뒤 next/current 쓰기, 빈 목록은 head/current만 쓰기 |
| `0044d0f0` | `RemoveDataManagerStream` | 먼저 unlink→같은 MemStream 소멸→같은 노드 반납, stale current 유지 |
| `0044d040` | 기존 `DestroyMemStream` 재사용 | 실제 버퍼 free 뒤 data/alloc/end/pos0 |

DWORD 겹침 복사는 일반 memcpy/memmove로 치환하지 않았다. 진단/allocator/free 경계 뒤의 재조회와 양의 압축에서 free 뒤 필드 변이를 보존하는 원본 차이도 유지한다. 물리적으로 부족한 span이나 실제 선택된 필수 소비자 누락은 명시 오류로 거부한다.0길이/빈 목록이 실제 접근하지 않는 버퍼/노드 소비자는 요구하지 않는다.

`CampaignTemplateStorage`는 같은 원시 섹션 벡터를 사용한다. 임시 할당은 별도 불투명 토큰으로 원본 함수에 빌려주며, 옛 버퍼는 free 전까지 유지하고 새 실제 할당은 완료 시 같은 원시 섹션에 넘긴다. 실제 State 재생성이 제거·16,384BYTE 할당·추가/마지막 압축·28BYTE 쓰기를 공통 함수로 수행한다. 기존 소멸과 같은 실제 버퍼/노드 반납을 공유한다. 호스트 토큰/벡터 어댑터이며 원본 힙 주소가 같은 것은 아니다.

## 제품 수정 전 독립 관측

`tools/decomp_datamanager_streams.py`로 여섯 실제 본문을 **404고유 입력·404정상 반환·17,826명령** 관측했다. 하위 압축/할당/쓰기/소멸을 대체 호출로 가로채지 않았다. **new/free/진단만 명시 외부 경계**다. 실제 CRT/힙/OS/게임 실행은 아니다. void EAX는 원본이 보장하지 않는 값이므로 결과 계약에 넣지 않았다.

진입 횟수는 할당114·쓰기234·압축120·추가108·소멸32·제거32다. 이 수는404행 안의 중첩 실행이며 별도 입력 수로 합산하지 않는다. 행 구성은 할당48·쓰기168·압축48·추가108·제거32다. 전체716BYTE 전후와 각 외부 경계 직전 같은 메모리, 명령의 허용 쓰기 범위·스택 정리·EBX/ESI/EDI/EBP·x87 제어어를 확인했다.

입력은 빈/기존 포인터,0/작은 길이/DWORD 및 나머지 복사, 겹치는 원문/대상, signed 음수 end, 빈/비어 있지 않은 목록, NULL 추가 인자, 첫/중간/마지막 제거와 진단/할당/free 뒤 필드 변이를 포함한다. 없는 노드의 잘못된 접근·OOM/예외 경로와 모든 가능한 길이를 증명한 관측은 아니다. 실제16,384BYTE 생성 크기는 현재 PC `0044d1b0`의 호출 인자와 기존 빈 State 연결 근거를 유지한다.

제품 수정 전에 최초 도구/fixture/JSON을 `extracted/datamanager-streams-20261021-first-capture/`에 동결했다. 최초 기대값은 제품 코드로 만들지 않았고 다시 생성하지 않았다. 공용 실행기 기반 `tools/decomp_local_mission_transport.py`가 시작 커밋과 같은 바이트임도 새 감사에 기록했다. 같은 PC 정밀 산출물과 보호 PE 격리 실행을 사용했으며 기존 REA H: 속성 실패 호출/설정 변경은 반복하지 않았다. 마지막 정밀 디컴파일 PC는 그대로다.

## 검증 상태

새 C++ 검사4개를 작성·컴파일했다. 원문404행 전체 바이트/경계 대조,0길이·물리 계약, 진단 뒤 선행 링크 수리 재조회, 실제 템플릿의 반복 State 재생성/빈 마지막 버퍼 반납을 포함한다. 선행 링크 수리는 별도 합성 계약 사례이며 원문404행 수에 넣지 않는다. 직전 State 생성4검사와 기존 전체 검사 소스도 같은 빌드에 남겨 두었으나 실행 통과로 보고하지 않는다.

최종 Release **종료0·경고0·오류0**이다. `extracted/cpp-datamanager-streams-20261021-build2.log`·`.raw.log`·`-result.json`에 최종 로그/원래 stdout/실제 종료 코드를 보존했다. `cpppj/datamanager-streams-session.json`은 현재 소스·속성·공용 기반·원문·빌드·세 바이너리를 고정한다. 새4검사의 등록 문자열이 최종 검사 바이너리에 있는지도 읽기 전용으로 확인했다. **C++ 검사/CTest/게임은 실행하지 않았다.**

`python tools/decomp_datamanager_streams.py --verify`, `python tools/cpp_datamanager_streams_audit.py`, 기존 State 초기화2560행·섹션 선택864행·DataManager 소멸392행 보존 확인과 `git diff --check`는 종료0이다. 과거 session의 소스/빌드 핀은 갱신하지 않았다. 원본 관측/정적 감사/컴파일을 현재 C++ 실행 검증과 구분한다.

## 다음 작업

기존 짧은 State 확장의 임시 노드 추가·rename·목록 순서를 같은 공통 함수로 연결한다. DataManager 전체 생성·파일 읽기/저장·오류, 섹션 생성/이름 교체의 전체 부모, 전체 State7/전투 정리004b2df0·Fort 선택/Guide 공장/가상 소멸도 남는다. 이번에 완료한 공통 할당/쓰기/압축/추가/제거와 실제 빈 State 소비자는 더 이상 모두 미복원으로 이월하지 않는다.

전체11미션 자연 완주·전략AI·지형/포획/희생/대피·외관/음향/동작 동일성·통합 실행 검증은 미완료다. 목표는 active로 유지한다. 인수인계 후 **20261021-30 CP**로 로컬 커밋하며 push하지 않는다. 마지막 정밀 디컴파일 PC는 계속 **VM-W11-CODEX / 2026-10-21**이다.
