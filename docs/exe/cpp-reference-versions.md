# cpppj 10.78 기준 복원과 추가 판본 비교

2026-10-07 사용자 결정: **cpppj 1차 목표는 `originals/Netstorm.exe`의 10.78 복원이다.** 10.37·10.62·10.82와 개발 일지는 해석을 교차 확인하는 자료로 사용한다. 최신판의 기능·수치·자료 구조를 10.78 규칙으로 채택하지 않는다. [정적 비교 기록](../../cpppj/recovery-reference-versions.json), [같은 위치 조회 x86 기록](../../cpppj/recovery-graphlookup-evidence.json).

현재 PC에서는 원본/복사본 게임, 업데이터·설치/SFX·배치 파일과 클론 창을 실행하지 않았다. Python 바이트 복원·Ghidra 헤드리스·허용한 함수의 Unicorn 격리 실행·Release 빌드·콘솔 CTest만 수행했다. 업데이트 서버에도 접속하지 않았다. 원본 다섯 디렉터리와 AGENTS.md는 변경하지 않았다.

## 10.62 패치의 정적 복원

`originalPatches/NSP1062/update.bat`은 설치 폴더의 현재 파일에 바로 패치를 적용하지 않는다. **CD의 `NetStorm.exe`와 `netstorm.tarc`를 먼저 복사**하고, `zpatch -p`로 각각 `1`·`2`를 적용한 뒤 버전 파일에 10.62를 쓴다. `update37.exe`·`zpatch.exe`·배치 파일은 실행하지 않았다.

패치 도구를 **148개 함수·실패 0**으로 디컴파일했다. 체크섬 `00401710`은 unsigned byte의 현재값과 직전값을 누적하는 DWORD이며, 패치 본문 `00401740`은 원본 오프셋·삭제 길이·삽입 길이·삽입 바이트를 읽는다. `004011c0`에서 길이 1/2/4바이트, 작은 오프셋 2바이트, 1바이트 치환 플래그를 교차 확인했다. [Python 복원 도구](../../tools/reconstruct_patch1062.py)는 이 정상 바이트 경로를 옮기고 체크섬/잘림/역순/범위 초과는 쓰기 전에 거부한다.

| 결과 | 크기 | SHA-256 |
|---|---:|---|
| `extracted/original1062/Netstorm.exe` | 1,695,744 | `16dd89e5a1b7d2ad2c8fffd4fadc4e3f2215f191d5e696b8d7d3848bbc82548a` |
| `extracted/original1062/netstorm.tarc` | 1,021,546 | `9fce7a6e56e461f95cad5207034c2ac96b5eb365d422037a2a1393702d655cdc` |

두 패치 헤더의 체크섬이 실제 CD 자료와 일치했다. 실행 파일 36,244개/아카이브 278개 패치 레코드를 적용하고, TARC **266개 항목** 전체의 경계/길이를 확인했다. `--verify`는 저장 결과의 모든 바이트를 새로 계산한 결과와 비교한다. 설치 프로그램을 실제로 실행해 얻은 파일과 대조한 것은 아니며, 복원 PE도 실행하지 않았다. 추가 10.37 사본은 CD와 InsertCD 분기 한 바이트가 다르므로 배치가 지정한 CD를 기준으로 쓴다([이전 비교](original1037-comparison.md)).

## 10.82 본체와 업데이터 분리

| 분석 대상 | 역할/결과 | 전체 내보낸 함수 |
|---|---|---:|
| `original1082/Netstorm.exe` | 콘솔 자동 업데이터, `0040129e`·`004013de` 등에서 `_spawnl`로 `./Netstorm.game` 시작 | 2,618 |
| `original1082/netstorm.game` | USER32/GDI32·영상/사운드 라이브러리를 사용하는 실제 PE 게임 본체 | 10,130 |
| 복원한 10.62 `Netstorm.exe` | 기존 방식의 게임 본체 | 3,729 |

세 대상과 패치 도구의 **디컴파일 실패 집계는 0**이다. MinGW/PNG/GIF 분석 경고와 본체 일부 pcode 경고는 남았다. 내보내기 성공은 모든 함수의 원형/동작이 정확하다는 뜻이 아니다. 두 게임에서 검토한 Graph 함수 각각 다섯 개를 별도 읽기 전용 프로젝트로 다시 내보내고 C/불연속 몸체의 SHA를 기록했다.

10.82 업데이터 문자열은 `v10.82+`지만 `netstorm.ver`는 **10.64**다. 이를 게임 본체의 실제 판본 확정 근거로 사용하지 않는다. 공급된 `file.list.txt`의 SHA-1/hex 크기는 **355개 일치**, `d/options.cfg`·`file.list.txt`·`trace.txt` 세 항목 불일치, 누락 0이다. `auto.list.txt`의 업데이터 한 항목은 일치한다. `R.exe`는 PE/해시 정보만 수집했고 역할 전체를 복원한 것은 아니다.

| 검토 구조 | 10.62 | 복원 기준 10.78 | 10.82 본체 |
|---|---:|---:|---:|
| SID 슬롯 폭 | 36 | **50** | 77 |
| graph 필드 위치/폭 | +28 / 1바이트 | **+30 / 1바이트** | +42 / 4바이트 |
| 그래프 한도/무효 번호 | 251 / 254 | **251 / 254** | 50,000 / 50,003 |
| 삭제 준비 | `00460880` | **`004637b0`** | `00472dd0` |
| 위치→그래프 표 조회 | `00460000` | **`00462c30`** | `00472150` |
| 그래프 할당 | `00460c40` | **`00463330`** | `004728c0` |

10.82는 SID 지도 항목 폭도 달라 기존 raw 레이아웃에 그대로 연결할 수 없다. 비교 주소는 검토한 역할 대응이며 전체 자동 매칭/기계어 동일성을 주장하지 않는다. cpppj의 50바이트 슬롯·1바이트 graph·251개 표는 유지했다.

## 10.82 V12 (`original1082v12/`)

`original1082v12/DevLog.txt` 맨 위가 `10.82 V12`이고, 기존 `original1082`는 같은 일지의 **V10**이다(v12는 V11·V12 항목이 추가됨). `netstorm.ver`는 둘 다 10.64여서 판본 근거가 아니다.

| 분석 대상 | 크기 | SHA-256 | 전체 내보낸 함수 |
|---|---:|---|---:|
| `original1082v12/netstorm.game` | 2,333,696 | `4c4a52ac9265667d288e4404c89579d9b43f6ee964a976915aa60d80ef0bcfd5` | 10,141 (V10: 10,130) |
| `original1082v12/Netstorm.exe` | 603,136 | `4e3074c542a165e3b27eac68ed2dda1d37b674a1ea04e7aa4179fb142adfa93a` | 2,618 (V10과 같음) |

디컴파일 실패 집계는 둘 다 0이다. TARC는 291개 항목 중 추가/삭제 0, 변경 18개(`help.english`, `tell.english`, rain/sun/thunder/wind 계열 `.type` 16개)다. 주소를 정규화한 함수 본문 비교에서 V10과 같은 것은 약 6,306개이며 나머지는 주소 이동 때문에 생긴 차이를 포함하는 상한이다. Graph 함수 대응과 DevLog V11/V12 항목의 코드 대조는 아직 하지 않았다. 10.78 복원 기준은 바뀌지 않는다.

```powershell
& tools/ghidra/run_decomp.ps1 -Edition original1082v12
& tools/ghidra/run_decomp.ps1 -Edition original1082v12-launcher
```

## 개발 일지와 바이너리/자료의 대조

`original1082/DevLog.txt`는 Windows-1252로 읽고 원본 인코딩을 바꾸지 않았다. 일지의 주장과 실제 확인 범위를 구분했다.

| 일지 기록 | 실제 자료 확인 | 10.78 복원에 적용하는 의미 |
|---|---|---|
| 10.73/74, 줄 1122: SID 소진 시 다리를 파괴하는 처리 추가 | 10.78의 기존 SID 경계/소진 복구 분석과 이어서 검토할 단서 | 소진 복구는 여전히 후속; 이번에 완료했다고 세지 않음 |
| 10.81 V1, 줄 319: 다리 코드를 10.37 방식으로 변경 | 10.78/10.82 TARC의 `bridge.type`는 바이트 동일 | 타입 파일 동일성을 코드 동일성으로 확대하지 않음; 10.78 함수 기준 유지 |
| 10.81 V1, 줄 279: Sun Generator 추가 | 10.82에만 `sunbattery.type`, 설명 `Sun Generator` 존재 | 10.78 1차 복원 대상에 넣지 않음 |
| 10.81 V4, 줄 164: 캠페인 난이도 변경 | 10.82 TARC에 easy/hard 미션 자료 추가 | 기존 10.78 캠페인 기준 유지 |
| 10.81 V1/V4: 자동 회전·글꼴/추가 설정 | 자동 회전·`fontSize`·기능 옵션 키는 10.82 PE에서 발견, 10.78에서는 미발견 | 문자열 존재를 실행 검증으로 세지 않음 |

일지의 `fontFaceFamily` 키는 두 PE의 ASCII에서 발견되지 않았다. 실제 설정 파서를 검토하기 전 지원 여부를 단정하지 않는다. 일지는 10.82 V9까지 포함하지만 현재 게임 PE가 모든 V9 동작을 구현했다는 증명으로 세지 않는다. 두 TARC 비교는 10.82 **추가 45/제거 0/변경 57/동일 189개**다. SHA/항목 목록/일지의 판본별 표제와 줄 번호는 정적 기록에 저장했다.

## cpppj 후속: 같은 위치의 다른 SID 조회

10.78 `004637b0`은 삭제 원천의 **위치·타입·프레임**과 위치에서 찾은 **그래프 번호**를 따로 사용한다. 0단계 기준점 해시 머리가 다른 SID면 그 SID의 graph byte로 표를 고르고, 이웃 연결 판정과 genus 정책은 삭제 원천 정보를 사용한다. 10.62와 10.82에서도 이 역할 분리를 정적으로 확인했다.

[RawGraph](../../cpppj/src/o/RawGraph.cpp)는 같은 위치의 다른 정상 표면 SID를 허용하고, [Graph::DetachAt](../../cpppj/src/o/Graph.cpp)은 조회 번호를 별도 인자로 받는다. 원천의 graph/state/type/frame을 조회 객체로 교체하지 않는다. 무효/미사용 조회 번호는 원천 프레임/이웃 탐색 전에 반환한다. 손상된 SID/위치/상태/번호·순환·소진은 복사본 계산에서 실패하며 기존 슬롯·지도·표·스택을 보존한다. fencemark/windArcher 특수 조회는 후속이다.

독립 fixture는 **세 실제 PE(10.78/CD/추가 10.37) × x87 53/64비트 × 64입력 = 384회 Detach**다. 원천과 머리의 다른 번호, 원천 254, 머리 254/미사용, 원천 프레임, 다른 해시 단계, 지도 가장자리, 1/9 감소, 일반/rebuild, 내부 spot, 살아 있는/죽은 조회 머리를 포함한다. 대체 함수/assert/OS 도달 0, 각 PE 내부 Allocate/Flood 각각 216회이며 공개 384회에 더하지 않는다. 준비 Reset/Create도 별도다. 10.82 함수의 실행 검증은 수행하지 않았다.

7개 슬롯/dirty 항목은 직접 비교하고 전체 풀/해시/spot/표/스택/통계는 Adler-32를 대조한다. 이전 oracle 도구/fixture/독립 SHA 기록은 수정하지 않았다. 최종 x64 Release 경고/오류 0, **CTest 내부 139개·실패 0**(37.81초), 누적 제한 x86 입력 **60,642개**다. 추가 C++ 검사는 무효/미사용 조회의 조기 반환과 손상 조회의 변경 전 거부를 확인한다.

## 재현과 다음 작업

```powershell
python -X utf8 tools/reconstruct_patch1062.py --self-test
python -X utf8 tools/reconstruct_patch1062.py
& tools/ghidra/run_decomp.ps1 -Edition patch1062
& tools/ghidra/run_decomp.ps1 -Edition original1062
& tools/ghidra/run_decomp.ps1 -Edition original1082
& tools/ghidra/run_decomp.ps1 -Edition original1082-launcher
& tools/ghidra/run_script.ps1 -Edition original1062 -Script ExportCreation.java -ScriptArgs @('extracted/reference-versions/original1062','00460c40','00460880','00460000','004b4030','0045ffa0')
& tools/ghidra/run_script.ps1 -Edition original1082 -Script ExportCreation.java -ScriptArgs @('extracted/reference-versions/original1082','004728c0','00472dd0','00472150','004cf4b0','004720f0')
# 10.78 전체 C와 앞선 graphremove/SID/생성의 추출물은 해당 복원 문서대로 준비한다.
python -X utf8 tools/compare_reference_versions.py
python -X utf8 tools/compare_reference_versions.py --verify
python -X utf8 tools/reconstruct_patch1062.py --verify
python -X utf8 tools/decomp_graphlookup_oracle.py
python -X utf8 tools/decomp_graphlookup_oracle.py --verify
# 빌드가 완료된 뒤 검사한다. 둘 다 콘솔 작업이며 게임 실행 옵션을 붙이지 않는다.
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build -C Release --output-on-failure
```

전역 Graph/SID 소진 복구·건물 부착/dirty/grid·참조/파생 삭제/반납 수명·form/process·상위 수신 목록·raw GameWorld 연결이 남았다. 이후 건설·경제·전투·AI·승패로 이어 간다. 미션 완주는 아직 불가능하다. raw 월드 연결 뒤 TEST01/1-1 생성·표시·선택·이동·해제·재진입, renderer/menu/world·전체화면·오디오 검사는 **다른 PC**에서 수행한다. 원본 비교 실행은 해당 PC의 AGENTS.md와 사용자 지시를 다시 확인하고, 업데이터의 막힌 서버 접속을 복원하려 시도하지 않는다.
