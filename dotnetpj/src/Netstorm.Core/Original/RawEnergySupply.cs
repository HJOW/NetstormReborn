// Mana.cpp의 실제 공급원 수집·요구 문자 대응·배치 허용값을 단일 월드에 적용한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// 004fcb80의 push1e로 생성되는 실제 lastSource 용량이다.
public static class RawEnergySupplyLimits
{
    /// <summary>실제 lastSource 용량이다.</summary>
    public const int SourceCapacity = 30;
    /// <summary>실제 공급 장부 용량이다.</summary>
    public const int ProviderCapacity = 256;
    /// <summary>정렬이 다시 보관하는 최대 공급원 수다.</summary>
    public const int SortKept = 10;
}

/// <summary>커서 반경과 별개인 제곱 반경 및 현재 Mana 전역이다.</summary>
public sealed class EnergySupplyState
{
    /// <summary>공급 가상+a0가 매 호출 읽는 현재 제곱 반경이다.</summary>
    public float RadiusSquared { get; set; } = 484f;
    /// <summary>실제 편집 저장소의 현재 DWORD다.</summary>
    public uint Editor { get; set; }
    /// <summary>실제 특수 저장소의 현재 DWORD다.</summary>
    public uint Special { get; set; }
    /// <summary>실제 미션 저장소의 현재 DWORD다.</summary>
    public uint Mission { get; set; }
    /// <summary>005cab30의 실제 userCheat DWORD다. 명령/Session/모드 교체로 초기화하지 않는다.</summary>
    public uint UserCheat { get; set; }
    /// <summary>실제 무시 DWORD다.</summary>
    public uint Ignore { get; set; }
    /// <summary>실제 동맹 사용 DWORD다.</summary>
    public uint Allied { get; set; } = 1;
}

/// <summary>Session이 이미 소유한 전역은 해당 읽기 위치에서 조회한다. 독립 원문 검사는 값형 상태를 사용한다.</summary>
public sealed class EnergySupplyReads
{
    /// <summary>실제 편집 저장소의 현재 DWORD다.</summary>
    public Func<uint>? Editor { get; set; }
    /// <summary>실제 특수 저장소의 현재 DWORD다.</summary>
    public Func<uint>? Special { get; set; }
    /// <summary>실제 미션 저장소의 현재 DWORD다.</summary>
    public Func<uint>? Mission { get; set; }
}

/// <summary>474970의 원본 진단 두 위치다. 반환하는 진단 소비자는 원문처럼 이후 검사를 계속한다.</summary>
public enum EnergySortDiagnostic
{
    /// <summary>유효하지 않은 공급원이다.</summary>
    InvalidSource,
    /// <summary>유효하지 않은 최적 인덱스다.</summary>
    InvalidBestIndex,
}

/// <summary>+a0의 정확한 가상 표 분배와 TYPE+a0 요구 문자열만 외부에서 공급한다.</summary>
public sealed class EnergySupplyHooks
{
    /// <summary>현재 가상 표의 실제+a0 주소다. 알 수 없는 표는 거부한다.</summary>
    public Func<uint, uint>? ProviderEntry { get; set; }
    /// <summary>현재 TYPE와 보호 AST로 만든 실제 mana 바이트다.</summary>
    public Func<uint, string>? Requirement { get; set; }
    /// <summary>원문 source/bestIndex 진단이다. 미연결이면 예외로 거부한다.</summary>
    public Action<EnergySortDiagnostic>? SortDiagnostic { get; set; }
}

/// <summary>4730c0/473330/4734d0/473570 및 실제 두 공급 가상 몸체를 구현한다.</summary>
public sealed class RawEnergySupply
{
    /// <summary>00540ad8의 실제 문자표이며 끝 NUL도 인덱스11로 읽을 수 있다.</summary>
    private const string Letters = "wrtsWRTS.?>";

    /// <summary>보호 PE83개 가상 표의+a0 주소다. decomp_energy_supply.py가 현재 원본과 다시 확인한다.</summary>
    private static readonly uint[][] ProviderEntries =
    [
        new uint[] { 0x00501dd0U, 0x0041c0f0U }, new uint[] { 0x00501ea0U, 0x0041c170U }, new uint[] { 0x00502370U, 0x0041c0f0U }, new uint[] { 0x00502448U, 0x0041c0f0U },
        new uint[] { 0x00502510U, 0x0041c0f0U }, new uint[] { 0x005025d8U, 0x0041c0f0U }, new uint[] { 0x005026a0U, 0x0041c0f0U }, new uint[] { 0x00502778U, 0x0041c0f0U },
        new uint[] { 0x00502840U, 0x0041c0f0U }, new uint[] { 0x00502908U, 0x0041c0f0U }, new uint[] { 0x005029d0U, 0x0041c0f0U }, new uint[] { 0x00502aa8U, 0x0041c0f0U },
        new uint[] { 0x00502b70U, 0x0041c0f0U }, new uint[] { 0x00502c38U, 0x0041c0f0U }, new uint[] { 0x00502cf0U, 0x0041c0f0U }, new uint[] { 0x00502db0U, 0x0041c0f0U },
        new uint[] { 0x00502e68U, 0x0041c0f0U }, new uint[] { 0x00502f30U, 0x0041c0f0U }, new uint[] { 0x00502ff8U, 0x0041c0f0U }, new uint[] { 0x005030c0U, 0x0041c0f0U },
        new uint[] { 0x00503188U, 0x0041c0f0U }, new uint[] { 0x00503360U, 0x0041c0f0U }, new uint[] { 0x005034c8U, 0x0041c0f0U }, new uint[] { 0x00503d80U, 0x0041c0f0U },
        new uint[] { 0x00503e50U, 0x0041c0f0U }, new uint[] { 0x00503f20U, 0x0041c0f0U }, new uint[] { 0x00503ff0U, 0x0041c0f0U }, new uint[] { 0x005040c0U, 0x0041c0f0U },
        new uint[] { 0x00504198U, 0x0041c0f0U }, new uint[] { 0x00506d98U, 0x0041c0f0U }, new uint[] { 0x00506e50U, 0x0041c0f0U }, new uint[] { 0x00506f08U, 0x0041c0f0U },
        new uint[] { 0x005070c0U, 0x0041c0f0U }, new uint[] { 0x005075d8U, 0x0041c0f0U }, new uint[] { 0x005076b8U, 0x0041c0f0U }, new uint[] { 0x00507770U, 0x0041c0f0U },
        new uint[] { 0x00507950U, 0x0041c0f0U }, new uint[] { 0x00508cc8U, 0x0041c0f0U }, new uint[] { 0x00509148U, 0x0041c0f0U }, new uint[] { 0x00509210U, 0x0041c0f0U },
        new uint[] { 0x005092d0U, 0x0041c0f0U }, new uint[] { 0x00509388U, 0x0041c0f0U }, new uint[] { 0x00509470U, 0x0041c0f0U }, new uint[] { 0x005096a0U, 0x0041c0f0U },
        new uint[] { 0x00509758U, 0x0041c0f0U }, new uint[] { 0x00509810U, 0x0041c0f0U }, new uint[] { 0x00509930U, 0x0041c0f0U }, new uint[] { 0x005099e8U, 0x0041c0f0U },
        new uint[] { 0x00509aa0U, 0x0041c0f0U }, new uint[] { 0x00509b98U, 0x0041c0f0U }, new uint[] { 0x0050a050U, 0x0041c0f0U }, new uint[] { 0x0050a1e8U, 0x0041c0f0U },
        new uint[] { 0x0050a388U, 0x0041c0f0U }, new uint[] { 0x0050a748U, 0x0041c0f0U }, new uint[] { 0x0050a820U, 0x0041c0f0U }, new uint[] { 0x0050a8e8U, 0x0041c0f0U },
        new uint[] { 0x0050b570U, 0x0041c0f0U }, new uint[] { 0x0050b6e8U, 0x0041c0f0U }, new uint[] { 0x0050b7c0U, 0x0041c0f0U }, new uint[] { 0x0050b888U, 0x0041c0f0U },
        new uint[] { 0x0050b9b8U, 0x0041c0f0U }, new uint[] { 0x0050bad0U, 0x0041c0f0U }, new uint[] { 0x0050c3b0U, 0x0041c0f0U }, new uint[] { 0x0050c9f0U, 0x0041c0f0U },
        new uint[] { 0x0050cc08U, 0x0041c0f0U }, new uint[] { 0x0050da88U, 0x0041c0f0U }, new uint[] { 0x0050db78U, 0x0041c0f0U }, new uint[] { 0x0050dc30U, 0x0041c0f0U },
        new uint[] { 0x0050e568U, 0x0041c0f0U }, new uint[] { 0x0050e668U, 0x0041c0f0U }, new uint[] { 0x0050e738U, 0x0041c0f0U }, new uint[] { 0x0050e888U, 0x0041c0f0U },
        new uint[] { 0x0050e940U, 0x0041c0f0U }, new uint[] { 0x0050ea08U, 0x0041c0f0U }, new uint[] { 0x0050ead0U, 0x0041c0f0U }, new uint[] { 0x0050eb98U, 0x0041c0f0U },
        new uint[] { 0x0050ee08U, 0x0041c0f0U }, new uint[] { 0x0050f210U, 0x0041c0f0U }, new uint[] { 0x0050f540U, 0x0041c0f0U }, new uint[] { 0x005196f0U, 0x0041c170U },
        new uint[] { 0x005197a8U, 0x0041c170U }, new uint[] { 0x00519860U, 0x0041c170U }, new uint[] { 0x00519918U, 0x0041c170U },
    ];

    /// <summary>동일 함수 주소를 공유하는 가상 표만 현재 원문 공급 몸체에 연결한다.</summary>
    /// <param name="vtable">조회할 가상 표 주소다.</param>
    /// <returns>연결된 +a0 주소다.</returns>
    public static uint EnergyProviderEntry(uint vtable)
    {
        foreach (uint[] entry in ProviderEntries) if (entry[0] == vtable) return entry[1];
        throw new InvalidOperationException("에너지공급의현재가상표미복원");
    }

    /// <summary>원문 표 밖을 호스트 메모리로 해석하지 않는다.</summary>
    /// <param name="theme">원소 번호다.</param>
    /// <returns>대응 문자다.</returns>
    private static char Letter(int theme)
    {
        if (theme < 0 || theme > Letters.Length) throw new ArgumentOutOfRangeException(nameof(theme), "에너지원소문자표범위오류");
        return theme == Letters.Length ? '\0' : Letters[theme];
    }

    /// <summary>raw float를 비정렬 little endian DWORD에서 읽는다.</summary>
    /// <param name="raw">원문 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>단정도 좌표다.</returns>
    private static float Coordinate(ReadOnlySpan<byte> raw, int offset) =>
        BitConverter.ToSingle(raw.Slice(offset, 4));

    /// <summary>원문 C 문자열은 첫 NUL에서 끝난다.</summary>
    /// <param name="text">자를 문자열이다.</param>
    /// <returns>NUL 앞부분이다.</returns>
    private static string CString(string text)
    {
        int nul = text.IndexOf('\0');
        return nul < 0 ? text : text.Substring(0, nul);
    }

    /// <summary>같은 현재 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>같은 현재188TYPE다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>postPop/삭제가 갱신하는 현재 공급자 목록이다.</summary>
    private readonly RawPostPopList _providers;
    /// <summary>커서/생산/Builder의 마지막 선택 목록이다.</summary>
    private readonly RawPostPopList _selected;
    /// <summary>현재 방향별9×9 DWORD 표다.</summary>
    private readonly IReadOnlyList<uint> _alliances;
    /// <summary>마나/사정거리와 같은 현재 정수 경계다.</summary>
    private readonly int _mapLimit;
    /// <summary>공급 판정의 현재 원문 전역이다.</summary>
    private readonly EnergySupplyState _state;
    /// <summary>TYPE 문자열/검증한 가상 진입점의 실제 조회다.</summary>
    private readonly EnergySupplyHooks _hooks;
    /// <summary>이미 Session이 소유한 현재 전역 조회다.</summary>
    private readonly EnergySupplyReads _reads;

    /// <summary>원본은 PATCH188TYPE와9×9 동맹표를 사용한다. 공급/선택 count는 사용 시 현재 값으로 검증한다.</summary>
    /// <param name="pool">같은 현재 풀이다.</param>
    /// <param name="types">같은 현재188TYPE다.</param>
    /// <param name="providers">postPop/삭제가 갱신하는 현재 공급자 목록이다.</param>
    /// <param name="selected">커서/생산/Builder의 마지막 선택 목록이다.</param>
    /// <param name="alliances">현재 방향별9×9 DWORD 표다.</param>
    /// <param name="mapLimit">마나/사정거리와 같은 현재 정수 경계다.</param>
    /// <param name="state">공급 판정의 현재 원문 전역이다.</param>
    /// <param name="hooks">TYPE 문자열/검증한 가상 진입점의 실제 조회다.</param>
    /// <param name="reads">이미 Session이 소유한 현재 전역 조회다.</param>
    /// <remarks>2026-11-26 1.054에서 SL-3a 전체 EnergySupply용으로 추가했다. 근거: cpppj o/RawEnergySupply.h·cpp다. 선택 목록·동맹·반경의 세션 결선은 같은 단위에서 잇는다.</remarks>
    public RawEnergySupply(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawPostPopList providers,
        RawPostPopList selected, IReadOnlyList<uint> alliances, int mapLimit,
        EnergySupplyState state, EnergySupplyHooks hooks, EnergySupplyReads? reads = null)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(providers);
        ArgumentNullException.ThrowIfNull(selected); ArgumentNullException.ThrowIfNull(alliances); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != 188 || alliances.Count != 81 || hooks.ProviderEntry is null || hooks.Requirement is null)
            throw new ArgumentException("에너지실제월드TYPE조회계약오류");
        _pool = pool; _types = types; _providers = providers; _selected = selected; _alliances = alliances; _mapLimit = mapLimit; _state = state; _hooks = hooks; _reads = reads ?? new();
    }

    /// <summary>가상 효과가 바꿀 수 있는 전역은 각 원문 읽기 위치에서 조회한다.</summary>
    /// <param name="fallback">연결되지 않았을 때의 값이다.</param>
    /// <param name="current">연결된 현재 전역 조회다.</param>
    /// <returns>연결됐으면 현재값, 아니면 폴백이다.</returns>
    private static uint Read(uint fallback, Func<uint>? current) => current is not null ? current() : fallback;

    /// <summary>이전 호출의 우회 결과를 저장하지 않는다.</summary>
    /// <returns>공급 검사를 우회하면 참이다.</returns>
    private bool Override() => (Read(_state.Special, _reads.Special) != 0 || (Read(_state.Mission, _reads.Mission) != 0 && _state.UserCheat != 0)) && _state.Ignore != 0;

    /// <summary>실제 TYPE를 다른 번호/그림 블록으로 치환하지 않는다.</summary>
    /// <param name="type">조회할 번호다.</param>
    /// <returns>같은 번호의 기록이다.</returns>
    private OriginalTypeRecord Type(uint type) => type < _types.Count ? _types[(int)type] : throw new ArgumentOutOfRangeException(nameof(type), "에너지TYPE범위오류");

    /// <summary>예약 슬롯도 원문처럼 읽을 수 있으나 SID0/풀 밖은 거부한다.</summary>
    /// <param name="sid">읽을 번호다.</param>
    /// <returns>원문 바이트다.</returns>
    private ReadOnlySpan<byte> Raw(int sid) => sid > 0 && sid < _pool.Capacity ? _pool.Slot(sid) : throw new ArgumentOutOfRangeException(nameof(sid), "에너지공급SID범위오류");

    /// <summary>40e880/40e8f0은 들어온 모서리만 클램프한 뒤 현재 양 끝점의 유효 여부로 min/max를 선택한다.</summary>
    /// <param name="number">배치할 TYPE 번호다.</param>
    /// <param name="x">배치 X다.</param>
    /// <param name="y">배치 Y다.</param>
    /// <returns>왼쪽·위·오른쪽·아래다.</returns>
    private float[] Bounds(uint number, float x, float y)
    {
        OriginalTypeRecord type = Type(number);
        float lowX = (float)((double)x - (type.FootX - 1)), lowY = (float)((double)y - (type.FootY - 1));
        float limit = _mapLimit;
        lowX = Math.Clamp(lowX, 1f, 255f); lowY = Math.Clamp(lowY, 1f, 255f);
        bool valid = x > 0 && y > 0 && x < limit && y < limit;
        return valid ? [Math.Min(x, lowX), Math.Min(y, lowY), Math.Max(x, lowX), Math.Max(y, lowY)] : [lowX, lowY, lowX, lowY];
    }

    /// <summary>41c0f0/41c170은 base는 0, 공급형은 실제 중심과 대상 크기 항을 더한 엄격한 거리 비교다.</summary>
    /// <param name="provider">검사할 공급원 SID다.</param>
    /// <param name="centerX">대상 중심 X다.</param>
    /// <param name="centerY">대상 중심 Y다.</param>
    /// <param name="sizeSquared">대상 크기 제곱이다.</param>
    /// <returns>덮으면 참이다.</returns>
    public bool Covers(int provider, float centerX, float centerY, float sizeSquared)
    {
        ReadOnlySpan<byte> raw = Raw(provider);
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(raw);
        uint entry = _hooks.ProviderEntry is { } providerEntry ? providerEntry(table) : throw new InvalidOperationException("에너지공급가상진입점미연결");
        if (entry == 0x41c0f0) return false; if (entry != 0x41c170) throw new InvalidOperationException("에너지공급가상a0미복원");
        OriginalTypeRecord type = Type(raw[10]);
        float dx = (float)((double)Coordinate(raw, 14) - type.FootX * 0.5) - centerX;
        double dy = (double)Coordinate(raw, 18) - type.FootY * 0.5 - centerY;
        double distance = dy * dy + (double)dx * dx;
        return distance < (double)_state.RadiusSquared + sizeSquared;
    }

    /// <summary>공급량은 선택 목록의 용량과 무관하게 집계하며 문자열은0..5 원소만254개까지 출력한다.</summary>
    /// <param name="x">배치 X다.</param>
    /// <param name="y">배치 Y다.</param>
    /// <param name="type">배치할 TYPE 번호다.</param>
    /// <param name="owner">배치 소유자다.</param>
    /// <param name="theme">특정 원소 요청이며 -1이면 모두 둔다.</param>
    /// <returns>공급 문자다.</returns>
    public string Collect(float x, float y, uint type, uint owner, int theme = -1)
    {
        float[] bounds = Bounds(type, x, y);
        float centerX = (float)(((double)bounds[0] + bounds[2]) * 0.5), centerY = (float)(((double)bounds[1] + bounds[3]) * 0.5);
        const float diagonal = 0.7071067690849304f;
        float size = (float)((double)(bounds[2] - bounds[0]) * diagonal); float square = size * size;
        uint flags = Type(type).Flags2; uint[] counts = new uint[8]; _selected.Count = 0;
        ReadOnlySpan<uint> items = _providers.Items();
        // 현재 공급 목록의 count/항목과 현재 소유자/동맹을 각 반복에서 읽는다.
        for (uint index = 0; index < (uint)items.Length; index++)
        {
            uint value = items[(int)index];
            if (value == 0 || value >= (uint)_pool.Capacity || value > 65535) throw new ArgumentOutOfRangeException(nameof(index), "에너지공급목록DWORD범위오류");
            int sid = unchecked((int)value); ReadOnlySpan<byte> raw = Raw(sid); uint sourceOwner = raw[34];
            bool allowed = Read(_state.Editor, _reads.Editor) != 0 || sourceOwner == owner;
            if (!allowed && _state.Allied != 0)
            {
                if (sourceOwner >= 9 || owner >= 9) throw new ArgumentOutOfRangeException(nameof(owner), "에너지동맹소유자범위오류");
                allowed = _alliances[(int)(sourceOwner * 9 + owner)] != 0;
            }
            if (!allowed || !Covers(sid, centerX, centerY, square)) continue;
            int kind = Type(raw[10]).Theme; if (kind < 0 || kind >= 8) throw new ArgumentOutOfRangeException(nameof(kind), "에너지공급원소카운터범위오류");
            counts[kind]++;
            if (_selected.Count < _selected.Capacity) { _selected.Storage[checked((int)_selected.Count)] = unchecked((uint)sid); _selected.Count++; }
        }
        var output = new System.Text.StringBuilder();
        // 특정 원소 요청은 그것을 먼저 쓰고 다른 모든0..5 원소를 범용s로 출력한다.
        if (theme != -1)
        {
            if (theme < 0 || theme >= 8) throw new ArgumentOutOfRangeException(nameof(theme), "에너지수집원소범위오류");
            output.Append(Letter(theme), (int)Math.Min(counts[theme], 254u)); counts[theme] = 0;
        }
        // 원본 반복 상한은6이며6/7 공급원은 목록에는 남지만 문구에는 없다.
        for (int index = 0; index < 6; index++)
            output.Append(theme == -1 ? Letter(index) : 's', (int)Math.Min(counts[index], (uint)Math.Max(0, 254 - output.Length)));
        return output.ToString();
    }

    /// <summary>일반 원소는 같은 문자만, 범용s는 s→다른 비점 원소→대체 원소 순서로 소비한다.</summary>
    /// <param name="available">수집 문자열이다. 소비한 문자는 점으로 바뀌어 돌려준다.</param>
    /// <param name="required">요구 문자열이다.</param>
    /// <param name="remainder">남은 원본 공급 문자열이다.</param>
    /// <param name="theme">대체 원소 번호이며 -1이면 s다.</param>
    /// <returns>모두 대응하면 1이다.</returns>
    public uint Match(ref string available, string required, out string remainder, int theme = -1)
    {
        ArgumentNullException.ThrowIfNull(available); ArgumentNullException.ThrowIfNull(required);
        string free = CString(available); string wants = CString(required);
        if (free.Length > 255) throw new ArgumentOutOfRangeException(nameof(available), "에너지원본256바이트임시문자열범위오류");
        char substitute = theme == -1 ? 's' : Letter(theme);
        var result = new System.Text.StringBuilder(); uint success = 1;
        // 이미 사용한 문자는 점으로 바꾸어 다음 요구가 같은 공급원을 재사용하지 못하게 한다.
        foreach (char wanted in wants)
        {
            if (wanted != 's' && wanted != 'w' && wanted != 'r' && wanted != 't') throw new InvalidOperationException("에너지원본요구문자진단");
            int found = free.IndexOf(wanted);
            if (wanted == 's' && found < 0)
            {
                found = free.IndexOfAnyExcept('.', substitute);
                if (found < 0) found = free.IndexOf(substitute);
            }
            if (found < 0) { result.Append(wanted); success = 0; }
            else
            {
                char[] chars = free.ToCharArray(); chars[found] = '.'; free = new string(chars);
                result.Append((char)(wanted - 'a' + 'A'));
            }
        }
        if (Override()) success = 1;
        available = result.ToString(); remainder = free; return success;
    }

    /// <summary>우회 경로는 출력/공급 목록을 건드리지 않는 원문 조기 반환이다.</summary>
    /// <param name="x">배치 X다.</param>
    /// <param name="y">배치 Y다.</param>
    /// <param name="type">배치할 TYPE 번호다.</param>
    /// <param name="owner">배치 소유자다.</param>
    /// <param name="output">대응 성공 시 수집 문자열이다.</param>
    /// <returns>빈 요구는 1, 나머지는 대응 성공 시 수집 문자열의 원래 길이다.</returns>
    public uint Evaluate(float x, float y, uint type, uint owner, out string output)
    {
        if (Override()) { output = ""; return 1; }
        string collected = Collect(x, y, type, owner); uint length = unchecked((uint)collected.Length);
        string required = _hooks.Requirement is { } requirement ? requirement(type) : throw new InvalidOperationException("에너지TYPE요구문자열미연결");
        if (required.Length == 0 || required[0] == '\0') { output = ""; return 1; }
        string rest;
        output = collected;
        return Match(ref output, required, out rest) != 0 ? length : 0;
    }

    /// <summary>Builder가 같은 목록을 순서대로 소비하게 한다.</summary>
    /// <returns>첫 비영 DWORD며 없으면 0이다. 목록의 count를 줄이지 않는다.</returns>
    public uint TakeSource()
    {
        if (unchecked((int)_selected.Count) <= 0) return 0;
        Span<uint> entries = _selected.Storage;
        for (uint index = 0; index < _selected.Count; index++)
        {
            ref uint value = ref entries[(int)index];
            if (value != 0) { uint result = value; value = 0; return result; }
        }
        return 0;
    }

    /// <summary>41cfc0의 제곱근 거리와float 임계값 저장을 사용한다. 원소/중심으로 후보를 다시 거르지 않는다.</summary>
    /// <param name="demand">요구 문자열이다. 문자 내용은 사용하지 않으며 첫NUL 또는 첫 미선택에서 끝난다.</param>
    /// <param name="x">기준 X다.</param>
    /// <param name="y">기준 Y다.</param>
    public void SortSources(string demand, float x, float y)
    {
        ArgumentNullException.ThrowIfNull(demand);
        uint[] kept = new uint[RawEnergySupplyLimits.SortKept]; int keptCount = 0;
        void Report(EnergySortDiagnostic diagnostic)
        {
            if (_hooks.SortDiagnostic is null) throw new InvalidOperationException("에너지정렬원본진단");
            _hooks.SortDiagnostic(diagnostic);
        }
        // 문자 내용은 사용하지 않으며 첫NUL 또는 첫 미선택에서 끝난다.
        foreach (char letter in demand)
        {
            if (letter == '\0') break;
            int best = -1; float distanceLimit = 999f;
            // 선택/진단 뒤에도 현재signed count와 현재 슬롯을 읽는다.
            for (int index = 0; index < unchecked((int)_selected.Count); index++)
            {
                uint value = _selected.Storage[index]; if (value == 0) continue;
                bool valid = value >= 1 && value < (uint)_pool.Capacity && value <= 65535 && (_pool.Slot(unchecked((int)value))[11] & 1) == 0;
                if (!valid) Report(EnergySortDiagnostic.InvalidSource);
                if (!valid) continue;
                ReadOnlySpan<byte> raw = _pool.Slot(unchecked((int)value));
                double dx = (double)x - Coordinate(raw, 14), dy = (double)y - Coordinate(raw, 18);
                double distance = Math.Sqrt(dx * dx + dy * dy);
                if (distance < distanceLimit) { distanceLimit = (float)distance; best = index; }
            }
            if (best == -1) break;
            if (best < 0 || best > unchecked((int)_selected.Count)) Report(EnergySortDiagnostic.InvalidBestIndex);
            if (best >= 0 && best <= unchecked((int)_selected.Count))
            {
                if (keptCount < kept.Length) kept[keptCount++] = _selected.Storage[best];
                _selected.Storage[best] = 0;
            }
        }
        _selected.Count = 0;
        // 임시 목록을 현재 용량까지만 앞에 쓰고 선택하지 않은 꼬리는 지우지 않는다.
        for (int index = 0; index < keptCount; index++)
            if (_selected.Count < _selected.Capacity) { _selected.Storage[checked((int)_selected.Count)] = kept[index]; _selected.Count++; }
    }

    /// <summary>연결 시 단일 풀의 정체성을 확인한다.</summary>
    /// <returns>같은 풀이다.</returns>
    public RawSidPool Pool => _pool;
}
