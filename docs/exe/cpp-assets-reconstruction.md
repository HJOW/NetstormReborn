# cpppj 타입·그래픽 자산 복원

> 2026-10-05. 패치판 10.78을 기준으로 C++ 로더를 복원하고 CD판 10.72와 대조했다.
> 원본 게임 프로세스는 실행하지 않았다. 원본 파일은 읽기만 했다. 게임 창·전투·전체 루프는 후속 작업이다.

`cpppj`는 이제 TAFF의 `.type` 정의와 디스크의 `_shapes.shp`를 연결하여 원본 팔레트·투명도를 가진 이미지를 내보낸다. 기본 8비트 그리기에는 원본의 pane 원점 이동과 양 끝을 포함하는 클리핑 규칙을 적용했다.

## 복원한 소스

| 소스 | 범위·근거 |
|---|---|
| `o/TypeParser.cpp`, `RiftType.h` | `.type` 숫자·문자열 속성, 생성자 수식어·플래그 이름, 클러스터·GIF 참조. `0049c3b0` ↔ CD `004460f0`의 프레임 코드·특수 인덱스 생성 규칙 |
| `o/TypeLoadOrder.cpp` | 패치 `00540dd0`의 116개·CD `0051c6f8`의 101개 이름 포인터 배열. 디렉터리 정렬 순서와 구분 |
| `client/VFXDraw.cpp` | VFX 1.10 블록 상대 오프셋·공유 프레임·헤더·RLE 읽기, 투명 마스크, 기본 8비트 합성 |
| `client/Screen.cpp` | `004a4850` ↔ CD `00424760`: 776바이트 COL의 RGB, 1,024바이트 Windows RGBQUAD의 BGRX 읽기 |
| `client/GameAssets.cpp` | 새 연결 계층. VFS에서 타입·SHP·팔레트를 읽고 원본 레이어 인덱스를 유지 |
| `platform/Bitmap.cpp` | 새 검토용 출력. BMP V4의 위에서 아래 방향·BGRA·알파 마스크 |

파일 이름과 `typename`은 따로 보존한다. `dude.type`의 이름은 `Man`이다. 속성 조회는 ASCII 대소문자를 구분하지 않고 마지막 지정값을 사용한다. `default`·`help`·`gumpframe`·`baseframe`도 원본 처리 순서를 유지한다. 명시적인 `help`가 나온 뒤에는 다음 `default`가 도움말 프레임을 덮어쓰지 않는다.

클러스터 `N00`의 변형은 `P`이고 `AA01`의 변형은 `A`다. 번호는 정상 `_ftol` 범위에서 버림 변환한 뒤 하위 8비트를 저장한다. 프레임 플래그 `fringe`·`suck`·`rim`·`lit`·`unlit`·`cracked`·`hard`는 7개 비트에 대응하며 그 외 단어는 코드 비트에 반영하지 않는다.

SHP 프레임 번호는 **레이어 × 클러스터 수 + 클러스터 순번**이다. GIF 이름·GIF 번호는 빌드용 참조로 보존하며 런타임 프레임을 재정렬하는 데 쓰지 않는다. RLE의 건너뛰기는 별도 투명 마스크에 저장한다. 팔레트 번호 **0·255도 불투명 색**일 수 있다.

손상된 테이블·프레임 경계·행 폭 초과·잘린 run은 예외로 보고한다. 다음 레코드까지 읽어 잘린 프레임을 정상으로 처리하지 않는다. 디코딩 할당 상한 64M 픽셀은 새 구현의 입력 검증 정책이며 원본의 상수는 아니다. 특수 레코드는 헤더·바이트를 보존하지만 이미지로 압축 해제하지 않는다.

## 확인한 판본 차이

| 항목 | 패치판 | CD판 |
|---|---|---|
| 그래픽 자산 타입·SHP 블록 | 116 | 101 |
| 프레임 / 이미지 / 특수 레코드 | 3,783 / 3,692 / 91 | 3,167 / 3,085 / 82 |
| `.type` 클러스터 | 2,829 | 2,260 |
| `manabolt` 정의 / SHP 프레임 | 4 / 8 | 4 / 4 |
| Sun Cannon `hpPerSec` | 16 | 14 |
| 원본 RiftType 구조체 크기 | 500바이트 | 468바이트 |
| foot_x / foot_y 필드 | `+0x1d4` / `+0x1d8` | `+0x1b4` / `+0x1b8` |

앞 101개의 자산 이름·순서는 두 판본에서 같다. CD판 데이터에는 `--cd`를 지정하며 다른 판본의 블록 개수는 실패로 보고한다. 패치판 `manabolt`의 추가 프레임을 잘라내거나 CD판 수치로 패치판 속성을 덮어쓰지 않는다.

RiftType 크기는 후처리 `0049b0d0`의 인덱스 계산 `/500`, CD `00444e10`의 `/0x1d4`에서 확인했다. 프레임 검색 필드 `+0x114`·`+0x124`는 공통이다. 앞선 Ghidra `RiftTypeFrameView32`의 CD 크기를 500에서 **468로 정정**했다. 두 후처리 모두 `foot_y == 6`이면 8로 보정한다. C++에서는 바이너리 객체 배치를 직접 복제하지 않는다.

## 기계어 검증

[tools/decomp_graphics_oracle.py](../../tools/decomp_graphics_oracle.py)는 두 함수만 허용한다.

| 함수 | 패치판 | CD판 | 반환 시 스택 복구 |
|---|---|---|---|
| 기본 그리기·클리핑 | `00401d92` | `00465772` | `ret 20` |
| 클리핑 없는 내부 압축 해제 | `004021b5` | `00465b95` | `ret 20` |

64개 생성 RLE와 두 판본의 대표 타입 24프레임을 서로 다른 pane·배치·배경에서 실행하고 오류 반환도 확인했다. 총 **709개**의 반환 코드·화면 전체 바이트가 두 원본 기계어와 C++에서 일치한다. 32비트 flat 세그먼트를 구성하여 `push/pop ES`·문자열 복사 명령도 그대로 실행한다. 이 검사에는 **CRT·assert 대체 함수가 없다**. 게임 진입점·OS API로 들어가면 즉시 실패한다.

기대값은 [graphics-x86.tsv](../../cpppj/tests/fixtures/graphics-x86.tsv), 실행 파일·기대값 SHA-256과 범위는 [recovery-graphics-evidence.json](../../cpppj/recovery-graphics-evidence.json)에 있다. 기존 프레임 검색·Config 기대값 1,806개도 유지하므로 CTest는 **17개 테스트에서 총 2,515개 x86 입력**을 대조한다. 원본 바이너리·Python 없이 C++ 회귀 검사를 실행할 수 있다.

[tools/cpp_assets_smoke.py](../../tools/cpp_assets_smoke.py)는 PE의 로딩 배열을 직접 읽고 기존 Python `.type`·SHP 판독기와 독립 대조한다. 두 판본 **217개 타입·6,777개 이미지의 15,940,472픽셀**에서 헤더·속성·클러스터 코드·팔레트 번호·투명 마스크가 일치했다. 일반 프레임뿐 아니라 특수 레코드 173개도 헤더를 비교했다. 각 판본의 Nimbian·Sun Cannon 본체/그림자·Altar, 총 **8개 BMP**를 Pillow로 다시 읽어 RGBA·상하 방향까지 확인한다. 원본 실행 파일·아카이브·SHP 해시는 검사 전후 동일하다.

## 재현

저장소 루트에서 실행한다. C++ 빌드에는 외부 라이브러리가 필요 없다.

```powershell
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
cpppj/build/bin/Release/NetstormCpp.exe --inspect-assets originals
cpppj/build/bin/Release/NetstormCpp.exe --inspect-assets originalCD --cd
python tools/cpp_assets_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --export-frame originals sunCannon N00 0 extracted/sunCannon.bmp

# 선택적 분석 도구. 기대값을 다시 만들 때만 원본 PE·정밀 함수 경계가 필요하다.
python tools/decomp_graphics_oracle.py
```

픽셀 검사 보고서와 검토용 이미지는 `extracted/cpp-assets-smoke/`에 생성한다. `--dump-assets`는 검사 도구용 길이·숫자·바이트 스트림이며 원본 파일 형식이나 게임 저장 형식이 아니다.

## 남은 범위

타입 파서 전체를 원본 x86으로 실행한 것은 아니다. `.type` 데이터 읽기·프레임 코드 생성은 정적 대조와 실제 두 판본 전체 자산 비교로 검증했다. 게임 플래그의 모든 비트·원본 생성자 함수·전역 타입 ID·속성별 float 변환·난수 프레임 선택·방향 폴백은 아직 연결하지 않았다. 그래픽도 기본 8비트 합성만 검증했으며 색 변환표·그림자 효과·확대·반전·폰트·실제 창 표시는 후속 작업이다.

`GameAssets`는 초기 `setup.cfg`의 공통 `gifcloud` 팔레트를 사용한다. Config 치환·동적 팔레트 선택, `.fort` 로더와 실제 프로세스 객체, Renderer·UserInput·게임 루프를 이어서 복원해야 한다. 한국어 글꼴·와이드 화면·전체화면·60/120프레임·MCP도 남아 있다.

후속 순서·의존 관계·완료 기준·검사 방법은 [cpppj 후속 복원 계획](../cpp-roadmap.md)에 정리했다.
