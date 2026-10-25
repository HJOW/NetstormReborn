# Editor의 로그인 없는 로컬 전송

2026-10-25 단위427의 원문 대조와 구현이다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 원본 게임/새 디컴파일/GUI는 실행하지 않았다. 아래 순서는 현재 보호 PE와 정밀 C의 정적 대조 근거이며 새 Native 실행 관측을 만들었다는 뜻은 아니다.

## State1의 실제 전송 경로

State1의 `004e22c0`는 `zlocal,*:port,s` 리스너와 등록9의 `zlocal,*:port,c`를 만든다. Editor에서는 캠페인 로그인으로 Player 연결을 확정하거나 buffered 전송을 켜지 않는다. 리스너는 실제 접속을 `004e02d0`에서 음수 pending 슬롯에 수락한다. 이 단일 로컬 접속은 첫 빈 슬롯 -1을 사용하며, 원문의 type1 처리기는 음수 sender에서 자연 반환한다.

명령은 `004e2dc0 → 가상 송신+4 → 004dff00`으로 반대쪽 바이트 큐에 기록되고, 후속 `004e28b0` 폴링에서 처리된다. 건설/이동 처리기를 송신 함수 안에서 직접 호출하면 상태 확정·기록·observer·수신 시점이 달라진다. 기존 `RawConstructionRequest`는 요새 모드에서 음수 sender를 현재 로컬 Player로 바꾸는 별도 원문 분기를 갖는다.

## 같은 분배기와 별도 Editor 수명

`EditorLocalTransport`는 기존 `LocalMissionTransport::DispatchFrame/Dispatch`의 실제 패킷 등록표·길이/버전 정책·기록·처리기/종료 순서를 재사용한다. 그 클래스의 캠페인 `Open/RegisterLogin/AssignConnection` 및 buffered 큐는 사용하지 않는다. 별도의 Editor listener/client9/accepted(-1)와 unbuffered 큐를 소유한다. Player·State·게임 난수나 로그인 성공 비트를 작성하지 않는다.

폴링은 리스너 수락, pending 한 번, 등록9 반복 순서다. `004dfc00`의 물리 프레임 소비와 길이0 sentinel에 따른 반복 중단을 구별한다. 무효 패킷이나 분배 실패 뒤의 등록9 다음 프레임은 같은 Poll에서 앞당겨 처리하지 않는다. 최초 연결의 type1, 송신 observer1과 단일 수신의 선행 observer0, 집합 내부 observer0는 각각 원문의 호출 위치를 유지한다.

수신 중 종료는 현재 깊이가0이 될 때 실제 close/release를 호출한다. 반대쪽 Zocket 소멸은 별도의 peer 손실 상태와 type2 분배로 전달하며 상대 Player/연결을 임의로 삭제하지 않는다. 연결별 실제 수신버퍼를 보유하므로 `004df9f0`의 type2 생성이 첫 BYTE를 보존하고 종류/길이만 바꾸는 효과도 남는다. 이전 정상 패킷의 protocol BYTE는 이 type2의 버전 정책에도 영향을 준다.

## 지원 경계와 다음 연결

이번 클래스는 State1이 만드는 한 로컬 접속을 위한 조립이다. TCP/SPX/전체 Ztrans, 여러 원격 접속, 부분 프레임과 지연 패킷의 복원 범위로 확대하지 않는다. 실제 Client State1 prefix에 이 수명을 생성하고 Session의 기존 건설/이동/삭제 처리기를 같은 등록표에 연결한 뒤, 원문 메인 루프의 전송 폴링/송신 위치에서 사용해야 한다. 현재 제품의 Edit 진입이나 사용자 편집 완료를 증명하는 호출자는 아직 없다.

새 집중 검사는 양끝 생성/시간·음수 sender·요청/응답 큐·중첩 Poll 거부·부분 예외·버전 종료·등록9 sentinel·상대 종료·현재 수신 BYTE 보존을 창 없이 확인한다. 빌드/집중 실행 결과와 보호 파일 감사는 단위 종료 시 아래에 기록한다. GUI와 전체 사용자 편집 검증은 다음 제품 연결 단계다.

## Session 접합 시 그대로 재사용할 요청 처리기

현재 `LoadLocalConnection()`의 설치 callback은 `RegisterConstructionCommands`(3e/54), `RegisterCarrierCommands`(40/4f/50/60/61/5b/5c/5f/7d/6d), `RegisterFactoryResearchCommands`(58)를 호출한다. 모두 `RawPacketDispatch&`를 받으므로 같은 Editor dispatcher에 등록할 수 있다. 이는 별도의37/3f/Form/Path 방송 처리기 전체가 복원됐다는 뜻은 아니다.

Client가 소유한 같은 `shared_ptr<EditorLocalTransport>`를 Session 생성 서비스에 전달하고, `ConnectMotion()` 전에 실제 Send 장치를 연결해야 한다. pickup/salvage/move는 송신 함수를 값으로 복사한다. 최종 설치 확정 뒤 입력 공개 전에 실제 세 등록 함수를 한 번 연결하고, Session보다 오래 사는 dispatcher에 해제된 callback이 남지 않도록 반납 경계를 둔다. 송신은 기존 loaded 관문 아래 Login 또는 이 같은 Editor transport를 선택한다. Editor 폴링에는 Login의 Flush/타이머/State/AssignConnection을 추가하지 않는다.

선행 수정이 필요한 실제 관문도 확인했다. `PrepareConstruction()`은 현재 `place.chargeBlocked`를 command에 복사하지만 이 값은 실제 `globals.missionPost(00594fa4)`에 연결되지 않았다. 원문00444af0의 Money 선검사 생략을 현재 전역으로 연결한 뒤,00444760의 Fort 모드 owner 치환을 기존 본문에 남겨야 한다. 송신 adapter가 음수 sender를 먼저 owner1로 바꾸면 순서가 달라진다. 이동40의 Fort 권한 예외와 삭제54의 missionPost 참조는 그대로 재사용한다. 연구58의 객체 owner BYTE 대 sender DWORD 비교는 Editor에서 임의로 완화하지 않는다.

## 단위427 검증 결과

고정5소스로 빌드1을 실행해 **종료0·컴파일 경고/오류0**을 확인했다. 새 `EditorLocalTransport`10개와 기존 `LocalMissionTransport`4개가 **실패0**으로 통과했다. 독립 읽기 검토에서 찾은 sentinel 반복 중단, 반대쪽 socket 소멸, 종료 통지의 현재 첫 BYTE 보존을 해당 회귀들이 구별한다. 보호1821파일과 고정5소스는 검사 뒤에도 SHA가 같다.

[실행 보고서](../../cpppj/editor-local-transport-runtime-20261025.json)에 같은 네 실행 파일 SHA, 빌드와 두 집중 로그를 기록했다. 제품 GUI·원본 게임·전체 CTest·긴 플레이는 이번에 실행하지 않았다. 마지막 통합 검사는 계속345b5691의2843개다. 새 전송 구현은 아직 실제 Session/State1 prefix의 호출자가 없으므로 사용자 편집 완료로 표시하지 않는다. **20261025-22 CP**로 인계 뒤 커밋하고 다음 접합을 진행한다.
