// PATCH 004923e0의 기본 기술/지역 사제 준비와 00492120/004921a0의 현재 목록 순서를 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// 일반 Fort 설치의 현재 전역과 실제 사제 목록만 빌리는 네임스페이스다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.

// PATCH 004923e0이 읽는 현재 전역을 담는 클래스다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class FortPriestPreparationGlobals
{
    /// <summary>원문의 미션 후 관문이다. 기술 생성 뒤와 지역 처리 전에 다시 읽는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public bool MissionPost;

    /// <summary>원문의 편집기 관문이다. 0이 아니면 지역 사제 정리를 건너뛴다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public uint Editor;

    /// <summary>네 번째 기술의 테마를 고를 때 쓰는 현재 구독자 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public uint Subscriber;

    /// <summary>같은 현재 헤더와 State+17 바이트를 읽는 템플릿 소유자다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public uint TemplateOwner;

    /// <summary>현재 플레이어 소유자와 지역 조회에 쓰는 소유자다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public uint PlayerOwner;
}

// Fort 사제 준비가 바깥에 요구하는 경계 묶음이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class FortPriestPreparationHooks
{
    /// <summary>앞 세 타입을 원문 시점에 읽는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Func<uint, uint>? BaseType { get; set; }

    /// <summary>구독자 테마의 네 번째 타입을 원문 시점에 읽는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Func<uint, uint>? ThemeType { get; set; }

    /// <summary>각 기술마다 0048fc60의 현재 부모를 다시 읽는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Func<uint>? Technology { get; set; }

    /// <summary>같은 부모의 마스크4·타입·인덱스0 첫 내용물이다. 0이면 없는 것으로 본다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Func<uint, uint, uint>? Contains { get; set; }

    /// <summary>실제 004afa70(parent, type, 0, 4, 0x10, 0)이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Action<uint, uint>? CreateTechnology { get; set; }

    /// <summary>같은 현재 헤더가 있는지 읽는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Func<uint, bool>? TemplatePresent { get; set; }

    /// <summary>같은 현재 헤더의 State 바이트를 읽는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Func<uint, byte[]>? State { get; set; }

    /// <summary>같은 실제 매니저의 20×6바이트 기록을 읽는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Action<uint, byte[]>? Territory { get; set; }

    /// <summary>현재 소유자와 지역 인덱스로 0046f380을 호출한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Func<uint, uint, int>? Island { get; set; }

    /// <summary>각 목록 항목의 실제 좌표를 0046f530에 전달한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Func<float, float, int>? Region { get; set; }

    /// <summary>실제 가상 74이며 목록·상태 부수 효과도 보존한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Action<uint, uint>? SetOwner { get; set; }

    /// <summary>실제 청크 범위에서 0040d900 뒤 0040e2b0의 사제 초기 배치다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Action<int>? Populate { get; set; }

    /// <summary>남은 수가 양수이면 번역·형식화한 전체 사제 수를 알린다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Action<uint>? TellPriests { get; set; }
}

// PATCH 004923e0의 기본 기술·지역 사제 준비를 복원한 클래스다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class RawFortPriestPreparation
{
    /// <summary>출력 목록의 고정 용량이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int EntryCapacity = 20;

    /// <summary>지역 기록 묶음의 바이트 수다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int TerritoryBytes = 120;

    /// <summary>지역 기록 한 개의 바이트 수다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int TerritoryStride = 6;

    /// <summary>처리할 지역 개수다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int TerritoryCount = 20;

    /// <summary>없는 섬을 뜻하는 지역 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int NoRegion = 127;

    /// <summary>같은 실제 목록을 읽는 슬롯 풀이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>같은 실제 풀의 등록·삭제가 공유하는 사제 목록이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly PriestPostPopState _priests;

    /// <summary>같은 실제 전역을 더 긴 소유자에서 빌린다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly FortPriestPreparationGlobals _globals;

    /// <summary>모드 사본이나 기본 성공 장치를 만들지 않는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly FortPriestPreparationHooks _hooks;

    /// <summary>
    /// 같은 실제 목록·풀·현재 전역을 더 긴 소유자에서 빌린다.
    /// </summary>
    /// <param name="pool">실제 풀이다.</param>
    /// <param name="priests">같은 실제 풀의 사제 목록이다.</param>
    /// <param name="globals">현재 전역이다.</param>
    /// <param name="hooks">외부 장치 묶음이다.</param>
    /// <exception cref="ArgumentException">Fort 사제 준비의 PATCH 10.78 계약 오류다.</exception>
    public RawFortPriestPreparation(RawSidPool pool, PriestPostPopState priests, FortPriestPreparationGlobals globals, FortPriestPreparationHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(priests);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("Fort 사제 준비의 PATCH 10.78 계약", nameof(pool));
        }

        _pool = pool;
        _priests = priests;
        _globals = globals;
        _hooks = hooks;
    }

    /// <summary>이 몸체가 쓰는 슬롯 풀이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 기술 4종 뒤 현재 관문을 다시 읽고 지역 20개를 원문 순서로 처리한다.
    /// </summary>
    public void Run()
    {
        if (!_globals.MissionPost)
        {
            // 타입은 먼저, 현재 기술 부모는 그 다음에 읽으며 네 번째만 현재 구독자를 쓰는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
            for (uint index = 0; index < 4; ++index)
            {
                uint type = index < 3 ? Need(_hooks.BaseType)(index) : Need(_hooks.ThemeType)(_globals.Subscriber % 3u);
                uint parent = Need(_hooks.Technology)();
                if (Need(_hooks.Contains)(parent, type) == 0u)
                {
                    Need(_hooks.CreateTechnology)(parent, type);
                }
            }
        }

        if (_globals.Editor != 0u || _globals.MissionPost)
        {
            return;
        }

        if (Need(_hooks.TemplatePresent)(_globals.TemplateOwner))
        {
            byte[] state = Need(_hooks.State)(_globals.TemplateOwner);
            if (state.Length < 18)
            {
                throw new ArgumentOutOfRangeException(nameof(state), "Fort 사제 준비 State+17 물리 범위");
            }

            if (state[17] != 0)
            {
                return;
            }
        }

        byte[] records = new byte[TerritoryBytes];
        Need(_hooks.Territory)(_globals.TemplateOwner, records);
        // 사본 지역 기록의 각 바이트1 비트0만 판정하고 조회 소유자는 그 시점의 현재값을 쓰는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (uint index = 0; index < (uint)TerritoryCount; ++index)
        {
            if ((records[(int)index * TerritoryStride + 1] & 1u) == 0u)
            {
                continue;
            }

            int region = Need(_hooks.Island)(_globals.PlayerOwner, index);
            if (region == NoRegion)
            {
                continue;
            }

            uint[] entries = new uint[EntryCapacity];
            uint count = 0u;
            Collect(region, entries, ref count);
            uint selected = Marked(entries.AsSpan(0, (int)count));
            if (selected == 0u && count == 0u)
            {
                Need(_hooks.Populate)(region);
            }
            else
            {
                if (selected == 0u)
                {
                    selected = entries[0];
                    Need(_hooks.SetOwner)(selected, _globals.PlayerOwner);
                }

                uint remaining = 0u;
                // 0040ea00처럼 모든 같은 번호를 안정적으로 제거하는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
                for (uint at = 0; at < count; ++at)
                {
                    if (entries[(int)at] != selected)
                    {
                        entries[(int)remaining++] = entries[(int)at];
                    }
                }

                count = remaining;
            }

            uint rest = count;
            // 가상 소유자가 원래 목록을 바꿔도 지역 임시 목록의 반복 수는 원문 지역값인 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
            for (uint at = 0; at < rest; ++at)
            {
                Need(_hooks.SetOwner)(entries[(int)at], 0u);
            }

            if (rest != 0u)
            {
                Need(_hooks.TellPriests)(rest);
            }
        }
    }

    /// <summary>
    /// 00492120의 현재 목록 재조회·유한 용량 계약이다.
    /// </summary>
    /// <param name="region">찾을 지역 번호다.</param>
    /// <param name="entries">모을 출력 배열이다.</param>
    /// <param name="count">모은 개수다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Fort 사제 준비 현재 목록 물리 범위다.</exception>
    public void Collect(int region, Span<uint> entries, ref uint count)
    {
        // 콜백이 바꾼 목록 개수와 다음 항목도 원문처럼 매번 다시 읽는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (uint index = 0; unchecked((int)index) < unchecked((int)_priests.GetPriests().Count); ++index)
        {
            RawPostPopList list = _priests.GetPriests();
            if (index >= (uint)list.Entries.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(entries), "Fort 사제 준비 현재 목록 물리 범위");
            }

            uint sid = list.Entries[(int)index];
            ReadOnlySpan<byte> raw = Raw(sid);
            float x = BitConverter.UInt32BitsToSingle(Word(raw, 14));
            float y = BitConverter.UInt32BitsToSingle(Word(raw, 18));
            if (Need(_hooks.Region)(x, y) == region && count < (uint)entries.Length)
            {
                entries[(int)count++] = sid;
            }
        }
    }

    /// <summary>
    /// 004921a0은 첫 실제 순서의 비트 0x80만 검사한다. 소유자·생존 필터를 추가하지 않는다.
    /// </summary>
    /// <param name="entries">검사할 목록이다.</param>
    /// <returns>표시된 첫 번호다. 없으면 0이다.</returns>
    public uint Marked(ReadOnlySpan<uint> entries)
    {
        // 실제 순서의 첫 비트 0x80을 반환하는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        foreach (uint sid in entries)
        {
            if ((Raw(sid)[12] & 0x80u) != 0u)
            {
                return sid;
            }
        }

        return 0u;
    }

    /// <summary>현재 DWORD 번호가 실제 풀의 물리 범위 안인지 접근 때만 검사한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="sid">조회할 번호다.</param>
    /// <returns>해당 슬롯의 읽기 전용 바이트다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">Fort 사제 준비의 실제 DWORD 번호 범위다.</exception>
    private ReadOnlySpan<byte> Raw(uint sid)
    {
        if (sid > ushort.MaxValue || sid >= (uint)_pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "Fort 사제 준비의 실제 DWORD 번호 범위");
        }

        return _pool.Slot((int)sid);
    }

    /// <summary>선택된 장치와 실제 리틀 엔디언 DWORD만 접근한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 슬롯이다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>조합한 DWORD 값이다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">Fort 사제 준비 DWORD 물리 범위다.</exception>
    private static uint Word(ReadOnlySpan<byte> bytes, int at)
    {
        if (at + 4 > bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "Fort 사제 준비 DWORD 물리 범위");
        }

        return BinaryPrimitives.ReadUInt32LittleEndian(bytes.Slice(at, 4));
    }

    /// <summary>도달한 미연결 장치만 거부하여 앞선 조회·생성의 부분 효과를 보존한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    /// <exception cref="InvalidOperationException">Fort 사제 준비의 선택 장치 미연결이다.</exception>
    private static T Need<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("Fort 사제 준비의 선택 장치 미연결");
        }

        return callback;
    }
}
