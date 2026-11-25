// 실제 Dude 최초 Pop의 idle 작업 번호·population 등록·I 첫 프레임을 같은 월드에 연결한다.
// cpppj o/RawDudeSpawn.h·RawDudeSpawn.cpp의 RawDudePostPop(00450cd0·CD004e9230)을 이식한다.
// 1.038에서 WP-4b Dude 전용 PostPop용으로 추가했다.
using Netstorm.Assets;

namespace Netstorm.Core.Original;

/// <summary>원본 population 목록·생성 차단·실제 Dude 자산 번호를 공유하는 상태다.</summary>
public sealed class DudeSpawnState
{
    /// <summary>생성 한도와 최초 Pop 등록이 같은 DWORD 목록을 사용한다. 세션이 SidLists.Dudes를 둔다.</summary>
    public RawPostPopList Population { get; set; } = new();

    /// <summary>패치0055a4e8·CD005417c8이 0이 아니면 생성을 막는다. PostPop 등록 자체는 막지 않는다.</summary>
    public bool Blocked { get; set; }

    /// <summary>원문00541170·CD0051ca5c의 실제 Dude 자산 번호다.</summary>
    public uint DudeType { get; set; } = 70;
}

/// <summary>최초 Dude Pop의 I 첫 프레임과 실제 공통 postPop을 연결하는 경계다.</summary>
public sealed class DudePostPopHooks
{
    /// <summary>실제004acee0·CD004acbc0의 I 첫 프레임 쓰기다.</summary>
    public Action<int, int, uint>? SetFrame { get; set; }

    /// <summary>같은 공통 후처리의 미연결 효과를 검사한다.</summary>
    public Action<int, uint>? ValidateBase { get; set; }

    /// <summary>실제004b0d30·CD004ae180 공통 후처리 호출이다.</summary>
    public Action<int, uint>? Base { get; set; }

    /// <summary>최초 QA 저장 뒤 현재 TYPE가 Bridge일 때만004214a0을 호출한다.</summary>
    public Action<int>? NotifySurface { get; set; }

    /// <summary>효과 묶음의 얕은 사본을 만든다. 외부 재연결을 캡처된 장치와 구별한다.</summary>
    internal DudePostPopHooks Copy() => (DudePostPopHooks)MemberwiseClone();
}

/// <summary>00450cd0·CD004e9230의 직접 전체 몸체다. 공유 Pop의 가상 분배 등록은 별도 연결한다.</summary>
public sealed class RawDudePostPop
{
    /// <summary>실패 없이 통과하는 PATCH Dude 가상 표다.</summary>
    private const uint PatchDudeVtable = 0x00508cc8u;

    /// <summary>실패 없이 통과하는 CD Dude 가상 표다.</summary>
    private const uint CdDudeVtable = 0x005072f0u;

    /// <summary>최초 등록 때 raw WORD만 쓰는 idle 작업 번호다. Regular 생성과 구분한다.</summary>
    private const uint IdleTask = 0x5101u;

    /// <summary>생성 차단과 무관한 기본 Bridge 번호다. 실제 월드는 현재 DWORD를 따로 빌린다.</summary>
    private const uint InitialBridgeType = 82u;

    /// <summary>Dude 슬롯을 공유하는 풀이다. 새 Dude와 population SID가 같은 저장소를 쓴다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 TYPE+150의 I 첫 프레임 Run 표다. 객체보다 오래 살아야 한다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>생성 한도와 같은 mutable DWORD 목록이다.</summary>
    private readonly DudeSpawnState _state;

    /// <summary>실제 프레임·공통 후처리 연결이다.</summary>
    private readonly DudePostPopHooks _hooks;

    /// <summary>QA를 저장한 직후 읽는005411a0의 현재 DWORD다. 콜백 뒤 TYPE와 함께 판정한다.</summary>
    private readonly Func<uint>? _bridgeTypeReader;

    /// <summary>같은 최초 Run·population을 참조하며 CD의 기존 구성 사전 검사를 유지한다.</summary>
    /// <param name="pool">Dude 슬롯을 가진 풀.</param>
    /// <param name="frames">TYPE별 I Run 표.</param>
    /// <param name="state">생성 한도·차단·자산 번호 상태.</param>
    /// <param name="hooks">프레임·공통 후처리·표면 통지 경계.</param>
    /// <param name="bridgeTypeReader">현재 Bridge DWORD 읽기다. 없으면 초기값 82를 쓴다.</param>
    public RawDudePostPop(RawSidPool pool, IReadOnlyList<RiftTypeFrames> frames, DudeSpawnState state, DudePostPopHooks hooks, Func<uint>? bridgeTypeReader = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        _pool = pool;
        _frames = frames;
        _state = state;
        _hooks = hooks.Copy();
        _bridgeTypeReader = bridgeTypeReader;
    }

    /// <summary>현재 Bridge DWORD를 읽는다. 읽기 장치가 없으면 초기값 82를 쓴다.</summary>
    private uint ReadBridgeType() => _bridgeTypeReader?.Invoke() ?? InitialBridgeType;

    /// <summary>Dude 슬롯을 가진 풀인지 검사한다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>실제 가상 표 DWORD를 정렬·호스트 포인터 해석 없이 읽는다.</summary>
    private static uint ReadVtable(ReadOnlySpan<byte> raw)
    {
        uint value = 0;
        // 원본 DWORD의 낮은 바이트부터 합친다.
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)raw[index] << (index * 8);
        }

        return value;
    }

    /// <summary>원본 signed count·capacity의 정상 비음수 목록 계약을 확인한다.</summary>
    private static void ValidatePopulation(RawPostPopList population)
    {
        if (population.Entries.Length > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(population), "Dude population signed capacity 오류");
        }

        _ = population.Items();
    }

    /// <summary>현재 용량을 원본 signed 비교에 사용할 DWORD로 읽는다.</summary>
    private static uint ReadCapacity(RawPostPopList population)
    {
        uint capacity = population.Capacity;
        if (capacity > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(population), "Dude population DWORD 용량 물리 범위");
        }

        return capacity;
    }

    /// <summary>현재 TYPE·가상 표가 Dude 전용 몸체의 대상인지 가린다.</summary>
    /// <param name="sid">가릴 오브젝트 번호.</param>
    /// <returns>PATCH는 실제 가상 표로 분배하며 CD는 기존 Dude 타입·가상 표 계약을 유지한다.</returns>
    public bool Handles(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        return (_pool.Edition == OriginalEdition.Patch1078 || raw[RawSidPool.TypeOffset] == _state.DudeType)
            && ReadVtable(raw) == (_pool.Edition == OriginalEdition.Patch1078 ? PatchDudeVtable : CdDudeVtable);
    }

    /// <summary>최초 population·프레임 쓰기 뒤 필요한 공통 효과를 공간 변경 전에 검증한다.</summary>
    /// <param name="sid">검증할 오브젝트 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    public void Validate(int sid, uint flags)
    {
        _ = _pool.AllocatedBytes(sid);
        if (!Handles(sid) || _pool.Slot(sid)[RawSidPool.TypeOffset] != _state.DudeType)
        {
            throw new ArgumentException("Dude postPop 실제 타입·가상 표 오류", nameof(sid));
        }

        if ((flags & 1) != 0)
        {
            ValidatePopulation(_state.Population);
            uint type = _pool.Slot(sid)[RawSidPool.TypeOffset];
            if (type >= _frames.Count)
            {
                throw new ArgumentOutOfRangeException(nameof(sid), "Dude postPop 현재 Run 표 오류");
            }

            RiftTypeFrames.LetterRun first = _frames[(int)type].Run((byte)'I');
            if (first.First < 0 || first.First >= _frames[(int)type].Codes.Count)
            {
                throw new ArgumentOutOfRangeException(nameof(sid), "Dude postPop 실제 I 프레임 미연결");
            }

            if (_hooks.SetFrame is null)
            {
                throw new InvalidOperationException("Dude postPop 실제 프레임 쓰기 미연결");
            }
        }

        (_hooks.ValidateBase ?? throw new InvalidOperationException("Dude의 선택된 실제 하위 미연결"))(sid, flags);
    }

    /// <summary>처음만 idle·등록·I 프레임을 쓰고 이후 항상 공통 postPop을 호출한다.</summary>
    /// <param name="sid">후처리할 오브젝트 번호.</param>
    /// <param name="flags">호출 플래그. 최초 비트가 있으면 최초 효과를 쓴다.</param>
    public void PostPop(int sid, uint flags)
    {
        if (_pool.Edition == OriginalEdition.Patch1078)
        {
            if ((flags & 1) != 0)
            {
                Span<byte> raw = _pool.AllocatedBytes(sid);
                raw[12] = unchecked((byte)IdleTask);
                raw[13] = unchecked((byte)(IdleTask >> 8));
                if (_pool.Slot(sid)[RawSidPool.TypeOffset] == ReadBridgeType())
                {
                    (_hooks.NotifySurface ?? throw new InvalidOperationException("Dude의 선택된 실제 하위 미연결"))(sid);
                }

                // 원문의 cmp에 앞선 index는 보존하고 배열·개수는 도달 위치에서 다시 읽는다.
                uint used = _state.Population.Count;
                uint capacity = ReadCapacity(_state.Population);
                if (unchecked((int)used) < unchecked((int)capacity))
                {
                    if (unchecked((int)used) < 0)
                    {
                        throw new ArgumentOutOfRangeException(nameof(sid), "Dude population 음수 저장 위치");
                    }

                    Span<uint> storage = _state.Population.Storage;
                    if (used >= unchecked((uint)storage.Length))
                    {
                        throw new ArgumentOutOfRangeException(nameof(sid), "Dude population 실제 배열 저장 범위");
                    }

                    storage[checked((int)used)] = unchecked((uint)sid);
                    _state.Population.Count = unchecked(used + 1);
                }

                uint type = _pool.Slot(sid)[RawSidPool.TypeOffset];
                if (type >= _frames.Count)
                {
                    throw new ArgumentOutOfRangeException(nameof(sid), "Dude postPop 현재 Run 표 물리 범위");
                }

                // TYPE+150의 현재 값은 코드 개수로 사전 거부하지 않고 실제 SetFrame에 전달한다.
                int first = _frames[(int)type].Run((byte)'I').First;
                (_hooks.SetFrame ?? throw new InvalidOperationException("Dude의 선택된 실제 하위 미연결"))(sid, first, 0);
            }

            (_hooks.Base ?? throw new InvalidOperationException("Dude의 선택된 실제 하위 미연결"))(sid, flags);
            return;
        }

        Validate(sid, flags);
        if ((flags & 1) != 0)
        {
            Span<byte> raw = _pool.AllocatedBytes(sid);
            raw[12] = unchecked((byte)IdleTask);
            raw[13] = unchecked((byte)(IdleTask >> 8));
            uint used = _state.Population.Count;
            if (used < unchecked((uint)_state.Population.Entries.Length))
            {
                _state.Population.Entries[checked((int)used)] = unchecked((uint)sid);
                _state.Population.Count = unchecked(used + 1);
            }

            (_hooks.SetFrame ?? throw new InvalidOperationException("Dude의 선택된 실제 하위 미연결"))(sid, _frames[_pool.Slot(sid)[RawSidPool.TypeOffset]].Run((byte)'I').First, 0);
        }

        (_hooks.Base ?? throw new InvalidOperationException("Dude의 선택된 실제 하위 미연결"))(sid, flags);
    }
}
