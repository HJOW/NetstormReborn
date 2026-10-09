# 사제 배치의 타입 기준점과 실제 자산 연결

마지막 디컴파일 수행 PC: **HJOW-Athlon**, 2026-10-09. 같은 PC에서 `typehotspot` 목록을 읽기 전용으로 내보냈다(10.78·CD·추가 10.37 각각 3개). 원본 게임과 OS 호출은 실행하지 않았다.

## 타입 기준점

| 항목 | 10.78 | CD·추가 10.37 |
|---|---|---|
| 타입 초기화 | `0049ebb0` | `004434d0` |
| 속성 로더 | `0049c3b0` | `004460f0` |
| X 숫자 속성 진입 | `0049d6f9` | `00447428` |
| Y 숫자 속성 진입 | `0049d758` | `00447487` |
| 다음 속성 경계 | `0049d090` | `0044825a` |
| CRT ftol | `004e49c0` | `004f161c` |
| X/Y 저장 위치 | 타입 `+0x1dc/+0x1e0` | 타입 `+0x1bc/+0x1c0` |
| X/Y 고정 배율 | `00503358=16`, `0050334c=11` | `005019b8=16`, `005019bc=11` |

초기화 함수는 타입 레코드를 0으로 지운다. `default_hotspot` 플래그는 flags1의 비트 0을 켜며, 비율 속성이 없으면 타입 기준점은 0이다. `hotFootRatioX/Y`는 숫자 AST 노드의 **float32** 값을 x87에 올리고 각각 16/11을 곱해 CRT ftol로 0쪽으로 버린다. 곱셈 뒤 중간 float32 저장은 없다. 속성은 선언 순서대로 각 축을 덮어쓴다.

[`RiftTypeTable`](../../cpppj/src/o/RiftTypeTable.cpp)은 파싱된 double을 float32로 변환한 뒤 `TypeHotFootOffset`에 전달하고, 결과를 [`RiftTypeRecord`](../../cpppj/src/o/RiftType.h)의 `hotspotX/Y`에 저장한다. 발자국 크기나 현재 화면 배율은 이 변환에 쓰이지 않는다. 예를 들어 0.49999999는 float32에서 0.5가 되어 X 기준점 8을 만든다. 원본이 assert하는 문자열 비율과 절대 `hotFootX/Y` 속성은 예외로 보고한다. 비유한 값과 signed int32 범위 밖 값도 예외로 처리하며, 그 범위 밖 ftol의 low DWORD 동작을 복원했다고 주장하지 않는다.

## 자산을 배치 모듈에 전달

[`PriestPlacementAssets`](../../cpppj/src/client/PriestPlacementAssets.h)는 `GameAssets`에서 다음 자료를 원본 타입 번호순으로 소유한다.

- 전체 188/171개 타입의 기준점. 내장 타입의 초기값도 포함한다.
- 자산 번호 70부터의 프레임 코드와 defaultFrame. 로딩 순서는 116/101개다.
- 기존 `SquidRenderer::Shapes`의 실제 SHP 물리 프레임 헤더. 칸 크기와 픽셀 크기/기준점을 혼동하지 않는다.
- CanonDecoder의 현재 패턴 타입 8개: puzzlePiece 107, bridge 82, island 94, noIsland 157, thunderCannon 131, rainCannon 129, windArcher 140, windBlocker 142. 실제 PE 전역과 대조한다.

배치 모듈의 span은 이 객체의 배열을 참조해야 하므로 객체는 모듈보다 오래 살아야 한다. `GameWorld::BuildSurfaces`도 이 공통 자료의 프레임 코드와 SHP를 사용한다. 패턴 번호 연결은 기존 비패턴 검사가 정확한 타입을 거부하도록 하는 자료 공급이며, 패턴 몸체를 복원한 것은 아니다.

새 콘솔 명령 `--inspect-priest-assets <game-dir> [--cd]`는 창 없이 실제 자산의 프레임 코드/defaultFrame/기준점/물리 프레임 개수와 일반 사제의 픽셀 getter 결과를 JSON으로 출력한다. 사제 GUI/일반 Pop을 활성화하지 않는다.

## 검증 근거와 재현

[`decomp_typehotspot_oracle.py`](../../tools/decomp_typehotspot_oracle.py)는 숫자 속성 구간과 CRT ftol을 **대체 없이** 실행한다. 각 PE 468개, 총 **1,404개**이며 두 x87 정밀도가 일치한다. 0과 음수·정수 경계의 인접 float32·2^24 부근·X/Y 순서·중복 덮어쓰기·다른 축의 기존 값 보존을 포함한다. 실제 ftol은 각 PE 1,872회, 총 5,616회다. 허용된 명령/기준점 8바이트 쓰기·지역 스택·기준 레지스터·x87 균형·assert/OS 0회·PE/내보내기/도구/fixture SHA를 검사한다.

[`TypeHotspotTests`](../../cpppj/tests/TypeHotspotTests.cpp)는 저장된 독립 출력 두 정수를 대조하고, 실제 타입 표의 초기값/대소문자/중복/AST float32/고정 배율과 잘못된 속성 진단을 검사한다. 새 [`근거 기록`](../../cpppj/recovery-typehotspot-evidence.json)과 fixture는 이전 감사 자료와 분리되어 있다. 누적 독립 fixture 입력은 **253,269개**다.

[`cpp_priestassets_smoke.py`](../../tools/cpp_priestassets_smoke.py)는 두 판본의 실제 .type·SHP를 기존 독립 Python parser로 읽어 C++ 자료를 비교한다. 기준점의 기대값은 실제 숫자 속성 구간에서 얻는다. 일반 사제의 모든 실제 물리 헤더/코드/defaultFrame/패턴 전역을 별도 분석 주소에 공급한 뒤 원본 **전체 픽셀 getter/decoder/SHP helper를 정상 반환까지** 실행하여 C++의 여섯 float 비트와 비교한다. SHP 주소는 분석 메모리에 재배치하며 픽셀 데이터는 getter에서 사용하지 않는다. 이 실제 자산 검사는 고정 합성 fixture의 누적 개수와 별도로 기록한다.

실제 자산 대조는 10.78 **116개 타입/3,783개 물리 프레임 개수**, CD **101개 타입/3,167개 물리 프레임 개수**에서 통과했다. 프레임 코드/defaultFrame/기준점/패턴 번호도 일치한다. 두 판본 사제 158의 defaultFrame은 **18**, 타입 기준점은 **(8,2)**이며 실제 기본 프레임 조회 결과는 **(-11,-23,13,4)**, 보정 기준점은 **(11,23)**이다. 이 결과는 원본 전체 getter를 두 x87 정밀도에서 실행해 얻었다. 모든 물리 헤더를 입력으로 공급하되 이 자산 검사의 실제 getter 조회는 기본 프레임 18이다. 다른 프레임/명시 프레임/빈 decoder의 기계어 대조는 기존 `priestshape` fixture를 따른다. 결과는 `extracted/cpp-priestassets-report.json`에 기록했다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name typehotspot
python -X utf8 tools/decomp_typehotspot_oracle.py
python -X utf8 tools/decomp_typehotspot_oracle.py --verify
cmake --build cpppj/build --config Release
python -X utf8 tools/cpp_priestassets_smoke.py
ctest --test-dir cpppj/build -C Release --output-on-failure
```

최종 Release 빌드 경고/오류 **0**, CTest 내부 **358개·실패 0**(110.19초), 전체 원본 근거 감사 **50종 모두 통과**, 두 판본 실제 자산 대조 통과다. 최종 검증 로그는 [LEFT_JOBS.md](../../LEFT_JOBS.md)의 이번 단계 기록을 따른다. 숫자 속성 분석은 **로더 중간 진입에서 다음 속성 경계까지**이며 전체 로더의 파싱/문자열 비교/ABI를 실행한 근거가 아니다. 실제 자산 검사도 전체 MayPlace/일반 사제 Pop/보호막 생성/GUI 플레이를 대조한 근거가 아니다. 다음은 특수 패턴/타입 조합, 일반 사제 Pop·회복 예약·Carrier 상태 검사와 비표면 raw 월드 연결이다.
