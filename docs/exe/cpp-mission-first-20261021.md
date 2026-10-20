# 현재 미션 F8과 First 안내 재표시

단위247의 시작 커밋은 `f396a821`이다. 현재 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-20**로 일치한다. AGENTS와 양 인수인계를 다시 읽었고 새 디컴파일, 원본/CD/AGENTS 변경은 없다. 전체 지정11미션 목표는 진행 중이다.

## 원본 경로와 이번 수정

원본 `Guide.cpp`의 `00463bb0`은 현재 미션의 순번/가상 단계를 관리한다. 프레임 관문 `004c79a0`이 보는 `005cab2c`는 `004cf120` Tell 창 생성에서 증가하고 `004cf1c0`에서 감소하는 창 수다. 이를 별도 Guide 스택으로 추정해 항상 false로 공급하던 Client 조회를 실제 Dialog/Briefing 상태 조회로 바꿨다. 현재 클론은 Tell 창 한 개를 표시하는 어댑터이므로 이 상태가 현재 창 유무다. 원본의 모든 중첩 Gump 수명 구현을 주장하지 않는다.

원본 key-down F8은 Ctrl/Shift가 없고 `00594fa0`의 활성 미션이 있으면 `00542938 = 02000091`의 **MissionFirst/0/0**을 `00452fe0`으로 예약한다. 즉시 표시하는 함수 호출이 아니다. Guide 사건 `00463e40`은 상위 modifier/release 비트를 제거한 명령을 먼저 읽고 이 명령이면 현재 미션의 가상+4를 호출한 뒤1을 반환한다. 현재 미션이 없어도 명령은 소비한다. 하위 사용자 입력의 release 거부는 그 뒤에 있으므로 명시 First 명령의 release 표시와 물리 F8 key-up을 구별한다.

가상+4 `004c2a70`은 순번0이면 flags bit1만 켠다. 순번이 비제로면 `004c2a90` 현재 안내를 다시 연다. `004c2a90`은 demo가 비제로면 `Demo`, 아니면 현재 단계의 낮은 char와 `.`을 NUL 종료 문자열로 만든다. 낮은 char0이면 실제 문자열은 빈 문자열이다. Tell의 반환이 비제로일 때만 팝업 Kill→가상+28 정지를 실행하고 마지막에 원래 반환 DWORD를+b4에 쓴다.

새 `client/MissionGuideInput`은 이 First 분기와 현재 안내, 활성 미션의 F8 예약을 구현한다. 기존 Early1~6의 같은 장기 `MissionStartState`, frame gate, Tell, 팝업, 입력 큐, 자기 정지 깊이에 연결했다. 재표시는 순번/단계를 진행시키거나 별도 정지 깊이를 늘리지 않는다. 기존 TellStage도 같은 원본 몸체를 사용한다.

## 제품 작성 전 독립 관측

`tools/decomp_mission_first.py`는 실제10.78 메모리 사본에서 `004c2a70`, `004c2a90`, `00463e40`을 실행했다. `004c2a70`은 현재 정밀 함수 표에서 누락되어 있어 실제 가상표 `005149f0`과 capstone으로 확인한17바이트 범위 `004c2a70..004c2a80`을 별도로 허용한다. 이를 새 디컴파일 결과로 덮어쓰지 않는다.

고유 입력278개는 **정상 함수 반환264개 + F8 내부 분기 관측14개**다. 실제 명령7,407개, First210회·현재 안내153회·Guide 사건72회다. F8은 `004d9611` 내부에서 시작해 `004d9650/004d9670`에 도달하는 제한된 명령 관측이다. 전체 UserInput 함수 진입/정상 반환으로 합산하지 않는다. 이14개는 key-down F8/활성 미션을 전제로 modifier와 활성 DWORD를 교차한다. release·다른 키·비활성/도전·편집·디버그 효과는 native 범위 밖이다. release 거부 위치는 전체 정밀 원문의 `if ((int)uVar6 < 0)`을 따로 확인했다.

Tell `00482de0`, 팝업 Kill `00466710`, 가상+28 정지, 가상+24 하위 입력, 큐 `00452fe0`은 명시 효과 경계다. Guide 사건은 First와 미처리 입력만 관측하며 Begin/Abort/Reload/Fork/Finish 명령 몸체를 이번 근거로 주장하지 않는다. 원본 함수는 명시 미션 flags/마지막 안내 결과와 제한된 스택 이외에 쓰지 못한다. 전체 미션208바이트, 반환 DWORD, 섹션 바이트, 효과 순서/인자를 보존했고 정상 반환 주소·스택 정리·비영 EBX/EDI 보존을 확인했다. 게임/OS/창 실행은 없다.

최초 fixture/도구/보고서는 `extracted/mission-first-20261021-first-capture/`에 제품 작성 전에 복사했다. 추적 fixture SHA-256은 `31fd6fbe01a483a0557f48d2ca7dddf244614e2a0e44ee9c76e32e900800e794`이며 최초 전체 바이트와 같다. 기존 Guide1,550/공통 프레임328 등 관측은 덮어쓰지 않는다.

## 검사와 남는 범위

새 `MissionGuideInput_Native278`은 위 원문 전체 상태/반환/순서를 비교한다. `MissionGuideInput_QueueOrderCapacityAndRelease`는 실제40칸 큐의 FIFO/39개 포화 폐기와 key-up/비활성/다른 키 관문을 검사한다. 기존 실제 Early1 로그인/영어 Start 검사는 B 안내에서 F8 예약→First 분배→반복 표시→동일 정지 깊이/단계/순번 보존을 추가했다. **이 C++검사는 작성·컴파일만 하며 실행하지 않는다.**

Release 빌드 로그와 현재 소스/보호 원본/최초 fixture SHA, 현재 핀 역변환 결과는 `cpppj/mission-first-session.json`과 `tools/cpp_mission_first_audit.py --verify`에 기록한다. 기존 보고서의 현재 소스 핀만 후속 이력으로 갱신하고 전체 JSON을 시작 커밋으로 역변환해 과거 files/원본/기대값/실행·바이너리 결과를 보존한다.

최종 전체 Release 빌드는 **경고0·오류0·종료0**이다. 로그는 `extracted/cpp-mission-first-20261021-build.log`이며 라이브러리·게임 EXE·검사 EXE를 생성했다. C++ 검사·CTest·실제 게임 실행 결과로 바꾸어 기록하지 않는다.

현재 핀은 **119보고서·207항목**이며 전체 JSON 역변환이 시작 커밋과 일치한다. 이번 native/현재 연결 감사와 직전 Dude 정기 감사는 종료0이다. 부가 `cpp_tutorial_later_runtime.py --verify`는 `RawTutorialLater.cpp`의 기존 CRLF 핀 때문에 종료1이다. 시작 커밋에도 같은 CRLF 핀 `1f40efc3…`가 있고 현재 소스는 LF `3ff41fc6…`다. 시작 커밋의 코드와 현재 코드가 줄 끝 정규화 후 전체 바이트로 일치하고, 현재 코드를 CRLF로 변환하면 기존 핀과 일치함을 읽기 전용으로 확인했다. 이번에 해당 소스나 역사 핀을 바꾸지 않았으며 이 부가 감사를 새 통과로 기록하지 않는다. `mission-first-session.json.supplementalVerification`에 구분했다.

현재 연결 범위는 Early1~6이다. 캠페인1-1/1-2와 데모3개의 공통 프레임/F8, Guide 전체0..6 수명/전환·중첩 창, 다른 키와 도전/편집/디버그 모드, 실제 UI/플레이/원본 동일성은 남는다. 현재 수동 진입 어댑터의 순번을 임의4로 고쳐 Guide 전체 수명을 복원한 것처럼 표시하지 않는다. 지형 Devastation, 포획/Drop, 제단/희생, 전략 AI와 전체 지정 미션 완주 검증도 계속 필요하다. 사용자 지시대로 게임/C++검사/CTest/전체 회귀는 후속 통합 실제 플레이 검증에 이월한다.
