# Sky Overview 미니맵 dirty 큐 복원

마지막 디컴파일 수행 PC는 **VM-W11-CODEX, 2026-10-13**이다. `AGENTS.md`와 두 `LEFT_JOBS`를 읽고 현재 호스트가 일치함을 확인했다. 원본 실행·GUI·보호 파일 편집 없이 읽기 전용 정밀 Ghidra와 격리 x86 실행을 사용했다.

`RawMetadisplayDirty`는 전투 시뮬레이션이 아니라 `minimapGump`/`Sky Overview`의 갱신 목록이다. Pop/Unpop의 비전투 null 표시 입력만 실행했던 기존 근거와 이번 활성 표시 큐 근거를 구별한다.

| 역할 | 10.78 패치 | 10.72 CD |
| --- | --- | --- |
| 고유 셀 등록 | `00481510` | `00442f30` |
| 화면별 처리 | `00481320` | `00442d10` |
| 실제 float 절삭 | `004e49c0` | `004f161c` |
| 표시 재생성 경계 | `00480270` | `004420d0` |
| 셀 그림 경계 | `004808d0` | `00442680` |
| 큐 count/전체 갱신 | `005949c0`/`005949d4` | `0051c540`/`0051c550` |
| 고정 큐 배열 | `005949e0` | `005654d0` |

## 복원한 동작

- 표시 객체가 존재하고 `+4 & 1`일 때만 좌표를 변환·등록한다. 좌표는 float→signed32 절삭 후 signed `/4`한다. 음수의 floor나 unsigned shift를 사용하지 않는다.
- `(x,y)`가 같으면 먼저 자연 반환한다. count가 100이어도 기존 항목 재통지는 전체 갱신을 요구하지 않는다. 새 항목일 때만 포화 플래그를 1로 쓰며 count와 배열을 유지한다.
- 등록 순서와 count 밖의 오래된 배열 바이트를 보존한다. 원본 세 번째 DWORD는 미초기화 스택 잔여값이고 셀 그림 몸체가 실질적으로 사용하는 인자는 두 좌표다. 새 C++ 항목은 잔여값을 0으로 정규화한다. 격리 원본 실행에서도 그 스택 영역을 0으로 입력해 비교한다.
- Refresh는 표시가 비활성이어도 최초 128×128 BYTE bitmap을 확보·0으로 채우고 전체 갱신을 1로 만든다. 비활성일 때 큐·전체 갱신·프레임·캐시를 소비하지 않는다.
- 활성일 때 static 초기화 비트를 기록하고 첫 사각형 캐시를 0으로 설정한다. dirty/count·카메라·사각형이 달라지면 현재 view를 먼저 저장하고 실제 표시 재생성 훅을 호출한다.
- **기계어에서 확인한 카메라 비교 차이:** 패치는 `fucompp`의 unordered를 배제하는 float 동등 비교다. CD 디컴파일의 DWORD 비교처럼 보이는 표현은 실제 `fcomp/test ah,40`으로, unordered NaN도 같은 좌표로 간주한다. 두 판본 모두 `+0 == -0`이다. 구현은 실제 분기를 따른다.
- DWORD 프레임을 증가시킨 뒤 현재 full을 다시 읽는다. full이 있으면 y/x 순으로 64×64 셀을 호출하고, 없으면 고유 목록 순서로 호출한다. 부분 처리 callback 뒤 현재 count를 재조회하므로 새 등록 항목도 같은 Refresh에서 처리한다. 마지막에는 count와 full만 0으로 만든다.
- 직접 로딩/전투 분기는 없다. 원본 메인 화면 처리의 미니맵 순서를 임의 시뮬레이션 tick으로 대체하지 않는다.

## 실제 월드 연결 계약

`MakeMetadisplayDirtyHook`는 다른 SID 풀의 연결을 거부하며 반환 훅은 현재 상태의 큐로 좌표를 전달한다. `Enqueue(Sid)`도 현재 raw 슬롯의 `+14/+18`을 읽는다. 표면 여부와 매몰 검사는 원래 Pop/Unpop 호출자가 수행해야 한다.

원본 Pop은 좌표 지정 후 `flags2 & 6`이고 매몰 `extra & 8`이 없을 때 spot 쓰기 전에 통지한다. Unpop은 void 상태 쓰기 직후 같은 조건에서 통지한다. 모듈 자체는 표면/전투/로딩 조건을 새로 추가하지 않는다.

bitmap의 실제 셀 그림과 표시 재생성은 아직 별도 복원 경계다. 활성 분기에 필요한 `rebuild`/`redrawCell`이 연결되지 않으면 예외로 거부한다. 미션 runtime에 fake/noop 훅을 설치한 상태를 완성으로 세지 않는다. root가 공통 `SquidPop`/`SquidUnpop`에 같은 풀의 `SetMetadisplayDirty`를 연결했고 별도 통합 검사는 실제 공간 몸체를 사용한다. `RawSurfaceWorld`와 실제 HUD 그림 연결은 후속 작업이다.

## 검증 범위

`tools/ghidra/metadisplaydirty-functions.json`의 실제 정밀 내보내기는 패치 5개/CD 3개다. `decomp_metadisplaydirty_oracle.py`는 판본별 **520개**, 합계 **1,040개**의 고유 입력을 두 x87 정밀도에서 실행한다. fixture 행과 누적 고유 비교에는 정밀도 반복을 더하지 않는다.

원본의 queue/refresh·실제 ftol·bitmap zero-fill·현재 view 비교·전체/부분 순서·callback 뒤 현재 count/full 읽기를 실행했다. CRT 확보(성공), 실제 표시 재생성, 실제 셀 그림, CD atexit 등록은 명시 관찰 경계다. 원본 코드/허용 전역/bitmap/stack 밖 쓰기와 몸체 밖 실행은 실패한다. 전체 큐·bitmap·프레임·캐시·효과 호출 순서를 fixture에 저장한다.

단위 파일 `MetadisplayDirtyTests.cpp`는 native 전체 재생, 같은 풀/현재 raw 좌표 alias와 고유 등록, 비활성 bitmap 확보/활성 필수 효과 거부 및 실제 Pop/Unpop 통합의 네 검사를 포함한다. 통합 검사는 patch overlap 앞의 새 좌표/큐 보존과 아직 void/hash 미등록, 등록 성공 뒤 같은 셀 중복, Unpop의 기존 위치, 매몰/flags2 필터, 다른 풀 거부를 두 판본에서 확인한다. root의 CMake 등록·공유 Release 링크 이후 `--filter metadisplayDirty_`를 실행해 **집중 4개/실패 0**과 native **1,040개 C++ 재생 일치**를 확인했다. 로그는 `extracted/metadisplaydirty-focused-final.log`다. 전체 CTest 결과는 root의 단위 작업 기록을 따른다.

근거는 `metadisplaydirty.json`, `cpppj/tests/fixtures/metadisplaydirty-x86.tsv`, `extracted/metadisplaydirty/`와 `extracted/metadisplaydirty-native-*.log`다. 실제 미니맵 그림·GUI/게임·Pop/Unpop 전체 연결·캠페인 완주는 이 좁은 대조로 입증하지 않는다.


## 2026-10-23 실제 HUD 후속 연결

위 초기복원시점에 남겼던 셀그림/표시/카메라/실제HUD 연결은 [실제 Sky Overview 미니맵 연결](cpp-minimap-20261023.md)에서 진행했다. 같은 Session의 Pool/Hash/Pop·Unpop, 공통Gump 수명·입력·정렬 및 Kernel앞Refresh를 사용한다. 당시1040행 dirty 관측은 그대로 두었으며 새셀4722관측/기하1054관측 및 실제GUI의 범위/제외/전체dock·viewport 잔여를 후속문서에서 구별한다.
