// PATCH AI 피해 반응의 현재 목록과 조건부 추가·로그 순서를 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 피해 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.

// 실제004105b0 호출의 서식 인수다. 로그 출력 여부는 현재 brain+b00을 읽는 소비자가 결정한다.
public struct AiDamageLog
{
    /// <summary>false는 I hate, true는 I hate MORE 서식이다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    public bool More;

    /// <summary>이번 로그의 실제 공격자 또는 홈 SID다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    public Sid Attacker;

    /// <summary>좌표 절삭 뒤 현재 공격자의 TYPE 번호다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    public uint Type;

    /// <summary>원본 CRT가 0방향으로 절삭한 좌표다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    public int X;

    /// <summary>원본 CRT가 0방향으로 절삭한 좌표다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    public int Y;
}

// 실제 큰 하위 둘만 외부 경계로 받는다. 목록 판단은 본체가 수행한다.
public sealed class AiDamageHooks
{
    /// <summary>0045c100의 같은 Host/FlyerProcessFields::Home 조회다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    public Func<Sid, Sid>? Home { get; set; }

    /// <summary>004105b0(ai,29,format,typeName,x,y)의 실제 소비자다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    public Action<uint, AiDamageLog>? Log { get; set; }
}

// 같은 실제 AI 소유 블록/Pool/현재 TYPE에00415460을 적용한다.
// 근거: cpppj o/RawAiDamage.h/.cpp의 RawAiDamage다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.
// cpp의 RiftTypeRecord는 DN의 OriginalTypeRecord로 받는다(Flags2·Group만 쓴다).
public sealed class RawAiDamage
{
    /// <summary>실제 네 pointer/capacity/count 기록이다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    private const int GuardOffset = 0x360;

    /// <summary>실제 hate pointer/capacity/count 기록이다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    private const int HateOffset = 0x36c;

    /// <summary>실제 very pointer/capacity/count 기록이다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    private const int VeryOffset = 0x378;

    /// <summary>실제 tracked pointer/capacity/count 기록이다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    private const int TrackedOffset = 0x384;

    /// <summary>실제 본체와 네 목록 블록의 유일한 소유자다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>실제 피해/공격자/홈 raw 슬롯이다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>같은 현재 188개 TYPE다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>실제 홈/로그 하위다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    private readonly AiDamageHooks _hooks;

    /// <summary>metadata와 memory/pool은 처리기보다 오래 유지해야 한다. PATCH188만 받는다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    /// <param name="memory">같은 실제 소유 본체/목록 등록부다.</param>
    /// <param name="pool">같은 실제 월드 풀이다.</param>
    /// <param name="types">같은 현재 188개 TYPE다.</param>
    /// <param name="hooks">선택된 실제 홈/로그 효과다.</param>
    public RawAiDamage(RawAiBrainMemory memory, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, AiDamageHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188)
        {
            throw new ArgumentException("AI 피해는 PATCH188의 같은 실제 세계가 필요합니다");
        }

        _memory = memory;
        _pool = pool;
        _types = types;
        _hooks = hooks;
    }

    /// <summary>해제되었거나 크기가 다른 본체를 목록 주소로 해석하지 않는다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 살아 있는 실제 0xb08 본체다.</returns>
    private byte[] Brain(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 피해 실제0xb08 본체 누락");
        }

        return raw;
    }

    /// <summary>비정렬 little endian DWORD만 읽는다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int offset)
    {
        if ((uint)offset + 4 > (uint)raw.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "AI 피해 DWORD 범위 오류");
        }

        // 실제 네 BYTE만 낮은 자리부터 합친다.
        return BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(offset, 4));
    }

    /// <summary>주변 padding을 보존하는 실제 DWORD 쓰기다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value)
    {
        if ((uint)offset + 4 > (uint)raw.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "AI 피해 쓰기 범위 오류");
        }

        // 낮은 바이트부터 쓴다.
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);
    }

    /// <summary>정상 유한 좌표의 원본 signed64 절삭 뒤 낮은 DWORD를 보존한다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    /// <param name="value">절삭할 좌표다.</param>
    /// <returns>CRT 절삭 좌표다.</returns>
    private static int Coordinate(float value)
    {
        if (!float.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 피해 CRT 좌표 범위 오류");
        }

        // 0방향 절삭 뒤 64비트 중 낮은 32비트만 둔다.
        return unchecked((int)(long)value);
    }

    /// <summary>원본은 count가 0 이하이면 현재 목록을 앞에서부터 검색하지 않는다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="offset">목록 기록 위치다.</param>
    /// <param name="sid">찾을 SID다.</param>
    /// <returns>들어 있으면 참이다.</returns>
    private bool Contains(uint token, int offset, Sid sid)
    {
        byte[] raw = Brain(token);
        int count = unchecked((int)Read(raw, offset + 8));
        if (count <= 0)
        {
            return false;
        }

        byte[] list = _memory.Bytes(Read(raw, offset));
        if ((uint)count > (uint)list.Length / 4)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "AI 피해 목록 count 범위 오류");
        }

        // 원문 순서의 첫 중복에서 반환하는 반복문이다.
        for (int index = 0; index < count; index++)
        {
            if (Read(list, index * 4) == sid.Value)
            {
                return true;
            }
        }

        return false;
    }

    /// <summary>full이면 검색도 하지 않는 00414450 순서다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="offset">목록 기록 위치다.</param>
    /// <param name="sid">추가할 SID다.</param>
    private void Append(uint token, int offset, Sid sid)
    {
        byte[] raw = Brain(token);
        int count = unchecked((int)Read(raw, offset + 8));
        int capacity = unchecked((int)Read(raw, offset + 4));
        if (count >= capacity)
        {
            return;
        }

        if (count < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "AI 피해 음수 목록 count 오류");
        }

        if (Contains(token, offset, sid))
        {
            return;
        }

        byte[] list = _memory.Bytes(Read(raw, offset));
        Put(list, count * 4, sid.Value);
        Put(raw, offset + 8, unchecked((uint)count + 1u));
    }

    /// <summary>현재 TYPE의 genus를 읽는다. free/dead는 호출 지점의 별도 조건이다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    /// <param name="sid">조회할 SID다.</param>
    /// <returns>현재 TYPE의 flags2다.</returns>
    private uint Genus(Sid sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid.Value);
        if (raw[RawSidPool.TypeOffset] >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "AI 피해 현재 TYPE 오류");
        }

        return _types[raw[RawSidPool.TypeOffset]].Flags2;
    }

    /// <summary>홈의 범위/free/group3 조건이며 dead 비트는 거부하지 않는다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    /// <param name="sid">검사할 홈 SID다.</param>
    /// <returns>유효한 홈이면 참이다.</returns>
    private bool ValidHome(Sid sid)
    {
        if (sid.Value == 0 || sid.Value >= _pool.Capacity)
        {
            return false;
        }

        ReadOnlySpan<byte> raw = _pool.Slot(sid.Value);
        if ((raw[RawSidPool.StateOffset] & RawSidPool.FreeBit) != 0)
        {
            return false;
        }

        if (raw[RawSidPool.TypeOffset] >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "AI 피해 홈 TYPE 오류");
        }

        return _types[raw[RawSidPool.TypeOffset]].Group == 3;
    }

    /// <summary>좌표는 Y/X 순서로 절삭하고 TYPE는 그 뒤 현재 슬롯에서 조회한다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="sid">로그할 SID다.</param>
    /// <param name="more">MORE 서식 여부다.</param>
    private void Log(uint token, Sid sid, bool more)
    {
        if (_hooks.Log == null)
        {
            throw new InvalidOperationException("AI 피해 실제 로그004105b0 미연결");
        }

        ReadOnlySpan<byte> raw = _pool.Slot(sid.Value);
        float x = BitConverter.UInt32BitsToSingle(Read(raw, 14));
        float y = BitConverter.UInt32BitsToSingle(Read(raw, 18));
        int iy = Coordinate(y);
        int ix = Coordinate(x);
        _hooks.Log(token, new AiDamageLog { More = more, Attacker = sid, Type = _pool.Slot(sid.Value)[RawSidPool.TypeOffset], X = ix, Y = iy });
    }

    /// <summary>실제 hate의 full 자연 경로도 로그 요청을 남긴다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="sid">미워할 SID다.</param>
    private void Hate(uint token, Sid sid)
    {
        if (sid.Value == 0 || Contains(token, HateOffset, sid))
        {
            return;
        }

        Append(token, HateOffset, sid);
        Log(token, sid, false);
    }

    /// <summary>실제 Player+0xb0의 opaque32 token으로 같은 brain과 목록을 변경한다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="victim">피해를 본 SID다.</param>
    /// <param name="attacker">공격한 SID다.</param>
    public void Notice(uint token, Sid victim, Sid attacker)
    {
        if (attacker.Value == 0)
        {
            return;
        }

        Brain(token);
        uint genus = Genus(victim);
        if (_pool.Slot(victim.Value)[RawSidPool.TypeOffset] == 164)
        {
            genus |= 0x400000u;
        }

        bool hate = Contains(token, TrackedOffset, victim);
        bool very = hate;
        if ((genus & 0x80604200u) != 0)
        {
            hate = true;
            very = true;
            if ((genus & 0x80404200u) != 0 && !Contains(token, VeryOffset, attacker))
            {
                byte[] raw = Brain(token);
                Put(raw, 0x404, (Read(raw, 0x248) >> 1) & 1u);
            }
        }

        if ((genus & 0x4000000u) != 0 && Contains(token, GuardOffset, victim))
        {
            hate = true;
        }

        if (((genus & 0x30000u) != 0 && Contains(token, TrackedOffset, victim)) || hate)
        {
            Hate(token, attacker);
            if ((Genus(attacker) & 0x100000u) != 0)
            {
                if (_hooks.Home == null)
                {
                    throw new InvalidOperationException("AI 피해 실제 비행 홈0045c100 미연결");
                }

                Sid home = _hooks.Home(attacker);
                if (ValidHome(home))
                {
                    Hate(token, home);
                }
            }
        }

        if (very && !Contains(token, VeryOffset, attacker))
        {
            Append(token, VeryOffset, attacker);
            if ((Genus(attacker) & 0x100000u) != 0)
            {
                if (_hooks.Home == null)
                {
                    throw new InvalidOperationException("AI 피해 실제 very 비행 홈0045c100 미연결");
                }

                Sid home = _hooks.Home(attacker);
                if (ValidHome(home))
                {
                    Append(token, VeryOffset, home);
                }
            }

            Log(token, attacker, true);
        }
    }

    /// <summary>같은 실제 월드에 연결하는지 검사할 수 있는 풀 참조다. 2026-11-25 R4-5b 피해 반응 이식에서 추가됐다.</summary>
    /// <returns>이 몸체가 쓰는 슬롯 풀이다.</returns>
    public RawSidPool Pool => _pool;
}
