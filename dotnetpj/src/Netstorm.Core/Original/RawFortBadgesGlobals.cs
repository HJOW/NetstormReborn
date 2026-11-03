namespace Netstorm.Core.Original;

/// <summary>
/// 뱃지 28개 기록의 고정 크기다(cpppj <c>o/RawFortBadgesGlobals.h</c>의 <c>kFortBadgesRecordCount</c>·<c>kFortBadgesRecordBytes</c>·<c>kFortBadgesGlobalsBytes</c>·<c>kFortBadgesCountOffset</c>).
/// 10.78의 실제 기록 수·원본 stride·전체 6160바이트·수량 DWORD 위치를 그대로 둔다.
/// </summary>
/// <remarks>2026-11-03 F2c-6에서 추가했다.</remarks>
public static class FortBadgesGlobalsShape
{
    /// <summary>원본 00593160..00594970의 실제 기록 수다.</summary>
    public const int RecordCount = 28;

    /// <summary>type와 count 사이 212바이트를 포함한 실제 기록 폭이다.</summary>
    public const int RecordBytes = 220;

    /// <summary>type·ASCII 자료·padding·count를 포함한 전체 6160바이트다.</summary>
    public const int GlobalsBytes = RecordCount * RecordBytes;

    /// <summary>각 기록에서 현재 수량 DWORD가 놓이는 위치다.</summary>
    public const int CountOffset = 216;
}

/// <summary>
/// 뱃지 원본 CRT 구성과 수량 초기화를 같은 장부에 적용한다(cpppj <c>o/RawFortBadgesGlobals</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-6에서 추가했다.</remarks>
public static class RawFortBadgesGlobals
{
    /// <summary>원본 CRT가 각 기록에 배치하는 고정 type와 앞 일곱 행이다.</summary>
    private static readonly (uint Type, string[] Rows)[] InitialRecords =
    [
        (71u, ["", " sss   ", "   s   ", "  ss   ", "   s   ", "       ", "       "]),
        (72u, ["       ", "       ", "       ", "   s   ", " s   s ", " ss ss ", "   s   "]),
        (73u, ["       ", "       ", "       ", "   s   ", " s   s ", " ss ss ", "   s   "]),
        (78u, ["       ", "       ", "   s   ", "  s s  ", " ss ss ", "       ", "       "]),
        (128u, ["       ", "       ", " ssss  ", "  s s  ", "       ", "  s    ", "       "]),
        (111u, ["       ", "  s s  ", "  s s  ", "   s   ", "   s   ", "   s   ", "       "]),
        (136u, ["       ", " s   s ", "       ", "   s   ", "       ", " s   s ", "       "]),
        (133u, ["       ", "     s ", "    s  ", "   s   ", "  s    ", " s     ", "       "]),
        (140u, ["       ", " w  w  ", "w    w ", "w ww w ", " w  w  ", "  ww   ", "       "]),
        (139u, ["       ", " wwww  ", "  www  ", "   ww  ", "    w  ", " wwww  ", "       "]),
        (138u, ["       ", " wwww  ", "  www  ", "   ww  ", "    w  ", " wwww  ", "       "]),
        (75u, ["       ", "   w   ", "  w w  ", "   w   ", "   w   ", "   ww  ", "       "]),
        (142u, ["       ", " wwwww ", "   w   ", "  w w  ", " ww ww ", "       ", "       "]),
        (141u, ["       ", "       ", "wwwwww ", "   w   ", " ww ww ", "  w w  ", "       "]),
        (137u, ["       ", "   w   ", "   ww  ", " w w w ", " wwwww ", "       ", "       "]),
        (148u, ["       ", "       ", " t   t ", "  t t  ", " t t t ", "       ", "       "]),
        (77u, ["       ", "  t    ", "  t t  ", "  t t  ", "  t t  ", "   t   ", "       "]),
        (79u, ["       ", " tt tt ", " tt tt ", " tt tt ", " tt tt ", " tt tt ", "       "]),
        (131u, ["       ", "   t   ", "  ttt  ", "   t   ", "  ttt  ", "  ttt  ", "       "]),
        (135u, ["       ", "    tt ", " ttttt ", " tttt  ", " tttt  ", " t  t  ", "       "]),
        (143u, ["       ", "  rrr  ", " r   r ", "   r   ", "   r   ", "       ", "       "]),
        (144u, ["       ", "  rrr  ", " r   r ", "   r   ", "   r   ", "       ", "       "]),
        (76u, ["       ", "  rr   ", "   rr  ", "   r   ", "  rr   ", "   r   ", "       "]),
        (147u, ["       ", "   r   ", "  rrr  ", "   r   ", "  r r  ", " rr rr ", "       "]),
        (129u, ["       ", "    rr ", " rr    ", "    rr ", " rr    ", "    rr ", "       "]),
        (146u, ["       ", " r r r ", "       ", "   r   ", "       ", " r   r ", "       "]),
        (145u, ["       ", " rr rr ", " r   r ", " rr rr ", "  r r  ", "   r   ", "       "]),
    ];

    /// <summary>
    /// 원본 물리 장부 밖 접근을 임의 초기화로 대신하지 않고 효과 전에 거부한다.
    /// </summary>
    /// <param name="records">검사할 장부다.</param>
    /// <exception cref="ArgumentException">뱃지 실제 6160바이트 장부가 아니다.</exception>
    private static void Physical(Span<byte> records)
    {
        if (records.Length != FortBadgesGlobalsShape.GlobalsBytes)
        {
            throw new ArgumentException("Badges 실제6160바이트 장부 누락", nameof(records));
        }
    }

    /// <summary>
    /// 비정렬 DWORD 네 바이트만 little endian으로 쓴다.
    /// </summary>
    /// <param name="records">쓸 장부다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> records, int at, uint value)
    {
        // 인접한 원본 자료는 보존한다
        for (int index = 0; index < 4; index++)
        {
            records[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>
    /// 004fcbe0: CRT가 지정한 6152바이트만 구성한다. 첫 기록 +4..11의 8바이트는 입력 그대로 보존한다.
    /// </summary>
    /// <param name="records">구성할 6160바이트 장부다.</param>
    public static void InitializeFortBadgesRecords(Span<byte> records)
    {
        Physical(records);
        // 마지막 28번째 기록은 원본 55DWORD 전체 0 구성을 적용한다
        for (int index = 0; index < FortBadgesGlobalsShape.RecordCount; index++)
        {
            int at = index * FortBadgesGlobalsShape.RecordBytes;
            for (int offset = 0; offset < FortBadgesGlobalsShape.RecordBytes; offset++)
            {
                // 원본이 쓰지 않는 첫 기록 +4..11만 입력 그대로 둔다
                if (index != 0 || offset < 4 || offset >= 12)
                {
                    records[at + offset] = 0;
                }
            }
            if (index == InitialRecords.Length)
            {
                continue;
            }
            (uint type, string[] rows) = InitialRecords[index];
            Put(records, at, type);
            for (int row = 0; row < rows.Length; row++)
            {
                // NUL·뒤 일곱 행은 앞선 원본 0 구성으로 남긴다
                string line = rows[row];
                for (int column = 0; column < line.Length; column++)
                {
                    records[at + 4 + row * 15 + column] = (byte)line[column];
                }
            }
        }
    }

    /// <summary>
    /// 0047d860: 28개 count DWORD만 0으로 만들고 다른 6048바이트를 보존한다.
    /// </summary>
    /// <param name="records">초기화할 6160바이트 장부다.</param>
    public static void ResetFortBadgeCounts(Span<byte> records)
    {
        Physical(records);
        // 원본 28개 count 주소만 지운다
        for (int index = 0; index < FortBadgesGlobalsShape.RecordCount; index++)
        {
            Put(records, index * FortBadgesGlobalsShape.RecordBytes + FortBadgesGlobalsShape.CountOffset, 0);
        }
    }
}

/// <summary>
/// 한 Client 수명에 BSS 0→CRT를 한 번 적용하는 실제 6160바이트 소유자다(cpppj <c>o/RawFortBadgesGlobals.h</c>의 <c>RawFortBadgesGlobals</c>).
/// 세션·저장소 생성에 묶지 않는다.
/// </summary>
/// <remarks>2026-11-03 F2c-6에서 추가했다. C++의 const·비const <c>Records</c> 겹침은 C#에서 이름으로 나눈다.</remarks>
public sealed class FortBadgesGlobalsOwner
{
    /// <summary>원본 BSS처럼 처음 0인 단일 고정 장부이며 타입·ASCII 자료도 함께 소유한다.</summary>
    private readonly byte[] records = new byte[FortBadgesGlobalsShape.GlobalsBytes];

    /// <summary>
    /// 새 배열을 최초 0으로 준비한 뒤 원본 CRT 구성을 한 번 적용한다.
    /// </summary>
    public FortBadgesGlobalsOwner()
    {
        RawFortBadgesGlobals.InitializeFortBadgesRecords(records);
    }

    /// <summary>
    /// 기존 MetaLoader와 같은 배열의 mutable 뷰다.
    /// </summary>
    /// <returns>같은 배열을 빌리는 Span이다.</returns>
    public Span<byte> Records()
    {
        return records.AsSpan();
    }

    /// <summary>
    /// 새 getter·writer가 같은 배열을 읽기만 빌리는 뷰다(C++ const 겹침).
    /// </summary>
    /// <returns>같은 배열을 읽기만 빌리는 Span이다.</returns>
    public ReadOnlySpan<byte> RecordsReadOnly()
    {
        return records.AsSpan();
    }

    /// <summary>
    /// 원본 시작 reset을 명시 호출하며 전체 CRT 초기화는 반복하지 않는다.
    /// </summary>
    public void ResetCounts()
    {
        RawFortBadgesGlobals.ResetFortBadgeCounts(records);
    }
}
