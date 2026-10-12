# cpppj raw 배경 자산 공급과 전체 가상 Draw 연결

작업 PC: **VM-W11-CODEX**(`10.0.0.17`), 기록일: **2026-10-12**. AGENTS.md·두 LEFT_JOBS와 현재/마지막 디컴파일 PC 일치를 확인했다. [글리프·돌 버튼 출력](cpp-gump-glyph-draw.md) 다음 단계인 배경의 실제 타입/SHP 공급과 가상 Draw 분배를 완료했다.

`gumpbackgrounddraw-functions.json`의 **9/9/9개** 함수를 읽기 전용으로 내보냈다. 처음 8개 내보내기에 CD Bounds의 실제 몸체를 추가해 최종 아홉 함수로 갱신했다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-12.** Ghidra 프로젝트와 보호 원본에는 변경을 저장하지 않았다.

## 원본 대응과 배경 분배

| 처리 | 10.78 | CD 10.72·추가 10.37 |
| --- | --- | --- |
| 배경 Gump 가상 Draw | `00465c60` | `00494e40` |
| 전체 배경 분배 | `00465880` | `00494a20` |
| 질감 반복 | `00465690` | `00494720` |
| 실제 소프트웨어 단색 fill | `004a2060` | `00423960` |
| 실제 체크 fill | `004a2180` | `00423a80` |
| 클립 설정 | `004a1580` | `00422e10` |
| 사각형 자르기 | `00445290` | `004951a0` |
| CRT 절삭 | `004e49c0` | `004f161c` |
| 기본 Bounds | `004648e0` | `00493670` |

[`GumpBackgroundDraw`](../../cpppj/src/client/GumpBackgroundDraw.h)는 같은 GumpCollection의 현재 Position/배율 크기/Bounds를 사용한다. 패치판의 가상 Draw는 직접 Position 뒤 두 x87 합계/CRT 절삭을 수행하고, CD판은 기본 Bounds에 위임한다. C++에서는 이미 복원한 동일 Bounds 규칙을 재사용한다. 노드가 보관하는 생성 당시 `shape/tileWidth/tileHeight`를 출력 캐시로 쓰지 않는다.

배경 우선순위는 **체크 `08000000` → 단색 `04000000` → 질감 `02000000` → 바탕 없음**이다. 단색/체크는 낮은 BYTE를 색 번호로 사용한다. `GumpPixelTarget::Fill`은 현재 VFX 클립 안에서 오른/아래 경계를 제외하고 쓰며, 체크는 화면 절대 좌표의 **x/y 홀짝이 다른 픽셀**만 채운다. 잘린 사각형에 새 체크 위상을 만들지 않는다.

단색/체크/무배경의 테두리는 **왼쪽 → 위 → 오른쪽 → 아래**의 양 끝 포함 1픽셀 선이다. 왼쪽/위는 파일 팔레트의 기본 흰색, 오른쪽/아래는 기본 회색이다. `SetGumpBackgroundColors`가 기존 `PaletteColorTable.basic[7]/[8]`을 연결한다. pressed는 이 평면 경로의 선 색을 바꾸지 않는다. 각 선은 현재 색 전역을 읽으므로 앞선 콜백의 변경을 다음 선에서 유지한다.

질감은 기존 [화면 기준 격자·2픽셀 명암](cpp-gump-background-reconstruction.md) 규칙을 재사용한다. `PlanGumpTexture`로 전체 반복 함수 한 번의 계획을 제공하며, 각 pass는 **현재 타입/SHP 프레임을 별도로 조회**한다. 바탕 → 왼쪽 → 오른쪽 → 위 → 아래 순서와 들어오는 명암 비트를 유지하는 OR/bright 우선, pressed의 명암 방향 전환을 보존한다. 잘못된 메타 크기 1 이하는 조회 후 clip/map/shape 없는 조기 반환이다.

자산 조회 뒤 현재 클립을 저장하고, 질감 출력의 정상/호스트 예외 반환에서 논리/VFX 클립을 복구한다. 색 표와 앞서 출력한 픽셀은 복구하지 않는다. 원색 VFX 호출은 이전 명암 표를 무시하며, 명암 호출은 마지막으로 설치한 소스 색 치환 표를 사용한다. 클립 콜백은 예외 없이 두 클립 전역을 쓰는 것이 계약이다.

debug는 현재 Bounds에 체크 fill만 호출한다. 이전 debugColor를 인자로 보관하고 **fill 호출 전에 DWORD 증가/감김**을 수행하며 일반 배경·테두리는 실행하지 않는다.

## 실제 자산과 같은 표시 저장소

`GumpBackgroundAssets`는 GameAssets의 실제 타입 번호 70부터 `.type` 논리 프레임 수·flags1·SHP 블록과 추가 헤더의 signed WORD 폭/높이를 연결한다. 물리 레이어 수를 논리 프레임 수로 대체하지 않는다. frameCheck의 `440000` 비트가 허용하는 추가 레이어와 실제 물리 프레임 범위를 확인한다. background의 abstract 인자는 0이다. 없는 타입/내장 무자산 타입·잘못된 프레임은 호스트 예외로 거부한다.

생성 경계의 타입/shape 토큰은 **호스트 타입 번호**이며 PE 주소나 호스트 포인터로 재해석하지 않는다. `BindGumpBackgroundAssets`가 기존 GumpBackgroundCreate의 type/frame 경계와 새 Draw의 pass별 헤더 조회에 같은 안정된 자산 표를 공급한다. 원본 생성자의 판본별 필드 쓰기 순서와 type=0의 미작성 필드 보존은 기존 생성 모듈에 남긴다.

`GumpBackgroundPixelTarget`은 동일 GumpPixelTarget의 현재 클립/선/fill과 실제 SHP 해제 캐시를 사용한다. SHP rect의 기준점 상대 left/top과 DWORD 감김을 유지하며 특별 빈 프레임/투명 skip는 이전 화면을 보존한다. 명암 표는 Screen의 마지막 파일 팔레트 표를 참조할 수 있고, 현재 VFX 색 표는 글리프 출력과 공유한다. 자산 표의 수명/내용은 이 어댑터 사용 중 안정되어 있어야 한다. raw 타입 포인터 메모리 자체를 임의로 수정하는 모델은 이번 실제 자산 어댑터 범위가 아니다.

`BindGumpBackgroundDraw`는 배경 토큰만 선택하고 기존 돌 버튼/다른 파생 Draw 분배를 보존한다. 두 판본의 C++ 검사에서는 실제 자산 공급 → 배경 자식 생성/부착 → UI 수집/CRT 정렬/kind 2 → 배경·부모 선·글자 → 실제 SetPressed의 배경 플래그/글자 위치 전환 → Kill/Cleanup/자식 우선 삭제까지 같은 저장소로 연결했다. 다음 수집에서 삭제된 두 노드가 제외되는 것도 확인했다.

## 독립 원본과 실제 자산 검증

`tools/decomp_gumpbackgrounddraw_oracle.py`는 세 실제 PE의 가상 Draw·전체 분배·질감·소프트웨어 단색/체크·클립·CRT를 격리 x86으로 실행한다. CD의 Bounds와 인라인 타입/SHP 조회도 실제 몸체다. **Position·패치 타입/프레임 조회·VFX 그림/선/색 설치만 명시 외부 경계**이며, 소프트웨어 fill의 픽셀 쓰기를 대체하지 않는다.

세 판본 **각 209개·새 독립 원본 627개**, 두 x87 모드 **027f/037f에서 정상 반환 1,254회**다. 무배경/단색/체크 우선순위·16종 테두리·pressed/명암 네 조합·빈/음수 원점/작은 클립·잘못된 타일·debug 색 감김/큰 float32 폭을 대조했다. 각 모드의 사건·768바이트 실제 픽셀·최종 debug/명암 결과가 모두 일치했다. C++는 저장한 **627개 사건/픽셀 출력 전수**를 재생한다. 누적 독립 원본은 **522,439 + 627 = 523,066개**다.

정상 반환/스택/callee-save·x87 제어/TOP, 내보낸 불연속 실행 범위, 픽셀/스택/클립·VFX 전역/debugColor 외 쓰기 거부, 노드·타입·SHP 입력 불변과 픽셀 앞뒤 보호 바이트를 감사했다. SHA/행/함수 진입·클립/fill/질감·VFX 경계 수 재감사도 통과했다. 단색의 DirectDraw/표면 재잠금 경로와 VFX 그림/선의 전체 래스터 몸체는 이번 독립 실행 범위가 아니다.

`tools/cpp_gumpbackground_assets_smoke.py`는 원본 파일을 읽기만 하는 선택 콘솔 검사다. 실제 PE의 타입 이름 포인터 표, 독립 `.type`/SHP/팔레트 해독과 기존 독립 원본 명암 표를 사용해 다음을 대조했다.

| 자료 | 10.78 | CD |
| --- | ---: | ---: |
| 실제 타입/블록 | 116 | 101 |
| 전체 논리 프레임 헤더 | 2,829 | 2,260 |
| raw 정상/눌림 질감·단색·체크 흐름 | 4 | 4 |
| 전체 출력 픽셀 | 3,072 | 3,072 |
| 불변 파일/존재 상태 | 1,394 | 425 |

합계 **217개 타입·5,089개 헤더·8개 흐름·6,144픽셀**이다. 실제 `fortGump/A00` 메타데이터 **200×146**와 반복 주기 **199×145**를 확인했다. 픽셀 기대값은 실제 PE의 VFX/선 사건 순서에 독립 RLE/명암 표를 합성한 결과이며 C++ 계획/픽셀을 기대값으로 사용하지 않는다. 이 실제 자료의 8개 출력 계획은 저장 fixture 627개의 누적에 합산하지 않는다.

새 C++ 검사 **8개**가 통과했다. 원본 전수 대조 외에 독립 3×3 질감/투명 레이어·특별 레이어 프레임 계약·두 판본 생성 연결·현재 클립/선 색/디버그 콜백·질감 출력 실패 복구·돌 버튼 배경/글자/눌림/삭제 연결을 확인했다. 최종 Release **경고/오류 0**, 전체 CTest 내부 **849개·실패 0**(82.85초, 전체 82.92초)다. SOURCE_MAP 생성 확인 후 내용 차이는 없다.

자료는 `cpppj/recovery-gumpbackgrounddraw-evidence.json`, `cpppj/tests/fixtures/gumpbackgrounddraw-x86.tsv`, `GumpBackgroundDrawTests.cpp`, `GumpBackgroundTestAssets.h`, `GumpTextTests.cpp`, `extracted/gumpbackgrounddraw/`, `extracted/gumpbackgrounddraw-*.log`, `extracted/gumpbackgrounddraw-assets/report.json`에 있다.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name gumpbackgrounddraw
python -X utf8 tools/decomp_gumpbackgrounddraw_oracle.py
python -X utf8 tools/decomp_gumpbackgrounddraw_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 2
ctest --test-dir cpppj/build --build-config Release --output-on-failure
python -X utf8 tools/cpp_gumpbackground_assets_smoke.py
```

## 다음 작업

MenuGump/Form의 생성·현재 가상 처리기 → OS 큐/폴링·overlay·Client 자동 UI 프레임을 연결한다. 기존 돌 버튼 전체 생성/부착/소멸자·배경 생성·기본 도움말 전체 몸체의 독립 PE 대조는 계속 미완료다. 이번 단계는 배경 생성에 실제 자산을 공급하고 C++ 연결을 검사했으며 배경 생성 전체 몸체의 독립 PE 대조를 추가하지 않았다.

**현재 Client/UberGump는 기존 메뉴 어댑터를 사용한다. 새 raw 버튼/배경의 Client 자동 GUI 연결과 게임 내 건설/미션 완주는 아직 불가능하다.** 이번 단계에서 원본/복사본 게임·클론 GUI·소리 장치를 실행하지 않았다. AGENTS.md·보호 원본·dotnetpj/인계는 변경하지 않았고 커밋/푸시하지 않았다.
