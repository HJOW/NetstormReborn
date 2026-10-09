# 일반 사제 배치의 다리·섬·지역 효과 복원

2026-10-09. **마지막 디컴파일 수행 PC: `HJOW-Athlon`**. [`priestterrain-functions.json`](../../tools/ghidra/priestterrain-functions.json)의 패치/CD/추가 10.37 목록 **7/8/8개**를 같은 PC에서 읽기 전용으로 내보냈다. 원본 게임·복사본·클론 창과 OS 호출은 실행하지 않았다.

[`RawPriestPlacementTerrain`](../../cpppj/src/o/RawPriestPlacementTerrain.h)은 실제 후보 충돌 이후의 다리/섬 처리, 모양별 지역 배열, 일반 사제의 마지막 판정을 복원한다. 앞 단계의 비패턴 decoder·SHP 조회·미리보기·finder·배치 접두·나선 생성과 연결한다. **일반 사제 지형 효과의 외부 경계가 실제 구현으로 바뀌었으며, 일반 사제 Pop과 GUI 월드 연결은 후속 작업이다.**

| 처리 | 10.78 | CD/추가 10.37 |
|---|---|---|
| 배치 몸체 | `0049b510` | `00445200` |
| 후보 지형 시작 | `0049bb1a` | `004457f7` |
| finder Next 호출 직전 | `0049bca5` | `00445966` |
| 모양 종료의 지역 동일성 검사 | `0049bcb9..0049bd1d` | `0044597e..004459d4` |
| 일반 사제 최종 판정 구간 | `0049bde0..0049bfc3` | `00445a9c..00445ce7` |
| 좌표별 지역 조회 | `0046f4e0` | `004bdbf0` |
| raw 객체 지역 getter | `004acda0` | `004acaa0` |

## 순서와 자료 수명

`Begin`은 decoder 생성 후 bridge=false, noIslandOnly=true, groundComplete=true, permission=true를 설정한다. 일반 사제의 genus `0x200000`은 원본 permission 초기화 조건 `0x10202000`에 포함되며 bridge 비트가 없으므로, 후보 권한 helper와 모양 종료 뒤의 주변 관계 탐색은 실행하지 않는다. 이를 허용 여부를 돌려주는 임시 훅으로 대체하지 않았다.

새 선택 훅 `PriestPlacementGeometryHooks::beginShape`를 finder 탐색 직전에 호출한다. `BeginShape`은 144바이트 배열을 현재 sentinel DWORD의 하위 BYTE로 채우고, 절삭한 모양 좌표에서 현재 발자국을 빼고 1을 더한 원점과 `flags1 & 6`을 캡처한다. 유효 프레임이 없는 decoder는 이 초기화를 실행하지 않으므로 **이전 지역 배열이 유지된다**.

`MakePriestTerrainGeometryHooks`는 순회 초기화/모양 시작/종료/최종 처리를 연결한다. `MakePriestTerrainCollisionHooks`는 후보 지형 처리만 교체한다. 후보 충돌이 거부되면 지형 효과·finder Next·모양 종료·최종 처리에 도달하지 않는다. 충돌을 무시한 후보는 지형 효과를 받는다. 같은 SID 풀을 사용하는 어댑터만 연결하며, 자료 참조와 상태 수명은 호출자가 보장한다.

## 다리와 섬

후보 genus의 bridge 비트 `4`, 배치 genus의 `0x208100`, 후보 extra bit 0이 모두 맞으면 bridgeOverlap=true다. 원본 assert 문구는 `footX == footY == 1`이지만 실제 명령은 **폭과 높이가 같은지만 검사**한다. 따라서 2×2도 통과한다. C++은 실제 비교 조건을 보존한다.

후보 genus의 island 비트 `2`가 있으면 현재 타입 번호가 noIsland 전역과 다른 경우 noIslandOnly=false로 누적한다. 이 갱신은 지면 종류가 맞지 않아도 실행된다. 후보의 현재 프레임은 패치 raw **+36의 signed DWORD**, CD raw **+34의 unsigned BYTE**이며, 프레임 코드의 flags bit 2가 있으면 지면 `4`, 없으면 지면 `2`다. 캡처한 허용 지면과 교집합이 있을 때만 후보 좌표를 CRT 방식으로 절삭해 지역 번호를 조회한다. 올림 보정은 없다.

지역 SID 지도는 256×256의 별도 입력이다. 범위 밖 좌표 또는 SID 0은 **127**을 반환한다. 실제 SID는 해당 슬롯의 타입 genus를 읽으며 bridge는 현재 sentinel을, island/island3x3은 raw **+8의 unsigned WORD**를 반환한다. 패치는 지원 genus가 없으면 assert하고, CD는 sentinel을 반환한다. C++ 패치 진단은 예외로 표현한다. 이 지도와 미리보기의 표면 SID 지도는 호출자가 별도로 제공한다.

후보 발자국의 오른쪽 아래는 절삭 좌표이며 왼쪽 위는 좌표−발자국+1이다. 모양 원점을 빼고 x 바깥/y 안쪽 순서로 지역 하위 BYTE를 쓴다. **원본 배열은 열 간격 12, 후보 쓰기 제한은 8×8**이다. C++은 배열 밖에 효과가 없는 반복을 생략하고 동일한 교집합을 방문한다. 지역 번호가 WORD 범위라도 배열에는 하위 BYTE만 저장한다.

## 모양 종료와 최종 사제 분기

첫 지역 BYTE를 signed로 확장해 현재 sentinel DWORD와 비교한다. 예를 들어 BYTE 255는 DWORD `0xffffffff`와 같고 DWORD 255와는 다르다. 빈 지역이면 groundComplete=false다. 그 외에는 현재 배치 발자국 전체를 열 간격 12로 검사하며, 첫 지역과 하나라도 다르면 false다. 앞 모양에서 false가 된 값은 뒤 모양에서 되돌리지 않는다.

최종 canPlaceGround는 `(flags1 & 0x400 && noIslandOnly) || groundComplete || bridgeOverlap`이다. `flags1 & 0x400`이고 noIslandOnly와 groundComplete가 모두 false이면 permission=false로 갱신한다. 그 결과와 관계없이 **일반 사제의 마지막 genus 분기는 true를 반환**한다. 앞선 후보 충돌 거부를 허용으로 바꾸지는 않는다.

타입 계약은 genus `0x200000`이 있고 bridge 비트가 없는 일반 사제다. 패치의 genus `0x200` 조합은 별도 특수 지역 helper가 필요하므로 미지원으로 진단한다. CD는 그 helper 분기가 없으므로 지원한다. 배치 발자국은 1..12로 제한하여 원본 배열 밖의 손상된 접근을 예외로 진단한다. 패턴 모양과 일반 자산의 권한/관계 탐색은 기존의 후속 범위다.

## 독립 원본 대조와 연결 검사

[`decomp_priestterrain_oracle.py`](../../tools/decomp_priestterrain_oracle.py)는 각 PE **2,240개**, 총 **6,720개**를 실행한다. 판본별 후보 **1,920개**, 모양 종료 **144개**, 좌표별 지역 조회 **128개**, 최종 판정 **48개**다. 두 FPCW **0x027f/0x037f**에서 전체 지역 배열·raw·다리/noIsland/ground/permission/canPlaceGround·반환 비트의 관찰이 같다.

후보와 모양 종료/최종 판정은 **MayPlace 중간 구간에 지역 변수와 레지스터를 공급하는 실행**이다. 후보는 클라이언트 SID 무시를 통과한 뒤의 실제 지형 효과를 실행하며 finder Next 호출 직전에 종료한다. 모양 종료는 주변 관계 검사 전에 종료하고, 최종 구간은 실제 사제 우회와 반환값 1 설정 뒤에 종료한다. 전체 MayPlace ABI나 finder/decoder의 원본 몸체 실행을 증명하는 fixture가 아니다. 좌표별 지역 조회만 실제 thiscall 정상 반환/스택/보존 레지스터를 검증한다. 모든 구간의 허용 코드/쓰기와 x87 균형, assert/OS **0회**, 입력/PE/내보내기/도구 SHA를 감사한다.

각 PE·두 정밀도 합산 실제 후보 **3,840회**, 모양 종료 **288회**, 최종 판정 **96회**, 지역 조회 **1,216회**, 지역 getter **496회**, CRT **1,920회**다. patch 타입 getter `0049a840` **4,336회**, CD/10.37 타입 번호 getter 각 **1,920회**다. finder Next 몸체 실행은 **0회**다.

별도 C++ 연결 검사는 실제 geometry/finder가 찾은 섬의 지역 9 쓰기, 허용 지면 불일치, permission 갱신과 사제 우회, 다리/extra 비트, 빈 decoder의 이전 배열 유지, 누적 false를 확인한다. 기존 사제 생성 통합에서도 지형 훅을 이 구현으로 교체했다. 첫 **21,22** 충돌 거부→**21,21** 생성, SID **15001/6001**, WORD **0x8101**, 미리보기 **16칸**을 유지한다. 지역 지도에 섬이 없어서 groundComplete=false여도 원본 사제 우회로 생성된다.

Release 경고/오류 **0**, CTest 내부 **353개·실패 0**(99.35초), 감사 **49종 모두 통과**, 누적 독립 x86 **251,865개**다. 최신 전체 회귀·감사 로그는 [LEFT_JOBS.md](../../LEFT_JOBS.md)를 따른다. 기존 fixture와 감사 도구는 유지한다. 다음은 **타입 기준점/실제 프레임 메타 자산 연결, 특수 패턴/타입 조합, 일반 사제 Pop·보호막/회복 예약·Carrier 검사**이며 비표면 raw 월드/GUI·건설/경제/전투/승패는 계속 후속이다.
