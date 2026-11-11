# QuickJS-ng JavaScript 엔진 (14차 탑재)

AGENTS.md 14차 요구("QuickJS 지원 — 일단 QuickJS 엔진 탑재만 하고, 어느 정도까지 연동할지는 추후 고민")에 따라 NetStorm 11.01 C++ 판(cpppj)에 넣은 [QuickJS-ng](https://github.com/quickjs-ng/quickjs) 엔진이다.

## 출처와 고정 판본

| 항목 | 값 |
| --- | --- |
| 저장소 | <https://github.com/quickjs-ng/quickjs> |
| 릴리스 | `v0.18.0` (태그, 2026-11-01) |
| 커밋 | `d7215b0ad3fc2c562e8317fd9bca058fd4863804` |
| 라이선스 | MIT (`LICENSE`: Copyright (c) 2017-2026 Fabrice Bellard, 2017-2024 Charlie Gordon, 2023-2026 Ben Noordhuis, 2023-2026 Saúl Ibarra Corretgé) |

- 상류 파일 23개(엔진 소스 4·헤더 14·생성 헤더의 원본 JS 3·`LICENSE`·`README.md`)는 공식 저장소의 로컬 클론에서 위 태그를 `git show`로 꺼내 **바이트 변경 없이** 두었다. 파일별 URL·SHA-256·크기는 `provenance.json`에 있다.
- 빌드 중에는 아무것도 내려받지 않는다. 저장소 `.gitattributes`의 `cpppj/third_party/quickjs-ng/** -text`가 Windows 체크아웃(core.autocrlf)에서도 바이트를 보존한다.
- `CMakeLists.txt`, `provenance.json`, 이 `README.ko.md`는 프로젝트가 추가한 빌드·설명 자료다.

## 빌드 구성

`NetstormQuickJs` 정적 라이브러리(상류 `qjs` 대상과 같은 소스 4개: `quickjs.c`·`dtoa.c`·`libregexp.c`·`libunicode.c`)로 만들어 게임 실행 파일 `Netstorm.exe`에 링크한다. 별도 DLL은 없다.

- C11, 상류 Windows 정의(`_GNU_SOURCE`·`WIN32_LEAN_AND_MEAN`·`_WIN32_WINNT=0x0601`), 상류 MSVC 경고 억제 목록을 그대로 쓴다.
- MSVC 추가 옵션: `/utf-8`(저작권 주석의 UTF-8 문자를 cp949 등 시스템 코드 페이지로 잘못 읽지 않게), `/experimental:c11atomics`(상류와 같음), `/wd4098`(상류 코드의 무해한 경고 4곳).
- **quickjs-libc는 넣지 않는다.** 그래서 스크립트에는 `std`/`os` 모듈·파일·소켓·환경 변수·워커가 없다.
- 포함 경로/정의는 이 대상의 PRIVATE로만 둔다. 엔진 헤더는 `src/platform/ScriptEngine.cpp` 한 파일에만 알린다.
- VS 2026(18.x) MSVC 14.51로 **x64와 ARM64 모두 경고 없이 빌드됨**을 확인했다(ARM64는 교차 빌드만, 실행은 ARM 실기 필요).

이 폴더만으로 같은 라이브러리를 다시 빌드할 수 있다.

```powershell
cmake -S . -B build -A x64      # ARM64는 -A ARM64
cmake --build build --config Release
```

## 게임 쪽 사용 범위 (현재)

`src/platform/ScriptEngine.{h,cpp}`의 `netstorm::platform::ScriptEngine`이 런타임·컨텍스트 한 쌍을 소유한다.

- 자원 한도: 메모리 16MiB(엔진 할당자 콜백이 직접 세고 거절), JS 스택 256KiB(게임 주 스레드 기본 스택 1MiB보다 작게), 평가 1회 1초(엔진 인터럽트).
- 게임 상태·파일·네트워크·OS 기능은 스크립트에 **노출하지 않는다**. 모듈 적재기도 없어 `import()`는 거부된다.
- 게임 시작 때 엔진을 한 번 만들어 고정 식 6개를 평가하고 해제한다(자체 검사). 결과는 7차 활동 기록에 남고, 실패하면 경고 로그 파일에 상세가 남는다(게임은 계속 시작).
- `Netstorm.exe --quickjs-selftest`는 창 없이 같은 자체 검사를 실행해 결과를 출력한다(통과 0, 실패 1).

연동 범위를 넓히는 일(개발용 진단 평가, 게임 상태 읽기·명령 노출, 미션 대본 확장 등)은 `LEFT_JOBS.md`의 결정 **D7** 뒤에 한다.

## 배포

배포 ZIP(`cpppj/package-cpppj.ps1`)의 `third-party\quickjs-ng\`에 `LICENSE`·`README.ko.md`·`provenance.json`을 넣는다. MIT 라이선스의 저작권·허가 고지 동봉 조건을 지키기 위해서다.

(2026-11-11 14차 QuickJS 엔진 탑재로 신규)
