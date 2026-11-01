namespace Netstorm.Core.Original;

/// <summary>원본 SaveImage의 진단 후 계속 조건이다. 물리 범위 초과는 호스트에서 별도로 거부한다.</summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public enum FortImageAssertion
{
    /// <summary>004bcd80의 numTypes&lt;256 진단이다.</summary>
    TypeCount,

    /// <summary>memstream의 pos+length&lt;=alloc 진단이다.</summary>
    StreamCapacity,

    /// <summary>0044d640의 destStream.pos+s-&gt;end&lt;alloc 엄격 진단이다.</summary>
    SectionCapacity,
}

/// <summary>원본 0x8000 메모리 stream을 소유하며 논리 끝과 할당의 0 tail을 구분한다(원본 <c>FortImageData</c>).</summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class FortImageData
{
    /// <summary>실제 32768바이트를 0으로 초기화한 뒤 현재 섹션을 쓰는 단일 배열이다.</summary>
    public byte[] Storage { get; init; } = [];

    /// <summary>원본 stream +8의 현재 논리 끝이다. 반환 cursor는 0이다.</summary>
    public int End { get; init; }

    /// <summary>실제 이미지 끝까지만 빌린다. 배열 밖으로 변조된 end는 거부한다.</summary>
    public ReadOnlySpan<byte> Bytes()
    {
        if (End < 0 || End > Storage.Length)
        {
            throw new ArgumentOutOfRangeException(null, "Fort 이미지의 논리 끝이 물리 배열을 초과함");
        }
        return Storage.AsSpan(0, End);
    }
}

/// <summary>현재 타입 개수와 원본 선택 진단만 외부 경계다. 파일 읽기나 게임 월드를 대신하지 않는다(원본 <c>FortImageHooks</c>).</summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class FortImageHooks
{
    /// <summary>004bcd80이 읽는 현재 00541348의 타입 개수다.</summary>
    public Func<uint>? TypeCount { get; set; }

    /// <summary>해당 원문 진단이 선택된 위치에서만 필수다.</summary>
    public Action<FortImageAssertion>? Assertion { get; set; }

    /// <summary>실제 manager의 서명 필드 쓰기/기본값 정규화 뒤 파일 flag를 조회한다. 생략하면 기존 fort.Flag를 쓴다.</summary>
    public Func<byte>? SignatureFlag { get; set; }

    /// <summary>실제 manager의 이름 검색/current 효과와 같은 현재 섹션을 빌린다. 생략하면 기존 raw 배열을 쓴다.</summary>
    public Func<int, ReadOnlyMemory<byte>>? Section { get; set; }
}

/// <summary>
/// 현재 Template의 35개 원문 섹션을 PATCH DataManager SaveImage 형식으로 직렬화한다(004bf520→0044d640/004bcd80).
/// 섹션 현재 원문을 복사하며 예전 파일 바이트나 해석 필드로 내용을 대체하지 않는다. cpppj <c>o/RawFortImage</c>를 옮겼다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class RawFortImage
{
    /// <summary>0044d640이 실제 memstream에 확보하는 고정 물리 크기다.</summary>
    public const int Capacity = 0x8000;

    /// <summary>이미지 첫 바이트의 서명('F')이다.</summary>
    private const byte Signature = 0x46;

    /// <summary>flag가 0xfe일 때 쓰는 서명 둘째 바이트다.</summary>
    private const byte FlagByte = 0xfe;

    /// <summary>길이 필드의 크기다. 길이에는 이 2바이트가 포함된다.</summary>
    private const int LengthBytes = 2;

    /// <summary>현재 35개 원문 섹션과 reader가 읽은 flag다. 별도 파일 이미지 캐시는 없다.</summary>
    private readonly FortTemplate _fort;

    /// <summary>현재 타입 개수와 실제 선택 진단만 빌린다.</summary>
    private readonly FortImageHooks _hooks;

    /// <summary>fort/현재 조회 대상은 <see cref="Save"/>가 끝날 때까지 살아 있어야 한다. 섹션 순서는 <see cref="FortTemplate.SectionNames"/>와 같다.</summary>
    /// <param name="edition">판본. PATCH(10.78)만 지원한다. 미확정 CD의 비슷한 몸체를 추정하지 않는다.</param>
    /// <param name="fort">직렬화할 Template.</param>
    /// <param name="hooks">타입 개수·진단 장치.</param>
    public RawFortImage(OriginalEdition edition, FortTemplate fort, FortImageHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(fort);
        ArgumentNullException.ThrowIfNull(hooks);
        if (edition != OriginalEdition.Patch1078 || hooks.TypeCount is null || fort.Sections.Count > FortTemplate.SectionCount)
        {
            throw new ArgumentException("Fort 이미지의 PATCH 현재 Template/타입 개수 계약 오류");
        }
        _fort = fort;
        _hooks = hooks;
    }

    /// <summary>선택된 진단이 연결되지 않으면 빈 성공으로 계속하지 않는다.</summary>
    /// <param name="kind">진단 종류.</param>
    private void Assert(FortImageAssertion kind)
    {
        if (_hooks.Assertion is null)
        {
            throw new InvalidOperationException("Fort 이미지의 실제 선택 진단 미연결");
        }
        _hooks.Assertion(kind);
    }

    /// <summary>길이 WORD를 쓴다. 원본처럼 자기 2바이트를 포함하고 낮은 바이트부터 저장한다.</summary>
    /// <param name="bytes">이미지 배열.</param>
    /// <param name="offset">길이 필드 위치.</param>
    /// <param name="length">저장할 길이.</param>
    private static void Length(byte[] bytes, int offset, int length)
    {
        bytes[offset] = (byte)length;
        bytes[offset + 1] = (byte)(length >> 8);
    }

    /// <summary>
    /// 004bf520→0044d640/004bcd80: F/flag + 35개 길이 WORD/현재 payload/마지막 길이 수정을 수행한다.
    /// 기존 해석된 Money/Technology 대신 현재 각 raw 섹션 끝까지 원문 그대로 쓴다.
    /// </summary>
    /// <returns>32768바이트 배열과 논리 끝.</returns>
    public FortImageData Save()
    {
        // 실제 0044cc20의 고정 배열 전체 0 초기화다
        byte[] storage = new byte[Capacity];
        // 원본 004bcd8f의 signed JL 관문이다
        if (unchecked((int)_hooks.TypeCount!()) >= 256)
        {
            Assert(FortImageAssertion.TypeCount);
        }
        storage[0] = Signature;
        storage[1] = _hooks.SignatureFlag is { } flag ? flag() : _fort.Flag == FlagByte ? FlagByte : (byte)0;
        int position = LengthBytes;
        // 이전 길이 필드는 다음 섹션 진입 직전과 마지막 반환 직전에 수정한다
        int previous = Capacity;
        // 고정 35 이름 순서로 없는 tail 섹션도 실제 빈 stream의 길이 2를 쓴다
        for (int index = 0; index < FortTemplate.SectionCount; index++)
        {
            // 원본 검색/current는 이전 길이 수정에 앞서 수행한다
            ReadOnlySpan<byte> section = _hooks.Section is { } lookup
                ? lookup(index).Span
                : index < _fort.Sections.Count ? _fort.Sections[index].Span : default;
            if (previous != Capacity)
            {
                Length(storage, previous, position - previous);
            }
            if (position > Capacity - LengthBytes)
            {
                Assert(FortImageAssertion.StreamCapacity);
                throw new ArgumentOutOfRangeException(null, "Fort 이미지 길이 필드의 물리 범위 초과");
            }
            previous = position;
            Length(storage, position, 0);
            position += LengthBytes;
            if (section.Length >= Capacity - position)
            {
                Assert(FortImageAssertion.SectionCapacity);
            }
            if (section.Length > Capacity - position)
            {
                throw new ArgumentOutOfRangeException(null, "Fort 이미지 payload의 물리 범위 초과");
            }
            section.CopyTo(storage.AsSpan(position));
            position += section.Length;
        }
        Length(storage, previous, position - previous);
        return new FortImageData { Storage = storage, End = position };
    }
}
