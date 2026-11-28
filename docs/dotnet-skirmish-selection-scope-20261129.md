# dotnetpj 스커미시 선택 표시 적용 범위 (2026-11-29)

사용자가 세 프로젝트의 스커미시 설정에서 선택 상태를 `*` 문자 대신 메인 Options와 같은 아이콘으로 표시하도록 요청했다. 현재 dotnetpj는 스커미시 설정 화면이 아직 없으므로 제품 코드 교체를 진행하지 않고 R8 12차의 완료 기준에 이 요구를 추가했다. 기존 Scene/State·모딩·생산 격자 작업의 일시 중지는 유지한다.

## 확인한 구현

- `dotnetpj/src/Netstorm.Client/ProductPresentation.cs`의 `TellBody`는 Campaign 목록에 `Patch1101.Skirmish` Tell 항목만 삽입한다. 같은 파일의 설명도 스커미시 화면 자체는 R8에서 연결한다고 명시한다.
- `dotnetpj/src/Netstorm.Client/Host/HostStartupScene.cs`는 `Patch1101` 설정 객체에 Credits 본문만 등록한다. 현재 DN 소스에는 스커미시 설정 본문, AI 수·난이도·동맹·금지 유닛 선택 생성기, 이 화면의 `*` 선택 접두어가 없다. 따라서 현재 DN 화면의 별표를 교체했다고 보고하지 않는다.
- 기존 HTML 메뉴 선택 계약은 `Gump/HtmlWindowMetadata.cs`의 `$Checked=라벨,명령,인자,선택값,활성값`이다. `BindHtmlWindowMetadataMenus`가 이를 `MenuGumpControls.AddChecked`에 전달한다.
- `Gump/MenuGumpControls.cs`의 `AddChecked`는 선택 시 항목 플래그 `4`를 설정한다. `Gump/MenuGumpDraw.cs`의 `Draw`가 같은 플래그로 `MarkerType` 아이콘을 출력하고, `BindMenuGumpDrawAssets`는 해당 타입의 `J` 첫 프레임 번호 `+2`를 기존 장식 그림 경로로 그린다. 별도 글자나 새 이미지가 필요하지 않다.

## 향후 완료 기준과 검증 범위

R8 12차 화면을 연결할 때 AI 수·개별 난이도·동맹·금지 유닛의 모든 선택 행은 위 `$Checked` 경로를 사용한다. 각 메뉴를 다시 열어 현재값에만 기존 Options 아이콘이 나타나고, 값을 바꾸면 이전 행의 아이콘이 사라지는지 확인한다. 별표 접두어를 만드는 방식은 사용하지 않는다.

이번 작업은 소스 검색과 기존 표시 경로의 정적 확인만 수행했다. 제품 코드·자산은 변경하지 않았으므로 빌드·게임 실행·시험을 추가 실행하지 않았고 제품1.229 및 정기 전체 회귀 주기는 올리지 않았다.
