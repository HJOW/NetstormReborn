# 일반 배치 후보 권한과 방향 소유 관계

2026-10-09. **마지막 디컴파일 수행 PC: `HJOW-Athlon`, 2026-10-09.** AGENTS.md·두 인계 문서·cpppj 내부 문서를 읽고 현재/마지막 호스트 일치를 확인했다. 새 `canonpermission` 목록을 같은 PC에서 읽기 전용으로 내보냈다(10.78 3개·CD/추가 10.37 각각 2개). 원본 게임/복사본·클론 창·OS 호출은 실행하지 않았다.

[`RawCanonPlacementPermission`](../../cpppj/src/o/RawCanonPlacementPermission.h)은 그래프 준비 상태·요청 소유자·현재 raw 소유 관계를 검사하고, 조건을 통과한 후보의 Player 작업장·그래프 조회 결과 DWORD를 그대로 돌려준다. **Player 조회 몸체는 필수 외부 경계이며 전체 MayPlace의 복원 완료를 뜻하지 않는다.**

후속 [Player 작업장·그래프 기준점 조회](cpp-player-anchor-reconstruction.md)에서 이 필수 경계를 실제 조회 몸체로 채웠다. 아래 내용과 검사 수는 후보 권한 단계 당시의 기록이며 주변 권한·최종 관계·전체 MayPlace는 계속 후속이다.

| 함수/전역 | 10.78 | CD·추가 10.37 |
|---|---|---|
| 후보 권한 helper | `00462cb0` | `0045bae0` |
| 방향 소유 관계 helper | `004629e0` | 후보 몸체에 인라인 |
| Player 작업장·그래프 조회(명시 대체) | `0048fdb0` | `00406f80` |
| 그래프 준비 상태 | `00540414` | `005207f8` |
| 편집기 / 동맹 사용 / 다른 소유자 허용 | `005c85a4` / `00540cb0` / `0059ab28` | `00518904` / `0050f824` / `0051cbf8` |
| 방향 관계 DWORD 표 | `00595200` | `0050f6e0` |

## 복원 계약

Candidate는 그래프 준비 상태가 0이면 0을 반환한다. 그 다음 요청 소유자가 0이면 1을 반환한다. 두 분기 모두 SID/raw를 읽지 않는다. 이어 현재 raw의 소유자 BYTE(패치 +34, CD +32)를 읽고 편집기·같은 소유자·동맹 사용 시 `현재 소유자 * 9 + 요청 소유자`의 관계 DWORD 순서로 검사한다. 역방향 관계는 대체하지 않는다.

소유 관계가 성립하지 않더라도 현재 소유자가 0이거나 다른 소유자 허용 전역이 켜져 있으면 Player 조회로 이어진다. 이 예외는 관계 조회 뒤에 적용된다. 별도 Related는 편집기·같은 소유자·방향 동맹만 검사하며 중립/다른 소유자 허용 예외를 적용하지 않는다. 원본 주소 산술의 DWORD 감김을 유지하고 실제 관계 읽기가 81칸 표 밖이면 C++에서 진단한다. owner 각각을 0..8로 제한하지 않으므로 현재 1/요청 `0xffffffff`의 인덱스 8도 보존한다.

Player 경계에는 요청 소유자 DWORD·현재 raw 타입 BYTE·좌표 float 비트값을 전달한다. 좌표 NaN/부호 있는 0을 산술 변환하지 않는다. 경계가 반환하는 0/1/SID/큰 DWORD를 bool로 축소하지 않으며, 호출 중 raw/전역이 바뀌어도 반환 후 같은 관계를 다시 검사하지 않는다. 다음 호출에서는 현재 상태를 다시 읽는다.

`MakeCanonPermissionTerrainHooks`는 같은 SID 풀만 연결하고, 원래 query의 owner/argument/mode를 유지한 채 후보 helper 인자만 원본 MayPlace의 **하위 BYTE 부호 확장**으로 변환한다. 이 지형 어댑터에서만 반환 DWORD를 0 여부로 변환한다. 주변 권한·최종 관계 정책은 기존 필수 경계로 남는다.

## 독립 원본 실행과 검증 범위

[`decomp_canonpermission_oracle.py`](../../tools/decomp_canonpermission_oracle.py)는 실제 PE의 후보 함수 전체를 정상 반환까지 실행한다. 10.78은 후보 **5,520개**와 별도 관계 **330개**, CD/추가 10.37은 후보 각각 **5,520개**, 총 **16,890개**다. 두 x87 정밀도 `0x027f/0x037f`의 모든 관찰이 일치한다. 조기 반환·큰 SID/요청 owner·방향 관계·중립/다른 소유자 허용·raw 타입 82/83/255·소수/부호 있는 0/NaN/최대 float·조회 반환 0/1/50/`0xffffffff`·조회 중 raw/편집기 변화를 포함한다. 기대 반환/사건을 Python/C++의 권한 규칙으로 계산하지 않는다.

**Player 조회 진입에서만 인자 네 DWORD를 관찰한 뒤 지정 반환/변화를 명시 대체한다.** 그 몸체의 그래프 조회·최대 40개 작업장/후보 목록·선택/할당은 실행하지 않는다. 후보 helper의 실제 조기 분기와 관계 표 읽기·정상 반환·cdecl ESP/보존 레지스터·x87 상태는 검사한다. 실제 관계 주소가 표 밖인 입력만 제외하고 조기 반환에서 읽히지 않는 큰 SID/owner는 유지한다.

각 판본 후보 진입 **11,040회**, 명시 Player 대체 **3,768회**, 패치 별도 관계 진입 **660회**를 감사한다(두 정밀도의 합계). Player 몸체의 native 호출은 0회다. 허용 코드/쓰기·반환 DWORD·콜백 인자/횟수·raw 전체·편집기·관계 표 checksum·assert/OS 0회·SHA/행/실제 진입 수를 [근거 JSON](../../cpppj/recovery-canonpermission-evidence.json)에 기록했다. 누적 독립 fixture 입력은 **328,123개**다. 기존 fixture/감사 도구는 수정하지 않았다.

새 C++ 검사는 세 PE 전체 재생, 두 판본의 실제 3×3 decoder→미리보기→finder→지형→후보 권한 연결, 첫 조회 거부/두 번째 조회 허용 뒤 권한 누적·조회 생략, 그래프 비활성일 때 모양별 주변 경계, signed owner 어댑터·현재 상태 재조회·조기 SID 읽기 생략·필수 경계/풀/관계 주소 진단을 포함한다. 합성의 Player·주변/최종 정책은 공급 경계이므로 전체 원본 배치 성공 검사로 해석하지 않는다.

## 다음 단계와 재현

다음 작은 작업은 **Player 작업장·그래프 조회 `0048fdb0`/CD `00406f80`**, 실제 주변 권한 finder, 최종 표면 소유 관계·특수 지역·거부 조건이다. 이어 사제 Pop·보호막 생성/회복 예약·Carrier 상태 검사, raw GUI·건설·경제·전투·승패를 진행한다. 장시간 실제 자산 전체 합성·변이 전수·창/픽셀·최대 지도 회귀는 인계한다.

```powershell
tools/ghidra/export_functions.ps1 -Name canonpermission
python -X utf8 tools/decomp_canonpermission_oracle.py
python -X utf8 tools/decomp_canonpermission_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

Release 경고/오류 **0**, CTest 내부 **401개·실패 0**(101.73초), 근거 감사 **58종 모두 통과**했다. 함수 목록 설명을 정확히 정리한 뒤 새 원본 관찰/근거를 다시 생성하고 새 SHA 감사도 통과했다. 로그는 `extracted/cpp-canonpermission-export.log`, `cpp-canonpermission-oracle.log`, `cpp-canonpermission-build.log`, `cpp-canonpermission-ctest.log`, `cpp-canonpermission-audits.log`다. 완료 검사와 후속은 [LEFT_JOBS.md](../../LEFT_JOBS.md)를 따른다.
