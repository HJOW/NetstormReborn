# 배치 모양의 주변 표면 탐색과 권한 누적 복원

2026-10-09, **마지막 디컴파일 수행 PC: `HJOW-Athlon`**. AGENTS.md·두 인계 문서·cpppj 문서를 읽고 현재 호스트와 이전 디컴파일 호스트가 같음을 확인했다. 새 `canonsurrounding` 목록을 읽기 전용으로 내보냈다(10.78 25개·CD/추가 10.37 각각 21개). 원본 게임/복사본·클론 창·OS를 실행하지 않았다. 보호 파일·AGENTS.md·dotnetpj는 변경하지 않았고 커밋/푸시는 하지 않았다.

`RawCanonSurfaceWalk`는 현재 CanonDecoder 모양의 flag 8 표면 지도 순회를, `RawCanonPlacementSurrounding`은 그 반환 표면에서의 주변 권한 누적을 복원한다. 이전의 후보 권한/Player 조회와 연결해 `RawCanonPlacementTerrain::EndShape`의 주변 경계를 실제 구현으로 바꾼다. **최종 표면 관계/특수 지역/최종 거부는 필수 외부 정책이며 전체 MayPlace와 실제 건설 플레이는 아직 미완료**다.

## 함수 대응과 자료

| 역할 | 10.78 | CD/추가 10.37 |
|---|---|---|
| 현재 모양에서 탐색기 생성 | `004b2590` | `004ebce0` |
| 모양 float 발자국/중심 | `00425af0` / `00425a90` | `00420140` / `004200d0` |
| 내부 spot 검사·flag 8 초기화 | `004b1d70` / `004ab880` | `004ebf30` / `00487810` |
| 지도 반복·파생 Next | `004b1c20` / `004b1e70` | `004ebdd0` / `004ec030` |
| 파생 가상 필터 | `004b1e80` | `004ec040` |
| 중심 방향·프레임 접합 | `0041ce90` / `00441e40` | `0043f620` / `004d2e00` |
| MayPlace 주변 권한 구간 | `0049bd1d` 이상 `0049bdc7` 미만 | `004459d4` 이상 `00445a82` 미만 |
| 실제 후보 권한·Player 조회 | `00462cb0` / `0048fdb0` | `0045bae0` / `00406f80` |

표면 탐색 지도는 `005c84bc` / CD `0052d590`의 **WORD 번호 지도**다. 일반 4단계 해시나 Player 그래프 기준점의 표면 조회와 구별한다. spot은 `005c7c44` / CD `0052fe48`의 BYTE 지도다. 후보 frame은 패치 +36 DWORD/CD +34 BYTE, owner는 패치 +34/CD +32 BYTE다.

## 복원한 순서

- 현재 모양의 **원래 x/y**로 float 발자국/중심을 계산한다. snapped 좌표나 기존 충돌 사각형으로 대신하지 않는다. 발자국 모서리 clamp와 가운데 계산은 원본 getter의 저장 정밀도를 따른다.
- 발자국 전체 spot을 1..255 범위에서 y/x 순서로 AND한다. 공통 내부 비트 8이면 첫 결과 0으로 종료한다.
- 확장 사각형을 y/x 순서로 진행하며 네 모서리는 읽지 않는다. 지도 밖 칸/0 번호와 기준점이 원래 float 발자국 안에 있는 후보를 제외한다.
- 기준점 spot은 원본 float `0.9999`를 더한 뒤 CRT처럼 절삭한 **선형 y×256+x 주소**에서 읽는다. 표면 타입→중심 방향/접합→dead/기준점 내부 순서로 필터한다. flag 8에서는 일반 발자국 교차 필터를 실행하지 않는다. abstract/buried는 이 경로에서 단독 거부 조건이 아니다.
- 원본 WORD 캐시로 같은 번호의 중복 반환을 막는다. 지도/raw/후보 프레임과 코드 표는 다음 후보에서 다시 읽는다. source의 프레임 **번호**는 고정하며 실제 source 코드 표도 후보마다 읽는다.
- 모양 종료 시 이미 권한이 true이면 주변 탐색을 생략한다. 그렇지 않으면 모든 반환 표면에 편집기/동일 owner/방향 동맹/다른 owner 허용을 먼저 적용하고 실제 후보 helper를 호출한다. 중립도 이 바깥 관계 조건을 통과해야 한다.
- helper가 true를 반환한 뒤에도 현재 모양의 나머지 표면을 계속 검사한다. 요청 owner는 원래 DWORD를 보존하고 이 구간 인자만 하위 BYTE를 부호 확장한다.

자료 계약은 실제 크기의 타입/프레임 표와 각 65,536칸 지도/spot, 양수 1..12 발자국과 유효 프레임/유한 지도 좌표다. 원본의 배열 밖 주소/64 WORD 캐시 초과/signed WORD 범위 밖 SID는 C++ 예외로 진단한다. **0 발자국·비정상 좌표의 주변 경로까지 동등하다는 주장은 하지 않는다.** 일반 지형의 0 발자국/빈 decoder 처리 자체는 기존 구현에 남아 있다.

## 독립 기계어 대조와 연결

새 [기대값 생성기](../../tools/decomp_canonsurrounding_oracle.py)와 [근거 JSON](../../cpppj/recovery-canonsurrounding-evidence.json)은 **세 PE 각각 524개(전체 finder 140·주변 구간 384), 총 1,572개**를 기록한다. 두 x87 제어 워드 `0x027f`/`0x037f`의 관찰이 같다. 누적 독립 fixture는 **334,489개**다.

finder 생성/가상 필터/순회/기하/접합/CRT는 전체 원본 몸체를 정상 반환까지 실행한다. ABI의 ESP/ret N·보존 레지스터·x87를 검사한다. 주변 권한은 MayPlace의 위 중간 구간만 실행하고 다음 decoder Advance 진입 전에 종료한다. 그 구간의 원본 바깥 관계/후보 helper/반복은 실제 명령이다. **Player locator 진입만 인자/횟수 관찰 후 지정 반환과 미래 입력 변경으로 명시 대체**한다. 이전 단계에서 복원한 Player 전체 실행과는 별도 근거다.

원본 코드의 쓰기는 finder/스택/FS 밖에서 거부한다. 결과/정수 커서/WORD 캐시·조회 인자/횟수·raw/지도/spot checksum을 C++와 비교한다. 입력은 1×1/3×2/12×12, 소수 좌표·지도 끝/clamp, 중복/내부/죽음/프레임, 방향 관계·초기 권한 생략, 첫 반환/조회 뒤의 raw/지도/코드 표 변화를 포함한다. 각 PE에서 생성 664회·Next 1,506회·후보 helper 970회·Player 대체 256회이며 assert/OS는 0회다. SHA/행/실제 진입/대체 횟수도 감사한다.

[C++ 검사](../../cpppj/tests/CanonPlacementSurroundingTests.cpp)는 세 PE 전체 재생, 실제 Player 기준점→주변 helper→EndShape 권한 누적, 중립의 바깥 조건과 그래프 비활성, 누락 프레임/잘못된 크기/다른 SID 풀의 진단을 검사한다. 임시 최종 정책의 true를 원본 전체 MayPlace 성공으로 해석하지 않는다.

## 재현과 후속

```powershell
tools/ghidra/export_functions.ps1 -Name canonsurrounding
python -X utf8 tools/decomp_canonsurrounding_oracle.py
python -X utf8 tools/decomp_canonsurrounding_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

다음은 **최종 표면 소유 관계·특수 지역·최종 거부**와 별도 Player 추가 목록의 생성/삭제/공간 수명이다. 이후 사제 Pop·보호막 생성/회복 예약·Carrier 상태 검사, raw GUI·건설·경제·전투·승패로 이어 간다. 실제 자산 전수/장시간 변이·최대 지도·창/픽셀 회귀는 계속 [LEFT_JOBS.md](../../LEFT_JOBS.md)에 인계한다.

Release 경고/오류 **0**, CTest 내부 **411개·실패 0**(114.98초), 감사 **60종 모두 통과**. 로그는 `extracted/canonsurrounding-oracle-final.log`, `canonsurrounding-build-final.log`, `canonsurrounding-ctest-final.log`, `canonsurrounding-audits-final.log`다. 기존 fixture/감사 도구는 바꾸지 않았다.
