// 건설과 배치가 공유하는 현재 역할·개발자 플래그를 장기 owner에 둔다.
namespace Netstorm.Core.Original;

/// <summary>RawGameAuxState.h의 건설 관련 현재 DWORD다. 일반 월드 준비가 값을 초기화하지 않는다.</summary>
public sealed partial class RawGameAuxState
{
    /// <summary>005410e8의 최초30이며 생성/수신/볼텍스가 함께 읽을 진행 프로세스 TYPE다.</summary>
    public UIntBox ConstructionProcessTypeBox { get; } = new() { Value = 30 };
    /// <summary>0055a49c의 최초BSS0이며 비영이면 현재 건설을 즉시 완료한다.</summary>
    public UIntBox QuickBuildBox { get; } = new();
    /// <summary>0055a498의 최초BSS0이며 비영이면 신전genus 건설만 즉시 완료한다.</summary>
    public UIntBox QuickVortexBox { get; } = new();
    /// <summary>005325b4의 최초0x10444200이며 건설 완료와 후속 배치/경로가 공유할 현재 마스크다.</summary>
    public UIntBox ConstructionDisplaceGenusBox { get; } = new() { Value = 0x10444200 };
}
