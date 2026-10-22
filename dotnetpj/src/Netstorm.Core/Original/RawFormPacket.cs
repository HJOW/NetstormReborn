namespace Netstorm.Core.Original;

/// <summary>
/// 원본 등록 표에서 writer 가 호출마다 읽는 패킷 헤더. 길이는 반환 크기와 별개로 기록된다.
/// </summary>
/// <param name="Message">메시지 BYTE(패킷 +1).</param>
/// <param name="Protocol">프로토콜 BYTE(패킷 +0). 기록할 때 bit7 은 지운다.</param>
/// <param name="Length">원본 헤더의 선언 길이. 4 보다 작으면 4 로 기록한다.</param>
public readonly record struct FormPacketHeader(byte Message, byte Protocol, ushort Length);

/// <summary>
/// 세 Form writer 가 쓰는 현재 헤더 값. 기본값은 두 PE 의 초기 메시지·프로토콜·길이와 같다.
/// </summary>
public sealed class FormPacketHeaders
{
    /// <summary>감시 Form(+0x54)의 헤더. SID/부모/상태/상대 Form 두 개를 담는 13바이트다.</summary>
    public FormPacketHeader Dependency { get; set; } = new(0x65, 1, 13);

    /// <summary>내용물 Form(+0x54)의 헤더. WORD 필드와 판본별 추가 WORD/순번 BYTE 를 담는 18바이트다.</summary>
    public FormPacketHeader Content { get; set; } = new(0x2d, 1, 18);

    /// <summary>공통 삭제(+0x58)의 헤더. SID/플래그/최근 표식을 담는 8바이트다.</summary>
    public FormPacketHeader Deletion { get; set; } = new(0x2e, 1, 8);
}

/// <summary>
/// 삭제 writer 가 한 번 소비하는 원본 전역(005c845c/60 ↔ CD 005395d4/d8).
/// SID 가 일치하면 SID 만 0 으로 지우고 token 의 낮은 WORD 를 플래그에 더한다.
/// </summary>
public sealed class FormDeletionStamp
{
    /// <summary>최근 삭제 요청의 SID. 소비 뒤 0 이 된다.</summary>
    public int Sid { get; set; }

    /// <summary>최근 삭제 요청의 표식. 소비해도 값 자체는 유지된다.</summary>
    public uint Token { get; set; }
}

/// <summary>
/// 원본 Form 가상 +0x54(데이터)와 공통 삭제 가상 +0x58(삭제)의 실제 패킷을 같은 raw 슬롯에서 작성한다
/// (원본 004ae560·004aeb40·004ac340 ↔ CD 004af750·004aff30·004abc10).
/// 게임 상태의 복사본을 만들지 않고 현재 슬롯·헤더·삭제 표식을 직접 읽는다. 슬롯은 바꾸지 않는다.
/// 기준 판본은 10.78 뿐이다(CD 는 내용물 필드 위치가 달라서 옮기지 않았다).
/// 근거: cpppj/src/o/RawFormPacket.cpp (W03 삭제·수명 열두째 조각, 2026-10-22).
/// </summary>
public sealed class RawFormPacket
{
    /// <summary>감시 Form(DependForm) 생성자의 10.78 가상 표.</summary>
    private const uint DependencyVtable = 0x512110;

    /// <summary>내용물 Form(ContentForm) 생성자의 10.78 가상 표.</summary>
    private const uint ContentVtable = 0x512208;

    /// <summary>Form 슬롯 안 부모 번호(16비트)의 위치.</summary>
    private const int ParentOffset = 16;

    /// <summary>감시 Form 의 상대 Form 번호(16비트) 첫 위치.</summary>
    private const int WatcherOffset = 18;

    /// <summary>감시 Form 의 상대 Form 번호(16비트) 둘째 위치.</summary>
    private const int WatchedOffset = 20;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>일반 client 번호의 첫 번호.</summary>
    private const int FirstOrdinary = 5;

    /// <summary>Form 슬롯을 읽는 풀. 이 객체보다 오래 살아야 한다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>호출마다 현재 값을 읽는 헤더 묶음.</summary>
    private readonly FormPacketHeaders _headers;

    /// <summary>다른 삭제 요청과 공유하는 한 번 소비 표식.</summary>
    private readonly FormDeletionStamp _stamp;

    /// <summary>
    /// 패킷 writer 를 만든다. 풀·헤더·표식은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">Form 슬롯을 가진 풀.</param>
    /// <param name="headers">현재 헤더 값.</param>
    /// <param name="stamp">삭제 표식.</param>
    public RawFormPacket(RawSidPool pool, FormPacketHeaders headers, FormDeletionStamp stamp)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(headers);
        ArgumentNullException.ThrowIfNull(stamp);
        _pool = pool;
        _headers = headers;
        _stamp = stamp;
    }

    /// <summary>정렬되지 않은 WORD/DWORD 를 낮은 바이트부터 읽는다.</summary>
    /// <param name="sid">읽을 슬롯 번호.</param>
    /// <param name="offset">슬롯 안 시작 위치.</param>
    /// <param name="width">읽을 바이트 수.</param>
    private uint Read(int sid, int offset, int width)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        uint value = 0;
        // 필드 폭만 읽고 인접 payload 는 섞지 않는다
        for (int i = 0; i < width; i++)
        {
            value |= (uint)raw[offset + i] << (8 * i);
        }
        return value;
    }

    /// <summary>
    /// 패킷 버퍼의 지정 폭만 리틀 엔디언으로 덮어쓴다. 반환 크기 뒤의 옛 버퍼는 보존한다.
    /// </summary>
    /// <param name="packet">쓸 버퍼.</param>
    /// <param name="offset">시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓸 바이트 수.</param>
    private static void Put(Span<byte> packet, int offset, uint value, int width)
    {
        // 낮은 바이트부터 쓴다
        for (int i = 0; i < width; i++)
        {
            packet[offset + i] = (byte)(value >> (8 * i));
        }
    }

    /// <summary>
    /// 원본 erase0 경로: 프로토콜의 bit7 을 지우고 선언 길이를 최소 4 로 올려 쓴다. 버퍼가 짧으면 아무것도 쓰지 않는다.
    /// </summary>
    /// <param name="packet">대상 버퍼.</param>
    /// <param name="header">쓸 헤더.</param>
    /// <param name="size">이 writer 의 반환 크기.</param>
    private static void Header(Span<byte> packet, FormPacketHeader header, int size)
    {
        if (packet.Length < size)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), packet.Length, "Form 패킷 버퍼 부족");
        }
        packet[0] = (byte)(header.Protocol & 0x7f);
        packet[1] = header.Message;
        Put(packet, 2, Math.Max((ushort)4, header.Length), 2);
    }

    /// <summary>
    /// 원본은 0~4 번을 이 검사에서 client SID 로 분류하지 않는다. 일반 client 번호만 거부한다.
    /// </summary>
    /// <param name="sid">검사할 번호.</param>
    private void ServerSid(int sid)
    {
        if (sid >= FirstOrdinary && sid < RawSidPool.ServerFirst)
        {
            throw new InvalidOperationException("Form 패킷의 client SID 원본 assert");
        }
    }

    /// <summary>
    /// ProcessForm 은 타입 BYTE 가 프로세스 번호여도 같은 생성자 가상 표의 +0 getter 를 쓴다.
    /// 현재 가상 표가 네 Form 생성자 중 하나가 아니면 거부하고, free/void 상태 필터는 더하지 않는다.
    /// </summary>
    /// <param name="form">부모를 읽을 Form 번호.</param>
    private ushort Parent(int form)
    {
        uint table = Read(form, 0, 4);
        if (table != DependencyVtable && table != ContentVtable && table != 0x512098 && table != 0x512188)
        {
            throw new InvalidOperationException("Form 부모 가상 getter 미복원");
        }
        return (ushort)Read(form, ParentOffset, 2);
    }

    /// <summary>
    /// 현재 Form 가상 표로 감시(13바이트) 또는 내용물(18바이트) writer 를 골라 쓴다.
    /// 두 writer 모두 부모의 client 번호를 먼저 거부하며, 감시 writer 는 서버 권한과 상대 Form 의 server 번호를 추가로 요구한다.
    /// </summary>
    /// <param name="form">패킷을 만들 Form 번호.</param>
    /// <param name="packet">쓸 버퍼. 반환 크기보다 길면 뒤는 보존된다.</param>
    /// <returns>13 또는 18.</returns>
    public ushort WriteData(int form, Span<byte> packet)
    {
        uint table = Read(form, 0, 4);
        bool dependency = table == DependencyVtable;
        if (!dependency && table != ContentVtable)
        {
            throw new InvalidOperationException("다른 Form 가상 +0x54 writer 미복원");
        }
        int parent = Parent(form);
        ServerSid(parent);
        if (dependency)
        {
            if (!_pool.IsServer)
            {
                throw new InvalidOperationException("감시 Form writer 의 원본 server assert");
            }
            // 상대 Form 과 그 부모도 server 번호여야 하며, 현재 0 인 상대는 getter 를 부르지 않는다
            foreach (int offset in new[] { WatcherOffset, WatchedOffset })
            {
                int other = (int)(ushort)Read(form, offset, 2);
                if (other != 0)
                {
                    ServerSid(other);
                    ServerSid(Parent(other));
                }
            }
            Header(packet, _headers.Dependency, 13);
            Put(packet, 4, (uint)form, 2);
            Put(packet, 6, (uint)parent, 2);
            packet[8] = _pool.Slot(form)[StateOffset];
            Put(packet, 9, Read(form, WatcherOffset, 2), 2);
            Put(packet, 11, Read(form, WatchedOffset, 2), 2);
            return 13;
        }
        Header(packet, _headers.Content, 18);
        Put(packet, 4, (uint)form, 2);
        Put(packet, 6, Read(form, 12, 2), 2);
        Put(packet, 8, Read(form, WatcherOffset, 2), 2);
        Put(packet, 10, Read(form, WatchedOffset, 2), 2);
        Put(packet, 12, Read(form, 24, 2), 2);
        Put(packet, 14, Read(form, ParentOffset, 2), 2);
        packet[16] = _pool.Slot(form)[23];
        packet[17] = _pool.Slot(form)[StateOffset];
        return 18;
    }

    /// <summary>
    /// 공통 삭제 writer: SID 를 먼저 검사하고, 최근 표식이 같은 SID 면 그 SID 만 소비하고 token 의 낮은 WORD 를
    /// flags 상위 WORD 의 0xfff0 비트와 합쳐 쓴다.
    /// </summary>
    /// <param name="sid">삭제할 번호.</param>
    /// <param name="flags">삭제 플래그. 상위 WORD 만 쓴다.</param>
    /// <param name="packet">쓸 버퍼. 반환 크기보다 길면 뒤는 보존된다.</param>
    /// <returns>8.</returns>
    public ushort WriteDeletion(int sid, uint flags, Span<byte> packet)
    {
        ServerSid(sid);
        Header(packet, _headers.Deletion, 8);
        Put(packet, 4, (uint)sid, 2);
        ushort token = 0;
        if (_stamp.Sid == sid)
        {
            token = (ushort)_stamp.Token;
            _stamp.Sid = 0;
        }
        Put(packet, 6, ((flags >> 16) & 0xfff0u) | token, 2);
        return 8;
    }
}
