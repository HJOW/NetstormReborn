// 건설과 배치가 공유하는 현재 역할·개발자 플래그를 장기 owner에 둔다.
namespace Netstorm.Core.Original;

/// <summary>RawGameAuxState.h의 건설 관련 현재 DWORD다. 일반 월드 준비가 값을 초기화하지 않는다.</summary>
public sealed partial class RawGameAuxState
{
    /// <summary>0054db84의 최초BSS0이며 로딩 깊이와 별개인 섬 정리 억제 DWORD다.</summary>
    public UIntBox IslandDropBlockedBox { get; } = new();
    /// <summary>005411d8의 최초96이며 섬 정리 때마다 읽는 그루터기 TYPE다.</summary>
    public UIntBox IslandStumpTypeBox { get; } = new() { Value = 96 };
    /// <summary>005410e8의 최초30이며 생성/수신/볼텍스가 함께 읽을 진행 프로세스 TYPE다.</summary>
    public UIntBox ConstructionProcessTypeBox { get; } = new() { Value = 30 };
    /// <summary>005326c4의 최초6700이며 추상 건물의 실제 건설 재시도 사건을 매 비교 위치에서 읽는다.</summary>
    public uint ConstructionRegularEvent { get; set; } = 6700;
    /// <summary>005325b8/b9의 최초37/1이며 일반 월드 준비로 초기화하지 않는 현재 메시지/프로토콜 BYTE다.</summary>
    public byte[] ConstructionHeader { get; } = [0x37, 1];
    /// <summary>0055a49c의 최초BSS0이며 비영이면 현재 건설을 즉시 완료한다.</summary>
    public UIntBox QuickBuildBox { get; } = new();
    /// <summary>0055a498의 최초BSS0이며 비영이면 신전genus 건설만 즉시 완료한다.</summary>
    public UIntBox QuickVortexBox { get; } = new();
    /// <summary>005325b4의 최초0x10444200이며 건설 완료와 후속 배치/경로가 공유할 현재 마스크다.</summary>
    public UIntBox ConstructionDisplaceGenusBox { get; } = new() { Value = 0x10444200 };
}
