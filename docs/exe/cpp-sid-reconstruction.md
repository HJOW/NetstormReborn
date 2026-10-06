# cpppj SID 풀·할당·반납 복원

2026-10-06. 원본 실행과 클론 창 표시 없이 두 판본의 Squid.cpp를 정적으로 대조하고 [SidPool](../../cpppj/src/o/SidPool.h)을 추가했다. `Sid`는 원본 16비트 슬롯 번호이며 현재 GameWorld의 32비트 임시 `SquidId`와 암묵 변환하지 않는다. **파생 생성자·공간 등록/삭제 효과·GameWorld 연결은 남았다.** 건설/전투나 미션 완주를 가능하게 한 변경은 아니다.

## 함수와 판본 경계

| 역할 | 패치판 10.78 | CD판 10.72 |
|---|---|---|
| 기존 풀 초기화 | `004abb10` | `004aaad0` |
| 번호 할당 | `004af1d0` | `004aae40` |
| void 슬롯 반납 | `004abf60` | `004ab250` |
| 서버 free list 재구성 | `004abe00` | `004aad30` |
| next 쓰기 | `004aba70` | 위 함수에 인라인 |
| 최근 삭제 기록 | `004952f0` | `004cff40` |
| 기록의 겹친 CRT 복사 | `004e5b40` | `004f1a60` |
| raw 슬롯 크기 | **50바이트** | **36바이트** |
| 일반 클라이언트 영역 | **5..14999** | **5..5999** |
| 일반 서버 영역 | **15000..23000** | **6000..13999** |
| predictableHead / 첫 예측 할당 | **23001 / 23002** | **14000 / 14001** |

[ExportSid.java](../../tools/ghidra/ExportSid.java)는 기존 프로젝트를 읽기 전용·헤드리스로 열어 패치 7개/CD 6개 함수의 C 출력과 실제 불연속 몸체 범위를 `extracted/sid/<판본>/sid.c`, `functions.tsv`에 내보낸다. 원본 함수의 인자 추정 C만으로 next나 포인터 산술을 정하지 않고 실제 어셈블리/기계어도 대조했다. 기존 전체 대응 **2,496쌍·검토 앵커 24쌍**에는 새 항목을 추가하지 않았다.

## 원본 의미를 보존한 부분

raw 풀의 `+4`는 short next, `+10`은 타입, `+11`은 free=1/dead=2/void=4 상태다. 일반 clientHead는 pool+0, serverHead는 **pool+14**의 겹친 객체이며 서버 목록의 next는 풀 시작+18에 놓인다. C++는 이 바이트 배치를 그대로 저장하되 원본의 32비트 vtable 값을 호스트 포인터로 역참조하지 않는다.

초기화는 풀 전체를 지우고 2번부터 free 비트를 켠다. 0..4번은 일반 할당에서 제외한다. 세 영역의 next는 번호순이며 끝은 0이다. `freeCount=capacity-5`, 예측 커서=1이다. 일반 할당은 머리의 첫 슬롯을 떼지만 **마지막 free 슬롯은 예약 꼬리로 남긴다**. 반환한 슬롯 전체를 지운 뒤 void 상태로 만든다. 할당 flags의 bit 1(값 1)은 예측, bit 2(값 2)는 client이고 예측이 우선한다.

예측 번호는 predictableHead+커서로 증가한다. 서버 모드에서는 다음 번호가 실제 free 슬롯인지 확인하고 예측 머리의 next도 갱신한다. 클라이언트 모드에서는 이 next 갱신/확인이 없어서 마지막 예측 슬롯도 쓸 수 있다. 반납해도 커서는 돌아가지 않고 예측 슬롯을 일반 free list에 넣지 않는다.

반납은 void이고 free가 아닌 슬롯에만 적용한다. 삭제 기록을 먼저 넣고 슬롯 전체를 지운 뒤 **이전 타입 번호를 보존한다**. 일반 번호는 해당 FIFO 꼬리에 붙이고 freeCount를 증가시키며 상태 3(free+dead)으로 남긴다. 서버 모드의 예측 반납도 상태 3이지만 목록/카운터에는 넣지 않는다. 클라이언트 모드의 서버/예측 번호 반납은 상태 1만 남기고 카운터를 증가시키지 않는다. 최근 삭제 기록은 client/server 각각 20항목이며 최신 기록이 맨 앞이다. SID 초기화 자체는 이 기록을 지우지 않는다.

서버 목록 재구성은 서버 범위의 free 비트만 읽어 번호순으로 잇고 마지막 next를 0으로 닫는다. freeCount는 **재계산하거나 0으로 만들지 않고 기존 값에 발견한 free 슬롯 수를 더한다**. 독립 함수의 원본 동작을 보존했으며 상위 호출 순서의 카운터 조정은 후속 분석 대상이다.

이 네 경로에는 **세대 카운터/세대 비트가 없다**. 슬롯은 초기화 후 재사용된다. 앞선 계획의 "SID 세대"를 완료 처리하기 위해 원본에 없는 세대를 추가하지 않았다. 오래된 객체 참조의 실제 처리, 네트워크 Take(`004af610`/CD `004ab440`), 의존 객체·파생 수명은 별도 복원 대상이다.

## 검증 범위

[decomp_sid_oracle.py](../../tools/decomp_sid_oracle.py)는 두 PE를 별도 Unicorn 메모리에 복사해 위 경로를 정상 반환까지 실행한다. **대체 함수는 없다.** 삭제 기록의 겹친 152바이트 CRT 복사도 실제 기계어다. Ghidra가 확인한 함수 몸체 밖 실행, 풀/지정 카운터/삭제 기록/가상 스택 밖 쓰기, assert 도달을 거부한다. 호출마다 레지스터/DF를 초기화하고 정상 EIP/ESP·cdecl ret 0, 최대 1,000만 명령·30초 제한을 확인한다.

입력은 판본별 최소 크기 근처·256개 예측 영역·32768·65535슬롯, 서버/클라이언트 모드의 **16개 시퀀스**다. **3,168회 호출**(초기화 32·할당 1,552·반납 1,552·재구성 32)을 기록한다. 순서가 다른 반납, 20항목 넘는 기록, 전체 슬롯을 오염시키는 합성 생성자 입력, 예측 우선 flags, 반복 재구성과 초기화를 포함한다. assert 도달은 두 판본 모두 0회다.

[SidTests.cpp](../../cpppj/tests/SidTests.cpp)의 6개 새 검사는 [sid-x86.tsv](../../cpppj/tests/fixtures/sid-x86.tsv)의 주요 카운터·머리/꼬리와 raw 풀 전체·삭제 기록 전체의 **Adler-32**를 대조한다. 이 체크섬 비교는 각 바이트를 직접 비교하거나 암호학적으로 동일성을 증명하는 검사가 아니다. 별도 경계 검사에서 FIFO 예약 꼬리·예측 소진·반납 후 전체 payload 초기화/타입 보존·카운터 누적·오류 전 상태 보존을 확인한다. fixture·원본·생성 도구의 SHA-256은 [검증 기록](../../cpppj/recovery-sid-evidence.json)과 `--verify`로 확인한다.

**제한:** 초기화는 이미 확보한 메모리 경로만 실행하여 CRT malloc/실패 처리를 검증하지 않았다. 할당은 소진 전 경로만 실행했다. 원본의 소진 시 UI·다리 파괴/재시도는 아직 없으며 C++는 풀을 바꾸지 않고 예외를 반환한다. 새 C++의 크기/상태/번호 보호는 원본 assert 이후의 잘못된 메모리 접근을 재현하지 않는다. 파생 생성자 입력 `Fill`은 실제 생성자가 아니며 가상 함수·공간 Pop/Unpop·Battle/프로세스·삭제 효과를 연결한 검증이 아니다. 이 사례 수는 게임 전체 완성도나 함수의 모든 분기 검증 수치가 아니다.

## 재현과 실행 제한

```powershell
& ./tools/ghidra/run_script.ps1 -Script ExportSid.java -ScriptArgs @('extracted/sid/originals','004abb10','004af1d0','004abf60','004abe00','004aba70','004952f0','004e5b40')
& ./tools/ghidra/run_script.ps1 -Edition originalCD -Script ExportSid.java -ScriptArgs @('extracted/sid/originalCD','004aaad0','004aae40','004ab250','004aad30','004cff40','004f1a60')
python tools/decomp_sid_oracle.py
python tools/decomp_sid_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

기대값 생성은 기존 Ghidra 프로젝트와 `tools/requirements-decomp-oracle.txt`의 분석 라이브러리가 필요하다. C++/CTest는 저장된 fixture만 읽으며 분석 환경/원본 파일/GUI가 필요 없다. 이 PC에는 VS 2026 Insiders와 내장 CMake를 사용해 `Visual Studio 18 2026` x64로 구성했다. 다른 PC의 VS 2022 프리셋을 변경하지 않았다.

**2026-10-06 사용자 제한:** 이 PC에서 원본 게임·복사본을 실행하거나 cpppj 창을 띄우는 검사는 진행하지 않는다. `--run`, `cpp_window_smoke.py`, `cpp_renderer_smoke.py`, `cpp_menu_smoke.py`, `cpp_world_smoke.py`는 다른 PC의 인수인계 항목이다. 이번 SID 풀은 월드에 연결하지 않았으므로 기존 GUI 기록을 새 SID 연동 검증으로 간주하지 않는다.

다음은 생성자/가상 초기화→공간 등록/해제와 반납의 수명 연결→실제 firstPop/postPop·영역/소유자·dirty/grid·부착 프레임→GameWorld 원본 SID 연결이다. 이어 다리 배치·건설/경제/전투/AI/승패를 복원한다. 자세한 다음 작업은 [LEFT_JOBS.md](../../LEFT_JOBS.md)에 남긴다.
