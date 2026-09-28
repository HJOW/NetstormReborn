# 개발용 스프라이트 탐색 뷰어

`_shapes.shp`의 프레임을 원본 팔레트로 표시하고 대응하는 `.type` 클러스터 정보를 확인한다.
지면의 `rim`·`fringe` 변형과 건물의 본체·추가 레이어를 비교할 때 사용한다.

저장소 루트에서 실행한다.

```powershell
dotnet run --project src/Netstorm.Game -- --sprites isle
dotnet run --project src/Netstorm.Game -- --sprites fringe
dotnet run --project src/Netstorm.Game -- --sprites island
```

`--sprites` 뒤에는 `TypeLoadOrder`에 있는 타입 이름을 넣는다. 이름의 대소문자는 구분하지 않는다.
원본 자산 경로는 기존 `GameDataLocator`가 찾으며, 필요하면 `NETSTORM_DATA`로 지정한다.
그림에는 실행 설정의 `battlePal` 팔레트를 적용한다. `--map`과 `--sprites`는 함께 사용할 수 없다.

| 조작 | 동작 |
|---|---|
| 위/아래 방향키 | 앞/뒤 타입 |
| 왼쪽/오른쪽 방향키 | 앞/뒤 프레임 |
| Page Up/Page Down 또는 마우스 휠 | 앞/뒤 20프레임 페이지 |
| Home/End | 타입의 처음/마지막 프레임 |
| 격자 클릭 | 해당 프레임 선택 |
| Esc | 종료 |

오른쪽에는 선택 프레임의 번호·크기·기준점과 대응 클러스터의 이름·플래그·그림 참조를 표시한다.
클러스터 목록 뒤에 붙은 추가 레이어는 이름 대응이 확인되지 않아 `추가 레이어`로 표시한다.
이미지가 아닌 특수 레코드는 디코딩하지 않는다. 페이지 또는 타입이 바뀔 때 이전 텍스처를 해제한다.

자동 캡처 예시:

```powershell
dotnet run --project src/Netstorm.Game -- --sprites isle --screenshot extracted/terrain/sprite-browser-isle.png
```

`--screenshot`의 상위 폴더는 미리 있어야 한다. 이 뷰어는 정적 프레임 탐색용이다.
애니메이션 재생, 팔레트 교체, `.type` 속성 전체 탐색은 후속 작업이다.
