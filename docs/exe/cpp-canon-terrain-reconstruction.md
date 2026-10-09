# 일반 타입 배치의 지형·지역 누적과 권한 경계

2026-10-09. **마지막 디컴파일 수행 PC: `HJOW-Athlon`, 2026-10-09.** AGENTS.md와 두 인계 문서·cpppj 내부 문서를 읽고 현재/마지막 PC 일치를 확인했다. 새 `canonterrain` 목록을 같은 PC에서 읽기 전용으로 내보냈다(10.78 7개·CD/추가 10.37 각각 8개). 원본 게임/복사본·클론 창·OS 호출은 실행하지 않았다.

[`RawCanonPlacementTerrain`](../../cpppj/src/o/RawCanonPlacementTerrain.h)은 일반 타입/패턴의 초기 권한, 후보 다리·섬 효과, 지역 배열, 모양 종료, 최종 관계 진입 전 지면/권한 누적을 복원한다. 기존 사제 지형 API/구현은 유지했다. 이 단계는 전체 MayPlace나 최종 배치 관계 판정의 완료를 뜻하지 않는다.

후속으로 [후보 권한·방향 소유 관계](cpp-canon-permission-reconstruction.md)를 복원하고 이 지형에 연결했다. 아래 내용과 검사 수는 지형 단계 당시의 기록이다. 현재 미복원 경계는 Player 내부 작업장·그래프 조회, 주변 권한 finder, 최종 표면 소유 관계/특수 지역이다.

| 관찰 구간 | 10.78 | CD·추가 10.37 |
|---|---|---|
| decoder 직후 초기화 | `0049b84e..0049b8af` | `00445542..0044559d` |
| 후보 진입 / 지형 시작 | `0049b9c4` / `0049bb1a` | `004456ad` / `004457f7` |
| finder Next 직전 종료 | `0049bca5` | `00445966` |
| 모양 종료 / 주변 권한 진입 전 | `0049bcb9..0049bd1d` | `0044597e..004459d4` |
| 최종 지면 누적 / 관계 분기 전 | `0049bde0..0049be34` | `00445a9c..00445b05` |
| 지역 조회 / raw 지역 getter | `0046f4e0` / `004acda0` | `004bdbf0` / `004acaa0` |

## 구현 계약

초기 permission은 `(group == 10 || flags2 & 0x10202000) && !(flags2 & 4)`다. Begin은 bridge=false·noIslandOnly=true·groundComplete=true로 시작하며 지역 배열은 유지한다. 유효 모양이 있을 때만 144바이트 배열을 sentinel 하위 BYTE로 채우고 snapped 좌표·현재 발자국으로 원점과 `flags1 & 6`을 캡처한다. 발자국 0도 허용하되 배열의 물리 범위 0..12를 넘는 입력은 진단한다.

후보가 충돌에서 무시되더라도 다리·섬 효과를 받는다. 현재 raw 타입/프레임, noIsland 전역과 실제 지역 SID 지도/getter를 사용한다. 다리 겹침의 실제 assert 조건은 배치 폭과 높이의 동일성이다. 허용 지면과 프레임 지면이 맞으면 원본 열 간격 12·8×8 쓰기 제한·지역 하위 BYTE를 유지한다. signed BYTE와 sentinel DWORD 비교, 모양 사이 groundComplete=false 누적은 [기존 사제 지형 기록](cpp-priest-terrain-reconstruction.md)과 같다.

최종 관계 분기 직전 canPlaceGround는 `(flags2 & 4) || (flags1 & 0x400 && noIslandOnly) || groundComplete || bridgeOverlap`이다. flags1 `0x400`이고 noIslandOnly와 groundComplete가 모두 false이면 permission을 해제한다. bridge의 지면 우선 허용을 사제 전용 식과 구분했다.

세 개의 **필수 경계**를 호출자가 제공한다. 지면이 맞는 섬 후보 뒤 permission이 false인 경우에만 candidatePermission을 호출한다. 모양 종료 뒤 permission이 false인 경우에만 shapePermission을 호출한다. true 권한은 다음 후보/모양에 유지된다. Finish는 지면 누적 뒤 finishRelations에 실제 요청과 현재 상태를 넘기고 그 반환값을 사용한다. 정책 누락은 생성 시 진단한다. 권한 helper·주변 관계 finder·최종 표면 소유 관계/특수 지역 helper의 몸체 복원은 후속이다.

`MakeCanonTerrainGeometryHooks`는 원래 타입/argument/flags/owner/mode를 캡처한다. 실제 geometry가 공급하는 모양 전체(프레임·라벨·방향·원래/snapped 좌표)를 주변 권한 경계에 전달한다. 후보 지형 어댑터는 기존 실제 finder 순회를 유지한다. 같은 SID 풀만 연결하며 후보 충돌 거부 뒤의 지형·모양 종료·최종 효과는 생략한다.

## 독립 원본 대조와 한계

[`decomp_canonterrain_oracle.py`](../../tools/decomp_canonterrain_oracle.py)는 세 실제 PE 각 **3,740개**, 총 **11,220개**를 두 x87 정밀도 `0x027f/0x037f`로 실행했다. 판본별 초기화 28개·후보 3,200개·모양 종료 144개·지역 조회 128개·최종 지면 누적 240개다. 일반/bridge/방어/사제/특수 genus와 그룹 0/10, 빈 발자국, 소수 좌표·지면·extra·noIsland·WORD 지역·signed sentinel·누적 상태를 포함한다. Python/C++의 지형 규칙으로 기대 배열을 계산하지 않는다.

초기화·후보·모양 종료·최종 지면 누적은 **MayPlace 중간 구간에 필요한 지역 변수/레지스터를 공급한 관찰**이다. 후보 permission=true를 공급하여 권한 helper를 우회하고 모양 종료도 주변 권한 진입 전에 종료한다. 최종 지면 누적은 소유 관계 분기 직전에 종료한다. fixture의 result는 지역 조회 행에서만 원본 반환값이고 나머지는 미관찰 값 0이다. canGround도 최종 지면 누적 행에서만 관찰한다. 이를 일반 배치 성공/최종 반환/전체 MayPlace ABI 검증으로 해석하지 않는다.

지역 조회는 실제 helper 전체를 정상 반환시켜 스택·보존 레지스터를 확인한다. 모든 관찰에서 허용 코드/쓰기·raw/144바이트 배열·ESP/x87·assert/OS 0회·SHA/행/구간/실제 호출 수를 감사한다. 각 PE의 두 정밀도 합계는 초기화 56회·후보 6,400회·모양 종료 288회·지역 조회 256회·최종 지면 누적 480회다. 실제 좌표 절삭 3,200회·지역 함수 1,856회·raw 지역 getter 784회다. [새 근거 JSON](../../cpppj/recovery-canonterrain-evidence.json)에 기록했으며 기존 fixture/감사 도구는 수정하지 않았다.

합성 검사는 실제 3×3 받침 decoder→미리보기→finder→지역 지도로 아홉 모양을 진행한다. 첫 섬의 권한을 이후 후보/모양에 유지하고, 지면 불일치의 후보 권한 조회 생략·모양별 주변 조회·필수 최종 정책의 거부와 충돌 이후 효과 생략을 확인한다. 빈 decoder의 배열 유지·주변 모양 라벨·별도 argument/owner/mode·빈 발자국/자료/풀 계약도 확인한다. 초기 픽셀 shape와 권한/최종 관계는 공급 경계이며 이 합성을 원본 전체 MayPlace 실행으로 해석하지 않는다.

다음은 후보 권한 helper, 실제 주변 관계 finder, 최종 표면 소유 관계·특수 지역·거부 조건의 복원이다. 이어 사제 Pop·보호막 생성/회복 예약·Carrier 상태 검사, raw GUI·건설·경제·전투·승패를 진행한다. 여러 판본 실제 자산 전체 합성·변이 전수·창/픽셀·최대 지도 회귀는 계속 인계한다.

Release 경고/오류 **0**, CTest 내부 **395개·실패 0**(106.75초), 감사 **57종 모두 통과**했다. 로그는 `extracted/cpp-canonterrain-export.log`, `cpp-canonterrain-oracle.log`, `cpp-canonterrain-build-final.log`, `cpp-canonterrain-ctest-final.log`에 있다. 완료 범위와 후속은 [LEFT_JOBS.md](../../LEFT_JOBS.md)를 따른다.

최근 일본 유통판 지원 추가로 기존 destroygraph 근거의 `run_script.ps1` SHA가 달랐다. 세 PE/두 x87 정밀도로 기존 1,728개를 다시 실행하고 fixture 바이트·실제 호출 수·관찰/한계 전부 동일함을 확인했다. [`recovery-destroygraph-evidence.json`](../../cpppj/recovery-destroygraph-evidence.json)의 해당 SHA 하나만 갱신했다. 기존 입력을 누적 수에 다시 더하지 않는다. 감사/재생 로그는 `extracted/cpp-canonterrain-audits-final.log`, `cpp-canonterrain-destroygraph-refresh.log`, `cpp-canonterrain-destroygraph-verify.log`다.
