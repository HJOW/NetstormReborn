# Template와 애시드 공유 전역 재반영

WB04-E509는 `CampaignTemplateStorage`와 `RawAcidZap`의 기존 이식분을 후보 `225553df`와 비교해 모두 **② 웹 반영 필요**로 판정했다.

`CampaignTemplateStorage.createSibling()`은 같은 직렬화 전역과 섹션 이름표를 빌리는 새 저장소를 만든다. 형제는 헤더와 이미지 할당을 독립 소유하며 생성 자체가 공유 판본이나 로딩 깊이를 초기화하지 않는다. `loadingDepth` 접근은 기존 `serializationGlobals`의 같은 DWORD에 연결한다.

`RawAcidZap`은 선택적인 `sharedAltar`·`sharedGrowingIce` 조회기가 있으면 포획 수신에서 실제 도달한 순서에 현재 TYPE을 읽는다. 조회기가 없으면 기존 독립 입력을 사용하며, 죽은 대상이나 먼저 제외된 대상에서는 뒤의 조회기를 호출하지 않는다.

관련 검사 25건은 원본 애시드 수신 927행·헤더 90행을 포함해 모두 통과했다. 추가 검사는 형제 저장소의 공유·독립 소유와 현재 역할 조회의 단락 순서를 확인한다. 실제 Session에서 장기 전역과 소리·분해·삭제 장치를 연결하는 일은 4-D에 남는다.
