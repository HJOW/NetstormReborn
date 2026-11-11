# Q: 14차 QuickJS 엔진 탑재 (제품 1301, 2026-11-11)

마지막 디컴파일 PC 호스트명: HJOW-Athlon(변경 없음, 재디컴파일 없음). AGENTS.md(`85551c6c2`) 14차 요구를 구현했다. 원본 10.78·10.72 모두 대응 코드가 없는 새 기능이며, 차후 10.72 복원에서도 판본과 무관하게 같은 장치를 쓴다.

## 1. 요구와 범위

| 항목 | 내용 |
| --- | --- |
| AGENTS 14차 | "QuickJS 지원(<https://github.com/quickjs-ng/quickjs>) — 일단 QuickJS 엔진 탑재만 하고, 어느 정도까지 연동할지는 추후 고민" |
| 이번 범위 | LEFT_JOBS 결정 **D7의 ①(탑재만)**: 엔진(런타임·컨텍스트) 생성·해제와 최소 자체 검사. 게임 상태·파일·네트워크·OS 기능은 스크립트에 노출하지 않는다 |
| 하지 않은 것 | 임의 JS 평가 CLI/MCP 도구(②), 게임 상태 읽기·명령 노출(③), 미션 대본·Tell 확장(④). 모두 D7 결정 뒤 |

## 2. 엔진 소스와 빌드

| 항목 | 값 |
| --- | --- |
| 위치 | `cpppj/third_party/quickjs-ng/` |
| 판본 | 릴리스 `v0.18.0`(경량 태그, 커밋 `d7215b0ad3fc2c562e8317fd9bca058fd4863804`, 2026-11-01) |
| 가져온 방법 | 사용자가 이 PC의 `H:\Workspace\git\quickjs`에 받아 둔 공식 저장소 클론에서 `git show v0.18.0:<파일>`로 꺼냈다(클론 HEAD는 v0.18.0+6이지만 고정 릴리스를 썼다. 클론은 바꾸지 않았고 전역 git 설정도 바꾸지 않았다). 빌드 중 내려받기는 없다 |
| 상류 파일 | 23개 바이트 그대로: 엔진 소스 `quickjs.c`·`dtoa.c`·`libregexp.c`·`libunicode.c`, 헤더 14개, 생성 헤더의 원본 JS 3개, `LICENSE`(MIT), `README.md`. 파일별 URL·SHA-256·크기는 `provenance.json` |
| 바이트 보존 | `.gitattributes`에 `cpppj/third_party/quickjs-ng/** -text`(libsmacker와 같은 방식). core.autocrlf 체크아웃에서도 SHA-256이 유지된다 |
| 빌드 대상 | `NetstormQuickJs` 정적 라이브러리(상류 `qjs`와 같은 소스 4개). C11, 상류 Windows 정의, 상류 MSVC 경고 억제 목록 + `/utf-8`(저작권 주석 UTF-8을 cp949로 잘못 읽지 않게) + `/experimental:c11atomics`(상류와 같음) + `/wd4098`(상류 코드의 무해한 경고 4곳) |
| 연결 | `netstorm` 라이브러리에 `target_link_libraries(... PUBLIC NetstormQuickJs)`. 엔진 포함 경로는 `ScriptEngine.cpp` 한 파일의 원천 속성으로만 알려 **netstorm의 다른 840여 소스 명령줄을 바꾸지 않았다**(증분 빌드 1.4분, 엔진 소스 4개·`ScriptEngine.cpp`·새 검사 대상 2개·`main.cpp`만 컴파일) |
| 제외 | quickjs-libc(`std`/`os` 모듈·파일·워커), qjs/qjsc 도구, 검사 자료 |
| 아키텍처 | VS 2026(18.11.3) MSVC 14.51: **x64 경고 0 빌드·실행**, **ARM64 교차 빌드 경고 0**(엔진+장치+검사 실행 파일, `AA64 machine` 확인). ARM64 실행과 게임 전체의 ARM64 빌드는 하지 않았다(ARM 실기 필요) |

## 3. 게임 쪽 장치 `netstorm::platform::ScriptEngine`

`cpppj/src/platform/ScriptEngine.{h,cpp}`. 헤더는 `quickjs.h`를 드러내지 않는다(내부 상태 pimpl).

| 항목 | 설계 |
| --- | --- |
| 수명 | 생성자에서 `JS_NewRuntime2` → 스택 한도·인터럽트 → `JS_NewContext`, 소멸자에서 컨텍스트 → 런타임 해제. 실패하면 `std::runtime_error` |
| 컨텍스트 | `JS_NewContext` 기본 내장 객체만(언어 내장 + `performance`). `console`·`std`·`os`·`require`·`fetch`·`XMLHttpRequest`·`WebSocket`·`process`·`print`가 없고 모듈 적재기가 없어 `import('fs')`는 거부된다(검사로 확인) |
| 평가 | `Evaluate(원문, 파일 이름)`: 엄격 모드 전역 스크립트, 결과를 UTF-8 문자열로 반환. 엔진 예외는 C++ 예외가 아니라 `{succeeded=false, text="TypeError: ...\n    at ..."}`. 평가 뒤 대기 중인 Promise 작업을 시간 한도 안에서 모두 실행한다(작업 안 처리되지 않은 예외도 실패 결과) |
| 메모리 한도 | 기본 16MiB. 상류 `JS_SetMemoryLimit`은 할당자 앞단에서 거절해 거절 사실을 알 수 없고, 한도가 완전히 차면 오류 객체도 못 만들어 **예외 값이 `null`**이 된다(첫 시험에서 결과 문자열 `null`로 관찰). 그래서 `JS_NewRuntime2`의 할당자 콜백(`_msize` 기준, 상류 Windows 기본값과 같음)에서 직접 세고 거절한다. 거절이 있었으면 결과는 `InternalError: out of memory (script engine memory limit of N bytes)`이고 `memoryLimitReached=true`. 스크립트가 직접 던진 `null`은 그대로 `null`이다 |
| 스택 한도 | 기본 256KiB. 게임 주 스레드는 MSVC 기본 1MiB이고 상류 기본 JS 스택도 1MiB라(상류는 실행 파일 스택을 8MiB로 키운다) 그대로 두면 깊은 재귀가 게임을 죽인다. 실행 파일 스택은 바꾸지 않고 JS 한도를 낮췄으며 `Evaluate`마다 `JS_UpdateStackTop`으로 호출 위치 기준을 다시 잡는다 |
| 시간 한도 | 기본 1초. 엔진 인터럽트 콜백(quickjs-ng 형식은 `int(JSContext*, void*)`)이 마감을 확인해 `InternalError: interrupted`로 끊는다. 평가 밖에서는 끊지 않는다 |
| 결정성 | 엔진 핵심 소스를 검사해 CRT `rand`/`srand`, FPU 제어 워드(`_controlfp`/`fesetround`), 작업 폴더·환경 변수·파일·소켓·모듈 적재 호출이 없음을 확인했다. `Math.random`은 컨텍스트별 xorshift 상태라 게임 난수에 영향이 없다 |
| 스레드 | 한 객체는 한 스레드에서만 쓴다. 엔진 전역 초기화는 상류가 `InitOnceExecuteOnce`로 보호한다 |

### 시작 자체 검사

`RunScriptEngineSelfCheck()`가 기본 한도 엔진 하나를 만들어 고정 항목 6개를 평가하고 해제한다(외부 입력 없음).

| 항목 | 원문 | 기대 |
| --- | --- | --- |
| expression | `[1, 2, 3].map((x) => x * 2).join(',')` | `2,4,6` |
| json | `JSON.stringify({ unit: 'Priest', cost: 25 })` | `{"unit":"Priest","cost":25}` |
| utf8-korean | `'사제·신전'.length + ':' + '사제·신전'` | `5:사제·신전` |
| bigint | `(2n ** 64n).toString()` | `18446744073709551616` |
| exception | `throw new RangeError('selfcheck')` | 예외, 첫 줄 `RangeError: selfcheck` |
| no-host-access | `[typeof std, typeof os, typeof require, typeof fetch, typeof XMLHttpRequest, typeof WebSocket].join(',')` | `undefined` 6개 |

- **게임 시작:** `main.cpp`의 `RunClient` 첫머리(`RecordStartupScriptSelfCheck`, 무인자 GUI·`--run`·`--mcp` 공통)에서 한 번 실행한다. 통과하면 7차 활동 기록에 `QuickJS-ng 0.18.0 자체 검사 통과 (6항목, x ms)`만 남겨 로그 파일을 만들지 않고, 실패하면 경고(`QuickJS` 분류)로 항목별 상세를 로그 파일에 쓴다. 실패해도 게임은 계속 시작한다. stdout을 쓰지 않아 `--mcp`(stdio JSON-RPC)와 충돌하지 않는다. 소요 약 1ms.
- **CLI:** `Netstorm.exe --quickjs-selftest`는 창·자산 없이 같은 검사를 실행해 UTF-8로 출력하고 통과 0/실패 1로 끝난다. `--help`에 `script engine: QuickJS-ng 0.18.0 (self-check: --quickjs-selftest)` 줄이 나온다. `--version` 출력은 바꾸지 않았다.

## 4. 배포·외부 접속

- `cpppj/package-cpppj.ps1`: ZIP의 `third-party\quickjs-ng\`에 `LICENSE`·`README.ko.md`·`provenance.json`(없으면 실패), `README.txt` 폴더 설명 한 줄과 영어 안내 9번, `BUILD_INFO.txt`에 `Script engine: QuickJS-ng v0.18.0 (commit …), static in Netstorm.exe, MIT` 줄(provenance.json에서 읽음). 엔진 소스 전체는 넣지 않는다(MIT는 고지 동봉만 요구). DryRun/실제 ZIP 생성은 M8 최종 관문에서 한다.
- `tools/verify_cpp_package.py`: 필수 항목에 `third-party/quickjs-ng/LICENSE`·`provenance.json` 추가. `third-party/` 항목은 기존 규칙대로 `cpppj/third_party/`와 SHA-256 대조된다.
- **가져오기 표(import) 비교(직전 빌드 산출물 2026-11-11 09:33 → 1301 빌드):** DLL 28개 집합 **변화 없음**. 함수 532 → 571(+39, 삭제 0): CRT 수학(`acos`·`pow`·`cbrt`·`expm1` 등 22)·힙(`calloc`·`_msize`)·`fputs`/`strcmp`/`memchr`/`strrchr`, KERNEL32의 시각/시간대(`GetSystemTime`·`GetTimeZoneInformation`·`SystemTimeToFileTime`)와 동기화(`InitOnceExecuteOnce`·임계 구역 3·조건 변수 3). WinINet/WinHTTP/URLMon·새 WinSock·파일/레지스트리/프로세스/`LoadLibrary` 가져오기는 **0**이다. [외부 정책](cpp-external-services-20261101.md)에 같은 내용을 적었다.

## 5. 검증

| 검사 | 결과 |
| --- | --- |
| 독립 x64 Release `script_engine_tests`(엔진+장치+검사만) | 14/14 통과. 첫 실행에서 메모리 한도 항목이 결과 `null`로 실패 → 할당자 예산 방식으로 교정 후 통과(기대값은 약화하지 않음) |
| 독립 x64 **Debug** | 14/14 통과. 상류 `JS_FreeRuntime`의 객체 누수 assert와 새 "해제 뒤 할당자 사용량 0" assert 모두 통과 |
| 독립 ARM64 Release 교차 빌드 | 성공, 경고 0, `AA64 machine (ARM64)`. 실행은 하지 않음 |
| 제품 빌드 `NetstormCpp`·`NetstormRoot`·`netstorm_script_engine_tests` | 종료 0, 경고 0, 1.4분(엔진 소스 4개·`ScriptEngine.cpp`·검사 대상의 `FocusedTestMain.cpp`/`ScriptEngineTests.cpp`·`main.cpp` 8개만 컴파일) |
| `netstorm_script_engine_tests` | **14/14 통과**: 식/undefined/JSON, 엔진별 전역 분리, UTF-8(한국어·일본어·보조 평면), 예외·스택의 파일 이름·SyntaxError·`throw 42`·이후 재사용, 엄격 모드, 시간 한도(50ms 무한 반복), 메모리 한도(4MiB, `throw null` 구별), 너무 작은 한도의 생성 실패, 스택 한도(무한 재귀 RangeError), Promise·`queueMicrotask` 작업, 외부 장치 부재·`import('fs')` 거부, 50회 생성/해제, 버전, 자체 검사 |
| `Netstorm.exe --quickjs-selftest` | PASS 6/6, 0.75~1.37ms, 종료 0. 출력 바이트가 올바른 UTF-8(`5:사제·신전`)임을 확인 |
| `Netstorm.exe --help`·`--version` | 엔진 줄 표시, `--version`은 `NetStorm 11.01` 그대로 |
| 실제 게임 시작(`--run originals --window --frames 150 --no-audio`, 사용자 데이터는 `NETSTORM_USER_DATA`로 임시 폴더) | 메인 메뉴 표시(State 4, 스크린샷 확인), 종료 0, `logs\` 없음(경고 없음), `originals` 최상위 파일 해시 불변 |
| 시작 기록 관찰(같은 시작 경로에서 알 수 없는 `--run` 옵션으로 최상위 치명 로그 유도) | 로그에 `시작: NetStorm 11.01` 다음 줄로 `QuickJS-ng 0.18.0 자체 검사 통과 (6항목, 1.11 ms)`가 기록됨 |

하지 않은 것: 통합 `netstorm_tests` 빌드(새 검사 파일은 등록만 했다), 배포 ZIP 생성·대조(M8 최종 관문에서 1회), ARM64 실행·게임 전체 ARM64 빌드.

## 6. dotnetpj·webpj에 넘길 계약

- **dotnetpj**(후순위 4~14차): 같은 범위로 QuickJS-ng 엔진을 탑재한다. 계약은 ① 고정 릴리스와 출처·MIT 고지 동봉, ② 엔진 생성·해제와 위 6개 자체 검사 항목(같은 원문·기대 문자열), ③ 게임 상태·파일·네트워크 비노출, ④ 메모리 16MiB·JS 스택 256KiB·평가 1초 한도와 메모리 한도 실패 문구, ⑤ `--quickjs-selftest`(통과 0/실패 1)와 시작 시 활동 기록. .NET에서의 결합 방식(네이티브 DLL·P/Invoke 등)은 DN이 정한다. DN 인계 파일은 CP 작업에서 수정하지 않았다.
- **webpj**: AGENTS가 14차를 제외했다(플랫폼이 JS를 지원).

## 7. 남은 일

- **D7:** 연동 범위(② 개발용 진단 평가, ③ 게임 상태/명령 노출, ④ 미션 대본 확장)는 사용자 결정 뒤. 넓힐 때 노출 API·파일/네트워크 차단·스크립트 위치(사용자 홈 아래)를 함께 정한다.
- **M8 최종 관문:** 이번 단위로 코드·배포 구성이 바뀌었으므로 최종 관문에서 ZIP을 새로 만들고 `verify_cpp_package.py`로 QuickJS 라이선스 동봉까지 대조한다.
- ARM64 실기 실행 확인(사용자 환경).
