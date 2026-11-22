using System.Buffers.Binary;

// 실제 Kernel의 단일 이미지 소유권을 공통 비행 본체와 연결한다.
namespace Netstorm.Core.Original;

/// <summary>RawFlyerProcess.cpp의 실제 FlyerProcess inner 어댑터다.</summary>
public sealed class FlyerProcess : SquidProcess
{
    /// <summary>같은 실제 Pool을 읽는 비소유 공통 본체다.</summary>
    private readonly RawFlyerProcess _logic;
    /// <summary>객체가 값 복사해 소유하는 기본/Sun 추가 이미지다.</summary>
    public FlyerProcessFields Fields { get; }
    /// <summary>두 원문 이미지를 모두 복사하고 같은 본체를 빌린다.</summary>
    public FlyerProcess(RawFlyerProcess logic, FlyerProcessFields fields) { _logic = logic; Fields = new(fields); }
    /// <summary>현재 Kernel 프레임을 실제 큰 비행 장치로 전달한다.</summary>
    public override void RunFrame() => _logic.Frame(Fields);
    /// <summary>원문 현재 Host의 Form/부모를 같은 inner DWORD에 저장한다.</summary>
    public override void OnAttached()
    {
        BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(52), (uint)Form()); BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(56), (uint)Parent());
    }
    /// <summary>현재 객체의 실제 표적 의존 통지를 전달한다.</summary>
    public override void Notify(uint message, uint first, uint second) => _logic.Notify(Fields, message, first, second);
    /// <summary>원문이 사용하지 않는 부모 인수 대신 현재 이미지의 홈 통지를 전달한다.</summary>
    public override void OnParentDying(int parent) => _logic.ParentDying(Fields);
    /// <summary>부착 통지보다 앞선 writer도 현재 실제 Host 식별자를 기록한다.</summary>
    public ushort WritePacket(Span<byte> packet, bool parentChanged, bool force = false, byte protocol = 1, byte message = 0x39)
    {
        OnAttached(); return RawFlyerProcess.WriteProcess(Fields, packet, parentChanged, force, protocol, message);
    }
}
