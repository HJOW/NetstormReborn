// 로그인 송신기와 수신기가 같은 이름 정제·원문 observer를 공유한다.
namespace Netstorm.Core.Original;

/// <summary>RawMissionClientPackets의 이름 정제와 2a/2b/67/81 및 metadata 관찰 본체다.</summary>
public static class RawMissionPacketFields
{
    /// <summary>원본 이름에서 삭제하는 단일바이트 문자표다. A0도 동일하게 제거한다.</summary>
    private const string Forbidden = "\u00a0%.?=()[]~\"`;: ";

    /// <summary>현재 NUL까지 금지문자를 삭제하며 뒤쪽 stale 바이트는 보존한다. 빈 이름은 Cheat+subscriber의 낮은BYTE다.</summary>
    public static void SanitizePlayerName(uint subscriber, Span<byte> name)
    {
        int length = name.IndexOf((byte)0);
        if (length < 0) throw new ArgumentOutOfRangeException(nameof(name), "미션 이름 NUL 누락");
        int index = 0;
        // 삭제한 자리는 다시 검사하여 연속 금지문자도 한 칸씩 제거한다.
        while (index < length)
        {
            if (Forbidden.IndexOf((char)name[index]) < 0) { index++; continue; }
            // 현재 NUL까지 옮기며 그 뒤의 원문 바이트는 건드리지 않는다.
            for (int next = index; next < length; next++) name[next] = name[next + 1];
            length--;
        }
        name[length] = 0;
        if (length != 0) return;
        byte[] replacement = System.Text.Encoding.Latin1.GetBytes("Cheat" + (subscriber & 255).ToString(System.Globalization.CultureInfo.InvariantCulture));
        CopyCString(name, 0, replacement);
    }

    /// <summary>송신만 현재 가변길이 WORD를 쓴다. 진단 뒤 길이 필드를 다시 읽으며 수신은 물리 버퍼를 읽지 않는다.</summary>
    public static void ObserveClient(uint type, Memory<byte> packet, uint sending, Action<string, string, uint>? assertion = null)
    {
        if (sending == 0) return;
        switch (type)
        {
            case 0x2a:
                if (Get(packet.Span, 12) == uint.MaxValue) Need(assertion)("fortFileSize != (int)-1", "\\Ns\\O\\_p_packets.cpp", 0x672);
                Put(packet.Span, 2, unchecked(Get(packet.Span, 12, 2) + 0x11), 2); return;
            case 0x2b:
                if (Get(packet.Span, 0x33, 1) == 255) Need(assertion)("nameLen != (unsigned char)-1", "\\Ns\\O\\_p_packets.cpp", 0x695);
                Put(packet.Span, 2, Get(packet.Span, 0x33, 1) + 0x3b, 2); return;
            case 0x67:
                if (Get(packet.Span, 5, 2) == 65535) Need(assertion)("dataLen != (unsigned short)-1", "\\Ns\\O\\_p_packets.cpp", 0xa07);
                Put(packet.Span, 2, Get(packet.Span, 5, 2) + 7, 2); return;
            case 0x81:
                if (Get(packet.Span, 4) == uint.MaxValue) Need(assertion)("numPlayers != (int)-1", "\\Ns\\O\\_p_packets.cpp", 0xc3d);
                Put(packet.Span, 2, ((Get(packet.Span, 4, 2) + 2) & 65535) * 4, 2); return;
            default: return; // 나머지 등록의 실제 빈 observer는 버퍼를 읽지 않는다.
        }
    }

    /// <summary>40a450은 두 송신 진단 뒤 현재 이름BYTE/이미지WORD로 길이WORD만 확정한다.</summary>
    public static void ObserveLogin(Memory<byte> packet, uint sending, Action<string, string, uint>? assertion = null)
    {
        if (sending == 0) return;
        if (Get(packet.Span, 12, 1) == 255) Need(assertion)("len != (unsigned char)-1", "\\Ns\\O\\_p_packets.cpp", 0x611);
        if (Get(packet.Span, 13) == uint.MaxValue) Need(assertion)("fortFileSize != (int)-1", "\\Ns\\O\\_p_packets.cpp", 0x612);
        Put(packet.Span, 2, Get(packet.Span, 12, 1) + Get(packet.Span, 13, 2) + 17, 2);
    }

    /// <summary>0040b110은 송신의 len!=-1 진단 뒤 현재 lowWORD+10만 기록한다.</summary>
    public static void ObserveMetadata(Memory<byte> packet, uint sending, Action<string>? assertion = null)
    {
        if (sending == 0) return;
        if (Get(packet.Span, 6) == uint.MaxValue) Need(assertion)("len != (int)-1");
        Put(packet.Span, 2, Get(packet.Span, 6, 2) + 10, 2);
    }

    /// <summary>선택된 하위만 요구하여 앞서 발생한 이름·필드 쓰기를 보존한다.</summary>
    internal static T Need<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("미션 패킷의 실제 하위 미연결");

    /// <summary>정렬되지 않은 BYTE/WORD/DWORD를 낮은 바이트부터 읽는다.</summary>
    internal static uint Get(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        if ((uint)offset > bytes.Length || width < 1 || width > 4 || width > bytes.Length - offset) throw new ArgumentOutOfRangeException(nameof(offset), "미션 패킷 읽기 물리 범위");
        uint value = 0;
        // 원문 폭만 읽어 주변 stale 바이트를 입력으로 사용하지 않는다.
        for (int index = 0; index < width; index++) value |= (uint)bytes[offset + index] << (index * 8);
        return value;
    }

    /// <summary>지정한 BYTE/WORD/DWORD의 낮은 비트만 쓰고 인접 바이트를 보존한다.</summary>
    internal static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        if ((uint)offset > bytes.Length || width < 1 || width > 4 || width > bytes.Length - offset) throw new ArgumentOutOfRangeException(nameof(offset), "미션 패킷 쓰기 물리 범위");
        // 원문 폭에서 넘치는 비트는 원본 저장처럼 버린다.
        for (int index = 0; index < width; index++) bytes[offset + index] = unchecked((byte)(value >> (index * 8)));
    }

    /// <summary>첫 NUL 앞까지만 복사하고 실제 목적지 끝에 NUL을 쓴다.</summary>
    internal static void CopyCString(Span<byte> bytes, int offset, ReadOnlySpan<byte> value)
    {
        int end = value.IndexOf((byte)0); if (end >= 0) value = value[..end];
        if ((uint)offset >= bytes.Length || value.Length >= bytes.Length - offset) throw new ArgumentOutOfRangeException(nameof(offset), "미션 패킷 문자열 쓰기 물리 범위");
        value.CopyTo(bytes[offset..]); bytes[offset + value.Length] = 0;
    }
}
