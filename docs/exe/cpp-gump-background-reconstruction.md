# 돌 버튼 배경의 질감 반복·명암 변환표 복원

2026-10-10, **HJOW-Athlon**. AGENTS.md·두 LEFT_JOBS 문서를 다시 읽었고 작업 시작 트리는 깨끗했다. 현재 PC와 마지막 디컴파일 PC가 일치했다. 새 `gumpbackground` 목록 **5/6/6개**를 읽기 전용 Ghidra로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10.** 원본 게임·복사본·analyzeManager를 실행하지 않았다.

[앞 단계의 부모 버튼](cpp-gump-visual-reconstruction.md)에 배경 자식의 질감 경로를 연결했다. 버튼의 상대 원점부터 반복하던 질감을 화면 기준 격자로 고쳤고, 단색 1픽셀 테두리를 원본의 색 번호 변환표를 쓰는 2픽셀 질감 테두리로 바꿨다. 눌림은 테두리 명암 방향만 바꾸며 바탕 전체를 어둡게 하지 않는다.

## 원본 주소와 계약

| 대상 | 10.78 | CD/10.37 | 복원/관찰 범위 |
| --- | --- | --- | --- |
| ColorSort 생성 | `00439de0` | `00458a90` | 전체 몸체 실행, RGB 정규화 입력 |
| GUI 변환표 생성 | `0043c010` | `00496530` | 전체 몸체 실행, 밝음/어두움 두 표 복원 |
| 밝음 후보 제외 | 생성 함수 내부 | `00496510` | 229~245 제외, 실제 내부 호출 실행 |
| 배경 그리기 | `00465880` | `00494a20` | 질감 경로의 전체 pass 분기/순서 |
| 질감 반복 | `00465690` | `00494720` | 전체 정수 격자·명암 우선·clip 저장/복구 |
| 사각형 자르기 | `00445290` | inline / `004951a0` | 실제 몸체 실행 |
| 배경 자식 생성 | `004655d0` | `00494640` | 앞 단계 전체 부모 생성 관찰과 이번 정적 분석의 자식 크기/부착 계약 |
| 배경 자식 Draw | `00465c60` | `00494e40` | 정적 분석; 전체 좌표 변환/가상 수명은 후속 |

[`GumpBackground`](../../cpppj/src/client/GumpBackground.h)은 질감 출력 계획과 실제 8비트 합성을 분리한다. [`PaletteShade`](../../cpppj/src/client/PaletteShade.h)는 논리 팔레트에서 두 색 번호 변환표를 만든다. [`Screen`](../../cpppj/src/client/Screen.h)은 파일 팔레트를 읽은 뒤 표를 저장하고 일시 `SetPalette`/모드 적용 중에는 유지한다. [`UberGump`](../../cpppj/src/client/UberGump.cpp)의 모든 현재 돌 버튼이 이 표와 실제 `fortGump/A00`의 메타데이터를 사용한다. 목록/패널의 기존 Tile/Bevel은 이번 범위에서 변경하지 않았다.

부모 `(L,T,R,B)`의 배경 자식은 상대 `(1,1)`, 폭 `R-L-2`, 높이 17이다. 기본 플래그는 `03e00000`, 눌림 비트는 `00040000`이다. 현재 어댑터에서는 배경→부모 선→글자를 합성한다. 원본 전체 Gump 트리의 그리기/입력/갱신 수명까지 복원한 것은 아니다.

## 질감과 테두리

주기는 SHP 메타데이터의 signed short 폭/높이에서 각각 1을 뺀 값이다. 실제 A00 메타데이터는 **200×146**, 반복 주기는 **199×145**다. 압축 픽셀 이미지 크기로 주기를 추측하지 않는다. VFX 기준점에 프레임 rect의 left/top을 더하여 그림을 놓는다.

각 축의 시작점은 `(L/주기)*주기`, 마지막 점은 `((R-L)/주기 + 2 + L/주기)*주기`다. 나눗셈은 음수에서도 0 방향 절삭이고 여분의 마지막 타일까지 포함한다. 실제 출력은 요청 영역으로 자르며 마지막에 기존 clip을 복구한다. 메타 폭/높이가 1 이하이면 clip/map/draw를 호출하지 않는다.

호출 순서는 **바탕 → 왼쪽 → 오른쪽 → 위 → 아래**다. 테두리는 각 2픽셀이며 요청 영역을 기존 clip 안으로 자른 뒤 질감을 다시 그린다. 정상은 왼쪽/위에 밝은 표, 오른쪽/아래에 어두운 표를 적용하고 눌림은 방향을 바꾼다. 들어오는 명암 비트를 지우지 않고 OR하며 둘 다 켜져 있으면 밝은 표가 우선한다. 마지막 아래 pass가 모서리를 덮는 순서도 보존한다.

안전 C++ API는 정상 순서 rect/clip을 받으며 음수·빈 영역을 허용한다. 100만 회를 넘는 출력 계획, 감겨서 역전하는 경계, 정수 넘침으로 끝나지 않는 반복은 거부한다. 단색/체크 fill·flat line·debug 경로는 아직 지원하지 않는다.

## 두 명암 표

논리 PALETTEENTRY의 R/G/B를 원본 double 계수 `1/255`로 정규화한다. 밝음 목표는 RGB 각각 **1.5배, 최대 1.0**, 어두움 목표는 각각 **0.6000000238418579배**다. 목표 RGB는 double로 저장한다. 거리 `abs(R차)+abs(G차)+abs(B차)`를 비교하고 최솟값만 float32로 저장하므로 단순 정수 거리/항상 첫 동률 규칙으로 바꾸지 않는다. 밝음에서는 원본 번호 **229~245**를 후보에서 제외하고 어두움에서는 허용한다. 예약 바이트는 RGB에 포함하지 않는다.

실제 `originals/d/!color.dat`의 두 캐시 표(밝음 offset 768, 어두움 offset 512)는 기본 팔레트로 실제 원본 생성 함수를 실행한 결과 **512바이트 전부와 같다**. 현재 C++는 파일 로딩마다 표를 직접 계산한다. 원본 캐시 파일 읽기/실패 시 쓰기·나머지 일곱 변환표·ColorSort 파생 정렬·`paletteDirty` 소비/재생성 시점은 후속이다. 합성 날씨 팔레트에서 표를 갱신하는 현재 화면 어댑터를 원본 전체 캐시 수명과 동일하다고 주장하지 않는다.

## 검증

[`decomp_gumpbackground_oracle.py`](../../tools/decomp_gumpbackground_oracle.py)는 세 PE의 전체 ColorSort 생성/변환표 생성과 배경/질감/clip 몸체를 제한 x86으로 실행했다. 독립 저장 입력 **177개**: 판본마다 팔레트 **3개**, 배경 **56개**다. 두 x87 정밀도 `027f`/`037f`에서 결과가 같았다. 생성/변환표 각각 **18회**, 배경 **336회**, 최상위 정상 반환 **372회**와 내부 질감 진입 **1,074회**를 확인했다. 저장 입력 누적 **455,755개**, 이전 85종과 새 근거 **86종 모두 통과**다.

명암 생성은 스텁 없이 전체 몸체를 실행하고 다른 일곱 출력 표의 쓰기도 감사한다. 복원/fixture 대조는 두 RGB 명암 표에 한정한다. 배경에서는 Patch 타입/프레임 조회와 clip/map/shape 장치 출력만 명시 경계다. 분기/정수 반복/clip 계산은 원본 명령이다. 입력 불변·clip 복구·실행/쓰기 범위·전체 진입/정상 반환·스택/보존 레지스터/x87·경계 수·입력 순서·원본/몸체/도구/fixture SHA를 검사했다. 게임/OS 호출은 0이다.

[`GumpBackgroundTests.cpp`](../../cpppj/tests/GumpBackgroundTests.cpp)의 새 검사 **5개**는 177개 관찰, 명암 후보 제외/예약 바이트, 화면 격자·테두리 전환·투명/프레임 오프셋·clip 출력, 파일 갱신/일시 팔레트 유지, 안전 입력을 검사한다. 기본 CTest는 원본 PE·Python·Ghidra 없이 fixture를 대조한다.

[`cpp_gumpbackground_smoke.py`](../../tools/cpp_gumpbackground_smoke.py)는 원본 자료와 두 검사 사본으로 클론 창만 실행했다. **1024×768·800×600·640×480**, 정상/눌림/밖으로 취소/안내 창/비활성 누름의 **18상태·18화면**에서 배경 계획 **106개**·부모 계획 **106개**를 실제 10.78 몸체로 따로 관찰했다. SHP/글꼴을 독립 해독하여 **142,676픽셀**을 대조했다(명암/질감 대상 131,546픽셀 포함, 둘을 더한 수가 아니다). 원본 외곽선 번호 15와 사본 37/73, 비활성 누름 후 화면 동일을 확인했다. 이 212개 화면 계획은 저장 fixture 177개 누적에 합산하지 않는다. 투명/부모 모서리 등 배경이 덮지 않는 픽셀과 전체 원본 창의 일치를 증명하는 검사는 아니다.

Release 경고/오류 **0**, CTest 내부 **555개·실패 0**(전체 139.26초). 기존 메뉴 **54상태**·배경 **774,432픽셀**과 두 판본 GIF, 합성 사본 날씨 **38상태·23화면** 회귀 통과. 보호 원본 **1,392개**·CD **425개**와 허용 두 설정 복구, AGENTS.md/LEFT_JOBS.dotnetpj.md 불변을 확인했다(총 **1,821개**의 바이트/존재 해시 대조). 검사 산출물은 Git 제외 `extracted/gumpbackground-*.log`, `gumpbackground-audits.json`, `cpp-gumpbackground-smoke/`에 있다.

## 다음

목록/패널 등 나머지 GUI 질감·색 소비자 → StyleText/전체 Gump 트리·팔레트 갱신 표시 소비를 이어간다. 없는 네 날씨 COL의 원본 RGB·tint 후속·raw 번개 부모·실제 전체화면도 후속이다. 건설·경제·전투·AI·승패가 남아 **미션 완주는 아직 불가능하다**. 현행 목표는 한국어 3차, outpost/LAN 4차, 화면 요구사항/MCP 5차다. AGENTS.md·dotnetpj·보호 원본 변경 및 커밋/푸시 없음. [최신 인계](../../LEFT_JOBS.md).

## 재현

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name gumpbackground
python -X utf8 tools/decomp_gumpbackground_oracle.py
python -X utf8 tools/decomp_gumpbackground_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
python -X utf8 tools/cpp_gumpbackground_smoke.py
python -X utf8 tools/cpp_menu_smoke.py
python -X utf8 tools/cpp_weather_smoke.py
```
