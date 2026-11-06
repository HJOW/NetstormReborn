# Process 부착의 진단 복귀와 현재 공통 후단 — 단위959

2026-11-06, HJOW-Athlon. AGENTS와 공통·DN 인계를 다시 읽고 LEFT_JOBS 3절4번의 Process/Factory 공유 수명을 이어갔다. 마지막 전체 정밀 디컴파일은 같은 HJOW-Athlon/2026-11-03이며 정밀 산출물과 원본은 변경하지 않았다. 제품은959/11.01이다.

## 원문과 구현

ProcessForm의 파생 부착 부모는 PATCH `004b1440`, CD `004af380`이다. 기존 SquidProcessHost::AttachForm은 부모 변경/debug 비트 누락을 즉시 예외로 바꿨다. 직렬화 크기0/0x800 이상도 정상 복귀 가능한 원문 진단 대신 예외로 바꿨으며, 선택 전 writeData/transmit을 함께 요구하고 서버의 로컬 flags에서는 프로세스 조회 자체를 생략했다.

새 AttachProcessForm은 첫 WORD16 비교로 부모 변경 여부를 저장하고, debug bit0x80이 없으면 원문 진단을 호출한 뒤 계속한다. 진단 뒤 현재 서버 값을 읽는다. 비서버이면 PID·writer·송신 장치를 요구하지 않고 원래 flags로 공통 Form 부착을 호출한다. 실제 Host는 같은 World 대여의 assertion을 **진단 선택 위치에서만** 조회하며 독립 Host는 제공한 state assertion을 사용한다. 현재 공통 공장도 접두 뒤 선택 위치에서 조회한다.

서버 분기는 원문 코드에 따라 먼저 현재 PID의 프로세스를 조회하고, flags&0x50이0일 때만 writer를 선택한다. 최초 변경 여부를 writer에 넘긴다. 크기0 진단 뒤에는 송신 없이 진행하며, 양수는 상한 진단 정상 복귀 뒤 송신하고, 음수는 송신하지 않는다. 선택된 서버 분기는 flags|0x40으로 공통 부착을 호출한다. writer와 transmit은 각 선택 위치의 현재 callable만 복사해 호출한다. NULL PID도 writer 선택 전까지는 dereference하지 않는다. 이 **직렬화/송신 분기는 정적 원문 반영과 컴파일만 확인했으며 신규 단위/패킷/MP 검사는 실행하지 않았다**. 상위 Attach/RefreshForm의 기존 선행 전송 검사까지 제거했다고 주장하지 않는다.

두 판본의 debug 진단은 PATCH0xa77/CD0xa2e, 크기0은0xa7f/0xa36, 상한은0xa81/0xa38이며 파일 표기 `\Ns\O\Squid.cpp`/`\ns\o\Squid.cpp`를 구별한다. PATCH는 낮은 BYTE의 signed 판정, CD는 bit0x80 검사지만 결과는 같고 전체 flags DWORD를 그대로 전달한다. PATCH 크기0의 공통 부착 뒤 즉시 반환과 CD의 공통 끝 분기는 최종 호출 순서가 같으므로 공통 후단으로 표현했다. CD의 별도 프로세스 조회 helper도 실제 Host의 같은 PID 조회로 연결한다.

## 독립 원문과 실제 Host/World 검사

제품 구현 전에 양판본 서버0 입력48행을 고정했다. 저장 부모0/7 × flags6종(0/80/100/180/ffffff7f/ffffff80) × 진단 정상 복귀/진단 중 WORD16 변경2종이다. 전체 진단 문자열/파일/행 → 공통 자식의 부모·flags·현재 WORD 인자와 슬롯50/36바이트를 관측했다. 공통 부착 자식은 명시 경계이며 실제 링크/Pop 몸체의 Native 관측이 아니다. 이 자식의 정상 복귀를 포함한 부모 ret8/스택을 확인했고 최초 fixture 재관측 차이0이다. 원문 게임·OS·직렬화·패킷·통신은 실행하지 않았다.

수정 전 실제 비서버 Host 검사는 양판본에서4단언이 실패했다. RefreshForm의 Unpop이 free 부모에서 반환한 뒤 파생 부착에서 부모 변경 진단을 실행해야 하지만 기존 예외가 먼저 중단했다. 수정 후 같은 입력은 진단 장치의 체인 수리를 거쳐 실제 공통 부착/Pop으로 이어지고 parent/head/void·contained/PID/Kernel 수가 맞는다. 추가한 실제 RawSurfaceWorld 검사도 현재 대여의 Client 진단 → 실제 공장 Unpop/공통 Attach/Pop → 깊이0 복귀를 확인한다. 포획/희생·적 공격·승리 경로는 없다.

| 최종 집중 검사 | 개수 | 결과 |
| --- | ---: | --- |
| ProcessAttachCurrent_ | 새3 | 통과, Native48행/실제 독립 양판본/실제 World |
| ProcessCreationCurrent_ | 기존4 | 통과, 실제 현재 번호/공장/연속 대여 |
| process_default_handler_repeats_with_payload_period | 기존1 | 통과 |
| process_dependent_chain_order_and_unlink | 기존1 | 통과 |
| ProcessHostStorage_CurrentEffectMustReturnBeforeBindingRetirement | 기존1 | 통과 |
| ProcessHostStorage_FallbackFinalShutdownKeepsHostAndPoolAlive | 기존1 | 통과 |

고유11검사=새3/기존8, 실패0이다. 기존8개는 최종 제품 라이브러리로 통과했고, 그 후 바뀐 것은 새 World 검사의 입력 설정 한 줄뿐이다. 최초 World 검사는 생성 전의 server0을 World 초기화가1로 되돌려 RefreshForm의 전송 앞단에서1실패했다. World 생성 뒤 서버0을 명시하고 실제 Pool 값도 확인한 뒤 새3개를 다시 실행해 통과했다. 첫 실패 로그는 덮어쓰지 않았다. 최초/후보/최종 빌드는 모두 종료0·경고/오류0이다. 파일 편집 스크립트의 CRLF 경계 단언 실패는 파일 쓰기 전에 중단됐고 이후 대상 함수만 바이트 보존 방식으로 교체했다.

## 잔여와 다음 순서

이번에는 파생 +74 접두와 실제 공통 후단 연결을 복원했다. 상위 `0041bd20`/CD`0048f590`과 SquidProcessHost::Attach/Reattach/RefreshForm에는 타입·지정 SID·genus/체인 선행 거부, 생성 실패의 Kernel/rotation 복구, CD dead/free 거부 등이 남는다. 실제 힙/SEH/할당 실패·외부 heap 반환과 전체 Factory를 완료로 세지 않는다. 다음960도 같은3절4번의 이 부모/실패 수명부터 진행한다. checking8=부모3/부분2/MP3, 완료부모39/부분4는 불변이다. 상위 수신/직렬화/MP 기능은 사용자 지시대로 추가 검사 없이 원문 근거로 작성한다.

새 게임/GUI·실제 포획/희생/특정 적 공격/승리·신규 MP/패킷/통신·공중3종 전용·물리 입력/PCM/CD 삽입·전체 CTest/34미션/ZIP은 실행하지 않았다. 기존 일반 로컬 프로세스 검사의 서버1/flags50은 직렬화/송신을 선택하지 않는다. 이를 신규 MP 검증으로 세지 않는다.

EXE SHA-256은 `e522cda5066d7efd08f895289ddce4ffddc15b89024de1978b31c3a5916909b9`, 11,298,816바이트다. 기존 보호2790개/fixture970개는 불변이며 새 fixture1개를 더한 다음 기준은2791개/971개다. 원시와 다음 보호 기준은 `extracted/cpp-resume-959/`이며 [완료 JSON](../../cpppj/process-attach-current-20261106.json), [Native 보고](../../cpppj/process-attach-current-native-20261106.json)를 따른다. 전체 목표는 계속 진행 중이다.
