# cpppj raw SID의 일반 Pop 복원과 창 회귀 검사

2026-10-06. 사용자 지시로 이번 PC의 창 검사를 허용한 뒤 [SquidPop](../../cpppj/src/o/SquidPop.h)을 raw SidPool·SquidHash·spot에 연결했다. **표시와 공통 postPop 효과가 억제된 비전투 단계**다. 일반 객체와 매몰로 영역 효과를 건너뛰는 객체만 지원한다. 기존 GameWorld는 아직 임시 객체 목록을 사용하며 새 raw 경로와 연결되지 않았다. 미션 완주는 불가능하다.

## 원본 근거와 디컴파일 해석

읽기 전용·헤드리스 Ghidra의 `ExportCreation.java`로 Pop 관련 패치 22개와 보조 2개, CD 18개 함수를 다시 내보냈다. `extracted/pop/<판본>/creation.c`, `functions.tsv`, `extracted/pop/helpers-originals/`에 저장한다. 필요한 SID/기본 생성 내보내기도 재생성했다. 완료 메시지와 실제 내보낸 행 수를 확인했으며 함수 정의 변경은 저장하지 않았다. 이전 전체 대응 2,496쌍과 검토 앵커 24쌍은 유지했다.

| 역할 | 패치판 10.78 | CD판 10.72 |
|---|---|---|
| Pop | `004b02d0` | `004ad490` |
| 비전투 Activate | `004ac560` | `004abfb0` |
| 공통 firstPop / postPop | `004ad470` / `004b0d30` | `004ae0f0` / `004ae180` |
| postPop 깊이 감소 | postPop 인라인 | `004abab0` |
| 표시 억제 Renderer | `004990d0` | `00456a00` |
| 좌표 입력 보정 | `x<=0` 또는 `y<=0`, 보드 이상 | float 비트값의 signed 비교, 보드 이상 |
| 점유 충돌 | 부분 spot OR 후 즉시 반환 | 충돌해도 OR 후 등록 계속 |

**두 판본 모두 유한한 양수 소수 좌표를 보존한다.** CD판 디컴파일의 `(int)param_2 < 1`은 실제 수치의 정수 변환이 아니었다. `004ad536`/`004ad552`는 float 입력을 담은 DWORD의 비트값과 0을 signed 비교한다. 처음 이를 좌표 절삭으로 해석한 변경은 원본 기계어와 기존 fixture에서 실패하여 되돌렸다. `SquidSpatial::Coordinates`에 이 근거를 주석으로 남기고 0.25 좌표 회귀 검사를 추가했다. NaN/무한대는 원본 오류 경로를 재현하지 않고 변경 전에 거부한다.

## C++의 적용 범위

Pop은 좌표와 화면 short 캐시(x×16+0.5, y×11+0.5)→발자국 spot OR→현재 SHP 크기의 해시 단계→next/기준점 머리→최초 등록 extra→void 해제 순서로 raw 슬롯을 갱신한다. HP·소유자·프레임과 다른 payload를 보존한다. 원본 좌표/발자국을 별도 객체로 복사한 예전 어댑터와 달리, 같은 풀에서 Unpop과 반납을 이어서 사용할 수 있다.

패치 충돌은 좌표와 앞서 쓴 spot을 남기고 `RawPopResult::Overlap`을 반환한다. next·머리·firstPop·활성화까지 진행하지 않는다. CD판은 같은 충돌 입력도 등록한다. 재등록에서 extra `0x80`이 있으면 firstPop을 다시 실행하지 않는다. 공통 firstPop의 abstract/buried 플래그와 비전투 Activate의 플래그 정규화는 억제된 공통 postPop 경로로 이어진다. 표시·postPop 효과가 활성화된 호출 단계에 이 클래스를 그대로 사용하면 안 된다.

[PopVtables.inc](../../cpppj/src/o/PopVtables.inc)는 PE의 firstPop/postPop과 공통 표시 두 경로를 대조한 **패치 26개/CD 16개** 지원 주소다. 전체 감사 표는 기존 생성자 vtable 151행이며 호출 수와 별도다. 미복원 override는 base 후처리로 대체하지 않는다. 매몰이 아닌 섬·다리·건물 부착 효과, 잘못된 발자국·자기 머리·free/transmitting/contained 상태를 변경 전에 거부한다. 패치의 이미 활성인 Pop은 원본처럼 그대로 반환하고 CD판은 오류로 보고한다.

## 독립 기계어 대조와 C++ 검사

[decomp_pop_oracle.py](../../tools/decomp_pop_oracle.py)는 원본 PE의 실제 SID 초기화·base fallback 생성·Pop·firstPop·Activate·공통 postPop·표시 억제·Unpop·void 반납을 실행한다. **대체 함수와 assert 도달은 0회**다. 코드 몸체/쓰기 범위·시간/명령·정상 EIP/ESP·x87 TOP/제어 워드·postPop 깊이를 제한한다. 합성 타입/SHP와 기존 확보 슬롯을 입력하며, 파생 vtable 입력은 기존 payload를 합성한 것이고 이 도구에서 파생 생성자를 실행한 것은 아니다.

8시퀀스, **4,600회**: Create 752, Pop 1,712, Unpop 1,384, Release 752. 실제 성공한 Pop의 postPop은 패치 688/CD 696회, firstPop은 패치 208/CD 216회다. 초기 Reset 8회와 vtable 표 151행은 4,600회에 더하지 않는다. 풀 32,768/65,535·x87 `0x027f`/`0x037f`·네 해시 단계·소수/잘못된 좌표·재등록·상태/extra·충돌 앞선 OR를 포함한다. 네 단계 각각 같은 버킷에 세 객체를 실제 Pop으로 넣고 중간/머리/꼬리를 해제했다. SID 65,534를 포함한 고위 SID는 명시적인 Claim 입력이며 고위 할당기나 상위 Take 목록 흐름을 검증한 수치가 아니다.

[PopTests.cpp](../../cpppj/tests/PopTests.cpp)는 대상 슬롯의 모든 바이트와 전체 풀·86,272머리·65,536 spot의 Adler-32를 대조한다. 전체 체크섬을 바이트별 동일성 증명으로 해석하지 않는다. 미복원 효과 거부·vtable 지원 표·양 판본 소수 경계를 추가 검사한다. 기존 누적 46,156행에 더한 제한 x86 입력은 **50,756행**, CTest 내부 검사는 **116개**다. [원본·도구·fixture 해시와 실제 호출 기록](../../cpppj/recovery-pop-evidence.json).

## 이번 PC의 실행 검증

Release 구성/빌드·CTest와 `cpp_window_smoke.py`, `cpp_renderer_smoke.py`, `cpp_menu_smoke.py`, `cpp_world_smoke.py`를 실행했다. 새 모듈을 빌드한 뒤 월드 검사를 다시 통과했다. 원본/복사본 게임 프로세스는 실행하지 않았으며 AGENTS.md와 기존 게임 자료는 변경하지 않았다. 허용된 두 설정 파일은 내용과 존재 상태를 복구했다.

- 영역: 양 판본 488요새, 배치 영역 2,119개, 2,607줄 대조.
- 표시: 타입 화면 1,266,432픽셀, 글꼴 18개/4,608글리프/302,480픽셀, 글꼴 창 786,432픽셀과 변화 없는 4프레임의 출력 생략.
- 메뉴: 54상태, 640×480·800×600·1024×768의 실제 창 크기, 옵션 저장/재실행, 브리핑 MORE/BACK, 메뉴 복귀.
- 월드: 두 판본 초기 자료 6사례와 393,216마스크 바이트, 1-1/TEST01의 사제 선택·이동·정지·카메라·복귀/재진입. 이 창 검사는 **기존 임시 월드의 회귀**다.

보고서는 `extracted/cpp-{window,renderer,menu,world}-smoke/report.json`, 새 빌드/CTest 로그는 `extracted/pop-build.log`, `extracted/pop-ctest.log`에 둔다. DirectDraw 전체화면 전환/정상 재실행과 원본 대비 실제 게임 전체의 동작 일치는 이번 검사 범위가 아니다.

## 재현과 후속

SID/기본 생성의 읽기 전용 내보내기를 먼저 준비한다. 새 Pop 내보내기는 다음과 같다. Ghidra 프로세스 종료 코드 외에 `생성/Take 디컴파일 완료`와 함수 행 수를 확인해야 한다.

```powershell
& ./tools/ghidra/run_script.ps1 -Script ExportCreation.java -ScriptArgs @('extracted/pop/originals','004b02d0','004ac560','004b0d30','004ad470','004ad500','004ad670','004210a0','004acd20','004afd30','004ac1e0','004aba70','004b2a90','0049a840','00419850','0040e800','0040da00','004ace40','0049a900','0040eaf0','00425df0','00498160','00498220')
& ./tools/ghidra/run_script.ps1 -Script ExportCreation.java -ScriptArgs @('extracted/pop/helpers-originals','004990d0','004e49c0')
& ./tools/ghidra/run_script.ps1 -Edition originalCD -Script ExportCreation.java -ScriptArgs @('extracted/pop/originalCD','004ad490','004abfb0','004ae180','004ae0f0','004ae380','004ae3e0','004aca80','004acfc0','004abf50','004abae0','004abac0','004acb00','004ae480','004442b0','00479d20','004f161c','004abab0','00456a00')
python -X utf8 tools/decomp_pop_oracle.py
python -X utf8 tools/decomp_pop_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
python -X utf8 tools/cpp_world_smoke.py
```

다음은 표시 활성/dirty·grid·영역/소유자·postPop의 생산 효과와 섬/다리/건물 부착·파생 가상 효과다. 파생 삭제/의존 객체/참조·수신 목록·form/process·SID 소진 복구도 남았다. raw Pop/Unpop을 GameWorld 지형·내용물·실제 프레임과 연결한 뒤 다리 배치/Construction·경제/전투/AI/승패를 이어 간다. 이번 GUI 통과를 raw 월드 연결이나 미션 완주로 완료 표시하지 않는다.
