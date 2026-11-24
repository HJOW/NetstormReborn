using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Net;

// 0x20 참가자 생성 본체의 같은 원문 필드·이름·장부·상태·이미지·객체·이름표·입장 순서를 복원한다.
// cpppj/src/client/NativeMetaParticipantCreate.cpp 의 ConstructNativeMetaParticipant 를 이식한다.
// 생성 호출자(0x20 수신 결선)는 후속에서 잇는다.
// 0.961에서 R4-6 메타 참가자 묶음용으로 추가됐다.
namespace Netstorm.Client;

// 0x20 참가자 생성의 실제 표시 장치 묶음이다.
public sealed class ClientMetaParticipantConstructDisplayHooks
{
    /// <summary>원문 진단의 실제 선택 줄 소비자다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>현재 종류 WORD 장치다.</summary>
    public Action<uint, uint>? SetKind { get; set; }

    /// <summary>생성 객체의 현재 물리다.</summary>
    public Func<uint, Memory<byte>>? Raw { get; set; }

    /// <summary>생성 객체의 실제 소유자 표시 장치다.</summary>
    public Action<uint, uint>? PaintOwner { get; set; }

    /// <summary>생성 객체의 실제 이동 장치다.</summary>
    public Action<uint, float, float, uint>? Move { get; set; }

    /// <summary>이름표의 실제 문구 장치다.</summary>
    public Action<uint, string, uint>? ReplaceText { get; set; }

    /// <summary>이름표의 실제 일련 장치다.</summary>
    public Func<uint, uint>? Gump { get; set; }

    /// <summary>이름표의 실제 부착 장치다.</summary>
    public Action<uint, int, int, uint, uint>? Attach { get; set; }

    /// <summary>실제 이름표 공장 장치다.</summary>
    public Func<uint>? CreateName { get; set; }

    /// <summary>이름표의 실제 선택 설정 장치다.</summary>
    public Action<uint, uint>? Selectable { get; set; }

    /// <summary>이름표의 실제 스크롤 설정 장치다.</summary>
    public Action<uint, uint>? Scrollable { get; set; }

    /// <summary>이름표의 실제 객체 연결 장치다.</summary>
    public Action<uint, uint>? Reference { get; set; }

    /// <summary>현재 루트 토큰 장치다.</summary>
    public Func<uint>? Root { get; set; }
}

// 0x20 참가자 생성이 선택한 효과의 실제 외부 장치다.
public sealed class ClientMetaParticipantConstructHooks
{
    /// <summary>같은 실제 게임 보조 owner다. SubscriberMarker를 직접 읽고 쓴다.</summary>
    public RawGameAuxState? GameAux { get; set; }

    /// <summary>같은 실제 자기 구독자 번호 owner다.</summary>
    public UIntBox? Subscriber { get; set; }

    /// <summary>같은 실제 표시 장치 묶음이다.</summary>
    public ClientMetaParticipantConstructDisplayHooks? Display { get; set; }

    /// <summary>같은 실제 이름표 서식 장치 묶음이다.</summary>
    public ClientMetaParticipantNameHooks? Name { get; set; }

    /// <summary>미초기화 80BYTE와 현재 변경 여부를 묻는다.</summary>
    public Func<uint, bool>? GuildChanged { get; set; }

    /// <summary>별도 80BYTE 확인 뒤 실제 목적에 길드 값을 쓴다.</summary>
    public Action<uint, Memory<byte>>? Guild { get; set; }

    /// <summary>같은 전투 8자리 구독자 장부를 갱신한다.</summary>
    public Action? RefreshCore { get; set; }

    /// <summary>제거·표시 MessageBoard 장치다.</summary>
    public Action<int, uint, bool>? Status { get; set; }

    /// <summary>현재 identity로 실제 그림을 만든 뒤 같은 CS 기록에 변환한다.</summary>
    public Action<Memory<byte>, byte[]>? Image { get; set; }

    /// <summary>현재 참가자 그림 TYPE을 읽는다.</summary>
    public Func<uint>? ParticipantType { get; set; }

    /// <summary>같은 실제 객체 공장이다.</summary>
    public Func<uint, uint, uint>? Create { get; set; }

    /// <summary>실제 참가자 Rig 입장 장치다.</summary>
    public Action<uint, bool>? Enter { get; set; }

    /// <summary>실제 인트로 카메라 장치다.</summary>
    public Action? Intro { get; set; }

    /// <summary>현재 인트로 숨김 설정을 읽는다.</summary>
    public Func<bool>? NoIntro { get; set; }

    /// <summary>선택된 실제 Tell 장치다.</summary>
    public Action<string>? Tell { get; set; }

    /// <summary>현재 이름 주소의 실제 감시 장치다.</summary>
    public Action<ReadOnlyMemory<byte>>? Monitor { get; set; }
}

// 42b580 참가자 생성의 전체 필드·이름·장부·상태·실제 이미지·객체·이름표·입장·모니터 순서다.
public static class ClientMetaParticipantConstruct
{
    /// <summary>길드 60BYTE 목적지의 실제 폭이다. 0.961에서 추가됐다.</summary>
    private const int GuildBytes = 60;

    /// <summary>식별자 6유효 BYTE의 실제 폭이다. 0.961에서 추가됐다.</summary>
    private const int IdentityBytes = 6;

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.961에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : class => value ?? throw new InvalidOperationException("메타 참가자 생성의 실제 장치 미연결");

    /// <summary>선택된 실제 값 대리자만 요구하며 가짜 성공으로 대신하지 않는다. 0.961에서 추가됐다.</summary>
    private static T NeedDelegate<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("메타 참가자 생성의 실제 장치 미연결");

    /// <summary>원문 비정렬 BYTE 폭을 낮은 자리부터 조립한다. 0.961에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "메타 참가자 생성의 실제 읽기 범위");
        }

        uint value = 0;
        // 낮은 BYTE부터 지정 폭을 조립한다.
        for (int index = 0; index < width; index++)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>같은 기록의 지정 폭만 쓰고 미작성 필드는 보존한다. 0.961에서 추가됐다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "메타 참가자 생성의 실제 쓰기 범위");
        }

        // 도달한 폭만 기록하고 선행 효과를 되돌리지 않는다.
        for (int index = 0; index < width; index++)
        {
            bytes[offset + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>고정 필드명과 무관하게 원문처럼 첫 NUL까지만 읽는다. 0.961에서 추가됐다.</summary>
    private static string Text(ReadOnlySpan<byte> bytes)
    {
        int end = bytes.IndexOf((byte)0);
        if (end < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "메타 참가자 생성의 실제 문자열 NUL 부족");
        }

        return Encoding.Latin1.GetString(bytes.Slice(0, end));
    }

    /// <summary>prefix·이름 포인터와 후행 표시까지 원문 순서로 수행한다. 0.961에서 추가됐다.</summary>
    /// <param name="record">같은 실제 참가자 기록이다.</param>
    /// <param name="packet">원문 0x20 패킷이다.</param>
    /// <param name="hooks">실제 보조·구독자·표시·이름표·길드·장부·상태·그림·객체·입장·감시 장치다.</param>
    public static void Construct(Memory<byte> record, ReadOnlyMemory<byte> packet, ClientMetaParticipantConstructHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        Span<byte> current = record.Span;
        ReadOnlySpan<byte> source = packet.Span;
        Put(current, 0, Read(source, 4, 1));
        Put(current, 4, Read(source, 5));
        Put(current, 0x4e, Read(source, 0x13));
        Put(current, 0x52, Read(source, 0x17, 2), 2);
        Put(current, 0x54, Read(source, 0x19));
        Put(current, 0x58, Read(source, 0x1d));
        Put(current, 0x5c, Read(source, 0x21, 2), 2);
        Put(current, 0x64, Read(source, 0x23) & 0xffffffbfu);
        Put(current, 0x9c, Read(source, 0x27));
        Put(current, 0xa0, Read(source, 0x2b));
        Put(current, 0xa4, Read(source, 0x2f, 1));
        Put(current, 0xa8, Read(source, 0x30, 1));
        Put(current, 0x60, Read(source, 0x31));
        byte[] guild = new byte[GuildBytes];
        guild[0] = 0;
        // strcpy의 실제 BYTE 부분 효과/NUL을 보존한다.
        for (int index = 0; ; index++)
        {
            uint value = Read(source, 0x35 + index, 1);
            if (index >= record.Length - 0x26)
            {
                throw new ArgumentOutOfRangeException(nameof(packet), "메타 참가자 생성의 실제 이름 복사 범위 초과");
            }

            record.Span[0x26 + index] = (byte)value;
            if (value == 0)
            {
                break;
            }
        }
        // strcpy의 실제 BYTE 부분 효과/NUL을 보존한다.
        for (int index = 0; ; index++)
        {
            uint value = Read(source, 0x35 + index, 1);
            if (index >= guild.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(packet), "메타 참가자 생성의 실제 길드 60BYTE 초과");
            }

            guild[index] = (byte)value;
            if (value == 0)
            {
                break;
            }
        }

        RawGameAuxState gameAux = Need(hooks.GameAux);
        uint subscriber = Need(hooks.Subscriber).Value;
        if (NeedDelegate(hooks.GuildChanged)(Read(current, 4)))
        {
            NeedDelegate(hooks.Guild)(Read(current, 4), guild);
            if (guild[0] != 0)
            {
                int end = Array.IndexOf(guild, (byte)0);
                if (end < 0 || end + 4 > guild.Length)
                {
                    throw new ArgumentOutOfRangeException(nameof(packet), "메타 참가자 생성의 실제 길드 버전 꼬리 부족");
                }

                BinaryPrimitives.WriteUInt32LittleEndian(guild.AsSpan(end, 4), 0x003a797eu);
            }
        }

        RawMissionPacketFields.SanitizePlayerName(Read(source, 5), current.Slice(0x26));
        Put(current, 0xb4, Read(source, 9, 2));
        Put(current, 0xb2, Read(source, 0xb, 2));
        Put(current, 0xb0, 10, 2);
        Put(current, 0xac, Read(source, 0xd, 1));
        NeedDelegate(hooks.RefreshCore)();
        bool local = false;
        if (gameAux.SubscriberMarker != 0 || Read(current, 4) == subscriber)
        {
            NeedDelegate(hooks.Status)(5, Read(current, 0), false);
            NeedDelegate(hooks.Status)(3, Read(current, 0), true);
            local = true;
            gameAux.SubscriberMarker = 1;
        }

        byte[] identity = new byte[IdentityBytes];
        BinaryPrimitives.WriteUInt32LittleEndian(identity.AsSpan(0, 4), Read(current, 0x4e));
        BinaryPrimitives.WriteUInt16LittleEndian(identity.AsSpan(4, 2), unchecked((ushort)Read(current, 0x52, 2)));
        NeedDelegate(hooks.Image)(record.Slice(0xbc, 20), identity);
        ClientMetaParticipantConstructDisplayHooks display = Need(hooks.Display);
        if (Read(current, 0xc0, 1) == 0)
        {
            NeedDelegate(display.Assertion)("image.shape != NULL", ".\\Ns\\O\\Ch.cpp", 0x2ee);
        }

        uint type = NeedDelegate(hooks.ParticipantType)();
        uint created = NeedDelegate(hooks.Create)(type, 2);
        Put(current, 0xb8, created);
        NeedDelegate(display.SetKind)(created, Read(current, 4));
        Memory<byte> raw = NeedDelegate(display.Raw)(created);
        if ((raw.Span[0xb] & 4u) == 0)
        {
            NeedDelegate(display.Assertion)("isVoid()", "c:\\ns\\o\\squid.h", 0x282);
        }

        raw.Span[40] &= 0xfe;
        NeedDelegate(display.PaintOwner)(created, Read(current, 0));
        BinaryPrimitives.WriteUInt16LittleEndian(raw.Span.Slice(8, 2), 0);
        raw.Span[0x1a] = 0;
        float y = BitConverter.UInt32BitsToSingle(Read(current, 0xa0));
        float x = BitConverter.UInt32BitsToSingle(Read(current, 0x9c));
        NeedDelegate(display.Move)(created, x, y, 0);
        string prefix = "~a" + Text(guild) + ':';
        ClientMetaParticipantName.Write(record, prefix, Need(hooks.Name));
        uint label = NeedDelegate(display.CreateName)();
        NeedDelegate(display.Selectable)(label, 1);
        NeedDelegate(display.ReplaceText)(label, string.Empty, 0);
        NeedDelegate(display.Scrollable)(label, 0);
        NeedDelegate(display.Reference)(label, created);
        BinaryPrimitives.WriteUInt32LittleEndian(current.Slice(0xd0, 4), NeedDelegate(display.Gump)(label));
        BinaryPrimitives.WriteUInt32LittleEndian(current.Slice(0xd4, 4), label);
        NeedDelegate(display.Attach)(label, 0, 0, NeedDelegate(display.Root)(), 1);
        if ((Read(source, 0x23, 1) & 0x40u) == 0)
        {
            NeedDelegate(hooks.Enter)(Read(current, 0xb8), Read(current, 4) == subscriber);
        }

        if (Read(current, 4) == subscriber)
        {
            NeedDelegate(hooks.Intro)();
            if (!NeedDelegate(hooks.NoIntro)())
            {
                NeedDelegate(hooks.Tell)("ChalIntro(1)");
            }
        }

        if (local)
        {
            NeedDelegate(hooks.Monitor)(record.Slice(0x26));
        }
    }
}
