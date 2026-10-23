# 점 명령 표의 실제 정적 데이터와 장기 소유자

2026-10-24 후속 단위386 준비다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 공용 빌드·CMake·실제 Client/Run/Session과 분리한 신규 모듈이며 게임·GUI를 실행하지 않았다.

## 제품 전에 고정한 정적 PE 데이터

원본00531b30부터2568BYTE는 이름64×32·콜백64×4·표시65×4·count4BYTE의 인접 물리다. PE 초기 count는 **1**이고 첫 이름은 `say`, 콜백 식별자는 `0042f1e0`, 표시0은 `00531b30`이다. 나머지 항목과 종료 포인터는0이다. 명령 등록을 count0의 빈 표에서 시작하면 실제 공통 초기화와 달라진다.

최초 정적 읽기 [원시 JSON](../../extracted/cpp-dot-command-register-20261024-initial-table.json)을 먼저 보존했다. 새 제품 소유자 작성 전에 [정적 추출 도구](../../tools/audit_dot_command_static_image.py)를 실행해 [전체 hex](../../cpppj/tests/fixtures/dot-command-static-image.hex)와 [정적 보고서](../../cpppj/dot-command-static-image.json)를 동결했다. 이후 `--verify`는 PE·도구·fixture의 최초 SHA를 대조하며 덮어쓰지 않는다. 두 정적 읽기의 전체2568BYTE는 동일하고 binary SHA는 `082954f8c0b846f0765c17214baa8f02972f082c7552d1a0136ec65ba58b7a5f`다.

이 자료는 **정적 PE 데이터**다. [원문 등록 함수의 최초250/보충12관측](cpp-dot-command-register-correction-20261024.md)과 합산하거나 정적 초기화 자체를 x86 실행 관측으로 세지 않는다.

## 같은 표의 수명과 검증

`ClientDotCommandGlobals`는 실제2568BYTE 한 배열을 소유하고 복사·이동을 금지한다. 최초 PE 비영10BYTE와 영 초기화 나머지를 구성하고, 현재 count를 별도로 저장하지 않는다. `Register`는 같은 배열을 실제 `RegisterDotCommand`에 전달하므로 signed 관문·copy 뒤 count 단일 캡처·인접 표 중첩·부분중단은 원문 등록 몸체를 그대로 사용한다. 원문 함수/표시 주소 값은 식별자이며 host 함수 포인터로 실행하지 않는다.

독립 검사3개는 전체 정적 이미지, 두 소비자의 등록과 최초 say 보존, 새 Client와 기존 표의 독립성, copy 뒤 중단의 부분 바이트 보존을 확인한다. MSVC `/std:c++20 /EHsc /utf-8 /W4 /MD`로 신규 owner·Raw 등록 부모·검사·격리 Runner만 직접 링크했고 최초 빌드/실행은 모두 종료0·경고오류0·**3검사 실패0**이다. 공용 라이브러리는 사용하지 않았다. 원시는 `extracted/cpp-dot-command-static-image-20261024-isolated/`에 보존한다. 동료 읽기 검토에서도 명백한 순서·수명 오류가 없었고 두 정적 자료 전체 일치를 독립 확인했다.

공용 등록 시 `NETSTORM_DOT_COMMAND_STATIC_IMAGE_FIXTURE`에 실제 hex 절대 경로를 지정한다. 현재는 공용 CMake 미등록이다. 실제 Client 멤버·공통 Chat 초기화·명령 실행 callback의 host 분배는 별도이며 이 소유자만으로 완료 처리하지 않는다. 최초 say 콜백을 임의 성공 장치로 채우지 않는다.
