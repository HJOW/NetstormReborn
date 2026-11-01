using System.Runtime.CompilerServices;

// 원본 월드 코어의 L1 재생 검사가 미할당 슬롯의 raw 바이트를 직접 심을 때
// 같은 어셈블리의 RawSidPool.RawSlot 을 쓰기 위한 테스트 전용 공개다.
// 게임·세션 코드의 실행 경로에는 영향을 주지 않는다 (W02 건물 공간 조각, 2026-10-20).
[assembly: InternalsVisibleTo("Netstorm.Core.Tests")]

// Template 저장소(Netstorm.Client/Host/CampaignTemplateStorage)가 DataManager·Template 계층의 비정렬 DWORD 읽기·쓰기와 장치 요구 도우미
// (RawStorageBytes)를 다시 구현하지 않고 같은 것을 쓰기 위한 공개다. 게임 실행 경로의 동작은 바꾸지 않는다 (0.131 TS1, 2026-11-01).
[assembly: InternalsVisibleTo("Netstorm.Client")]
