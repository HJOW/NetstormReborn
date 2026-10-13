# cpppj UI·Kernel 정리와 Gump 루트 교체

작업 PC **VM-W11-Codex**, 기록일 **2026-10-13**. AGENTS.md와 두 LEFT_JOBS를 먼저 읽었고 최신 정밀 디컴파일 수행 PC **VM-W11-CODEX**와 현재 PC가 일치했다. 원본 10.78/CD 10.72 PE와 현재 정밀 프로젝트를 읽기만 했다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-13.** DirectDraw 전체화면 오류는 이번 범위에서 제외한다.

## State 시작·미션 전환의 정리 순서

원본 State의 시작/초기화 함수는 **10.78 004b2df0 ↔ CD 0042f5c0**다. 상위 State 전환 **004b6dd0 ↔ CD 0042d630**의 요새/전투/인트로/미션 전환 분기에서 같은 정리를 호출한다. 플레이어별 기술 목록·커서·UI·프로세스는 아직 살아 있는 Squid 풀을 참조하므로 이 함수의 순서는 풀 메모리를 먼저 비우는 초기화와 다르다.

| 순서 | 10.78 | CD 10.72 | 직접 확인한 효과 |
| --- | --- | --- | --- |
| 앞 단계 | 004d5c60 | 0041ed70 | 현재 커서 선택이 있을 때 해당 커서 상태 해제 helper/선택 관련 전역/후속 helper 호출 |
| 1 | 0048fcd0 | 00406e00 | 플레이어별 동적 필드 초기화와 포인터 해제. 10.78은 9개 0xb4바이트 구조의 +9c/+8을 0, +68 포인터를 free 뒤 0 |
| 2 | 00471af0 | 004ed4d0 | Kernel의 현재 각 슬롯을 가상 scalar 소멸자(1)로 정리한 뒤 0. 패치 1~39999/CD 1~3999 |
| 3 | 004666d0 | 00492a60 | 등록표 1~999 전체에서 제외 타입 이외·비 dead Gump만 실제 Kill |
| 4 | 004662b0 | 004929b0 | 현재 삭제 큐를 실제 Cleanup하여 등록 해제·가상 소멸자 실행 |
| 5 | 004c77c0 | 004cd000 | 새 UberGump 생성·임시 등록칸 제거·이전 루트의 살아 있는 직계 자식 이동·루트 교체 |
| 뒤 단계 | 0048f3b0(0,1) | 00405f40(0,1) | 현재 플레이어/뷰 참조 해제 |
| 풀 초기화 | 004af170(0) | 004aaa70(0) | 기존 크기의 Squid 풀 초기화·추가 공통 표 초기화. 앞선 모든 수명 정리 뒤 실행 |

앞/뒤 단계 사이에는 표시·저장·상태 전역·맵/그래프/해시/명령 등의 다른 원본 호출이 있다. 위 표는 실제 순서에서 UI/Kernel/풀 수명과 직접 관계된 호출만 발췌한 것이며 State 함수 전체 구현이나 전체 월드 복원 완료의 주장이 아니다. `004af170`은 `004abb10`의 풀 초기화를 호출하고, `004952d0`은 두 개의 40 DWORD 표를 지운다. `004c2450/CD 0040ba90`의 세 개 255 DWORD 표 정리도 풀 초기화 뒤다.

초기 Client WinMain **00438dc0**은 `00471930` Kernel 표 초기화 → 타입 → `004af170(120000)` 첫 Squid 풀 생성 → 해시/spot → **004663d0** 기본 Gump 루트 초기화 → 영상 → **004c77c0** UberGump 루트 교체 순서다. `InitializeRoot`는 등록표만 비우고 기본 루트를 만드는 시작 단계이며 이전 객체의 소멸을 대신하지 않는다. 미션 전환마다 이 함수를 반복 호출해 count/serial을 새 값으로 만드는 구현은 실제 State의 호출 경로와 다르다.

## 복원 API와 정확한 필터

`GumpLifetime::KillExceptType(const std::int8_t& excludedType)`는 **004666d0/CD 00492a60**를 복원한다. 등록표 +0의 루트는 생략하고 +1~+999를 현재 값으로 읽는다. count·visible·void·미등록 비트로 순회나 호출을 생략하지 않는다. 각 비영 칸에서 현재 `+0x20` DWORD와 현재 signed 제외 BYTE를 비교하고, 타입이 다르면서 dead **0x10000**이 없는 노드에 실제 `Kill`을 호출한다. 자신의 자식은 기존 Kill의 자식 우선 처리로 삭제하므로 제외 타입인 손자라도 부모가 삭제되는 경우에는 살아남지 않는다.

실제 PE의 **DAT_00540426/CD DAT_00535ed4 초기값은 11**이다. 이 필드는 Gump 타입 식별자다. 현재 C++에서 대응하는 필드 이름은 `GumpCreateNode.command`이며, 파생 상태의 color/alignment 이름과 관계없이 이 reset 비교에서는 타입 번호로 취급한다. 원본 Attach의 진단도 이 위치를 `gt`로 읽는다. 서브클래스의 확정된 이름 없이 타입 11의 역할을 다른 UI 종류로 추정하지 않는다. **UberGump 타입은 DAT_00540451/CD 생성자의 해당 타입 BYTE, 패치 초기값 54**다.

`excludedType`을 const reference로 받아 가상 Push/Form 등의 콜백이 같은 BYTE를 바꾸었으면 뒤 슬롯에서 바뀐 값을 읽는다. 등록표의 이미 방문한 칸을 바꾸어도 되돌아가 재방문하지 않는다. 미래 칸 변경·지워진 칸·중복 토큰·앞선 재귀 Kill로 dead가 된 뒤 칸의 필터를 그대로 유지한다.

`GumpLifetime::ReplaceRoot(token)`는 **004666a0/CD 00492a20**를 복원한다. 이전 등록표 +0이 비영이면 그 토큰에 실제 Kill → 이전 루트 유무와 무관한 현재 Cleanup → 등록표 +0/수집 루트에 새 토큰 저장 → 새 노드 `id(+1c)=0` 순서다. count/serial을 초기화하거나 새 노드의 기존 등록칸을 제거하지 않는다. Cleanup 소멸자의 현재 큐 증가/변경도 기존 수명 몸체에서 처리한다. 잘못된 토큰은 호스트 계약의 예외이며 원본 assert/접근 위반 동작을 실행하지 않는다.

## UberGump의 상위 루트 교체

**004c77c0/CD 004cd000**은 새 Uber를 생성한 다음, 그 생성자가 일단 배정받은 등록칸을 0으로 비우고 새 `id=0`을 쓴다. count를 대신 감소시키지 않는다. 그 뒤 현재 이전 루트 `child(+18)` 체인을 다음 순서로 옮긴다.

1. 각 자식의 현재 `next(+0c)`를 가상 호출 **전에** 저장한다.
2. 자식의 가상 **+18 Push**를 호출한다.
3. Push 뒤 현재 자식의 **x(+4c)/y(+50)**를 읽어 가상 **+20 Attach(x,y,newRoot,0)**를 호출한다.
4. 선저장한 next로 이동한다. 끝나면 실제 `ReplaceRoot`를 호출한다.

새 UberGump 생성 **004c7160/CD 004cd1a0**은 기본 생성·타입·idle/표시 상태와 패널/스크롤/지도/생산/채팅/배경 같은 실제 파생 자식을 생성한다. 10.78의 객체 크기는 **0xc8**, CD는 **0xc0**다. 생성 도중에도 여러 차례 Cleanup과 자식의 가상 resize를 호출한다. 이 파생 생성자는 이번 두 helper 구현에 포함하지 않는다.

대조적으로 **004c7880**은 이전 루트를 Kill/Cleanup한 다음 새 Uber를 생성하므로 기존 자식 이동을 하지 않는다. 두 교체 경로를 같은 무조건 전체 삭제 함수로 합치면 State/해상도 등의 전환에서 보존되는 자식 수명이 달라질 수 있다.

## 실제 Client 연결의 직접 위치

검토 착수 당시 `Client::Run`은 멤버 Kernel을 만든 뒤 `UberGump`를 직접 생성하고, `UberGump::Compose`는 `GumpControlInput`과 현재 Renderer 장면/비트맵에 돌 버튼·목록·문구를 직접 제출했다. 이 경계가 실제 원본 raw UI로 전환할 직접적인 위치다. 같은 작업 단계의 별도 담당자가 `RawButtonControls`로 돌 버튼의 공유 GumpCreate/Tree/Input/Frame/Lifetime 저장소를 실제 Uber에 연결한다. 메뉴 행·전체 파생 UI·월드 Form까지 포함하는 자동 UI 전체 호스트는 후속이다. 생성기·트리·입력·버튼·수명은 같은 안정된 노드/등록표/표시 저장소를 사용해야 한다. Frame 순서는 실제 원본처럼 **Cleanup → idle → Kernel 실행 → Draw**를 보존해야 한다.

`UberGump::ClearWorld`는 현재 번개를 지우고 `UserInput` 참조를 해제한 뒤 Kernel의 `GameWorld` 프로세스를 지운다. `GameWorld`의 표면만 `RawSurfaceWorld` 풀/Graph/Kernel을 사용하며 비표면 건물/사제/이동은 기존 어댑터다. `RawSurfaceWorld::Impl`은 Kernel을 마지막 멤버로 선언하여 파생 host/분배기/pool보다 먼저 소멸하지만, 풀/Graph/UI를 원본 State 순서로 반복 초기화하는 명시 전체 reset을 대신하지 않는다. UI Form과 건물/사제 같은 비표면 SID를 하나의 실제 풀에서 등록·정리하는 후속 연결이 필요하다.

이번 Gump helper 두 개는 원본 상위 호출자의 수명 정리 계약을 실제 처리기로 연결할 준비를 완료한다. Kernel/SquidProcess의 별도 복원과 현재 RawButtonControls 호스트 연결은 병렬 작업이며 본 문서의 완료 범위에 합산하지 않는다. **캠페인 1-1 건설/경제/전투/승리와 원본 동일성은 아직 완료되지 않았다.**

## 독립 관찰과 검증

고유 정밀 내보내기 `tools/ghidra/uireset-functions.json`을 두 읽기 전용 프로젝트에서 수행했다. 최종 10.78 **23개**, CD **22개** 함수 성공. 출력은 `extracted/uireset/<판본>/creation.c`, `functions.tsv`다. 앞선 목록의 잘못된 후보 주소 `004c2080`은 실제 함수 정의가 없어 제거하고 최종 목록을 다시 내보냈다. 최초 실패 로그와 최종 성공 로그를 보존했다.

`tools/decomp_uireset_oracle.py`는 두 reset의 **실제 PE 전체 몸체**와 실제 Kill/Cleanup/Destroy/기본 scalar/CRT 겹침 복사 하위 몸체를 실행한다. 가상 Push/파생 소멸자/Form/free만 명시 응답 경계이며 C++ 구현이나 기대 필터/삭제 결과를 읽거나 호출하지 않는다. State/Uber 전체/allocator/OS/게임은 실행하지 않는다.

판본당 **121개**, 합계 **242개** 새 독립 원본 입력과 두 x87 모드의 **최상위 정상 반환 496회**를 관찰했고 두 모드 출력이 일치했다. 여섯 전체 0xa0바이트 노드·등록 1000/idle 500 또는 80/overlay 100/kill 1000/dialog 16칸·아홉 scalar(count/큐/dialog/focus/override/current/진단/serial/제외 BYTE)·각 가상 경계 당시 전체 상태·인자를 저장한다. ABI/스택/보존 레지스터/x87/SEH/불연속 실행·쓰기/읽기 전용 pool/가상 표/노드 사이·표 인접 보호바이트·signed BYTE 인접 바이트를 감사했다. OS/assert 호출은 0이다.

원본 루트의 자식이 연결되었는데 void인 모순된 초기 입력 한 건은 실제 하위 Cleanup에서 assert 경로에 도달해 입력을 정상 부착/Push 분리 응답으로 수정했다. 이 실패를 성공 관찰로 계산하지 않는다. `--verify`는 최종 SHA/전체 완성 입력/행·노드·표 폭/실제 진입·반환·경계 합계를 통과했다. 로그: `extracted/uireset-oracle-{smoke,debug,final}.log`, 최종 정밀 출력 `uireset-export-{originals,cd}-validated.log`.

`GumpResetTests.cpp`의 native 두 검사는 fixture의 전체 상태/콜백을 C++와 비교한다. 실제 생성·트리·기본 소멸자 통합 두 검사는 제외 타입의 직접 자식 보존/삭제되는 부모의 제외 타입 손자 삭제·새 Uber 임시 등록칸 제거·현재 자식 Push/Attach·루트 교체·count/serial 유지·등록칸 재사용을 확인한다. 상위 담당자의 최종 Release 빌드는 종료 0·경고/오류 0이다. 그 최종 검사 실행 파일로 새 **GumpReset 4개·실패 0**, 기존 **GumpLifetime 9개·실패 0**, **GumpInput 9개·실패 0**을 직접 실행했다. 새 C++ 대조는 두 판본 **242개 전부 일치**다. 독립 `--verify`도 최종 **242개/496회 정상 반환**으로 통과했고 구현/검사/도구/fixture/evidence/문서의 고유 manifest SHA 불일치는 0이다. 로그는 `extracted/uireset-focused-final.log`, `uireset-lifetime-regression-final.log`, `uireset-input-regression-final.log`, `uireset-verify-final.log`다. 전체 CTest·Client 실제 GUI/픽셀 검사는 상위 담당자가 별도로 기록한다.

새 코드/도구/문서/fixture는 UTF-8이며 설명 주석은 한국어다. AGENTS.md·보호 원본·dotnetpj·공용 CMake/LEFT/README/SOURCE_MAP을 본 담당자가 수정하지 않았다. 원본 게임/GUI 실행·커밋/푸시는 없다.

별도 담당자의 실제 버튼 배경 픽셀 대조에 필요한 `extracted/gumpbackground/originals/functions.tsv`가 현재 PC에서 빠져 있어 기존 `tools/ghidra/gumpbackground-functions.json`의 10.78 함수 **00439de0/0043c010/00465880/00465690/00445290, 5개**를 현재 정밀 프로젝트에서 읽기 전용으로 복구했다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-13.** 결과는 `extracted/gumpbackground/originals/`, 로그는 `extracted/gumpbackground-export-vmw11-20261013.log`다. 기존 목록의 과거 PC 설명·C++ 소스·기존 근거 manifest를 바꾸지 않았으며 별도 새 manifest도 만들지 않았다. 이 재출력만으로 새로운 독립 원본 관찰 수를 더하지 않는다. 실제 GUI 픽셀 스모크의 실행·결과 기록은 상위 담당자가 수행한다.
