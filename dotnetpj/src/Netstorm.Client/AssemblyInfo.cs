using System.Runtime.CompilerServices;

// 화면 버퍼·입력 큐의 L1 검사가 내부 생성자와 상태를 직접 쓰기 위한 테스트 전용 공개다.
// 게임 실행 경로에는 영향을 주지 않는다 (2026-10-28 R1).
[assembly: InternalsVisibleTo("Netstorm.Client.Tests")]
