# 실제 연결 등록·재배치와 Client 장기 연결표 — 단위458

2026-10-26, HJOW-Athlon에서 원본10.78을 기준으로 작업했다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 `extracted/refined`를 변경하지 않았다. 목표 전체의 완료 판정은 아니다.

## 원본 증거

제품 변경 전에 실제 PE의 `004e2870`(양수 등록), `004df4f0`(pending 제거), `004e27b0`(pending 재배치)을 Unicorn에서 실행했다. 288관측 중60개는 선택한 하위 효과의 실패 위치에서 중단했다. `004df470`의 buffered 설정과 `004e0620`의 진단만 관측 장치로 치환했다. 원본 게임 실행 파일을 프로세스로 실행하지 않았다.

등록은 현재 양수 슬롯 진단 → 슬롯 쓰기 → 같은 연결의 sender 쓰기다. 재배치는 현재 서버/대상 진단 → pending 조회 → 양수 슬롯 쓰기 → sender 쓰기 → **현재 양수 슬롯**에 buffered 통지 → 음수 출처 진단 → pending0 → 현재 count 감소 순서를 유지한다. callback이 슬롯·count를 바꾸거나 중단하는 경우도 바이트 기대값에 포함했다. 별도 pending 제거는 개수를 재계산하지 않고 DWORD 감기는 감소를 수행한다.

- [Native 보고서](../../cpppj/connection-registration-native-20261026.json)
- [동결 관측](../../cpppj/tests/fixtures/connection-registration-x86.tsv)
- [관측 도구](../../tools/decomp_connection_registration.py)

## 실제 제품 연결

`RawConnectionRegistry`가 Client 장기22개 연결 슬롯·별도22개 송신버퍼 슬롯·32개 pending 슬롯·count를 소유한다. 동기화는 같은 `ClientLocalStateGlobals`의 현재 `ClockSynchronization.connection`을 직접 빌리고 그 owner 수명을 유지한다. 숫자 토큰은 실제 연결 주소와 close/free callback을 가리키며 호스트 포인터를32비트로 절삭하지 않는다.

일반 `LocalMissionTransport`와 `EditorLocalTransport`가 같은 Registry에 실제 노드를 등록한다. 첫 빈 pending1..31 선택, 음수 sender, owner 재배치, 현재 분배 중 지연 종료와 실제 반납이 같은 노드/슬롯을 사용한다. Native 정리 부모가 실행되는 동안에는 하위 free가 부모의 후속 슬롯/count 쓰기를 앞질러 실행하지 않는다. 일반 패킷 분배의 반납은 현재 별칭을 해제하고 pending count를 중복 감소시키지 않는다. close/free 실패 때는 도달한 플래그·동기화·슬롯의 선행 효과가 남는다.

Editor가 일반 전송의 `DispatchFrame`만 빌릴 때 그 전송의 사유 양끝으로 외부 노드를 오인하지 않도록 기존 장치에 분배한다. 실제 Campaign 서비스가 Client의 같은 Registry를 제공하며, 기존 Editor 설치가 다른 Registry를 빌렸다면 GameplayUse를 얻기 전에 거부한다.

호스트 소유자 최종 소멸의 `Forget`은 사라지는 자기 노드의 dangling 별칭을 반납하는 기존 RAII 경계다. 이것을 Native State1 전체 정리가 실행됐다는 증거로 취급하지 않는다.

## 검증과 실패 기록

최종 소스·빌드·집중·GUI의 SHA와 실행 횟수는 [실행 보고서](../../cpppj/connection-registration-runtime-20261026.json)에 기록했다. 신규8개는 Native288 전체 바이트/순서, buffered 실패, free 실패, Editor의 실제 공유 표, 음수-2 pending의 분배 중 종료, 일반 미션의 같은 양끝과 owner 정리, 동기화 owner 수명을 검증한다. 기존 검사 본문과652자료(그중 TSV649개)는 보존했다. 전체 CTest는 이번 단위에서 반복하지 않았다. 마지막 전체 실행은456의3,114개 통과다.

첫 빌드의 namespace 닫기 누락과 둘째 빌드의 const 서비스 대입 오류를 교정했다. 첫 집중의 두 실패는 외부 Editor 노드를 내부 LocalMissionTransport 소유자로 오인한 경계였으며, 위 실제 장치 분배로 교정했다. 기대 fixture나 기존 숫자 단언을 바꾸지 않았다. 초기 집중1·2 결과의 잘못 복사된 `units:[456]`만458로 정정하고 원본 JSON과 해시를 별도 보존했다. 원시 실행 로그는 변경하지 않았다.

복사본 자료의 실제 메인 메뉴 → Campaign → Early Missions → Bridge the Gap → MORE/OK에서 Mission4·정지0과 양수 연결2/pending0/count0을 확인했다. 다만 ESC 막대가 열리지 않은 검사에서는 Game 행을 찾을 수 없어 메뉴 복귀·재진입 계획을 실행 전에 중단했다. 이 실패를 연결표 진입 검사 성공과 구분한다. ESC 문자를 가상 키로 바꾸면 된다는 추정은 틀렸으며 임시 검사 키 추가도 최종 제품에서 제거했다. 이전 실행 결과와 준비 실패 기록은 그대로 보존한다. 실제 GUI의 메뉴 관문 확인은 후속이다.

## 남은 경계

- 일반 로컬 전송은 기존 `Open`에서 서버 accepted 노드를 미리 만드는 시점을 유지한다. 원문 OS listener/accept 전체와 TCP/SPX, 상대 소멸 부모의 완전 복원은 별도다.
- 장기22칸 송신버퍼는 연결 객체 안의 byte vector와 다른 저장소다. 실제 writer/반납 장치가 아직 미연결이므로 vector를 그 슬롯의 대체로 등록하지 않는다.
- Native State1의 전체34Effect/cleanup·prefix, Runtime/Player 반납 coordinator와 실제 Edit 예약 소비를 이어 연결해야 한다.
- 전체 미션 자연 완주, 픽셀·음향·입력 전체 동일성, Edit 설치 완료는 검증하지 않았다.

원시 자료는 `extracted/cpp-connection-registration-458-*`에 있다. 원본/CD/AGENTS 등 보호1,821파일과 기존 Native 자료를 변경하지 않는다.
