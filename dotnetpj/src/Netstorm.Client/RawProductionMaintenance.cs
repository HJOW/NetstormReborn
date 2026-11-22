using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;

namespace Netstorm.Client; // 실제 같은 내용물의 공급·품질·수량·비용 전환을 유지한다.

/// <summary>개별 패널/미션보다 오래 사는 원문 생산 전역이다.</summary>
public sealed class ProductionMaintenanceGlobals
{
    /// <summary>557d80의 TYPE별 벽시계이며 최초 한 번0으로 준비한다.</summary>
    public List<double> StockUntil { get; } = [];
    /// <summary>557dac의 감기는 공급 순서 DWORD다.</summary>
    public uint Sequence { get; set; }
    /// <summary>5318f4의 현재 수량 모드다.</summary>
    public uint StockEnabled { get; set; } = 1;
    /// <summary>54db78의 현재 빠른 다리 모드다.</summary>
    public uint QuickBridges { get; set; }
    /// <summary>54db7c의 강제 패턴이며0은 해제다.</summary>
    public uint ForcedPattern { get; set; }
    /// <summary>5411a0의 현재 다리 TYPE다.</summary>
    public uint BridgeType { get; set; } = 82;
    /// <summary>557d7c의 현재 생산 부모 serial이다.</summary>
    public uint RootSerial { get; set; }
    /// <summary>557d74의 현재 예산 serial이다.</summary>
    public uint BudgetSerial { get; set; }
    /// <summary>557d70의 실제 정책 첫 인자다.</summary>
    public uint PolicyFirst { get; set; }
    /// <summary>557d68의 실제 정책 BYTE 비교 원문이다.</summary>
    public uint PolicyByte { get; set; }
    /// <summary>5ca920의 현재 배경 TYPE다.</summary>
    public uint BackgroundType { get; set; }
    /// <summary>5ca924의 현재 배경 프레임이다.</summary>
    public uint BackgroundFrame { get; set; }
    /// <summary>5ca928의 최초 패널 프레임2다.</summary>
    public uint PanelFrame { get; set; } = 2;
    /// <summary>5ca92c의 수신 Chat 안내 serial이다.</summary>
    public uint ChatAlertSerial { get; set; }
    /// <summary>54db74의 최초 재고 상한1이며0은 상한 없음이다.</summary>
    public int StockLimit { get; set; } = 1;
    /// <summary>531b2c의 현재 공급 지연이다.</summary>
    public int BridgeDelay { get; set; } = 1;
    /// <summary>원본 분석 결과가 아닌 테스트 전용 임시 지정 순서다.</summary>
    public List<uint> TestBridgePatterns { get; } = [];
    /// <summary>테스트 전용 실제 새 조각 공급 횟수다.</summary>
    public ulong TestBridgeDraws { get; set; }
    /// <summary>테스트 전용: 전체 입력을 검증한 뒤 지정 순서/소비 횟수를 한 번에 교체한다.</summary>
    public void ConfigureTestBridgePatterns(string text)
    {
        ArgumentNullException.ThrowIfNull(text); var parsed = new List<uint>();
        if (text.Length != 0 && text != "off")
        {
            // ASCII 공백만 제거하며 빈 항목/부호/0~25 밖 값은 원문처럼 거부한다.
            foreach (string part in text.Split(',')) { string field = part.Trim(' '); if (!uint.TryParse(field, NumberStyles.None, CultureInfo.InvariantCulture, out uint value) || value >= 26) throw new ArgumentException("테스트 다리 조각은0~25의 쉼표 목록 또는 off여야 합니다", nameof(text)); parsed.Add(value); }
        }
        TestBridgePatterns.Clear(); TestBridgePatterns.AddRange(parsed); TestBridgeDraws = 0;
    }
    /// <summary>테스트 전용:0도 지정값이며 실제 공급할 때만 다음 순서를 소비한다.</summary>
    public uint? NextTestBridgePattern() { if (TestBridgePatterns.Count == 0) return null; uint value = TestBridgePatterns[(int)(TestBridgeDraws % (ulong)TestBridgePatterns.Count)]; TestBridgeDraws = unchecked(TestBridgeDraws + 1); return value; }
}

/// <summary>같은 CombatGump 파생에 붙이는 단일 현재 유지 상태다.</summary>
public sealed class ProductionMaintenancePanel
{
    /// <summary>재그리기와 실제 새 부모 생성을 구별하는 수명 표식이다.</summary>
    public bool Initialized { get; set; }
    /// <summary>+c8의 벽시계 공급 기한이다.</summary>
    public double BridgeUntil { get; set; }
    /// <summary>+d8의 절삭한 이전 SP다.</summary>
    public int OldMoney { get; set; }
    /// <summary>+e0의 실제 다리 슬롯 상한이다.</summary>
    public int Slots { get; set; }
    /// <summary>+dc의 이전 템플릿 소유자 신전 SID다.</summary>
    public uint Temple { get; set; }
}

/// <summary>현재 실제 Client/Player/설정을 각 원문 소비 위치에서 읽는다.</summary>
public sealed class ProductionMaintenanceReads
{
    /// <summary>fort/battle이며 challenge가 아닌 실제 관문이다.</summary>
    public required Func<bool> Active { get; init; }
    /// <summary>현재 로컬 기술 SID다.</summary>
    public required Func<uint> Technology { get; init; }
    /// <summary>현재 템플릿 기술 SID다.</summary>
    public required Func<uint> TemplateTechnology { get; init; }
    /// <summary>현재 템플릿 신전 SID다.</summary>
    public required Func<uint> Temple { get; init; }
    /// <summary>현재 편집 DWORD다.</summary>
    public required Func<uint> Editor { get; init; }
    /// <summary>현재 generatorTimer 설정7DWORD다.</summary>
    public required Func<uint> GeneratorTimer { get; init; }
    /// <summary>현재 옵션BYTE2의 재고 속도다.</summary>
    public required Func<uint> StockRate { get; init; }
    /// <summary>부모 owner BYTE와 같은 Player+b0의 AI 여부다.</summary>
    public required Func<uint, bool> Artificial { get; init; }
}

/// <summary>같은 Session의 실제 생성/삭제/조회와 같은 패널의 갱신이다.</summary>
public sealed class ProductionMaintenanceHooks
{
    /// <summary>460d70의 현재 벽시계 초다.</summary>
    public required Func<double> WallSeconds { get; init; }
    /// <summary>55b4d0의 현재 실제 게임 시각이다.</summary>
    public required Func<double> GameSeconds { get; init; }
    /// <summary>현재 분산 SP 또는 known0의 원문 float0이다.</summary>
    public required Func<float> Money { get; init; }
    /// <summary>공급 판정 전에 실제 Deck getter를 실행한다.</summary>
    public required Action TouchDeck { get; init; }
    /// <summary>실제 게임 난수10000과4257c0 패턴 선택이다.</summary>
    public required Func<uint> Pattern { get; init; }
    /// <summary>실제 부모/kind/quantity/flags/attach/quality 생성·부착이다.</summary>
    public required Action<uint, uint, uint, uint, uint, uint> Create { get; init; }
    /// <summary>같은 내용물의 가상 삭제다.</summary>
    public required Action<uint> Destroy { get; init; }
    /// <summary>현재 실제 종류 허용 여부다.</summary>
    public required Func<uint, bool> Allowed { get; init; }
    /// <summary>현재 비용표의 signed 복호 값이다.</summary>
    public required Func<uint, int> Cost { get; init; }
    /// <summary>bit0 텔레포트/bit1 다리의 실제 Push→가상 부착이다.</summary>
    public required Action<uint> Refresh { get; init; }
}

/// <summary>Source43ef10의 두 시계·현재 순회/삭제·품질/수량·비용 전환 전체다.</summary>
public sealed class RawProductionMaintenance
{
    /// <summary>실제 유일한 내용물 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>부모와 같은 TYPE 수다.</summary>
    private readonly int _typeCount;
    /// <summary>같은 현재 목록 순회 정책이다.</summary>
    private readonly ContainedFinderState _finder;
    /// <summary>같은 장기 전역이다.</summary>
    private readonly ProductionMaintenanceGlobals _globals;
    /// <summary>같은 현재 부모 파생의 유지 필드다.</summary>
    private readonly ProductionMaintenancePanel _panel;
    /// <summary>현재 실제 읽기 장치다.</summary>
    private readonly ProductionMaintenanceReads _reads;
    /// <summary>현재 실제 효과 장치다.</summary>
    private readonly ProductionMaintenanceHooks _hooks;
    /// <summary>원본 옵션BYTE2가 고르는 초 간격이다.</summary>
    private static readonly double[] StockIntervals = [30, 15, 8];
    /// <summary>동일 물리/패널/전역을 빌리며 실제 전체 연결이 있어야 생성한다.</summary>
    public RawProductionMaintenance(RawSidPool pool, int typeCount, ContainedFinderState finder, ProductionMaintenanceGlobals globals, ProductionMaintenancePanel panel, ProductionMaintenanceReads reads, ProductionMaintenanceHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(finder); ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(panel); ArgumentNullException.ThrowIfNull(reads); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || typeCount < 0 || reads.Active is null || reads.Technology is null || reads.TemplateTechnology is null || reads.Temple is null || reads.Editor is null || reads.GeneratorTimer is null || reads.StockRate is null || reads.Artificial is null || hooks.WallSeconds is null || hooks.GameSeconds is null || hooks.Money is null || hooks.TouchDeck is null || hooks.Pattern is null || hooks.Create is null || hooks.Destroy is null || hooks.Allowed is null || hooks.Cost is null || hooks.Refresh is null) throw new ArgumentException("생산 유지의 실제 연결 누락");
        _pool = pool; _typeCount = typeCount; _finder = finder; _globals = globals; _panel = panel; _reads = reads; _hooks = hooks;
    }
    /// <summary>부모0도 허용하며 실제 물리 범위를 확인한 뒤 WORD로 좁힌다.</summary>
    private int Parent(uint value) => value < _pool.Capacity ? unchecked((ushort)value) : throw new ArgumentOutOfRangeException(nameof(value), "생산 유지 현재 부모 범위");
    /// <summary>현재 비정렬 WORD다.</summary>
    private static uint Word(ReadOnlySpan<byte> raw, int offset) => BinaryPrimitives.ReadUInt16LittleEndian(raw[offset..]);
    /// <summary>원본 CRT signed64 절삭의 하위DWORD다.</summary>
    private static int Truncate(double value) => !double.IsFinite(value) || value >= 9223372036854775808.0 || value < -9223372036854775808.0 ? 0 : unchecked((int)(long)value);
    /// <summary>현재 관문 뒤 같은 목록/필드를 원문 위치에서만 변경한다.</summary>
    public void Run()
    {
        if (!_reads.Active() || _reads.Technology() == 0) return;
        if (_globals.StockUntil.Count == 0) _globals.StockUntil.AddRange(new double[_typeCount]); if (_globals.StockUntil.Count != _typeCount) throw new InvalidOperationException("생산 TYPE별 시계 크기 변경");
        int parent = Parent(_reads.TemplateTechnology()); var count = new RawContainedFinder(_pool, _finder); int total = 0, single = 0;
        // 삭제 전 flags40과 pattern0의 현재 개수를 따로 센다.
        for (int sid = count.Begin(parent); sid != 0; sid = count.Next()) { var raw = _pool.Slot(sid); if ((Word(raw, 20) & 0x40) != 0) { total = unchecked(total + 1); if (Word(raw, 12) == 0) single = unchecked(single + 1); } }
        bool room = total < _panel.Slots;
        if (_reads.Editor() == 0 && _panel.Temple != 0 && _reads.Temple() == 0)
        {
            var remove = new RawContainedFinder(_pool, _finder);
            // 신전 소실에서는 현재 삭제 뒤 검색기의 next를 읽는다.
            for (int sid = remove.Begin(Parent(_reads.Technology())); sid != 0; sid = remove.Next()) if ((Word(_pool.Slot(sid), 20) & 0x40) != 0) _hooks.Destroy((uint)sid);
            _panel.Temple = 0;
        }
        _panel.Temple = _reads.Temple(); _hooks.TouchDeck();
        if (!_reads.Artificial((uint)parent) && (_reads.Editor() != 0 || _reads.Temple() != 0) && (_panel.BridgeUntil <= _hooks.WallSeconds() || _reads.GeneratorTimer() == 0))
        {
            _panel.BridgeUntil = _hooks.WallSeconds() + (float)_globals.BridgeDelay;
            if (room) { uint pattern = _hooks.Pattern(); int before = unchecked((int)_globals.Sequence); _globals.Sequence = unchecked(_globals.Sequence + 1); if (before % 5 == 0 && single == 0) pattern = 0; if (_globals.ForcedPattern != 0) pattern = _globals.ForcedPattern; if (_globals.NextTestBridgePattern() is { } specified) pattern = specified; _hooks.Create((uint)parent, _globals.BridgeType, pattern, 0x40, 0x10, 0); }
        }
        bool teleportChanged = false, bridgeChanged = false; var contents = new RawContainedFinder(_pool, _finder); int current = contents.Begin(Parent(_reads.Technology()));
        // 실제 체인을 순회하며 두 삭제 방식의 next 조회 위치를 구별한다.
        while (current != 0)
        {
            var raw = _pool.AllocatedBytes(current);
            if ((Word(raw, 20) & 0x40) != 0 && _reads.Editor() == 0 && unchecked((int)BinaryPrimitives.ReadUInt32LittleEndian(raw[31..])) <= Truncate(_hooks.GameSeconds() * 10.0) && (Word(raw, 20) & 0x100) == 0)
            {
                if (_globals.QuickBridges != 0) { if (raw[30] == 0) { raw[30] = 2; bridgeChanged = true; } }
                else if (raw[30] == 0) { raw[30] = 1; uint until = unchecked(60U - (uint)Truncate(_hooks.GameSeconds() * -10.0)); BinaryPrimitives.WriteUInt32LittleEndian(raw[31..], until); bridgeChanged = true; }
                else if (raw[30] == 1) { raw[30] = 2; bridgeChanged = true; }
            }
            raw = _pool.AllocatedBytes(current);
            if ((Word(raw, 20) & 8) != 0)
            {
                uint kind = Word(raw, 18); if (kind >= _typeCount) throw new ArgumentOutOfRangeException(nameof(kind), "생산 유지 내용물 종류 범위");
                if (_globals.StockEnabled != 0 && _globals.StockUntil[(int)kind] <= _hooks.WallSeconds())
                {
                    int next = raw[30] + 1; if (_globals.StockLimit != 0 && _globals.StockLimit < next) next = _globals.StockLimit; raw[30] = unchecked((byte)next); uint rate = _reads.StockRate(); if (rate >= StockIntervals.Length) throw new ArgumentOutOfRangeException(nameof(rate), "생산 재고 옵션 범위"); _globals.StockUntil[(int)kind] = StockIntervals[rate] + _hooks.WallSeconds(); if (_reads.Editor() != 0) _globals.StockUntil[(int)kind] = _hooks.WallSeconds(); teleportChanged = true;
                }
                float money = _hooks.Money();
                if ((double)money != _panel.OldMoney)
                {
                    if (!_hooks.Allowed(kind)) { int removed = current; current = contents.Next(); _hooks.Destroy((uint)removed); continue; }
                    int cost = _hooks.Cost(kind); if ((cost <= _panel.OldMoney) != ((float)cost <= _hooks.Money())) teleportChanged = true;
                }
            }
            current = contents.Next();
        }
        _panel.OldMoney = Truncate(_hooks.Money()); if (teleportChanged) _hooks.Refresh(1); if (bridgeChanged) _hooks.Refresh(2);
    }
}
