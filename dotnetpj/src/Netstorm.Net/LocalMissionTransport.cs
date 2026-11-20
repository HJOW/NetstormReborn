namespace Netstorm.Net;

/// <summary>
/// 단독 미션의 실제 두 연결과 Zone·로그인 바이트를 같은 패킷 분배기에 연결한다
/// (cpppj <c>src/zacket/LocalMissionTransport.h</c>의 `LocalMissionPacket`·`LocalMissionLoginView`·`LocalMissionTransport` 본체).
/// </summary>
/// <remarks>기존 사전 구현을0.691에서 실제 고정 노드/공유표/현재ID Poll로 교체한다. 제품Root설치는후속이다.</remarks>
public sealed partial class LocalMissionTransport : IDisposable
{
    /// <summary>원본 예약 전체와 observer가 확정한 선언 길이를 함께 보관한다.</summary>
    public sealed class LocalMissionPacket
    {
        /// <summary>Zone 512·로그인 Fort 이미지+67의 전체 물리 바이트다.</summary>
        public byte[] Storage { get; set; } = [];

        /// <summary>
        /// 실제 송신·State 이미지 복사가 사용하는 현재 선언 WORD 길이만 빌린다.
        /// </summary>
        /// <returns>현재선언길이만큼의같은읽기뷰다.</returns>
        /// <exception cref="ArgumentOutOfRangeException">선언 길이가 예약을 넘는다.</exception>
        public ReadOnlySpan<byte> Bytes()
        {
            if (Storage.Length < 4)
            {
                throw new ArgumentOutOfRangeException(nameof(Storage), "로컬 미션 패킷 헤더 부족");
            }
            uint length = (uint)(Storage[2] | (Storage[3] << 8));
            if (length > (uint)Storage.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(Storage), "로컬 미션 패킷 선언 길이 초과");
            }
            return new ReadOnlySpan<byte>(Storage, 0, (int)length);
        }
        /// <summary>같은실제소유배열의현재선언길이만운반/로그인뷰에빌린다.</summary>
        public ReadOnlyMemory<byte> BytesMemory() => Storage.AsMemory(0, Bytes().Length);
    }

    /// <summary>
    /// 현재 0x27 버퍼의 필드 참조이며 처리기 호출 동안만 유효하다.
    /// </summary>
    /// <param name="Subscriber">+8의 실제 subscriber DWORD다.</param>
    /// <param name="Handle">+17의 현재 NUL 문자열이다.</param>
    /// <param name="FortImage">+17+이름 길이에서 fort 파일 크기만큼의 실제 파일 이미지다.</param>
    public readonly record struct LocalMissionLoginView(uint Subscriber, string Handle, ReadOnlyMemory<byte> FortImage);

    /// <summary>실제 0x27 패킷을 같은 큐에 넣는 송신 경계다.</summary>
    /// <param name="destination">받을 소유자 번호다.</param>
    /// <param name="packet">선언 길이만큼 보낼 패킷이다.</param>
    /// <returns>받아들였으면 참이다.</returns>
    public delegate bool PacketSend(int destination, ReadOnlySpan<byte> packet);

    /// <summary>같은 실제 분배기에 건설·준비·서버 응답 처리기를 더하는 경계다.</summary>
    /// <param name="frame">수신한 한 프레임이다.</param>
    /// <returns>분배했으면 참이다.</returns>
    public delegate bool FrameDispatch(ReadOnlySpan<byte> frame);

}


/// <summary>
/// 단독 미션의 Zone·로그인 바이트 작성과 로그인 뷰 판독을 공유한다
/// (cpppj <c>src/zacket/LocalMissionTransport.cpp</c>의 `MakeLocalMissionZone`·`MakeGuideMissionZone`·`MakeLocalMissionLogin`·`ReadLocalMissionLogin`).
/// </summary>
/// <remarks>기존 사전 구현을0.691에서 같은 설치/분배기와 독립 원본66입력으로 대조한다. 제품Root설치는후속이다.</remarks>
public static class LocalMissionPackets
{
    /// <summary>Zone 예약 바이트 수다.</summary>
    public const int ZoneReserve = 512;

    /// <summary>Zone 한 슬롯의 바이트 수다.</summary>
    public const int ZoneClient = 71;

    /// <summary>Zone 첫 슬롯의 위치다.</summary>
    public const int ZoneHead = 15;

    /// <summary>Zone 패킷 종류 바이트다.</summary>
    public const byte ZoneType = 0x1a;

    /// <summary>로그인 패킷 종류 바이트다.</summary>
    public const byte LoginType = 0x27;

    /// <summary>패킷 버전 바이트다.</summary>
    public const byte PacketVersion = 1;

    /// <summary>004b51e0의 실제 네 정적 이름이다.</summary>
    private static readonly string[] ZoneNames = ["zsimpson", "jgreer", "kdemarest", "tbratton"];

    /// <summary>원본 비정렬 little endian 값을 지정 폭으로 읽는다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 바이트 수다.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        if (offset < 0 || offset > bytes.Length || width < 0 || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "로컬 미션 원문 읽기 범위 오류");
        }
        uint value = 0;
        // 낮은 바이트부터 같은 폭의 원본 비트를 모은다
        for (int index = 0; index < width; ++index)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }
        return value;
    }

    /// <summary>주변 stale 바이트를 유지하며 지정 원본 폭만 쓴다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        if (offset < 0 || offset > bytes.Length || width < 0 || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "로컬 미션 원문 쓰기 범위 오류");
        }
        // 원본 little endian의 바이트 위치만 변경한다
        for (int index = 0; index < width; ++index)
        {
            bytes[offset + index] = unchecked((byte)(value >> (8 * index)));
        }
    }

    /// <summary>원본 C문자열의 첫 NUL까지만 읽는다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    private static string Text(ReadOnlySpan<byte> bytes, int offset)
    {
        if (offset < 0 || offset > bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "로컬 미션 문자열 위치 오류");
        }
        int end = offset;
        // 끝까지 NUL이 없으면 호스트 밖 읽기로 보고 거부한다
        while (end < bytes.Length && bytes[end] != 0)
        {
            ++end;
        }
        if (end >= bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "로컬 미션 원문 문자열 NUL 누락");
        }
        return System.Text.Encoding.Latin1.GetString(bytes[offset..end]);
    }

    /// <summary>원본 strcpy처럼 NUL까지 복사한다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="text">쓸 문자열이다.</param>
    private static void Copy(Span<byte> bytes, int offset, string text)
    {
        int nul = text.IndexOf('\0');
        if (nul >= 0)
        {
            text = text.Substring(0, nul);
        }
        byte[] encoded = System.Text.Encoding.Latin1.GetBytes(text);
        if (offset < 0 || offset >= bytes.Length || encoded.Length >= bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "로컬 미션 strcpy 물리 저장소 부족");
        }
        // 선언 필드 폭을 넘어도 실제 물리 저장소 안에서는 원본 쓰기를 유지한다
        encoded.CopyTo(bytes[offset..]);
        bytes[offset + encoded.Length] = 0;
    }

    /// <summary>004df590의 전체 0xff 예약·종류·버전·선언 길이 헤더로 패킷을 만든다.</summary>
    /// <param name="reserve">예약 바이트 수다.</param>
    /// <param name="type">패킷 종류다.</param>
    private static LocalMissionTransport.LocalMissionPacket Packet(int reserve, byte type)
    {
        var result = new LocalMissionTransport.LocalMissionPacket
        {
            Storage = new byte[reserve],
        };
        Array.Fill(result.Storage, (byte)0xff);
        result.Storage[0] = PacketVersion;
        result.Storage[1] = type;
        Put(result.Storage, 2, 4u, 2);
        return result;
    }

    /// <summary>PATCH 004b51e0의 Zone 512·네 슬롯 작성과 86바이트 길이 확정이다.</summary>
    /// <returns>선언 길이가 확정된 Zone 패킷이다.</returns>
    public static LocalMissionTransport.LocalMissionPacket MakeLocalMissionZone()
    {
        LocalMissionTransport.LocalMissionPacket packet = Packet(ZoneReserve, ZoneType);
        Span<byte> bytes = packet.Storage;
        Put(bytes, 4, 0x82000004u);
        Put(bytes, 8, 0u, 2);
        Put(bytes, 10, 0u);
        bytes[14] = 1;
        // 004b51e0의 네 번 쓰기를 보존한다. State에 복제되는 길이는 마지막에 첫 슬롯 하나로 확정된다
        for (int index = 0; index < ZoneNames.Length; ++index)
        {
            int slot = ZoneHead + index * ZoneClient;
            bytes[slot] = unchecked((byte)index);
            Put(bytes, slot + 1, (uint)(index + 1));
            Put(bytes, slot + 5, 0u);
            Copy(bytes, slot + 9, ZoneNames[index]);
            bytes[slot + 41] = 0;
        }
        Put(bytes, 2, (uint)(ZoneHead + bytes[14] * ZoneClient), 2);
        return packet;
    }

    /// <summary>004b50b0의 같은 Zone 표식과 한 슬롯의 현재 handle 작성이다. 나머지 예약 0xff를 보존한다.</summary>
    /// <param name="handle">쓸 안내 handle이다.</param>
    /// <returns>선언 길이 4를 유지하는 안내 Zone 패킷이다.</returns>
    public static LocalMissionTransport.LocalMissionPacket MakeGuideMissionZone(string handle)
    {
        LocalMissionTransport.LocalMissionPacket packet = Packet(ZoneReserve, ZoneType);
        Span<byte> bytes = packet.Storage;
        Put(bytes, 4, 0x82000004u);
        Put(bytes, 8, 0u, 2);
        Put(bytes, 10, 0u);
        bytes[14] = 1;
        bytes[ZoneHead] = 0;
        Put(bytes, ZoneHead + 1, 1u);
        Put(bytes, ZoneHead + 5, 0u);
        Copy(bytes, ZoneHead + 9, handle);
        bytes[ZoneHead + 41] = 0;
        return packet;
    }

    /// <summary>PATCH 004b63d0의 실제 외부 Fort 이미지 이후 패킷 작성과 0040a450의 길이 확정이다. Template 읽기·파일 선택은 상위 원본 부모의 책임이다.</summary>
    /// <param name="subscriber">쓸 subscriber DWORD다.</param>
    /// <param name="handle">쓸 handle이다.</param>
    /// <param name="fortImage">뒤에 붙일 Fort 파일 이미지다.</param>
    /// <returns>선언 길이가 확정된 로그인 패킷이다.</returns>
    public static LocalMissionTransport.LocalMissionPacket MakeLocalMissionLogin(uint subscriber, string handle, ReadOnlySpan<byte> fortImage)
    {
        int nul = handle.IndexOf('\0');
        if (nul >= 0)
        {
            handle = handle.Substring(0, nul);
        }
        if ((ulong)fortImage.Length > uint.MaxValue - 67ul)
        {
            throw new ArgumentOutOfRangeException(nameof(fortImage), "로컬 로그인 이미지 크기 초과");
        }
        LocalMissionTransport.LocalMissionPacket packet = Packet(fortImage.Length + 67, LoginType);
        Span<byte> bytes = packet.Storage;
        Put(bytes, 8, subscriber);
        byte[] encoded = System.Text.Encoding.Latin1.GetBytes(handle);
        bytes[12] = unchecked((byte)(encoded.Length + 1));
        Copy(bytes, 17, handle);
        Put(bytes, 13, (uint)fortImage.Length);
        int start = 17 + bytes[12];
        if (start < 0 || start > bytes.Length || fortImage.Length > bytes.Length - start)
        {
            throw new ArgumentOutOfRangeException(nameof(fortImage), "로컬 로그인 실제 이미지 복사 범위 초과");
        }
        fortImage.CopyTo(bytes[start..]);
        Put(bytes, 2, (uint)(17 + bytes[12]) + Read(bytes, 13, 2), 2);
        return packet;
    }

    /// <summary>원본 0040a580·0040a4b0의 위치와 실제 물리 버퍼 경계를 함께 확인해 로그인 뷰를 읽는다. 선언 길이를 고쳐 만든 포인터를 재사용하되 손상된 원본의 범위 밖 host 접근은 명시적으로 거부한다.</summary>
    /// <param name="storage">같은 실제 로그인 패킷의 소유 메모리 뷰다.</param>
    /// <returns>처리기 호출 동안만 유효한 필드 뷰이다.</returns>
    public static LocalMissionTransport.LocalMissionLoginView ReadLocalMissionLogin(ReadOnlyMemory<byte> storage)
    {
        ReadOnlySpan<byte> packet = storage.Span;
        if (packet.Length < 17 || packet[1] != LoginType)
        {
            throw new ArgumentException("실제 0x27 로그인 패킷 누락", nameof(storage));
        }
        uint start = 17u + Read(packet, 12, 1);
        uint size = Read(packet, 13);
        if (start > (uint)packet.Length || size > (uint)packet.Length - start)
        {
            throw new ArgumentOutOfRangeException(nameof(storage), "실제 0x27 이미지 물리 범위 부족");
        }
        string name = Text(packet, 17);
        if ((uint)(System.Text.Encoding.Latin1.GetByteCount(name) + 1) > Read(packet, 12, 1))
        {
            throw new ArgumentOutOfRangeException(nameof(storage), "실제 0x27 nameLen의 NUL 범위 부족");
        }
        return new LocalMissionTransport.LocalMissionLoginView(Read(packet, 8), name, storage.Slice((int)start, (int)size));
    }
}
