# dotnetpj 외부 접속 기능 소비자 목록과 클라이언트 유효성 검사 비활성화 (2026-11-03)

cpppj 단위684의 [외부 접속 목록](../../cpppj/external-endpoints-20261101.json)과 [처리 방침](../../docs/exe/cpp-external-services-20261101.md)을 dotnetpj에 적용한다.
원본·CD 파일은 수정하지 않았고 게임을 실행하지 않았다. 네트워크 탐색·DNS 조회·HTTP 요청·프로그램 실행은 하지 않았다.

## dotnetpj 소비자 목록 (C# 정적 검색, 2026-11-03)

| 기능 | dotnetpj 소비자 | 처리 |
| --- | --- | --- |
| 구 클라이언트 검증·security.dat·UserUpdated·Not Validated 안내 | 없음 (`Netstorm.*.cs`에 해당 문자열 없음) | 부재로 비활성화다. 앞으로 연결하지 않고 성공 합성도 하지 않는다. |
| 인터넷 master/root 서버·디렉터리·공식 포트·루트 실행 | 없음 (`Netstorm.Net`은 `.csproj`뿐이며 소켓·HTTP 클라이언트 코드 없음) | 미구현으로 둔다. 3차 로컬 TCP/IP만 사전 구현하며 옛 인터넷 서버를 복구하지 않는다. |
| 인터넷 ping·진단 접속 | 없음 (설정 `PassServerDiagnostic`은 값 저장만 하며 접속 코드 없음) | 미구현으로 둔다. 목록만 cpppj JSON에 보존한다. |
| 사용자 선택 도움말 링크 실행 | 기존 경로 `Netstorm.Game/HelpWindow.cs` 329행의 `Process.Start(UseShellExecute=true)` 1곳 | 사용자 클릭(드래그 4px 이내·링크 명중·http/https 절대 URI)에만 실행한다. 클릭 없는 자동 접속·업데이트 다운로드·오류 자동 전송은 구현하지 않는다. |
| 기술 도움말·설정 명령 실행 | 위 도움말 실행 1곳이 전부다 | 고정 원격 서버 주소가 있는 별도 TCP 클라이언트로 보지 않는다. |
| 로컬 게임 서버·LAN 접속 | 없음 (현재 싱글/Edit 전송은 프로세스 내부다) | 3차 후순위로 TCP/IP 로컬만実装하며 외부 도메인·검증 성공을 요구하지 않는다. |

## 클라이언트 유효성 검사 비활성화 확인

- dotnetpj에 구 검증 writer·상태 조회·안내 문구가 없다. cpppj가 제품 시작 분기에서 제외한 것과 같은 효과를 부재로 유지한다.
- `DisplaySettings.PassServerDiagnostic`은 옵션 목록의 값 저장용이며 실제 접속·검증 분기가 없다. 원문 설정값과 혼동하지 않는다.
- 새 경로의 Help 링크(R3 범위)를 옮길 때마다 이 문서의 사용자 선택 정책을 따른다. `Netstorm.Net`(3차)을 옮길 때도 이 목록과 검증 비활성 정책을 적용한다.

## 근거와 한계

- 기준은 cpppj `external-endpoints-20261101.json`의 5분류(검증·master/root·ping/진단·사용자 선택 실행·로컬)다.
- 이 문서는 C# 소스의 정적 존재 여부만 확인한다. 실제 접속·동적 URL 전체·간접 별칭 부재를 증명하지 않는다.
- 다음은 멀티플레이 사전 구현(로컬 TCP/IP 골격, 테스트 없음)이다.
