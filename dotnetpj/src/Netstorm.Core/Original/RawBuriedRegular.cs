// 오벨리스크의 실제 buried Handle004622f0을 같은 풀/프레임/공유 난수에 연결한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// 매몰 정기 모듈을 담는 네임스페이스다. 2026-11-19 0.659에서 R4-4 정기 처리용으로 추가했다.

/// <summary>원본 오벨리스크는 event0만 애니메이션·지연을 바꾸고 다른 사건은 기본 Handle로 반환한다.</summary>
/// <remarks>2026-11-19 0.659에서 추가했다. 근거: cpppj <c>o/RawBuriedRegular.h/.cpp</c>의 <c>RawBuriedRegular</c>다. event0의 0 나머지에서만 같은 게임 난수를 소비한다.</remarks>
public sealed class RawBuriedRegular
{
    /// <summary>10.78 생성자가 기록하는 실제 오벨리스크 가상 표다. 0.659에서 추가했다.</summary>
    private const uint BuriedHandleTable = 0x0050a748u;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치다. 0.659에서 추가했다.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 프레임 DWORD(4바이트)의 위치다. 0.659에서 추가했다.</summary>
    private const int FrameOffset = 36;

    /// <summary>현재 QA/TYPE/프레임을 읽고 쓰는 실제 raw 풀이다. 0.659에서 추가했다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 TYPE의 A/B 구간이다. 0.659에서 추가했다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>같은 풀의 실제 표시 갱신/프레임 지정기다. 0.659에서 추가했다.</summary>
    private readonly SquidFrame _setter;

    /// <summary>전체 월드가 공유하는 현재 난수 상태다. 0.659에서 추가했다.</summary>
    private readonly GameRandom _random;

    /// <summary>매몰 정기 본체를 만든다. 같은 풀/188개 현재 TYPE 프레임만 허용한다.</summary>
    public RawBuriedRegular(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames, SquidFrame setter, GameRandom random)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(setter);
        ArgumentNullException.ThrowIfNull(random);
        if (pool.Edition != OriginalEdition.Patch1078 || frames.Count != 188 || !ReferenceEquals(setter.Pool, pool))
        {
            throw new ArgumentException("오벨리스크 Handle의 판본/TYPE/프레임 풀 불일치", nameof(pool));
        }

        _pool = pool;
        _frames = frames;
        _setter = setter;
        _random = random;
    }

    /// <summary>실제 단일 풀을 연결 단계에서 확인한다. 0.659에서 추가했다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>TYPE 번호만 같은 다른 부모를 잘못 분배하지 않는다. 0.659에서 추가했다.</summary>
    public static bool Handles(RawSidPool pool, int sid)
    {
        ArgumentNullException.ThrowIfNull(pool);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            return false;
        }

        return BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(sid)) == BuriedHandleTable;
    }

    /// <summary>event1의 004acd10→0049a840은 QA로 주소를 계산한 반환값을 버린다. 다른 실제 상태/효과 변경은 없다. 0.659에서 추가했다.</summary>
    public float Handle(int sid, uint @event, uint count, float payload)
    {
        if (!Handles(_pool, sid))
        {
            throw new ArgumentException("오벨리스크 Handle 실제 가상 표 불일치", nameof(sid));
        }

        // 기본 00496e70도 전달된 float만 반환하며 event1에 추가 효과는 없다.
        if (@event != 0)
        {
            return payload;
        }

        int type = _pool.Slot(sid)[TypeOffset];
        if (type >= _frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "오벨리스크 Handle TYPE 범위 오류");
        }

        RiftTypeFrames.LetterRun run = _frames[type].Run((byte)'B');
        // 원본 TYPE+178/+138의 현재 B구간 수/첫 프레임이다.
        int divisor = unchecked((int)(unchecked((uint)run.Count) + 1u));
        int signedCount = unchecked((int)count);
        if (divisor == 0 || (signedCount == int.MinValue && divisor == -1))
        {
            throw new DivideByZeroException("오벨리스크 Handle 원본 IDIV 오류");
        }

        int remainder = signedCount % divisor;
        if (remainder != 0)
        {
            int frame = unchecked((int)(unchecked((uint)run.First) + unchecked((uint)remainder) - 1u));
            _setter.Set(sid, frame, 0);
            return 0.1f;
        }

        _setter.Set(sid, _frames[type].Run((byte)'A').First, 0);
        return _random.Next(6) + 2;
    }
}
