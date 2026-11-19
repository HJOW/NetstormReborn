// 원본004166e0/00414de0의 같은 Player→AI 생성·삭제 목록 갱신이다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 생성·삭제 통지 모듈을 담는 네임스페이스다. 2026-11-19 0.648에서 R4-5 AI 통지용으로 추가했다.

// 원본의 세 로그 요청을 실제 표시 장치에 전달하는 종류다.
public enum AiObjectLogKind
{
    /// <summary>건설 로그다. 0.648에서 추가했다.</summary>
    Built,

    /// <summary>간헐천 제외 성공 로그다. 0.648에서 추가했다.</summary>
    MadeGeyserIneligible,

    /// <summary>제외 후보 없음 로그다. 0.648에서 추가했다.</summary>
    CannotExcludeGeyser,
}

// 게임 계산을 끝낸 뒤 원본 로그 포맷에 필요한 현재 값을 전달한다.
public readonly record struct AiObjectLog(AiObjectLogKind Kind, uint Object, uint Type = 0u, int X = 0, int Y = 0);

// 같은 경로/발 기준점/로그 장치를 선택된 위치에서만 조회한다.
public sealed class AiObjectNoticeHooks
{
    /// <summary>현재 이동 프로세스 두 대상이며 없으면 0/0이다. 0.648에서 추가했다.</summary>
    public Func<int, uint[]>? CarrierTargets { get; set; }

    /// <summary>두 현재 발 중심 간 거리 제곱이다. 0.648에서 추가했다.</summary>
    public Func<int, uint, double>? DistanceSquared { get; set; }

    /// <summary>형식화/표시 경계다. 게임 타이머는 본문이 갱신한다. 0.648에서 추가했다.</summary>
    public Action<uint, AiObjectLog>? Log { get; set; }
}

/// <summary>실제 0xb08/17목록과 Player9를 공유하며 부착 bool 사본을 만들지 않는다.
/// 생성과 삭제는 같은 분류 본문을 사용하며 외부 효과 뒤 현재 목록/본체를 다시 읽는다.</summary>
/// <remarks>2026-11-19 0.648에서 추가했다. 근거: cpppj <c>o/RawAiObjectNotice.h/.cpp</c>의 <c>RawAiObjectNotice</c>이다.</remarks>
public sealed class RawAiObjectNotice
{
    /// <summary>보호 거리 제곱이다. 0.648에서 추가했다.</summary>
    private const double GuardDistanceSquared = 180.5;

    /// <summary>잃은 운반자 지연 시긱다. 0.648에서 추가했다.</summary>
    private const double LostCarrierDelay = 30.0;

    /// <summary>기존 생성자/행동과 같은 실제 본체·목록 소유자다. 0.648에서 추가했다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>현재 SID 타입/좌표를 읽는 실제 풀이다. 0.648에서 추가했다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>같은 현재 188 TYPE의 genus다. 0.648에서 추가했다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>같은 9×180바이트의 +b0를 호출마다 읽는다. 0.648에서 추가했다.</summary>
    private readonly byte[] _players;

    /// <summary>현재 게임 시각이다. 0.648에서 추가했다.</summary>
    private readonly Func<double> _now;

    /// <summary>현재 분류 제외 번호다. 0.648에서 추가했다.</summary>
    private readonly Func<uint> _outpostType;

    /// <summary>현재 값이며 Player 조회 전에 읽는다. 0.648에서 추가했다.</summary>
    private readonly Func<bool> _disabled;

    /// <summary>선택한 이동/거리/표시의 실제 하위다. 0.648에서 추가했다.</summary>
    private readonly AiObjectNoticeHooks _hooks;

    /// <summary>AI 생성·삭제 통지 본체를 만든다. 같은 PATCH 풀·TYPE·Player 기록 계약을 확인한다.</summary>
    public RawAiObjectNotice(RawAiBrainMemory memory, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types,
        byte[] players, Func<double> now, Func<uint> outpostType, Func<bool> disabled, AiObjectNoticeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(players);
        ArgumentNullException.ThrowIfNull(now);
        ArgumentNullException.ThrowIfNull(outpostType);
        ArgumentNullException.ThrowIfNull(disabled);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188 || players.Length != 1620)
        {
            throw new ArgumentException("AI 통지의 실제 PATCH188/Player9/현재 모드 누락", nameof(pool));
        }

        _memory = memory;
        _pool = pool;
        _types = types;
        _players = players;
        _now = now;
        _outpostType = outpostType;
        _disabled = disabled;
        _hooks = hooks;
    }

    /// <summary>004166e0의 현재 Player+b0 비영일 때만 본문을 호출한다. 0.648에서 추가했다.</summary>
    public void Notice(uint owner, int sid, bool added)
    {
        if (Need(_disabled)() || unchecked((int)owner) < 1)
        {
            return;
        }

        if (owner >= 9)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "AI 통지 Player9 범위");
        }

        uint token = BinaryPrimitives.ReadUInt32LittleEndian(_players.AsSpan((int)owner * 180 + 0xb0, 4));
        if (token != 0)
        {
            Update(token, sid, added);
        }
    }

    /// <summary>00414de0의 타입 수·목록·목표·타이머를 원문 순서로 갱신한다. 0.648에서 추가했다.</summary>
    public void Update(uint token, int sid, bool added)
    {
        byte[] raw = Body(token);
        int type = _pool.Slot(sid)[10];
        uint counter = unchecked(0x6d4u + unchecked((uint)type) * 4u);
        Put(raw, counter, unchecked(Read(raw, counter) + (added ? 1u : 0xffffffffu)));
        if (type >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "AI 통지 현재 TYPE 범위");
        }

        uint genus = _types[type].Flags2;
        if (!added)
        {
            if ((genus & 0x200000u) != 0)
            {
                if (Read(raw, 0x2a0) == unchecked((uint)sid))
                {
                    Put(raw, 0x2a0, 0);
                }

                if (Read(raw, 0x29c) == unchecked((uint)sid))
                {
                    Put(raw, 0x29c, 0);
                }
            }

            if ((genus & 0x30000u) != 0)
            {
                uint[] targets = Need(_hooks.CarrierTargets)(sid);
                if (targets[0] != 0)
                {
                    ExcludeGeyser(token, targets[0]);
                }

                if (Contains(token, 0x384, unchecked((uint)sid)))
                {
                    Remove(token, 0x384, unchecked((uint)sid));
                    Time(Body(token), 0x5e0, Need(_now)() + LostCarrierDelay);
                }
            }

            raw = Body(token);
            if ((genus & 0x200u) != 0 && Read(raw, 0x294) == unchecked((uint)sid))
            {
                Put(raw, 0x294, 0);
            }

            Remove(token, 0x36c, unchecked((uint)sid));
            Remove(token, 0x378, unchecked((uint)sid));
            raw = Body(token);
            if (Read(raw, 0x3f8) == unchecked((uint)sid))
            {
                Put(raw, 0x3f8, 0);
            }

            if (Read(raw, 0x3fc) == unchecked((uint)sid))
            {
                Put(raw, 0x3fc, 0);
            }
        }

        if ((genus & 0x400000u) != 0)
        {
            raw = Body(token);
            if (added)
            {
                Put(raw, 0x2a4, unchecked((uint)sid));
            }
            else if (Read(raw, 0x2a4) == unchecked((uint)sid))
            {
                Put(raw, 0x2a4, 0);
            }
        }

        if ((genus & 0x4000000u) != 0)
        {
            if (!added)
            {
                Remove(token, 0x360, unchecked((uint)sid));
            }
            else
            {
                bool nearby = false;
                uint temple = Read(Body(token), 0x294);
                if (temple != 0 && Need(_hooks.DistanceSquared)(sid, temple) < GuardDistanceSquared)
                {
                    nearby = true;
                }

                // 첫 근접 판정 뒤에도 원본처럼 현재 목록의 모든 거리 조회를 진행한다.
                for (int index = 0; index < Count(token, 0x300); index++)
                {
                    if (Need(_hooks.DistanceSquared)(sid, Item(token, 0x300, index)) < GuardDistanceSquared)
                    {
                        nearby = true;
                    }
                }

                if (nearby)
                {
                    AppendUnique(token, 0x360, unchecked((uint)sid));
                }
            }
        }

        if ((genus & 0x30000u) != 0 && (genus & 0x200000u) == 0)
        {
            if (added)
            {
                Append(token, 0x2dc, unchecked((uint)sid));
            }
            else
            {
                Remove(token, 0x2dc, unchecked((uint)sid));
            }

            Built(token, sid);
        }

        if ((genus & 0x800u) != 0)
        {
            if (added)
            {
                Append(token, 0x2f4, unchecked((uint)sid));
            }
            else
            {
                Remove(token, 0x2f4, unchecked((uint)sid));
            }

            Time(Body(token), 0x5b8, 0.0);
        }

        if ((genus & 0x4000u) != 0 && _pool.Slot(sid)[10] != Need(_outpostType)())
        {
            if (added)
            {
                Append(token, 0x300, unchecked((uint)sid));
            }
            else
            {
                Remove(token, 0x300, unchecked((uint)sid));
            }

            if (!added)
            {
                Remove(token, 0x30c, unchecked((uint)sid));
                raw = Body(token);
                Put(raw, 0x3e0, 0);
                Put(raw, 0x3e4, 6);
                Put(raw, 0x3f0, 1);
                Put(raw, 0x3f4, 0);
                Put(raw, 0x3fc, 0);
                Put(raw, 0x400, 0);
                Put(raw, 0x3ec, 0);
                Remove(token, 0x318, unchecked((uint)sid));
                // 남은 원소 시설을 원래 순서로 방문하며 대기 목록의 중복만 막는다.
                for (int index = 0; index < Count(token, 0x300); index++)
                {
                    uint item = Item(token, 0x300, index);
                    if (!Contains(token, 0x30c, item))
                    {
                        Append(token, 0x30c, item);
                    }
                }
            }
            else
            {
                Append(token, 0x30c, unchecked((uint)sid));
            }

            Put(Body(token), 0x2c8, Signed(Body(token), 0x3b0) <= Count(token, 0x300) ? 1u : 0u);
        }

        if ((genus & 0x40800u) != 0)
        {
            if (!added)
            {
                if (Contains(token, 0x2e8, unchecked((uint)sid)))
                {
                    Remove(token, 0x2e8, unchecked((uint)sid));
                }

                return;
            }

            if (Signed(Body(token), 0x2ec) <= Count(token, 0x2e8))
            {
                Remove(token, 0x2e8, Item(token, 0x2e8, 0));
            }

            Append(token, 0x2e8, unchecked((uint)sid));
            raw = Body(token);
            Put(raw, 0x3c8, Read(raw, 0x3c8) + 3u);
        }

        if (added && (genus & 0x4200u) != 0)
        {
            Time(Body(token), 0x5c0, 0.0);
        }
    }

    /// <summary>공통 Pop/삭제 처리기와의 풀 혼합을 거부할 때 사용한다. 0.648에서 추가했다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>실제 본체의 수명/크기를 검사한다. 0.648에서 추가했다.</summary>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 통지의 실제0xb08 본체 누락", nameof(token));
        }

        return raw;
    }

    /// <summary>signed count는 원본의 비양수 자연 루프를 보존한다. 0.648에서 추가했다.</summary>
    private int Count(uint token, uint offset) => Signed(Body(token), offset + 8);

    /// <summary>논리 count와 별도로 현재 실제 할당 안의 인덱스를 확인한다. 0.648에서 추가했다.</summary>
    private uint Item(uint token, uint offset, int index)
    {
        if (index < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(index), "AI 통지 음수 목록 인덱스");
        }

        return Read(_memory.Bytes(Read(Body(token), offset)), unchecked((uint)index) * 4u);
    }

    /// <summary>실제 목록의 첫 일치에서만 검색을 끝낸다. 0.648에서 추가했다.</summary>
    private bool Contains(uint token, uint offset, uint value)
    {
        // 외부에서 공유하는 count와 포인터를 각 방문 시 다시 읽는다.
        for (int index = 0; index < Count(token, offset); index++)
        {
            if (Item(token, offset, index) == value)
            {
                return true;
            }
        }

        return false;
    }

    /// <summary>일반 추가는 중복을 허용하고 용량이 찼으면 포인터를 조회하지 않는다. 0.648에서 추가했다.</summary>
    private void Append(uint token, uint offset, uint value)
    {
        byte[] raw = Body(token);
        int count = Count(token, offset);
        if (count >= Signed(raw, offset + 4))
        {
            return;
        }

        if (count < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(token), "AI 통지 음수 추가 인덱스");
        }

        Put(_memory.Bytes(Read(raw, offset)), unchecked((uint)count) * 4u, value);
        Put(raw, offset + 8, unchecked((uint)count) + 1u);
    }

    /// <summary>full 판정은 중복 검색보다 먼저 수행한다. 0.648에서 추가했다.</summary>
    private void AppendUnique(uint token, uint offset, uint value)
    {
        if (Count(token, offset) >= Signed(Body(token), offset + 4) || Contains(token, offset, value))
        {
            return;
        }

        Append(token, offset, value);
    }

    /// <summary>모든 중복만 압축하며 count 밖의 이전 꼬리를 0으로 지우지 않는다. 0.648에서 추가했다.</summary>
    private void Remove(uint token, uint offset, uint value)
    {
        int removed = 0;
        // 생존 항목의 상대 순서와 원본의 현재 count 재조회를 유지한다.
        for (int index = 0; index < Count(token, offset); index++)
        {
            uint item = Item(token, offset, index);
            if (item == value)
            {
                removed++;
            }
            else
            {
                Put(_memory.Bytes(Read(Body(token), offset)), unchecked((uint)(index - removed)) * 4u, item);
            }
        }

        Put(Body(token), offset + 8, unchecked((uint)Count(token, offset)) - unchecked((uint)removed));
    }

    /// <summary>제외 능력이 없으면 어떤 로그/목록도 선택하지 않는다. 0.648에서 추가했다.</summary>
    private void ExcludeGeyser(uint token, uint geyser)
    {
        if ((Read(Body(token), 0x248) & 8u) == 0)
        {
            return;
        }

        if (Count(token, 0x330) > 0)
        {
            Append(token, 0x33c, geyser);
            Remove(token, 0x330, geyser);
            Need(_hooks.Log)(token, new AiObjectLog(AiObjectLogKind.MadeGeyserIneligible, geyser));
        }
        else
        {
            Need(_hooks.Log)(token, new AiObjectLog(AiObjectLogKind.CannotExcludeGeyser, geyser));
        }
    }

    /// <summary>실제 00410620은 fadd QWORD now 뒤 QWORD에 저장한다. 0.648에서 추가했다.</summary>
    private void Built(uint token, int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        float x = BitConverter.UInt32BitsToSingle(Read(raw, 14));
        float y = BitConverter.UInt32BitsToSingle(Read(raw, 18));
        int iy = Truncate(y), ix = Truncate(x);
        int type = _pool.Slot(sid)[10];
        byte[] body = Body(token);
        double rate = BitConverter.UInt32BitsToSingle(Read(body, 0x228));
        Time(body, 0x5a8, rate + Need(_now)());
        Time(body, 0x5b0, rate + 1.0 + Need(_now)());
        Need(_hooks.Log)(token, new AiObjectLog(AiObjectLogKind.Built, unchecked((uint)sid), unchecked((uint)type), ix, iy));
    }

    /// <summary>원본 비정렬 DWORD를 현재 바이트에서 읽는다. 0.648에서 추가했다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, uint at) => BinaryPrimitives.ReadUInt32LittleEndian(bytes.Slice((int)at, 4));

    /// <summary>주변 바이트를 보존하고 지정 DWORD만 쓴다. 0.648에서 추가했다.</summary>
    private static void Put(Span<byte> bytes, uint at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.Slice((int)at, 4), value);

    /// <summary>목록 비교의 signed DWORD를 보존한다. 0.648에서 추가했다.</summary>
    private static int Signed(ReadOnlySpan<byte> bytes, uint at) => unchecked((int)Read(bytes, at));

    /// <summary>타이머는 float로 축소하지 않고 같은 QWORD에 쓴다. 0.648에서 추가했다.</summary>
    private static void Time(Span<byte> bytes, uint at, double value)
    {
        ulong bits = BitConverter.DoubleToUInt64Bits(value);
        Put(bytes, at, unchecked((uint)bits));
        Put(bytes, at + 4, unchecked((uint)(bits >> 32)));
    }

    /// <summary>로그 좌표의 CRT signed64 절삭 후 낮은 DWORD다. 0.648에서 추가했다.</summary>
    private static int Truncate(float value)
    {
        if (!float.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 통지 로그 좌표 변환 범위");
        }

        return unchecked((int)(long)value);
    }

    /// <summary>도달한 실제 하위가 없으면 그 위치에서 명시 오류로 남긴다. 0.648에서 추가했다.</summary>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 생성·삭제 통지의 실제 하위·전역 미연결");
}
