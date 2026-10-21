# DataManager와 섹션 MemStream의 소멸 연결 — 단위262

사용자의 cpppj 다음 작업 요청으로 단위261 뒤 일시 정지한 구현을 재개했다. 시작 커밋은 `eac5cfd2`, 현재 PC와 마지막 정밀 디컴파일 PC는 **VM-W11-CODEX / 2026-10-21**로 일치한다. 이번에 디컴파일을 다시 생성하거나 원본/CD/AGENTS/dotnetpj 파일을 변경하지 않았다.

재개 목록의 전체 State7/전투 정리 `004b2df0`에는 연결 종료, 월드, UI, 템플릿과 타이머 정리가 함께 들어 있다. 이번 단위는 그 목록 중 **DataManager 연결 섹션의 소멸**을 복원하고 실제 Template 저장소에 연결했다. 전체 비동기 State7과 파일 읽기/저장/오류 경로는 여전히 후속 작업이다.

## 원본 근거와 실행 경계

| 식별자 | 근거 | 확인한 내용 | 남은 경계 |
| --- | --- | --- | --- |
| DM262-S1 | 현재 PC `extracted/refined/originals/Netstorm.c`, `functions.tsv`, 원본 PE 명령 | `004bd330`은 5바이트 `jmp 0044da60`이다. 함수표의 cdecl 표기와 달리 ECX의 같은 manager를 실제 thiscall 몸체에 넘긴다. | 원래 C++ 선언과 전체 클래스 구조는 복원 소스가 아닌 추론이다. |
| DM262-N1 | `datamanager-lifetime-x86.tsv`, `datamanager-lifetime-native.json` | 세 본문 진입, 전체412BYTE, 각 free 직전 메모리, 목록 변이 뒤 head 재조회, ABI/보존 레지스터/x87/쓰기 범위 | CRT free만 명시 외부 효과다. 실제 힙·OS·게임·파일 실행이 아니다. |
| DM262-C1 | `RawDataManagerLifetime.*`, `CampaignTemplateStorage.cpp`, Release 빌드 | 원본 소멸 순서의 C++ 복원과 같은 Fort 원시 섹션 벡터/실제 노드 할당의 반납 연결 | C++ 실행 동일성은 아직 확인하지 않았다. |

`0044da60`은 base 가상 표 `00507dd8`을 쓰고, 각 노드의 `+38`을 읽어 **manager.head를 먼저 다음 노드로 옮긴다**. 그다음 `0044d040`으로 버퍼를 반납하고 같은 노드의 할당을 free한다. 각 외부 효과 뒤에는 현재 head를 다시 읽는다. 따라서 현재 노드의 next가 free 효과에서 바뀌어도 이미 저장한 다음 head는 유지되며, manager.head 자체의 변이는 다음 순회에 반영된다.

`0044d040`은 첫DWORD의 버퍼가 비영일 때만 free를 호출한다. 그 뒤 pointer, alloc, end, pos 순서로 네 DWORD를0으로 만든다. manager.current, 섹션 이름40BYTE, next와 다른 필드는 지우지 않는다. 빈 목록도 base 가상 표를 쓰며, 빈 버퍼는 free 장치 없이 자연 반환한다. 호스트의 부족한 span이나 연결되지 않은 필수 장치는 명시 오류로 거부한다.

제품 변경 전에 **392고유 입력·392정상 최상위 반환·20,740명령**을 관측했다. 실제 본문 진입 수는 `004bd330` 192, `0044da60` 384, `0044d040` 664다. 목록0/1/3/5개, 정방향/역방향, 빈/일부/전체 버퍼, 버퍼 및 노드 free 뒤 변이와 단일 MemStream 소멸을 교차했다. 전체 manager16BYTE와 다섯 노드60BYTE의 양쪽8BYTE를 저장한다. 첫 도구 실행은 예상 입력 수를 잘못 계산한 마지막 assert에서 중단됐고 파일을 만들지 않았다. 행수 계산만392로 고친 뒤 원본 관측을 완료했으며 제품 코드는 그 이후 작성했다.

최초 도구/fixture/JSON을 `extracted/datamanager-lifetime-20261021-first-capture/`에 동결했다. 도구의 `--verify`는 SHA와 원문만 검사하고 명령 실행을 반복하지 않는다. 기존 Template 수명 v2의240행과 원문도 보존했다.

REA `open_binary`는 원본 SHA를 확인했지만, 후속 `procedure_pseudo_code`는 H: 드라이브 경로 속성 읽기 `Win32 87 / ERR_REA_WINDOWS_NATIVE`로 실패했다. 설치된 CLI의 읽기 전용 `doctor` 결과에는 MCP/CLI 버전 차이와 CLI 셸의 Ghidra 환경변수 누락도 있었다. 설정이나 설치를 변경하지 않고 세션을 닫았다. 위 근거는 기존 Ghidra 산출물 및 독립 Unicorn 실행이며, 성공한 REA 분석 Evidence로 표현하지 않는다.

CD 10.72의 대응 함수/가상 소멸은 이번에 관측하지 않았다. 기존 대응표의 유사 함수 주소만으로 이 동작을 CD판에도 같다고 판정하지 않는다.

## 제품 연결

`RawDataManagerLifetime`이 manager head와60BYTE 노드를 직접 처리한다. `CampaignTemplateStorage::Image`는 파싱한 각 원시 섹션 벡터에 대응하는 실제 노드 객체를 소유한다. 반납 직전에는 현재 벡터의 버퍼 할당을 다시 연결해 설치 뒤 State 추가/확장, 빈 버퍼의 reserve와 호스트 섹션 축소를 처리한다. 원본과 같은 순서로 실제 벡터 할당을 반납한 뒤 실제 노드 객체를 해제하고, 마지막으로 기존 Template 헤더 수명 코드가 manager 객체를 반납한다. 빌린 헤더를 지우는 경로는 이 소멸을 호출하지 않는다.

호스트 노드는 원본 포인터를DWORD로 자른 값이 아니라 비영 핸들을 사용한다. 파일 순서의 노드 구성과 현재 벡터 연결은 호스트 어댑터다. 원본 DataManager의 전체 생성/파일 읽기/검색/current cursor/저장/오류 처리와 scalar 가상 소멸을 구현했다는 뜻은 아니다. 소멸의 버퍼/노드 순서만 같은 실제 소유 자료에 적용한다.

새 검사3개를 작성·컴파일했다. 독립392행의 전체 메모리/모든 free 직전 상태를 비교하는 검사, 물리 범위와 빈 저장소 검사, 실제 저장소의 나중 State 할당/확장·빌림·빈 할당·섹션 축소 반납 검사다. **C++검사/CTest/게임/전체 회귀는 기존 지시대로 실행하지 않았다.**

## 검증과 보존

최종 `cmake --build cpppj/build --config Release --parallel`은 **종료0·경고0·오류0**이다. 라이브러리·게임·테스트 실행 파일을 빌드했다. 최종 로그는 `extracted/cpp-datamanager-lifetime-20261021-build2.log`, 현재 소스/근거/바이너리의 전체 SHA는 `cpppj/datamanager-lifetime-session.json`에 있다.

이번 정적 감사와 최초392행 보존, 기존 Template v2 원문 보존의3개 명령은 종료0이다. 현재 시작 소스 SHA와 일치하는 **107보고서·138핀**만 새 소스로 갱신하고 가역 이력을 추가했다. 이 이력을 되돌린 전체 JSON이 시작 커밋과 같은 것을 확인했다. 시작 소스와 이미 다른 과거 시점 핀105개와 기존 원문/기대값/실행 결과/바이너리 해시는 변경하지 않았다.

기존 감사3건은 새 통과로 세지 않는다.

- `cpp_mission_fort_request_audit.py`: 변경하지 않은 `RawLocalMissionState.cpp`의 작업 트리 CRLF SHA가 과거 LF SHA와 다르다. LF로 정규화한 SHA와 시작 커밋의 SHA는 과거 기대값과 같아 소스 내용 차이가 아니다. 그 파일의 줄바꿈과 기대 SHA를 바꾸지 않았다.
- `cpp_tutorial_init_audit.py`: 다른 PC에서 만든 Git 제외 `extracted/tutorial-init-20261021-first-capture/decomp_tutorial_init.py`가 현재 PC에 없다.
- `cpp_mission_lifecycle_audit.py`: 과거 Git 제외 `extracted/cpp-mission-lifecycle-20261021-build.log`가 현재 PC에 없다.

이전 PC 자료를 이번 결과로 다시 생성하거나 과거 실행 성공으로 덮어쓰지 않았다. 이번 신규 감사는 이번 최초 자료와 빌드 로그를 직접 확인한다. 테스트 실행 성공이나 전체 플레이 동일성은 주장하지 않는다.

인수인계 뒤 **20261021-24 CP** 형식으로 로컬 커밋한다. push하지 않는다. 다음 작업은 DataManager의 생성/파일·MemStream 연결/저장·오류 처리 또는 전체 State7/전투 정리의 실제 소비자 연결이다. Fort 선택/Guide 공장·가상 소멸, 전체11미션 자연 완주·전략AI·외관/음향/동작 동일성은 미완료다. 마지막 정밀 디컴파일 PC는 계속 **VM-W11-CODEX / 2026-10-21**이다.
