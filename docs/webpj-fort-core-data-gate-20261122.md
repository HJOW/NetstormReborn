# Fort CoreData 읽기(FortCoreDataLoad)의 웹 범위 판정 (WB04-E1204)

`FortCoreDataLoad.cpp`의 `LoadFortCoreData`(원문 004beca0)는 Fort 의 `CoreData` 구역(시드·세 무결성 블록·최소 병합)을 읽는 함수다. cpppj 구현의 **첫 줄**이 자연 관문이다.

```cpp
void LoadFortCoreData(CampaignTemplateStorage& storage,std::uint32_t slot,bool minimum,FortCoreDataLoadContext c) {
        if (c.missionPost || c.editor) return;
```

- 웹 단독 미션은 항상 `missionPost = true`이다(`core/client-gameplay-globals.js`가 월드를 만들 때 `this.#world.missionPost = true`, `editor = 0`). 스커미시도 같은 미션 월드를 쓴다.
- 호출처(cpppj `RawCampaignSession` 의 `ReadExistingFortCoreData`)는 편집기 월드 적재(`LoadExistingEditorWorld`)와 일반(비미션) 전투의 자기 Fort 적재(`RawCampaignSessionLogin.inc` `LocalFort`)·메타(`UberMeta.inc`)뿐이다.

따라서 단독 웹에서는 이 함수가 어떤 경로에서도 첫 줄에서 반환하고 본문은 실행되지 않는다. 편집기와 멀티플레이·메타는 웹 범위 밖이므로 이식 대상에서 뺀다(`tools/port-map.mjs`의 `mp-fort-core-data` 규칙, `exclude-mp`). 남은 양은 102단위·28,591줄 → 101단위·28,494줄이다.
