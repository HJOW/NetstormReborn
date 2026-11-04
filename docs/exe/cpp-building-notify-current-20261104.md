# NotifySurface의 현재 TYPE와 원문 진단 복귀

단위850 완료 기록이다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03** 일치다. 직전849는 `7386bc03`이며 검증 제품은850/11.01이다. 전체 목표는 미완료다.

PATCH004214a0의 정밀 범위4214a0~4214f7을 실제 PE와 대조했다. 현재 Bridge DWORD와 raw TYPE가 같고 extra8이 없으면 낮은 BYTE의 `(value>>3)&15`를 먼저 읽는다. checking이 비영이고 값이7보다 크면 식 `num >= 0 && num <= 7`, 파일 `\Ns\O\Bridge.cpp`, 줄ff를 진단 장치에 전달한다. movzx/and 뒤 음수 분기는 도달하지 않는다. 진단에서 복귀하면 재검사나 추가 쓰기 없이 반환한다. CD00448c10의 정밀 범위448c10~448c21은 SID×36+pool 포인터를 반환할 뿐 raw/진단에 접근하지 않는다. 상위 호출자는 반환값을 버리므로 기존 C++ void API에서 이 차이를 유지한다.

RawBuildingSpace가 같은 World BridgeState의 현재 TYPE와 현재 checking 뷰를 읽도록 연결했다. 독립 기존 state 값 참조는 유지한다. 진단은 도달할 때만 요구하고 callable을 호출 동안 보존하므로 진단 중 자체 연결을 반납할 수 있다. 첫 World는 같은 Storage의 현재 Client 장치를 매번 조회하며 Session은 임시 Session 주소를 보관하지 않는 실제 서비스 callable과 같은 Gameplay checking owner를 사용한다. 서비스 owner/대여 TYPE/뷰는 해당 실행기보다 오래 살아야 한다. 다른 TYPE/모드 전역 전체를 공유한 것은 아니다.

새 격리 실행기는 두 원문 함수 범위와 명시 진단 장치만 허용한다. 총2264행을 서로 다른 두 FPU 제어 입력에서 독립 관측4528회 했고 전체를 재관측하여 TSV가 byte 단위 일치했다. PATCH1132행 중 진단164회/명시 중단6행이고 CD1132행은 진단0이다. TYPE/전체 Bridge DWORD·extra·checking 상위 비트·free/예약 SID·모든 낮은 BYTE를 교차하며 진단은 raw/전역 변경 후 정상 복귀하거나 명시 중단한다. raw 슬롯 전체/128슬롯 전체 Adler·trace의 원문 문자열/현재 DWORD와 복귀를 저장했다. CD checking은 원문이 읽지 않는 명시 장치 입력이며 실제 반환 포인터는 관측 후 C++ void API 비교에서 제외한다. 기존 fixture는 바꾸지 않고 새 `bridge-notify-x86.tsv`만 추가했다.

새 C++9개는 Native2264행 전체와 실제 옆면 쓰기 뒤 진단 복귀/중단·현재 장치 수명/값 대여·CD 미도달 SID·같은 실제 World/Session 연결을 확인한다. 실제 Session은 명시 일반 슬롯/현재 TYPE와 서비스가 바꾼 raw/checking의 정상 복귀만 사용하고 입력을 복구한다. 게임·Frame/Process 틱·생산/행동·적 공격·포획/희생·승리·MP·진단 UI는 실행하지 않는다. 집중119개/27묶음(새9)이 첫 실행에서 모두 통과했다. Release 제품/두 집중 대상 빌드1회 종료0·컴파일 경고/오류0이며 핸들39969의 종료와 자식 해제를 확인했다. 원문2264행 전체와 기존 옆면128장면/공간 단계, Asset376행·Factory372행·Carrier7344행의 입력/기대값도 유지했다.

첫9소스/원문/관측·원시는 Git 제외 `extracted/cpp-resume-850/`에 보존한다. 기존2759개/fixture939개는 불변이며 새 보호 합계2760개/fixture940개다. 기존779의51함수 집계 밖의 부속 연결이므로 checking12·완료부모36/부분3 유지다. 다른 현재 가상20 파생/전체 Pop과 상위 수명·다른 TYPE/모드 전역 및 LEFT_JOBS의 잔여는 남는다. 사용자 직접 확인·실제 포획/희생/특정 적 공격/승리·공중3종 전용·신규MP/통신·물리/PCM/CD·전체71/CTest/34미션/ZIP 반복을 제외한다.

기존2759개/fixture939개는 최종 검사 뒤에도 불변이다. 새 fixture를 포함한 보호 합계2760개/fixture940개와 첫9소스의 빌드 중 불변을 확인했다. 제품 EXE SHA-256은 `f55aecdafdf8f7e396fc8cd8ea2adfa27014b89764f2c0830bb0f2ae6faf4301`이며 [기계 판독 기록](../../cpppj/building-notify-current-20261104.json)에 소스/함수/관측/실행 로그 해시를 보존한다. 부속004214a0의 현재 TYPE/진단 복귀는 완료했으나 기존779의51함수 집계와 별도이므로 checking12를 줄이지 않는다. 다음851은 다른 현재 가상20 파생 자체 검사/현재 부모 연결이며 전체 목표는 active다.
