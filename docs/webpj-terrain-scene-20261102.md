# webpj 정적 지형 장면·개발용 화면 — WB03-AA

2026-11-02, HJOW-Athlon. WB03-Z의 공통 출력기를 실제 `.fort` 지면·절벽·작은 받침 계산에 연결했다. **중립 정적 지형을 확인하는 개발용 화면**이며 M2 전체나 미션 실행 완료는 아니다.

## 1. 근거·보존 범위

AGENTS.md·공통/dotnetpj/webpj 인계를 확인하고 `git fetch`를 수행했다. 현재 PC는 마지막 전체 정밀 디컴파일 호스트(HJOW-Athlon/2026-10-23)와 같다. 디컴파일 산출물을 읽거나 원본 게임·cpppj EXE·analyzeManager를 실행하지 않았다.

검토 기준은 `67aae862` 트리다. 추가로 읽은 근거는 cpppj `o/RiftTypeTable.cpp`(23개 zorder 이름/문자열 해석/숫자 변환), `SquidFactory.cpp`(PostCreate의 깊이 BYTE), `RawTerrainStamp.cpp`(지면/절벽의 소유자0·절벽 y+4), `RawFortObjectLoad.cpp`(noIsland 읽기에서 소유자0), `RawIslandPostPop.cpp`(F 칸의 받침 위치), `RawBridgeConnect.cpp`(같은 위치의 바위·색 번호별 받침 프레임), `RawPlayerColor.h`다. `RawRendererClouds.cpp`·`ClientRendererFrame.cpp`도 읽었지만 전체 배경 경로를 이식하지는 않았다. 기존 dotnetpj 구현은 근거로 쓰지 않았다.

AGENTS.md·보호 원본/CD·cpppj·dotnetpj·공통 인계·README·자산/매니페스트는 수정하지 않았다. 작업 중 생긴 다른 세션의 dotnetpj 파일도 커밋에 넣지 않는다. 새 파일과 주석은 UTF-8·한국어다.

## 2. 장면 조립

`assets/terrain-scene.js`의 `buildTerrainScene(fort, table, options)`는 기존 판독/영역 배치/마스크/타일/절벽/받침 함수를 호출해 `WorldSpriteRenderer` 입력을 만든다. 새 게임 상태나 SID를 부여하지 않는다.

- **깊이:** 타입의 마지막 `zorder` 속성을 읽는다. 문자열은 원본23이름의 표 순서·부분 문자열 첫 일치·대소문자 구분·선택적 정수 +/- 규칙을 따른다. 숫자는 cpppj `ToInt` 범위, 슬롯에는 하위 BYTE를 signed로 저장한다. 실제 지면/받침 윗면은 **0**, 절벽/받침 바위는 **20**이다. 옛 근사 GameWorld의 지면30/절벽40 상수를 사용하지 않는다.
- **지면/절벽:** 기존 난수·CRT·stampVariants·themeOf를 주입할 수 있다. 첫 지면 프레임 뒤 절벽 프레임을 선택하는 난수 순서를 그대로 쓴다. 절벽은 지면 칸에서 y+4에 놓이고 생성 소유자 색은0이다.
- **받침:** F 칸에서 오른쪽 아래 anchor(x+2,y+2)에 `island`를, 그 최초 Pop에서 같은 위치에 `islandStalag`를 놓는다. 색0이면 프레임8, 명시적인 색1~8이면 프레임0~7이다. 저장된 noIsland.owner를 실제 소유자로 읽지 않는다. 원본 로더는 먼저0을 쓰며 이후 소유자 마감이 별도다.
- **색/원소:** `colorOfTerritory(영역)`·`colorOfSupport(받침)`로 실제 색 번호를 공급할 수 있다. 생략하면 모두 중립0이다. `themeOf`가 없으면 지역 원소 도장도 하지 않는다. 플레이어 번호→색 번호를 추측하거나 저장된 신전만 보고 지역 소유권을 근사하지 않는다.
- **표시 자료:** `entries`에 land/fringe/support/stalag를 구별해 담고 읽기 전용으로 만든다. 마스크·프레임 배열·배치는 진단용으로 함께 반환한다. full SID 생성/삭제·연결/점유·표시 수집 순서를 복원한 것은 아니다.

`loadTerrainScene(files, name)`는 이미 받은 `GameFiles`만 사용한다. 공개 아카이브의 공식32맵에 접근할 수 있으며 새 네트워크 요청·사용자 파일 가져오기는 없다. 이름은 경로 없는 파일 이름이고 `.fort` 확장자는 생략 가능하다. 조회 실패 시 명시 오류를 낸다. webpj에서 제외한 Edit/사용자 맵 기능을 추가하지 않는다.

## 3. 화면과 입력

```powershell
npm --prefix webpj start
# 브라우저에서 아래 개발용 경로를 연 뒤 클릭하여 시작한다.
# http://localhost:9803/?map=savetheisland&mute&lang=ko&aspect=4:3
```

일반 주소는 기존 메뉴로 시작한다. `?map=이름`을 지정하면 기존 로딩·시작 클릭을 거친 뒤 `AppState.TERRAIN_PREVIEW`로 간다. 콘솔/호스트의 `app.previewMap('savetheisland')`도 시작 뒤 사용할 수 있다. `closeTerrainPreview()` 또는 Escape로 기존 메뉴에 돌아간다. 미션 명령 `MissionBegin`·`StartDemo`를 미리보기로 바꾸지 않았다.

`ui/terrain-preview.js`는 실제 SHP 픽셀 범위를 합쳐 처음 카메라를 가운데에 놓고, 크기를 바꾸면 현재 월드 중심을 유지한다. 방향키는4칸, 왼쪽 드래그는 논리 픽셀 차이로 이동한다. 안내 띠에서 끌기를 시작하지 않고, 포인터 취소/포커스 상실/resize/메뉴 복귀에서 끌기를 지운다. 입력은 점검 정책이며 원본 미션 카메라의 시작/제한/보간·가장자리 이동으로 세지 않는다.

화면은 기존8비트 표면·팔레트·표시기와 영어 비트맵 글꼴/한국어 D2Coding을 사용한다. 화면 아래에 정적 미리보기임과 조작법을 표시한다. 변경이 없으면 다시 합성하지 않는다. 구름은 기존 메뉴의 고정 타일이고 실제 카메라 배경 위상·날씨·동적 구름은 아직 연결하지 않았다. 미션 Config·기술·SP·진행 저장은 바꾸지 않는다. 맵 조회가 실패해도 현재 화면을 보존한다.

## 4. 검증

- 신규 `terrain-scene.test.mjs` **6/6 통과**. 깊이23이름/부분 일치/범위/하위 BYTE, 공식32맵의 지면·받침 개수(기존 독립 Python 기대값), 실제 모든 제출 프레임 범위, 중립/명시 색과 절벽0, 받침/바위 위치, 경로/누락 거부, 실제 합성과 반복성, 드래그/키/resize/취소, 시작 관문·조회 실패 보존·메뉴 복귀·미션Config 불변을 확인했다.
- `npm --prefix webpj test`: **264개 중262개 통과, 실패2, 건너뜀0**, 약22.95초. 실패는 기존 `fonts/License.txt` CRLF와 웹 복사본 LF 차이로 자산/글꼴 비교가 실패하는 환경 문제다. 이번에 자산이나 기대값을 바꾸지 않았다.
- 실제 Codex IAB 브라우저: 영어4:3·한국어16:9에서 시작 클릭 전 미리보기 없음→클릭 뒤 지형 표시, 방향키·드래그로 이동, Escape로 메뉴 복귀, 언어/화면비를 바꾼 새 로딩을 확인했다. 한국어 안내 띠도 출력됐다. 경고/오류 로그는0이었다. 시험은 `mute`였고 소리·다른 브라우저/OS·고DPI·같은 문서에서 destroy/init 반복은 확인하지 않았다. 도구의614×614 뷰포트에서 레터박스와 함께 보았으며 별도 화면 배율/픽셀 비율을 측정하지 않았다.
- 원본 `screenShots/Save the Islands! - Started.png`도 읽었다. 원본은 게임의 시작 카메라·소유자/원소 도장·건물과 덱을 포함한다. 이번 화면은 전체 지형 가운데의 중립 미리보기라 **같은 카메라/상태의 픽셀 비교나 L4 통과로 세지 않는다.**
- 브라우저 결과 캡처는 이번 채팅 outputs의 `webpj-terrain-preview.jpg`에 저장했다. 저장소에는 캡처를 추가하지 않았다. 점검용 서버는 종료한다.

## 5. 다음 시작점

**M2 4단계의 지형 장면 연결만 완료했다.** 다음은 저장 오브젝트와 생성자/가상 Draw의 실제 프레임·소유자 마감·신전 원소 투표/지역 도장·플레이어 색 번호·그림자 색표를 잇는 작업이다. 실제 배경/카메라 정책, 덱·미니맵 등 UI를 모으고 메뉴의 미션 선택으로 정적 화면을 연 뒤 동일 상태의 원본 캡처와 대조한다. 임의의 기본 프레임이나 저장 소유자 값만으로 건물/유닛을 그려 미션 시작 상태 완료로 처리하지 않는다. `edgeFarm`은 기존 계획대로 WB04 이후다.
