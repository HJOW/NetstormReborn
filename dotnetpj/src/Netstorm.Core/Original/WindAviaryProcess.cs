using System.Buffers.Binary;

// Wind의 실제80BYTE/Kernel/Host와 가상Form삭제를 같은 객체에 둔다.
namespace Netstorm.Core.Original;

/// <summary>RawWindAviaryProcess.h의 실제inner+38 Host 어댑터다.</summary>
public sealed class WindAviaryProcess : SquidProcess
{
    /// <summary>같은현재Pool/전역/효과의 비소유 본체다.</summary>
    private readonly RawWindAviaryProcess _logic;
    /// <summary>값 복사해 소유하는 유일한80BYTE다.</summary>
    public WindAviaryProcessFields Fields { get; }
    /// <summary>원문이미지를 복사하고 같은현재 본체를 빌린다.</summary>
    public WindAviaryProcess(RawWindAviaryProcess logic, WindAviaryProcessFields fields) { _logic = logic; Fields = new(fields); }
    /// <summary>실제Kernel 프레임을 원문 상태기계로 전달한다.</summary>
    public override void RunFrame() => _logic.Frame(Fields);
    /// <summary>현재Host 식별자를 실제inner DWORD에 기록한다.</summary>
    public override void OnAttached() { BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(60), (uint)Form()); BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(64), (uint)Parent()); }
    /// <summary>실제Form삭제 전에 원문의 현재임시플라이어를 정리한다. 전달flags는 사용하지 않는다.</summary>
    public override void OnFormDestroy(uint flags) => _logic.FormDestroy(Fields);
    /// <summary>원문00480260의 실제ret12 빈 통지다.</summary>
    public override void Notify(uint message, uint first, uint second) { }
    /// <summary>부착통지 앞의 전송도 같은현재Host 식별자를 기록한다.</summary>
    public ushort WritePacket(Span<byte> packet, bool parentChanged, bool force = false, byte protocol = 1, byte message = 0x74)
    {
        OnAttached(); return RawWindAviaryState.WriteProcess(Fields, packet, parentChanged, force, protocol, message);
    }
}
