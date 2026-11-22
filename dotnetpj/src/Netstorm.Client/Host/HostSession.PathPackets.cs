using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;

// 실제30h의 free Form 생성/decoder/가상 부착과 부모 재배치를 연결한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignPathBindings.ReceivePacket의 실제 Native 수신이다.</summary>
public sealed partial class HostSession
{
    /// <summary>콜백 뒤 선택된 현재 WORD만 읽으며 헤더 길이로 입력 범위를 대신하지 않는다.</summary>
    private static ushort PathPacketWord(ReadOnlyMemory<byte> packet, int offset)
    {
        if (offset < 0 || offset > packet.Length || 2 > packet.Length - offset) throw new ArgumentOutOfRangeException(nameof(packet), "Path 수신의 선택된 WORD 부족"); return BinaryPrimitives.ReadUInt16LittleEndian(packet.Span[offset..]);
    }
    /// <summary>실제 현재 TYPE constructor/같은 decoder/Host로free Form만 복원하고live는 상세 진단 뒤 반환한다.</summary>
    private void ReceivePathPacket(ReadOnlyMemory<byte> packet)
    {
        RawSurfaceStorage storage = FixedPacketNeed(_squidPacketStorage); _ = PathStarter;
        if (Pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("Path 수신의 실제 PATCH 연결 필요");
        if (packet.Length < 6) throw new ArgumentOutOfRangeException(nameof(packet), "Path 수신의 실제 Form WORD 부족");
        if (PlayerProcessClock.Boss) storage.AssertDisplay("!boss", @"\Ns\O\Pathprocess.cpp", 0x224);
        int form = PathPacketWord(packet, 4); if (form == 0 || form >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(packet), "Path 수신 Form 물리 범위");
        if ((Pool.Slot(form)[11] & 1) == 0)
        {
            string name = SquidPacketType(Pool.Slot(form)[10]).Name; byte[] detail = Encoding.UTF8.GetBytes($"Receiving an invalid update for a path process. Client thinks the form is a {name}.\n");
            storage.RecordAssertionDetail(Encoding.UTF8.GetString(detail.AsSpan(0, Math.Min(255, detail.Length)))); storage.AssertDisplay("0", @"\Ns\O\Pathprocess.cpp", 0x24f); return;
        }
        int parent = PathPacketWord(packet, 6); if (parent >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(packet), "Path 수신 부모 물리 범위");
        uint movement = PriestWorld.MovementProcessType(Pool.Slot(parent)[10]); OriginalTypeRecord type = SquidPacketType(movement);
        RawPathProcess? pending = CreatePathByConstructor(type.ConstructorAddress) ?? throw new InvalidOperationException("Path 수신의 실제 TYPE 생성자 미연결");
        try
        {
            var hooks = new RawPathDataReadHooks
            {
                StagePoint = FixedPacketNeed(_pathEffects).StagePoint,
                Metadata = sid => { RawPathStartType value = FixedPacketNeed(_pathEffects?.Metadata)(sid); return (value.Speed, value.TurningSpeed); },
                Now = () => PlayerProcessClock.Now, Assertion = storage.AssertDisplay, Trace = text => storage.TraceGraph(GameAux.PathTraceLevel, text),
                ClearAnimation = sid => { if (PlayerProcesses.FindEvent(sid, 0x5105) is { } current) PlayerProcesses.Kill(current, 0x40); },
            };
            new RawPathData(Pool.Edition).Read(pending.Runtime, packet, Pool, FixedPacketNeed(_terrainPopTypes), hooks);
            int count = pending.Runtime.Count; RawPathPoint first = count != 0 ? pending.Runtime.Point(0) : new();
            RawPathProcess owned = pending; pending = null; RawPathProcess? attached = RawPathProcess.Attach(PlayerProcesses, owned, parent, 0, form);
            if ((attached?.Runtime.Count ?? count) != 0) { UnpopWorldObject(parent, 0); RawPathPoint point = attached is null ? first : attached.Runtime.Point(0); PopWorldObject(parent, point.X, point.Y, 0); }
        }
        finally { pending?.Dispose(); }
    }
}
