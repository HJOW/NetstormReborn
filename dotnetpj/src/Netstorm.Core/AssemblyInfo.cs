using System.Runtime.CompilerServices;

// 원본 월드 코어의 L1 재생 검사가 미할당 슬롯의 raw 바이트를 직접 심을 때
// 같은 어셈블리의 RawSidPool.RawSlot 을 쓰기 위한 테스트 전용 공개다.
// 게임·세션 코드의 실행 경로에는 영향을 주지 않는다 (W02 건물 공간 조각, 2026-10-20).
[assembly: InternalsVisibleTo("Netstorm.Core.Tests")]
