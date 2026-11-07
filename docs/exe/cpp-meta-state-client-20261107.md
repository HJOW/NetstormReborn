# 1055 — 실제 State19와 같은 Client의 Fort/Core/채널

1054(`8efc76eb / 20261107-71 CP`)의 State19 부품을 실제 Uber/상태 분배기와 Client 장치에 접합했다. World 정리 뒤 같은 Fort/Core/CS/뉴스/Chat을 사용하며 임시 World를 생성하지 않는다. **즉시 다음21/25/34 부모는 아직 미연결이므로 현재 메타 진입은 그 invalidState 경계에서 중단한다.** 실제 메타 진입/표시나 LAN 플레이 완료가 아니다.

## State19 실제 연결

기존 Session의 PollLocalConnectionState는8/9/10/13/14만 소비한다. next19는 Uber PollEditorState가 같은 장기 분배기를 선택해 처리한다. RawLocalMissionState의 큰 부모 선택에19를 등록하고 실제 NativeMetaState로 연결했다. cleanup에서 이전 World/Session을 반납해도 분배기/EditorRuntime과 빌린 Client 전역은 살아 있다.19를 Fort 설치 완료로 처리하거나 없는 World에 CompleteFortInstall을 호출하지 않는다.

1054의 실제 순서대로 fortPal→cleanup0→별도CD→메타/권한→renderer→CS→즉시 다음 선택→Core1/1→현재 Fort 파일→현재 Core 최소병합0→HUD4→known0→주변음/뉴스/채널/입력 장치를 연결했다. 파일은 기존 사용자 홈/자산 이관 정책의 실제 현재 Fort만 준비하고 같은 TemplateStorage.LoadFile reader/manager/제목/권한/진단을 사용한다. 손상 파일 제거 장치는 검증된 사용자 작업 namespace만 사용한다. 예외나 후속 상태 미연결에서 앞선 원문 쓰기를 rollback하지 않는다. 공통144BYTE 확정은 기존 분배기에 남는다.

이 연결로 실제 State19 몸체가 선택되지만 **21/25/34의 endpoint/로컬 서버 부모와 메타 표시가 없으므로 진입은 아직 완주하지 않는다.** 새 성공 플래그/로컬 서버 응답이나 별도 완료 상태를 만들지 않았다. 공식 서버 도메인/폐기 클라이언트 검증·7f/80은 계속 비활성화한다.

## 같은 CoreData 읽기와 깊이/표본

기존 Session 내부004beca0 읽기를 `FortCoreDataLoad` 공통 함수로 옮겼다. missionPost/editor 자연 반환→중첩 깊이 증가→같은 현재 Template/CoreData 선택→owner 진단/active1→현재 Core10BYTE→선택 최소 병합(랭크/승리/패배)→실제 Chat 랭크→남은 seed.low16/세32비트 무결성 블록 순서다. 같은 MemStream 물리와 cursor/end를 사용하며 현재 Core를 원문 위치에서 다시 조회한다. BYTE/WORD/DWORD 읽기 폭과 WORD/DWORD의 cursor 저장 뒤 논리 end 진단, seed 상위WORD 보존, 실제 Core+4=1 뒤 진단 표본 쓰기를 유지한다. Session은 기존 진단 중단 계약을 쓰고 메타는 원문 진단 로그 정상 복귀를 제공한다. 물리 범위 밖 접근은 명시 거부한다.

005c89b8은 이제 Client의 FortSerializationGlobals가 한 DWORD로 소유한다. 같은 TemplateStorage/형제 Storage와 실제 Session의 FortObjectLoadState가 참조로 빌린다. 독립 대조 입력만 자기 fallback 깊이를 쓴다. 객체 상태는 주소 참조를 복사/이동하지 않는다. 기존 Session World의 깊이 소비자 갱신을 보존하고 World 없는 메타는 같은 실제 깊이만 증가/반납한다. 단순 getter/Session 생성에서 외부 깊이를 초기화하지 않는다.

004bea70은 불일치 때 현재00540cb8을005318ec에 쓴다. 보호 PE의00540cb8 초기값은 **6**이다. 이전 Session 내부 sample0 대신 Client의 PatchSample6을 추가하고 실제 사제 Next(9) writer/추적·SP·로그인 소비자와 Core 읽기가 같은 주소를 사용하도록 연결했다. 독립 PriestRegenPatchState 입력의 sample 필드는 fallback으로 유지한다. 실제 Client 표본을 Session 생성/반납에서0으로 바꾸지 않는다. 이 단위는 다른 모든 추적 전역의 수명까지 완료했다고 주장하지 않는다.

## BATTLE 접두와 채널8

0042f3d0/0042f330을 같은 현재 Chat/CS/Player/RawChatChannels에 연결했다. 현재 Chat이 있을 때만 mode0은 BATTLE 접두 제거1, 비0은 known/참가자+64의 bit4 조건에 따른 추가0을 실행한다. 번역 뒤 현재 Chat을 다시 조회하고, 접두 처리 여부와 무관하게 채널8을 먼저 지운다. 현재 known0이면 CS를 읽지 않고 반환한다. known 비0/현재 참가자+a4 전투번호 비0이면 당시 전투+38의8DWORD 목록 포인터를 고정해 순회하고, 각 비영 참가자는 당시 현재 CS에서 subscriber+4/이름+26을 다시 읽어 실제 채널8에 추가한다. 가짜 명단/Player 레코드 사본을 만들지 않는다.19의 known0 경로는 접두 제거/채널 정리만 선택한다.

## 기존 검사 파일의 컴파일 계약

새 loadingDepth를 추가한 뒤 기존 TemplateSerializationGlobalsTests.cpp42행의 host 전체4BYTE static_assert가 더 이상 성립하지 않는 것을 정적 점검에서 발견했다. 앞선 SubscriberGlobals 제보와 같은 종류의 C2607이 생기지 않도록 version/중첩깊이 각각의 DWORD 폭을 단언하도록 교정했다. 기존 실제 번역단위를 별도 OBJECT 대상으로 Release 컴파일해 종료0·경고/오류0을 확인했다. 실행 파일/새 검사 사례/새fixture는 만들지 않았고 검사는 실행하지 않았다. 다른 테스트 파일/전체 솔루션 통과를 의미하지 않는다.

## CD 차이와 검증

10.72 Core 읽기00429200은 인자 없는 함수이며10.78 최소 병합 인자와 세 무결성 블록이 없다. CD 깊이는005178d4/seed는005178d0이다. 채널 대응은004b2cd0/004b2c40이며 참가자128h 구조를 사용한다. 현재 PATCH CS1cch/5350h와 섞지 않으며 전체 CD 복원은 후순위다.

NetstormCpp Release **종료0·경고/오류0**. 실행파일 12,264,448바이트/SHA-256 `5b3945d7ea2b1089bf3cb3425c91fa71a8101e3cd340f386ccf977d17b1b35b5`. 신규 MP/단위/게임/통신 검사0·새fixture0·보호 2800파일 불변이다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이며 이번에는 같은 산출물/보호PE 정적 대조만 했다. 전체 솔루션/플레이 회귀/배포 ZIP 결과로 확대하지 않는다. 다음은 즉시21/25/34의 실제 로컬 부모와State30/메타 표시·나머지LAN·Outpost/checking5→한국어 후속→시나리오 UI다.
