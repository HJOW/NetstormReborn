# 배치의 최종 표면 소유 관계·특수 지역·거부 조건 복원

2026-10-09, **마지막 디컴파일 수행 PC: `HJOW-Athlon`**. AGENTS.md·두 인계·cpppj 문서를 읽고 현재/이전 디컴파일 호스트 일치를 확인했다. 새 `canonrelations` 목록을 읽기 전용으로 내보냈다(10.78 6개·CD/추가 10.37 각각 2개). 게임/창 실행 없이 정적 분석·제한 기계어 실행·콘솔 검사만 수행했다.

[`RawCanonPlacementRelations`](../../cpppj/src/o/RawCanonPlacementRelations.h)은 `RawCanonPlacementTerrain::Finish`의 지면 누적 뒤에 실행하는 최종 표면 관계와 특수 지역 제한을 복원한다. 일반/3×3 받침 검사에서 픽셀 getter→decoder→미리보기→finder→지형→Player/주변 권한→최종 반환을 실제 C++ 구현으로 연결했다. **원본 전체 MayPlace의 진입부터 반환까지 독립 대조한 것은 아니며, 실제 자산 전수·GUI 건설·미션 완주는 후속이다.**

## 함수와 자료 대응

| 역할 | 10.78 | CD/추가 10.37 |
|---|---|---|
| MayPlace / 독립 실행 시작 | `0049b510` / `0049bde0` | `00445200` / `00445a9c` |
| 현재 표면 좌표 조회 | `0040eaf0` | 최종 구간 인라인 |
| 표면 번호/포인터 유효성 | `0040ea40` | 최종 구간 인라인 |
| 편집기/동일 owner/활성 방향 관계 | `004629e0` | 최종 구간 인라인 |
| 특수 섬 지역 제한 | `0046f260` | 해당 분기 없음 |
| 실제 CRT 절삭 | `004e49c0` | `004f161c` |
| 현재 표면 지도 포인터 | `00542514` | `005670cc` |
| 표시용 차단 / 지면·권한 우회 | `0059ab2c` / `0059ab30` | `0051cbfc` / `0051cc00` |
| 방향 DWORD 관계 표 | `00595200` | `0050f6e0` |

현재 표면 지도는 각 65,536칸 WORD이며, 앞 단계 지형/주변 탐색의 고정 섬 지도 `005c84bc`/CD `0052d590`과 구별한다. 조회 좌표는 **원래 요청 x/y에 원본 float `0.9999`를 더한 뒤 0 방향으로 절삭**한다. 디컴파일 C에 생략된 fadd를 기계어/원본 상수로 확인했다.

요청 owner는 하위 BYTE를 부호 확장한 DWORD다. raw 표면 owner는 패치 +34/CD +32의 부호 없는 BYTE다. 유효 표면은 번호 1 이상·풀 용량 미만이며 free/dead/type 상태를 추가 검사하지 않는다. 원본 고정 용량은 24,000이다.

패치 `IslandList`는 `00568b60`의 부호 있는 count DWORD와 id×32의 레코드로 구성된다. 레코드 +4가 제한 DWORD, +8이 존재 DWORD다. 지역 번호는 BYTE를 **부호 확장**하여 sentinel DWORD와 비교한다. 없는 지역/잘못된 count는 원본 assert에 해당하며 C++에서는 예외로 진단한다. CD에는 이 읽기가 없다.

## 최종 판정 순서

1. 기존 Terrain::Finish가 bridge/genus/flags1/noIsland/groundComplete를 이용해 canPlaceGround를 계산하고 필요한 permission 취소를 수행한다.
2. genus `0x34000` 중 하나가 있고 `0x200000`이 없으면 현재 표면을 조회한다. 무효 표면은 내부 거부다. `0x4000`이고 중립 표면이면 관계 조회를 생략한다. 나머지는 편집기/동일 owner/활성 방향 동맹을 적용한다.
3. 내부 관계 거부를 표시 전역에 쓰고, genus `0x4200` 중 하나가 있으면 **표시 전역만 0으로 만든다**. 내부 거부는 남아 최종 반환을 막는다.
4. 패치에서 groundComplete가 true·genus `0x200`·첫 지역이 sentinel과 다르면 실제 지역 제한을 읽는다. 제한 DWORD가 0이 아니면 canPlaceGround를 취소한다. groundComplete 자체는 유지한다.
5. genus `0x200000`이면 여기서 true를 반환한다. 앞선 특수 지역 읽기는 여전히 수행할 수 있다. 비정상 좌표를 읽지 않는 이 최종 구간의 성질은 전체 MayPlace 접두의 좌표 계약과 별개다.
6. 현재 표면을 다시 조회한다. genus `4`가 없고 유효한 비중립 표면이며 방향 관계 표가 0이고 genus `0x20a000`이 없으면 최종 소유자 거부를 만든다. 이 직접 표 읽기는 **편집기·동일 owner·동맹 활성 조건을 보지 않는다**. 패치는 앞부분의 거부 지역 변수를 유지하고 CD는 그 지역 변수에 1을 쓴다.
7. `(지면·권한 우회 || (permission && canPlaceGround)) && !내부 관계 거부 && !최종 소유자 거부`를 반환한다. 우회 전역은 관계/소유자 거부를 없애지 않는다.

`RawCanonPlacementPermission::Alliance`는 직접 방향 표 읽기를 공유한다. DWORD 인덱스 `currentOwner*9+owner`의 감김을 유지하되 실제 81칸 밖 읽기는 진단한다. `MakeCanonRelationsTerrainHooks`는 기존 후보/주변 훅을 유지하고 최종 반환만 연결한다. 자료와 상태는 호출자가 소유하며 같은 SID 풀을 사용하는 모듈만 연결한다.

## 독립 원본 관찰과 C++ 연결 검사

새 [생성기](../../tools/decomp_canonrelations_oracle.py)·[근거 JSON](../../cpppj/recovery-canonrelations-evidence.json)·[fixture](../../cpppj/tests/fixtures/canonrelations-x86.tsv)는 세 PE 각각 **2,307개, 총 6,921개**다. 판본별 입력은 지면/권한 768개, 소유자/편집기/활성·비활성 동맹/이전 거부 1,296개, 좌표/번호 경계 192개, signed 지역/sentinel 42개, 최종 조기 반환 9개다. 관계 표 한 방향 칸만 채워 전치 방향과 다른 표면 owner 읽기를 구별한다. 누적 독립 fixture는 **341,410개**다.

원본 앞부분의 누적 지역 변수만 공급하고 최종 구간에서 성공·실패 에필로그를 거쳐 실제 `ret 24`로 반환한다. 패치 표면/유효성/방향 관계/특수 지역 helper와 CRT는 전체 원본 몸체를 실행한다. CD 표면/관계 인라인도 실제 명령이다. **함수 대체 0, OS 호출 0**이며 decoder/finder/Player/전체 MayPlace는 이 생성기에서 실행하지 않는다.

두 x87 제어 워드 `0x027f`/`0x037f` 관찰이 같으며 ESP·반환·EBX/ESI/EDI/EBP 복원·x87 스택·허용 쓰기·assert 0을 확인한다. 반환/permission/canPlaceGround/표시 차단/내부 거부/최종 거부/이전 거부 저장값과 raw·지도·관계 표 checksum을 기록한다. 도구/PE/내보내기/fixture SHA, 입력 순서/31열/개수, 실제 조건/helper 진입도 감사한다.

두 정밀도 합계로 각 PE의 최종 구간 진입은 4,614회·마지막 조건 관찰은 3,522회다. 패치 실제 표면 조회 5,338회·유효성 4,994회·방향 관계 1,098회·특수 지역 932회·CRT 10,676회, CD/추가 10.37 CRT 각각 10,676회다. 원본 특수 지역 존재 assert에 닿는 입력은 제외했다.

새 [C++ 검사](../../cpppj/tests/CanonPlacementRelationsTests.cpp) 6개는 세 PE 전체 fixture 재생, 두 판본 일반/3×3 받침의 실제 모듈 합성, 필수 자료/다른 풀/잘못된 지역/조기 읽기 생략을 검사한다. 실제 Player 권한을 얻어도 마지막 방향 표가 0이면 거부한다. 현재 표면 지도만 외국 소유자로 바꾼 경우, 편집기/우회 조건과 패치에만 존재하는 특수 지역 제한을 검사한다. 최초 통합 검사에서 합성 섬 raw의 배치 허용 extra 비트가 빠져 지형 전에 거부되는 장면을 찾아 입력을 보완했다. 또한 일반 0x4000도 표시 초기화 마스크 0x4200에 포함됨을 원본 관찰로 확인하여 통합 검사 기대치를 보완했다. 원본 관찰값을 수정해 실패를 숨기지 않았다.

## 재현과 남은 범위

```powershell
tools/ghidra/export_functions.ps1 -Name canonrelations
python -X utf8 tools/decomp_canonrelations_oracle.py
python -X utf8 tools/decomp_canonrelations_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

2026-10-09 후속: [전체 MayPlace 제한 입력의 정상 반환 대조·대표 실제 자산 연결](cpp-canon-mayplace-reconstruction.md)을 완료했다. 다음은 실제 저장 맵/raw 세계·공간 장부와 별도 Player 추가 목록의 생성/삭제/공간 수명이다. 이어 사제 Pop·보호막 생성/회복 예약·Carrier 상태 검사와 raw GUI·건설·경제·전투·승패를 연결한다. 여러 판본 자산 전수·장시간 변이·최대 지도·창/픽셀 회귀는 [LEFT_JOBS.md](../../LEFT_JOBS.md)에 인계한다.

검증 로그는 `extracted/canonrelations-export.log`, `canonrelations-oracle-final.log`, `canonrelations-build-final.log`, `canonrelations-ctest-final.log`, `canonrelations-audits-final.log`다. Release 경고/오류 **0**, CTest 내부 **417개·실패 0**(109.58초), 감사 **61종 모두 통과**. 기존 fixture/감사 도구는 바꾸지 않았다.
