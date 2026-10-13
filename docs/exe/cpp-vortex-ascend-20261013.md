# 신전 Ascend 정리 틱과 후보 판정 복원

2026-10-13, 현재 호스트와 마지막 디컴파일 PC는 **HJOW-Athlon**이다. 작업 전 AGENTS.md와 두 인수인계를 읽었고, 작업 중 추가된 **CD10.72 주소와 판본 차이도 문서에 기록**하는 지침을 다시 확인했다. 기존 `RawAssetRegular` 완료 파일은 동결했고 이번 단위는 독립 `RawVortexAscend`에 작성했다.

## 실제 주소와 동작

| 몸체 | 10.78 | CD10.72 | 역할 |
| --- | --- | --- | --- |
| Ascend 정리 틱 | `004db8e0` | `0044ee30` | 반경finder→최대20개 후보→공유 난수 선택→가상 삭제 `0x100000`→free→지속 여부 |
| 후보 판정 | `004db680` | `0044e230` | 같은 섬·genus/spot/표면/flags1·신전과 다른 소유자 |
| 다른 소유자 섬 개수 | `0046f480` | `004bdba0` | 존재하는 섬 중 nonzero owner이며 요청 owner와 다른 섬의 수 |
| 섬 owner getter | `0046f320` | `004bda70` | 현재 존재/owner DWORD 읽기 |
| 섬 존재/범위 | 패치 인라인 | `004bd4c0`→`004bd480` | CD는 별도 helper로 현재 존재 DWORD를 조회한다 |

신전 event0은 Ascend 정리 틱을 호출한다. signedDWORD count가32를 넘으면64로 바꾸고 신전 좌표에 원본 float **0.99999f**를 더한 뒤 정수로 절삭해 정사각형 반경을 만든다. 동일한 실제 일반 finder를 사용하며 부모의 현재 owner를 후보마다 다시 읽는다. 첫20개 후보를 모으면 Next를 추가 호출하지 않고, 동일한 `GameRandom`에서 `[0,개수)` 한 번을 소비해 한 자산의 실제 `vtable+10(flags0x100000)` 삭제를 호출한다. 후보가 하나도 없고 최종 반경이64일 때만false이며 `RawAssetRegular`는 이 반환에서−1을 반환해 원본 Regular 상태를 유지한다.

후보는 genus `0x0128b302`와 겹치지 않아야 하고 현재 섬 번호가 신전 섬과 같아야 한다. 원래 좌표의 정수 칸 spot에2가 있으면 실제 hash0 표면 SID가 존재하며 그 타입이 noIsland157이 아닌 경우에만 진행한다. spot2가 없으면 현재 타입 flags1의 `0x200000`을 요구한다. 마지막 ownerBYTE를 신전 ownerDWORD와 비교해 **다른 소유자만** 허용한다. 후보 함수 자체에 별도 free/dead/void/abstract 거부를 넣지 않았으며 일반 finder의 실제 필터가 앞단 조건을 담당한다.

새 모듈은 같은 raw pool/hash/spot/현재 타입 표/게임 난수와 섬 존재·owner 목록을 참조한다. `RawAssetRegular`의 `ascendTick`와 `countForeignOwnedIslands`에 연결할 수 있다. 섬 좌표 조회, 원본 new80/free, 실제 파생 삭제는 필수 외부 효과이며 미연결이면 생성 단계에서 거부한다.

new가 실패했지만 후보가 없으면 원본도NULL free 후 정상 반환한다. 후보가 생겨NULL을 쓰려는 원본 경로는 충돌이므로 정상 입력으로 계수하지 않으며 C++은`bad_alloc`으로 진단한다. 성공으로 바꾸거나 삭제를 생략하지 않는다. 검사에서는 원본의 할당 실패/빈 후보 반환도 별도로 대조한다.

## 판본 차이

패치는 독립 정수 반경finder 생성자`00422230`→좌표절삭`0041d7a0`→일반 Begin/Next를 사용한다. CD는 같은 생성 흐름을 Ascend 몸체 안에 인라인하며`00440a80`과 `004eae20/004eafe0`을 호출한다. 후보의 정수 표면 조회도 패치는 `0040da00`, CD는 범위/표면WORD 읽기가 인라인이다. 기본 판정·최대20개·공유 게임 난수와 최종 빈 반경의 반환은 같다.

CD의 후보 버퍼 free는 몸체의`0044efc2` 조각→벡터 소멸자`00407c50`→`00407c91`→`00407c40`을 실제 실행하고 **CRTfree`004f1a50`**에서 관찰한다. `00407c50`에 전달되는 ECX는 벡터 스택 객체이며 CRT 포인터가 아니다. 패치는`004e4354`에서 관찰한다. 섬 개수 조회도 CD의 실제 Exists/owner helper를 실행했고, 좁은 내보내기에 없는`004bd480`과 세 소멸자 helper는 현 호스트의 전체 정밀 functions.tsv 범위로 실행/출처SHA를 고정한다.

## 독립 원본 근거와 현재 검사 단계

최종 원본 고유 입력은 **1496개(패치748/CD748)**이며 각 판본은 **Candidate435/Tick293/Count20**이다. 두 x87정밀도의 정상 반환은2992회, assert0, OS/게임/창 진입0이다. 후보 조건 격자·콜백 뒤 owner/flags1/spot 변경·count0/32/33/64/음수·후보0/1/19/20/21/40·공유 난수·allocation 뒤 owner/abstract 변경·new실패/빈 후보·섬 존재/owner 기록을 포함한다. 원본 반환·난수·외부 효과 순서·40개 후보 raw 상태·현재 부모 owner를 독립 fixture에 보존한다.

처음 분석용 spot 주소가 finder의1단계 해시 영역과 겹침을 발견해, 현재 실행기는 spot을 **별도 주소0x11400000**에 매핑하고 원본 전역을 그 입력 지도에 연결한다. 교정 전 부분 실행은 `extracted/vortexascend-oracle-generate.log`로 보존하고 최종 관찰에 합산하지 않는다. 새 지도의 두 판본 smoke13개씩과1496개 첫 생성은 `extracted/vortexascend-oracle-final.log`에 기록했다.

첫 C++ 대조에서 반경0/빈 후보 입력은 신전 좌표50.75/52.5에 원본 편향을 더한51/53을 요구했다. C++의 단순 절삭50/52를 교정한 뒤 패치748개 전수재생이 통과했다. CD의 이전free 관찰F0는 위 벡터 소멸자에서 실제free까지 실행하여F1로 교정했고, 해당 판본만748개를 두 x87 정밀도에서 다시 관찰했다. 패치748개는 이전 행을 그대로 보존하며 추가 원본 입력 수로 합산하지 않는다. 최종1496개 current SHA/고유입력/두정밀도 정상반환과 재사용 출처 감사가 `--link-reuse`→`--verify`에서 통과했다.

선택 재생성의 `native_reuse.originals`는 이전 전체 보고서/fixture SHA, 패치748개 행 SHA, 이전 실행기 SHA, 현재 실행기 SHA와 보호PE·패치 함수 범위·부모 실행기 SHA를 함께 기록한다. `previous_sources`는 이전 전체 출처 표를 남기고 `preserved_sources`는 이번에도 동일함을 확인한 패치 근거다. 실제 native의 판본 교정은 **CDfree 경계와 CD helper 허용 범위**이며 선택 생성·출처 연결·재감사는 기록 워크플로 변경이다. 따라서 현재 도구 SHA로 이전 패치 관찰의 실행기 출처를 덮어쓰지 않는다. 교정 전 전체 자료는 `extracted/vortexascend-before-cd-repair-{evidence.json,x86.tsv}`, 신규 CD 관찰은 `extracted/vortexascend-oracle-cd-repaired.log`에 보존한다.

자료는 `tools/decomp_vortexascend_oracle.py`, `tools/ghidra/vortexascend-functions.json`, `cpppj/vortexascend.json`, `cpppj/tests/fixtures/vortexascend-x86.tsv`에 있다. 최신 공통 Release에서 **VortexAscend_3 tests와 패치748/CD748 전수재생1496개가 모두 통과**했으며 CDfree의F1도 대조했다. 실제 GUI/생산/피해·전체 Ascend 송신/종료 또는 캠페인 완주를 검증한 보고서는 아니다.
