# cpppj 기본 객체 생성·Take·가상 초기화 복원

2026-10-06. [SquidFactory](../../cpppj/src/o/SquidFactory.h)를 기존 [SidPool](../../cpppj/src/o/SidPool.h)에 연결했다. 원본/복사본 게임 프로세스와 클론 창은 실행하지 않았다. **실제 GameWorld, 파생 생성자와 non-void 객체의 공간 해제는 아직 연결하지 않았다.**

## 정적 근거와 판본 차이

| 역할 | 패치판 10.78 | CD판 10.72 |
|---|---|---|
| 생성 | `004af530` | `004ab390` |
| 서버 번호 Take | `004af610` | `004ab440` |
| base postCreate / postTake | `004b0c60` / `004ad4a0` | `004ae040` / `004ae120` |
| 최대 HP 조회 | `0049fce0` | `00444720` |
| base Unpop | `004afe50` | `004ad0b0` |
| 타입 초기화 | `0049ebb0` | `004434d0` |
| 타입의 생성자 위치 | `+0x1d0` | `+0x1b0` |
| 기본 vtable 기록값 | `00501dd0` | `005058d8` |
| raw owner / zOrder | `+34` / `+35` | `+32` / `+33` |
| raw HP | **+26의 4바이트** | **+26의 2바이트** |
| dais abstract 비트 | `+40 & 1` | `+35 & 1` |

[ExportCreation.java](../../tools/ghidra/ExportCreation.java)는 기존 프로젝트를 읽기 전용·헤드리스로 열어 패치 8개/CD 13개 함수의 C와 불연속 몸체를 `extracted/creation/<판본>/creation.c`, `functions.tsv`에 내보낸다. 기존 프로젝트에 없던 CD Unpop은 메모리에서만 함수로 정의하고 변경을 버렸다. 인자·ECX·vtable offset·쓰기 폭은 실제 어셈블리도 대조했다. 기존 전체 함수 대응 **2,496쌍·검토 앵커 24쌍**은 그대로다.

생성자 표는 타입 초기화 중 **대입 두 구간**의 실제 명령을 격리 실행해 확보했다. 패치 **188타입 중 122개**, CD **171타입 중 105개**에 주소가 있고 나머지는 0이다. CD판 마지막 대입까지 포함한다. [TypeConstructors.inc](../../cpppj/src/o/TypeConstructors.inc)는 원본 주소의 기록이다. 호스트 함수 포인터로 캐스팅하거나 호출하지 않는다. 파일 로딩·프로세스 이름 복사·메모리 확보·타입 후처리 전체를 실행한 결과로 해석하면 안 된다.

## C++에 옮긴 동작

`RiftTypeTable`에 생성자 주소, `maxHitPoints`, `zOrder`를 연결했다. 숫자 깊이와 원본 23개 깊이 이름/선택적 +/- 오프셋을 읽는다. 이름 표는 패치 `00540d10`, CD `0051c630`에서 같다. 문자열 깊이는 원본처럼 표 이름에 대한 부분 문자열의 첫 일치이며 대소문자를 구분한다. 깊이와 HP 속성 파서 연결은 C++ 단위 검사로 확인했고 원본 속성 파서 전체의 x86 대조는 하지 않았다.

`Create`는 기존 SID 할당→base vtable→타입 바이트→base postCreate 순서다. constructor=0인 자산 타입만 지원한다. 미복원 파생 생성자·form·잘못된 타입, 패치판의 fakeThreeByThreeSurface(162)는 할당 전에 거부한다. **미복원 타입을 base 객체로 조용히 대체하지 않는다.**

base postCreate는 타입 `+0x104`의 zOrder 하위 바이트를 복사한다. 서버이면 owner를 0으로 만들고 필요할 때 HP를 초기화한다. 일반 타입은 flags1의 `0x10`, dais는 flags2의 `0x400000`과 객체 abstract 비트가 HP 필요성을 정한다. mana(158)의 약화 옵션이 켜지면 최대 HP를 **0 방향으로 절삭하는 정수 /4**로 계산한다. CD판은 결과의 하위 16비트만 저장한다. base postTake는 zOrder만 복사하며 owner·HP를 보존한다. 이 base 메서드 둘은 후속 파생 클래스의 base 호출에도 쓸 수 있다.

`Take`는 서버 시작 번호 이상이고 capacity 미만인 번호를 받는다. **전체 슬롯을 초기화하지 않는다.** free/dead 비트만 지우고 void를 켜며 vtable·타입·zOrder를 갱신한다. 나머지 payload와 free list·freeCount·예측 커서는 보존한다. free 슬롯이면 기존 타입은 달라도 된다. 이미 할당한 슬롯이면 같은 타입의 base vtable·void 상태만 지원한다. 예측 머리 자체와 마지막 슬롯도 Take의 원본 명시 범위 안이므로 별도로 허용한다.

Take는 free list를 떼거나 카운터를 조정하는 Allocate와 다르다. **Take 직후의 풀을 일반 서버 Allocate에 그대로 혼용하는 상위 네트워크 흐름을 구현한 것이 아니다.** 목록 재구성/카운터 조정·패킷/참조 수명은 후속이다. 타입 불일치·contained·non-void·파생 vtable은 풀을 변경하기 전에 예외로 거부한다. 원본 오류 UI/assert 이후의 잘못된 메모리 접근을 재현하지 않는다.

## 검증

[decomp_creation_oracle.py](../../tools/decomp_creation_oracle.py)는 PE를 Unicorn 메모리에 읽고 Ghidra의 허용 몸체 안에서 원본 명령만 실행한다. **대체 함수는 없다.** 이미 void인 Take도 기존 base Unpop을 실제 호출하여 조기 반환한다(각 판본 74회). 허용 범위 밖 코드/쓰기·assert 도달을 거부하고 명령 수·30초 제한·정상 EIP/ESP와 cdecl/ECX 반환을 확인했다. 두 판본의 assert 도달은 0회다.

서버/클라이언트의 **4시퀀스·964회 호출**: Create 192, postCreate 304, postTake 304, Take 164. 합성 타입 필드·오염된 payload·상태 상위 비트·HP 16/32비트 경계와 음수·dais abstract·약화 mana·예측 flags·free 서버/예측 머리/최종 슬롯·반복 Take를 포함한다. 준비 Reset 4회와 생성자 대입 구간은 이 호출 수에 더하지 않았다.

[CreationTests.cpp](../../cpppj/tests/CreationTests.cpp)는 대상 raw 슬롯의 **모든 바이트를 직접 비교**하고 풀 전체의 Adler-32와 카운터·머리/꼬리를 대조한다. 생성자 주소 표는 0을 포함한 359행을 따로 비교한다. CTest의 새 5개 검사에는 지원하지 않는 입력의 변경 전 거부와 실제 타입 로더→SID HP/깊이 연결도 포함된다. 원본/도구/fixture/생성 주소 표의 SHA-256은 [검증 기록](../../cpppj/recovery-creation-evidence.json)과 `--verify`로 확인한다.

Release 빌드는 경고/오류 없이 통과했고 CTest 한 실행 파일의 **104개 내부 검사**가 통과했다. 타입 로더 변경의 실제 자료 회귀도 창 없는 `cpp_fort_smoke.py`로 확인했다. 두 판본 합계 **488요새·320,634객체·326,169줄**의 구조/타입/해시/미션 값이 기존 Python 판독기와 같고 원본 아카이브/낱개 요새의 전후 해시가 같다. 보고서는 `extracted/cpp-fort-smoke/report.json`이다. 이 자료 대조는 새 HP/깊이 원본 파서 전체나 생성/월드 동작의 독립 검증을 대신하지 않는다.

**제한:** 생성은 base fallback만 검증했다. 합성 타입 입력은 실제 자산 로딩 결과 전체가 아니다. 파생 생성자·다른 가상 메서드·non-void Unpop·공간/영역 효과·삭제/반납·네트워크 패킷·실제 참조 수명·GameWorld 연결은 남았다. 이 검증은 미션 완주나 원본 화면/애니메이션의 일치를 증명하지 않는다.

## 재현과 다른 PC 인수인계

```powershell
& ./tools/ghidra/run_script.ps1 -Script ExportCreation.java -ScriptArgs @('extracted/creation/originals','004af530','004af610','004b0c60','004ad4a0','0049a840','0049fce0','004afe50','0049ebb0')
& ./tools/ghidra/run_script.ps1 -Edition originalCD -Script ExportCreation.java -ScriptArgs @('extracted/creation/originalCD','004ab390','004ab440','004ae040','004ae120','00444720','004aecf0','004aed20','004abac0','004abae0','004abf50','004448e0','004ad0b0','004434d0')
python -X utf8 tools/decomp_creation_oracle.py
python -X utf8 tools/decomp_creation_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
python -X utf8 tools/cpp_fort_smoke.py
```

분석에는 기존 Ghidra 프로젝트와 지정 Python 분석 패키지가 필요하다. C++ 검사는 저장된 fixture만 사용하며 원본 파일/분석 환경/창이 필요 없다. 이 PC에서는 VS 2026 Insiders의 내장 CMake를 사용했다.

**실행 제한 유지:** 원본/복사본과 `NetstormCpp.exe --run`, 창을 띄우는 스모크는 여기서 실행하지 않는다. 파생/월드 연결 후 다른 PC에서 TEST01/1-1의 생성·선택·이동·정지·재진입을 확인한다. 명령과 다음 복원 순서는 [LEFT_JOBS.md](../../LEFT_JOBS.md)에 남긴다.
