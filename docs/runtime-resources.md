# 실행 자산·설정·언어 연결

> 2026-09-28 · `GameResources`와 MonoGame 실행 프로젝트 연결

게임 실행 코드는 `GameDataLocator`로 찾은 데이터 폴더를 `GameFileSystem`으로 열고,
`GameResources`에서 설정·팔레트·셰이프·타입·맵·번역·미션 스크립트를 조회한다.
디스크 → 이름순 `*.tarc` → 보조 폴더 순서의 파일 조회를 공유한다.
타입 정의도 이 순서를 사용하므로 느슨한 `.type` 파일이 아카이브 정의보다 우선한다.
UTF-8로 작성한 타입 속성과 한국어 텍스트도 읽을 수 있다.

## 설정 구성

1. 필수 경로 지정식의 최소 기본값을 바닥에 둔다. 설정 파일이 없는 개발용 데이터에서도 사용한다.
2. `d/options.cfg` → `d/setup.cfg`를 읽어 **한 설정 텍스트에 이어 붙인다**.
   원본의 첫 일치 조회 규칙 때문에 options 값이 setup 기본값보다 우선한다.
3. 영어 `config.english`를 별도 층에 올리고, 선택 언어 용어표가 있으면 그 위에 올린다.
   일부만 번역한 용어표의 누락 키는 영어 용어표에서 찾는다.
4. 선택한 `currentLanguage`를 최상단에 둔다. 용어표의 같은 키가 선택 언어를 바꾸지 않는다.

인코딩된 `.cfg`와 평문 설정은 `ConfigText.FromFileBytes`에서 구별한다.
원본의 전체 시작 목록(identity·user·dev·guild 설정)까지 연결한 것은 아니다.
원본 설정 파일에 쓰지 않으며 창 크기·전체화면·오디오 설정의 적용과 저장은 후속 작업이다.

팔레트는 `GamePalSpec`과 `fortPal`(기본 확인 화면) 또는 `battlePal`(맵 뷰어)을 사용한다.
셰이프는 `DataDir/_shapes.shp`, 맵 이름은 `fortSpec`으로 경로를 만든다.
`--map`의 명시적 실제 파일은 직접 읽고, `d/b0.fort` 같은 가상 경로도 받을 수 있다.

## 언어 선택과 대체

선택 순서는 **`--language` → 설정의 `currentLanguage` → OS UI 언어 → 영어**다.
옵션 값은 `english`, `korean`, `german`, `french`, `spanish`, `japanese`, `portuguese`이며
대소문자를 무시한다. 알 수 없는 값은 영어로 정규화한다.

```powershell
dotnet run --project src/Netstorm.Game -- --language german
dotnet run --project src/Netstorm.Game -- --map savetheisland --language korean
```

용어표는 `languageSpec`, UI 번역표는 `d/xlat.<언어>`에서 찾는다.
선택 언어 파일이 없으면 영어 파일로 대체하며, 영어 번역표도 없으면 영어 원문을 반환한다.
기본 확인 화면에는 선택 언어·실제 용어표·번역표·치환된 용어가 표시된다.
맵 뷰어에는 선택 언어를 표시한다. 개발용 안내 문구 전체를 번역한 기능은 아니다.

`TryLoadMission`은 `missionSpec`에 미션 이름과 선택 언어를 넣는다.
파일이 없으면 지정식의 폴더·접두어를 유지한 채 영어로 다시 조회한다.
반환값 `LoadedMission`에는 실제 파일 경로·사용 언어·해석된 스크립트가 있다.
영어 파일을 읽어도 전역 선택 언어를 바꾸지 않으며 임시 설정 층은 호출 뒤 제거한다.
`ConfigStore.MissionLoader`도 연결하여 `{@tutorial1.missionType}` 같은 머리 값 치환에 사용한다.
미션의 조건 태그·명령 실행·HTML 렌더링은 아직 구현하지 않았다.

## 검증

`GameResourcesTests` 검사 10개를 추가하여 전체 **84개 통과**를 확인했다.
설정 우선순위, 부분 번역의 영어 용어 대체, UTF-8 한국어 용어·번역·미션,
사용자 지정 경로의 영어 미션 대체, OS 언어, 설정으로 지정한 팔레트를 검사한다.
원본 통합 검사에서는 셰이프 116블록, 팔레트, 맵 이름·가상 경로·명시 파일,
미션 머리 값 참조와 느슨한 UTF-8 타입 우선순위를 확인했다.

솔루션 빌드 성공(기존 NU1900 취약성 조회 연결 경고), MonoGame PNG 저장·자동 종료도 확인했다.
캡처는 `extracted/screens/resources-german.png`, `resources-savetheisland-korean.png`이며
추출 결과이므로 저장소에 포함하지 않는다.
