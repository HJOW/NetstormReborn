namespace Netstorm.Core.Original;

/// <summary>
/// 원본 동적 stack 예약은 wire보다 3바이트 크며 그 tail도 0xff로 보존한다.
/// 길이 확정 전이나 변조된 범위 밖 선언은 <see cref="Bytes"/>가 거부한다.
/// </summary>
/// <remarks>2026-11-02 0.134에서 추가했다. cpppj <c>o/RawMissionLoginPackets</c>의 <c>MissionFortPacket</c>을 옮겼다.</remarks>
public sealed class MissionFortPacket
{
    /// <summary>이미지 크기+20바이트의 실제 예약이다. 최종 wire는 이미지 크기+17이다.</summary>
    public byte[] Storage { get; set; } = [];

    /// <summary>현재 길이 WORD만큼만 빌린다. 길이 확정 전이나 변조된 범위 밖 선언은 거부한다.</summary>
    /// <exception cref="ArgumentOutOfRangeException">헤더 4바이트가 없거나 선언 길이가 4 미만이거나 예약 크기를 넘는다.</exception>
    public ReadOnlySpan<byte> Bytes()
    {
        if (Storage.Length < 4)
        {
            throw new ArgumentOutOfRangeException(null, "sendFort packet 헤더 물리 범위 오류");
        }
        int length = Storage[2] | (Storage[3] << 8);
        if (length < 4 || length > Storage.Length)
        {
            throw new ArgumentOutOfRangeException(null, "sendFort 선언 길이 물리 범위 오류");
        }
        return Storage.AsSpan(0, length);
    }
}

/// <summary>
/// Template 선택은 같은 현재 레코드를 각 원문 위치에서 읽는다. VFS/파일 로더를 이 모듈이 대신하지 않는다.
/// </summary>
/// <remarks>2026-11-02 0.134에서 추가했다. <c>with</c> 식으로 복사해 일부 항목만 바꿀 수 있어 cpppj의 값 전달과 같다.</remarks>
public sealed record MissionFortPacketReads
{
    /// <summary>실제 9 Template의 현재 manager다. null은 미설치다.</summary>
    public Func<uint, FortTemplate?>? Current { get; init; }

    /// <summary>004df590 직전에 읽는 00544c48의 현재 WORD다.</summary>
    public Func<ushort>? InitialLength { get; init; }
}

/// <summary>실제 wire 송신을 받는다. <see cref="ReadOnlySpan{T}"/>를 받으므로 전용 대리자를 쓴다.</summary>
/// <param name="target">수신 대상(signed).</param>
/// <param name="bytes">길이 확정 뒤의 최종 wire 바이트.</param>
/// <remarks>2026-11-02 0.134에서 추가했다.</remarks>
public delegate void MissionFortPacketSend(int target, ReadOnlySpan<byte> bytes);

/// <summary>실제 선택된 로그·송신·누락 manager 진단만 외부 경계다.</summary>
/// <remarks>2026-11-02 0.134에서 추가했다. <c>with</c> 식으로 복사해 일부 항목만 바꿀 수 있어 cpppj의 값 전달과 같다.</remarks>
public sealed record MissionFortPacketHooks
{
    /// <summary>004a68e0("sendFort from %d to %d\n", owner, target)이다.</summary>
    public Action<uint, int>? Log { get; init; }

    /// <summary>현재 연결 gate를 통과할 실제 최종 wire 송신이다.</summary>
    public MissionFortPacketSend? Send { get; init; }

    /// <summary>004bf520의 manager 누락 진단이다. 반환한 뒤 실제 현재 manager를 다시 읽는다.</summary>
    public Action? MissingTemplate { get; init; }
}

/// <summary>
/// 현재 Template SaveImage와 로그인 sendFort의 실제 0x2a 예약·필드·길이 확정을 연결한다.
/// 원본 동적 stack 예약(이미지+20바이트)과 wire 선언(이미지+17바이트)을 구분하고, 현재 선택을 원문 위치마다 다시 읽는다.
/// cpppj <c>o/RawMissionLoginPackets</c>를 옮겼다.
/// </summary>
/// <remarks>2026-11-02 0.134에서 추가했다. 원본 x86 fixture <c>fort-image-packets</c>의 송신(S) 93행·예약(F) 10행이 근거다.</remarks>
public sealed class RawMissionLoginPackets
{
    /// <summary>Player 한 명의 실제 폭(0xb4)이다.</summary>
    private const int Stride = 0xb4;

    /// <summary>Player 수(9)다.</summary>
    private const int Owners = 9;

    /// <summary>패킷 안 이미지가 시작하는 위치(0x11)다. wire 선언 길이는 이미지 크기에 이 값을 더한다.</summary>
    private const int ImageOffset = 0x11;

    /// <summary>예약이 이미지 크기에 더하는 실제 폭(0x14)이다. 선언 길이보다 3바이트 크다.</summary>
    private const int ReservationOverhead = 0x14;

    /// <summary>패킷 종류 바이트(0x2a)다.</summary>
    private const byte PacketKind = 0x2a;

    /// <summary>같은 9×0xb4 Player의 현재 +58 바이트를 읽으며 watcher 사본을 두지 않는다.</summary>
    private readonly ReadOnlyMemory<byte> _players;

    /// <summary>원문 위치별 현재 Template와 길이 조회다.</summary>
    private readonly MissionFortPacketReads _reads;

    /// <summary>실제 Save의 현재 타입 개수와 선택 진단이다.</summary>
    private readonly FortImageHooks _image;

    /// <summary>현재 로그와 실제 송신, 누락 manager 진단 경계다.</summary>
    private readonly MissionFortPacketHooks _hooks;

    /// <summary>
    /// 같은 9 Player와 현재 Template·타입 개수·송신 수명은 이 객체보다 오래 살아야 한다. 현재 조회 없이 값형 기본 Template를 끼워 넣지 않는다.
    /// </summary>
    /// <param name="edition">원본 판본. 10.78만 허용한다.</param>
    /// <param name="players">9명의 Player 바이트(1620).</param>
    /// <param name="reads">현재 Template와 길이 조회.</param>
    /// <param name="image">이미지 직렬화 장치(타입 개수 필수).</param>
    /// <param name="hooks">로그·송신·누락 진단.</param>
    /// <exception cref="ArgumentException">10.78이 아니거나 Player 크기가 1620이 아니거나 필수 조회가 없다.</exception>
    public RawMissionLoginPackets(OriginalEdition edition, ReadOnlyMemory<byte> players, MissionFortPacketReads reads, FortImageHooks image, MissionFortPacketHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(reads);
        ArgumentNullException.ThrowIfNull(image);
        ArgumentNullException.ThrowIfNull(hooks);
        _players = players;
        _reads = reads;
        _image = image;
        _hooks = hooks;
        if (edition != OriginalEdition.Patch1078 || players.Length != Stride * Owners || _reads.Current is null || _reads.InitialLength is null || _image.TypeCount is null)
        {
            throw new ArgumentException("sendFort의 PATCH 현재 Player/Template/길이/타입 조회 누락");
        }
    }

    /// <summary>
    /// 004df590/004a7240의 실제 예약을 작성한다: 전체를 0xff로 채운 뒤 헤더와 현재 필드만 쓰며 끝 3바이트는 손대지 않는다.
    /// 길이 observer(<see cref="FinalizeMissionFortPacket"/>)는 별도 원문 단계로 수행한다.
    /// </summary>
    /// <param name="owner">소유 Player 번호.</param>
    /// <param name="predictable">예측 값 DWORD.</param>
    /// <param name="watcher">대상 Player의 현재 +58 바이트.</param>
    /// <param name="image">직렬화된 이미지(논리 끝까지).</param>
    /// <param name="initialLength">00544c48의 현재 WORD. 4 미만이면 4로 올려 쓴다.</param>
    /// <exception cref="ArgumentOutOfRangeException">이미지가 실제 할당(0x8000)을 넘는다.</exception>
    public static MissionFortPacket MakeMissionFortPacket(uint owner, uint predictable, byte watcher, ReadOnlySpan<byte> image, ushort initialLength)
    {
        if (image.Length > RawFortImage.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(image), image.Length, "sendFort 실제 이미지 할당 범위 초과");
        }
        byte[] storage = new byte[image.Length + ReservationOverhead];
        Array.Fill(storage, (byte)0xff);
        storage[0] = 1;
        storage[1] = PacketKind;
        Write(storage, 2, Math.Max((uint)initialLength, 4u), 2);
        Write(storage, 4, owner);
        Write(storage, 8, predictable);
        Write(storage, 12, (uint)image.Length);
        storage[16] = watcher;
        image.CopyTo(storage.AsSpan(ImageOffset));
        return new MissionFortPacket { Storage = storage };
    }

    /// <summary>
    /// 0040a540의 실제 bool 인자·0xffffffff 진단·하위 WORD 감김을 수행한다. observer가 false면 아무것도 읽지 않는다.
    /// 정상 true에서는 이미지 크기+17을 하위 16비트로 감아 길이 WORD에 쓴다. 물리 packet은 17바이트 이상이어야 한다.
    /// </summary>
    /// <param name="packet">예약 전체.</param>
    /// <param name="writeLength">길이를 확정할지 나타내는 observer 인자.</param>
    /// <param name="assertion">크기가 0xffffffff일 때의 원본 진단. 필요한 경우에만 요구한다.</param>
    /// <exception cref="ArgumentOutOfRangeException">길이 확정에 필요한 17바이트가 없다.</exception>
    /// <exception cref="InvalidOperationException">크기가 0xffffffff인데 진단 장치가 없다.</exception>
    public static void FinalizeMissionFortPacket(Span<byte> packet, bool writeLength, Action? assertion = null)
    {
        if (!writeLength)
        {
            return;
        }
        if (packet.Length < ImageOffset)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), packet.Length, "sendFort 길이 observer 물리 범위 오류");
        }
        uint size = ReadField(packet, 12, 4);
        if (size == 0xffffffffu)
        {
            if (assertion is null)
            {
                throw new InvalidOperationException("sendFort 실제 크기 진단 미연결");
            }
            assertion();
        }
        Write(packet, 2, unchecked(size + (uint)ImageOffset), 2);
    }

    /// <summary>
    /// 004a7240: Template가 없으면 자연스럽게 false다. 그 밖에는 현재 이미지 작성 → 현재 watcher → 실제 길이 확정/송신 순으로 진행한다.
    /// 로그 뒤와 Save 직전에 현재 Template를 원문 위치마다 다시 읽고, 원본 wrapper는 manager 존재 검사와 실제 Save 대상 조회를 합치지 않는다.
    /// </summary>
    /// <param name="owner">소유 Player 번호(0~8).</param>
    /// <param name="target">수신 대상(signed).</param>
    /// <param name="predictable">예측 값 DWORD.</param>
    /// <returns>현재 Template가 있어 송신했으면 true.</returns>
    /// <exception cref="ArgumentOutOfRangeException">owner가 범위 밖이다.</exception>
    /// <exception cref="InvalidOperationException">선택된 실제 소비자가 연결되지 않았거나 Save 대상 manager가 사라졌다.</exception>
    public bool SendFort(uint owner, int target, uint predictable)
    {
        if (owner >= Owners)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), owner, "sendFort 현재 Template owner 범위 오류");
        }
        if (_reads.Current!(owner) is null)
        {
            return false;
        }
        Need(_hooks.Log)(owner, target);
        if (_reads.Current(owner) is null)
        {
            Need(_hooks.MissingTemplate)();
        }
        FortTemplate current = _reads.Current(owner) ?? throw new InvalidOperationException("sendFort Save의 현재 Template manager NULL");
        FortImageData image = new RawFortImage(OriginalEdition.Patch1078, current, _image).Save();
        ushort initialLength = _reads.InitialLength!();
        // 원본은 길이 WORD 조회/헤더 작성 뒤 현재 watcher를 읽는다
        byte watcher = _players.Span[(int)owner * Stride + 0x58];
        MissionFortPacket packet = MakeMissionFortPacket(owner, predictable, watcher, image.Bytes(), initialLength);
        FinalizeMissionFortPacket(packet.Storage, true);
        Need(_hooks.Send)(target, packet.Bytes());
        return true;
    }

    /// <summary>실제 선택 소비자 누락은 해당 위치에서 거부한다.</summary>
    /// <typeparam name="T">장치 대리자 형태.</typeparam>
    /// <param name="device">요구할 장치.</param>
    /// <exception cref="InvalidOperationException">장치가 연결되지 않았다.</exception>
    private static T Need<T>(T? device) where T : class
    {
        return device ?? throw new InvalidOperationException("sendFort 실제 선택 소비자 미연결");
    }

    /// <summary>비정렬 DWORD/WORD를 원본 little endian 필드에 쓴다. 지정 폭의 하위 바이트만 원문 순서로 기록한다.</summary>
    /// <param name="packet">대상 예약.</param>
    /// <param name="offset">시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="size">필드 폭(2 또는 4).</param>
    private static void Write(Span<byte> packet, int offset, uint value, int size = 4)
    {
        // 지정 폭의 하위 바이트만 원문 순서로 기록한다
        for (int index = 0; index < size; index++)
        {
            packet[offset + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>실제 packet 필드의 원본 폭을 현재 배열에서 읽는다(정렬 가정 없이 낮은 바이트부터 조립).</summary>
    /// <param name="packet">대상 예약.</param>
    /// <param name="offset">시작 위치.</param>
    /// <param name="size">필드 폭.</param>
    private static uint ReadField(ReadOnlySpan<byte> packet, int offset, int size)
    {
        uint value = 0;
        // 정렬 가정 없이 낮은 바이트부터 조립한다
        for (int index = 0; index < size; index++)
        {
            value |= (uint)packet[offset + index] << (8 * index);
        }
        return value;
    }
}
