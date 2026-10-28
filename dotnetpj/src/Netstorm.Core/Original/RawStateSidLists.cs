namespace Netstorm.Core.Original;

/// <summary>
/// State SID 등록 표(원본 00592938의 실제 포인터 20칸)와 종류별 개수 정리를 Session보다 오래 유지한다.
/// Native 헤더 주소는 식별 토큰이며 호스트 포인터로 역참조하지 않는다.
/// 근거: cpppj/src/o/RawStateSidLists.h (R0-4 신규 이식, 2026-10-28).
/// </summary>
public sealed class RawStateSidLists
{
    /// <summary>실제 정적 생성 순서대로 등록하는 Native 헤더 주소 9개.</summary>
    private static readonly uint[] Headers =
    [
        0x55a4d0u, 0x55a4ecu, 0x55a708u, 0x592f80u, 0x5950d8u, 0x5954c0u, 0x59a9d4u, 0x5caf28u, 0x5e1898u,
    ];

    /// <summary>다중선택 목록 헤더. 개수 정리 때 같은 Client 필드를 빌린다.</summary>
    private const uint MultipleHeader = 0x5caf28u;

    /// <summary>STATE SID 등록 표의 실제 칸 수.</summary>
    public const int RegistryCapacity = 20;

    /// <summary>0055a4d0 헤더의 500DWORD 공장 목록이다.</summary>
    public RawPostPopList Factories { get; } = new() { Entries = new uint[500] };

    /// <summary>0055a4ec 헤더의 30DWORD 주민(population) 목록이다.</summary>
    public RawPostPopList Dudes { get; } = new() { Entries = new uint[30] };

    /// <summary>0055a708 헤더의 100DWORD 전초기지 목록이다. 신전 목록과 다르다.</summary>
    public RawPostPopList Outposts { get; } = new() { Entries = new uint[100] };

    /// <summary>00592f80 헤더의 256DWORD 공급원 목록이다.</summary>
    public RawPostPopList Providers { get; } = new() { Entries = new uint[256] };

    /// <summary>005950d8 헤더의 100DWORD 간헐천 목록이다.</summary>
    public RawPostPopList Geysers { get; } = new() { Entries = new uint[100] };

    /// <summary>005954c0 헤더의 16DWORD 사제 목록이다.</summary>
    public RawPostPopList Priests { get; } = new() { Entries = new uint[16] };

    /// <summary>0059a9d4 헤더의 50DWORD 거주 구역 목록이다.</summary>
    public RawPostPopList Residences { get; } = new() { Entries = new uint[50] };

    /// <summary>005e1898 헤더의 20DWORD 신전 목록이다.</summary>
    public RawPostPopList Temples { get; } = new() { Entries = new uint[20] };

    /// <summary>실제 정적 생성 순서 9개와 빈 11칸. 개수 정리에서 보존한다.</summary>
    public uint[] RegisteredHeaders { get; } = new uint[RegistryCapacity];

    /// <summary>
    /// 목록 8개를 만들고 등록 표에 실제 헤더 9개를 정적 생성 순서대로 등록한다. 최초 한 번만 원본 용량을 확보한다.
    /// </summary>
    public RawStateSidLists()
    {
        // 정적 생성자가 원문 순서로 같은 20칸 표에 등록한다
        foreach (uint header in Headers)
        {
            Register(RegisteredHeaders, header);
        }
    }

    /// <summary>
    /// 00472880: 첫 빈 칸에만 기록하고 가득 찬 표에는 쓰지 않는다. 헤더 0도 원문대로 허용한다.
    /// </summary>
    /// <param name="headers">20DWORD 등록 표.</param>
    /// <param name="header">기록할 헤더 토큰.</param>
    public static void Register(Span<uint> headers, uint header)
    {
        RequireRegistry(headers);
        // 첫 빈 칸에서 즉시 반환한다
        for (int index = 0; index < RegistryCapacity; index++)
        {
            if (headers[index] == 0)
            {
                headers[index] = header;
                return;
            }
        }
    }

    /// <summary>
    /// 004728a0: 중복을 포함한 모든 일치 슬롯만 0으로 만든다. 목록 개수·데이터는 건드리지 않는다.
    /// </summary>
    /// <param name="headers">20DWORD 등록 표.</param>
    /// <param name="header">지울 헤더 토큰.</param>
    public static void Unregister(Span<uint> headers, uint header)
    {
        RequireRegistry(headers);
        // 첫 일치에서 끝내지 않는다
        for (int index = 0; index < RegistryCapacity; index++)
        {
            if (headers[index] == header)
            {
                headers[index] = 0;
            }
        }
    }

    /// <summary>
    /// 00472830: 매 슬롯의 현재 포인터를 읽고 0이 아닌 헤더의 개수만 0으로 쓴다. 앞선 실제 개수 저장이 뒤 포인터를 바꾸면 다음 조회에 반영한다.
    /// </summary>
    /// <param name="headers">20DWORD 등록 표.</param>
    /// <param name="resetCount">헤더별 개수 쓰기. null이면 도달할 때 거부한다.</param>
    public static void ResetRegistered(ReadOnlySpan<uint> headers, Action<uint>? resetCount)
    {
        RequireRegistry(headers);
        // 앞선 실제 개수 저장이 뒤 포인터를 바꾸면 다음 조회에 반영한다
        for (int index = 0; index < RegistryCapacity; index++)
        {
            uint header = headers[index];
            if (header != 0)
            {
                if (resetCount is null)
                {
                    throw new InvalidOperationException("State SID 개수 정리의 실제 쓰기 미연결");
                }
                resetCount(header);
            }
        }
    }

    /// <summary>
    /// 그때 선택된 실제 개수만 0으로 쓴다. 다중선택도 같은 Client 필드를 빌린다.
    /// </summary>
    /// <param name="header">개수를 지울 목록의 헤더 토큰.</param>
    /// <param name="multipleCount">다중선택 개수. 다중선택 헤더일 때만 0으로 쓴다.</param>
    public void ResetCount(uint header, ref int multipleCount)
    {
        switch (header)
        {
            case 0x55a4d0u: Factories.Count = 0; return;
            case 0x55a4ecu: Dudes.Count = 0; return;
            case 0x55a708u: Outposts.Count = 0; return;
            case 0x592f80u: Providers.Count = 0; return;
            case 0x5950d8u: Geysers.Count = 0; return;
            case 0x5954c0u: Priests.Count = 0; return;
            case 0x59a9d4u: Residences.Count = 0; return;
            case MultipleHeader: multipleCount = 0; return;
            case 0x5e1898u: Temples.Count = 0; return;
            default: throw new ArgumentOutOfRangeException(nameof(header), header, "State SID 등록 표의 미연결 실제 헤더");
        }
    }

    /// <summary>
    /// 기존 독립 일반 설치의 fresh 의미만 호환한다. Native 개수 정리·prepared 대여에는 호출하지 않는다.
    /// </summary>
    public void PrepareGeneralWorld()
    {
        // 기존 독립 일반 World의 새 목록 의미를 같은 주소에서 유지한다
        foreach (RawPostPopList list in new[] { Factories, Dudes, Outposts, Providers, Geysers, Priests, Residences, Temples })
        {
            Array.Fill(list.Entries, 0u);
            list.Count = 0;
        }
    }

    /// <summary>짧은 호스트 뷰를 원문 고정 20칸으로 잘못 역참조하지 않는다.</summary>
    /// <param name="headers">검사할 등록 표.</param>
    private static void RequireRegistry(ReadOnlySpan<uint> headers)
    {
        if (headers.Length < RegistryCapacity)
        {
            throw new ArgumentException("State SID 등록 표의 20DWORD 누락", nameof(headers));
        }
    }
}
