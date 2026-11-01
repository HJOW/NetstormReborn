using System.Buffers.Binary;
using System.Text;

namespace Netstorm.Client.Mission;

/// <summary>Client가 미션 요청과 이후 State 실행에 공유하는 현재 저장소다.</summary>
public sealed class MissionFortRequestGlobals
{
    public uint[] Next { get; } = new uint[36];
    public byte[] LocalAddress { get; } = new byte[9];
    public uint Sequence { get; set; }
    public uint Port { get; set; }
}

/// <summary>요청이 소유한 Zone 복사본과 선택된 요새의 수명이다.</summary>
public sealed class MissionFortLoadRequest
{
    public string Fort { get; set; } = "";
    public byte[] Zone { get; internal set; } = [];
    public bool Consumed { get; set; }
}

/// <summary>실제 요새 선택과 패킷 복제 효과다.</summary>
public sealed class MissionFortRequestHooks
{
    public Action<string>? SelectFort { get; init; }
    public Func<byte[], uint>? Duplicate { get; init; }
}

/// <summary>원본 004b51e0 요새 요청과 로컬 Zone 패킷을 작성한다.</summary>
public static class MissionFortRequest
{
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("Fort 요청 실제 효과 누락");
    private static void Put(byte[] buffer, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(buffer.AsSpan(offset, 4), value);

    /// <summary>512바이트 물리 저장소에 원본의 네 클라이언트 슬롯을 만든다.</summary>
    public static byte[] MakeLocalMissionZone()
    {
        byte[] packet = new byte[512]; Array.Fill(packet, (byte)0xff);
        packet[0] = 1; packet[1] = 0x1a;
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(2), 86);
        Put(packet, 4, 0x82000004); packet[8] = packet[9] = 0; Put(packet, 10, 0); packet[14] = 1;
        string[] names = ["zsimpson", "jgreer", "kdemarest", "tbratton"];
        for (int i = 0; i < names.Length; i++)
        {
            int offset = 15 + i * 71; packet[offset] = (byte)i; Put(packet, offset + 1, (uint)i + 1); Put(packet, offset + 5, 0);
            int length = Encoding.ASCII.GetBytes(names[i], packet.AsSpan(offset + 9)); packet[offset + 9 + length] = 0; packet[offset + 41] = 0;
        }
        return packet;
    }

    /// <summary>실제 요새 선택 후 현재 주소와 포트를 쓰고 복제 뒤 순번을 증가한다.</summary>
    public static void Write(string name, MissionFortRequestGlobals globals, MissionFortRequestHooks hooks)
    {
        int nul = name.IndexOf('\0'); Need(hooks.SelectFort)(nul < 0 ? name : name[..nul]);
        byte[] packet = MakeLocalMissionZone(); packet[2] = 4; packet[3] = 0;
        globals.Next[0x54 / 4] = BinaryPrimitives.ReadUInt32LittleEndian(globals.LocalAddress.AsSpan(0, 4));
        globals.Next[0x58 / 4] = BinaryPrimitives.ReadUInt32LittleEndian(globals.LocalAddress.AsSpan(4, 4));
        globals.Next[0x5c / 4] = (globals.Next[0x5c / 4] & 0xffffff00u) | globals.LocalAddress[8];
        globals.Next[0] = 7; globals.Next[0x7c / 4] = globals.Port;
        globals.Next[0x80 / 4] = Need(hooks.Duplicate)(packet); globals.Sequence = unchecked(globals.Sequence + 1);
    }

    /// <summary>관찰기의 실제 길이를 확정해 요청이 소유한 복사본을 만든다.</summary>
    public static uint CaptureZone(MissionFortLoadRequest request, byte[] packet)
    {
        if (request.Consumed || request.Zone.Length != 0 || packet.Length != 512) throw new InvalidOperationException("Fort 요청 Zone 중복/예약 크기 오류");
        int length = 15 + packet[14] * 71;
        if (length > packet.Length) throw new ArgumentOutOfRangeException(nameof(packet));
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(2), (ushort)length);
        request.Zone = packet.AsSpan(0, length).ToArray(); return 1;
    }
}
