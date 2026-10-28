using System.Runtime.InteropServices;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// 패치판 플레이어 SP 저장소(원본 0040eba0 초기화·0040ec50 읽기·0040edb0 쓰기·00491680 회전).
/// 플레이어별 32비트 SP 를 32개의 9칸 DWORD 배열에 비트로 흩뿌려 보관한다.
/// 값의 k번째 비트는 배열 k 의 (offset+k)&amp;31 번째 비트이며, 같은 칸의 다른 비트는 난수 잡음으로 채운다.
/// 초기화는 재시드 콜백 → offset 추첨 → 32번 순열 교환 → 항등 순열 복원 → 배열 채우기 순서다.
/// 이 클래스는 10.78 패치판 SP 저장소만 옮겼다(CD 판은 지역 float SP 를 쓴다).
/// 근거: cpppj/src/o/SpStore.cpp (W05 선행 조각, 2026-10-22; 명시 할당 공급자는 R0-4 재대조, 2026-10-28).
/// </summary>
public sealed class RawSpStore
{
    /// <summary>SP 비트 수. 값 하나가 차지하는 비트 수와 배열 수가 같다.</summary>
    public const int Bits = 32;

    /// <summary>소유자 수(0~8). 한 배열마다 이 수만큼 DWORD 가 있다.</summary>
    public const int Owners = 9;

    /// <summary>초기화가 offset 을 고를 때 쓰는 난수 상한(원본 push 0x64).</summary>
    private const uint OffsetLimit = 100;

    /// <summary>순열 교환과 잡음이 쓰는 난수 상한(원본 push 0x20).</summary>
    private const uint BitLimit = 32;

    /// <summary>한 비트를 쓸 때 섞는 잡음 비트의 수(원본 edi=5).</summary>
    private const int NoisePerBit = 5;

    /// <summary>회전 한 번이 전진시키는 offset. 원본 ROL3 과 같다.</summary>
    private const uint RotateStep = 3;

    /// <summary>offset 이 이 값보다 크면 빼서 되돌린다. 같은 값일 때는 빼지 않는다.</summary>
    private const uint OffsetWrap = 9600;

    /// <summary>원본 전역 난수. 초기화·잡음 비트가 같은 상태를 공유한다.</summary>
    private readonly NetstormRandom _rng;

    /// <summary>초기화 때 난수 상태를 주입하는 콜백. 원본 시각 기반 재시드를 대신한다.</summary>
    private readonly Func<uint>? _reseed;

    /// <summary>초기화가 배열을 채우는 값(원본 fill). 명시 할당 공급자가 있으면 쓰지 않는다.</summary>
    private readonly uint _fill;

    /// <summary>초기화가 32개 행마다 한 번씩 부르는 실제 할당 입력 공급자. 36바이트(9 DWORD)를 채우며, 던지면 그 행부터 초기화가 중단된다.</summary>
    private readonly Action<Span<byte>>? _allocation;

    /// <summary>추적 offset 의 외부 기록자. 원본이 기존 전역 주소를 빌리는 자리다.</summary>
    private Action<int>? _trackingWrite;

    /// <summary>초기화가 끝났는지. false 면 Get/Set/Rotate 가 먼저 초기화한다.</summary>
    private bool _initialized;

    /// <summary>SP 비트 위치 기저. 회전마다 3씩 전진한다.</summary>
    private uint _offset;

    /// <summary>초기화가 만든 순열. 실제 원본은 교환 뒤 항등 순열로 되돌린다.</summary>
    private readonly short[] _permutation = new short[Bits];

    /// <summary>실제 비트를 흩뿌려 보관하는 32×9 DWORD 배열. 배열 k 의 owner 번째가 k*9+owner 다.</summary>
    private readonly uint[] _pools = new uint[Bits * Owners];

    /// <summary>실제 배열 준비 여부(원본 00594fd0 의 NULL 조건). 첫 행 할당이 끝나면 참이 되며 전체 초기화 완료보다 앞설 수 있다.</summary>
    private bool _firstPool;

    /// <summary>
    /// SP 저장소를 만든다. 난수는 이 객체보다 오래 살아야 하며 초기화 전에는 배열을 쓰지 않는다.
    /// </summary>
    /// <param name="rng">원본 전역 난수.</param>
    /// <param name="reseed">초기화 때 난수 상태를 만드는 콜백. null 이면 Get/Set/Rotate 가 초기화에서 예외다.</param>
    /// <param name="fill">초기화가 모든 배열 칸에 채우는 값.</param>
    public RawSpStore(NetstormRandom rng, Func<uint>? reseed, uint fill = 0)
    {
        ArgumentNullException.ThrowIfNull(rng);
        _rng = rng;
        _reseed = reseed;
        _fill = fill;
        for (int index = 0; index < Bits; index++)
        {
            _permutation[index] = (short)index;
        }
    }

    /// <summary>
    /// SP 저장소를 만든다. 초기화 32개 행의 실제 할당 입력을 공급자로 받으며, 공급자가 던지면 그 행부터 초기화가 중단되고 앞선 행은 유지된다.
    /// 난수는 이 객체보다 오래 살아야 하며 초기화 전에는 배열을 쓰지 않는다.
    /// 근거: cpppj/src/o/SpStore.cpp 명시 공급자 생성자 (R0-4 재대조, 2026-10-28).
    /// </summary>
    /// <param name="rng">원본 전역 난수.</param>
    /// <param name="reseed">초기화 때 난수 상태를 만드는 콜백. null 이면 Get/Set/Rotate 가 초기화에서 예외다.</param>
    /// <param name="allocation">32개 행마다 36바이트 입력을 채우는 공급자. null 이면 거부한다.</param>
    public RawSpStore(NetstormRandom rng, Func<uint>? reseed, Action<Span<byte>> allocation)
    {
        ArgumentNullException.ThrowIfNull(rng);
        ArgumentNullException.ThrowIfNull(allocation);
        _rng = rng;
        _reseed = reseed;
        _allocation = allocation;
        for (int index = 0; index < Bits; index++)
        {
            _permutation[index] = (short)index;
        }
    }

    /// <summary>초기화를 마쳤는지.</summary>
    public bool Initialized => _initialized;

    /// <summary>현재 SP 비트 위치.</summary>
    public uint Offset => _offset;

    /// <summary>실제 배열 준비 여부. 초기화 전에는 false 다.</summary>
    public bool FirstPool => _firstPool;

    /// <summary>현재 순열(진단용). 초기화 뒤에는 항등 순열이다.</summary>
    public ReadOnlySpan<short> Permutation => _permutation;

    /// <summary>실제 32×9 DWORD 배열(진단·체크섬용). 배열 k 의 owner 번째가 k*9+owner 다.</summary>
    public ReadOnlySpan<uint> Pools => _pools;

    /// <summary>
    /// 외부 추적 offset 에 초기화 offset 을 쓰게 연결한다. 원본은 늦은 연결이나 다른 소유자 연결을 거부한다.
    /// </summary>
    /// <param name="write">초기화가 offset 을 넘겨줄 기록자.</param>
    public void BindTrackingOffset(Action<int> write)
    {
        ArgumentNullException.ThrowIfNull(write);
        if (_initialized || (_trackingWrite is not null && !ReferenceEquals(_trackingWrite, write)))
        {
            throw new InvalidOperationException("SP 추적 offset 의 늦은 연결/다른 소유자 오류");
        }
        _trackingWrite = write;
    }

    /// <summary>소유자 번호가 0~8 인지 확인한다. 원본은 범위 밖에서 힙 밖을 읽거나 쓴다.</summary>
    /// <param name="owner">확인할 소유자 번호.</param>
    private static void CheckOwner(uint owner)
    {
        if (owner >= Owners)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), owner, "SP 저장소 소유자 번호는 0~8 이어야 합니다");
        }
    }

    /// <summary>
    /// 원본 0040eba0: 재시드 → offset 추첨 → 추적 offset 기록 → 32번 순열 교환 → 항등 순열 복원 → 배열 채우기.
    /// 배열 채우기는 행마다 공급자 입력(또는 fill 값)으로 같은 현재 행을 발행하며, 공급자가 던지면 앞선 행을 되돌리지 않고 중단한다.
    /// 추적 offset 쓰기와 첫 배열 준비 표시는 R4 세션 조립 때 장기 주소 공유로 바꾸며, 이 단위는 기록 콜백으로 L1을 대조한다.
    /// </summary>
    public void Initialize()
    {
        if (_reseed is null)
        {
            throw new InvalidOperationException("SP 저장소 재시드 콜백 누락");
        }
        _rng.SetState(_reseed());
        _offset = (uint)_rng.Next((int)OffsetLimit);
        // 원본은 첫 항등 순열을 만든 뒤 같은 추적 offset 전역에도 현재 offset 을 쓴다
        for (int index = 0; index < Bits; index++)
        {
            _permutation[index] = (short)index;
        }
        _trackingWrite?.Invoke(unchecked((int)_offset));
        // 실제 32번 교환을 수행해 SP 몸체가 읽는 순열을 원본과 같게 만든다
        for (int index = 0; index < Bits; index++)
        {
            int other = _rng.Next((int)BitLimit);
            (_permutation[index], _permutation[other]) = (_permutation[other], _permutation[index]);
        }
        // 원본은 교환 결과를 저장에 쓰기 전에 다시 항등 순열로 덮어쓴다
        for (int index = 0; index < Bits; index++)
        {
            _permutation[index] = (short)index;
        }
        // 각 행의 정상 복귀 뒤에만 같은 현재 행을 발행한다. 뒤의 실패가 앞선 행을 되돌리지 않는다
        Span<uint> input = stackalloc uint[Owners];
        for (int index = 0; index < Bits; index++)
        {
            if (_allocation is not null)
            {
                Span<byte> bytes = MemoryMarshal.AsBytes(input);
                _allocation(bytes);
            }
            else
            {
                input.Fill(_fill);
            }
            for (int owner = 0; owner < Owners; owner++)
            {
                _pools[index * Owners + owner] = input[owner];
            }
            if (index == 0)
            {
                _firstPool = true;
            }
        }
        _initialized = true;
    }

    /// <summary>
    /// 원본 0040ec50: 배열 k 의 (offset+k)&amp;31 비트를 모아 값을 복원한다. 초기화 전이면 먼저 초기화한다.
    /// </summary>
    /// <param name="owner">읽을 소유자 번호(0~8).</param>
    public uint Get(uint owner)
    {
        CheckOwner(owner);
        if (!_initialized)
        {
            Initialize();
        }
        uint bits = 0;
        // 값의 32비트를 낮은 비트부터 하나씩 모은다
        for (int k = 0; k < Bits; k++)
        {
            uint mask = 1u << (int)((_offset + (uint)k) & 31);
            if ((_pools[_permutation[k] * Owners + (int)owner] & mask) != 0)
            {
                bits |= 1u << k;
            }
        }
        return bits;
    }

    /// <summary>
    /// 원본 0040edb0: 실제 비트를 쓰고 같은 DWORD 의 다른 위치에 난수 잡음 5개를 더한다.
    /// 실제 비트 위치는 잡음에서 제외하므로 값은 그대로 읽힌다.
    /// </summary>
    /// <param name="owner">쓸 소유자 번호(0~8).</param>
    /// <param name="bits">쓸 32비트 값.</param>
    public void Set(uint owner, uint bits)
    {
        CheckOwner(owner);
        if (!_initialized)
        {
            Initialize();
        }
        // 값의 32비트를 낮은 비트부터 하나씩 기록한다
        for (int k = 0; k < Bits; k++)
        {
            int index = _permutation[k] * Owners + (int)owner;
            uint mask = 1u << (int)((_offset + (uint)k) & 31);
            if (((bits >> k) & 1) != 0)
            {
                _pools[index] |= mask;
            }
            else
            {
                _pools[index] &= ~mask;
            }
            // 잡음은 실제 비트를 제외한 위치에만 들어간다
            for (int n = 0; n < NoisePerBit; n++)
            {
                _pools[index] |= 1u << _rng.Next((int)BitLimit) & ~mask;
            }
        }
    }

    /// <summary>
    /// 원본 00491680: 전체 배열을 왼쪽 3비트 회전하고 offset 도 같이 전진시켜 실제 값을 유지한다.
    /// offset 이 9600 보다 크면 9600 을 뺀다(같으면 그대로 둔다).
    /// </summary>
    public void Rotate()
    {
        if (!_initialized)
        {
            Initialize();
        }
        uint advanced = _offset + RotateStep;
        _offset = unchecked((int)advanced) > (int)OffsetWrap ? advanced - OffsetWrap : advanced;
        // 실제 순열의 각 배열을 소유자 0..8 순으로 같은 물리 DWORD 에서 회전한다
        foreach (short index in _permutation)
        {
            for (int owner = 0; owner < Owners; owner++)
            {
                int slot = index * Owners + owner;
                uint word = _pools[slot];
                _pools[slot] = (word << 3) | (word >> 29);
            }
        }
    }
}
