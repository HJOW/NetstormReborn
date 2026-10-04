# cpppj — NetStorm C++ 빌드

기존 게임(NetStorm: Islands at War)을 디컴파일한 소스를 토대로 C++ 소스를 다시 만드는 프로젝트다.
C# + MonoGame 빌드(`../dotnetpj/`)와 함께 개발한다.

**지금은 기초 프로젝트 구조만 있다.** 창을 띄우지 않으며, 실행 파일은 빌드 정보만 출력하고 끝난다.

## 빌드와 테스트

CMake 3.21 이상과 C++20 컴파일러(Visual Studio 2022 이상, GCC 11 이상, Clang 14 이상)가 필요하다. 저장소 루트에서 실행한다.

```sh
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

실행 파일은 Windows 에서 `cpppj/build/bin/Release/NetstormCpp.exe`, Linux 에서 `cpppj/build/bin/NetstormCpp` 에 생긴다.

## 폴더

| 폴더 | 내용 |
|---|---|
| `src/o/` | 원본 `\Ns\O\` 의 공용 모듈 (게임 규칙·데이터·프로세스 커널) |
| `src/client/` | 원본 클라이언트 모듈 (화면·입력·소리·메인 루프) |
| `src/zacket/` | 원본 `\Ns\Zacket\` 의 네트워크 패킷 |
| `src/platform/` | 새로 쓰는 코드 — 원본의 Win32·DirectX 호출을 대신하는 계층 |
| `src/app/` | 새로 쓰는 코드 — 실행 파일 진입점 |
| `tests/` | 단위 테스트 (`TestSupport.h` 의 `TEST_CASE`·`CHECK`) |
| `cmake/` | 공통 컴파일 옵션 |

원본 소스 파일이 어느 경로로 가는지는 [SOURCE_MAP.md](SOURCE_MAP.md)에 있다(`python tools/cpp_source_map.py` 로 생성).

## 소스를 옮기는 방법과 규칙

[docs/cpp-build.md](../docs/cpp-build.md)에 정리했다: 디컴파일 결과에서 C++ 소스를 만드는 절차, 출처 주석 규칙, 32비트 코드를 64비트로 옮길 때의 주의, 아직 정하지 않은 것.
