# 메모리 실패의 실제 new_handler와 종료 부모 — 단위979

2026-11-06 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이다. 우선순위4의 막연한 외부 heap/실제 할당 실패 항목에서 실제 활성10.78 시작 caller를 찾아 유한한 범위로 연결한다. 원본·정밀 자료는 읽기만 했으며 실제 메모리 고갈·오류창·멀티플레이 검사는 실행하지 않았다. 별도 일반GUI 진입/창 종료는 아래 실제 결과에만 귀속한다.

## 실제 등록과 원문 순서

PATCH 시작00438dc0은00438de9에서 CRT new-handler setter004e6b3c에 LAB00435f50을 전달한다. setter는 현재005e4b38에 같은 함수 주소를 설치한다. 원본 operator new004e4391은 nh_malloc004e4316에 flag1을 전달하며 실패하면004e6b60이 현재 handler를 호출한다. 일반 CRT malloc0043fa80의 선택 flag005e4cb0과 구별한다. 이번 연결이 모든 malloc 실패를 포괄한다는 뜻이 아니다.

LAB00435f50은 canonical 함수목록에 별도 함수로 등록되지 않아 정밀 C에는 본문이 없었다. 보호 PE의00435f50..00435f8b를 읽어 다음 순서를 확인했다.

1. MessageBoxA(NULL, `Out of memory!`, `Unrecoverable Error!`, 0x10).
2. 종료 정리 부모004b3490.
3. 현재0054dbcc가 비NULL이면0046d140으로 해당 이벤트 SetEvent.
4. 00435060의 return1 빈 몸체.
5. 004aad70의 음악 정지.
6. _exit004e3fc0(0).

004b3490은 현재 초기화 완료0054dbb8이 비영이고 fort00594fb8/battle00594fbc/metamap00594fc8 중 하나가 비영이며 종료 guard005c890c가0일 때만 guard1→004b2df0(0)→빈004437f0을 실행한다. guard는 전체 정리보다 먼저 기록하며 정리 실패 시 원상 복구하지 않는다. 기존 UberGump::CleanupNativeStateWorld가004b2df0 전체 부모를 이미 연결하므로 이번에 정리 알고리즘을 다시 만들지 않는다.

현재0054dbcc는 음악 이벤트가 아니라 선택적 구 HTTP 서버의 종료 이벤트다. 시작은 httpRootPath/httpPort/httpMimeFilePath를 읽고0046d720의 CreateEvent/CreateThread로 이를 만든다. 현재 제품은 해당 HTTP 서버를 생성하지 않으므로 실제 연결의 현재값은NULL이다. 공유본문의 비NULL 분기는 명시효과 검사로 대조하되 서버 생성을 추가하지 않는다. 마지막004aad70은 기존 SoundMusic::Stop의 준비/채널 관문을 사용한다. 전체 장치·작업 스레드 Shutdown으로 바꾸지 않는다.

CD10.72는 시작00485b20→setter004f34d0(LAB00485ab0), 메시지 후42fc50/현재52e99c/4b92d0/빈45de80/439910/4f3150 순서다. 두 문자열은 같은 내용이다. 이번 실제 제품 연결은10.78이며 CD 전체 복원은 별도 후순위다.

## 현재 Client와 호스트 경계

공유본문 RawAllocationFailure는 고정 함수 포인터와 void* context만 보관하여 handler 진입 때 std::function/문자열 포장을 새로 할당하지 않는다. 같은 현재 초기화/모드/guard 주소를 빌리며 메시지 복귀 뒤 정리 관문, 정리 복귀 뒤 현재 이벤트를 읽는다. 빈 원문 함수에 추가 효과를 만들지 않는다.

실제 handler는 Client::Run의 첫 위치에서만 scoped 등록하고 정상·예외 복귀에서 이전 프로세스 전역 handler를 복원한다. Run 전에 실행되는 Client 생성자·CLI의 초기 할당까지 이 등록이 포괄하지는 않는다. 원문00439919의 초기화 완료 쓰기는 같은 최초 카메라10칸 복사보다 앞에 연결한다. 실제 WM_DESTROY도 같은 현재 종료 정리 부모를 사용한다. C++ 최종 소멸에서는 메뉴·모듈을 반납하기 전에 초기화 값을 해제하여 부분 반납 뒤 호스트가 합성한 WM_DESTROY가 이미 반납한 메뉴를 다시 정리하지 않게 한다.

치명적 실패 handler는 재귀 new_handler를 해제한 뒤 원문 부모를 실행한다. 정리 중 추가 할당 실패가 나면 같은 오류창/정리를 재귀 실행하지 않고 종료0으로 끝낸다. 이는 현대 C++ 수명 경계이며 원문32비트 스택·SEH·임의 주소/미작성 heap 바이트 자체의 동등성을 주장하지 않는다. 실제 종료는 std::_Exit(0)으로 C++ 소멸/새 설정 저장을 추가하지 않는다.

## 집중 검사와 남은 구분

새 AllocationFailure_ 검사5개는 고정 문구/아이콘/종료0, 메시지가 바꾼 현재 모드, 초기화3×편집/전투모드4×guard3의36조합, 정리의 현재 이벤트 변경/NULL, guard 선행·재진입과 정리 예외의 부분 효과를 확인한다. 메타맵 양성 조합은 신규 멀티플레이 검사 제외 지시에 따라 실행하지 않는다. ScopedNewHandler의 실제 표준 등록값을 읽어 정상·중첩·예외 복원도 확인하며 실제 부족한 할당을 만들지 않는다. 실제 Client 연결의 OS 경고창·강제 종료·메모리 고갈·HTTP 이벤트는 실행하지 않는다.

PATCH 현재 Create/Take/Retype의 공통 부모와 자산 bare/내장 Form/Teleport 생성 분배는 기존899·905·975 및888까지의 대조 범위를 따른다. 원문 외부 heap 주소를 임의 SID로 바꾸는 동작, NULL/범위 밖 포인터 쓰기, 전체SEH/실제 OS 고갈, CD 전체를 계속적인 비멀티 구현 잔여로 중복 계수하지 않는다. 새 활성 caller의 구체적 미연결이 확인되면 해당 함수를 별도 항목으로 올린다. 단위978의 legacy 외부 Kernel 수명과 이번 실제 handler를 전체 외관·모든 시나리오 자연 완주로 확대하지 않는다.

원시 정적 PE 근거는 `extracted/cpp-resume-979/allocation-failure-static.json`, 기존 파일 반영안은 같은 경로의 `pending/existing-sources.patch`다.

## 최종 검증과 실제 일반 종료

Release 제품/집중 빌드 첫 명령 종료0·경고/오류0이다. 고유20검사(새5/기존15)가 첫 실행부터 실패0이며, 새5개는비멀티36관문조합/현재이벤트/guard부분효과/실제표준new_handler등록복원을대조한다. 실제메모리고갈이나오류창/치명적종료를일으킨검증이아니다.

현재979제품을면제호스트HJOW-Athlon/192.168.0.94의격리실행자료/사용자프로필에서실제MCP로실행했다. 첫메인메뉴→tutorial1진입1회/안내닫기→100프레임대기의게임시각10.446→12.127→활성미션에서WM_CLOSE→종료0이다. 앱입력5/관측7·메인/미션800×600 PNG2장을확인했고설치자료/보호2800개/fixture980개는불변이다. 첫!color.dat부재는기존캐시miss경로이며사용자Cache에생성됐다. 포획/희생/특정적공격/승리·MP·공중3종·물리입력은재현하지않았다.

제품979/11.01은11,393,536바이트·SHA256 `148a9950b58cae2a903b7ecf3b0cdcb701b0c8e1ebce34168775d170bd71e5c1`이다. [단위보고서](../../cpppj/allocation-failure-20261106.json)와[양판본LAB정적명령](../../cpppj/allocation-failure-static-20261106.json)을따른다. 다음980의4번마감/7번분류후8번을계속한다.
