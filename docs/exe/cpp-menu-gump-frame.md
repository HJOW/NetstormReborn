# cpppj MenuGump 크기·확장 프레임·controlled 메뉴 스택

작업일 **2026-10-12**, PC **HJOW-Athlon / 192.168.0.94**. AGENTS.md와 두 LEFT_JOBS를 확인했다. 같은 PC에서 전체 정밀 디컴파일을 재생성한 다음, 이번 함수 **13/13개**를 정밀 프로젝트에서 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-12.**

## 복원한 원본 몸체

| 동작 | 10.78 | CD 10.72 |
| --- | --- | --- |
| 폭/높이 지정 | `00475f20` / `00475fb0` | `00452260` / `004522f0` |
| 가상 재그리기 요청 | `00476050` | `00452380` |
| 항목 위치 조회 | `00476630` | `004528d0` |
| 확장 idle / 확장 시작 | `00476700` / `004767d0` | `00452a70` / `00452b00` |
| 현재 메뉴 꺼내기 / 조회 / 항목 수 | `00475c00` / `00475c40` / `00475c50` | `00451070` / `004510b0` / `004510c0` |
| 현재 메뉴 부착 / 객체 부착 | `00475ca0` / `00475cf0` | `00451310` / `00451360` |
| 실제 idle 등록 / CRT 절삭 | `00465330` / `004e49c0` | `00494380` / `004f161c` |

`MenuGumpNode`는 같은 `GumpBackgroundNode` 기본 저장소를 상속한다. 크기·배율·트리·수명 필드가 복사본으로 분리되지 않는다. 이번 파생 필드는 expanding(+b8), 항목 폭·목표 폭/높이(+170/+174/+178), 40개 offset(+17c), 메뉴 flags·항목 수(+220/+224), 장식/제목(+b88/+b8c)이다. 전체 0xb90 메뉴 객체의 생성·항목 문자열 배열을 구현했다고 간주하지 않는다.

폭/높이는 먼저 부모의 기본 크기와 배율 크기를 쓴다. fancy 비트가 있으면 현재 장식의 가상 Push 뒤 **현재** 장식·제목·부모 크기를 다시 읽는다. 장식 폭은 부모 폭+32, 장식 높이는 제목 높이+부모 높이+48이다. 장식의 기본 크기를 float32로 저장한 뒤에도 배율 곱에는 저장 전 중간값을 사용한다. 장식 Attach는 `(-16, float(signed DWORD(_ftol(-제목 높이-32))), this, 0)`이다.

재그리기 요청은 메뉴 flags 대신 현재 장식 포인터를 검사한다. 장식 가상 Redraw 뒤 기본 Gump Redraw를 호출한다. 실제 픽셀 Draw를 대신하는 함수가 아니다.

확장 시작은 방향에 따라 폭이나 높이 한 축을 0으로 만들고 실제 `GumpLifetime::SetIdle(true)` 뒤 expanding을 1로 쓴다. idle은 가상 Push→현재 높이 지정→현재 폭 지정→가상 Pop 순서다. 목표 크기에 도달한 다음 호출에서 expanding만 0으로 바꾸며 idle 등록은 유지한다. 최초 x87 비교는 NaN도 확장 분기로 들어간다. Push 이후 축 비교에서 **10.78은 ordered 미만만 크기를 지정하고 CD는 unordered도 지정한다.** 디컴파일 C의 단순 `<` 표현만으로 이 차이를 판단하지 않았다.

controlled 메뉴 표는 20칸이며 0번을 예약한다. 부착 콜백 후 현재 index를 다시 읽어 해당 칸을 제거한다. 패치는 칸 제거→index 감소, CD는 index 감소→이전 칸 제거 순서를 유지한다. 객체 부착은 가상 +3c에 원본 객체 SID와 현재 등록표 루트를 전달한다. 화면 상대 이동으로 해석하지 않는다. 예약 칸 제거·범위 밖 index·없는 노드 같은 무효 호스트 입력은 접근 전에 예외로 거부한다.

## 연결과 독립 검증

`BindMenuGumpFrame`은 실제 메뉴의 가상 +8/+30만 기존 `GumpFrameHooks`에 연결하고 다른 타입의 분배를 보존한다. 같은 수명 표에 등록한 메뉴가 `GumpFrame::Idle`을 통해 크기 갱신과 확장 완료를 수행한다. controlled 메뉴의 부착 경계에 기존 `GumpTree::Attach`를 공급한 검사에서는 관리 표에서 빠진 메뉴가 같은 트리에 남고 `GumpCollection`에 수집된다. 원본 MenuGump의 추가 부착 보정 몸체는 이 경계에 아직 연결하지 않았다.

`tools/decomp_menugump_oracle.py`는 두 실제 PE의 내보낸 명령 범위와 CRT/idle 등록을 실행한다. 가상 Push/Pop/Attach/객체 Attach/Redraw 및 기본 Gump Redraw만 명시 경계다. **판본별 154개, 합계 308개** 독립 입력에서 두 x87 제어 워드 027f/037f의 기록이 같았다. 최상위 **정상 반환 696회**, 스택/callee-save·x87 제어/TOP·실행/쓰기 범위·선택하지 않은 객체 바이트·가상 표·idle 뒷칸 보호를 확인했다.

C++는 원본 fixture의 입력·콜백 당시 상태·최종 55 DWORD×세 노드·전체 메뉴 표·idle 칸·루트·반환값을 전수 재생한다. 폭/배율/장식 조합, 큰 float32 폭, 가로/세로 항목 위치, 확장 방향·미확장·완료·NaN, 장식 포인터와 플래그의 불일치, Push 뒤 크기/장식/제목 변경, 부착 뒤 index 변경을 포함한다. 새 독립 원본 누적은 **523,066 + 308 = 523,374개**다.

자료: `cpppj/menugump.json`, `cpppj/tests/fixtures/menugump-x86.tsv`, `MenuGumpTests.cpp`, `tools/ghidra/menugump-functions.json`, `extracted/menugump/`, `extracted/menugump-*-export.log`, `extracted/menugump-build.log`.

새 C++ 검사 **5개**가 모두 통과했다. 최종 Release **경고/오류 0**, 전체 CTest 내부 **854개·실패 0**(130.73초, CTest 전체 130.79초)이다. 결과 로그는 `extracted/menugump-focused.log`, `extracted/menugump-ctest.log`에 있다. SOURCE_MAP의 MenuGump 파일 존재 상태를 갱신했다.

재현은 `menugump-functions.json`의 판본별 주소 목록을 `run_script.ps1 -Refined -Script ExportCreation.java -ScriptArgs`에 넘겨 내보낸 뒤 다음을 실행한다.

```powershell
python -X utf8 tools/decomp_menugump_oracle.py
python -X utf8 tools/decomp_menugump_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 2
cpppj/build/bin/Release/netstorm_tests.exe --filter MenuGump
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

## 후속

메뉴 가변 인자 생성·항목 작성/레이아웃·StyleText·fancy 생성과 소멸자, 가상 Attach 보정·입력/통지 처리기를 이어서 복원한다. 이후 OS 큐/폴링·overlay·Client 자동 UI 프레임을 연결한다. 돌 버튼 전체 생성/부착/소멸자·배경 생성·기본 도움말 전체 몸체의 독립 PE 대조도 남았다. **Client/UberGump의 실제 메뉴는 여전히 기존 어댑터이며 새 raw 메뉴의 자동 GUI 연결 및 게임 내 건설/미션 완주는 미완료다.**

원본/복사본 게임·클론 GUI·소리 장치를 실행하지 않았다. 원본 두 바이너리 SHA-256이 기존 기록과 같고 AGENTS.md·보호 원본·dotnetpj를 변경하지 않았다. 커밋/푸시하지 않았다.
