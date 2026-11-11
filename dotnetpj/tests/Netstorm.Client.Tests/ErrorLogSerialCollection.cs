namespace Netstorm.Client.Tests;

/// <summary>
/// 오류 로그 전역·콘솔·환경 변수를 쓰는 검사의 직렬 묶음이다. 프로세스 전역을 건드리므로 서로 겹치지 않게 돌린다.
/// </summary>
[CollectionDefinition("ErrorLogSerial", DisableParallelization = true)]
public sealed class ErrorLogSerialCollection
{
}
