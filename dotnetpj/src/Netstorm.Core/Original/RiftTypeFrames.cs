namespace Netstorm.Core.Original;

/// <summary>
/// 프레임의 방향·변형·번호·플래그 코드다(cpppj <c>o/RiftType.h</c>의 <c>FrameCode</c>). 원본 배열 원소는 정확히 4바이트다.
/// 기존 경로의 <c>Netstorm.Assets.FrameCode</c>(문자·정수 필드, 지금도 쓰는 곳이 있다)와 이름이 겹치지 않도록 <c>RiftFrameCode</c>로 둔다.
/// </summary>
/// <param name="Side">원본 코드 +0, 방향 문자.</param>
/// <param name="Variant">원본 코드 +1, 변형 문자.</param>
/// <param name="Number">원본 코드 +2, 8비트 프레임 번호.</param>
/// <param name="Flags">원본 코드 +3. 검색에서 MSVC signed char로 부호 확장한다.</param>
/// <remarks>2026-11-02 F1에서 추가했다.</remarks>
public readonly record struct RiftFrameCode(byte Side, byte Variant, byte Number, byte Flags);

/// <summary>
/// 한 타입의 프레임 코드 배열과 순차 검색 세 가지, 방향 글자 구간 표다(cpppj <c>o/RiftType.cpp</c>의 <c>RiftTypeFrames</c>).
/// 원본 타입 +0x114(개수), +0x124(코드 포인터)를 소유하는 배열로 바꾼 것이며 검색은 모두 앞에서부터 훑어 첫 일치를 돌려준다(없으면 -1).
/// </summary>
/// <remarks>
/// 2026-11-02 F1에서 추가했다. 패치 0049a9a0·0049a9e0·0049aa30 ↔ CD 00444350·00444410·00444460의 순차 검색 세 가지와
/// 방향 글자 구간을 만드는 0049b060 ↔ CD 00444da0의 결과다. 기존 경로의 <c>Netstorm.Assets.TypeFrameTable</c>은 교체 대상이라 연결하지 않는다.
/// </remarks>
public sealed class RiftTypeFrames
{
    /// <summary>방향 글자 하나('A'~'P')가 차지하는 프레임 구간이다.</summary>
    /// <param name="First">그 글자를 가진 첫 프레임 번호(없으면 -1).</param>
    /// <param name="Count">그 글자를 가진 프레임 수.</param>
    public readonly record struct LetterRun(int First, int Count);

    /// <summary>방향 글자 구간 표의 첫 글자('A')다.</summary>
    private const int FirstLetter = 'A';

    /// <summary>방향 글자 구간 표의 마지막 글자('P')다.</summary>
    private const int LastLetter = 'P';

    /// <summary>타입별 프레임 코드다. 원본 배열의 저장 순서를 보존한다.</summary>
    private readonly RiftFrameCode[] _frames;

    /// <summary>코드 배열을 복사해 소유한다. 반환 인덱스는 부호 있는 32비트이므로 그 범위를 넘는 배열은 .NET 배열 한도상 만들 수 없어 따로 거부하지 않는다.</summary>
    /// <param name="frames">클러스터 선언 순서의 프레임 코드.</param>
    public RiftTypeFrames(IEnumerable<RiftFrameCode> frames)
    {
        ArgumentNullException.ThrowIfNull(frames);
        _frames = frames.ToArray();
    }

    /// <summary>코드 배열을 검증·직렬화 도구에 공개한다. 원본 코드의 저장 순서 그대로다.</summary>
    public IReadOnlyList<RiftFrameCode> Codes => _frames;

    /// <summary>movsx로 읽는 원본 signed char를 부호 확장한다.</summary>
    /// <param name="value">플래그 바이트.</param>
    private static int SignedFlag(byte value) => (sbyte)value;

    /// <summary>번호까지 같은 첫 프레임을 찾는다(패치 0049a9a0 ↔ CD 00444350). 중복 프레임은 원본 순서의 첫 항목이 이긴다.</summary>
    /// <param name="side">방향 문자.</param>
    /// <param name="variant">변형 문자.</param>
    /// <param name="number">프레임 번호.</param>
    /// <returns>프레임 인덱스, 없으면 -1.</returns>
    public int FindNumber(byte side, byte variant, byte number)
    {
        // 첫 일치를 반환하므로 중복 프레임의 원본 순서가 유지된다
        for (int index = 0; index < _frames.Length; index++)
        {
            RiftFrameCode frame = _frames[index];
            if (frame.Side == side && frame.Variant == variant && frame.Number == number)
            {
                return index;
            }
        }
        return -1;
    }

    /// <summary>방향·변형·번호가 맞고 (부호 확장한 플래그 &amp; 마스크)가 0이 아닌 첫 프레임을 찾는다(패치 0049a9e0 ↔ CD 00444410).</summary>
    /// <param name="side">방향 문자.</param>
    /// <param name="variant">변형 문자.</param>
    /// <param name="number">프레임 번호.</param>
    /// <param name="mask">플래그 마스크.</param>
    /// <returns>프레임 인덱스, 없으면 -1.</returns>
    public int FindMasked(byte side, byte variant, byte number, uint mask)
    {
        // 원본처럼 방향·변형·번호를 확인한 뒤 마스크를 검사한다
        for (int index = 0; index < _frames.Length; index++)
        {
            RiftFrameCode frame = _frames[index];
            if (frame.Side == side && frame.Variant == variant && frame.Number == number
                && (unchecked((uint)SignedFlag(frame.Flags)) & mask) != 0)
            {
                return index;
            }
        }
        return -1;
    }

    /// <summary>방향·변형·플래그가 정확히 같은 첫 프레임을 찾는다(패치 0049aa30 ↔ CD 00444460). 번호는 이 검색의 조건이 아니다.</summary>
    /// <param name="side">방향 문자.</param>
    /// <param name="variant">변형 문자.</param>
    /// <param name="flags">부호 있는 플래그 값(프레임의 플래그를 부호 확장해 비교한다).</param>
    /// <returns>프레임 인덱스, 없으면 -1.</returns>
    public int FindFlags(byte side, byte variant, int flags)
    {
        // 정확한 플래그 값까지 일치하는 첫 코드를 찾는다
        for (int index = 0; index < _frames.Length; index++)
        {
            RiftFrameCode frame = _frames[index];
            if (frame.Side == side && frame.Variant == variant && SignedFlag(frame.Flags) == flags)
            {
                return index;
            }
        }
        return -1;
    }

    /// <summary>
    /// 방향 글자 하나가 차지하는 프레임의 첫 번호와 그 글자를 가진 프레임 수를 센다. 원본 타입 +0x130(첫 번호)·+0x170(개수) 표를
    /// 만드는 0049b060 ↔ CD 00444da0의 결과다. 비교는 signed char라 0x80 이상의 방향 바이트는 'A'~'P'와 같아질 수 없다.
    /// </summary>
    /// <param name="letter">'A'~'P' 중 하나.</param>
    /// <exception cref="ArgumentOutOfRangeException">글자가 'A'~'P' 밖이다(원본 표에 없다).</exception>
    public LetterRun Run(byte letter)
    {
        if (letter < FirstLetter || letter > LastLetter)
        {
            throw new ArgumentOutOfRangeException(nameof(letter), letter, "프레임 글자 구간은 A~P만 있다");
        }
        int first = -1;
        int count = 0;
        // 배열 전체를 한 번 훑어 같은 글자를 센다
        for (int index = 0; index < _frames.Length; index++)
        {
            if (_frames[index].Side != letter)
            {
                continue;
            }
            if (first == -1)
            {
                first = index;
            }
            count++;
        }
        return new LetterRun(first, count);
    }

    /// <summary>구간이 하나라도 있는 글자 수다(원본 타입 +0x12c). 원본이 글자 16개를 차례로 보며 첫 번호가 처음 정해질 때마다 올리는 값이다.</summary>
    public int LetterKinds()
    {
        int kinds = 0;
        // 글자 'A'~'P'를 하나씩 확인한다
        for (int letter = FirstLetter; letter <= LastLetter; letter++)
        {
            if (Run((byte)letter).First != -1)
            {
                kinds++;
            }
        }
        return kinds;
    }
}
