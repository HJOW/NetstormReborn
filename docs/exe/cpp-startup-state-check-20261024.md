# startupStateCheck 부모의 독립 복원

2026-10-24 단위385 후속 준비다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 진행 중인 단위384 공용 소스/CMake/빌드/lib에는 연결하지 않았으며 게임/GUI/실제 저장상태 파일을 실행·수정하지 않았다.

## 최초 원문과 범위

[도구](../../tools/decomp_startup_state_check.py)는 기존 [Editor 외곽 oracle](../../tools/decomp_existing_editor_install_outer.py)의 명시 Config/저장상태 reader/시계/번역/format/Tell 장치만 재사용한다. 실제 진입은004b5ba0이며 전체249BYTE 부모의 정상ret 또는 선택 하위 중단까지 실행한다. 이전 외곽258관측을 새 관측으로 더하지 않는다.

제품을 작성하기 전에 [fixture](../../cpppj/tests/fixtures/startup-state-check-x86.tsv)와 [보고서](../../cpppj/startup-state-check-native.json)를 고정했다. **195입력×두x87정밀도=390관측/12430명령/정밀도차0**이며 정상ret374,설정·reader·시각·번역1/2·format1/2·Tell 중단 각2다. 옵션0/1/ffffffff,저장 signedDWORD 양끝·음수·0,벽시각±0/Inf/NaN/경계 이웃,엄격0/45분과reader의pending 부분효과를 포함한다. 최초 도구/fixture/report를 덮어쓰지 않으며 `--verify`가 보호 PE/정밀 C/기존 helper/부모 oracle/새 도구/fixture SHA를 검증한다.

772BYTE의 실제 saved-state reader004b5890은 명시 하위다. Player9/이름/CoreData/게임시간/State 파일의 복원을 완료한 관측이 아니다. 실제 번역/printf/임시 Tell도 명시 경계이며 독립 문자열 `ExplicitButtons`/`ExplicitWarning`을 사용한다.

## 제품 순서와 검증

[RawStartupStateCheck](../../cpppj/src/o/RawStartupStateCheck.h)는 Config.GetInt0에서만 즉시0을 반환한다. 비영이면 실제reader를 호출하고 반환값0이어도 현재 벽시각을 읽는다. signed 저장DWORD를double로 읽어 현재double에서 빼고,원문double1/60을 곱한 뒤float32로 저장한다. 저장시각비영 및엄격0<분<45에서만 Yes/No 번역→버튼format→질문번역→float32분의double승격/버튼format→Tell(text,0,0)→1 반환을 수행한다. 선택하지 않은 장치를 미리 요구하지 않고 선택 하위의 예외/부분효과는 전파한다.

[신규2검사](../../cpppj/tests/StartupStateCheckTests.cpp)는 390관측 전체의 정상ret EAX,호출·정확한double인자비트와현재pending을 대조한다. 중단시 EAX는 host ABI가 달라 최초 분석자료로 보존하되 C++ 대조에서 제외한다. 기존 oracle이 진입에 붙인 고정 debug항목 `STARTUP:editor=1`도 실제 원문 외부호출이 아니므로 대조 trace에서 명시 제외한다. 추가 검사는 옵션0의 미선택 장치 생략,비영의필수reader누락,reader0뒤벽시각호출을 검증한다.

격리 `extracted/cpp-startup-state-check-20261024-isolated`의 build2는 MSVC14.51.36231/WindowsSDK10.0.26100.0을 직접 지정해 **종료0/경고·오류0**,test1은 **2검사 실패0/종료0**이다. 새 모듈/검사/Runner만 컴파일했고 공용lib를 사용하지 않았다. 처음 build1의vcvars64 배치는 출력·자식 없이 대기해 해당cmd만 정확히 확인하고 중단했으며 로그/종료-1은 보존했다. 코드 오류로 판정하거나 첫 실패를 숨기지 않는다.

공용 등록과 실제 Client Config/번역/formatter/Tell 바인딩은 후속이다. 옵션값을0으로 강제하지 않으며, 비영에서 실제reader가 없으면 필수 오류로 남겨야 한다. 전체 Editor 설치나 재접속/StateReconnect/DelStateFile 구현 완료로 판정하지 않는다.
