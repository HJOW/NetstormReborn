// 사제 AI의 다음 건설 TYPE와 수집자 여유 비용/적 자금 조건을 같은 본체에서 판단한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 건설 선택 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.

// 원본 전역의 현재 저장소/조회기를 빌린다.
public sealed class AiBuildChoiceGlobals
{
    /// <summary>00540c70의 현재 로컬 소유자다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    public Func<uint>? LocalOwner { get; set; }

    /// <summary>00595344가 0이면 분산 SP를 읽지 않고 원본 0.0f를 쓴다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    public Func<uint>? PlayerKnown { get; set; }

    /// <summary>0054126c의 현재 신전 생산 TYPE이며 초기값 133이다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    public Func<uint>? CollectorType { get; set; }

    /// <summary>005412ec의 현재 다이스 TYPE다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    public Func<uint>? DaisType { get; set; }

    /// <summary>004709d0의 현재 0..3 워크샾 TYPE다. 같은 배열을 빌리며 복사하지 않는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    public IList<uint>? Workshops { get; set; }

    /// <summary>00470a30의 remainder 0..2에 대응하는 현재 541244/248/24c다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    public Func<uint, uint>? Temple { get; set; }
}

// 기존 실제 사제 상태/비용/지갑/SP와 원본 진단만 요청한다.
public sealed class AiBuildChoiceHooks
{
    /// <summary>00492340의 현재 참여/ownerState1이다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    public Func<uint, bool>? Captured { get; set; }

    /// <summary>004922e0의 현재 참여/ownerState2다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    public Func<uint, bool>? Dead { get; set; }

    /// <summary>다이스 관문의 같은 00410280이다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    public Func<uint, uint, bool>? CanAfford { get; set; }

    /// <summary>00413c20의 현재 Player+b0에서 실제 AI 2d8이다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    public Func<uint, int>? Wallet { get; set; }

    /// <summary>0040ec50의 현재 분산 SP이며 지갑 배열로 대체하지 않는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    public Func<uint, float>? StoredSp { get; set; }

    /// <summary>Isleinfo.cpp의 복귀 가능한 원본 진단/줄 번호다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    public Action<string, uint>? Assertion { get; set; }
}

// 독립 구매 추천/수량표 없이 원본 지갑/선호 목록/현재 TYPE 비용을 읽는다.
// 근거: cpppj o/RawAiBuildChoice.h/.cpp의 RawAiBuildChoice다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.
// cpp의 RiftTypeRecord는 DN의 OriginalTypeRecord로 받는다(cost만 쓴다).
public sealed class RawAiBuildChoice
{
    /// <summary>00411ad0의 signed 지갑/SP 절삭값 경계다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    private const int LowFunds = 1000;

    /// <summary>PE00500fc4의 로컬 소유자 미확정 SP 대체값이다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    private const float UnknownSp = 0.0f;

    /// <summary>다른 AI와 같은 본체/목록 소유자다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>현재 cost를 읽는 같은 188 TYPE다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>복사 상수가 아닌 현재 전역 참조/조회기다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    private readonly AiBuildChoiceGlobals _globals;

    /// <summary>선택된 실제 하위와 진단 효과다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    private readonly AiBuildChoiceHooks _hooks;

    /// <summary>생성은 비용/사제 상태/자금을 읽지 않는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    /// <param name="memory">다른 AI와 같은 본체/목록 소유자다.</param>
    /// <param name="types">현재 cost를 읽는 같은 188 TYPE다.</param>
    /// <param name="globals">복사 상수가 아닌 현재 전역 참조/조회기다.</param>
    /// <param name="hooks">선택된 실제 하위와 진단 효과다.</param>
    public RawAiBuildChoice(RawAiBrainMemory memory, IReadOnlyList<OriginalTypeRecord> types, AiBuildChoiceGlobals globals, AiBuildChoiceHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count != 188 || globals.Workshops == null || globals.Workshops.Count != 4)
        {
            throw new ArgumentException("AI 건설 선택 실제188TYPE/워크샾4 누락");
        }

        _memory = memory;
        _types = types;
        _globals = globals;
        _hooks = hooks;
    }

    /// <summary>해제/가짜 본체를 거부한다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>현재 살아 있는 실제 0xb08 본체다.</returns>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 건설 선택 실제0xb08본체 누락");
        }

        return raw;
    }

    /// <summary>비정렬 원문 DWORD를 물리 범위 안에서 읽는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(byte[] raw, long at)
    {
        if (at < 0 || (ulong)at + 4 > (ulong)raw.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 건설 선택 DWORD 범위");
        }

        // 지정 네 BYTE만 합치는 반복문이다.
        return BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan((int)at, 4));
    }

    /// <summary>하위 효과 뒤 현재 필드를 다시 읽는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="at">읽을 DWORD 위치다.</param>
    /// <returns>현재 본체 DWORD다.</returns>
    private uint Field(uint token, long at) => Read(Body(token), at);

    /// <summary>원본 DWORD의 signed 비교 해석이다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    /// <returns>signed 해석이다.</returns>
    private static int Signed(uint value) => unchecked((int)value);

    /// <summary>원본 ftol의 signed64 절삭 하위 DWORD이며 무효 FISTP는 하위 0이다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    /// <param name="value">절삭할 값이다.</param>
    /// <returns>절삭 하위 DWORD다.</returns>
    private static uint Ftol(double value)
    {
        if (!double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            return 0;
        }

        return unchecked((uint)(long)Math.Truncate(value));
    }

    /// <summary>선택된 하위 누락을 가짜 구매 허용/기본 TYPE로 대신하지 않는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 건설 선택 실제 하위 미연결");

    /// <summary>같은 현재 전역 TYPE를 읽는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    /// <param name="name">읽을 전역 장치 이름이다.</param>
    /// <param name="function">읽을 전역 장치다.</param>
    /// <returns>현재 전역 값이다.</returns>
    private static uint Global(string name, Func<uint>? function) => Need(function)();

    /// <summary>비용 조회에 종류/소유자 필터를 추가하지 않는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    /// <param name="type">조회할 TYPE 번호다.</param>
    /// <returns>현재 TYPE 기록이다.</returns>
    private OriginalTypeRecord Type(uint type)
    {
        if ((ulong)type >= (ulong)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), "AI 건설 선택 현재TYPE 범위");
        }

        return _types[(int)type];
    }

    /// <summary>원본 imul의 하위 DWORD 곱과 signed 지갑 비교다. 곱셈 overflow를 signed UB로 만들지 않는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>수집자 여유분을 남기면 참이다.</returns>
    public bool ReserveForCollectors(uint token)
    {
        uint cost = Ftol(Type(Global(nameof(_globals.CollectorType), _globals.CollectorType)).Cost);
        uint reserve = unchecked(cost * Field(token, 0x234));
        return Signed(reserve) <= Signed(Field(token, 0x2d8));
    }

    /// <summary>원격은 AI 지갑, 로컬 확정은 분산 SP, 미확정은 0을 사용하며 리소스를 변경하지 않는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>적 자금이 1000 미만이면 참이다.</returns>
    public bool EnemyLowFunds(uint token)
    {
        uint enemy = Field(token, 0x264);
        uint local = Global(nameof(_globals.LocalOwner), _globals.LocalOwner);
        if (enemy != local)
        {
            return Need(_hooks.Wallet)(enemy) < LowFunds;
        }

        float value = Global(nameof(_globals.PlayerKnown), _globals.PlayerKnown) != 0 ? Need(_hooks.StoredSp)(local) : UnknownSp;
        return Signed(Ftol(value)) < LowFunds;
    }

    /// <summary>범위 진단이 반환하면 원본 switch의 default 0으로 계속한다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    /// <param name="theme">조회할 theme 번호다.</param>
    /// <returns>현재 워크샾 TYPE이며 4/5·범위 밖은 0이다.</returns>
    public uint WorkshopType(int theme)
    {
        if (theme < 0 || theme > 5)
        {
            Need(_hooks.Assertion)("theIndex>=0 && theIndex<=(MAX_THEMES)", 0x1a);
        }

        return theme >= 0 && theme < 4 ? Need(_globals.Workshops)[theme] : 0u;
    }

    /// <summary>IDIV 5의 signed 나머지다. 원본 점프표의 5번 칸은 정상 나머지에서 결코 선택되지 않는다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    /// <param name="theme">조회할 theme 번호다.</param>
    /// <returns>현재 신전 TYPE이며 해당 없음은 0이다.</returns>
    public uint TempleType(int theme)
    {
        int index = theme % 5;
        if (index >= 0 && index < 3)
        {
            return Need(_globals.Temple)(unchecked((uint)index));
        }

        Need(_hooks.Assertion)("0", 0x47);
        return 0;
    }

    /// <summary>모든 비용은 현재 TYPE의 원본 cost에서 ftol이며 각 관문 뒤 현재 본체를 재조회한다. 2026-11-25 R4-5b 건설 선택 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>선택한 건설 TYPE이며 없으면 0이다.</returns>
    public uint Select(uint token)
    {
        if ((Field(token, 0x248) & 0x200u) != 0 && Field(token, 0x2a4) == 0
            && Need(_hooks.Captured)(Field(token, 0x264)) && !Need(_hooks.Dead)(Field(token, 0x264))
            && Need(_hooks.CanAfford)(token, Global(nameof(_globals.DaisType), _globals.DaisType))
            && (Field(token, 0x294) != 0 || EnemyLowFunds(token)))
        {
            return Global(nameof(_globals.DaisType), _globals.DaisType);
        }

        if (Field(token, 0x294) == 0 && Field(token, 0x390) != 3)
        {
            uint temple = TempleType(Signed(Field(token, 0x390)));
            uint cost = Ftol(Type(temple).Cost);
            return Signed(Field(token, 0x2d8)) >= Signed(cost) ? temple : 0u;
        }

        uint candidate = Field(token, 0x394);
        if (candidate == 0)
        {
            return 0;
        }

        if (Signed(Field(token, 0x2d8)) < Signed(Ftol(Type(candidate).Cost)) || !ReserveForCollectors(token))
        {
            return 0;
        }

        var counts = new uint[5];
        // 원본은 선호 count가 0이어도 다섯 theme 수량을 먼저 복사하며 theme 4는 TYPE 0의 수량이다.
        for (uint index = 0; index < (uint)counts.Length; index++)
        {
            uint type = WorkshopType(unchecked((int)index));
            counts[index] = Field(token, 0x6d4L + (long)type * 4L);
        }

        int count = Signed(Field(token, 0x3b0));
        // 선호 중복마다 같은 지역 수량을 감소시켜 첫 음수의 현재 선호 TYPE를 반환하는 반복문이다. 실제 수량표는 쓰지 않는다.
        for (int index = 0; index < count; index++)
        {
            long at = 0x398L + (long)index * 4L;
            uint theme = Field(token, at);
            if (theme >= (uint)counts.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(token), "AI 건설 선택 원본임시수량 인덱스 범위");
            }

            counts[theme]--;
            if (Signed(counts[theme]) < 0)
            {
                return WorkshopType(Signed(Field(token, at)));
            }
        }

        return 0;
    }
}
