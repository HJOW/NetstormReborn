using System.Buffers.Binary;

// 실제 Kernel의 동일 객체/이미지를 맨 오 워 풀 본체에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>원문 RainAviaryProcess inner의 실제 Host 어댑터다.</summary>
public sealed class RainAviaryProcess : SquidProcess
{
    /// <summary>현재 Pool/전역/효과를 빌리는 같은 비소유 본체다.</summary>
    private readonly RawRainAviaryProcess _logic;
    /// <summary>값 복사해 소유하는 실제48BYTE 이미지다.</summary>
    public RainAviaryProcessFields Fields { get; }
    /// <summary>caller의 이미지를 복사하고 현재 본체를 빌린다.</summary>
    public RainAviaryProcess(RawRainAviaryProcess logic, RainAviaryProcessFields fields) { _logic = logic; Fields = new(fields); }
    /// <summary>Kernel의 현재 프레임을 실제 상태 기계로 전달한다.</summary>
    public override void RunFrame() => _logic.Frame(Fields);
    /// <summary>같은 Host의 실제 Form/부모를 전체DWORD 폭으로 기록한다.</summary>
    public override void OnAttached()
    {
        BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(36), (uint)Form()); BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(40), (uint)Parent());
    }
    /// <summary>원문00480260은 실제 ret12의 빈 몸체다.</summary>
    public override void Notify(uint message, uint first, uint second) { }
    /// <summary>부착 통지보다 앞선 writer도 현재 Host 식별자를 사용한다.</summary>
    public ushort WritePacket(Span<byte> packet, bool parentChanged, bool force = false, byte protocol = 1, byte message = 0x73)
    {
        OnAttached(); return RawRainAviaryProcess.WriteProcess(Fields, packet, parentChanged, force, protocol, message);
    }
}
