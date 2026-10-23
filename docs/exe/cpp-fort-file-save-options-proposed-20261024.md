# Fort 저장 관문 옵션의 Client 연결 제안

현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 단위379 `3a62aded`·설명 복구380 `5166c972` 이후 **단위382용 제안 patch**를 만들었다. 하위 작업자는 기존 제품 공유파일·원본/CD·AGENTS·LEFT·CMake·공유 build/lib·git를 변경하지 않았고 GUI/게임도 실행하지 않았다. [앞선 전역 조사](cpp-fort-file-save-globals-20261024.md)를 바탕으로 `extracted/cpp-fort-file-save-options-20261024-proposed.patch`를 동결했으며, **단위381 `fa7762f9` 완료 뒤 root가 이 patch를 공유 소스에 적용했다.** 아래 공유 SHA 불변 검사는 적용 전 격리 당시 범위이며 적용 뒤의 공유 소스를 이전 SHA로 재검사하지 않는다.

## 두 파일의 최소 변경

`ClientMain.h/.cpp`에 Client 수명의 **실제 `std::uint32_t readOnlyForts_{}` 한개**와 `const std::uint32_t& ReadOnlyForts() const`를 제안한다. 최초0은 원본 `0054db30`의 PE 가상0 영역에 근거한다. 이후 미션 교체/누락 키/저장 호출마다0으로 다시 만들지 않는다. 저장 소비자는 이 참조를 `Session::BindFortFileSave(client.ReadOnlyForts(),실제EditorFiles)`에 빌리며 값복제나 저장 때 Config를 조회하는 우회를 쓰지 않는다. 이 patch 자체에는 Session/파일 장치 연결이 없다.

`InterpretFortFileOptions()`는 현재 DWORD를 `bit_cast<int32_t>`한 작업값으로 `ReadInt("readOnlyForts",...)`하고 결과를 같은 DWORD에 `bit_cast<uint32_t>`한다. 누락이면 현재값을 보존한다. 빈 문자열은 존재하는 키의 `atol=0`이므로 누락과 다르다. -1/2/80000000 등 모든 비영 DWORD를 저장 억제로 유지하며 bool로 축약하지 않는다. 초기 `InterpretOptions()`도 같은 helper를 sleepPerLoop/maxFPS 앞에서 부른다.

## 원본 시점과 좁은 callback의 차이

PE 실제 `00435860 push 0054db30`→`00435865 push "readOnlyForts"`→`0043586a call 00441270`이고 일반 ReadInt의 실제 쓰기는 `004412dd mov [esi],eax`다. 파일 저장은 `0044d915 cmp [0054db30],ebx`에서 현재 DWORD를 읽는다. 정밀 C의 중복 블록을 여러 쓰기로 세지 않는다.

원본 `00440e00`은 시작 설정 Load에서 전체 `Interpret Options00435220`을 `00558dd0`에 등록한다. 실제 Set은 `00441500 call 004404e0`로 기록한 **뒤** `00441507`에서 현재 callback을 읽고 `00441510 call eax`로 동기 호출한다. SetInt/Config 명령도 같은 기록 뒤 시점이다. 제안은 생성자에서 callback을 설치하므로 현재 제품의 `Startup.Load→Set(InstallDir)→Set(CDDir)` 두 알림과 모든 후속 Set/SetInt/Config가 반환되기 전에 같은 readOnly DWORD를 갱신한다. 직접 Load 자체는 기존처럼 알리지 않으며 초기 명시 InterpretOptions를 유지한다.

**이번 callback은 readOnly helper만 실행한다.** 원본 callback은 언어/해상도/소리 등 전체 해석을 수행한다. 그 전체 순서·다른 전역의 재설정·등록 창 메시지의 설정 재적재와 상태 정리 뒤 직접 재해석까지 복원했다고 하지 않는다. 현재 `ChangeResolution`은 장치를 만든 뒤 SCREENW/H를 연속 Set하며, 전체 InterpretOptions를 연결하면 첫 Set에서 새 폭/옛 높이를 읽어 창을 다시 움직일 수 있다. 실제 해상도 Yes의 두 Config도 종료 전 화면 상태를 바꾸게 된다. root와 합의하여 이 기존 동작을 근거 없이 확대 변경하지 않는 좁은 제안을 택했다. 원본 전체 해석 내부에서 readOnly가 읽히기 전 다른 효과가 실행되는 순서는 이번 범위에 없다.

## 소유권·설치·해제

- 설정은 각 Client가 새 `ConfigInterface`로 소유한다. 현재 제품의 `ConfigInterface::onChanged` 설치자는 없고 검사 두 곳은 각자 독립 Config에 설치한다. 제안은 새 빈 슬롯에 **한 번 대입**하며 기존 callback을 합성하거나 저장하지 않는다. public 슬롯을 후속 코드가 덮어쓰면 이 연결이 사라지므로 다른 설치자가 생길 때는 Client의 한 dispatcher로 통합해야 한다. 그런 덮어쓰기를 자동 방지하는 API 변경은 이 제안에 없다.
- 설치는 archive/환경 공급자 구성과 `g_client` 중복 검사 뒤, `g_client=this` 앞이다. std::function 대입이 실패해도 전역에 미완성 Client를 남기지 않는다. 설치 시 helper를 직접 실행하지 않으며 Startup의 실제 Set/초기 해석이 읽는다. 생성자가 끝난 뒤의 선행 Set에도 Win32/화면/자산 없이 안전한 좁은 helper만 실행한다.
- 소멸 첫 문장에서 `onChanged={}`로 해제하고 menu/Session을 반납한다. readOnly DWORD는 그 반납 동안 살아 있으며 실제 Session callback은 자신보다 오래 사는 Client 필드를 빌려야 한다. 현재 menu 소멸은 OnExit를 입력 큐에 예약하고 같은 소멸 중 실행하지 않는다. 이 해제는 C++ 객체의 부분 소멸 재진입을 막는 수명 처리이며 원본 process-global callback에 같은 해제 명령이 있었다는 주장이 아니다. Config/참조/callback을 Client 밖으로 탈출시키면 안 된다.

## 검증 범위

전용 복사본은 `extracted/cpp-fort-file-save-options-20261024-copy`다. `prepare.py`는 두 공유 소스의 최초 바이트를 보존하고 복사본에서만 변경한다. `verify.py`는 공유 SHA 불변, 설치1/해제1, 생성자 등록 순서/소멸 첫 해제, 실제 DWORD/참조 getter를 검사한다. `ChangeResolution`, `SaveOptions`, `Run`, `HandleMessage` 본문은 변경 전과 같다. 실제 제안 Client 전체는 독립 `/Zs /std:c++20 /utf-8 /W4 /WX` 구문검사 통과이며 CRT 경고 정의는 공용 CompilerOptions와 같다.

실행 계약은 제안 복사본의 **실제 helper/getter/설치·해제문/초기 호출을 그대로 추출한 어댑터**와 수정하지 않은 Config/ConfigInterface/ApplyScriptConfig/토큰/입력큐/인코딩 구현을 자체 링크한다. 완전한 Client를 생성/실행한 검사가 아니다. 최종 실행 결과는 아래 고정 보고서에 기록하며 공유 제품·GUI·OnExit·실제 디스크 저장 성공으로 확대하지 않는다.

격리 최종 구문/링크/실행은 **종료0·경고오류0·10 계약군/실패0**이다. 실제 Startup/동기 Set·SetInt·Config, 전체 DWORD 경계10개, 누락 재적재 보존, 현재 Registry, 해상도 연속 Set, 옵션 암호화 바이트의 메모리 왕복800×600/readOnly1, 해제 뒤 재진입 차단과 독립 소유자0을 확인했다. patch 두 파일의 문맥/행 수를 메모리에 적용한 결과도 제안 복사본과 같았다. patch SHA-256은 `7770caf250f58b4abe240583abeef6d1dee766c3258a05730bd7aa8e8fb74b77`, 최종 격리 실행파일은 `8f9916bfba62ec9eaa1bfc238c6d27aca8b6b4132a78e3759ad02d363e8241b2`다. 독립 읽기 검토도 현재 callback 충돌/수명·누락 보존의 명백한 오류를 찾지 못했다.

root 요청으로 새 `cpppj/tests/ClientFortFileOptionsTests.cpp`를 별도 추가했다. 기존 ClientPause와 같은 **Run 없는 실제 Client 생성/소멸**을 사용해 실제 const 참조/동기 Set, Client 입력 큐의 Config, Startup/누락 재적재, 중복 Client 거부와 새 수명0을 검사한다. 다섯째 검사는 root가 실제 Client에 연결한 Badges 장부의 주소/전체6160BYTE가 Config 뒤 보존되고, 첫 shared_ptr를 유지한 채 만든 새 Client는 별도 장부/count0을 갖는지 구별한다. 최초 장부의 count 변경은 명시 검사 입력이다. Run을 부르지 않으므로 시작 지도 초기화 지점의 ResetCounts 실행까지 확인하지 않는다. **새5검사 소스 자체 `/Zs /W4 /WX`는 종료0·경고오류0**이며 공용 등록/링크/실행은 root가 담당한다. 위 추출 어댑터10군과 합산하지 않는다. Client 전체 생성/소멸을 확인해도 실제 Session/GUI/파일 저장/OnExit 성공 검증을 대신하지 않는다. 최종 기계 판독 기록은 `extracted/cpp-fort-file-save-options-20261024-copy/verification.json`이다.

최초 실행의 named mission 객체를 비접두 키 덮어쓰기 대상으로 둔 기대와, 옵션 본문 부재/중복 END 입력의 저장 왕복 기대는 잘못된 검사 입력이었다. 익명 상위 객체와 실제 시작과 같은 유일 `[options.cfg]→[END]` 입력으로 검사만 교정했다. 제안 patch/기존 Config 정책은 바꾸지 않았다. UTF-8 batch/CRT 정의/응답파일 끝 역슬래시의 최초 실행 문제와 실패 로그도 전용 경로에 보존한다.

## 적용 뒤 필요한 실제 회귀

현재 제안 단계에서는 **resolutionNo/Yes·OnExit·실제 options.cfg 디스크 저장은 미검증**이다. root 적용 뒤 실제 Client에서 -1/1/0/누락 재해석, Config 직후 참조값/주소 불변, Session의 같은 참조 소비와 장치/Client 해제 순서를 확인한다. 기존 `ConfigInterface`·`ScriptCommand`·`ConfigEncoding`·`DialogCommandLifetime` 관련 검사를 다시 사용한다. `tools/cpp_config_smoke.py`의 설정 저장/실패복구, `tools/cpp_dialog_exit_smoke.py`의 No 유지와 Yes→SetSize2→Config SCREENW/H→저장→지연 OnExit QuitApp99/복원 전 실제800×600 저장 확인, `tools/cpp_game_menu_smoke.py`의 ESC/해상도No/음량/키 회귀를 같은 최종 제품으로 순차 확인해야 한다. 입력 뒤 SaveOptions가 Cleanup/OnExit 예약보다 앞서는 현재 Run 순서와 종료에 새 저장을 추가하지 않는 정책은 유지한다.

원본 PE SHA-256 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`, 정밀 C `af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646`이다. 이번에는 새 원문 실행 fixture를 만들거나 원본/게임을 실행하지 않았다.
