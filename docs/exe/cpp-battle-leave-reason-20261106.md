# LeaveBattle 양수 이유와 CD판 차이 — 단위955

2026-11-06, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 HJOW-Athlon/2026-11-03이다. 정밀 산출물은 수정하지 않았다. 사용자 작업순서 정정에 따라 LEFT_JOBS 3절3번 셋째 항목의 LeaveBattle 나머지 이유/CD·State6 분류를 진행했다.

## 원문과 실제 소비자

PATCH10.78 `004d803e..004d80d9`의 양수 이유 본문을 `PrepareBattleLeaveState`로 묶고 실제 `UberGump::Event`가 같은 Client 전역과 사용자 저장 삭제 장치를 전달하도록 했다. 기존 인라인 본문의 동작을 보존한 추출이며, 이번 대조에서 새 제품 결함이 발견됐다고 주장하지 않는다. 앞선 뗌 거부/이유0 이하 ConfirmRetreat는 기존 관문에 남긴다.

| 이유 | 원문 동작 | 보존해야 하는 부분 효과 |
| --- | --- | --- |
| 2 | ScriptBreak1 → `004b3540`의 next3만 예약 | 공통 abort·결과·source·세 +84·저장 삭제를 실행하지 않는다 |
| 3·9 | ScriptBreak1 → `004b5880`의 선행 삭제 → source0 → `004b6350` | 선행 삭제 실패 시 source/abort/State는 이전 값이다. 정상 복귀 뒤 공통 부모가 다시 삭제한다 |
| 나머지 signed 양수 | ScriptBreak1 → `004b6350` | 비멀티 미션/Editor는 abort1·결과0·next3·세 +84를 지운 뒤 삭제한다 |
| known && battle && !missionPost, 이유2·3 제외 | 위 쓰기보다 먼저 원격 페널티/재접속 처리 | 후순위 MP 구현 범위다. 현재 필수 장치 누락을 명시 거부하며 빈 성공으로 바꾸지 않는다 |

원격 분기는 `004cbab0(0)` → 조건부 `004180a0(005c8618)`와 `004b4ac0/004b4b40` → `004bfb50`이다. 이 부분과 공통 abort의 비미션 Battle→State6, State6의 결과 안내/일반 Fort 복귀는 **8번 MP 이월**로 분류한다. 신규 원격/State6 검사는 하지 않았다. 일반 비멀티 종료는 이벤트 안에서 World를 반납하지 않고 기존 Tick의 다음3/cleanup에 맡긴다. 949에서 교정한 사용자 `!state.dat` 경로도 그대로 전달한다.

## 독립 PE 관측과 C++ 대조

제품 추출 전에 `decomp_battle_leave_reason.py`로 보호 PE의 실제 부모와 `004b3540/004b5880/004b6350`을 실행해 최초22행을 고정했다. 미션/Editor 각각 이유1·2·3·4·9·INT32_MAX, 이유3/9의 path1/delete1/path2/delete2 중단, 이유1의 path1/delete1 중단이다. 실제 명령620개를 실행했고 독립 재관측 차이0이다. 원격 함수를 허용 주소로 등록하지 않았으며 두 모드 모두 missionPost1이다. 단독 미션의 기존 로컬 로그인/송수신도 이 관측에서는 실행하지 않았다.

파일 경로 `004b5470`와 CRT remove `004e58e4`는 명시 외부 장치다. 실제 파일을 열거나 삭제하지 않는다. 원문 직접 쓰기8DWORD와 스택 이외의 쓰기를 거부한다. 다섯144BYTE 전체와 두 삭제의 각 장치 진입 순간 ScriptBreak/source/abort/결과/next/세 +84를 기록한다. source0을 삭제 앞으로 옮기거나 두 번째 삭제를 생략한 별도 기대 모델을 사용하지 않는다. 최초 fixture는 배타 생성하고 verify는 원문/도구/fixture SHA와 전체22행을 비교한다.

새 C++ 검사2개는 그22행 전체/부분 효과와 양수 계약·선택된 삭제 장치 누락을 대조한다. 같은 실제 `RawBattleLeave`를 제품과 공유한다. 기존 RetreatConfirmation2/NativeMainMenu11/StatePollAction4/MissionLifecycle2까지 **고유21검사, 실패0**이다. 기존 Native82행과 미션수명831행은 C++ 재생이며 새 원문 관측으로 합산하지 않는다. 원격 장치를 호출하는 새 검사는 없다.

최초 빌드는 정상 종료했지만 새 검사의 BYTE 채움에서 C4244 경고가 발생했다. `uint8_t`를 명시한 뒤 최종 제품/집중 대상 Release 빌드가 경고·오류0으로 끝났다. 원시는 `extracted/cpp-resume-955/build.log`와 `build-final.log`, 필터별 `*-final.log`, `native-verify.log`에 보존했다. 게임/GUI·전체 CTest/34미션/ZIP은 반복하지 않았다.

## CD10.72 정적 비교

CD의 같은 입력 부모는 `0041ae80`, LeaveBattle 명령 전역은 `00512b90`이다. signed 이유0 이하의 안내와 양수 이유2/3/9 분기는 같은 구조지만 주소/장치가 다르다.

| 항목 | PATCH10.78 | CD10.72 |
| --- | --- | --- |
| ScriptBreak / source | 00542544 / 0055a420 | 00518948 / 0052e2e0 |
| known / battle / missionPost / Editor / metamap | 00595344 / 00594fbc / 00594fa4 / 005c85a4 / 00594fc8 | 0050f828 / 00540a20 / 0052e170 / 00518904 / 00540a24 |
| 다음3 / 다음6 | 004b3540 / 004b3550 | 0042fd70 / 0042fd80 |
| 공통 abort / 선행 삭제 | 004b6350 / 004b5880 | 0042fd90 / 00432a00 |
| abortPending / outcome | 005c85e0 / 005c85ac | 00518958 / 00518910 |
| next / current+84 / immediate+84 / next+84 | 005c8878 / 005c886c / 005c874c / 005c88fc | 00564c90 / 00564da4 / 00564fec / 00564d14 |
| 경로 / CRT 삭제 | 004b5470 / 004e58e4 | 00432700 / 004f48c0 |

CD 원격 타이머는 `004011b0` 반환 현재 시각과 `00564f20`을 비교하고 `004318e0/00431850`을 선택한다. PATCH의 timer 객체 호출을 주소만 바꿔 재사용하지 않는다. 자동 대응표에서 PATCH004b6350은 대응 없음으로 나오지만 CD0042fd90 본문이 같은 공통 구조를 갖는 것을 직접 확인했다.

현재 CD 호스트는 여전히 기존 MainMenu/음악 직접 복귀 어댑터다. 이번에는 차후10.72 복원을 위한 차이 문서화만 했으며, 10.78 공용 함수를 CD 전체 Native 구현이나 CD 실행 검증으로 세지 않는다. 주 복원 대상은10.78이다.

## 현행 기준과 다음

제품955/표시11.01, EXE SHA-256 `19fd9ba91650eb976a4d43a350b6a07d798ee806dbc2b9126e36ef5d60d912be`, 11,281,408바이트다. 기존 보호2788개/fixture968개 불변, 새 fixture1개 뒤2789개/969개다. 다음 보호 기준은 `extracted/cpp-resume-955/protected-final.json`이다. [완료 JSON](../../cpppj/battle-leave-reason-20261106.json)과 [Native JSON](../../cpppj/battle-leave-reason-native-20261106.json)을 따른다.

실제 포획/희생·특정 적 공격·승리·공중3종 전용·MP/실제 통신/물리 입력/PCM/CD 삽입 검사는 하지 않았다. 다음956은 같은3절3번의 Native3 다른 선택·State1/단독State7·cleanup Tick 나머지 판정이다. LeaveBattle의 비멀티 양수 이유 대조와 CD/State6 분류는 이번 범위에서 완료지만 셋째 체크 항목 전체는 아직 완료가 아니다. 그 뒤4→7→8 순서를 유지한다.
