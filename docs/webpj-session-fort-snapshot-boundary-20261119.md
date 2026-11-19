# Fort 스냅샷의 콜백과 편집기 경계 (WB04-E946)

기준선 `225553df`의 `RawCampaignSession.cpp`와 현행 파일을 대조했다. 해당 파일의 기준선 이후 변경은 없다. `LoadExistingEditorWorld`는 `ExistingEditor` 설치 모드에서만 월드를 준비하며, 편집기 checkpoint가 전체 `loaded` 이전에 `SnapshotFortBody`를 호출한다. 이 예외는 Edit·커스텀 맵을 제외한 웹에 필요하지 않다. 웹에서는 설치 전 본문·공개 저장이 거부되고 설치 뒤 같은 본문이 호출된다.

기존 E829 검사에서 명시된 Fort 모드의 실제 TheWarBegins Session을 적재한 뒤 기술 표시와 직접 저장을 호출하던 경로를 실제 `learnNewTechnology` 콜백으로 바꾸었다. 콜백이 같은 Session의 Fort snapshot을 선택하고 TypeNames·Technology·Badges·Territory·Money·지형 섹션을 기록하며 안내까지 실행하는 것을 확인했다. 이 스냅샷을 별도 Session에 다시 적재해 객체 1,875개의 타입·소유자·프레임·좌표 다중집합을 비교했다. SID의 일치는 주장하지 않는다. 검사는 Fort 전역을 명시한 통합 검사이며 실제 Editor 진입이나 브라우저 미션 플레이를 수행한 것은 아니다.

검증: 실제 Battle·Fort 재적재와 중복 기술 획득 3건, 원본 부모 2,728관측과 CoreData·설치 전/반납 뒤 수명을 포함하는 연관 검사 11건이 Node v26에서 통과했다. Battle 스냅샷의 기존 근거는 E698이다. Tutorial1 앱 수명 및 L2 판정은 4D-6 이후에 남는다.
