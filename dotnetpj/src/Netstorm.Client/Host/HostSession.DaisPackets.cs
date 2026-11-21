using System.Buffers.Binary;
using Netstorm.Core.Original;

// 현재 Form/PID·장기 TYPE·진단과 Dais의 실제 부분 수신/송신을 연결한다.
namespace Netstorm.Client.Host;

/// <summary>Native6ah와 실제 위치 수신이 같은 Host/공간 수명을 쓰는 부분이다.</summary>
public sealed partial class HostSession
{
    /// <summary>현재 Runtime에 한 번 결합한 같은 Dais 파생 전송 부모다.</summary>
    private ProcessFormTransmission? _daisPacketTransmission;
    /// <summary>기존 파생 writer를 보존하며 같은 실제 공통 버퍼/Native 또는 typed 송신을 연결한다.</summary>
    private void BindDaisPacketTransmission()
    {
        if (_daisPacketTransmission is not null) return;
        _daisPacketTransmission = RawDaisTransmission.Make(GameAux, length =>
        {
            RawSurfaceStorage storage = FixedPacketNeed(_squidPacketStorage);
            if (storage.UsesNativeTransmissionDevices()) { storage.BroadcastProcessForm(length); return; }
            byte[] buffer = GameAux.SquidTransmission.Buffer;
            if (length < 0 || length > buffer.Length) throw new ArgumentOutOfRangeException(nameof(length), "DaisProcess의 실제 송신 범위");
            FixedPacketNeed(_squidPacketEffects?.FormPacket)(buffer.AsMemory(0, length));
        }, PlayerProcesses.Transmission());
        PlayerProcesses.SetFormTransmit(_daisPacketTransmission);
    }
    /// <summary>처음 읽은 Form DWORD/free 여부는 유지하고 복귀한 진단 뒤 현재 mask/PID/TYPE를 사용한다.</summary>
    private void ReceiveDaisPacket(ReadOnlyMemory<byte> packet)
    {
        if (Pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("6ah DaisProcess 수신의 PATCH 판본 불일치");
        uint number = BinaryPrimitives.ReadUInt32LittleEndian(packet.Span.Slice(4, 4));
        if (number > ushort.MaxValue || number >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(packet), "Dais 수신의 Form DWORD 물리 범위");
        int form = (int)number; bool free = (Pool.Slot(form)[11] & 1) != 0; byte mask = packet.Span[8];
        // 같은 현재 Client 진단이 복귀하면 동일 mutable 패킷을 다음 선택 위치에서 다시 읽는다.
        void Diagnose(string text, uint line) => FixedPacketNeed(_squidPacketStorage).AssertDisplay(text, @"..\o\_p_daisprocessdata.cpp", line);
        if (free)
        {
            if (mask != 0xff) Diagnose("*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF", 0x36);
            var pending = new DaisProcess(); bool transferred = false;
            try
            {
                uint parent = RawDaisProcessData.Read(pending.Fields, packet, true), type = GameAux.DaisProcessTypeBox.Value;
                if (parent > ushort.MaxValue || parent >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(packet), "Dais 수신의 parent DWORD 물리 범위");
                transferred = true; _ = PlayerProcesses.Attach(pending, type, (int)parent, 0x10, form); return;
            }
            finally { if (!transferred) pending.Dispose(); }
        }
        if ((mask & 1) != 0) Diagnose("(*(char *)(( (char *)zacket+8) ) & 1) == 0", 0x3e);
        DaisProcess? current = PlayerProcesses.FromForm(form) as DaisProcess;
        _ = RawDaisProcessData.Read(current?.Fields, packet, false);
    }
    /// <summary>client SID/현재 비활성은 일찍 반환하고 실제 Unpop 뒤 frame을 직접 저장한 다음 signed16.16 좌표로 Pop한다.</summary>
    private void ReceivePositionPacket(ReadOnlyMemory<byte> packet)
    {
        ushort number = BinaryPrimitives.ReadUInt16LittleEndian(packet.Span.Slice(4, 2));
        if (number >= 5 && number < 15000) return;
        if ((Pool.Slot(number)[11] & 7) != 0) return;
        UnpopWorldObject(number, 0);
        BinaryPrimitives.WriteUInt32LittleEndian(Pool.AllocatedBytes(number).Slice(36, 4), BinaryPrimitives.ReadUInt32LittleEndian(packet.Span.Slice(14, 4)));
        float x = (float)((double)BinaryPrimitives.ReadInt32LittleEndian(packet.Span.Slice(6, 4)) * (1.0 / 65536.0));
        float y = (float)((double)BinaryPrimitives.ReadInt32LittleEndian(packet.Span.Slice(10, 4)) * (1.0 / 65536.0));
        PopWorldObject(number, x, y, 0);
    }
}
