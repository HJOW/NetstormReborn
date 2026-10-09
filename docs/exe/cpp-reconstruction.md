# CD판 대조·기계어 검증과 C++ 1차 복원

> 2026-10-05. 기준은 패치판 `originals/Netstorm.exe`(10.78), 참고는 사용자 제공 CD판 `originalCD/NETSTORM.EXE`(10.72).
> 원본 게임 프로세스는 실행하지 않았다. 현재 cpppj는 메뉴/브리핑·저장 미션의 실제 지형/객체·선택/사제 이동·정지/복귀까지 연결했다([월드 검사와 한계](cpp-world-reconstruction.md)). CTest 69개·기존 x86 5365개 기대값이며 건설·전투·미션 완주는 후속이다. 아래 수치와 내용은 당시 1차 복원 기록이다.

> 후속: [타입·그래픽 자산 복원](cpp-assets-reconstruction.md). `.type`·SHP·팔레트와 기본 VFX 그리기를 추가했다. 그 뒤 [설정 계층](cpp-config-reconstruction.md), [타입 표·요새 파일](cpp-fort-reconstruction.md), [Win32 창·화면 장치·입력 큐·영역 배치](cpp-screen-reconstruction.md)를 추가했다. 현재 CTest는 **43개 테스트·4,955개 x86 기대값**을 검사한다. cpppj는 2026-10-05부터 Windows 전용이다. 아래 1차 검증 결과는 당시 작업 기록이다.

> 최신 생성 후속: [생성자 주소 표·base 생성/가상 초기화·free/void Take](cpp-creation-reconstruction.md)를 원본 SID 풀에 연결했다. 현재 CTest **104개 내부 검사**, 제한 x86 입력 행 **36,510개**이며 생성자 주소 표 359행은 별도다. 파생/공간 수명·실제 GameWorld 연결은 남았다. 위의 43/69개 등의 수치는 앞선 단계의 기록이다.

> 2026-10-09 최신: [배치의 최종 표면 관계·특수 지역·거부](cpp-canon-relations-reconstruction.md). 새 독립 x86 6,921개·누적 341,410개, 최종 구간/실제 helper/성공·실패 에필로그 정상 반환·함수 대체 0, 실제 일반/3×3 배치 모듈 합성, CTest 내부 417개·감사 61종 통과. 전체 MayPlace 진입부터 반환까지 독립 대조/실제 자산 전수/GUI 건설·미션 완주는 후속이다. 마지막 디컴파일 PC: HJOW-Athlon, 2026-10-09.

> 2026-10-09 앞 단계: [배치 모양의 주변 표면 탐색·권한 누적](cpp-canon-surrounding-reconstruction.md). 새 독립 x86 1,572개·누적 334,489개, flag 8 finder 전체 정상 반환/주변 구간·Player 진입만 대체, 실제 Player 조회→EndShape 연결, 동적 raw/지도/프레임 코드 표 검증, CTest 내부 411개·감사 60종 통과. 최종 표면 관계/특수 지역/거부·전체 MayPlace/실제 플레이 완주는 후속이다. 마지막 디컴파일 PC: HJOW-Athlon, 2026-10-09.

> 2026-10-09 앞 단계: [Player 작업장·그래프 기준점 조회](cpp-player-anchor-reconstruction.md). 새 독립 x86 4,794개·누적 332,917개, 전체 조회/거리/그래프/contained 정상 반환·임시 메모리만 대체, 실제 작업장 장부/후보 권한/지형 연결, CTest 내부 406개·감사 59종 통과. 주변 권한/최종 관계·전체 MayPlace/실제 플레이 완주는 후속이다. 마지막 디컴파일 PC: HJOW-Athlon, 2026-10-09.

> 2026-10-09 앞 단계: [일반 배치 후보 권한·방향 소유 관계](cpp-canon-permission-reconstruction.md). 새 독립 x86 16,890개·누적 328,123개, 실제 3×3 받침/미리보기/finder/지형의 권한 누적 연결, CTest 내부 401개·감사 58종 통과. Player 작업장·그래프 조회 몸체/주변 권한/최종 표면 관계는 필수 경계이며 전체 MayPlace/건설·전투·미션 완주는 후속이다. 마지막 디컴파일 PC: HJOW-Athlon, 2026-10-09.

> 2026-10-09 앞 단계: [일반 타입 지형·지역 누적과 권한 경계](cpp-canon-terrain-reconstruction.md). 새 독립 x86 11,220개·누적 311,233개, 실제 3×3 받침/미리보기/finder/지역 지도 연결, CTest 내부 395개·감사 57종 통과. 후보 권한·주변 관계·최종 표면 소유 관계는 필수 경계이고 전체 MayPlace/건설·전투·미션 완주는 후속이다. 마지막 디컴파일 PC: HJOW-Athlon, 2026-10-09.

> 2026-10-09 앞 단계: [일반 타입 후보 충돌·실제 패턴 연결](cpp-canon-collision-reconstruction.md). 새 독립 x86 22,680개·누적 300,013개, 실제 받침/미리보기/finder와 후속 모양 생략 검사, CTest 내부 389개·감사 56종 통과. 원본 후보 중간 구간 관찰과 C++ 합성을 구별하며 전체 MayPlace/건설·전투·미션 완주는 후속이다.

> 2026-10-09 앞 단계: [일반 타입·패턴 로컬 미리보기](cpp-canon-preview-reconstruction.md). 새 독립 x86 4,860개·누적 277,333개, 실제 decoder 범위/3×3 받침 연결, CTest 내부 382개·감사 55종 통과. 모양/decoder 진입과 후반 충돌 구간 대체를 구별하며 전체 MayPlace/건설·전투·미션 완주는 후속이다.

> 2026-10-09 앞 단계: [일반 타입·패턴 배치 접두](cpp-canon-placement-reconstruction.md). 새 독립 x86 3,360개·누적 272,473개, 실제 두 판본 자산 접두 8,072개 일치, CTest 내부 377개·감사 54종 통과. 전체 MayPlace/건설·전투·실제 플레이 완주는 후속이다.

## 디컴파일을 더 확실하게 만드는 방법

최신 후속은 [일반 공간 탐색과 다리 삭제 훅 연결](cpp-rawfinder-reconstruction.md)이다. 세 판본의 실제 Begin/Next·커서 전체·동적 필드와 삭제 훅 연결을 새 x86 1,776개로 대조했다. 전체 월드/실제 삭제·낙하·소리는 후속이며 최신 전체 검사 수는 [cpppj README](../../cpppj/README.md)를 따른다. 아래 수치는 각 당시 작업 기록이다.

기존 A~D 등급은 함수가 존재하고 경계가 맞는지에 관한 근거다. C++ 복원에는 반환값·부호·자료형·인자 순서까지 확인해야 한다. 이번에는 다음 세 가지를 실제로 적용했다.

1. **인자·`ret N`·본문까지 대조해 오버로드를 구분한다.** 자동 매칭 결과를 그대로 믿지 않고, 같은 일을 하는 함수를 찾아 검토 목록에 기록한다. 다른 판본의 함수는 의미가 같아도 용량·경계 조건이 다를 수 있다.
2. **원본 x86 기계어의 결과와 C++ 결과를 비교한다.** Unicorn 가상 메모리에 PE 바이트·가짜 객체·스택만 올려 선택한 함수부터 정상 반환까지 에뮬레이션한다. 게임 진입점이나 OS API로 들어가면 실패한다. 기대값은 디컴파일 C를 다시 구현한 Python 모델에서 얻지 않는다. CPU 에뮬레이션 API의 근거는 [Unicorn 공식 안내](https://www.unicorn-engine.org/docs/tutorial.html)다.
3. **검증한 구조체 필드와 원형만 Ghidra에 적용한다.** `FrameCode32`, `RiftTypeFrameView32`, `ConfigTextPrefix32`를 정의하고 ECX·ESP 인자 위치를 직접 지정했다. Ghidra도 구조체 정보를 보강하면 디컴파일이 개선된다고 설명한다([공식 입문 자료](https://ghidra.re/ghidra_docs/GhidraClass/Beginner/Introduction_to_Ghidra_Student_Guide.html)). 이전에 실패한 전체 Parameter ID 일괄 추정을 다시 사용한 것은 아니다.

추가로 실제 아카이브 전체를 기존 Python 추출기와 바이트 단위로 비교했다. 데이터 읽기 자체가 어긋나면 이후 게임 화면·규칙 비교가 무의미해지므로 먼저 확인한다.

## 확인한 오대응과 판본 차이

| 항목 | 패치판 | CD판 | 판정 |
|---|---|---|---|
| 번호 검색, 실패 시 -1 | `0049a9a0`, `ret 12` | `00444350`, `ret 12` | 동일 의미 |
| 번호＋플래그 마스크 검색, 실패 시 -1 | `0049a9e0`, `ret 16` | `00444410`, `ret 16` | 동일 의미 |
| 방향＋변형＋정확한 플래그 검색 | `0049aa30`, `ret 12` | `00444460`, `ret 12` | 동일 의미 |
| 자동 매칭의 이전 후보 | `0049a9e0` | `00444350` | 다른 오버로드. 금지 목록에 기록 |
| 커널 용량 | `00471950`: **39,999** | `004ed500`: **3,999** | cpppj는 패치판 값 사용 |
| 설정 원시 값 assert 경계 | `00440150`: 8,191 | `0042ab70`: 4,095 | cpppj는 동적 문자열 사용 |

마스크 검색의 플래그는 `movsx`로 **signed 8비트 → 32비트** 확장한다. `0x80`인 플래그에서 `0x100`, `0x80000000` 마스크가 적용되는 입력까지 검사하여, C++ 기본 `char` 부호에 따라 동작이 바뀌지 않게 했다.

CD판의 `NETSTORM.VER` 파일에는 `10.37`이 남아 있고 `D/setup.cfg`에는 패치판의 `gamemaster`·`gameminor` 키가 없다. 그 파일 하나로 실행 파일 판본을 재판정하지 않았다. 보고서와 타입 적용 스크립트는 **실행 파일 SHA-256**을 식별 기준으로 사용한다.

검토 목록은 [recovery-manifest.json](../../cpppj/recovery-manifest.json), 실행 근거는 [recovery-evidence.json](../../cpppj/recovery-evidence.json)이다. 자동 대응에 검토된 다섯 쌍을 먼저 적용하며 바이너리나 목록의 SHA-256이 달라지면 재검증하도록 실패한다. 수동 정답은 홀드아웃 평가에 포함하지 않는다.

## 생성한 C++ 소스와 범위

| 소스 | 복원한 내용 | 아직 없는 내용 |
|---|---|---|
| `o/BaseFile.cpp` | TAFF 헤더·이름 인덱스·플래그별 XOR, 기본 디스크 → 아카이브 → 보조 디스크 읽기 | 파일 쓰기, 존재 검사 결과 코드, CD 경로 자동 탐색, 실패한 절대 경로 재조회 |
| `o/Config.cpp` | XOR 파일 감지·서명, 첫 일치 키, 공백·따옴표·백틱·주석, 끝에 이어 붙이기 | 객체 스택·치환·설정 저장 |
| `o/Xlat.cpp` | 줄 첫 글자 구분, 중복 원문은 마지막 번역, 영어·미등록 원문 폴백 | 언어 변경 UI·한국어 파일 자동 연결 |
| `o/RiftType.cpp`, `TypeParser.cpp`, `TypeLoadOrder.cpp` | 프레임 코드 검색 3개, `.type` 자산 정의·코드 생성·판본별 로딩 목록 | 게임 플래그·생성자·ID 연결, 난수 선택·방향 폴백 |
| `o/Kernel.cpp`, `o/BaseProcess.cpp` | 슬롯 등록·삭제, 매 프레임 실제 슬롯 순서 실행 | 원본 SID 연결·프로세스 파생 클래스·디버그 출력 |
| `o/GameClock.cpp` | 밀리초 래핑, 중첩 정지·재개, 프레임 시각 고정 | 원본 초기 시간 보정·FPS 통계·OS 시계 공급 |
| `client/ClientMain.cpp` | `1/maxFPS`, 1ms 눈금에서 14ms 기본 간격 계산 | WinMain·입력·화면 루프 |
| `client/VFXDraw.cpp`, `Screen.cpp`, `GameAssets.cpp` | SHP·RLE·투명 마스크·기본 클리핑·팔레트·타입 연결 | 창 장치·색 변환·그림자 효과·확대·반전 |
| `o/OriginalText.cpp`, `platform/Console.cpp`, `app/main.cpp` | 신규 UTF-8 경계·바이너리 콘솔·자산 검사 명령 | 그래픽·오디오·플레이 화면·MCP |

자료형과 변수 이름은 복원 목적에 맞춰 다시 지었다. 디컴파일 의사 코드를 확장자만 `.cpp`로 바꾼 결과가 아니다. 전체 함수 5,549개를 C++로 복원했다고 볼 수 없으며 원본의 파일 오프셋·함수 주소를 호스트 포인터로 사용하는 코드도 없다. `BaseProcess`는 실행 인터페이스만 옮겼다. `Kernel::Add`의 소유권 API와 빈 키 거부 등 신규 API 차이는 소스 주석에 명시했다.

`ConfigText`·`XlatTable`의 직접 텍스트 입력은 UTF-8이다. 원본 자산은 Windows-1252에서 UTF-8로 변환한다. `FromBytes`에서 UTF-8 파일을 읽으려면 BOM을 사용한다. BOM 없는 신규 UTF-8 파일은 직접 UTF-8 텍스트 생성 API를 사용한다. 글꼴 렌더링은 아직 구현하지 않았다.

## 재현

저장소 루트에서 실행한다. 정밀 디컴파일이 없는 PC에서는 먼저 `tools/ghidra/refine_all.ps1`을 실행한다.

```powershell
# 선택적 분석 의존성. C++ 빌드에는 Python·Unicorn·Ghidra가 필요하지 않다.
python -m pip install --target extracted/oracle-python -r tools/requirements-decomp-oracle.txt
python tools/decomp_oracle.py
python tools/decomp_refine.py
powershell -NoProfile -ExecutionPolicy Bypass -File tools/ghidra/recover_types.ps1

cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
python tools/cpp_recovery_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --inspect-data originals
```

자료형 적용 결과는 `extracted/refined/originals/typed-core.c`, `extracted/refined/originalCD/typed-core.c`다. **정밀 Ghidra 프로젝트는 읽기 전용으로 열고 수정 내용을 버린다.** 기존 `raw.c`·기본 디컴파일은 보존한다. `decomp_refine.py`만 기존처럼 대응표·등급·주석 있는 최종 C를 재생성한다.

기계어 기대값 TSV는 `cpppj/tests/fixtures/original-x86.tsv`에 포함되어 있어 원본 바이너리 없는 환경에서도 C++ 회귀 검사를 실행할 수 있다. 생성 도구는 저장소 고정 시드로 같은 결과를 만든다.

## 검증 결과와 한계

- C++ Release 빌드 경고·오류 0. CTest의 실행 파일 안에서 **10개 테스트** 통과.
- 두 판본 기계어를 대조한 **1,806개 입력**: 프레임 검색 3종 각 512개, 설정 키 위치·원시 값 270개. C++ 결과도 모든 기대값과 일치.
- 실제 TAFF: 패치판 **246개**, CD판 **258개**, 총 **504개 엔트리**의 복호화된 내용 전체 일치. 원본 아카이브 SHA-256 유지.
- 독일어 번역 키 수: 패치판 **774개**, CD판 **759개**. 각 판본 실제 번역 20개와 임시 디스크 파일 우선 조회 검증.
- 자료형 적용 디컴파일: 두 판본 각 5개 완료. `object->frameCodes`, `object->frameCount`, `FrameCode32::flags`로 읽힌다.
- 자동 대응은 2,493 → **2,495쌍**, 약 45%다. 수동 앵커를 제외한 홀드아웃은 기존대로 **195/198 = 98.5%**. 대응 개수의 증가를 전체 코드 정확도 향상률로 해석하지 않는다.

에뮬레이션에서 CRT `toupper`·`strnicmp`는 **ASCII 로케일만** 대체했고 assert 오류 보고 UI도 대체했다. 원본 CRT의 다른 로케일, 전역 초기화 순서, 예외 UI, 그래픽·소리·실제 게임 전체 실행은 검증 범위에 없다. 커널·시계는 정적 코드 대조와 단위 검증 수준이며 기계어 기대값 1,806개에 포함하지 않았다.

`.type` 자산 정의·SHP 읽기·팔레트·기본 그리기는 [후속 작업](cpp-assets-reconstruction.md)에서 복원했다. 다음 대상은 `.fort` 로더와 Config 치환, 게임 타입의 비트·생성자·ID·파생 프로세스, 화면 장치·Renderer·UserInput이다. 게임 전체 복원을 위해서는 각 클래스의 생성자·소멸자·가상 함수 표와 읽기/쓰기 오프셋을 함께 추적하고, 추가 순수 함수부터 이 검증 방식에 넣는다.
