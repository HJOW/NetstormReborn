# dotnetpj 외부 접속 소비자와 클라이언트 검증 비활성화 (2026-11-21 재대조)

cpppj의 [외부 접속 목록](../cpppj/external-endpoints-20261101.json)과 [처리 방침](exe/cpp-external-services-20261101.md)을 dotnetpj에 적용한다. 0.737에서 C# 소스를 읽기 전용으로 다시 검색해 초기 목록의 오래된 소비자 설명을 갱신했다. 이 목록 갱신을 위해 네트워크 탐색·DNS 조회·HTTP 요청이나 도움말 링크 실행을 수행하지 않았다. 원본/CD 파일은 수정하지 않았다.

## dotnetpj 소비자 목록 (C# 정적 검색, 2026-11-21)

| 기능 | dotnetpj 소비자 | 처리 |
| --- | --- | --- |
| 구 클라이언트 검증·security.dat·Not Validated 안내 | 활성 검증 경로 없음. `ClientLocalStateGlobals.UserUpdated`의 최초0 저장소만 있고 현재 다른 소비자는 없다. | 검증 writer/원격 조회/성공 합성을 연결하지 않는다. 필드 존재를 실제 검증 동작으로 보지 않는다. |
| 인터넷 master/root 서버·디렉터리·공식 포트·루트 실행 | `ClientNsInfoGlobals`의 서버 행·이름·선택값, `RootServerRecords.Read/Resolve`의 바이트 판독, `ClientHostGame.NsInfo`의 옵션3값 읽기 | 0.737 추가 경계는 주소/포트/이름 계산뿐이다. DNS·소켓·HTTP를 호출하지 않으며 폐기 서버 접속을 설치하지 않는다. 서버 목록의 실제 파일 적재/Root UI 연결은 후속이다. |
| 인터넷 ping·진단 접속 | `DisplaySettings.PassServerDiagnostic`과 `ClientNsInfoGlobals.PassedDiags` 값 저장 | 새 원격 진단/검증 요청을 보내지 않는다. `PassedDiags` 읽기는 원문 현재DWORD 보존이다. |
| 사용자 선택 도움말 링크 실행 | 기존 경로 `Netstorm.Game/HelpWindow.cs` 329행의 `Process.Start(UseShellExecute=true)` 1곳 | 사용자 클릭(드래그 4px 이내·링크 명중·http/https 절대 URI)에만 실행한다. 클릭 없는 자동 접속·업데이트 다운로드·오류 자동 전송은 구현하지 않는다. |
| 기술 도움말·설정 명령 실행 | 위 도움말 실행 1곳이 전부다 | 고정 원격 서버 주소가 있는 별도 TCP 클라이언트로 보지 않는다. |
| TCP 소켓·호스트 이름 조회 | `Netstorm.Net/LocalTcpTransport.cs`의 `Listen`·`Connect`·`ResolveLoopback`·수락 처리 | 숫자 주소는 점 파서 뒤 `LocalNetworkPolicy.RequireLoopback`으로 제한한다. 비숫자 입력은 **DNS 조회를 먼저 수행한 뒤** 첫IPv4에 같은 루프백 관문을 적용한다. 따라서 외부 서버 연결은 거부해도 입력 호스트의 DNS 조회는 가능한 경계다. 리슨은 Any 또는 허용 주소이고 수락은 루프백만 유지한다. 제품 Root/메뉴의 원격 접속에는 아직 연결하지 않았다. |
| UDP 소켓·자기 호스트 조회 | `Netstorm.Net/LocalUdpTransport.cs`의 `Open`·`Broadcast`·송수신·주소 조회 | 바인딩/송신 대상은 루프백이고 비루프백 수신을 거부한다. 자기 주소 조회에서 `Dns.GetHostName/GetHostAddresses`를 사용한다. 외부 브로드캐스트나 인터넷 서버 탐색을 설치하지 않는다. |
| 싱글/Edit 내부 패킷 | `LocalMissionTransport`·`RawConnectionRegistry`·`ClientConnectionGlobals`·고정 패킷 등록표 | 현재 세션 양끝과 큐/노드/호출 수명이며 이것만으로 외부 소켓을 열지 않는다. 실제 LAN 전체 기능과 Outpost는 R11에서 검증한다. |

## 클라이언트 유효성 검사 비활성화 확인

- dotnetpj에 구 검증 writer·원격 상태 조회·안내 경로가 없다. `UserUpdated` 최초0 저장소와 검증 호출을 구별하며 활성 원격 검증은 비활성 상태를 유지한다.
- `DisplaySettings.PassServerDiagnostic`은 옵션 목록의 값 저장용이며 실제 접속·검증 분기가 없다. 원문 설정값과 혼동하지 않는다.
- 새 경로의 Help 링크(R3 범위), 실제 TCP/UDP·서버 이름·Root 연결 소비자를 추가할 때 이 목록과 검증 비활성 정책을 갱신한다. 숫자 대상 연결·DNS 조회·사용자 선택 링크 실행을 각각 구별한다.

## 근거와 한계

- 기준은 cpppj `external-endpoints-20261101.json`의 5분류(검증·master/root·ping/진단·사용자 선택 실행·로컬)다.
- 이 문서는 C# 소스의 정적 존재 여부만 확인한다. 실제 접속·동적 URL 전체·간접 별칭 부재를 증명하지 않는다.
- 2026-11-03의 `.csproj`뿐/테스트 없음 설명은 현행 코드에 맞지 않아 제거했다. 현재 Net 전송/등록표의 검증 이력은 [현행 인계](../LEFT_JOBS.dotnetpj.md)의 R4-6과 단위 문서를 따른다. 이 문서가 실제 LAN 플레이 완료나 모든 간접 접속 부재를 증명하지 않는다.
