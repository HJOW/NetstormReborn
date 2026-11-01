namespace Netstorm.Core.Original;

/// <summary>
/// 크기(<see cref="Count"/>)와 용량(<see cref="Capacity"/>)을 따로 가지는 섹션 바이트 버퍼다(cpppj가 섹션에 쓰는 <c>std::vector&lt;std::uint8_t&gt;</c>).
/// 원본 MemStream의 alloc 필드는 이 용량에 대응한다. 용량이 0이면 data 토큰이 0이라 "섹션 없음"과 "빈 섹션"이 갈리고,
/// 크기가 용량보다 작으면 뒤쪽 압축을 건너뛰지 않는 등 Template 저장소의 분기가 용량으로 정해지므로 <c>byte[]</c>나 <c>List&lt;byte&gt;</c>로 바꿀 수 없다.
/// 재할당 규칙은 MSVC <c>vector</c>를 따른다: <see cref="Resize"/>가 용량을 넘으면 <c>max(용량 + 용량/2, 요청)</c>, <see cref="Reserve"/>가 용량을 넘으면 요청값 그대로다.
/// </summary>
/// <remarks>
/// 2026-11-01 0.130(TS0)에서 추가했다. 계획은 <c>docs/dotnet-template-storage-plan-20261101.md</c> 3.1절이다.
/// 재할당을 일으킬 수 있는 호출(<see cref="Resize"/>, <see cref="Reserve"/>, <see cref="TakeFrom"/>, <see cref="Release"/>)을 넘겨 <see cref="Span"/>·<see cref="Memory"/>를 들고 있으면
/// 옛 배열을 가리키게 된다. C++에서는 정의되지 않은 동작이지만 C#에서는 쓰기가 조용히 사라지므로 호출자가 다시 빌려야 한다.
/// </remarks>
public sealed class RawSectionBuffer
{
    /// <summary>용량만큼 할당된 배열이다. 앞 <see cref="_count"/>바이트가 현재 크기이고 뒤쪽은 예약 용량이다.</summary>
    private byte[] _storage = [];

    /// <summary>현재 크기(<c>size()</c>)다.</summary>
    private int _count;

    /// <summary>현재 크기다(<c>size()</c>).</summary>
    public int Count => _count;

    /// <summary>현재 용량이다(<c>capacity()</c>). 내부 배열 길이와 같다.</summary>
    public int Capacity => _storage.Length;

    /// <summary>앞 <see cref="Count"/>바이트만 빌려 준다(<c>data()</c>+<c>size()</c>). 재할당이 일어나면 이전에 빌린 범위는 옛 배열이다.</summary>
    public Span<byte> Span => _storage.AsSpan(0, _count);

    /// <summary>앞 <see cref="Count"/>바이트를 <see cref="Memory{T}"/>로 빌려 준다. 커서가 보관하는 용도이며 재할당 뒤에는 다시 빌려야 한다.</summary>
    public Memory<byte> Memory => _storage.AsMemory(0, _count);

    /// <summary>
    /// 지정한 위치의 바이트를 읽고 쓴다(<c>at()</c>). 크기를 넘으면 용량 안이라도 예외다.
    /// </summary>
    /// <param name="index">0 이상 <see cref="Count"/> 미만의 위치.</param>
    /// <exception cref="ArgumentOutOfRangeException">위치가 크기 밖이다.</exception>
    public byte this[int index]
    {
        get
        {
            CheckIndex(index);
            return _storage[index];
        }
        set
        {
            CheckIndex(index);
            _storage[index] = value;
        }
    }

    /// <summary>
    /// 범위 생성(<c>vector(first, last)</c>)과 같이 용량이 크기와 같은 새 버퍼를 만든다. 빈 범위는 용량 0이다.
    /// </summary>
    /// <param name="bytes">복사할 바이트.</param>
    public static RawSectionBuffer FromBytes(ReadOnlySpan<byte> bytes)
    {
        RawSectionBuffer buffer = new();
        if (!bytes.IsEmpty)
        {
            buffer._storage = bytes.ToArray();
            buffer._count = bytes.Length;
        }
        return buffer;
    }

    /// <summary>
    /// 복사 생성과 같이 크기만큼만 복사하고 용량은 새 크기와 같게 만든다. 원본의 예약 용량은 따라가지 않는다.
    /// </summary>
    public RawSectionBuffer Clone() => FromBytes(Span);

    /// <summary>
    /// <c>resize(n)</c>: 용량 안이면 재할당 없이 늘어난 구간만 0으로 채운다(예전에 줄였다가 남은 바이트를 되살리지 않는다).
    /// 용량을 넘으면 <c>max(용량 + 용량/2, n)</c>로 재할당하고 기존 내용을 보존하며 나머지는 0이다. 줄이는 경우 용량은 그대로다.
    /// </summary>
    /// <param name="size">새 크기.</param>
    /// <exception cref="ArgumentOutOfRangeException">크기가 음수이거나 배열 한도를 넘는다.</exception>
    public void Resize(int size)
    {
        if (size < 0 || size > Array.MaxLength)
        {
            throw new ArgumentOutOfRangeException(nameof(size), size, "섹션 버퍼 크기 범위");
        }
        if (size > Capacity)
        {
            // MSVC _Calculate_growth: 1.5배가 요청보다 작으면 요청 크기로 정한다. 배열 한도에서 자른다
            long grown = Math.Min((long)Capacity + Capacity / 2, Array.MaxLength);
            Reallocate((int)Math.Max(grown, size));
        }
        else if (size > _count)
        {
            // 줄이기 전에 있던 바이트가 배열에 남아 있으므로 늘어난 구간을 명시적으로 0으로 채운다
            _storage.AsSpan(_count, size - _count).Clear();
        }
        _count = size;
    }

    /// <summary>
    /// <c>reserve(n)</c>: 요청이 용량보다 크면 요청 크기 그대로 재할당한다. 크기와 내용은 유지한다.
    /// </summary>
    /// <param name="capacity">필요한 용량.</param>
    /// <exception cref="ArgumentOutOfRangeException">용량이 음수이거나 배열 한도를 넘는다.</exception>
    public void Reserve(int capacity)
    {
        if (capacity < 0 || capacity > Array.MaxLength)
        {
            throw new ArgumentOutOfRangeException(nameof(capacity), capacity, "섹션 버퍼 용량 범위");
        }
        if (capacity > Capacity)
        {
            Reallocate(capacity);
        }
    }

    /// <summary><c>clear()</c>: 크기만 0으로 만들고 용량은 유지한다.</summary>
    public void Clear()
    {
        _count = 0;
    }

    /// <summary><c>std::vector&lt;std::uint8_t&gt;{}.swap(v)</c>: 크기와 용량을 모두 0으로 돌려 할당을 반납한다.</summary>
    public void Release()
    {
        _storage = [];
        _count = 0;
    }

    /// <summary>
    /// 이동 대입: 배열을 넘겨받고 <paramref name="other"/>는 용량 0의 빈 버퍼가 된다. 이 버퍼가 가졌던 할당은 버려진다.
    /// 자기 자신을 넘기면 아무 일도 하지 않는다.
    /// </summary>
    /// <param name="other">내용을 넘길 버퍼.</param>
    public void TakeFrom(RawSectionBuffer other)
    {
        ArgumentNullException.ThrowIfNull(other);
        if (ReferenceEquals(this, other))
        {
            return;
        }
        _storage = other._storage;
        _count = other._count;
        other._storage = [];
        other._count = 0;
    }

    /// <summary>현재 크기만큼의 내용을 새 배열로 복사한다. 검사와 진단에서 비교하는 용도다.</summary>
    public byte[] ToArray() => Span.ToArray();

    /// <summary>
    /// 용량을 <paramref name="capacity"/>로 맞춘 새 배열로 옮긴다. 현재 크기 안의 내용만 복사하고 나머지는 0이다.
    /// </summary>
    /// <param name="capacity">새 용량(현재 크기 이상).</param>
    private void Reallocate(int capacity)
    {
        byte[] storage = new byte[capacity];
        _storage.AsSpan(0, _count).CopyTo(storage);
        _storage = storage;
    }

    /// <summary>위치가 크기 안인지 확인한다.</summary>
    /// <param name="index">검사할 위치.</param>
    private void CheckIndex(int index)
    {
        if ((uint)index >= (uint)_count)
        {
            throw new ArgumentOutOfRangeException(nameof(index), index, "섹션 버퍼 위치가 크기 밖");
        }
    }
}
