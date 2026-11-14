# dotnetpj 0.502 — 실제 AI초기 갱신과 원본149입력

2026-11-14, HJOW-Athlon. R4의 실제AI초기 갱신 부품을 완료했다. **라이브호스트는 아직 임시brain토큰을 사용한다.** 다음 단위에서0.500생성자·0.501기술처리·이번초기갱신을 같은세션에 묶는다.

## 이식 범위

기준은 `bcad4b479`의 `cpppj/src/o/RawAiBrainInitialUpdate.h/.cpp`와 `cpppj/tests/AiBrainInitialUpdateTests.cpp`다. 새 `RawAiBrainInitialUpdate.cs`는 같은 실제메모리·Player9·기술내용물·TYPE표를 빌린다.

- 600바이트를 다시 복사하기 전에 이름소유자를 캡처한다. 현재미션모드일 때 설정이름 또는 실제언어의defaultAiName을 같은Player원문에 쓴다.
- 실제기술부모가 없으면 Player생성기를 호출하고, 현재부모를 다시 확인한다. 기술문자열은0.501의40개목록·Ensure·실제내용물생성 사슬을 사용한다.
- flags4내용물을 기존원본커서로 순회한다. 빌더목록·원소목록의기존누적개수·워크샾·원소선택·요구에너지최대길이를 보존하고, 원문5분류플래그·최대비용만 초기화한다.
- 에너지길이갱신은 현재문자열을두번조회한다. 비용은float→double·signed64절삭의하위DWORD를 보존하며NaN/FISTP무효결과를0으로처리한다.
- theme5와sun battery는 실제counts[5]를공유한다. 단독battery의중립3치환·한번제거·원래source인덱스를유지하는압축을 그대로옮겼다.
- 실제지갑2d8과SP는재지정하지않는다. 일부효과뒤필수metadata조회가빠져도성공으로치환하지않는다.

C#의차이: C++장기전역참조는현재값조회기와공유워크샾배열, 정렬되지않은원문은byte배열·little endian접근으로표현한다. 8비트이름의표시부호화는기존상위언어경계의책임이며이몸체는원문바이트를복사한다.

## 검증

원본fixture를복사하지않고Core에Link했다. 기대값은불변이다.

| fixture | 행수 | 고유입력 | SHA256 |
| --- | ---: | ---: | --- |
| ai-initial-update-x86.tsv | 134(헤더포함) | 133 | 8784606de578d467f5ab2fbedc05662044782accf770ce9eb9ba1b18665a059d |
| ai-initial-update-theme5-x86.tsv | 16 | 16 | e6ec842056c6df0dc8ec294dc5454775e36ccb2069f996936bf48ccaf2ba77c3 |

신규3건은본문전체2824바이트·빌더40바이트·Player1620바이트·새Form전체·모든조회/생성/부착순서를대조했다. 기존지갑999/새설정StartMoney777의분리와필수metadata누락뒤부분효과도검증했다. 신규포함집중17건통과.

대표변이로battery개수증가를1→2로바꾸면 `Native133`의67/133입력과 `NativeTheme5AndSharedCounter`의5/16입력이실패한다. 원복뒤전체비증분Release빌드 경고0·오류0과PORT_MAP포함집중19건재통과를확인했다.

PORT_MAP2행추가(694=L1 344+port350), fixture240개다.0.500의할당레코드에토큰/크기주석도보충했다. 새NB수정·외부접속변화·기존경로은퇴·게임실행없음. 보호파일·cpppj·LEFT_JOBS.md·병행webpj작업은보존했다.

## 다음 결선

같은세션에서 `RawAiBrainMemory`·생성자·초기갱신·Participation수명을소유해야한다. 사제조회/좌표지역·실제TYPE요구에너지·이름언어조회·현재Player/동맹·기술Form부착을연결한후 `CreateAiBrains`의임시토큰을교체한다. 초기갱신이실제로완료되기전에는토큰만만들어성공으로기록하지않는다. 아직Pop후속·농지·장부·메뉴시작/R5표시는남아있으며M1전체일치는0/34다.
