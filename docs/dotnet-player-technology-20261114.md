# dotnetpj 0.501 — 실제 Player기술 목록과 내용물 사슬

2026-11-14, HJOW-Athlon. R4의 실제AI초기 갱신에 필요한 기술부모를 이식했다. **부품의 원본 대조 완료이며 호스트 결선은 초기갱신 이식 뒤에 한다.** 기존 근사 기술목록이나 별도Player사본을 새 경로에 연결하지 않았다.

## 이식 범위

기준은 `f3a5c821f`의 `cpppj/src/o/RawPlayerTechnology.h/.cpp`와 `cpppj/tests/PlayerTechnologyTests.cpp`다. `RawPlayerTechnology.cs`는 같은 `RawPlayerWorld`·`RawCarrierPreDestroy`·`RawContainedCreate`를 빌린다.

- Find00490330: 타입assert뒤PlayerKnown진단·현재 기술부모를 읽고flags4/kind의첫내용물을 조회한다.
- Clear004903b0: 부모를 한 번 캡처하고 실제 가상삭제 뒤 같은 체인에서 다음내용물을 다시 찾는다. 삭제가Player기술부모를 바꾸어도 처음부모의내용물을 끝까지 제거한다.
- Ensure00490420: 자신의타입진단과 중첩Find진단을 구별하고, 없을 때 현재부모를 다시 읽어 실제Form을(0,4,10,0)으로생성한다.
- Parse004914e0:40개용량·중복제거·group10제외all·항목번호감김·미등록비빈이름의동일항목반복을 보존한다.
- AddList004915c0: 전체목록을 먼저 완성한 다음Ensure한다. 원문의무한반복은생성전명시예외로 남긴다.
- `BindPlayerTechnologyTypes`가현재 `RiftTypeTable.Find`를 직접 빌린다. 기존진단·삭제효과를 보존하고 별도검색표를 만들지 않는다.

C#의차이: 원문의포인터목록을40칸int배열·개수·현재항목번호로 표현하며, 가상삭제는동일풀의내용물을실제로바꾸는대리자를받는다. Config목록해석은이미대조한 `ConfigNumbers.ListItem`을공유한다.

## 검증

원본 `player-technology-x86.tsv`를Core에Link했다. SHA256 `f3970d37d545c272fab7b0e80c04932ef0f6d9838a67f5789fe3b8de7f832690`, 헤더포함159행·158고유입력이다. 원본fixture는불변이다.

`PlayerTechnologyTests.Native158`은반환·두번의무진행·모든조회/진단/생성/삭제/부착순서·최대40개목록·관찰5슬롯전체·새Form전체·Player기술부모를전수대조한다. 별도검사는실제출하아카이브116개 `.type`으로188TYPE를구성해 sunWalker/windBattery/sunCannon조회와미등록이름의생성전거부를검증한다. 기대값을C#출력으로바꾸지않았다.

신규2건+AI본체·내용물생성·fixture무결성의집중16건통과. Find의flags4를1로바꾸는변이는158입력중42입력불일치를검출했다. 원복뒤전체비증분Release빌드 경고0·오류0과PORT_MAP검사를포함한집중18건통과를확인했다.

PORT_MAP2행추가(692=L1 342+port350), fixture238개다. 게임실행·새NB수정·외부접속변화·기존코드은퇴는없다. 보호파일·cpppj·LEFT_JOBS.md·병행webpj작업을보존했다.

다음은원본133+theme5추가16입력의 `RawAiBrainInitialUpdate`와동일기술사슬대조, 그뒤라이브AI생성/초기갱신결선이다. M1전체일치는0/34로유지한다.
