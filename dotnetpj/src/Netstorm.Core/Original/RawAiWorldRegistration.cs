// 원본 AI가 기존 월드와 간헐천을 같은 본체 목록에 등록하는 몸체다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 월드 등록 모듈을 담는 네임스페이스다. 2026-11-19 0.649에서 R4-5 AI 등록용으로 추가했다.

// 원본 중심점·로그·유효 표면 진단만 실제 세계에 요청한다.
public sealed class AiWorldRegistrationHooks
{
    /// <summary>현재 신전 발 중심이다. 자신의 신전 0일 때는 본체 268/26c를 쓴다. 0.649에서 추가했다.</summary>
    public Func<uint, float[]>? Center { get; set; }

    /// <summary>가까움 로그이며 본문이 후보를 추가하지 않은 뒤 호출한다. 0.649에서 추가했다.</summary>
    public Action<uint, uint>? FartherGeyser { get; set; }

    /// <summary>좌표/표면 0 진단이다. 반환하면 원본처럼 SID 0 분류를 계속한다. 0.649에서 추가했다.</summary>
    public Action<uint>? MissingSurface { get; set; }
}

/// <summary>00415720/004153c0의 실제 0xb08·Player9·방향 동맹·표면 지도다. 별도 관찰용 목록을 만들지 않는다.</summary>
/// <remarks>2026-11-19 0.649에서 추가했다. 근거: cpppj <c>o/RawAiWorldRegistration.h/.cpp</c>의 <c>RawAiWorldRegistration</c>이다.</remarks>
public sealed class RawAiWorldRegistration
{
    /// <summary>원본 0x4b0 임시 목록 용량이다. 0.649에서 추가했다.</summary>
    private const int DelayedLimit = 300;

    /// <summary>생성자/통지/전략과 공유하는 유일한 본체·목록 소유자다. 0.649에서 추가했다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>현재 live/free/type/좌표의 실제 풀이다. 0.649에서 추가했다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>같은 현재 188개 TYPE다. 0.649에서 추가했다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>같은 9×180바이트의 현재 신전 기록이다. 0.649에서 추가했다.</summary>
    private readonly byte[] _players;

    /// <summary>방향별 81 DWORD 동맹표다. 0.649에서 추가했다.</summary>
    private readonly uint[] _allies;

    /// <summary>같은 hash0 원문(65536 WORD)이다. 0.649에서 추가했다.</summary>
    private readonly ushort[] _surface;

    /// <summary>지연한 방어 자산도 같은 통지로 알린다. 0.649에서 추가했다.</summary>
    private readonly RawAiObjectNotice _objects;

    /// <summary>선택 위치의 실제 중심/장치/진단 경계다. 0.649에서 추가했다.</summary>
    private readonly AiWorldRegistrationHooks _hooks;

    /// <summary>AI 월드 등록 본체를 만든다. 같은 Pool·본체·현재 65536 WORD를 빌린다.</summary>
    public RawAiWorldRegistration(RawAiBrainMemory memory, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types,
        byte[] players, uint[] allies, ushort[] surface, RawAiObjectNotice objects, AiWorldRegistrationHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(players);
        ArgumentNullException.ThrowIfNull(allies);
        ArgumentNullException.ThrowIfNull(surface);
        ArgumentNullException.ThrowIfNull(objects);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || !ReferenceEquals(objects.Pool, pool)
            || types.Count != 188 || players.Length != 1620 || allies.Length != 81 || surface.Length != 65536)
        {
            throw new ArgumentException("AI 월드 등록의 같은 PATCH188/Player9/동맹81/표면65536 누락", nameof(pool));
        }

        _memory = memory;
        _pool = pool;
        _types = types;
        _players = players;
        _allies = allies;
        _surface = surface;
        _objects = objects;
        _hooks = hooks;
    }

    /// <summary>00415720: SID2부터 현재 live 항목→방어 자산 최대 300개 지연→마지막 3c8=3이다. 25c는 caller가 쓴다. 0.649에서 추가했다.</summary>
    public void FullScan(uint token)
    {
        Body(token);
        uint temporary = _memory.Allocate(DelayedLimit * 4);
        uint count = 0;
        int end = _pool.Capacity;
        int cursor = 1;
        try
        {
            // 0040eb40의 최초 호출도 1을 먼저 증가시키므로 SID1은 방문하지 않는다.
            Advance();
            // 하위 로그 뒤 현재 풀/owner/type을 다시 읽되 iterator 끝은 최초 cap을 유지한다.
            while (cursor >= 1 && cursor < _pool.Capacity)
            {
                int sid = cursor;
                ReadOnlySpan<byte> raw = _pool.Slot(sid);
                if (raw[10] >= 70 && (Genus(sid) & 2u) == 0)
                {
                    uint genus = Genus(sid);
                    if ((genus & 0x10000000u) != 0)
                    {
                        Geyser(token, unchecked((uint)sid), 1);
                    }
                    else if (raw[34] == Read(Body(token), 0x260))
                    {
                        if ((genus & 0x4000000u) == 0)
                        {
                            _objects.Update(token, sid, true);
                        }
                        else if (count < DelayedLimit)
                        {
                            Put(_memory.Bytes(temporary), count * 4u, unchecked((uint)sid));
                            count++;
                        }
                    }
                }

                Advance();
            }

            // 방어용 300개 제한을 넘은 항목은 통지하지 않으며 SID 순서를 유지한다.
            for (uint index = 0; index < count; index++)
            {
                _objects.Update(token, unchecked((int)Read(_memory.Bytes(temporary), index * 4u)), true);
            }

            Put(Body(token), 0x3c8, 3);
        }
        catch
        {
            _memory.Free(temporary);
            throw;
        }

        _memory.Free(temporary);

        // 원본은 free bit만 건너뛴다. dead는 제외하지 않는다.
        void Advance()
        {
            cursor++;
            while (cursor < end && (_pool.Slot(cursor)[11] & 1u) != 0)
            {
                cursor++;
            }
        }
    }

    /// <summary>004153c0: 0은 다섯 목록 제거, 비영은 가까움/표면 소유에 따라 분류한다. 0.649에서 추가했다.</summary>
    public void Geyser(uint token, uint sid, uint state)
    {
        Body(token);
        if (state == 0)
        {
            // 반복문은 원본 다섯 목록의 순서대로 모든 중복을 제거한다.
            foreach (uint at in new uint[] { 0x324u, 0x330u, 0x33cu, 0x348u, 0x354u })
            {
                Remove(token, at, sid);
            }

            return;
        }

        if (CloserToAlly(token, sid))
        {
            Need(_hooks.FartherGeyser)(token, sid);
        }
        else
        {
            Append(token, 0x324, sid);
            ClassifySurface(token, sid);
        }
    }

    /// <summary>004152b0의 엄격한 거리 비교이며 self 거리만 float에 저장한다. 0.649에서 추가했다.</summary>
    public bool CloserToAlly(uint token, uint number)
    {
        int sid = PhysicalSid(number);
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        float[] point = [BitConverter.UInt32BitsToSingle(Read(raw, 14)), BitConverter.UInt32BitsToSingle(Read(raw, 18))];
        uint temple = Read(Body(token), 0x294);
        float[] own = temple != 0
            ? Need(_hooks.Center)(temple)
            : [BitConverter.UInt32BitsToSingle(Read(Body(token), 0x268)), BitConverter.UInt32BitsToSingle(Read(Body(token), 0x26c))];
        // 상대 거리에는 float 축소를 넣는다.
        float ownDistance = (float)Distance(point, own);
        // Player0은 방문하지 않고 현재 owner행의 방향 동맹과 현재 Player+7c만 선택한다.
        for (uint candidate = 1; candidate < 9; candidate++)
        {
            uint owner = Read(Body(token), 0x260);
            if (owner >= 9)
            {
                throw new ArgumentOutOfRangeException(nameof(token), "AI 간헐천 현재 동맹 owner 범위");
            }

            if (candidate == owner || _allies[owner * 9 + candidate] == 0)
            {
                continue;
            }

            uint other = Read(_players, candidate * 180 + 0x7c);
            if (other != 0 && Distance(point, Need(_hooks.Center)(other)) < ownDistance)
            {
                return true;
            }
        }

        return false;
    }

    /// <summary>후속 실제 월드 조립 확인용 동일 풀이다. 0.649에서 추가했다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>해제 포인터나 크기 없는 가짜 본체를 성공으로 대신하지 않는다. 0.649에서 추가했다.</summary>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 월드 등록 실제0xb08 본체 누락", nameof(token));
        }

        return raw;
    }

    /// <summary>슬롯 0은 진단 뒤의 원본 분류에도 사용하므로 임의 금지하지 않는다. 0.649에서 추가했다.</summary>
    private int PhysicalSid(uint value)
    {
        if (value >= (uint)_pool.Capacity || value > ushort.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 월드 등록 SID 물리 범위");
        }

        return unchecked((int)value);
    }

    /// <summary>현재 TYPE의 순수 flags2다. 추가 분류 비트를 합성하지 않는다. 0.649에서 추가했다.</summary>
    private uint Genus(int sid)
    {
        int type = _pool.Slot(sid)[10];
        if (type >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "AI 월드 등록 현재 TYPE 범위");
        }

        return _types[type].Flags2;
    }

    /// <summary>현재 signed 목록 count다. 0.649에서 추가했다.</summary>
    private int Count(uint token, uint at) => unchecked((int)Read(Body(token), at + 8));

    /// <summary>용량이 남을 때만 실제 현재 포인터로 쓴다. 0.649에서 추가했다.</summary>
    private void Append(uint token, uint at, uint sid)
    {
        byte[] raw = Body(token);
        int count = Count(token, at);
        if (count >= unchecked((int)Read(raw, at + 4)))
        {
            return;
        }

        if (count < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(token), "AI 월드 등록 음수 목록 인덱스");
        }

        Put(_memory.Bytes(Read(raw, at)), unchecked((uint)count) * 4u, sid);
        Put(raw, at + 8, unchecked((uint)count) + 1u);
    }

    /// <summary>모든 중복을 안정 압축하고 inactive 꼬리를 보존한다. 0.649에서 추가했다.</summary>
    private void Remove(uint token, uint at, uint sid)
    {
        int removed = 0;
        // 현재 count와 실제 포인터를 원본처럼 재조회하며 생존 항목의 순서를 유지한다.
        for (int index = 0; index < Count(token, at); index++)
        {
            byte[] list = _memory.Bytes(Read(Body(token), at));
            uint value = Read(list, unchecked((uint)index) * 4u);
            if (value == sid)
            {
                removed++;
            }
            else
            {
                Put(list, unchecked((uint)(index - removed)) * 4u, value);
            }
        }

        Put(Body(token), at + 8, unchecked((uint)Count(token, at)) - unchecked((uint)removed));
    }

    /// <summary>표면 0 진단 뒤 원본처럼 현재 0슬롯을 분류한다. own 표면만 330, 섬/기타는 348이다. 0.649에서 추가했다.</summary>
    private void ClassifySurface(uint token, uint number)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(PhysicalSid(number));
        int x = Truncate(BitConverter.UInt32BitsToSingle(Read(raw, 14)));
        int y = Truncate(BitConverter.UInt32BitsToSingle(Read(raw, 18)));
        uint surface = 0;
        if (x >= 0 && y >= 0 && x < 256 && y < 256)
        {
            surface = _surface[y * 256 + x];
        }

        if (surface == 0)
        {
            Need(_hooks.MissingSurface)(number);
        }

        int sid = PhysicalSid(surface);
        // own surface만 330, 섬/기타는 348이다. 0슬롯도 분류한다.
        if ((Genus(sid) & 0x1000002u) == 2u)
        {
            int owner = _pool.Slot(sid)[34];
            if (owner == 0 || owner == Read(Body(token), 0x260))
            {
                Append(token, 0x330, number);
            }
        }
        else
        {
            Append(token, 0x348, number);
        }
    }

    /// <summary>원본 비정렬 DWORD를 지정 범위에서 읽는다. 0.649에서 추가했다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, uint at) => BinaryPrimitives.ReadUInt32LittleEndian(bytes.Slice((int)at, 4));

    /// <summary>현재 DWORD의 네 바이트만 바꾸며 inactive/padding을 유지한다. 0.649에서 추가했다.</summary>
    private static void Put(Span<byte> bytes, uint at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.Slice((int)at, 4), value);

    /// <summary>원본 x87의 두 float 좌표 차이는 중간 float 저장 없이 계산한다. 0.649에서 추가했다.</summary>
    private static double Distance(float[] first, float[] second)
    {
        double dx = first[0] - second[0], dy = first[1] - second[1];
        return dx * dx + dy * dy;
    }

    /// <summary>원본 CRT signed64 절삭의 낮은 DWORD다. 주소 계산 불가능한 비유한 입력은 호스트 계약 오류다. 0.649에서 추가했다.</summary>
    private static int Truncate(float value)
    {
        if (!float.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 간헐천 CRT 좌표 범위 오류");
        }

        return unchecked((int)(long)value);
    }

    /// <summary>선택되지 않은 하위를 미리 요구하지 않고 도달한 경계에서만 거부한다. 0.649에서 추가했다.</summary>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 월드 등록 실제 하위 미연결");
}
