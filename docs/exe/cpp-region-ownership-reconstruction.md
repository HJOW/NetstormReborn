# outpost·작업장의 지역 소유 투표

2026-10-09. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-09.** AGENTS.md·두 인계·cpppj 문서를 읽고 호스트 일치를 확인했다. 새 `regionownership` 함수 10/8/8개를 같은 PC에서 읽기 전용으로 내보냈다. 원본 게임/복사본·클론 창은 실행하지 않았다.

[outpost 두 목록 접두](cpp-outpost-lifecycle-reconstruction.md)의 필수 지역 통지 몸체를 [`RawRegionOwnership`](../../cpppj/src/o/RawRegionOwnership.h)으로 복원했다. 추가 outpost 목록과 작업장 목록의 현재 raw 좌표·소유자를 읽어 지역 소유자를 정한다. **투표 결정과 지형 flood/도장 몸체는 서로 다른 단계**이며 이번에는 도장 요청까지 복원했다.

| 몸체/자료 | 10.78 | CD·추가 10.37 |
|---|---|---|
| 전체 지역 투표 | `00455380(x,y,mode)` | `0047b3c0(x,y)` |
| 추가 outpost Array | `0055a70c` | `005670e0` |
| 작업장 Array | `005e189c` | `00565af0` |
| 좌표별 지역 getter | `0046f530` | `004bdc50` → `004bdbf0` |
| float 좌표 snap / CRT 절삭 | `0041d7a0` / `004e49c0` | `00440a80` / `004f161c` |
| raw 지역 / genus getter | `004acda0` / `0049a840` | `004acaa0` / `004abae0` |
| raw 타입 테마 getter | `004ac970` → 타입 `+0x98` | 사용하지 않음 |
| 지역 drawer 생성자 | `0046ebd0` | `004bd070` |
| 지형 flood/도장 경계 | `00470900(this,x,y,owner,theme)` | `004bd2d0(this,x,y,owner)` |
| theammode 정수 조회 경계 | `00441270` | 사용하지 않음 |
| 영향 지역 vector / clear | `0055a718` / `00455160` → `00454560` | 사용하지 않음 |
| wrapper revision / dirty 쓰기 | `0059a8b0` / `005949d4` | 없음 |

## 복원 계약

추가 outpost와 작업장 목록은 **별도 거리 후보 목록·factories·ownerFactories와 합치지 않는다**. 항목마다 객체의 현재 지역을 조회한 뒤 query 지역을 다시 조회한다. 지역이 같으면 그 시점 raw 소유자 0~8의 표를 한 개 더한다. 목록의 중복 SID도 표로 센다. 최대값이 동률이면 중립이며 소유자 0의 단독 최대도 중립으로 처리한다. 뒤에 더 큰 최대값이 나오면 앞선 동률은 해제한다.

[`MakeTerrainRegionOwnershipHooks`](../../cpppj/src/o/RawRegionOwnership.cpp)는 기존 `RawCanonPlacementTerrain::RegionAt`에 **float 편향 `0x3f7fff58`(약 0.99999)을 더한 snap**을 연결한다. Player 기준점의 0.9999와 다르다. double 합산→0 방향 절삭→float 저장→지역 getter 순서를 유지한다. 지역 getter의 WORD SID·섬 genus·raw +8·다리 sentinel·지도 밖/빈 칸 127을 사용한다. 도장 좌표는 **원래 query float를 편향 없이 절삭**하므로 조회한 칸과 도장 원점이 다를 수 있다.

CD는 최종 소유자로 **한 번** 도장을 요청하며 테마·영향 지역·wrapper revision/dirty가 없다. 패치는 중립일 때 owner 1로 먼저 도장을 요청하고 영향 지역을 지운 뒤 owner 0으로 최종 도장을 요청한다. 비중립일 때는 owner 0의 초기 도장과 승자의 최종 도장을 요청한다.

패치 mode 0의 테마는 **같은 소유자의 모든 작업장**을 순서대로 읽는다. 지역을 다시 걸러내지 않으며 마지막 일치 작업장의 테마가 남는다. 매번 타입 +0x98을 먼저 읽고 초기값 1인 theammode 옵션을 조회한다. 옵션 0이면 테마 0, 그 외에는 타입 값 0→2, 1→3, 2→1, 다른 값→0이다. 다른 일반 mode는 테마 0이다. 타입 +0x98 표는 별도 `themes` span 입력이며 실제 TYPE/옵션 로더 전체 연결을 주장하지 않는다.

패치 mode 10은 처음에 영향 지역을 지우지 않고 각 12바이트 기록의 지역 번호를 대상으로 두 목록을 반복한다. **지역마다 표를 초기화하지 않으며 중복 지역도 다시 누적**한다. 기록의 x/y는 투표에 사용하지 않는다. 테마는 mode 0처럼 선택한다. 일반 호출의 두 도장 이후 영향 지역이 두 개 이상이면 같은 query로 mode 10을 한 번 재귀 호출한다. mode 10 자체는 다시 재귀하지 않는다.

패치는 두 도장 뒤 wrapper revision DWORD를 증가시키고 dirty를 1로 쓴다. 원본 지형 도장 내부에도 별도 revision 증가가 있으므로 **이번 관찰/상태 증가를 전체 지형 갱신 횟수로 해석하지 않는다**. 실제 도장을 연결할 때 같은 상태의 내부 증가와 기록 공급을 보존해야 한다.

[`MakeOutpostRegionOwnershipHooks`](../../cpppj/src/o/RawRegionOwnership.cpp)는 outpost 등록/삭제 뒤 통지를 이 모듈의 mode 0으로 잇는다. 두 어댑터는 다른 SID 풀을 거부한다. 필수 경계/테마 표/목록 동일성·비유한/정수 범위 밖 좌표·잘못된 SID/소유자/표 개수를 진단한다. 외부 효과 이후 실패를 되돌리는 트랜잭션은 제공하지 않는다. 정상 입력 범위는 소유자 0~8·signed int에 담기는 좌표/표 개수다.

## 독립 원본 관찰과 연결 검사

[`decomp_regionownership_oracle.py`](../../tools/decomp_regionownership_oracle.py)는 세 PE 각각 **440개**, 총 **1,320개**를 두 x87 제어 워드 `0x027f`/`0x037f`로 실행한다. wrapper 진입부터 **cdecl 정상 반환**까지 실제 투표·재귀·좌표/지역/테마 getter·CRT·Array clear·drawer 생성자를 실행한다. **지형 flood/도장 몸체와 theammode 조회만 명시 대체**한다. 도장 경계에 영향 지역 입력을 공급하며 실제 flood의 결과를 계산하지 않는다.

12개 목록 장면 × 소유자/테마 변형 3종 × mode 3종 × 옵션 2종 × 도장 후 기록 공급 2종의 432개에 경계 좌표 8개를 더한다. 빈 목록·중복 SID·중립·동률·지역 밖 작업장 테마·기본/잘못된 테마 값·다리 sentinel 31/127·0.99999 경계·지도 밖을 포함한다. 기대 투표나 테마 결과를 Python/C++에서 만들지 않고 원본의 getter/옵션/도장 호출 순서와 최종 영향 지역/revision/dirty를 저장한다.

각 PE 정상 반환 **880회**다. 패치 wrapper 진입 **1,168회**(재귀 288회), 도장 대체 **2,336회**, 옵션 대체/실제 테마 getter 각각 **984회**, 실제 지역 getter **13,152회**다. CD/10.37 각각 도장 **880회**, 지역 getter **8,592회**, 옵션 0회다. ESP·EBX/ESI/EDI/EBP·x87 제어 워드/TOP·drawer 포인터/초기값·옵션 key/초기값·허용 코드/쓰기·raw/두 목록 불변·assert/OS 0·SHA/입력/행/실제 진입/대체 수를 확인한다. [fixture](../../cpppj/tests/fixtures/regionownership-x86.tsv), [근거 JSON](../../cpppj/recovery-regionownership-evidence.json)은 LF다. 앞 인계의 누적 344,146개에 이번 1,320개를 더한 인계 집계는 **345,466개**다.

새 [C++ 검사](../../cpppj/tests/RegionOwnershipTests.cpp) 5개는 세 PE 전체 관찰 재생, 실제 지역 getter와 outpost 생성자→등록→동률/중립→Damageable 삭제 준비→남은 작업장 소유자/테마 복귀, 빠진 경계/다른 풀/손상 입력을 검사한다. 지형 도장과 작업장 postPop 전체는 외부 경계로 유지한다. 최종 빌드/전체 검사/감사 결과는 [LEFT_JOBS.md](../../LEFT_JOBS.md)의 이번 완료 항목을 따른다.

## 재현과 후속

```powershell
tools/ghidra/export_functions.ps1 -Name regionownership
python -X utf8 tools/decomp_regionownership_oracle.py
python -X utf8 tools/decomp_regionownership_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

다음은 **지역 flood/도장 몸체·실제 테마/옵션 자료 연결·작업장 postPop/Regular·미션 목록 확보/해제·저장 맵/raw outpost 일반 Pop/삭제 연결**이다. 사제 Pop·보호막/회복 예약·Carrier와 GUI 건설·경제·전투·승패도 남았다. 이번 투표 복원을 지형 변경이나 실제 미션 완주로 해석하지 않는다. 자산 전수·장시간 변이·최대 지도·창/픽셀 회귀는 계속 인계한다.
